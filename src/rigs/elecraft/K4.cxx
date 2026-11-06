// ----------------------------------------------------------------------------
// Copyright (C) 2022
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

#include "elecraft/K4.h"
#include "status.h"

#include "support.h"

const char K4name_[] = "K4";

// 0=N/A, 1=LSB, 2=USB, 3=CW, 4=FM, 5=AM, 6=DATA, 7=CW REV, 8=N/A, 9=DATA REV.
static std::vector<std::string>K4modes_;
static const char *vK4modes_[] =
	{ "LSB", "USB", "CW", "FM", "AM", "DATA", "CW-R", "DATA-R"};
const char modenbr[] =
	{ '1', '2', '3', '4', '5', '6', '7', '9' };
static const char K4_mode_type[] =
	{ 'L', 'U', 'L', 'U', 'U', 'U', 'U', 'L' };

static std::vector<std::string>K4_widths;

static int mode_bwA[] = { -1, -1, -1, -1, -1, -1, -1, -1 };
static int mode_bwB[] = { -1, -1, -1, -1, -1, -1, -1, -1 };

// entry # = (FREQ - 50)/10; FREQ: 50...5000, #: 0...495
static int mode_def_bw[] = { 275, 275, 75, 495, 395, 275, 75, 275 };
 
//----------------------------------------------------------------------
static std::vector<std::string>K4_att_labels;
static const char *vK4_att_labels[] = { "ATT", "3 dB", "6 dB", "9 dB", "12 dB", "15 dB", "18 dB", "21 dB" };

static std::vector<std::string>K4_pre_labels;
static const char *vK4_pre_labels[] = { "PRE", "10 dB", "18 dB", "30 dB" };

// nr button used for AGC on K4
static std::vector<std::string>K4_nr_labels;
static const char *vK4_nr_labels[] = { "AGC", "AGC S", "AGC F" };
//----------------------------------------------------------------------

static GUI k4_widgets[]= {
	{ (Fl_Widget *)btnVol, 2, 125,  50 },
	{ (Fl_Widget *)sldrVOLUME, 54, 125, 156 },
	{ (Fl_Widget *)sldrRFGAIN, 54, 145, 156 },
	{ (Fl_Widget *)btnIFsh, 214, 105,  50 },
	{ (Fl_Widget *)sldrIFSHIFT, 266, 105, 156 },
	{ (Fl_Widget *)sldrMICGAIN, 266, 125, 156 },
	{ (Fl_Widget *)sldrPOWER, 266, 145, 156 },
	{ (Fl_Widget *)NULL, 0, 0, 0 }
};

static int agcvalA = 1;
static int agcvalB = 1;

int RIG_K4::power_scale()
{
	return powerScale;
}

RIG_K4::RIG_K4() {
// base class values
	name_ = K4name_;
	modes_ = K4modes_;
	bandwidths_ = K4_widths;

	serial_baudrate = BR38400;

	widgets = k4_widgets;

	stopbits = 1;
	serial_retries = 2;

//	serial_write_delay = 0;
//	serial_post_write_delay = 0;

	serial_timeout = 50;
	serial_rtscts = false;
	serial_rtsplus = false;
	serial_dtrplus = false;
	serial_catptt = true;
	serial_rtsptt = false;
	serial_dtrptt = false;

	def_freq = freqA = freqB = 14070000ULL;
	def_mode = modeA = modeB = 1;
	def_bw = bwA = bwB = 3000;

	can_change_alt_vfo =

	has_split_AB = true;
	has_micgain_control =
	has_rf_control = true;
	has_bandwidth_control =
	has_power_control = true;
	has_volume_control =
	has_mode_control =
	has_ptt_control = true;
	has_noise_control =
	has_attenuator_control = true;
	has_smeter = true;
	has_power_out = true;
	has_split = true;
	has_ifshift_control =
	has_preamp_control = true;
	has_agc_control = true;

	has_notch_control =
	has_tune_control =

	has_swr_control = false;

	if_shift_min = 300;
	if_shift_max = 3000;
	if_shift_step = 10;
	if_shift_mid = 300;

	precision = 1;
	ndigits = 8;

}

int  RIG_K4::adjust_bandwidth(int m)
{
	return def_bandwidth(m);
}

int  RIG_K4::def_bandwidth(int m)
{
	int width = -1;
	if (m < 0 || m > 7) return 0;
	if (inuse == onA) {
		width = mode_bwA[m];
		if (width == -1) width = mode_def_bw[m];
		mode_bwA[m] = width;
	} else {
		width = mode_bwA[m];
		if (width == -1) width = mode_def_bw[m];
		mode_bwB[m] = width;
	}
	return width;
}

#define K4_WAIT_TIME 800

void RIG_K4::initialize()
{
	VECTOR (K4modes_, vK4modes_);

	char width[10];
	K4_widths.clear();
	for (int n = 50; n <= 5000; n += 10) {
		snprintf(width, sizeof(width), "%d", n);
		K4_widths.push_back(width);
	}

	modes_ = K4modes_;
	bandwidths_ = K4_widths;

	VECTOR (K4_att_labels, vK4_att_labels);
	att_labels_ = K4_att_labels;

	VECTOR (K4_pre_labels, vK4_pre_labels);
	pre_labels_ = K4_pre_labels;

	VECTOR (K4_nr_labels, vK4_nr_labels);
	nr_labels_ = K4_nr_labels;

	LOG_INFO("K4");
	k4_widgets[0].W = btnVol;
	k4_widgets[1].W = sldrVOLUME;
	k4_widgets[2].W = sldrRFGAIN;
	k4_widgets[3].W = btnIFsh;
	k4_widgets[4].W = sldrIFSHIFT;
	k4_widgets[5].W = sldrMICGAIN;
	k4_widgets[6].W = sldrPOWER;

	powerScale = 1;

	cmd = "AI0;"; // disable auto-info
	set_trace(1, "disable auto info");
	sendCommand(cmd);
	sett("");

	cmd = "K41;"; // K4 advanced mode
	set_trace(1, "set K4 advanced mode");
	sendCommand(cmd);
	sett("");

	cmd = "OM;"; // request options to get power level
	get_trace(1, "get options");
	wait_char(';', 16, K4_WAIT_TIME, "Options", ASC);
	gett("");

	if (replystr.rfind("OM") != std::string::npos) {
		if (replystr.rfind("P") == std::string::npos) { // KPA4 present
			minpwr = 0.0;
			maxpwr = 10.0;
			steppwr = 0.1;
		} else {
			minpwr = 0.0;
			maxpwr = 110.0;
			steppwr = 1.0;
		}
	}

	get_vfoA();
	get_modeA();
	get_bwA();

	get_vfoB();
	get_modeB();
	get_bwB();

	set_split(false); // normal ops

}

void RIG_K4::shutdown()
{
}

bool RIG_K4::check ()
{
	return true;
	cmd = "FA;";
	get_trace(1, "check vfoA");
	int ret = wait_char(';', 14, 2000, "check", ASC);
	gett("");

	if (ret < 14) return false;
	return true;
}

unsigned long long RIG_K4::get_vfoA ()
{
	cmd = "FA;";
	get_trace(1, "get vfoA");
	wait_char(';', 14, K4_WAIT_TIME, "get vfo A", ASC);
	gett("");

	size_t p = replystr.rfind("FA");
	if (p == std::string::npos) return freqA;

	unsigned long long f = 0;
	for (size_t n = p + 2; n < replystr.length() - 1; n++)
		f = f*10 + replystr[n] - '0';
	freqA = f;
	return freqA;
}

void RIG_K4::set_vfoA (unsigned long long freq)
{
	freqA = freq;
	cmd = "FA00000000000;";
	for (int i = 12; i > 1; i--) {
		cmd[i] += freq % 10;
		freq /= 10;
	}
	set_trace(1, "set vfoA");
	sendCommand(cmd);
	sett("");
}

unsigned long long RIG_K4::get_vfoB ()
{
	cmd = "FB;";
	get_trace(1, "get vfoB");
	wait_char(';', 14, K4_WAIT_TIME, "get vfo B", ASC);
	gett("");

	size_t p = replystr.rfind("FB");
	if (p == std::string::npos) return freqB;

	unsigned long long f = 0;
	for (size_t n = p + 2; n < replystr.length() - 1; n++)
		f = f*10 + replystr[n] - '0';
	freqB = f;
	return freqB;
}

void RIG_K4::set_vfoB (unsigned long long freq)
{
	freqB = freq;
	cmd = "FB00000000000;";
	for (int i = 12; i > 1; i--) {
		cmd[i] += freq % 10;
		freq /= 10;
	}
	set_trace(1, "set vfoB");
	sendCommand(cmd);
	sett("");
}

// Volume control
void RIG_K4::set_volume_control(int val)
{
	if (isOnA()) {
		cmd = "AG000;";
		cmd[4] += val % 10; val /= 10;
		cmd[3] += val % 10; val /= 10;
		cmd[2] += val % 10;

		set_trace(1, "set volume");
		sendCommand(cmd);
		sett("");
	} else {
		cmd = "AG$000;";
		cmd[5] += val % 10; val /= 10;
		cmd[4] += val % 10; val /= 10;
		cmd[3] += val % 10;

		set_trace(1, "set volume");
		sendCommand(cmd);
		sett("");
	}
}

int RIG_K4::get_volume_control()
{
	int v;

	if (isOnA()) {
		cmd = "AG;";
		get_trace(1, "get volume control");
		wait_char(';', 7, K4_WAIT_TIME, "get volume", ASC);
		gett("");

		size_t p = replystr.rfind("AG");
		if (p == std::string::npos) return 0;

		v = atoi(&replystr[p + 3]);
	} else {
		cmd = "AG$;";
		get_trace(1, "get volume control");
		wait_char(';', 8, K4_WAIT_TIME, "get volume", ASC);
		gett("");

		size_t p = replystr.rfind("AG$");
		if (p == std::string::npos) return 0;

		v = atoi(&replystr[p + 4]);
	}
	return v;
}

void RIG_K4::get_vol_min_max_step(double &min, double &max, double &step)
{
	min = 0; max = 60; step = 1;
}


//----------------------------------------------------------------------
static const char *agcstrs[] = {"AGC", "AG-S", "AG-F"};

const char *RIG_K4::agc_label()
{
	if (agcval >= 0 && agcval <= 2)
		return agcstrs[agcval];
	return "AGC";
}

int  RIG_K4::get_agc()
{
	if (isOnA()) {
		cmd = "GT;";
		wait_char(';', 4, 100, "get AGC", ASC);
		gett("get agc");
		size_t p = replystr.rfind("GT");
		if (p == std::string::npos) return agcvalA;
		switch (replystr[p+2]) {
			default:
			case '0': noise_reduction_label(nr_label(), false); agcvalA = 0; break;
			case '1': noise_reduction_label(nr_label(), true); agcvalA = 1; break;
			case '2': noise_reduction_label(nr_label(), true); agcvalA = 2; break;
		}
		agcval=agcvalA;
	} else {
		cmd = "GT$;";
		wait_char(';', 5, 100, "get AGC", ASC);
		gett("get agc");
		size_t p = replystr.rfind("GT");
		if (p == std::string::npos) return agcvalB;
		switch (replystr[p+3]) {
			default:
			case '0': noise_reduction_label(nr_label(), false); agcvalB = 0; break;
			case '1': noise_reduction_label(nr_label(), true); agcvalB = 1; break;
			case '2': noise_reduction_label(nr_label(), true); agcvalB = 2; break;
		}
		agcval=agcvalB;
	}
	if (agcval == 0) agcvalA = agcvalB = 0;

	return agcval;
}


void  RIG_K4::set_agc_level(int val)
{
	if (isOnA()) {
		agcvalA = val;
		cmd = "GT0;";
		switch (val) {
			default:
			case '0': noise_reduction_label("AGC", false); cmd[2] = '0'; break;
			case '1': noise_reduction_label("AG-S", true); cmd[2] = '1'; break;
			case '2': noise_reduction_label("AG-F", true); cmd[2] = '2'; break;
		}
		set_trace(1, "set agc");
		sendCommand(cmd);
		sett("");
	} else {
		agcvalB = val;
		cmd = "GT$0;";
		switch (val) {
			default:
			case '0': noise_reduction_label("AGC", false); cmd[3] = '0'; break;
			case '1': noise_reduction_label("AG-S", true); cmd[3] = '1'; break;
			case '2': noise_reduction_label("AG-F", true); cmd[3] = '2'; break;
		}
		set_trace(1, "set agc");
		sendCommand(cmd);
		sett("");
	}
	return;
}


// Turning off AGC on either A or B results in AGC OFF for BOTH receivers. 
//
// Turning on AGC Fast for either one turns on AGC Slow for the 
//   other one when starting from OFF, but if the Other is ON then 
//   it's fast or slow setting remains.
//
// Turning on AGC Slow from the OFF state for either one turns
//   on AGC Slow for the other one.
//
// Make the AGC button toggle between off and Slow.  To set fast use
//   on screen menus to avoid needing a second AGC button.

int RIG_K4::incr_agc()
{
	static const char ch[] = {'0', '1', '2'};

	if (isOnA()) {
			agcvalA++;
			if (agcvalA > 1) agcvalA = agcvalB = 0;
			cmd = "GT0;";
		 	cmd[2] = ch[agcvalA];

			sendCommand(cmd);
			showresp(WARN, ASC, "SET agc", cmd, replystr);
			sett("set_agc");
		agcval = agcvalA;
		return agcvalA;
	} else {
			agcvalB++;
			if (agcvalB > 1) agcvalA = agcvalB = 0;
			cmd = "GT$0;";
		 	cmd[3] = ch[agcvalB];

			sendCommand(cmd);
			showresp(WARN, ASC, "SET agc", cmd, replystr);
			sett("set_agc");
		agcval = agcvalB;
		return agcvalB;
	}
}

void RIG_K4::set_modeA(int val)
{
	modeA = val;
	cmd = "MD0;";
	cmd[2] = modenbr[val];
	set_trace(1, "set modeA");
	sendCommand(cmd);
	sett("");

	set_pbt_values(val);
}

int RIG_K4::get_modeA()
{
	cmd = "MD;";
	get_trace(1, "get modeA");
	wait_char(';', 4, K4_WAIT_TIME, "get mode A", ASC);
	gett("");

	size_t p = replystr.rfind("MD");
//	if (replystr[p+2] == '0' || replystr[p+2] == '8') return modeA;
	if (p == std::string::npos) return modeA;

	int md = replystr[p + 2] - '1';
	if (md < 0 || md > 7) return modeA;
	if (md != modeA) set_pbt_values(md);
	return (modeA = md);
}

void RIG_K4::set_modeB(int val)
{
	modeB = val;
	cmd = "MD$0;";
	cmd[3] = modenbr[val];
	set_trace(1, "set modeB");
	sendCommand(cmd);
	sett("");

	set_pbt_values(val);
}

int RIG_K4::get_modeB()
{
	cmd = "MD$;";
	get_trace(1, "get modeB");
	wait_char(';', 4, K4_WAIT_TIME, "get mode B", ASC);
	gett("");

	size_t p = replystr.rfind("MD$");
//	if (replystr[p+3] == '0' || replystr[p+3] == '8') return modeB;
	if (p == std::string::npos) return modeB;

	int md = replystr[p + 3] - '1';
	if (md < 0 || md > 7) return modeA;
	if (md != modeB) set_pbt_values(md);
	return (modeB = md);
}

int RIG_K4::get_modetype(int n)
{
	return K4_mode_type[n];
}

// SET/RESP format: PA$nm; where n is as shown below, and m is 0 (off) or 1 (on).
//   n = 0 (off)
//   n = 1 (10 dB regular preamp)
//   n = 2 (160-15 m: 18 dB regular preamp; 12-6 m: 20 dB LNA)
//   n = 3 (12-6 m only: 10 dB regular preamp + 20 dB LNA)

void RIG_K4::set_preamp(int val)
{
	set_trace(1, "set preamp");
	if (isOnA()) cmd = "PA";
	else cmd = "PA$";
	switch (val) {
		case 0: cmd.append("00;"); break;
		case 1: cmd.append("11;"); break;
		case 2: cmd.append("21;"); break;
#ifdef HAVE_LNA
		case 3: cmd.append("31;"); break;
#endif
	}
	preamp_state = val;
	sendCommand(cmd);
	sett("");
}

int RIG_K4::get_preamp()
{
	int val;

	if (isOnA()) {
		cmd = "PA;";
		get_trace(1, "get_preamp");
		wait_char(';', 4, K4_WAIT_TIME, "get preamp", ASC);
		gett("");

		size_t p = replystr.rfind("PA");

		if (p == std::string::npos) return progStatus.preamp;
		val = progStatus.preamp = (replystr[p + 2] - '0');
	} else {
		cmd = "PA$;";
		get_trace(1, "get_preamp");
		wait_char(';', 5, K4_WAIT_TIME, "get preamp", ASC);
		gett("");

		size_t p = replystr.rfind("PA$");

		if (p == std::string::npos) return progStatus.preamp;
		val = progStatus.preamp = (replystr[p + 3] - '0');
	}

	preamp_state = val;
	return val;
}

// THIS IS A DO NOTHING METHOD !!
void K4_return(int val)
{
}

int RIG_K4::next_attenuator()
{
	if (atten_state < 7) atten_state++;
	else atten_state = 0;

	return atten_state;
}

int RIG_K4::next_preamp()
{
#ifdef HAVE_LNA
	if (preamp_state < 3) preamp_state++;
#else
	if (preamp_state < 2) preamp_state++;
#endif
	else preamp_state = 0;

	return preamp_state;
}

/*
  RA$nnm; where nn is 0/3/6/9/12/15/18/21 (dB) and m = 0 (off), 1 (on)
*/
void RIG_K4::set_attenuator(int val)
{
	sett("set_attenuator(int val)");

	if (isOnA()) cmd = "RA";
	else cmd = "RA$";
	switch (val) {
		default:
		case 0: cmd.append("000;"); break;
		case 1: cmd.append("031;");break;
		case 2: cmd.append("061;"); break;
		case 3: cmd.append("091;"); break;
		case 4: cmd.append("121;"); break;
		case 5: cmd.append("151;"); break;
		case 6: cmd.append("181;"); break;
		case 7: cmd.append("211;"); break;
	}
	set_trace(1, "set attenuator");
	sendCommand(cmd);
	sett("");
}

int RIG_K4::get_attenuator()
{
	std::string valstr;
	if (isOnA()) {
		cmd = "RA;";
		get_trace(1, "get attenuator");
		wait_char(';', 5, K4_WAIT_TIME, "set ATT", ASC);
		gett("");

		size_t p = replystr.rfind("RA");
		if (p == std::string::npos) return 0;
		valstr = replystr.substr(p + 2, 2);
	} else {
		cmd = "RA$;";
		get_trace(1, "get attenuator");
		wait_char(';', 6, K4_WAIT_TIME, "set ATT", ASC);
		gett("");

		size_t p = replystr.rfind("RA$");
		if (p == std::string::npos) return 0;
		valstr = replystr.substr(p + 3, 2);
	}

	if (valstr == "03") return 1;
	if (valstr == "06") return 2;
	if (valstr == "09") return 3;
	if (valstr == "12") return 4;
	if (valstr == "15") return 5;
	if (valstr == "18") return 6;
	if (valstr == "21") return 7;

	return 0;
}

// Transceiver power level
void RIG_K4::set_power_control(double val)
{
	int ival = val;
	cmd = "PC000";
		if (ival > 10) {
		for (int i = 4; i > 1; i--) {
			cmd[i] += ival % 10;
			ival /= 10;
		}
	} else {
		for (int i = 3; i > 1; i--) {
			cmd[i] += ival % 10;
			ival /= 10;
		}
	}

	if ((int)val > 10) cmd.append("H;");
	else cmd.append("L;");
	set_trace(1, "set power control");
	sendCommand(cmd);
	showresp(INFO, ASC, "set power ctrl", cmd, replystr);
	sett("");
}

double RIG_K4::get_power_control()
{
	cmd = "OM;"; // request options to get power level
	get_trace(1, "get options/pc_pwr_level");
	wait_char(';', 16, K4_WAIT_TIME, "Options", ASC);
	gett("");

	if (replystr.rfind("OM") != std::string::npos) { 
		if (replystr.find("P") == std::string::npos) {
			minpwr = 0.0;
			maxpwr = 10.0;
			steppwr = 0.1;
		} else {
			minpwr = 0.0;
			maxpwr = 110.0;
			steppwr = 1.0;
		}
	} else {
		minpwr = 0.0;
		maxpwr = 100.0;
		steppwr = 1.0;
	}

	cmd = "PCX;";
	get_trace(1, "get power control");
	wait_char(';', 8, K4_WAIT_TIME, "get power level", ASC);
	gett("");

	size_t p = replystr.rfind("PCX");
	if (p == std::string::npos) return progStatus.power_level;
	int level = fm_decimal(replystr.substr(p+3), 4);
	if (replystr[5] == 'L') {
		powerScale = 10;
		return level / 10.0;
	}
	powerScale = 1;
	return level;
}

void RIG_K4::get_pc_min_max_step(double &min, double &max, double &step)
{
	min = minpwr; max = maxpwr; step = steppwr;
}

// Transceiver rf control
void RIG_K4::set_rf_gain(int val)
{
	if (isOnA()) {
		int ival = abs(val);
		cmd = "RG-00;";
		cmd[3] += ival / 10;
		cmd[4] += ival % 10;

		set_trace(1, "set rf gain");
		sendCommand(cmd);
		sett("");
	} else {
		int ival = abs(val);
		cmd = "RG$-00;";
		cmd[4] += ival / 10;
		cmd[5] += ival % 10;

		set_trace(1, "set rf gain");
		sendCommand(cmd);
		sett("");
	}
}

int RIG_K4::get_rf_gain()
{
	int v;

	if (isOnA()) {
		cmd = "RG;";
		get_trace(1, "get rf gain");
		wait_char(';', 6, K4_WAIT_TIME, "get RF gain", ASC);
		gett("");

		size_t p = replystr.rfind("RG");
		if (p == std::string::npos) return progStatus.rfgain;

		replystr[p + 5] = 0;
		v = atoi(&replystr[p + 2]);
	} else {
		cmd = "RG$;";
		get_trace(1, "get rf gain");
		wait_char(';', 7, K4_WAIT_TIME, "get RF gain", ASC);
		gett("");

		size_t p = replystr.rfind("RG$");
		if (p == std::string::npos) return progStatus.rfgain;

		replystr[p + 6] = 0;
		v = atoi(&replystr[p + 3]);
	}
	return v;
}

void RIG_K4::get_rf_min_max_step(int &min, int &max, int &step)
{
   min = 0; max = -60; step = 1;
}

// Transceiver mic control
void RIG_K4::set_mic_gain(int val)
{
	cmd = "MG000;";
	cmd[4] += val % 10; val /= 10;
	cmd[3] += val % 10; val /= 10;
	cmd[2] += val % 10;

	set_trace(1, "set mic gain");
	sendCommand(cmd);
	sett("");
}

int RIG_K4::get_mic_gain()
{
	cmd = "MG;";
	get_trace(1, "get mic gain");
	wait_char(';', 6, K4_WAIT_TIME, "get MIC gain", ASC);
	gett("");

	size_t p = replystr.rfind("MG");
	if (p == std::string::npos) return progStatus.mic_gain;

	replystr[p + 5] = 0;
	int v = atoi(&replystr[p + 2]);
	return v;
}

void RIG_K4::get_mic_min_max_step(int &min, int &max, int &step)
{
   min = 0; max = 80; step = 1;
}

// Tranceiver PTT on/off
void RIG_K4::set_PTT_control(int val)
{
	set_trace(1, "set PTT");
	if (val) sendCommand("TX;", 0);
	else	 sendCommand("RX;", 0);
	sett("");
	ptt_ = val;
}

int RIG_K4::get_PTT()
{
	cmd = "TQ;";
	get_trace(1, "get PTT");
	wait_char(';', 4, K4_WAIT_TIME, "get PTT", ASC);
	gett("");

	size_t p = replystr.rfind("TQ");
	if (p == std::string::npos) return 0;

	if (fm_decimal(replystr.substr(p+2), 1) == 1)
		   ptt_ = true;
		else
		   ptt_ = false;

		return ptt_;
}

//SM $ (S-meter Read; GET only)
// SM$nn; where nn is 00-42. S9=17; S9+20=25; S9+40=33; S9+60=42.

int RIG_K4::get_smeter()
{
	cmd = "SMH;";
	get_trace(1, "get smeter");
	wait_char(';', 8, K4_WAIT_TIME, "get Smeter", ASC);
	gett("");

	size_t p = replystr.rfind("SMH");
	if (p == std::string::npos) return 0;

	int mtr = fm_decimal(replystr.substr(p+4), 3);
	mtr = (mtr - 150);

//	if (mtr <= 9) mtr = (int) (50.0 * mtr / 9.0);
//	else mtr = (int)(50 + (mtr - 9.0) * 50.0 / 12.0);

	return mtr;
}

void RIG_K4::set_noise(bool on)
{
	if (on) sendCommand("NB1;", 0);
	else	sendCommand("NB0;", 0);
	sett("set noise blanker");
}

int RIG_K4::get_noise()
{
	cmd = "NB;";
	wait_char(';', 4, K4_WAIT_TIME, "get Noise Blanker", ASC);
	gett("get noise blanker");

	size_t p = replystr.rfind("NB");
	if (p == std::string::npos) return progStatus.noise;
	return (replystr[p+2] == '1' ? 1 : 0);
}

// BW $ (Filter Bandwidth and Number; GET/SET)
// KX3 Extended SET/RSP format (K31): BWxxxx; where xxxx is 0-9999, the bandwidth in 10-Hz units. May be
// quantized and/or range limited based on the present operating mode.
// BW00050 => BW0005;
// 50 - 1000 in 50 Hz increments
// 1000 - 3000 in 100 Hz increments
// 3000 - 4000 in 200 Hz increments

void RIG_K4::set_bwA(int val)
{
	char command[10];
	short bw = 0;
	try {
		sscanf(K4_widths.at(val).c_str(), "%hd", &bw);
	} catch (...) {
		bw = 1800;
	}
	snprintf(command, sizeof(command), "BW%04d;", (bw/10));
	cmd = command;

	set_trace(1, "set bwA");
	sendCommand(cmd);
	sett("");
	bwA = val;
	mode_bwA[vfoA.imode] = bwA;
}

int RIG_K4::get_bwA()
{
	cmd = "BW;";
	get_trace(1, "get bwA val");
	wait_char(';', 7, K4_WAIT_TIME, "get bwA val", ASC);
	gett("");
	int bw = 0;
	sscanf(replystr.c_str(), "BW%d", &bw);
	bw *= 10;
	int n = 0;
	try {
		while (atol(K4_widths.at(n).c_str()) < bw) n++;
	} catch (...) {
		n = 0;
	}
	mode_bwA[vfoA.imode] = bwA = n;
	return bwA;
}

void RIG_K4::set_bwB(int val)
{
	char command[10];
	short bw = 0;
	try {
		sscanf(K4_widths.at(val).c_str(), "%hd", &bw);
	} catch (...) {
		bw = 1800;
	}
	snprintf(command, sizeof(command), "BW$%04d;", (bw/10));
	cmd = command;

	set_trace(1, "set bwB");
	sendCommand(cmd);
	sett("");
	bwB = val;
	mode_bwB[vfoB.imode] = bwB;
}

int RIG_K4::get_bwB()
{
	cmd = "BW$;";
	get_trace(1, "get bwB val");
	wait_char(';', 7, K4_WAIT_TIME, "get bwB val", ASC);
	gett("");
	int bw = 0;
	sscanf(replystr.c_str(), "BW$%d", &bw);
	bw *= 10;
	int n = 0;
	try {
		while (atol(K4_widths.at(n).c_str()) < bw) n++;
	} catch (...) {
		n = 0;
	}
	mode_bwB[vfoB.imode] = bwB = n;
	return bwB;
}

int RIG_K4::get_power_out()
{
		cmd = "TM;";
		get_trace(1, "get power out");
		wait_char(';', 15, K4_WAIT_TIME, "get power out", ASC);
		gett("");

		size_t p = replystr.rfind("TM");
		if (p == std::string::npos) return progStatus.power_level;
		int level = fm_decimal(replystr.substr(p+8), 3);
		return level;
}

bool RIG_K4::can_split()
{
	return true;
}

void RIG_K4::set_split(bool val)
{
	set_trace(1, "set split");
	if (val) {
		cmd = "FT1;";
		sendCommand(cmd);
	} else {
		cmd = "FT0;";
		sendCommand(cmd);
	}
	sett("");

	split_on = val;
}

// 01234567890123456789012345678901234567
// 0         1         2         3      7
// IF00014070000*****+yyyyrx*00tmvspbd1*;
//   |---------|                   |
//     vfo a/b                     split on = '1', off = '0'
// IF00014070000     -000000 0002000011 ;  OFF
// IF00014070000     -000000 0002001011 ;  ON

int RIG_K4::get_split()
{
	cmd = "IF;";
	get_trace(1, "get split");
	wait_char(';', 38, K4_WAIT_TIME, "get split", ASC);
	gett("");

	size_t p = replystr.rfind("IF");
	if (p == std::string::npos) return split_on;
	split_on = replystr[p+32] - '0';
	return split_on;
}

void RIG_K4::set_pbt_values(int val)
{
	switch (val) {
		case 0 :
		case 1 :
		case 3 :
		case 4 :
			if_shift_min = 300; if_shift_max = 3000;
			if_shift_step = 10; if_shift_mid = 300;
			break;
		case 2 :
		case 6 :
			if_shift_min = 300; if_shift_max = 2000;
			if_shift_step = 10; if_shift_mid = 300;
			break;
		case 5 :
		case 7 :
			if_shift_min = 300; if_shift_max = 3000;
			if_shift_step = 10; if_shift_mid = 300;
			break;
	}

	progStatus.shift_val = if_shift_mid;
	Fl::awake(adjust_if_shift_control, (void *)0);
}

void RIG_K4::set_if_shift(int val)
{
		if (isOnA()) {
		cmd = "IS0000;";
			val /= 10;
		cmd[5] += val % 10; val /= 10;
		cmd[4] += val % 10; val /= 10;
		cmd[3] += val % 10; val /= 10;
		cmd[2] += val % 10;
	} else {
		cmd = "IS$0000;";
			val /= 10;
		cmd[6] += val % 10; val /= 10;
		cmd[5] += val % 10; val /= 10;
		cmd[4] += val % 10; val /= 10;
		cmd[3] += val % 10;
	}
	set_trace(1, "set if shift");
	sendCommand(cmd);
	sett("");
}

bool RIG_K4::get_if_shift(int &val)
{
	if (isOnA()) {
		cmd = "IS;";
		get_trace(1, "get if shift");
		wait_char(';', 7, K4_WAIT_TIME, "get IF shift", ASC);
		gett("");
		size_t p = replystr.rfind("IS");
		if (p == std::string::npos) return progStatus.shift;
		sscanf(&replystr[p + 3], "%d", &progStatus.shift_val);
		val = progStatus.shift_val*10;
	} else {
		cmd = "IS$;";
		get_trace(1, "get if shift");
		wait_char(';', 8, K4_WAIT_TIME, "get IF shift", ASC);
		gett("");
		size_t p = replystr.rfind("IS$");
		if (p == std::string::npos) return progStatus.shift;
		sscanf(&replystr[p + 4], "%d", &progStatus.shift_val);
		val = progStatus.shift_val*10;
	}
	if (val == if_shift_mid) progStatus.shift = false;
	else progStatus.shift = true;
	return progStatus.shift;
}

void RIG_K4::get_if_min_max_step(int &min, int &max, int &step)
{
	min = if_shift_min; max = if_shift_max; step = if_shift_step;
}

void  RIG_K4::get_if_mid()
{
	cmd = "IS;";
	get_trace(1, "get center pbt");
	wait_char(';', 8, 500, "get PBT center", ASC);
	gett("");

	size_t p = replystr.rfind("IS ");
	if (p == std::string::npos) return;
	sscanf(&replystr[p + 3], "%d", &if_shift_mid);
}

void RIG_K4::selectA()
{
		if (!isOnA()) {
			cmd = "SW83;SW44;";
			set_trace(1, "selectA");
			sendCommand(cmd);
			sett("");

//	        K4split = false;
			showresp(WARN, ASC, "select A", cmd, replystr);
			inuse = onA;
	}
}

void RIG_K4::selectB()
{
	if (!isOnB()) {
			cmd = "SW83;SW44;";
		set_trace(1, "selectB");
			sendCommand(cmd);
			sett("");

//	        K4split = false;
			showresp(WARN, ASC, "select B", cmd, replystr);
			inuse = onB;
	}
}

