// ----------------------------------------------------------------------------
// Copyright (C) 2023
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

#include "other/SmartSDR.h"
#include "support.h"
#include <sstream>

static const char SmartSDRname_[] = "SmartSDR";

enum SMART_MODES {SM_LSB, SM_USB, SM_CWL, SM_CWU, SM_FM, SM_AM, SM_DIGU, SM_DIGL, SM_SAM, SM_NFM, SM_RTTY, SM_DFM};

static std::vector<std::string>SmartSDRmodes_;
static const char *vSmartSDRmodes_[] = {
	"LSB", "USB", "CW-L", "CW-U", "FM", "AM", "DIGU", "DIGL", "SAM", "NFM", "RTTY", "DFM"
};

static std::vector<std::string>SmartSDR_mode_chr;
static const char *vSmartSDR_mode_chr[] = { 
	"00",  "01",  "03",  "04",  "05",  "06", "07", "09",   "10",   "11",  "30",  "12"
};
static const char SmartSDR_mode_type[] = { 
	'L',  'U',    'L',   'U',  'U',   'U',   'U',  'L',   'U',    'U',  'L',   'U' };

static std::vector<std::string>SmartSDR_empty;
static const char *vSmartSDR_empty[] =
{ "NONE" };

//------------------------------------------------------------------------
static std::vector<std::string>SmartSDR_bws;
static const char *vSmartSDR_bws[] = {
	"Fil 1x1", "Fil 1x2", "Fil 1x3", "Fil 1x4", "Fil 2x1", "Fil 2x2", "Fil 2x3", "Fil 2x4" };
static std::vector<std::string>SmartSDR_CAT_BW;
static const char *vSmartSDR_CAT_BW[] = {
"ZZFI07;", "ZZFI06;", "ZZFI05;", "ZZFI04;", "ZZFI03;", "ZZFI02;", "ZZFI01;", "ZZFI00;" };

static std::vector<std::string>SmartSDR_widths_SSB;
static const char *vssb[] = {
 "1800", "2100", "2400", "2700", "2900", "3300", "4000", "6000" };

static std::vector<std::string>SmartSDR_widths_AM;
static const char *vam[] = {
"5.6 K", "6.0 K", "8.0 K", "10 K",  "12 K",  "14 K", "16 K", "20 K" };

static std::vector<std::string>SmartSDR_widths_CW;
static const char *vcww[] = {
  "50",  "100",  "250",  "400",  "500",  "800", "1000", "3000" };

static std::vector<std::string>SmartSDR_widths_RTTY;
static const char *vrtty[] = {
 "250",  "300",  "350",  "400",  "500", "1100", "1500", "3000" };

static std::vector<std::string>SmartSDR_widths_DATA;
static const char *vdata[] = {
 "100",  "300",  "600", "1000", "1500", "2000", "3000", "6000" };

static std::vector<std::string>SmartSDR_widths_FM;
static const char *vfm[] = {
  "FIXED" };

static std::vector<std::string>SmartSDR_widths_DFM;
static const char *vdfm[] = {
"6.0 K", "8.0 K", "10.0 K", "12 K",  "14 K",  "16 K", "18 K", "20 K" };

//------------------------------------------------------------------------------
static GUI rig_widgets[]= {
	{ (Fl_Widget *)sldrRFGAIN,   54, 125, 156 }, // 2
	{ (Fl_Widget *)sldrMICGAIN, 266, 125, 156 }, // 8
	{ (Fl_Widget *)sldrPOWER,    54, 145, 368 }, // 9
	{ (Fl_Widget *)NULL,          0,   0,   0 }
};

// mid range on loudness
static std::string menu012 = "EX01200004";

void RIG_SmartSDR::initialize()
{
	VECTOR (SmartSDRmodes_, vSmartSDRmodes_);
	VECTOR (SmartSDR_mode_chr, vSmartSDR_mode_chr);
	VECTOR (SmartSDR_empty, vSmartSDR_empty);

	VECTOR (SmartSDR_bws, vSmartSDR_bws);
	VECTOR (SmartSDR_CAT_BW, vSmartSDR_CAT_BW);

	VECTOR (SmartSDR_widths_SSB, vssb);
	VECTOR (SmartSDR_widths_AM, vam);
	VECTOR (SmartSDR_widths_CW, vcww);
	VECTOR (SmartSDR_widths_RTTY, vrtty);
	VECTOR (SmartSDR_widths_DATA, vdata);
	VECTOR (SmartSDR_widths_DFM, vdfm);
	VECTOR (SmartSDR_widths_FM, vfm);

	modes_ = SmartSDRmodes_;
	bandwidths_ = SmartSDR_bws;

	rig_widgets[0].W = sldrRFGAIN;
	rig_widgets[1].W = sldrMICGAIN;
	rig_widgets[2].W = sldrPOWER;

// turn off AI and set the input to DAX
	sendCommand("ZZAI0;");
	sendCommand("ZZDX1;");
}

void RIG_SmartSDR::shutdown()
{
}

RIG_SmartSDR::RIG_SmartSDR() {
// base class values
	name_ = SmartSDRname_;
	modes_ = SmartSDRmodes_;
	bandwidths_ = SmartSDR_empty;

	widgets = rig_widgets;

	serial_baudrate = BR9600;
	stopbits = 1;
	serial_retries = 2;

	serial_timeout = 50;
	serial_rtscts =  false;
	serial_rtsplus = false;
	serial_dtrplus = false;
	serial_catptt =  true;
	serial_rtsptt =  false;
	serial_dtrptt =  false;
	B.imode = A.imode = SM_USB;
	B.iBW = A.iBW = 3;
	B.freq = A.freq = 7070000ULL;
	can_change_alt_vfo = false;

	has_dsp_controls =	false;

	has_power_out =
	has_rf_control =
	has_smeter =
	has_noise_control =
	has_power_control =
	has_mode_control =
	has_bandwidth_control =
	has_ptt_control =
	has_extras = true;

	precision = 1;
	ndigits = 9;

}

static int ret = 0;

const char * RIG_SmartSDR::get_bwname_(int n, int md)
{
	try {
		return SmartSDR_bws.at(n).c_str();
	} catch (const std::exception& e) {
		LOG_ERROR("%s", e.what());
	}
	return "Fil ???";
}

int RIG_SmartSDR::get_smeter()
{
	int smtr = 0;
	cmd = "ZZSM;";
	get_trace(1, "get_smeter");
	ret = wait_char(';', 8, 100, "get smeter", ASC);
	gett("");
	size_t p = replystr.rfind("ZZSM");
	if (p != std::string::npos) {
		smtr = fm_decimal(replystr.substr(p+4), 3);
		smtr *= (100.0 / 260.0);
	}
	return smtr;
}

// Transceiver power level
void RIG_SmartSDR::set_power_control(double val)
{
	int ival = (int)val;
	cmd = "ZZPC";
	cmd.append(to_decimal(ival, 3)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "set pwr ctrl", cmd, "");
	sett("pwr control");
}

double RIG_SmartSDR::get_power_control()
{
	int pctrl = 0;
	cmd = "ZZPC;";
	get_trace(1, "get_power_control");
	ret = wait_char(';', 8, 100, "get pout", ASC);
	gett("");
	size_t p = replystr.rfind("ZZPC");
	if (p != std::string::npos) {
		pctrl = fm_decimal(replystr.substr(p+4), 3);
	}
	return pctrl;
}

int RIG_SmartSDR::set_widths(int val)
{
	return get_bw(val);
}

void RIG_SmartSDR::set_modeA(int val)
{
	try {
		_currmode = A.imode = val;
		cmd = "ZZMD";
		cmd += SmartSDR_mode_chr[val];
		cmd += ';';
		sendCommand(cmd);
		showresp(WARN, ASC, "set mode", cmd, "");
		sett("modeA");
//		A.iBW = set_widths(val);
//		vfoA.iBW = A.iBW;
	} catch (const std::exception& e) {
		LOG_ERROR("%s", e.what());
	}
}

int RIG_SmartSDR::get_modeA()
{
	if (tuning()) return A.imode;
	cmd = "ZZMD;";
	get_trace(1, "get_modeA");
	ret = wait_char(';', 7, 100, "get mode A", ASC);
	gett("");
	size_t p = replystr.rfind("MD");
	if (p != std::string::npos) {
		for (size_t md = 0; md < SmartSDR_mode_chr.size(); md++) {
			if (replystr.substr(p+2, 2) == SmartSDR_mode_chr.at(md)) {
				A.imode = md;
				break;
			}
		}
	}
	_currmode = A.imode;
	return A.imode;
}


void RIG_SmartSDR::set_modeB(int val)
{
	try {
		_currmode = B.imode = val;
		cmd = "MD";
		cmd += SmartSDR_mode_chr[val];
		cmd += ';';
		sendCommand(cmd);
		showresp(WARN, ASC, "set mode B", cmd, "");
		sett("modeB");
//		MilliSleep(100); // give rig a chance to change width
//		B.iBW = set_widths(val);
//		vfoB.iBW = B.iBW;
	} catch (const std::exception& e) {
		LOG_ERROR("%s", e.what());
	}
}

int RIG_SmartSDR::get_modeB()
{
	return B.imode;
	if (tuning()) return B.imode;
	cmd = "ZZMD;";
	get_trace(1, "get_modeA");
	ret = wait_char(';', 7, 100, "get mode B", ASC);
	gett("");
	size_t p = replystr.rfind("MD");
	if (p != std::string::npos) {
		for (size_t md = 0; md < SmartSDR_mode_chr.size(); md++) {
			if (replystr.substr(p+2, 2) == SmartSDR_mode_chr.at(md)) {
				B.imode = md;
				break;
			}
		}
	}
	_currmode = B.imode;
	return B.imode;
}

int RIG_SmartSDR::adjust_bandwidth(int val)
{
	int bw = 0;
	bandwidths_ = bwtable(val);
	
		switch (val) {
		case SM_CWL:
		case SM_CWU:   bw = 4; break;

		case SM_AM:
		case SM_SAM:  bw = 5; break;

		case SM_RTTY: bw = 6; break;

		case SM_DIGL:
		case SM_DIGU: bw = 5; break;

		case SM_FM:
		case SM_NFM:  bw = 0; break;

		case SM_DFM:  bw = 3; break;
		default: ;
			bw = 4;
	}
	return bw;
}

int RIG_SmartSDR::def_bandwidth(int val)
{
	return adjust_bandwidth(val);
}

void RIG_SmartSDR::set_bwA(int val)
{
	if (A.imode == FM || A.imode == DFM) return; // mode is fixed

	try {
		cmd = SmartSDR_CAT_BW.at(val);
		sendCommand(cmd);
		showresp(WARN, ASC, "set_bwA", cmd, "");
		sett("bwA");
		A.iBW = val;
	} catch (const std::exception& e) {
		LOG_ERROR("%s", e.what());
	}
}

int RIG_SmartSDR::get_bw(int mode)
{
	int bw = 0;

	cmd = "ZZFI;";
	get_trace(3, "get ", SmartSDRmodes_[mode].c_str(), " bandwidth");
	ret = wait_char(';', 7, 100, "get ZZFI", ASC);
	gett("");
	for (size_t i = 0; i < SmartSDR_CAT_BW.size(); i++) {
		if (replystr.find(SmartSDR_CAT_BW.at(i)) != std::string::npos) {
			bw = i;
			break;
		}
	}
	return bw;
}

int RIG_SmartSDR::get_bwA()
{
	vfoA.iBW = A.iBW = get_bw(A.imode);
	progStatus.iBW_A = A.iBW;
	return A.iBW;
}

void RIG_SmartSDR::set_bwB(int val)
{
	if (B.imode == FM || B.imode == DFM) return; // mode is fixed

	try {
		cmd = SmartSDR_CAT_BW.at(val);
		sendCommand(cmd);
		showresp(WARN, ASC, "set_bwA", cmd, "");
		sett("bwA");
		B.iBW = val;
	} catch (const std::exception& e) {
		LOG_ERROR("%s", e.what());
	}
}

int RIG_SmartSDR::get_bwB() // same as A
{
	vfoB.iBW = B.iBW = get_bw(B.imode);
	progStatus.iBW_B = B.iBW;
	return B.iBW;
}

int RIG_SmartSDR::get_modetype(int n)
{
	if (n >= (int)(sizeof(SmartSDR_mode_type)/sizeof(*SmartSDR_mode_type))) return 0;
	return SmartSDR_mode_type[n];
}

void RIG_SmartSDR::set_rf_gain(int val)
{
	cmd = "ZZAR";
	if (val < 0) cmd += '-';
	else cmd += '+';
	cmd.append(to_decimal(abs(val), 3)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "set rf gain", cmd, "");
	sett("RFgain");
}

int  RIG_SmartSDR::get_rf_gain()
{
	cmd = "ZZAR;";
	int rfg = 100;
	get_trace(1, "get_rf_gain");
	ret = wait_char(';', 9, 100, "get rf gain", ASC);
	gett("");
	size_t p = replystr.rfind("ZZAR");
	if (p != std::string::npos) {
		sscanf(replystr.c_str(),"ZZAR%d",  &rfg);
	}
	return rfg;
}

void RIG_SmartSDR::get_rf_min_max_step(int &min, int &max, int &step)
{
	min = 0;
	max = 100;
	step = 1;
}

void RIG_SmartSDR::set_mic_gain(int val)
{
	cmd = "ZZMG";
	if (val < 0) cmd += "-";
	else cmd += "+";
	cmd.append(to_decimal(abs(val),2)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "set mic", cmd, "");
	sett("MICgain");
}

int RIG_SmartSDR::get_mic_gain()
{
	int mgain = 0;
	cmd = "ZZMG;";
	get_trace(1, "get_mic_gain");
	ret = wait_char(';', 8, 100, "get mic", ASC);
	gett("");
	if (ret == 8) {
		size_t p = replystr.rfind("MG");
		if (p != std::string::npos) {
			sscanf(replystr.c_str(),"ZZMG%d",&mgain);
		}
	}
	return mgain;
}

void RIG_SmartSDR::get_mic_min_max_step(int &min, int &max, int &step)
{
	min = 0;
	max = 100;
	step = 1;
}


// Transceiver PTT on/off
void RIG_SmartSDR::set_PTT_control(int val)
{
	if (val) sendCommand("ZZTX1;");
	else     sendCommand("ZZTX0;");
	ptt_ = val;
}

int RIG_SmartSDR::get_PTT()
{
	cmd = "ZZTX;";
	get_trace(1, "get_PTT");
	ret = wait_char(';', 6, 100, "get PTT", ASC);
	gett("");
	if (ret < 6) return ptt_;
	ptt_ = (replystr[4] == '1');
	return ptt_;
}

unsigned long long RIG_SmartSDR::get_vfoA()
{
	cmd = "ZZFA;";
	get_trace(1, "get vfoA");
	ret = wait_char(';', 16, 100, "get vfoA", ASC);
	gett("");
	if (ret < 16) return A.freq;
	size_t p = replystr.rfind("ZZFA");
	if (p == std::string::npos) return A.freq;
	long long int newfreq = 0;
	if (sscanf(&replystr[p+4], "%lld", &newfreq) == 1)
		A.freq = newfreq;
	return A.freq;
}

void RIG_SmartSDR::set_vfoA(unsigned long long freq)
{
	A.freq = freq;
	cmd = "ZZFA00000000000;";
	for (int i = 14; i > 3; i--) {
		cmd[i] += freq % 10;
		freq /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "set vfo A", cmd, "");
	sett("vfoA");
}

unsigned long long RIG_SmartSDR::get_vfoB()
{
	return B.freq;
}

void RIG_SmartSDR::set_vfoB(unsigned long long freq)
{
	B.freq = freq;
	return;
}

void RIG_SmartSDR::selectA()
{
}

void RIG_SmartSDR::selectB()
{
}

std::vector<std::string>& RIG_SmartSDR::bwtable(int val)
{
	std::vector<std::string>& bw_ = SmartSDR_widths_SSB;
	switch (val) {
		case SM_CWL:
		case SM_CWU:   bw_ = SmartSDR_widths_CW; break;

		case SM_AM:
		case SM_SAM:  bw_ = SmartSDR_widths_AM; break;

		case SM_RTTY: bw_ = SmartSDR_widths_RTTY; break;

		case SM_DIGL:
		case SM_DIGU: bw_ = SmartSDR_widths_DATA; break;

		case SM_FM:
		case SM_NFM:  bw_ = SmartSDR_widths_FM; break;

		case SM_DFM:  bw_ = SmartSDR_widths_DFM; break;
		default: ;
	}
	return bw_;
}

