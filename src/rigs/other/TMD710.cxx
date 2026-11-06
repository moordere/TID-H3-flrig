// ----------------------------------------------------------------------------
// Copyright (C) 2023
//              David Freese, W1HKJ
//              Dominik, DB1RUF
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


// For Kennwood TM-D710 and similar 2m / 70cm mobile transceivers
// Tested with TM-D710E

// Changes in 2023:
//   reworked due get_vfo was broken in or before flrig version 2.0.04
//   corrected reading of the current transceiver mode (AM FM NFM)
//   corrected selection (read and write) of the used site / vfo a or b
//   smaller improvements
//   forced dual band mode on startup removed

// Limitations:
//   Transceiver has for both vfo an independent squelch settings with
//   two analog mechanical rotated knops. Due to this the restore is not
//   working.  In "Config" -> "Setup" -> "Restore" menu the "Squelch"
//   should be unchecked.
//
//   CTRL and PTT are not independent in flrig. CTRL is leading and PTT
//   follows.
//
//   Changing frequency band is only possible on the transceiver control
//   panel. The needed cat command is not implemented in the transceiver.
//
//   The time can only be set on the display serial interface. But the
//   other commands need the interface on the transceiver modul. Due to
//   this, it's not implemented.
//
//   Tab CTCSS is not implemented. The transceiver is via cat capable to
//   read and write the values but currently in flrig is only set_tones
//   and set_offset for FT-857D and FT897D implemented. The needed changes
//   in flrig are currently too much.


#include "other/TMD710.h"
#define TMD710_WAIT_TIME 300

const char TMD710name_[] = "TMD710";

static std::vector<std::string>TMD710modes_;
static const char *vTMD710modes_[] = {
	"FM", "NFM", "AM"};

//not available
static const char TMD710_mode_type[] =
	{'U', 'U', 'U'};

RIG_TMD710::RIG_TMD710() {
// base class values
	name_ = TMD710name_;
	modes_ = TMD710modes_;
	bandwidths_ = vNOBWS;
	serial_baudrate = BR57600;
	stopbits = 1;
	serial_retries = 3;

	serial_write_delay = 0;
	serial_post_write_delay = 0;

	serial_timeout = 100;
	serial_rtscts = true;
	serial_rtsplus = false;
	serial_dtrplus = false;
	serial_catptt = true;
	serial_rtsptt = false;
	serial_dtrptt = false;
	modeA = 1;

	has_mode_control = true;
	has_ptt_control = true;


	has_power_control = true;
	has_sql_control = true;
	has_getvfoAorB =true;
	has_vfoAB = true;
	precision = 10;
	ndigits = 10;

}

void RIG_TMD710::initialize()
{
	VECTOR (TMD710modes_, vTMD710modes_);
	modes_ = TMD710modes_;
	bandwidths_ = vNOBWS;
}

bool RIG_TMD710::check()
{
// Get radio model. Expected is for example "ID TM-D710" but it should
// work with the TM-V71 and TM-D710G as well
	cmd = "ID\r";  
	wait_char('\r', 15, TMD710_WAIT_TIME, "get radio model", ASC);
	get_trace(1, "get radio model");
	if (replystr.find("TM-") == std::string::npos) return 0;
	return 1;
}

unsigned long long RIG_TMD710::get_vfoA ()
{
	cmd = rsp = "FO 0";
	cmd += '\r';
	wait_char('\r',49, TMD710_WAIT_TIME, "get vfo A", ASC);
	get_trace(1,"get_vfoA");
	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return freqA;
	p += 5;
	unsigned long long f = 0;
	for (int n = 0; n < ndigits; n++)
		f = f * 10 + replystr[p + n] - '0';
	freqA = f;
	return freqA;

/*
1 	Band
					0 	Band A
					1 	Band B
					2 	TX A - RX B
					3 	TX B - RX A
2 	Frequency in Hz 10 digit. Must be within selected frequency band.
3 	Step size
					0 	5 Khz
					1 	6.25 Khz
					2 	28,33 KHz
					3 	10 Khz
					4 	12.5 Khz
					5 	15 Khz
					6 	20 Khz
					7 	25 KHz
					8 	30 KHz
					9 	50 Khz
					A 100 Khz
4 	Shift direction
					0 	simplex or split
					1 	up
					2 	down
5 	Reverse
					0 	off
					1 	on
6 	Tone status
					0 	off
					1 	on
7 	CTCSS status
					0 	off
					1 	on
8 	DCS status
					0 	off
					1 	on
9 	Tone frequency
					01 	67
					02 	69,3
					03 	71,9
					04 	74,4
					05 	77
					06 	79,7
					07 	82,5
					08 	85,4
					09 	88,5
					10 	91,5
					11 	94,8
					12 	97,4
					13 	100
					14 	103,5
					15 	107,2
					16 	110,9
					17 	114,8
					18 	118,8
					19 	123
					20 	127,3
					21 	131,8
					22 	136,5
					23 	141,3
					24 	146,2
					25 	151,4
					26 	156,7
					27 	162,2
					28 	167,9
					29 	173,8
					30 	179,9
					31 	186,2
					32 	192,8
					33 	203,5
					34 	240,7
					35 	210,7
					36 	218,1
					37 	225,7
					38 	229,1
					39 	233,6
					40 	241,8
					41 	250,3
					42 	254,1
10 	CTCSS frequency
					01 	67
					02 	69,3
					03 	71,9
					04 	74,4
					05 	77
					06 	79,7
					07 	82,5
					08 	85,4
					09 	88,5
					10 	91,5
					11 	94,8
					12 	97,4
					13 	100
					14 	103,5
					15 	107,2
					16 	110,9
					17 	114,8
					18 	118,8
					19 	123
					20 	127,3
					21 	131,8
					22 	136,5
					23 	141,3
					24 	146,2
					25 	151,4
					26 	156,7
					27 	162,2
					28 	167,9
					29 	173,8
					30 	179,9
					31 	186,2
					32 	192,8
					33 	203,5
					34 	240,7
					35 	210,7
					36 	218,1
					37 	225,7
					38 	229,1
					39 	233,6
					40 	241,8
					41 	250,3
					42 	254,1
11 	DCS frequency
12 	Offset frequency in Hz 8 digit
13 	Mode
					0 	FM
					1 	NFM
					2 	AM
*/
}

unsigned long long RIG_TMD710::get_vfoB ()
{
	cmd = rsp = "FO 1";
	cmd += '\r';
	wait_char('\r',49, TMD710_WAIT_TIME, "get vfo B", ASC);
	get_trace(1,"get_vfoB");
	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return freqB;
	p += 5;
	unsigned long long f = 0;
	for (int n = 0; n < ndigits; n++)
		f = f * 10 + replystr[p + n] - '0';
	freqB = f;
	return freqB;
}

void RIG_TMD710::set_vfoA (unsigned long long freq)
{
	cmd = "FO 0\r";
	int ret = wait_char('\r', 49, TMD710_WAIT_TIME, "get VFO A", ASC);
	get_trace(1,"get_vfoA");
	if (ret < 49) return;
	cmd = replystr;
	char frequency[11];
	snprintf(frequency,11, "%010llu", freq);
	int n = 0;
	int p = 5;
	for (n = 0; n < 10; n++) (cmd[p + n])= frequency[n];
	wait_char('\r', 49, TMD710_WAIT_TIME, "set frequency A", ASC);
	get_trace(1,"set_vfoA");
	return;
}

void RIG_TMD710::set_vfoB (unsigned long long freq)
{
	cmd = "FO 1\r";
	int ret = wait_char('\r', 49, TMD710_WAIT_TIME, "get VFO B", ASC);
	get_trace(1,"get_vfoB");
	if (ret < 49) return;
	cmd = replystr;
	char frequency[11];
	snprintf(frequency,11, "%010llu", freq);
	int n = 0;
	int p = 5;
	for (n = 0; n < 10; n++) (cmd[p + n])= frequency[n];
	wait_char('\r', 49, TMD710_WAIT_TIME, "set frequency B", ASC);
	get_trace(1,"set_vfoB");
	return;
}

int RIG_TMD710::get_vfoAorB()
{
	cmd = "BC\r";
	int ret = wait_char('\r', 7, TMD710_WAIT_TIME, "get vfo A/B", ASC); //Response CTRL PTT
	get_trace(1,"get vfo A/B");
	if (ret < 7) return inuse;
	if (replystr[3] == '0')
		inuse = onA;
	if (replystr[3] == '1')
		inuse = onB;
	return inuse;
}

int RIG_TMD710::get_modetype(int n)
{
	return TMD710_mode_type[n];
}

void RIG_TMD710::set_modeA(int val)
{
	// AM is not available at 430 MHz
	cmd = "FO 0\r";
	int ret = wait_char('\r', 49, TMD710_WAIT_TIME, "get VFO A", ASC);
	get_trace(1, "get_vfoA");
	if (ret < 49) return;
	cmd = replystr;
	cmd[47] = val +'0';
	wait_char('\r', 49, TMD710_WAIT_TIME, "set mode A", ASC);
	get_trace(1, "set_modeA");
	return;
}

void RIG_TMD710::set_modeB(int val)
{
	cmd = "FO 1\r";
	int ret = wait_char('\r', 49, TMD710_WAIT_TIME, "get VFO B", ASC);
	get_trace(1, "get_vfoB");
	if (ret < 49) return;
	cmd = replystr;
	cmd[47] = val +'0';
	wait_char('\r', 49, TMD710_WAIT_TIME, "set mode B", ASC);
	get_trace(1, "set_modeB");
	return;
}

int RIG_TMD710::get_modeA()
{
	cmd = "FO 0\r";
	int ret = wait_char('\r', 49, TMD710_WAIT_TIME, "get modeA", ASC);
	get_trace(1, "get_modeA");
	if (ret < 49) return modeA;
	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return modeA;
	unsigned long long f = 0;
	f = f * 1 + replystr[47] - '0';
	return f;
}

int RIG_TMD710::get_modeB()
{
	cmd = "FO 1\r";
	int ret = wait_char('\r', 49, TMD710_WAIT_TIME, "get modeB", ASC);
	get_trace(1, "get_modeB");
	if (ret < 49) return modeB;
	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return modeB;
	unsigned long long f = 0;
	f = f * 1 + replystr[47] - '0';
	return f;
}

void RIG_TMD710::selectA()
{
	cmd = "BC 0,0\r";
	wait_char('\r', 3, TMD710_WAIT_TIME, "set select A PTT/CTRL band", ASC);
	get_trace(1,"set_selectA");
	sett("selectA");
	inuse = onA;
	return;
}

void RIG_TMD710::selectB()
{
	cmd = "BC 1,1\r";
	wait_char('\r', 3, TMD710_WAIT_TIME, "set select B PTT/CTRL band", ASC);
	get_trace(1,"set_selectB");
	sett("selectB");
	inuse = onB;
	return;
}

// Transceiver PTT on/off
void RIG_TMD710::set_PTT_control(int val)
{
	if (val) sendCommand("TX\r");
		else sendCommand("RX\r");
	ptt_ = val;
}

int RIG_TMD710::get_PTT()
{
	return ptt_;
}

// Transceiver power level
double RIG_TMD710::get_power_control()
{
	cmd = "PC ";
	if (inuse == onA) cmd += '0';
	if (inuse == onB) cmd += '1';
	cmd += '\r';
	wait_char('\r', 8, TMD710_WAIT_TIME, "get power", ASC);
	get_trace(1,"get_power_control");
	int wert;
	int mtr = 30;
	wert = replystr[5] - '0';
	switch (wert) {
		case 0: mtr = 50; break;
		case 1: mtr = 10; break;
		case 2: mtr = 5; break;
		}
	return mtr;
}

void RIG_TMD710::set_power_control(double val)
{
	char mtrs = '2';
	if (val >= 35) mtrs = '0';
	if (val < 35) mtrs = '1';
	if (val < 10) mtrs = '2';
	cmd = "PC ";
	if (inuse == onA) cmd += '0';
	if (inuse == onB) cmd += '1';
	cmd += ',';
	cmd += mtrs;
	cmd += '\r';
	wait_char('\r', 8, TMD710_WAIT_TIME, "set power", ASC);
	get_trace(1,"set_power");
	return;
}

// SQ  0: band A, 1: band B , 00 to 1F
void RIG_TMD710::set_squelch(int val)
{
	char hexstr[3] = "00";
	cmd = "SQ ";
	if (inuse == onA) cmd += '0';
	if (inuse == onB) cmd += '1';
	cmd += ',';
	snprintf(hexstr,3,"%02X", val);
	cmd += hexstr;
	cmd += '\r';
	wait_char('\r', 9, TMD710_WAIT_TIME, "set squelch", ASC);
	get_trace(1,"set_squelch");
	return;
}

int  RIG_TMD710::get_squelch()
{
	cmd = "SQ ";
	if (inuse == onA) cmd += '0';
	if (inuse == onB) cmd += '1';
	cmd += '\r';
	int ret = wait_char('\r', 6, TMD710_WAIT_TIME, "get squelch", ASC);
	get_trace(1,"get_squelch_value");
	if (ret < 6) return false;
	char hexstr[3];
	hexstr[0]= replystr[3];
	hexstr[1]= replystr[4];
	hexstr[2]= '\0';
	return std::strtol(hexstr, 0, 16);
}
