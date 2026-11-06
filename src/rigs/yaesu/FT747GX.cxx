// ----------------------------------------------------------------------------
// Copyright (C) 2024
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

#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "util.h"

#include "yaesu/FT747GX.h"

static const char FT747GXname_[] = "FT-747GX";

static std::vector<std::string>FT747GXmodes_;
static const char *vFT747GXmodes_[] = {
	"LSB", "USB", "CW", "CWN", "AMW", "AMN", "FMW", "FMN"};
static const char FT747_mode_type[] = { 
	'L', 'U', 'U', 'U', 'U', 'U', 'U', 'U' };

RIG_FT747GX::RIG_FT747GX() {
// base class values
	name_ = FT747GXname_;
	modes_ = FT747GXmodes_;

	serial_baudrate = BR4800;
	stopbits = 2;

	serial_retries = 2;

	serloop_timing = 1000;
	serial_timeout = 50;
	serial_write_delay = 50;
	serial_post_write_delay = 50;

	serial_rtscts = false;
	serial_rtsplus = false;
	serial_dtrplus = true;
	serial_catptt = true;
	serial_rtsptt = false;
	serial_dtrptt = false;

	modeA = 1;
	bwA = 0;

	has_split = true;
	has_split_AB = true;
	has_get_info = true;
	has_ptt_control = true;
	has_mode_control = true;

	precision = 10;
	ndigits = 7;

	read_N = 344; // always responds to query with N bytes of data
};

void RIG_FT747GX::initialize()
{
	VECTOR( FT747GXmodes_, vFT747GXmodes_);
	modes_ = FT747GXmodes_;
}

void RIG_FT747GX::init_cmd()
{
	cmd = "00000";
	cmd[0] = cmd[1] = cmd[2] = cmd[3] = cmd[4] = 0x00;
}

void RIG_FT747GX::selectA()
{
	init_cmd();
	cmd[4] = 0x05;
	sendCommand(cmd);
	showresp(INFO, HEX, "select A", cmd, replystr);
	inuse = onA;
}

void RIG_FT747GX::selectB()
{
	init_cmd();
	cmd[3] = 0x01;
	cmd[4] = 0x05;
	sendCommand(cmd);
	showresp(INFO, HEX, "select B", cmd, replystr);
	inuse = onB;
}

static bool split_status = false;

void RIG_FT747GX::set_split(bool val)
{
	split = val;
	init_cmd();
	cmd[3] = val ? 0x01 : 0x00;
	cmd[4] = 0x01;
	sendCommand(cmd);
	split_status = val;
	if (split_status)
		showresp(INFO, HEX, "set split ON", cmd, replystr);
	else
		showresp(INFO, HEX, "set split OFF", cmd, replystr);
}

int RIG_FT747GX::get_split()
{
	return split_status;
}

bool RIG_FT747GX::check()
{
	return get_info();
}

unsigned long long RIG_FT747GX::get_vfoA ()
{
	return A.freq;
}

void RIG_FT747GX::set_vfoA (unsigned long long freq)
{
	A.freq = freq;
	freq /=10; // 747GX does not support 1 Hz resolution
	cmd = to_bcd_be(freq, 8);
	cmd += 0x0A; // SET FREQUENCY
	LOG_INFO("cmd: %s", str2hex(cmd.c_str(), cmd.length()));
	sett("set_vfoA");
	sendCommand(cmd);
	seth();
}

int RIG_FT747GX::get_modeA()
{
	return A.imode;
}

void RIG_FT747GX::set_modeA(int val)
{
	A.imode = val;
	init_cmd();
	cmd[3] = val;
	cmd[4] = 0x0C; // MODESEL
	sett("set_modeA");
	LOG_INFO("cmd: %s", str2hex(cmd.c_str(), cmd.length()));
	sendCommand(cmd);
	seth();
}

unsigned long long RIG_FT747GX::get_vfoB ()
{
	return B.freq;
}

void RIG_FT747GX::set_vfoB (unsigned long long freq)
{
	B.freq = freq;
	freq /=10;
	cmd = to_bcd_be(freq, 8);
	cmd += 0x0A; // SET FREQUENCY
	LOG_INFO("cmd: %s", str2hex(cmd.c_str(), cmd.length()));
	sendCommand(cmd);
}

int RIG_FT747GX::get_modeB()
{
	return B.imode;
}

void RIG_FT747GX::set_modeB(int val)
{
	B.imode = val;
	init_cmd();
	cmd[3] = val;
	cmd[4] = 0x0C; // MODESEL
	LOG_INFO("cmd: %s", str2hex(cmd.c_str(), cmd.length()));
	sendCommand(cmd);
}

void RIG_FT747GX::set_PTT_control(int val)
{
	init_cmd();
	cmd[3] = val ? 0x01 : 0x00;
	cmd[4] = 0x0F;
	sendCommand(cmd);
	ptt_ = val;
}

/*
'UPDATE' returns

Bytes:
  0     : displayed status flags (1)
  1-5   : displayed frequency (2)
  6     : current band (3)
  7     : vfo A status flags (1)
  8-12  : vfo A frequency (2)
  13-14 : N/A
  15    : vfo B status flags (1)
  16-20 : vfo B frequency (2)
  21    : displayed memory number, not used by flrig
  22    : mode (4)
 
  (1) bit 0 : data lock
      bit 1 : split on/off
      bit 2 : clarifier on/off
      bit 3 : vfo A/B; 0 == A
      bit 4 : vfo / memory, 0 == vfo
      bit 5 : TX/RX, 0 == RX
      bit 6 : N/A
      bit 7 : priority monitoring enabled
   
  (2) packed BCD frequency in left to right order, first byte always zero
      14250.00 ==> 0x00 0x14 0x25 0x00 0x00

      byte[n+4} always either x00, x25, x50 or x75

  (3) X1  0.1 - 2.5 MHz
      X2  2.5 - 4.0 MHz
      X3  4.0 - 7.5 MHz
      X4  7.5 - 10.5 MHz
      X5  10.5 - 14.5 MHz
      X6  14.5 - 18.5 MHz
      X7  18.5 - 21.5 MHz
      X8  21.5 - 25.0 MHz
      X9  25.0 - 30.0 MHz

  (4) bit 7 - narrow
      bit 4 - LSB
      bit 3 - USB
      bit 2 - CW
      bit 1 - AM
      bit 0 - FM
*/

static long long int bcdfreq(std::string &str, int n)
{
	long long int val = 0;
	for (int i = 0; i < 4; i++) {
		val *= 10;
		val += ((str[n + i] & 0xF0) >> 4);
		val *= 10;
		val += ((str[n + i] & 0x0F));
	}
	return val;
}

// 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30
// 01 00 14 07 00 00 04 01 14 07 00 00 04 00 00 00 03 50 25 00 02 00 00 00 00 00 00 00 00 00 00

bool RIG_FT747GX::get_info()
{
	unsigned long long afreq = A.freq, bfreq = B.freq;
	int amode = A.imode;
	init_cmd();
	cmd[4] = 0x10;
	int ret = 0;

	gett("get info");

	if (1) {
		ret = waitN(344, 1000, "get info", HEX);
	} else {
		ret = 23;
		replystr = "01234567890123456789012";

		replystr[0] = 0x00;
		replystr[0] |= (split_status ? 0x02 : 0x00);

		replystr[1] = 0x00;
		replystr[2] = 0x14;
		replystr[3] = 0x07;
		replystr[4] = 0x00;
		replystr[5] = 0x00;
		replystr[6] = 0x85;
		replystr[7] = 0x00;
		replystr[8] = 0x00;
		replystr[9] = 0x28;
		replystr[10] = 0x07;
		replystr[11] = 0x40;
		replystr[12] = 0x00;
		replystr[13] = 0x00;
		replystr[14] = 0x00;
		replystr[15] = 0x00;
		replystr[16] = 0x00;
		replystr[17] = 0x03;
		replystr[18] = 0x50;
		replystr[19] = 0x25;
		replystr[20] = 0x00;
		replystr[21] = 0x00;
if (inuse == onA)
		replystr[22] = 0x08; // USB
else
		replystr[22] = 0x84;  // CW narrow
	}

	geth();
//	showresp(INFO, HEX, "get info", cmd, replystr);

	if (ret >= 23) {

		afreq = bcdfreq(replystr, 9);
		bfreq = bcdfreq(replystr, 17);

		int md = replystr[22] & 0xFF;
		switch (md) {
			case 0x01 : amode = 6; // FMW
						break;
			case 0x81 : amode = 7; // FMN
						break;
			case 0x02 : amode = 4; // AMW
						break;
			case 0x82 : amode = 5; // AMN
						break;
			case 0x04 : amode = 2; // CW
						break;
			case 0x84 : amode = 3; // CWN
						break;
			case 0x08 : amode = 1; // USB
						break;
			case 0x10 : amode = 0; // LSB
						break;
		}

		A.freq = afreq;
		A.imode = amode;

		B.freq = bfreq;
		B.imode = amode;

		split_status = ((replystr[0] & 0x02) == 0x02);

		return true;
	}

	return false;
}
