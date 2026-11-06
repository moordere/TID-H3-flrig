// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
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

#include "yaesu/FT890.h"

const char FT890name_[] = "FT-890";

enum {FT_LSB, FT_USB, FT_CWW, FT_CWN, FT_AMW, FT_AMN, FT_FM};

static std::vector<std::string>FT890modes_;
static const char *vFT890modes_[] = {
		"LSB", "USB", "CW-W", "CW-N", "AM-W", "AM-N", "FM" };
static const int FT890_mode_val[] =  { 0, 1, 2, 3, 4, 5, 6 };

static const char FT890_mode_type[] = { 'L', 'U', 'U', 'U', 'U' };

static std::vector<std::string>FT890widths_;
static const char *vFT890widths_[] = { "wide", "narr"};

static const int FT890_bw_val[] =
{ 0, 1 };

RIG_FT890::RIG_FT890() {
	name_ = FT890name_;
	modes_ = FT890modes_;
	bandwidths_ = FT890widths_;
	bw_vals_ = FT890_bw_val;

	serial_baudrate = BR4800;
	stopbits = 2;
	serial_retries = 2;

	serial_write_delay = 5;
	serial_post_write_delay = 50;

	serial_timeout = 50;
	serial_rtscts = false;
	serial_rtsplus = false;
	serial_dtrplus = true;
	serial_catptt = true;
	serial_rtsptt = false;
	serial_dtrptt = false;

	afreq = bfreq = A.freq = B.freq = 14070000ULL;
	amode = bmode = A.imode = B.imode = 1;
	aBW = bBW = A.iBW = B.iBW = 0;
	precision = 10;

	has_getvfoAorB =
	has_split =
	has_smeter =
	has_power_out =
	has_get_info =
	has_ptt_control =
	has_mode_control = true;

	precision = 10;
	ndigits = 7;

	read_N = 5; // always responds to query with 5 bytes of data
}

void RIG_FT890::init_cmd()
{
	cmd = "00000";
	for (size_t i = 0; i < 5; i++) cmd[i] = 0;
}

void RIG_FT890::initialize()
{
	VECTOR (FT890modes_, vFT890modes_);
	VECTOR (FT890widths_, vFT890widths_);

	modes_ = FT890modes_;
	bandwidths_ = FT890widths_;
	bw_vals_ = FT890_bw_val;
}

void RIG_FT890::selectA()
{
	init_cmd();
	cmd[4] = 0x05;
	sendCommand(cmd);
	showresp(WARN, HEX, "select A", cmd, replystr);
	inuse = onA;
}

void RIG_FT890::selectB()
{
	init_cmd();
	cmd[3] = 0x01;
	cmd[4] = 0x05;
	sendCommand(cmd);
	showresp(WARN, HEX, "select B", cmd, replystr);
	inuse = onB;
}

void RIG_FT890::set_split(bool val)
{
	split = val;
	init_cmd();
	cmd[3] = val ? 0x01 : 0x00;
	cmd[4] = 0x01;
	sendCommand(cmd);
	if (val)
		showresp(INFO, HEX, "set split ON", cmd, replystr);
	else
		showresp(INFO, HEX, "set split OFF", cmd, replystr);
}



bool RIG_FT890::check()
{
	init_cmd();
	cmd[3] = 0x03;
	cmd[4] = 0x10;
	int ret = waitN(18, 100, "check", HEX);
	if (ret >= 18) return true;
	return false;
}

// 18 byte return values
/*

VFO A is 5000.00 MHz on FLRIG
VFO B is .740.00 MHz on the rig
Radio is set to 5.000.00 MHz, the same as FLRIG.

05:37:41.733 : read_smeter()
05:37:41.733 : WriteBuffer:  00 00 00 00 F7
05:37:41.783 : ReadBuffer [0.032000 msec]: 36 36 36 36 F7

05:37:42.203 : read_vfo()
05:37:42.203 : WriteBuffer:  00 00 00 03 10
05:37:42.260 : ReadBuffer [6.529000 msec]: 08 07 A1 20 00 00 03 04 04 01 01 21 10 00 00 03 04 85

05:37:42.260 : vfoA active get vfo A
05:37:42.260 : read_mode vfoA active
05:37:42.260 : read: 0

05:37:42.661 : read_smeter()
05:37:42.661 : WriteBuffer:  00 00 00 00 F7
05:37:42.711 : ReadBuffer [0.033000 msec]: 3C 3C 3C 3C F7

05:37:43.131 : read_vfo()
05:37:43.131 : WriteBuffer:  00 00 00 03 10
05:37:43.188 : ReadBuffer [6.500000 msec]: 08 07 A1 20 00 00 03 04 04 01 01 21 10 00 00 03 04 85

05:37:43.188 : vfoA active get vfo A
05:37:43.189 : read_mode vfoA active
05:37:43.189 : read: 0

05:37:43.589 : read_smeter()
05:37:43.589 : WriteBuffer:  00 00 00 00 F7
05:37:43.640 : ReadBuffer [0.019000 msec]: 3C 3C 3C 3C F7

------------------------------------
VFO A is 5000.00 MHz on FLRIG
VFO B is 2961.92 on FLRIG, .740.00 MHz on the rig.
Radio is set to 5.001.00 MHz, different from FLRIG.

05:50:47.284 : read_vfo()
05:50:47.285 : WriteBuffer:  00 00 00 03 10
05:50:47.340 : ReadBuffer [5.437000 msec]: 08 07 A1 84 00 00 03 04 04 01 01 21 10 00 00 03 04 85

05:50:47.340 : vfoA active get vfo A
05:50:47.341 : read_mode vfoA active
05:50:47.341 : read: 0

05:50:47.741 : read_smeter()
05:50:47.741 : WriteBuffer:  00 00 00 00 F7
05:50:47.791 : ReadBuffer [0.067000 msec]: 42 42 42 42 F7

05:50:48.211 : read_vfo()
05:50:48.211 : WriteBuffer:  00 00 00 03 10
05:50:48.268 : ReadBuffer [6.509000 msec]: 08 07 A1 84 00 00 03 04 04 01 01 21 10 00 00 03 04 85

05:50:48.268 : vfoA active get vfo A
05:50:48.268 : read_mode vfoA active
05:50:48.268 : read: 0

05:50:48.669 : read_smeter()
05:50:48.669 : WriteBuffer:  00 00 00 00 F7
05:50:48.719 : ReadBuffer [0.035000 msec]: 3F 3F 3F 3F F7

05:50:49.139 : read_vfo()
05:50:49.139 : WriteBuffer:  00 00 00 03 10
05:50:49.196 : ReadBuffer [6.537000 msec]: 08 07 A1 84 00 00 03 04 04 01 01 21 10 00 00 03 04 85

05:50:49.196 : vfoA active get vfo A
05:50:49.197 : read_mode vfoA active
05:50:49.197 : read: 0

05:50:49.597 : read_smeter()
05:50:49.597 : WriteBuffer:  00 00 00 00 F7
05:50:49.648 : ReadBuffer [0.037000 msec]: 3E 3E 3E 3E F7

*/

static bool testing = false;

bool RIG_FT890::get_info()
{
	init_cmd();
	cmd[3] = 0x03;
	cmd[4] = 0x10;

	size_t ret;

	if (testing) {
		replystr.clear();
// vfo A
		replystr += '\x08'; replystr += '\x07'; replystr += '\xA1'; replystr += '\x20';
		replystr += '\x00'; replystr += '\x00'; replystr += '\x01'; replystr += '\x04';
		replystr += '\x04';
// vfo B
//		replystr += '\x01'; replystr += '\x01'; replystr += '\x21'; replystr += '\x10';
		replystr += '\x08'; replystr += '\x07'; replystr += '\xA1'; replystr += '\x20';
		replystr += '\x00'; replystr += '\x00'; replystr += '\x03'; replystr += '\x04';
		replystr += '\x85';

		ret = replystr.length();
	} else {
		ret = waitN(18, 100, "get info", HEX);
	}

	if (ret == 18) {

		A.iBW = (replystr[0] & 1);
		A.freq = 10 * ((((replystr[1] & 0xFF) * 256 +
				   (replystr[2] & 0xFF) ) * 256 +
				   (replystr[3] & 0xFF) ) );

		switch (replystr[6] & 0x07) {
			case 0: A.imode = FT_LSB; break;
			case 1: A.imode = FT_USB; break;
			case 2: A.imode = FT_CWW; break;
			case 3: A.imode = FT_AMW; break;
			case 4: A.imode = FT_FM; break;
			default: A.imode = 0;
		}

// After the base mode has been determined, we need to check for the narrow filters.
// We need to verify the base mode, as the narrow mode bits remain on until changed.

		if (((replystr[8] & 0x80) == 0x80) && (A.imode == FT_CWW)) {
			A.imode = FT_CWN;
		}

		if (((replystr[8] & 0x40) == 0x40) && (A.imode == FT_AMW)) {
			A.imode = FT_AMN;
		}

		B.iBW = (replystr[9] & 1);
		B.freq = 10 * ((((replystr[10] & 0xFF) * 256 +
				   (replystr[11] & 0xFF) ) * 256 +
				   (replystr[12] & 0xFF) ) );

		switch (replystr[15] & 0x07) {
			case 0: B.imode = FT_LSB; break;
			case 1: B.imode = FT_USB; break;
			case 2: B.imode = FT_CWW; break;
			case 3: B.imode = FT_AMW; break;
			case 4: B.imode = FT_FM; break;
			default: B.imode = 0;
		}

// After the base mode has been determined, we need to check for the narrow filters.
// We need to verify the base mode, as the narrow mode bits remain on until changed.

		if (((replystr[17] & 0x80) == 0x80) && (B.imode == FT_CWW)) {
			B.imode = FT_CWN;
		}

		if (((replystr[17] & 0x40) == 0x40) && (B.imode == FT_AMW)) {
			B.imode = FT_AMN;
		}

	} else {
		return false;
	}

// read flag bytes
	if (testing) {
		replystr.clear();
		replystr += '\x04'; replystr += '\xFF'; replystr += '\x80'; replystr += '\xFF';
		replystr += '\xFF';
		ret = replystr.length();
	} else {
		init_cmd();
		cmd[4] = 0xFA;
		ret = waitN(5, 100, "get info", HEX);
	}

	ptt_ = false;
	split = false;
	inuse = onA;
	if (ret == 5) {
		if ( (replystr[0] & 0x04) == 0x04 ) split = true;
		if ( (replystr[0] & 0x40) == 0x40 ) inuse = onB;
		if ( (replystr[2] & 0x80) == 0x80 ) ptt_ = true;

	} else {
		return false;
	}

	return true;
}

unsigned long long RIG_FT890::get_vfoA ()
{
	return A.freq;
}

void RIG_FT890::set_vfoA (unsigned long long freq)
{
	A.freq = freq;
	freq /=10; // 890 does not support 1 Hz resolution
	cmd = to_bcd_be(freq, 8);
	cmd += 0x0A;
	sendCommand(cmd);
	showresp(WARN, HEX, "set vfo A", cmd, replystr);
}

int RIG_FT890::get_modeA()
{
	return A.imode;
}

void RIG_FT890::set_modeA(int val)
{
	A.imode = val;
	init_cmd();
	cmd[3] = FT890_mode_val[val];
	cmd[4] = 0x0C;
	sendCommand(cmd);
	showresp(WARN, HEX, "set mode A", cmd, replystr);
}

unsigned long long RIG_FT890::get_vfoB()
{
	return B.freq;
}

void RIG_FT890::set_vfoB(unsigned long long freq)
{
	B.freq = freq;
	freq /=10; // 890 does not support 1 Hz resolution
	cmd = to_bcd_be(freq, 8);
	cmd += 0x0A; // Not 0x8A, as it is the same as vfo-a
	sendCommand(cmd);
	showresp(WARN, HEX, "set vfo B", cmd, replystr);
}

void RIG_FT890::set_modeB(int val)
{
	B.imode = val;
	init_cmd();
	cmd[3] = FT890_mode_val[val];
	cmd[4] = 0x0C;
	sendCommand(cmd);
	showresp(WARN, HEX, "set mode B", cmd, replystr);
}

int  RIG_FT890::get_modeB()
{
	return B.imode;
}

// Tranceiver PTT on/off
void RIG_FT890::set_PTT_control(int val)
{
	init_cmd();
	if (val) cmd[3] = 1;
	else	 cmd[3] = 0;
	cmd[4] = 0x0F;
	sendCommand(cmd, 0);
	LOG_INFO("%s", str2hex(cmd.c_str(), 5));
	ptt_ = val;
}

int RIG_FT890::get_PTT() {
	return ptt_;
}

int RIG_FT890::get_smeter()
{
	init_cmd();
	cmd[4] = 0xF7;
	int ret = waitN(5, 100, "get smeter", HEX);
	if (ret < 5) return 0;
	int sval = (unsigned char)(replystr[ret - 2] & 0xFF);
	sval = round(sval * 100.0 / 240.0);
	return sval;
}

int RIG_FT890::get_power_out()
{
	init_cmd();
	cmd[4] = 0xF7;
	size_t ret;
	if (testing) {
		replystr[0] = 120; replystr[1] = 60; replystr[2] = 60; replystr[3] = 60; 
		replystr[4] = 0xF7;
		ret = replystr.length();
	} else {
		ret = waitN(5, 100, "get pwr out", HEX);
	}
	if (ret < 5) return 0;
	int sval = (unsigned char)(replystr[ret - 5]);
	sval = round(sval * 100.0 / 240.0);
	return sval;
}
