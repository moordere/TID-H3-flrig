// ----------------------------------------------------------------------------
// Copyright (C) 2020
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
#include <iostream>

#include "yaesu/FT991A.h"
#include "debug.h"
#include "support.h"

#define FL991A_WAIT_TIME 200

enum mFT991 {
  mLSB, mUSB, mCW, mFM, mAM, mRTTY_L, mCW_R, mPKT_L, mRTTY_U, mPKT_FM, mFM_N, mPKT_U, mAM_N, mC4FM };
//  0,    1,    2,   3,   4,   5,       6,     7,      8,       9,       10,    11,     12	// mode index

static const char FT991Aname_[] = "FT-991A";

static std::vector<std::string>FT991Amodes_;
static const char *vFT991Amodes_[] = {
"LSB", "USB", "CW-U", "FM", "AM", "RTTY-L",
"CW-L", "DATA-L", "RTTY-U", "DATA-FM",
"FM-N", "DATA-U", "AM-N", "C4FM"};

static const char FT991A_mode_chr[] =  { '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E' };
static const char FT991A_mode_type[] = { 'L', 'U', 'U', 'U', 'U', 'L', 'L', 'L', 'U', 'U', 'U', 'U', 'U', 'U' };

static const int FT991A_def_bw[] = {
	17,   17,   5,   0,   0,   10,       5,     16,     10,       0,       0,     16,     0,      0 };
// mLSB, mUSB, mCW, mFM, mAM, mRTTY_L, mCW_R, mPKT_L, mRTTY_U, mPKT_FM, mFM_N, mPKT_U, mAM_N, mC4FM

static std::vector<std::string>FT991A_widths_SSB;
static const char *vFT991A_widths_SSB[] = {
"200",   "400",  "600",  "850", "1100", "1350", "1500", "1650", "1800", "1950", 
"2100", "2200", "2300", "2400", "2500", "2600", "2700", "2800", "2900", "3000",
"3200" };

static int FT991A_wvals_SSB[] = {
 1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
21, WVALS_LIMIT};

static std::vector<std::string>FT991A_widths_SSBD;
static const char *vFT991A_widths_SSBD[] = {
	"50",  "100",  "150",  "200",  "250",  "300",  "350",  "400",  "450",  "500",
  "800", "1200", "1400", "1700", "2000", "2400", "3000" };

static int FT991A_wvals_SSBD[] = {
 1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
11, 12, 13, 14, 15, 16, 17, WVALS_LIMIT};

static std::vector<std::string>FT991A_widths_CW;
static const char *vFT991A_widths_CW[] = {
	"50",  "100",  "150",  "200",  "250",  "300",  "350",  "400",  "450",  "500",
  "800", "1200", "1400", "1700", "2000", "2400", "3000" };

static int FT991A_wvals_CW[] = {
 1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
11, 12, 13, 14, 15, 16, 17, WVALS_LIMIT};

// Single bandwidth modes
static std::vector<std::string>FT991A_widths_AMFMnar;
static const char *vFT991A_widths_AMFMnar[]  = { "NARR" };
static std::vector<std::string>FT991A_widths_AMFMnorm;
static const char *vFT991A_widths_AMFMnorm[] = { "NORM" };

static const int FT991A_wvals_AMFM[] = { 0, WVALS_LIMIT };

// mPKT_FM Multi bandwidth mode
static std::vector<std::string>FT991A_widths_NN;
static const char *vFT991A_widths_NN[] = {"NORM", "NARR" };

static const int FT991A_wvals_NN[] = {0, 1, WVALS_LIMIT};

// US 60M 5-USB, 5-CW
static std::vector<std::string>US_60m_chan;
static const char *vUS_60m_chan[]  = {"000","125","126","127","128","130","141","142","143","144","146"};
static std::vector<std::string>US_60m_label;
static const char *vUS_60m_label[] = {"VFO","U51","U52","U53","U54","U55","U56","U57","U58","U59","U50"};

// UK 60m channel numbers by Brian, G8SEZ
//static std::vector<std::string>UK_60m_chan  = {"000","118","120","121","127","128","129","130"};
//static std::vector<std::string>UK_60m_label = {"VFO","U51","U52","U53","U54","U55","U56","U57"};

//static std::vector<std::string>& Channels_60m = US_60m_chan;
//static std::vector<std::string>& label_60m    = US_60m_label;

//----------------------------------------------------------------------
static std::vector<std::string>FT991A_att_labels;
static const char *vFT991A_att_labels[] = { "ATT", "12 dB" };

static std::vector<std::string>FT991A_pre_labels;
static const char *vFT991A_pre_labels[] = { "IPO", "Amp 1", "Amp 2" };
//----------------------------------------------------------------------

static GUI rig_widgets[]= {
	{ (Fl_Widget *)btnVol,        2, 125,  50 }, // 0
	{ (Fl_Widget *)sldrVOLUME,   54, 125, 368 }, // 1
	{ (Fl_Widget *)sldrRFGAIN,   54, 145, 156 }, // 2
	{ (Fl_Widget *)sldrSQUELCH, 266, 145, 156 }, // 3

	{ (Fl_Widget *)sldrMICGAIN,  54, 165, 156 }, // 4
	{ (Fl_Widget *)btnNotch,    214, 165,  50 }, // 5
	{ (Fl_Widget *)sldrNOTCH,   266, 165, 156 }, // 6

	{ (Fl_Widget *)btnNR,         2, 185,  50 }, // 7
	{ (Fl_Widget *)sldrNR,       54, 185, 156 }, // 8
	{ (Fl_Widget *)btnIFsh,     214, 185,  50 }, // 9
	{ (Fl_Widget *)sldrIFSHIFT, 266, 185, 156 }, // 10

	{ (Fl_Widget *)sldrPOWER,    54, 205, 368 }, // 11

	{ (Fl_Widget *)NULL,          0,   0,   0 }
};

RIG_FT991A::RIG_FT991A() {
// base class values
	IDstr = "ID";
	name_ = FT991Aname_;
	modes_ = FT991Amodes_;
	bandwidths_ = FT991A_widths_SSB;
	bw_vals_ = FT991A_wvals_SSB;

	widgets = rig_widgets;

	serial_baudrate = BR38400;
	stopbits = 1;
	serial_retries = 2;

	serial_write_delay = 0;
	serial_post_write_delay = 25;

	serial_timeout = 50;
	serial_rtscts = true;
	serial_rtsplus = false;
	serial_dtrplus = false;
	serial_catptt = true;
	serial_rtsptt = false;
	serial_dtrptt = false;

	A.imode = B.imode = modeB = modeA = def_mode = 1;
	A.iBW = B.iBW = bwA = bwB = def_bw = 12;
	A.freq = B.freq = freqA = freqB = def_freq = 14070000ULL;

	has_compON =
	has_compression =
	has_a2b =
	has_ext_tuner =
	has_xcvr_auto_on_off =
	has_split =
	has_split_AB =
	has_band_selection =
	has_noise_reduction =
	has_noise_reduction_control =
	has_extras =
	has_vox_onoff =
	has_vox_gain =
	has_vox_anti =
	has_vox_hang =
	has_vox_on_dataport =

	has_vfo_adj =

	has_cw_wpm =
	has_cw_keyer =
//	has_cw_vol =
	has_cw_spot =
	has_cw_spot_tone =
	has_cw_qsk =
	has_cw_weight =
	has_cw_break_in =
	has_cw_zero_in =

	can_change_alt_vfo =
	has_smeter =
	has_alc_control =
	has_swr_control =
	has_agc_control =
	has_idd_control =
	has_voltmeter =
	has_power_out =
	has_power_control =
	has_volume_control =
	has_rf_control =
	has_sql_control =
	has_micgain_control =
	has_mode_control =
	has_noise_control =
	has_bandwidth_control =
	has_notch_control =
	has_auto_notch =
	has_attenuator_control =
	has_preamp_control =
	has_ifshift_control =
	has_ptt_control =
	has_tune_control =
	can_synch_clock = true;

// derived specific
	atten_state = 1;
	preamp_state = 2;
	notch_on = false;
	m_60m_indx = 0;

	precision = 1;
	ndigits = 9;

}

void RIG_FT991A::initialize()
{
	VECTOR (FT991Amodes_, vFT991Amodes_);
	VECTOR (FT991A_widths_SSB, vFT991A_widths_SSB);
	VECTOR (FT991A_widths_SSBD, vFT991A_widths_SSBD);
	VECTOR (FT991A_widths_CW, vFT991A_widths_CW);
	VECTOR (FT991A_widths_AMFMnar, vFT991A_widths_AMFMnar);
	VECTOR (FT991A_widths_AMFMnorm, vFT991A_widths_AMFMnorm);
	VECTOR (FT991A_widths_NN, vFT991A_widths_NN);
	VECTOR (US_60m_chan, vUS_60m_chan);
	VECTOR (US_60m_label, vUS_60m_label);

	VECTOR (FT991A_att_labels, vFT991A_att_labels);
	att_labels_ = FT991A_att_labels;

	VECTOR (FT991A_pre_labels, vFT991A_pre_labels);
	pre_labels_ = FT991A_pre_labels;

	modes_ = FT991Amodes_;
	bandwidths_ = FT991A_widths_SSB;
	bw_vals_ = FT991A_wvals_SSB;

	rig_widgets[0].W = btnVol;
	rig_widgets[1].W = sldrVOLUME;
	rig_widgets[2].W = sldrRFGAIN;
	rig_widgets[3].W = sldrSQUELCH;
	rig_widgets[4].W = sldrMICGAIN;
	rig_widgets[5].W = btnNotch;
	rig_widgets[6].W = sldrNOTCH;
	rig_widgets[7].W = btnNR;
	rig_widgets[8].W = sldrNR;
	rig_widgets[9].W = btnIFsh;
	rig_widgets[10].W = sldrIFSHIFT;
	rig_widgets[11].W = sldrPOWER;

// set progStatus defaults
	if (progStatus.notch_val < 10) progStatus.notch_val = 1500;
	if (progStatus.noise_reduction_val < 1) progStatus.noise_reduction_val = 1;
	if (progStatus.power_level < 5) progStatus.power_level = 5;
// first-time-thru, or reset
	if (progStatus.cw_qsk < 15) {
		progStatus.cw_qsk = 15;
		progStatus.cw_spot_tone = 700;
		progStatus.cw_weight = 3.0;
		progStatus.cw_wpm = 18;
		progStatus.vox_on_dataport = false;
		progStatus.vox_gain = 50;
		progStatus.vox_anti = 50;
		progStatus.vox_hang = 500;
	}
// Disable Auto Information mode
	sendCommand("AI0;");
/*
// "MRnnn;" if valid, returns last channel used, "mrlll...;", along with channel nnn info.
	cmd = "MR118;";
	wait_char(';', 27, FL991A_WAIT_TIME, "Read UK 60m Channel Mem", ASC);
	size_t p = replystr.rfind("MR");
	if (p == std::string::npos) {
		Channels_60m = US_60m_chan;
		label_60m    = US_60m_label;
		op_yaesu_select60->clear();
		char **p = (char **)US_60m_label;
		while (*p) op_yaesu_select60->add(*p++);
	}
	else {
		Channels_60m = UK_60m_chan;
		label_60m    = UK_60m_label;
		op_yaesu_select60->clear();
		char **p = (char **)UK_60m_label;
		while (*p) op_yaesu_select60->add(*p++);
	}
	op_yaesu_select60->index(m_60m_indx);
*/
}

bool valid_reply (std::string str)
{
	if (str.empty()) {
		get_trace (1, "no response");
		return false;
	}
	if (str.find(";") == std::string::npos) {
		get_trace (2, "missing semicolon", str.c_str());
		return false;
	}
	if (str.find("?;") != std::string::npos) {
		get_trace (2, "unknown command: ", str.c_str());
		return false;
	}
	return true;
}

void RIG_FT991A::post_initialize()
{
//	enable_yaesu_bandselect(12, false);
//	enable_yaesu_bandselect(13, false);
}

bool RIG_FT991A::check ()
{
	cmd = rsp = "FA";
	cmd += ';';
	int ret = wait_char(';',12, FL991A_WAIT_TIME, "check", ASC);
	if ( !valid_reply( replystr ) )
		return false;
	if (ret >= 12) return true;
	return false;
}

unsigned long long RIG_FT991A::get_vfoA ()
{
	cmd = rsp = "FA";
	cmd += ';';
	wait_char(';',12, FL991A_WAIT_TIME, "get vfo A", ASC);
	gett("get_vfoA");

	if ( !valid_reply( replystr ) )
		return freqA;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return freqA;

	replystr = replystr.substr(p);
	if (replystr.length() < 12) {
		char failed[50];
		snprintf(failed, sizeof(failed), "get vfo A failed, returned %d bytes", (int)(replystr.length()));
		get_trace(1, failed);
		return freqA;
	}
	p = 2;
	unsigned long long f = 0;
	for (int n = 0; n < ndigits; n++)
		f = f * 10 + replystr[p + n] - '0';
	freqA = f;
	return freqA;
}

void RIG_FT991A::set_vfoA (unsigned long long freq)
{
	freqA = freq;
	cmd = "FA000000000;";
	for (int i = 0; i < ndigits; i++) {
		cmd[ndigits + 1 - i] += freq % 10;
		freq /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "SET vfo A", cmd, replystr);
	sett("set_vfoA");
}

unsigned long long RIG_FT991A::get_vfoB ()
{
	cmd = rsp = "FB";
	cmd += ';';
	wait_char(';',12, FL991A_WAIT_TIME, "get vfo B", ASC);
	gett("get_vfoB");

	if ( !valid_reply( replystr ) )
		return freqB;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return freqB;

	replystr = replystr.substr(p);
	if (replystr.length() < 12) {
		char failed[50];
		snprintf(failed, sizeof(failed), "get vfo A failed, returned %d bytes", (int)(replystr.length()));
		get_trace(1, failed);
		return freqA;
	}
	p = 2;
	unsigned long long f = 0;
	for (int n = 0; n < ndigits; n++)
		f = f * 10 + replystr[p + n] - '0';
	freqB = f;
	return freqB;
}


void RIG_FT991A::set_vfoB (unsigned long long freq)
{
	freqB = freq;
	cmd = "FB000000000;";
	for (int i = 0; i < ndigits; i++) {
		cmd[ndigits + 1 - i] += freq % 10;
		freq /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "SET vfo B", cmd, replystr);
	sett("set_vfoB");
}

void RIG_FT991A::selectA()
{
	cmd = "FT2;";
	sendCommand(cmd);
	showresp(WARN, ASC, "select A", cmd, replystr);
	sett("selectA");
	inuse = onA;
}

void RIG_FT991A::selectB()
{
	cmd = "FT3;";
	sendCommand(cmd);
	showresp(WARN, ASC, "select B", cmd, replystr);
	sett("selectB");
	inuse = onB;
}

void RIG_FT991A::A2B()
{
	cmd = "AB;";
	sendCommand(cmd);
	showresp(WARN, ASC, "vfo A->B", cmd, replystr);
	sett("A2B");
}

void RIG_FT991A::B2A()
{
	cmd = "BA;";
	sendCommand(cmd);
	showresp(WARN, ASC, "vfo B->A", cmd, replystr);
	sett("B2A");
}

void RIG_FT991A::swapAB()
{
	cmd = "SV;";
	sendCommand(cmd);
	showresp(WARN, ASC, "vfo A<>B", cmd, replystr);
	sett("swapAB");
}

bool RIG_FT991A::can_split()
{
	return true;
}

void RIG_FT991A::set_split(bool val)
{
	split = val;
	if (inuse == onB) {
		if (val) {
			cmd = "FT2;";
			sendCommand(cmd);
			showresp(WARN, ASC, "Rx on B, Tx on A", cmd, replystr);
		} else {
			cmd = "FT3;";
			sendCommand(cmd);
			showresp(WARN, ASC, "Rx on B, Tx on B", cmd, replystr);
		}
	} else {
		if (val) {
			cmd = "FT3;";
			sendCommand(cmd);
			showresp(WARN, ASC, "Rx on A, Tx on B", cmd, replystr);
		} else {
			cmd = "FT2;";
			sendCommand(cmd);
			showresp(WARN, ASC, "Rx on A, Tx on A", cmd, replystr);
		}
	}
	sett("set_split");
	Fl::awake(highlight_vfo, (void *)0);
}

int RIG_FT991A::get_split()
{
	size_t p;
	char rx, tx;
// tx vfo
	cmd = rsp = "RI6";
	cmd.append(";");
	wait_char(';', 5, FL991A_WAIT_TIME, "get split tx vfo", ASC);
	gett("get split tx");

	if ( !valid_reply( replystr ) )
		return false;

	p = replystr.rfind(rsp);
	if (p == std::string::npos) return false;
	tx = replystr[p+3] - '0';

// rx vfo
// The FT991/A doesn't support FR so use RI7
// Radio Information -> VFO-A RX -> 0 : 1
	cmd = rsp = "RI7";
	cmd.append(";");
	wait_char(';', 5, FL991A_WAIT_TIME, "get split rx vfo", ASC);
	gett("get split rx");

	p = replystr.rfind(rsp);
	if (p == std::string::npos) return false;
	rx = replystr[p+3] - '0';

	// If tx & rx are different: not running split
	if (tx == rx) return true;
	return false;
}


int RIG_FT991A::get_smeter()
{
	cmd = rsp = "SM0";
	cmd += ';';
	wait_char(';',7, FL991A_WAIT_TIME, "get smeter", ASC);
	gett("get_smeter");

	if ( !valid_reply( replystr ) )
		return 0;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return 0;
	if (p + 6 >= replystr.length()) return 0;
	int mtr = atoi(&replystr[p+3]);
	mtr = mtr * 100.0 / 256.0;
	return mtr;
}

const static PAIR swr_tbl[] = {
	{  0,  1 }, // 1.0
	{ 10,  3 }, // 1.1
	{ 63, 11 }, // 1.5
	{ 92, 23 }, // 2.0
	{107, 35 }, // 2.5
	{116, 48 }, // 3.0
	{255, 100 }, // infinity
};

int RIG_FT991A::get_swr()
{
	PAIRS swr_pairs( define_PAIR(swr_tbl) );

	cmd = rsp = "RM6";
	cmd += ';';
	wait_char(';',7, FL991A_WAIT_TIME, "get swr", ASC);
	gett("get_swr");

	if ( !valid_reply( replystr ) )
		return 0;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return 0;
	if (p + 6 >= replystr.length()) return 0;
	int mtr = atoi(&replystr[p+3]);
	return swr_pairs.value(mtr);
}

int RIG_FT991A::get_alc()
{
	cmd = rsp = "RM4";
	cmd += ';';
	wait_char(';',7, FL991A_WAIT_TIME, "get alc", ASC);
	gett("get_alc");

	if ( !valid_reply( replystr ) )
		return 0;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return 0;
	if (p + 6 >= replystr.length()) return 0;
	int mtr = atoi(&replystr[p+3]);
	return (int)ceil(mtr / 2.56);
}

double RIG_FT991A::get_voltmeter()
{
	cmd = "RM8;";
	std::string resp = "RM";

	int mtr = 0;
	double val = 0;

	get_trace(1, "get_voltmeter()");
	wait_char(';',7, 100, "get vdd", ASC);
	gett("get_voltmeter");

	if ( !valid_reply( replystr ) )
		return 0;

	size_t p = replystr.rfind(resp);
	if (p != std::string::npos) {
		mtr = atoi(&replystr[p+3]);
		val = 13.8 * mtr / 190;
		return val;
	}

	return -1;
}

double RIG_FT991A::get_idd()
{
	cmd = rsp = "RM7";
	cmd += ';';
	wait_char(';',10, 100, "get alc", ASC);
	gett("get_idd");

	if ( !valid_reply( replystr ) )
		return 0;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return 0;
	if (p + 9 >= replystr.length()) return 0;
	replystr[6] = '\x00';
	double mtr = atoi(&replystr[p+3]);
	return mtr / 10.0;
}

int RIG_FT991A::get_power_out()
{
	cmd = rsp = "RM5";
	cmd += ';';
	wait_char(';',7, FL991A_WAIT_TIME, "get pout", ASC);
	gett("get_power_out");

	if ( !valid_reply( replystr ) )
		return 0;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return 0;
	double mtr = (double)(atoi(&replystr[p+3]));

	mtr = 3.8 - 0.0047 * mtr + .0022257 * mtr * mtr;

	return (int)ceil(mtr);
}

// Transceiver power level
double RIG_FT991A::get_power_control()
{
	cmd = rsp = "PC";
	cmd += ';';
	wait_char(';',6, FL991A_WAIT_TIME, "get power", ASC);
	gett("get_power_control");

	if ( !valid_reply( replystr ) )
		return progStatus.power_level;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return progStatus.power_level;
	if (p + 5 >= replystr.length()) return progStatus.power_level;

	int mtr = atoi(&replystr[p+2]);
	return mtr;
}

void RIG_FT991A::set_power_control(double val)
{
	int ival = (int)val;
	cmd = "PC000;";
	for (int i = 4; i > 1; i--) {
		cmd[i] += ival % 10;
		ival /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "SET power", cmd, replystr);
	sett("set_power_control");
}

// Volume control return 0 ... 100
int RIG_FT991A::get_volume_control()
{
	cmd = rsp = "AG0";
	cmd += ';';
	wait_char(';',7, FL991A_WAIT_TIME, "get vol", ASC);
	gett("get_volume_control");

	if ( !valid_reply( replystr ) )
		return progStatus.volume;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return progStatus.volume;
	if (p + 6 >= replystr.length()) return progStatus.volume;
	int val = round(atoi(&replystr[p+3]) / 2.55);
	if (val > 100) val = 100;
	return ceil(val);
}

void RIG_FT991A::set_volume_control(int val) 
{
	int ivol = (int)(val * 2.55);
	cmd = "AG0000;";
	for (int i = 5; i > 2; i--) {
		cmd[i] += ivol % 10;
		ivol /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "SET vol", cmd, replystr);
	sett("set_volume_control");
}

// Tranceiver PTT on/off
void RIG_FT991A::set_PTT_control(int val)
{
	cmd = val ? "TX1;" : "TX0;";
	sendCommand(cmd);
	MilliSleep(50);
	showresp(WARN, ASC, "SET PTT", cmd, replystr);
	ptt_ = val;
	sett("set_ptt_control");
}

int RIG_FT991A::get_PTT()
{
	cmd = "TX;";
	rsp = "TX";
	wait_char(';', 4, FL991A_WAIT_TIME, "get PTT", ASC);
	gett("get PTT");

	if ( !valid_reply( replystr ) )
		return ptt_;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return ptt_;
	switch (replystr[p+2]) {
		default: case '0' : ptt_ = false; break;
		case '1': case '2': ptt_ = true; break;
	}
	return ptt_;
}


// internal or external tune mode
void RIG_FT991A::tune_rig(int how)
{
	cmd = "AC000;";
	if (how == 1) cmd[4] = '1';
	else if (how == 2) cmd[4] = '2';
	sendCommand(cmd);
	showresp(WARN, ASC, "tune rig", cmd, replystr);
	sett("tune_rig");
}

int  RIG_FT991A::get_tune()
{
	cmd = "AC;";
	wait_char(';',6, FL991A_WAIT_TIME, "Tuner Enabled?", ASC);
	gett("get_tune");

	if ( !valid_reply( replystr ) )
		return 0;

	size_t p = replystr.rfind("AC");
	if (p == std::string::npos) return 0;
	if ((p + 5) >= replystr.length()) return 0;
	if (replystr[p+4] == '0') {
		return 0;
	}
	return 1;
}

int  RIG_FT991A::next_attenuator()
{
	switch (atten_state) {
		case 0: return 1;
		case 1: return 0;
	}
	return 0;
}

void RIG_FT991A::set_attenuator(int val)
{
	atten_state = val;
	cmd = "RA00;";
	cmd[3] += atten_state;
	sendCommand(cmd);
	showresp(WARN, ASC, "SET att", cmd, replystr);
	sett("set_attenuator");
}

int RIG_FT991A::get_attenuator()
{
	cmd = rsp = "RA0";
	cmd += ';';
	wait_char(';',5, FL991A_WAIT_TIME, "get att", ASC);
	gett("get_attenuator");

	if ( !valid_reply( replystr ) )
		return progStatus.attenuator;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return progStatus.attenuator;
	if (p + 3 >= replystr.length()) return progStatus.attenuator;
	atten_state = replystr[p+3] - '0';
	return atten_state;
}

int  RIG_FT991A::next_preamp()
{
	switch (preamp_state) {
		case 0: return 1;
		case 1: return 2;
		case 2: return 0;
	}
	return 0;
}

void RIG_FT991A::set_preamp(int val)
{
	preamp_state = val;
	cmd = "PA00;";
	cmd[3] = '0' + preamp_state;
	sendCommand (cmd);
	showresp(WARN, ASC, "SET preamp", cmd, replystr);
	sett("set_preamp");
}

int RIG_FT991A::get_preamp()
{
	cmd = rsp = "PA0";
	cmd += ';';
	wait_char(';',5, FL991A_WAIT_TIME, "get pre", ASC);
	gett("get_preamp");

	if ( !valid_reply( replystr ) )
		return preamp_state;

	size_t p = replystr.rfind(rsp);
	if (p != std::string::npos)
		preamp_state = replystr[p+3] - '0';
	return preamp_state;
}

int RIG_FT991A::adjust_bandwidth(int val)
{
	switch (val) {
		case mCW     :
		case mCW_R   :
		case mRTTY_L :
		case mRTTY_U :
			bandwidths_ = FT991A_widths_CW;
			bw_vals_ = FT991A_wvals_CW;
			break;
		case mFM     :
		case mAM     :
			bandwidths_ = FT991A_widths_AMFMnorm;
			bw_vals_    = FT991A_wvals_AMFM;
			break;
		case mFM_N   :
		case mAM_N   :
			bandwidths_ = FT991A_widths_AMFMnar;
			bw_vals_    = FT991A_wvals_AMFM;
			break;
		case mPKT_FM :
			bandwidths_ = FT991A_widths_NN;
			bw_vals_ = FT991A_wvals_NN;
			break;
		case mPKT_L :
		case mPKT_U :
			bandwidths_ = FT991A_widths_SSBD;
			bw_vals_ = FT991A_wvals_SSBD;
			break;
		default:
			bandwidths_ = FT991A_widths_SSB;
			bw_vals_ = FT991A_wvals_SSB;
	}
	return FT991A_def_bw[val];
}

int RIG_FT991A::def_bandwidth(int val)
{
	return FT991A_def_bw[val];
}

std::vector<std::string>& RIG_FT991A::bwtable(int n)
{
	switch (n) {
		case mPKT_FM : return FT991A_widths_NN;
		case mFM     :
		case mAM     : return FT991A_widths_AMFMnorm;
		case mFM_N   :
		case mAM_N   : return FT991A_widths_AMFMnar;
		case mCW     :
		case mCW_R   :
		case mRTTY_L :
		case mRTTY_U : return FT991A_widths_CW;
		case mPKT_L  :
		case mPKT_U  : return FT991A_widths_SSBD;
		default      : break;
	}
	return FT991A_widths_SSB;
}

void RIG_FT991A::set_modeA(int val)
{
	modeA = val;
	cmd = "MD0";
	cmd += FT991A_mode_chr[val];
	cmd += ';';
	sendCommand(cmd);
	showresp(WARN, ASC, "SET mode A", cmd, replystr);
	adjust_bandwidth(modeA);
	if (val == mCW || val == mCW_R) return;
	if (progStatus.spot_onoff) {
		progStatus.spot_onoff = false;
		set_spot_onoff();
		cmd = "CS0;";
		sendCommand(cmd);
		showresp(WARN, ASC, "SET spot off", cmd, replystr);
		btnSpot->value(0);
	}
	sett("set_modeA");
}

int RIG_FT991A::get_modeA()
{
	cmd = rsp = "MD0";
	cmd += ';';
	wait_char(';',5, FL991A_WAIT_TIME, "get mode A", ASC);
	gett("get_modeA");

	if ( !valid_reply( replystr ) )
		return modeA;

	size_t p = replystr.rfind(rsp);
	if (p != std::string::npos) {
		if (p + 3 < replystr.length()) {
			int md = replystr[p+3];
			if (md <= '9') md = md - '1';
			else md = 9 + md - 'A';
			modeA = md;
		}
	}
	adjust_bandwidth(modeA);
	return modeA;
}

void RIG_FT991A::set_modeB(int val)
{
	modeB = val;
	// Need to swap the vfos for the MD command to work on VFO B
	cmd = "SV;MD0";
	cmd += FT991A_mode_chr[val];
	cmd += ";SV;";
	sendCommand(cmd);
	showresp(WARN, ASC, "SET mode B", cmd, replystr);
	if (val == mCW || val == mCW_R) return;
	if (progStatus.spot_onoff) {
		progStatus.spot_onoff = false;
		set_spot_onoff();
		cmd = "CS0;";
		sendCommand(cmd);
		showresp(WARN, ASC, "SET spot off", cmd, replystr);
		btnSpot->value(0);
	}
	sett("set_modeB");
}

int RIG_FT991A::get_modeB()
{
	// Use Opposite Information command to read VFO B mode
	int n = 21;
	cmd = rsp = "OI";
	cmd += ';';
	wait_char(';',28, FL991A_WAIT_TIME, "get mode B", ASC);
	gett("get_modeB");

	if ( !valid_reply( replystr ) )
		return modeB;

	size_t p = replystr.rfind(rsp);
	if (p != std::string::npos) {
		// JBA - n tells us where to start in the response
		if (p + n < replystr.length()) {
			int md = replystr[p+n];
			if (md <= '9') md = md - '1';
			else md = 9 + md - 'A';
			modeB = md;
		}
	}
	return modeB;
}

void RIG_FT991A::set_bwA(int val)
{
	int bw_indx = bw_vals_[val];
	bwA = val;

	if (modeA == mFM || modeA == mAM || modeA == mFM_N || modeA == mAM_N) return;
	if (modeA == mPKT_FM) {
		if (val == 1) cmd = "NA01;";
		else cmd = "NA00;";
		sendCommand(cmd);
		showresp(WARN, ASC, "SET bw A", cmd, replystr);
		return;
	}
	if ((((modeA == mLSB || modeA == mUSB) && val < 8)) ||
		((modeA == mCW || modeA == mCW_R ||
		  modeA == mRTTY_L || modeA == mRTTY_U ||
		  modeA == mPKT_L || modeA == mPKT_U) && val < 4) ) cmd = "NA01;";
	else cmd = "NA00;";

	cmd.append("SH0");
	cmd += '0' + bw_indx / 10;
	cmd += '0' + bw_indx % 10;
	cmd += ';';
	sendCommand(cmd);
	showresp(WARN, ASC, "SET bw A", cmd, replystr);
	sett("set_bwA");
}

int RIG_FT991A::get_bwA()
{
	size_t p;
	if (modeA == mFM || modeA == mAM || modeA == mFM_N || modeA == mAM_N) {
		bwA = 0;
		return bwA;	
	}
	if (modeA == mPKT_FM) {
		cmd = rsp = "NA0";
		cmd += ';';
		wait_char(';',5, FL991A_WAIT_TIME, "get bw A narrow", ASC);
		p = replystr.rfind(rsp);
		if (p == std::string::npos) { bwA = 0; return bwA; }
		if (p + 4 >= replystr.length()) { bwA = 0; return bwA; }
		if (replystr[p+3] == '1') bwA = 1;	// narrow on
		else bwA = 0;
		return bwA;
	}
	cmd = rsp = "SH0";
	cmd += ';';
	wait_char(';',6, FL991A_WAIT_TIME, "get bw A", ASC);
	gett("get_bwA");

	if ( !valid_reply( replystr ) )
		return bwA;

	p = replystr.rfind(rsp);
	if (p == std::string::npos) return bwA;
	if (p + 5 >= replystr.length()) return bwA;

	replystr[p+5] = 0;
	int bw_idx = fm_decimal(replystr.substr(p+3), 2);
	const int *idx = bw_vals_;
	int i = 0;
	while (*idx != WVALS_LIMIT) {
		if (*idx == bw_idx) break;
		idx++;
		i++;
	}
	if (*idx == WVALS_LIMIT) i--;
	bwA = i;
	return bwA;
}

void RIG_FT991A::set_bwB(int val)
{
	int bw_indx = bw_vals_[val];
	bwB = val;

	if (modeB == mFM || modeB == mAM || modeB == mFM_N || modeB == mAM_N) return;
	if (modeB == mPKT_FM) {
		if (val == 1) cmd = "NA01;";
		else cmd = "NA00;";
		sendCommand(cmd);
		showresp(WARN, ASC, "SET bw B", cmd, replystr);
		return;
	}
	if ((((modeB == mLSB || modeB == mUSB) && val < 8)) ||
		((modeB == mCW || modeB == mCW_R ||
		  modeB == mRTTY_L || modeB == mRTTY_U ||
		  modeB == mPKT_L || modeB == mPKT_U) && val < 4) ) cmd = "NA01;";
	else cmd = "NA00;";

	cmd.append("SH0");
	cmd += '0' + bw_indx / 10;
	cmd += '0' + bw_indx % 10;
	cmd += ';';
	sendCommand(cmd);
	showresp(WARN, ASC, "SET bw B", cmd, replystr);
	sett("set_bwB");
}

int RIG_FT991A::get_bwB()
{
	size_t p;
	if (modeB == mFM || modeB == mAM || modeB == mFM_N || modeB == mAM_N) {
		bwB = 0;
		return bwB;
	}
	if (modeB == mPKT_FM) {
		cmd = rsp = "NA0";
		cmd += ';';
		wait_char(';',5, FL991A_WAIT_TIME, "get bw B narrow", ASC);
		p = replystr.rfind(rsp);
		if (p == std::string::npos) { bwB = 0; return bwB; }
		if (p + 4 >= replystr.length()) { bwB = 0; return bwB; }
		if (replystr[p+3] == '1') bwB = 1;	// narrow on
		else bwB = 0;
		return bwB;
	}
	cmd = rsp = "SH0";
	cmd += ';';
	wait_char(';',6, FL991A_WAIT_TIME, "get bw B", ASC);
	gett("get_bwB");

	if ( !valid_reply( replystr ) )
		return bwB;

	p = replystr.rfind(rsp);
	if (p == std::string::npos) return bwB;
	if (p + 5 >= replystr.length()) return bwB;

	replystr[p+5] = 0;
	int bw_idx = fm_decimal(replystr.substr(p+3), 2);
	const int *idx = bw_vals_;
	int i = 0;
	while (*idx != WVALS_LIMIT) {
		if (*idx == bw_idx) break;
		idx++;
		i++;
	}
	if (*idx == WVALS_LIMIT) i--;
	bwB = i;
	return bwB;
}

int RIG_FT991A::get_modetype(int n)
{
	return FT991A_mode_type[n];
}

void RIG_FT991A::set_if_shift(int val)
{
	cmd = "IS0+0000;";
	if (val < 0) cmd[3] = '-';
	val = abs(val);
	for (int i = 4; i > 0; i--) {
		cmd[3+i] += val % 10;
		val /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "SET if shift", cmd, replystr);
	sett("set_if_shift");
}

bool RIG_FT991A::get_if_shift(int &val)
{
	cmd = rsp = "IS0";
	cmd += ';';
	wait_char(';',9, FL991A_WAIT_TIME, "get if shift", ASC);
	gett("get_if_shift");

	if ( !valid_reply( replystr ) )
		return progStatus.shift;

	size_t p = replystr.rfind(rsp);
	val = progStatus.shift_val;
	if (p == std::string::npos) return progStatus.shift;
	val = atoi(&replystr[p+4]);
	if (replystr[p+3] == '-') val = -val;
	return (val != 0);
}

void RIG_FT991A::get_if_min_max_step(int &min, int &max, int &step)
{
	if_shift_min = min = -1000;
	if_shift_max = max = 1000;
	if_shift_step = step = 20;
	if_shift_mid = 0;
}

void RIG_FT991A::set_notch(bool on, int val)
{
// set notch frequency
	if (on) {
		cmd = "BP00001;";
		sendCommand(cmd);
		showresp(WARN, ASC, "SET notch on", cmd, replystr);
		cmd = "BP01000;";
		if (val % 10 >= 5) val += 10;
		val /= 10;
		for (int i = 3; i > 0; i--) {
			cmd[3 + i] += val % 10;
			val /=10;
		}
		sendCommand(cmd);
		showresp(WARN, ASC, "SET notch val", cmd, replystr);
		sett("set_notch_val");
		return;
	}

// set notch off
	cmd = "BP00000;";
	sendCommand(cmd);
	sett("set_notch_on/off");
	showresp(WARN, ASC, "SET notch off", cmd, replystr);
}

bool  RIG_FT991A::get_notch(int &val)
{
	bool ison = false;
	cmd = rsp = "BP00";
	cmd += ';';
	wait_char(';',8, FL991A_WAIT_TIME, "get notch on/off", ASC);
	gett("get_notch");

	if ( !valid_reply( replystr ) )
		return progStatus.notch_val;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return ison;

	if (replystr[p+6] == '1') { // manual notch enabled
		ison = true;
		val = progStatus.notch_val;
		cmd = rsp = "BP01";
		cmd += ';';
		wait_char(';',8, FL991A_WAIT_TIME, "get notch val", ASC);
		p = replystr.rfind(rsp);
		if (p == std::string::npos)
			val = 10;
		else
			val = fm_decimal(replystr.substr(p+4), 3) * 10;
	}
	return ison;
}

void RIG_FT991A::get_notch_min_max_step(int &min, int &max, int &step)
{
	min = 10;
	max = 3000;
	step = 10;
}

void RIG_FT991A::set_auto_notch(int v)
{
	cmd.assign("BC0").append(v ? "1" : "0" ).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET DNF Auto Notch Filter", cmd, replystr);
	sett("set_auto_notch");
}

int  RIG_FT991A::get_auto_notch()
{
	cmd = "BC0;";
	wait_char(';',5, FL991A_WAIT_TIME, "get auto notch", ASC);
	gett("get_auto_notch");

	if ( !valid_reply( replystr ) )
		return 0;

	size_t p = replystr.rfind("BC0");
	if (p == std::string::npos) return 0;
	if (replystr[p+3] == '1') return 1;
	return 0;
}

void RIG_FT991A::set_noise(bool b)
{
	if (b) cmd = "NB01;";
	else   cmd = "NB00;";
	sendCommand (cmd);
	showresp(WARN, ASC, "SET noise blanker", cmd, replystr);
	sett("set_noise");
}

int RIG_FT991A::get_noise()
{
	cmd = "NB0;";
	wait_char(';',5, FL991A_WAIT_TIME, "get NB", ASC);
	gett("get_noise");

	if ( !valid_reply( replystr ) )
		return 0;

	size_t p = replystr.rfind("NB0");
	if (p == std::string::npos) return 0;
	return replystr[p+3] - '0';
}

// val 0 .. 100
void RIG_FT991A::set_mic_gain(int val)
{
	cmd = "MG000;";
	for (int i = 3; i > 0; i--) {
		cmd[1+i] += val % 10;
		val /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "SET mic", cmd, replystr);
	sett("set_mic_gain");
}

int RIG_FT991A::get_mic_gain()
{
	cmd = rsp = "MG";
	cmd += ';';
	wait_char(';',6, FL991A_WAIT_TIME, "get mic", ASC);
	gett("get_mic_gain");

	if ( !valid_reply( replystr ) )
		return progStatus.mic_gain;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return progStatus.mic_gain;
	int val = atoi(&replystr[p+2]);
	if (val > 100) val = 100;
	return val;
}

void RIG_FT991A::get_mic_min_max_step(int &min, int &max, int &step)
{
	min = 0;
	max = 100;
	step = 1;
}

void RIG_FT991A::set_rf_gain(int val)
{
	cmd = "RG0000;";
	int rfval = (int)(val * 2.50);
	for (int i = 5; i > 2; i--) {
		cmd[i] = rfval % 10 + '0';
		rfval /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "SET rfgain", cmd, replystr);
	sett("set_rf_gain");
}

int  RIG_FT991A::get_rf_gain()
{
	int rfval = 0;
	cmd = rsp = "RG0";
	cmd += ';';
	wait_char(';',7, FL991A_WAIT_TIME, "get rfgain", ASC);
	gett("get_rf_gain");

	if ( !valid_reply( replystr ) )
		return progStatus.rfgain;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return progStatus.rfgain;
	for (int i = 3; i < 6; i++) {
		rfval *= 10;
		rfval += replystr[p+i] - '0';
	}
	rfval = (int)(rfval / 2.50);
	if (rfval > 100) rfval = 100;
	return ceil(rfval);
}

void RIG_FT991A::get_rf_min_max_step(int &min, int &max, int &step)
{
	min = 0;
	max = 100;
	step = 1;
}

void RIG_FT991A::set_vox_onoff()
{
	cmd = "VX0;";
	if (progStatus.vox_onoff) cmd[2] = '1';
	sendCommand(cmd);
	showresp(WARN, ASC, "SET vox", cmd, replystr);
	sett("set_vox_onoff");
}

void RIG_FT991A::set_vox_gain()
{
	cmd = "VG";
	cmd.append(to_decimal(progStatus.vox_gain, 3)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET vox gain", cmd, replystr);
	sett("set_vox_gain");
}

void RIG_FT991A::get_vox_gain()
{
	cmd = "VG;";
	wait_char(';', 6, FL991A_WAIT_TIME, "get VOX gain", ASC);
	gett("get_vox_gain");

	if ( !valid_reply( replystr ) )
		return;

	size_t p = replystr.rfind("VG");
	if (p == std::string::npos) return;
	replystr[p+5] = 0;
	progStatus.vox_gain = atoi(&replystr[p+2]);
}

void RIG_FT991A::set_vox_anti()
{
	cmd = "EX117";
	cmd.append(to_decimal(progStatus.vox_anti, 3)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET anti-vox", cmd, replystr);
	sett("set_vox_anti");
}

void RIG_FT991A::set_vox_hang()
{
	cmd = "VD";
	cmd.append(to_decimal(progStatus.vox_hang, 4)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET vox delay", cmd, replystr);
	sett("set_vox_hang");
}

void RIG_FT991A::get_vox_hang()
{
	cmd = "VD;";
	wait_char(';', 7, FL991A_WAIT_TIME, "get VOX delay", ASC);
	gett("get_vox_hang");

	if ( !valid_reply( replystr ) )
		return;

	size_t p = replystr.rfind("VD");
	if (p == std::string::npos) return;
	replystr[p+6] = 0;
	progStatus.vox_hang = atoi(&replystr[p+2]);
}

void RIG_FT991A::set_vox_on_dataport()
{
	cmd = "EX1420;";
	if (progStatus.vox_on_dataport) cmd[5] = '1';
	sendCommand(cmd);
	showresp(WARN, ASC, "SET vox on data port", cmd, replystr);
	sett("set_vox_on_dataport");
}

void RIG_FT991A::set_cw_wpm()
{
	cmd = "KS";
	if (progStatus.cw_wpm > 60) progStatus.cw_wpm = 60;
	if (progStatus.cw_wpm < 4) progStatus.cw_wpm = 4;
	cmd.append(to_decimal(progStatus.cw_wpm, 3)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET cw wpm", cmd, replystr);
	sett("set_cw_wpm");
}


void RIG_FT991A::enable_keyer()
{
	cmd = "KR0;";
	if (progStatus.enable_keyer) cmd[2] = '1';
	sendCommand(cmd);
	showresp(WARN, ASC, "SET keyer on/off", cmd, replystr);
	sett("enable_keyer");
}

bool RIG_FT991A::set_cw_spot()
{
	if (vfo->imode == mCW || vfo->imode == mCW_R) {
		cmd = "CS0;";
		if (progStatus.spot_onoff) cmd[2] = '1';
		sendCommand(cmd);
		showresp(WARN, ASC, "SET spot on/off", cmd, replystr);
		sett("set_cw_spot");
		return true;
	} else
		return false;
}

void RIG_FT991A::set_cw_weight()
{
	int n = round(progStatus.cw_weight * 10);
	cmd.assign("EX014").append(to_decimal(n, 2)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET cw weight", cmd, replystr);
	sett("set_cw_weight");
}

void RIG_FT991A::set_cw_qsk()
{
	int n = progStatus.cw_qsk / 5 - 3;
	cmd.assign("EX056").append(to_decimal(n, 1)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET cw qsk", cmd, replystr);
	sett("set_cw_qsk");
}

void RIG_FT991A::set_cw_spot_tone()
{
	int n = (progStatus.cw_spot_tone - 300) / 50;
	cmd.assign("EX059").append(to_decimal(n, 2)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET cw tone", cmd, replystr);
	sett("set_cw_spot_tone");
}

/*
void RIG_FT991A::set_cw_vol()
{
}
*/

// DNR
void RIG_FT991A::set_noise_reduction_val(int val)
{
	cmd.assign("RL0").append(to_decimal(val, 2)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET_noise_reduction_val", cmd, replystr);
	sett("set_noise_reduction_val");
}

int  RIG_FT991A::get_noise_reduction_val()
{
	int val = 1;
	cmd = rsp = "RL0";
	cmd.append(";");
	wait_char(';',6, FL991A_WAIT_TIME, "GET noise reduction val", ASC);

	if ( !valid_reply( replystr ) )
		return val;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return val;
	val = atoi(&replystr[p+3]);
	return val;
}

// DNR
void RIG_FT991A::set_noise_reduction(int val)
{
	cmd.assign("NR0").append(val ? "1" : "0" ).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET noise reduction", cmd, replystr);
	sett("set_noise_reduction_on/off");
}

int  RIG_FT991A::get_noise_reduction()
{
	int val = 0;
	cmd = rsp = "NR0";
	cmd.append(";");
	wait_char(';',5, FL991A_WAIT_TIME, "GET noise reduction", ASC);

	if ( !valid_reply( replystr ) )
		return 0;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return 0;
	val = replystr[p+3] - '0';
	return val;
}

void RIG_FT991A::set_xcvr_auto_on()
{
	if (!progStatus.xcvr_auto_on) return;

// This command requires dummy data be initially sent. Then after one
// second and before two seconds the command is sent.

	cmd = "PS1;"; // use as the dummy data
	sendCommand(cmd);
	update_progress(0);
	for (int i = 0; i < 1200; i += 100) {
		MilliSleep(100);
		update_progress(100 * i / 6000);
		Fl::awake();
	}

	cmd = rsp = "PS1";
	cmd.append(";");
	get_trace(1, "xcvr ON? ()");
	wait_char(';',4, 500, "Test: Is Rig ON", ASC);
	gett("");

	int powerstat;
	if(sscanf(replystr.c_str(),"PS%d", &powerstat)) {
		if (powerstat != 0) {
			update_progress(0);
			return;
		}
	}

	cmd = "PS1;";
	set_trace(1, "set xcvr ON()");
	sendCommand(cmd);
	sett("");

	for (int i = 1500; i < 6000; i += 100) {
		MilliSleep(100);
		update_progress(100 * i / 6000);
		Fl::awake();
	}
}

void RIG_FT991A::set_xcvr_auto_off()
{
	if (!progStatus.xcvr_auto_off) return;

	cmd = "PS0;";
	sendCommand(cmd);
	showresp(WARN, ASC, "SET xcvr auto on/off", cmd, replystr);
	sett("set_xcvr_auto_off");
}

void RIG_FT991A::set_compression(int on, int val)
{
	if (on) {
		if (val == 0) {
			cmd.assign("PR2;");	// mic eq on
			sendCommand(cmd);
			showresp(WARN, ASC, "set Comp EQ on", cmd, replystr);
		} else {
			cmd.assign("PR1;PL").append(to_decimal(val, 3)).append(";");
			sendCommand(cmd);
			showresp(WARN, ASC, "set Comp on", cmd, replystr);
		}
	} else{
		cmd.assign("PR0;");
		sendCommand(cmd);
		showresp(WARN, ASC, "set Comp off", cmd, replystr);
	}
	sett("set_comression");
}

void RIG_FT991A::setVfoAdj(double v)
{
	char cmdstr[20];
	int iv = fabs(v);
	snprintf(cmdstr, sizeof(cmdstr), "%02d", iv);
	cmd = "EX039";
	if (v < 0) cmd += '-';
	else cmd += '+';
	cmd.append(cmdstr);
	cmd.append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET vfo adjust", cmd, replystr);
	sett("set_VfoAdj");
}

double RIG_FT991A::getVfoAdj() 
{
	cmd = rsp = "EX039";
	sendCommand(cmd.append(";"));
	wait_char(';',9, FL991A_WAIT_TIME, "get Vfo Adjust", ASC);

	if ( !valid_reply( replystr ) )
		return 0;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return 0;
	return (double)(atoi(&replystr[p+5]));
}

void RIG_FT991A::get_vfoadj_min_max_step(double &min, double &max, double &step)
{
	min = -25;
	max = 25;
	step = 1;
}

void RIG_FT991A::get_band_selection(int v)
{
	const char *bands[] = {"00", "01", "03", "04", "05", "06", "07", "08", "09", "10", "15", "16", "11"};
	cmd.assign("BS").append(bands[v-1]).append(";");
	sendCommand(cmd);
	sett("get band");

	if ( !valid_reply( replystr ) )
		return;

}

void RIG_FT991A::set_break_in()
{
	if (progStatus.break_in) {
		cmd = "BI1;";
		break_in_label("BK-IN");
	} else {
		cmd = "BI0;";
		break_in_label("QSK ?");
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "SET break in on/off", cmd, replystr);
	sett("set_break_in");
}

int RIG_FT991A::get_break_in()
{
	cmd = "BI;";
	wait_char(';', 4, FL991A_WAIT_TIME, "get break in", ASC);

	if ( !valid_reply( replystr ) )
		return progStatus.break_in;

	if (replystr.find("BI") == std::string::npos)
		return progStatus.break_in;

	progStatus.break_in = (replystr[2] == '1');
	if (progStatus.break_in) {
		break_in_label("BK-IN");
		progStatus.cw_delay = 0;
	} else {
		break_in_label("QSK ?");
//		get_qsk_delay();
	}
	return progStatus.break_in;
}

// ---------------------------------------------------------------------
// set date and time
// 1A 05 00 95 HH MM : set time
// 1A 05 00 94 YY YY MM DD : set date
// ---------------------------------------------------------------------
// dt formated as YYYYMMDD
// ---------------------------------------------------------------------
void RIG_FT991A::sync_date(char *dt)
{
	cmd.assign("DT0");
	cmd.append(dt);
	cmd += ';';
	sendCommand(cmd);
	showresp(WARN, ASC, "sync_date", cmd, replystr);
	sett("sync_date");
}

// ---------------------------------------------------------------------
// tm formated as HH:MM:SS
// ---------------------------------------------------------------------
void RIG_FT991A::sync_clock(char *tm)
{
	cmd.assign("DT1");
	cmd += tm[0]; cmd += tm[1];
	cmd += tm[3]; cmd += tm[4];
	cmd += tm[6]; cmd += tm[7];
	cmd += ';';
	sendCommand(cmd);
	showresp(WARN, ASC, "sync_time", cmd, replystr);
	sett("sync_time");
}

int  RIG_FT991A::get_agc()
{
	cmd = "GT0;";
	wait_char(';', 6, FL991A_WAIT_TIME, "get agc", ASC);
	gett("get_agc");

	if ( !valid_reply( replystr ) )
		return agcval;

	size_t p = replystr.rfind("GT");
	if (p == std::string::npos) return agcval;

	switch (replystr[3]) {
		default:
		case '0': agcval = 0; break;
		case '1': agcval = 1; break;
		case '2': agcval = 2; break;
		case '3': agcval = 3; break;
		case '4': case '5':
		case '6': agcval = 4; break;
	}
	return agcval;
}

int RIG_FT991A::incr_agc()
{
static const char ch[] = {'0', '1', '2', '3', '4'};
	agcval++;
	if (agcval > 4) agcval = 0;
	cmd = "GT00;";
	cmd[3] = ch[agcval];

	sendCommand(cmd);
	showresp(WARN, ASC, "SET agc", cmd, replystr);
	sett("set_agc");

	return agcval;
}


static const char *agcstrs[] = {"AGC", "FST", "MED", "SLO", "AUT"};
const char *RIG_FT991A::agc_label()
{
	return agcstrs[agcval];
}

int  RIG_FT991A::agc_val()
{
	return (agcval);
}

void RIG_FT991A::set_squelch(int val)
{
	cmd = "SQ0000;";
	for (int i = 5; i > 2; i--) {
		cmd[i] = val % 10 + '0';
		val /= 10;
	}

	set_trace(1, "set_squelch()");
	sendCommand(cmd);
	sett("");
	showresp(WARN, ASC, "SET squelch", cmd, replystr);
}

int  RIG_FT991A::get_squelch()
{
	int sqval = 0;
	cmd = rsp = "SQ0";
	cmd += ';';
	get_trace(1, "get_squelch()");
	wait_char(';',7, FL991A_WAIT_TIME, "get squelch", ASC);
	gett("");

	if ( !valid_reply( replystr ) )
		return sqval;

	size_t p = replystr.rfind(rsp);
	if (p == std::string::npos) return progStatus.squelch;
	for (int i = 3; i < 6; i++) {
		sqval *= 10;
		sqval += replystr[p+i] - '0';
	}
	return ceil(sqval);
}

void  RIG_FT991A::zero_in()
{
	sendCommand("ZI;");
}

