// ----------------------------------------------------------------------------
// Copyright (C) 2026
//
// This file is part of flrig.
//
// flrig is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// flrig is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------
//
// TIDRADIO TD-H3 with nicFW 2.5x firmware by nicsure
//   https://github.com/nicsure/nicFW_H3_2.5
//   https://github.com/nicsure/nicFWRemoteBT
//
// nicFW has no conventional CAT command set.  Its serial port (38400 8N1,
// programming cable) understands these commands (nicfw2.5/uart/uart.c;
// the same dispatch code is present in the V2.52.2x binaries):
//
//   0x01 / 0x02       ping, radio echoes the byte
//   0x30 n            read EEPROM block n (32 bytes at n*32)
//                       reply: 0x30, 32 bytes, additive checksum
//   0x31 n d[32] cs   write EEPROM block n, reply 0x31
//   0x4C 0x4D k       remote key: k is held "pressed" until another key
//                     code is sent; 0xFF = no key.  0x10 = PTT A,
//                     0x11 = PTT B, 0..9 digits, 0x0A MENU, 0x0B UP,
//                     0x0C DOWN, 0x0D EXIT, 0x0E STAR, 0x0F HASH
//
// (uart.c also contains an 0xAA "VFO status" case, but any byte with bit 7
// set is diverted to the remote key handler before the command switch, so
// it can never be reached.  There is therefore no way to read RSSI.)
//
// The MCU polls a single byte receive register, so the first byte of a
// multi byte command is sent alone and the remainder after a short pause,
// once the firmware has entered its receive loop.
//
// Radio state is read from EEPROM.  The firmware writes the active VFO's
// record to its EEPROM slot (validateVfo -> channelSave) whenever it is
// changed, and writes the settings block within a second of a change.
//
// channelInfo record (32 bytes, big endian - Keil C51):
//   0  u32 rx frequency, 10 Hz units
//   4  u32 tx frequency, 10 Hz units
//   8  u16 rx subtone, 10 u16 tx subtone
//   12 u8  tx power, 13 u16 groups
//   15 u8  bits: b0 bandwidth (0 wide, 1 narrow), b1-2 modulation
//                (0 Auto, 1 FM, 2 AM, 3 USB), ...
//   20 char name[12]
// EEPROM slots 0 and 1 hold VFO A and VFO B, channels are slots 2..199.
//
// Settings block at EEPROM 0x1900 (blocks 0xC8..0xCA):
//   0x05 activeVfo
//   0x20 vfoState[2], 19 bytes each:
//          +0 group, +1 lastGroup, +2 groupModeChannels[16], +18 mode
//          (mode 0 = VFO/frequency mode, 1 = channel mode)
//
// Frequency changes are made by emulating the key presses used to enter a
// frequency on the radio's keypad, so the VFO must be in VFO (frequency)
// mode.  Modulation and bandwidth are reported but can only be changed on
// the radio.
// ----------------------------------------------------------------------------

#include <cstring>
#include <cstdio>

#include "other/nicFW_H3.h"
#include "tod_clock.h"

static const char nicFW_H3name_[] = "TD-H3 nicFW";

static std::vector<std::string>nicFW_H3modes_;
static const char *vnicFW_H3modes_[] = {
	"Auto", "FM", "AM", "USB" };

static std::vector<std::string>nicFW_H3_bws_;
static const char *vnicFW_H3_bws_[] = {
	"Wide", "Narrow" };

// key codes understood by the firmware (make.h)
enum {
	NK_0 = 0x00, NK_MENU = 0x0A, NK_UP = 0x0B, NK_DOWN = 0x0C,
	NK_EXIT = 0x0D, NK_STAR = 0x0E, NK_HASH = 0x0F,
	NK_PTTA = 0x10, NK_PTTB = 0x11, NK_NONE = 0xFF
};

// a key must be held for 6 firmware ticks (60 Hz, 100 msec) to latch and
// released for as long again before the press is acted on
static const int KEY_HOLD_MS    = 150;
static const int KEY_RELEASE_MS = 150;
// pause between the first byte of a command and the remainder
static const int CMD_GAP_MS     = 10;
// reuse of the settings block between consecutive poll queries
static const unsigned long long SETTINGS_CACHE_MS = 500;
// the radio writes its settings lazily (up to 1 sec); trust our own idea
// of the active VFO for this long after changing it
static const unsigned long long SELECT_HOLDOFF_MS = 2500;

static const unsigned long long FREQ_MIN_HZ = 18000000ULL;
static const unsigned long long FREQ_MAX_HZ = 1300000000ULL;

static const int SETTINGS_BLOCK = 0x1900 >> 5;   // 0xC8
static const int VFOSTATE_OFS   = 0x20;
static const int VFOSTATE_SIZE  = 19;

// scoped lock for the driver's I/O mutex
class io_lock {
public:
	io_lock(pthread_mutex_t *m) : mtx(m) { pthread_mutex_lock(mtx); }
	~io_lock() { pthread_mutex_unlock(mtx); }
private:
	pthread_mutex_t *mtx;
};

RIG_NICFW_H3::RIG_NICFW_H3() {
	name_ = nicFW_H3name_;
	modes_ = nicFW_H3modes_;
	bandwidths_ = nicFW_H3_bws_;

	serial_baudrate = BR38400;
	stopbits = 1;
	serial_retries = 2;

	serial_write_delay = 0;
	serial_post_write_delay = 0;

	serial_timeout = 200;
	serial_rtscts = false;
	serial_rtsplus = false;
	serial_dtrplus = false;
	serial_catptt = true;
	serial_rtsptt = false;
	serial_dtrptt = false;

	def_freq = A.freq = B.freq = freqA = freqB = 145500000ULL;
	def_mode = A.imode = B.imode = modeA = modeB = 1;
	def_bw = A.iBW = B.iBW = bwA = bwB = 0;

	has_mode_control = true;
	has_bandwidth_control = true;
	has_ptt_control = true;
	has_getvfoAorB = true;
	has_vfoAB = true;
	// set_vfoA/B switch the radio themselves when needed
	can_change_alt_vfo = true;

	precision = 10;
	ndigits = 10;

	memset(&rs, 0, sizeof(rs));
	settings_time = 0;
	select_time = 0;
	radio_vfo = 0;
	want_vfo = 0;
	select_pending = false;
	inuse = onA;

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&io_mutex, &attr);
	pthread_mutexattr_destroy(&attr);
}

void RIG_NICFW_H3::initialize()
{
	VECTOR(nicFW_H3modes_, vnicFW_H3modes_);
	VECTOR(nicFW_H3_bws_, vnicFW_H3_bws_);
	modes_ = nicFW_H3modes_;
	bandwidths_ = nicFW_H3_bws_;
	settings_time = 0;
	select_time = 0;
	select_pending = false;
}

std::vector<std::string> &RIG_NICFW_H3::bwtable(int m)
{
	return nicFW_H3_bws_;
}

//----------------------------------------------------------------------
// low level protocol
//----------------------------------------------------------------------

void RIG_NICFW_H3::decode_vfo(const unsigned char *p, vfo_info &v)
{
	unsigned long rx = ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) |
	                   ((unsigned long)p[2] << 8)  |  (unsigned long)p[3];
	unsigned long tx = ((unsigned long)p[4] << 24) | ((unsigned long)p[5] << 16) |
	                   ((unsigned long)p[6] << 8)  |  (unsigned long)p[7];
	v.rx_hz = (unsigned long long)rx * 10ULL;
	v.tx_hz = (unsigned long long)tx * 10ULL;
	v.bandwidth  = p[15] & 0x01;
	v.modulation = (p[15] >> 1) & 0x03;
	v.valid = (v.rx_hz >= FREQ_MIN_HZ && v.rx_hz <= FREQ_MAX_HZ);
}

// 0x30 n : read 32 byte EEPROM block
bool RIG_NICFW_H3::read_block(int block, unsigned char *data)
{
	io_lock lock(&io_mutex);
	for (int tries = 0; tries <= serial_retries; tries++) {
		cmd.assign(1, '\x30');
		sendCommand(cmd);
		MilliSleep(CMD_GAP_MS);
		cmd.assign(1, (char)(block & 0xFF));
		int ret = waitN(34, 200, "read eeprom", HEX);
		if (ret >= 34 && (unsigned char)replystr[0] == 0x30) {
			unsigned char cs = 0;
			for (int i = 0; i < 32; i++) {
				data[i] = (unsigned char)replystr[i + 1];
				cs += data[i];
			}
			if (cs == (unsigned char)replystr[33])
				return true;
		}
		MilliSleep(50);
	}
	LOG_WARN("nicFW: EEPROM block %02X read failed", block);
	return false;
}

bool RIG_NICFW_H3::read_settings(bool force)
{
	io_lock lock(&io_mutex);
	if (!force && rs.valid && (zmsec() - settings_time) < SETTINGS_CACHE_MS)
		return true;

	unsigned char data[96];
	for (int i = 0; i < 3; i++)
		if (!read_block(SETTINGS_BLOCK + i, data + 32 * i))
			return false;

	rs.active_vfo = data[0x05] ? 1 : 0;
	for (int v = 0; v < 2; v++) {
		const unsigned char *vs = data + VFOSTATE_OFS + v * VFOSTATE_SIZE;
		rs.group[v] = vs[0] & 0x0F;
		rs.mode[v] = vs[18] ? 1 : 0;
		rs.channel[v] = vs[2 + rs.group[v]];
	}
	rs.valid = true;
	settings_time = zmsec();

	if (zmsec() - select_time >= SELECT_HOLDOFF_MS)
		radio_vfo = rs.active_vfo;
	return true;
}

// EEPROM slot holding what the given VFO is showing
int RIG_NICFW_H3::slot_for(int vfo)
{
	vfo &= 1;
	if (rs.valid && rs.mode[vfo] && rs.channel[vfo] >= 2 && rs.channel[vfo] < 200)
		return rs.channel[vfo];
	return vfo;
}

bool RIG_NICFW_H3::read_vfo(int vfo, vfo_info &v)
{
	io_lock lock(&io_mutex);
	unsigned char data[32];
	read_settings();
	if (!read_block(slot_for(vfo), data)) return false;
	memset(&v, 0, sizeof(v));
	decode_vfo(data, v);
	return v.valid;
}

void RIG_NICFW_H3::update_vfo(int vfo)
{
	vfo_info v;
	if (!read_vfo(vfo, v)) return;
	if (vfo == 0) {
		freqA = v.rx_hz; modeA = v.modulation; bwA = v.bandwidth;
	} else {
		freqB = v.rx_hz; modeB = v.modulation; bwB = v.bandwidth;
	}
}

// 0x4C 0x4D k : set the remote key state
void RIG_NICFW_H3::send_key(int key)
{
	io_lock lock(&io_mutex);
	cmd.assign(1, '\x4C');
	sendCommand(cmd);
	MilliSleep(CMD_GAP_MS);
	cmd.assign(1, '\x4D');
	cmd += (char)(key & 0xFF);
	sendCommand(cmd);
}

void RIG_NICFW_H3::press_key(int key)
{
	io_lock lock(&io_mutex);
	send_key(key);
	MilliSleep(KEY_HOLD_MS);
	send_key(NK_NONE);
	MilliSleep(KEY_RELEASE_MS);
}

// The first key press after the backlight has timed out only switches the
// backlight on.  KEY_0 does nothing in VFO mode, or in channel mode without
// a group (with a group selected it would change the group), so it is
// used to wake the keypad when that is safe.
void RIG_NICFW_H3::wake_keypad()
{
	io_lock lock(&io_mutex);
	read_settings();
	int v = radio_vfo & 1;
	if (rs.valid && rs.mode[v] && rs.group[v]) return;
	press_key(NK_0);
}

// Keypad frequency entry on the active VFO in VFO mode: the MHz digits,
// STAR as the decimal point (not needed after 4 MHz digits), then 5
// decimal digits (10 Hz resolution).  The radio applies the entry after
// the last decimal digit.
bool RIG_NICFW_H3::enter_frequency(unsigned long long freq)
{
	if (freq < FREQ_MIN_HZ || freq > FREQ_MAX_HZ) {
		LOG_WARN("nicFW: frequency %llu out of range", freq);
		return false;
	}
	if (ptt_) return false;

	io_lock lock(&io_mutex);
	read_settings(true);
	if (rs.valid && rs.mode[radio_vfo]) {
		LOG_WARN("nicFW: VFO %c is in channel mode, frequency not set",
			radio_vfo ? 'B' : 'A');
		return false;
	}

	unsigned long long f10 = (freq + 5) / 10;
	unsigned long mhz = (unsigned long)(f10 / 100000ULL);
	unsigned long dec = (unsigned long)(f10 % 100000ULL);

	char sz[16];
	std::string keys;
	snprintf(sz, sizeof(sz), "%lu", mhz);
	for (const char *p = sz; *p; p++) keys += (char)(*p - '0');
	if (keys.length() < 4) keys += (char)NK_STAR;
	snprintf(sz, sizeof(sz), "%05lu", dec);
	for (const char *p = sz; *p; p++) keys += (char)(*p - '0');

	wake_keypad();
	for (size_t i = 0; i < keys.length(); i++)
		press_key((unsigned char)keys[i]);

	vfo_info v;
	if (read_vfo(radio_vfo, v) && v.rx_hz == f10 * 10ULL)
		return true;

	LOG_WARN("nicFW: frequency entry not confirmed (wanted %llu, radio %llu)",
		f10 * 10ULL, v.rx_hz);
	return false;
}

// HASH swaps the active VFO
void RIG_NICFW_H3::swap_vfo()
{
	io_lock lock(&io_mutex);
	wake_keypad();
	press_key(NK_HASH);
	radio_vfo = radio_vfo ? 0 : 1;
	select_time = zmsec();
}

//----------------------------------------------------------------------
// flrig interface
//----------------------------------------------------------------------

bool RIG_NICFW_H3::check()
{
	cmd.assign(1, '\x01');
	int ret = waitN(1, 200, "check", HEX);
	if (ret < 1 || replystr[0] != '\x01') return false;
	return read_settings(true);
}

// flrig selects B then A while reading the radio at start up; both VFOs
// can be read without switching, so the radio is only switched (HASH) when
// the selection is still different on the next poll, or before a set/PTT.
void RIG_NICFW_H3::apply_select()
{
	io_lock lock(&io_mutex);
	if (!select_pending) return;
	select_pending = false;
	if (want_vfo != radio_vfo) swap_vfo();
}

int RIG_NICFW_H3::get_vfoAorB()
{
	io_lock lock(&io_mutex);
	if (select_pending)
		apply_select();
	else {
		read_settings();
		want_vfo = radio_vfo;
	}
	inuse = radio_vfo ? onB : onA;
	return inuse;
}

void RIG_NICFW_H3::selectA()
{
	want_vfo = 0;
	select_pending = true;
	inuse = onA;
}

void RIG_NICFW_H3::selectB()
{
	want_vfo = 1;
	select_pending = true;
	inuse = onB;
}

unsigned long long RIG_NICFW_H3::get_vfoA()
{
	if (!ptt_) update_vfo(0);
	return freqA;
}

unsigned long long RIG_NICFW_H3::get_vfoB()
{
	if (!ptt_) update_vfo(1);
	return freqB;
}

void RIG_NICFW_H3::set_vfoA(unsigned long long freq)
{
	io_lock lock(&io_mutex);
	apply_select();
	read_settings(true);
	if (rs.valid && rs.mode[0]) {
		LOG_WARN("nicFW: VFO A is in channel mode, frequency not set");
		return;
	}
	bool swap = (radio_vfo != 0);
	if (swap) swap_vfo();
	if (enter_frequency(freq)) freqA = freq;
	if (swap) swap_vfo();
}

void RIG_NICFW_H3::set_vfoB(unsigned long long freq)
{
	io_lock lock(&io_mutex);
	apply_select();
	read_settings(true);
	if (rs.valid && rs.mode[1]) {
		LOG_WARN("nicFW: VFO B is in channel mode, frequency not set");
		return;
	}
	bool swap = (radio_vfo != 1);
	if (swap) swap_vfo();
	if (enter_frequency(freq)) freqB = freq;
	if (swap) swap_vfo();
}

// Modulation and bandwidth can only be changed from the radio's menu;
// flrig's display follows the radio on the next poll.
void RIG_NICFW_H3::set_modeA(int val)
{
	if (val != modeA)
		LOG_WARN("nicFW: modulation can only be changed on the radio");
}

int RIG_NICFW_H3::get_modeA()
{
	return modeA;
}

void RIG_NICFW_H3::set_modeB(int val)
{
	if (val != modeB)
		LOG_WARN("nicFW: modulation can only be changed on the radio");
}

int RIG_NICFW_H3::get_modeB()
{
	return modeB;
}

void RIG_NICFW_H3::set_bwA(int val)
{
	if (val != bwA)
		LOG_WARN("nicFW: bandwidth can only be changed on the radio");
}

int RIG_NICFW_H3::get_bwA()
{
	return bwA;
}

void RIG_NICFW_H3::set_bwB(int val)
{
	if (val != bwB)
		LOG_WARN("nicFW: bandwidth can only be changed on the radio");
}

int RIG_NICFW_H3::get_bwB()
{
	return bwB;
}

// PTT is emulated by holding the PTT key of the active VFO (holding the
// other VFO's PTT would swap VFOs in dual PTT mode).
void RIG_NICFW_H3::set_PTT_control(int val)
{
	io_lock lock(&io_mutex);
	if (val) {
		apply_select();
		send_key(radio_vfo ? NK_PTTB : NK_PTTA);
		ptt_ = 1;
	} else {
		send_key(NK_NONE);
		ptt_ = 0;
	}
}
