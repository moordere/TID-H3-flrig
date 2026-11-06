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

#include "kenwood/TrxAVR.h"
#include "support.h"

static const char TRXAVRname_[] = "TrxAVR";

static std::vector<std::string>TRXAVRmodes_;
static const char *vTRXAVRmodes_[] = {
	"LSB", "USB", "CWL", "CWU" };

static const char TRXAVR_mode_chr[] =  { '1', '2', '3', '7' };
static const char TRXAVR_mode_type[] = { 'L', 'U', 'L', 'U' };

static std::vector<std::string>TRXAVR_empty;
static const char *vTRXAVR_empty[] = { "N/A" };
//------------------------------------------------------------------------------
static std::vector<std::string>TRXAVR_SL;
static const char *vTRXAVR_SL[] = {
 "0",   "50", "100", "200", "300",
"400",  "500", "600", "700", "800",
"900", "1000" };
static std::vector<std::string>TRXAVR_CAT_SL;
static const char *vTRXAVR_CAT_SL[] = {
"SL00;", "SL01;", "SL02;", "SL03;", "SL04;",
"SL05;", "SL06;", "SL07;", "SL08;", "SL09;",
"SL10;", "SL11;" };
static const char *TRXAVR_SL_tooltip = "lo cut";
static const char *TRXAVR_SSB_btn_SL_label = "L";
//------------------------------------------------------------------------------
static std::vector<std::string>TRXAVR_SH;
static const char *vTRXAVR_SH[] = {
"1400", "1600", "1800", "2000", "2200",
"2400", "2600", "2800", "3000", "3400",
"4000", "5000" };
static std::vector<std::string>TRXAVR_CAT_SH;
static const char *vTRXAVR_CAT_SH[] = {
"SH00;", "SH01;", "SH02;", "SH03;", "SH04;",
"SH05;", "SH06;", "SH07;", "SH08;", "SH09;",
"SH10;", "SH11;" };
static const char *TRXAVR_SH_tooltip = "hi cut";
static const char *TRXAVR_SSB_btn_SH_label = "H";
//------------------------------------------------------------------------------
static std::vector<std::string>TRXAVR_CWwidths;
static const char *vTRXAVR_CWwidths[] = {
"50", "80", "100", "150", "200",
"300", "400", "500", "600", "1000",
"2000"};
static std::vector<std::string>TRXAVR_CWbw;
static const char *vTRXAVR_CWbw[] = {
"FW0050;", "FW0080;", "FW0100;", "FW0150;", "FW0200;",
"FW0300;", "FW0400;", "FW0500;", "FW0600;", "FW1000;",
"FW2000;" };
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
static std::vector<std::string>TRXAVR_nr_labels;
static const char *vTRXAVR_nr_labels[] = { "NR", "NR 1", "NR 2" };
//------------------------------------------------------------------------------

static GUI rig_widgets[]= {
	{ (Fl_Widget *)btnVol,        2, 125,  50 }, // 0
	{ (Fl_Widget *)sldrVOLUME,   54, 125, 156 }, // 1
	{ (Fl_Widget *)btnIFsh,     214, 105,  50 }, // 3
	{ (Fl_Widget *)sldrIFSHIFT, 266, 105, 156 }, // 4
	{ (Fl_Widget *)btnNotch,    214, 125,  50 }, // 5
	{ (Fl_Widget *)sldrNOTCH,   266, 125, 156 }, // 6
	{ (Fl_Widget *)sldrMICGAIN, 266, 165, 156 }, // 8
	{ (Fl_Widget *)sldrPOWER,    54, 165, 368 }, // 9
	{ (Fl_Widget *)NULL,          0,   0,   0 }
};

// mid range on loudness
static std::string menu012 = "EX01200004";

void RIG_TRXAVR::initialize()
{
	VECTOR (TRXAVRmodes_, vTRXAVRmodes_);
	VECTOR (TRXAVR_empty, vTRXAVR_empty);
	VECTOR (TRXAVR_SL, vTRXAVR_SL);
	VECTOR (TRXAVR_SH, vTRXAVR_SH);
	VECTOR (TRXAVR_CAT_SL, vTRXAVR_CAT_SL);
	VECTOR (TRXAVR_CAT_SH, vTRXAVR_CAT_SH);
	VECTOR (TRXAVR_CWwidths, vTRXAVR_CWwidths);
	VECTOR (TRXAVR_CWbw, vTRXAVR_CWbw);

	VECTOR (TRXAVR_nr_labels, vTRXAVR_nr_labels);
	nr_labels_ = TRXAVR_nr_labels;

	modes_ = TRXAVRmodes_;
	bandwidths_ = TRXAVR_empty;

	dsp_SL     = TRXAVR_SL;
	dsp_SH     = TRXAVR_SH;

	rig_widgets[0].W = btnVol;
	rig_widgets[1].W = sldrVOLUME;
	rig_widgets[2].W = btnIFsh;
	rig_widgets[3].W = sldrIFSHIFT;
	rig_widgets[4].W = btnNotch;
	rig_widgets[5].W = sldrNOTCH;
	rig_widgets[6].W = sldrMICGAIN;
	rig_widgets[7].W = sldrPOWER;

	std::string current_nr;
	cmd = "NR;";
	if (wait_char(';', 4, 100, "read current NR", ASC) == 4)
		current_nr = replystr;
	if (current_nr == "?;") return;

	cmd = "NR1;";
	sendCommand(cmd);
	gett("get NR");
	cmd = "RL;";
	if (wait_char(';', 5, 100, "GET noise reduction val", ASC) == 5) {
		size_t p = replystr.rfind("RL");
		if (p != std::string::npos)
			_nrval1 = atoi(&replystr[p+2]);
	}
	cmd = "NR2;";
	sendCommand(cmd);
	gett("get NR value");

	cmd = "RL;";
	if (wait_char(';', 5, 100, "GET noise reduction val", ASC) == 5) {
		size_t p = replystr.rfind("RL");
		if (p != std::string::npos)
			_nrval2 = atoi(&replystr[p+2]);
	}

}

void RIG_TRXAVR::shutdown()
{
}

static bool is_tuning = false;
static int  skip_get = 2;

RIG_TRXAVR::RIG_TRXAVR() {
// base class values
	name_ = TRXAVRname_;
	modes_ = TRXAVRmodes_;
	bandwidths_ = TRXAVR_empty;

	dsp_SL     = TRXAVR_SL;
	SL_tooltip = TRXAVR_SL_tooltip;
	SL_label   = TRXAVR_SSB_btn_SL_label;

	dsp_SH     = TRXAVR_SH;
	SH_tooltip = TRXAVR_SH_tooltip;
	SH_label   = TRXAVR_SSB_btn_SH_label;

	widgets = rig_widgets;

	serial_baudrate = BR4800;
	stopbits = 2;
	serial_retries = 2;

	serial_timeout = 50;
	serial_rtscts = true;
	serial_rtsplus = false;
	serial_dtrplus = false;
	serial_catptt = true;
	serial_rtsptt = false;
	serial_dtrptt = false;
	B.imode = A.imode = 1;
	B.iBW = A.iBW = 0x8803;
	B.freq = A.freq = 14070000ULL;
	can_change_alt_vfo = true;

	has_power_out =
	has_swr_control =
	has_alc_control =
	has_split =
	has_split_AB =
	has_dsp_controls =
//	has_rf_control =
	has_notch_control =
	has_auto_notch =
	has_ifshift_control =
	has_smeter =
	has_noise_reduction =
	has_noise_reduction_control =
	has_noise_control =
	has_micgain_control =
	has_volume_control =
	has_power_control =
	has_tune_control =
	has_attenuator_control =
	has_preamp_control =
	has_mode_control =
	has_bandwidth_control =
	has_ptt_control = 
	has_extras = true;

	rxona = true;

	precision = 1;
	ndigits = 9;

	atten_state = 0;
	preamp_state = 0;
	nr_state = 0;
	_nrval1 = 2;
	_nrval2 = 4;

	is_tuning = false;
}

static int ret = 0;

const char * RIG_TRXAVR::get_bwname_(int n, int md)
{
	static char bwname[20];
	if (n > 256) {
		int hi = (n >> 8) & 0x7F;
		int lo = n & 0xFF;
		snprintf(bwname, sizeof(bwname), "%s/%s", TRXAVR_SL[lo].c_str(), TRXAVR_SH[hi].c_str());
	} else {
		snprintf(bwname, sizeof(bwname), "%s", TRXAVR_CWwidths[n].c_str());
	}
	return bwname;
}

int RIG_TRXAVR::get_smeter()
{
	int smtr = 0;
	if (rxona)
		cmd = "SM0;";
	else
		cmd = "SM1;";
	get_trace(1, "get_smeter");
	ret = wait_char(';', 8, 100, "get smeter", ASC);
	gett("");
	if (ret == 8) {
		size_t p = replystr.rfind("SM");
		if (p != std::string::npos) {
			smtr = fm_decimal(replystr.substr(p+3),4);
			if (rxona)
				smtr = (smtr * 100) / 30;
			else
				smtr = (smtr * 100) / 15;
		}
	}
	return smtr;
}

// Transceiver power level
void RIG_TRXAVR::set_power_control(double val)
{
	int ival = (int)val;
	cmd = "CG";
	cmd.append(to_decimal(ival, 3)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "set pwr ctrl", cmd, "");
	sett("pwr control");
}

int RIG_TRXAVR::get_power_out()
{
	int poutmtr = 0;
	cmd = "SM0;";
	get_trace(1, "get_power_out");
	ret = wait_char(';', 8, 100, "get power out", ASC);
	gett("");
	if (ret == 8) {
		size_t p = replystr.rfind("SM0");
		if (p != std::string::npos) {
			poutmtr = fm_decimal(replystr.substr(p+3),4);
			if (poutmtr <= 6) poutmtr = poutmtr * 2;
			else if (poutmtr <= 11) poutmtr = 11 + (poutmtr - 6)*(26 - 11)/(11 - 6);
			else if (poutmtr <= 18) poutmtr = 26 + (poutmtr - 11)*(50 - 26)/(18 - 11);
			else poutmtr = 50 + (poutmtr - 18)*(100 - 50)/(27 - 18);
			if (poutmtr > 100) poutmtr = 100;
		}
	}
	return poutmtr;
}

double RIG_TRXAVR::get_power_control()
{
	int pctrl = 0;
	cmd = "CG;";
	get_trace(1, "get_power_contro");
	ret = wait_char(';', 6, 100, "get pout", ASC);
	gett("");
	if (ret == 6) {
		size_t p = replystr.rfind("CG");
		if (p != std::string::npos) {
			pctrl = fm_decimal(replystr.substr(p+2), 3);
		}
	}
	return pctrl;
}

void RIG_TRXAVR::set_attenuator(int val)
{
	atten_state = val;
	if (val) cmd = "RA01;";
	else     cmd = "RA00;";
	sendCommand(cmd);
	showresp(WARN, ASC, "set ATT", cmd, "");
	sett("attenuator");
}

int RIG_TRXAVR::get_attenuator()
{
	cmd = "RA;";
	get_trace(1, "get_attenuator");
	ret = wait_char(';', 7, 100, "get ATT", ASC);
	gett("");
	if (ret == 7) {
		size_t p = replystr.rfind("RA");
		if (p != std::string::npos && (p+3 < replystr.length())) {
			if (replystr[p+2] == '0' && replystr[p+3] == '0')
				atten_state = 0;
			else
				atten_state = 1;
		}
	}
	return atten_state;
}

void RIG_TRXAVR::set_preamp(int val)
{
	preamp_state = val;
	if (val) cmd = "PA1;";
	else     cmd = "PA0;";
	sendCommand(cmd);
	showresp(WARN, ASC, "set PRE", cmd, "");
	sett("preamp");
}

int RIG_TRXAVR::get_preamp()
{
	cmd = "PA;";
	get_trace(1, "get_preamp");
	ret = wait_char(';', 5, 100, "get PRE", ASC);
	gett("");
	if (ret == 5) {
		size_t p = replystr.rfind("PA");
		if (p != std::string::npos && (p+2 < replystr.length())) {
			if (replystr[p+2] == '1')
				preamp_state = 1;
			else
				preamp_state = 0;
		}
	}
	return preamp_state;
}

int RIG_TRXAVR::set_widths(int val)
{
	int bw;
	switch (val) {
	default:
	case LSB: case USB:
		bandwidths_ = TRXAVR_SH;
		dsp_SL = TRXAVR_SL;
		SL_tooltip = TRXAVR_SL_tooltip;
		SL_label   = TRXAVR_SSB_btn_SL_label;
		dsp_SH = TRXAVR_SH;
		SH_tooltip = TRXAVR_SH_tooltip;
		SH_label   = TRXAVR_SSB_btn_SH_label;
		if (val == FM) bw = 0x8A03; // 200 ... 4000 Hz
		else bw = 0x8803; // 200 ... 3000 Hz
		break;
	case CW: case CWR:
		bandwidths_ = TRXAVR_CWwidths;
		dsp_SL = TRXAVR_empty;
		dsp_SH = TRXAVR_empty;
		bw = 7;
		break;
	}
	return bw;
}

std::vector<std::string>& RIG_TRXAVR::bwtable(int val)
{
	if (val == LSB || val == USB)
		return TRXAVR_SH;
	return TRXAVR_CWwidths;
}

std::vector<std::string>& RIG_TRXAVR::lotable(int val)
{
	if (val == LSB || val == USB)
		return TRXAVR_SL;
	return vNOBWS;
}

std::vector<std::string>& RIG_TRXAVR::hitable(int val)
{
	if (val == LSB || val == USB)
		return TRXAVR_SH;
	return vNOBWS;
}

void RIG_TRXAVR::set_modeA(int val)
{
	if (val >= (int)(sizeof(TRXAVR_mode_chr)/sizeof(*TRXAVR_mode_chr))) return;
	_currmode = A.imode = val;
	cmd = "MD";
	cmd += TRXAVR_mode_chr[val];
	cmd += ';';
	sendCommand(cmd);
	showresp(WARN, ASC, "set mode", cmd, "");
	sett("modeA");
	A.iBW = set_widths(val);
}

int RIG_TRXAVR::get_modeA()
{
	if (tuning()) return A.imode;
	if (skip_get) {
		skip_get--;
		return A.imode;
	}
	cmd = "MD;";
	get_trace(1, "get_modeA");
	ret = wait_char(';', 4, 100, "get mode A", ASC);
	gett("");
	if (ret == 4) {
		size_t p = replystr.rfind("MD");
		if (p != std::string::npos) {
			switch (replystr[p+2]) {
				default:
				case '1': A.imode = 0; break;
				case '2': A.imode = 1; break;
				case '3': A.imode = 2; break;
				case '7': A.imode = 3; break;
			}
			A.iBW = set_widths(A.imode);
		}
	}
	_currmode = A.imode;
	return A.imode;
}

void RIG_TRXAVR::set_modeB(int val)
{
	if (val >= (int)(sizeof(TRXAVR_mode_chr)/sizeof(*TRXAVR_mode_chr))) return;
	_currmode = B.imode = val;
	cmd = "MD";
	cmd += TRXAVR_mode_chr[val];
	cmd += ';';
	sendCommand(cmd);
	showresp(WARN, ASC, "set mode B", cmd, "");
	sett("modeB");
	B.iBW = set_widths(val);
}

int RIG_TRXAVR::get_modeB()
{
	if (tuning()) return B.imode;
	if (skip_get) return B.imode;
	cmd = "MD;";
	get_trace(1, "get_modeB");
	ret = wait_char(';', 4, 100, "get mode B", ASC);
	gett("");
	if (ret == 4) {
		size_t p = replystr.rfind("MD");
		if (p != std::string::npos) {
			switch (replystr[p+2]) {
				default:
				case '1': B.imode = 0; break;
				case '2': B.imode = 1; break;
				case '3': B.imode = 2; break;
				case '7': B.imode = 3; break;
			}
			B.iBW = set_widths(B.imode);
		}
	}
	_currmode = B.imode;
	return B.imode;
}

int RIG_TRXAVR::adjust_bandwidth(int val)
{
	int bw = 0;
	if (val == LSB || val == USB)
		bw = 0x8803;
	else if (val == FM)
		bw = 0x8A03;
	else if (val == AM)
		bw = 0x8301;
	else if (val == CW || val == CWR)
		bw = 7;
	else if (val == FSK || val == FSKR)
		bw = 1;
	return bw;
}

int RIG_TRXAVR::def_bandwidth(int val)
{
	return adjust_bandwidth(val);
}

void RIG_TRXAVR::set_bwA(int val)
{
	if (A.imode == LSB || A.imode == USB || A.imode == FM || A.imode == AM) {
		if (val < 256) return;
		A.iBW = val;
		int index = A.iBW & 0x7F;
		try {
			cmd = TRXAVR_CAT_SL[index];
			sendCommand(cmd);
			showresp(WARN, ASC, "set lower", cmd, "");
			sett("bwA lower");
		} catch (const std::exception& e) {
			std::cout << e.what() << '\n';
		}
		index = (A.iBW >> 8) & 0x7F;
		try {
			cmd = TRXAVR_CAT_SH[index];
			sendCommand(cmd);
			showresp(WARN, ASC, "set upper", cmd, "");
			sett("bwA upper");
		} catch (const std::exception& e) {
			std::cout << e.what() << '\n';
		}
	}
	if (val > 256) return;
	else if (A.imode == CW || A.imode == CWR) {
		A.iBW = val;
		int index = A.iBW & 0x7F;
		try {
			cmd = TRXAVR_CWbw[index];
			sendCommand(cmd);
			showresp(WARN, ASC, "set CW bw", cmd, "");
			sett("CW bw");
		} catch (const std::exception& e) {
			std::cout << e.what() << '\n';
		}
	}
}

int RIG_TRXAVR::get_bwA()
{
	if (tuning()) return A.iBW;
	if (skip_get) return A.iBW;
	size_t p;
	if (A.imode == LSB || A.imode == USB || A.imode == FM || A.imode == AM) {
		int lo = A.iBW & 0xFF, hi = (A.iBW >> 8) & 0x7F;
		cmd = "SL;";
		get_trace(1, "get SL");
		ret = wait_char(';', 5, 100, "get SL", ASC);
		gett("");
		if (ret == 5) {
			p = replystr.rfind("SL");
			if (p != std::string::npos)
				lo = fm_decimal(replystr.substr(2), 2);
		}
		cmd = "SH;";
		get_trace(1, "get SH");
		ret = wait_char(';', 5, 100, "get SH", ASC);
		gett("");
		if (ret == 5) {
			p = replystr.rfind("SH");
			if (p != std::string::npos)
				hi = fm_decimal(replystr.substr(2), 2);
			A.iBW = ((hi << 8) | (lo & 0xFF)) | 0x8000;
		}
	} else if (A.imode == CW || A.imode == CWR) { // CW
		cmd = "FW;";
		get_trace(1, "get FW");
		ret = wait_char(';', 7, 100, "get FW", ASC);
		gett("");
		if (ret == 7) {
			p = replystr.rfind("FW");
			if (p != std::string::npos) {
				try {
					for (A.iBW = 0; A.iBW < (int)TRXAVR_CWbw.size(); A.iBW++)
						if (replystr == TRXAVR_CWbw.at(A.iBW))
							break;
				} catch (const std::exception& e) {
					std::cout << e.what() << '\n';
				}
			}
		}
	}
	return A.iBW;
}

void RIG_TRXAVR::set_bwB(int val)
{
	if (B.imode == LSB || B.imode == USB) {
		if (val < 256) return;
		B.iBW = val;
		int index = B.iBW & 0x7F;
		try {
			cmd = TRXAVR_CAT_SL[index];
			sendCommand(cmd);
			showresp(WARN, ASC, "set lower", cmd, "");
			sett("bwB lower");
		} catch (const std::exception& e) {
			std::cout << e.what() << '\n';
		}
		index = (B.iBW >> 8) & 0x7F;
		try {
			cmd = TRXAVR_CAT_SH[index];
			sendCommand(cmd);
			showresp(WARN, ASC, "set upper", cmd, "");
			sett("bwB upper");
		} catch (const std::exception& e) {
			std::cout << e.what() << '\n';
		}
	}
	if (val > 256) return;
	else if (B.imode == CW || B.imode == CWR) {
		B.iBW = val;
		int index = B.iBW & 0x7F;
		try {
			cmd = TRXAVR_CWbw[index];
			sendCommand(cmd);
			showresp(WARN, ASC, "set CW bw", cmd, "");
			sett("bwB CW");
		} catch (const std::exception& e) {
			std::cout << e.what() << '\n';
		}
	}
}

int RIG_TRXAVR::get_bwB()
{
	if (tuning()) return B.iBW;
	if (skip_get) return B.iBW;
	size_t p;
	if (B.imode == LSB || B.imode == USB || B.imode == FM || B.imode == AM) {
		int lo = B.iBW & 0xFF, hi = (B.iBW >> 8) & 0x7F;
		cmd = "SL;";
		get_trace(1, "get SL");
		ret = wait_char(';', 5, 100, "get SL", ASC);
		gett("");
		if (ret == 5) {
			p = replystr.rfind("SL");
			if (p != std::string::npos)
				lo = fm_decimal(replystr.substr(2), 2);
		}
		cmd = "SH;";
		get_trace(1, "get SH");
		ret = wait_char(';', 5, 100, "get SH", ASC);
		gett("");
		if (ret == 5) {
			p = replystr.rfind("SH");
			if (p != std::string::npos)
				hi = fm_decimal(replystr.substr(2), 2);
			B.iBW = ((hi << 8) | (lo & 0xFF)) | 0x8000;
		}
	} else if (B.imode == CW || B.imode == CWR) {
		cmd = "FW;";
		get_trace(1, "get FW");
		ret = wait_char(';', 7, 100, "get FW", ASC);
		gett("");
		if (ret == 7) {
			p = replystr.rfind("FW");
			if (p != std::string::npos) {
				try {
					for (B.iBW = 0; B.iBW < (int)TRXAVR_CWbw.size(); B.iBW++)
						if (replystr == TRXAVR_CWbw.at(B.iBW))
							break;
				} catch (const std::exception& e) {
					std::cout << e.what() << '\n';
				}
			}
		}
	}
	return B.iBW;
}

int RIG_TRXAVR::get_modetype(int n)
{
	if (n >= (int)(sizeof(TRXAVR_mode_type)/sizeof(*TRXAVR_mode_type))) return 0;
	return TRXAVR_mode_type[n];
}

void RIG_TRXAVR::get_if_min_max_step(int &min, int &max, int &step)
{
	if_shift_min = min = 400;
	if_shift_max = max = 1000;
	if_shift_step = step = 50;
	if_shift_mid = 700;
}

void RIG_TRXAVR::set_notch(bool on, int val)
{
	if (on) {
		cmd = "BC1;"; // set manual notch
		sendCommand(cmd);
		showresp(WARN, ASC, "set notch on", cmd, "");
		sett("notch ON");
		cmd = "BP";
		val = round((val - 200) / 50);
		cmd.append(to_decimal(val, 3)).append(";");
		sendCommand(cmd);
		showresp(WARN, ASC, "set notch val", cmd, "");
		sett("notch val");
	} else {
		cmd = "BC0;"; // no notch action
		sendCommand(cmd);
		showresp(WARN, ASC, "set notch off", cmd, "");
		sett("notch OFF");
	}
}

bool  RIG_TRXAVR::get_notch(int &val)
{
	bool ison = false;
	cmd = "BC;";

	get_trace(1, "get_notch_on_off");
	ret = wait_char(';', 4, 100, "get notch on/off", ASC);
	gett("");

	if (ret == 4) {
		size_t p = replystr.rfind("BC");
		if (p != std::string::npos) {
			if (replystr[p+2] == '2') {
				ison = true;
				cmd = "BP;";

				get_trace(1, "get_notch_val");
				ret = wait_char(';', 6, 100, "get notch val", ASC);
				gett("");

				if (ret == 6) {
					gett("notch val");
					p = replystr.rfind("BP");
					if (p != std::string::npos)
						val = 200 + 50 * fm_decimal(replystr.substr(p+2),3);
				}
			}
		}
	}
	return (ison);
}

void RIG_TRXAVR::get_notch_min_max_step(int &min, int &max, int &step)
{
	min = 0;
	max = 46;
	step = 1;
}

void RIG_TRXAVR::set_auto_notch(int v)
{
	cmd = v ? "NT1;" : "NT0;";
	sendCommand(cmd);
	showresp(WARN, ASC, "set auto notch", cmd, "");
	sett("auto notch");
}

int  RIG_TRXAVR::get_auto_notch()
{
	int anotch = 0;
	cmd = "NT;";

	get_trace(1, "get_auto_notch");
	ret = wait_char(';', 4, 100, "get auto notch", ASC);
	gett("");

	if (ret == 4) {
		size_t p = replystr.rfind("NT");
		if (p != std::string::npos) {
			anotch = (replystr[p+2] == '1');
		}
	}
	return anotch;
}

void RIG_TRXAVR::set_noise_reduction(int val)
{
	if (val == -1) {
		return;
	}
	nr_state = val;
	if (nr_state == 0) {
		noise_reduction_label(nr_label(), false);
	} else if (nr_state == 1) {
		noise_reduction_label(nr_label(), true);
	} else if (nr_state == 2) {
		noise_reduction_label(nr_label(), true);
	}
	cmd.assign("NR");
	cmd += '0' + nr_state;
	cmd += ';';
	sendCommand (cmd);
	showresp(WARN, ASC, "SET noise reduction", cmd, "");
	sett("noise reduction");
}

int  RIG_TRXAVR::get_noise_reduction()
{
	cmd = rsp = "NR";
	cmd.append(";");

	get_trace(1, "get_noise_reduction");
	ret = wait_char(';', 4, 100, "GET noise reduction", ASC);
	gett("");

	if (ret == 4) {
		size_t p = replystr.rfind(rsp);
		if (p == std::string::npos) return nr_state;
		nr_state = replystr[p+2] - '0';
	}
	if (replystr == "?;") {
		nr_state = 0;
		return 0;
	}

	if (nr_state == 1) {
		noise_reduction_label(nr_label(), true);
	} else if (nr_state == 2) {
		noise_reduction_label(nr_label(), true);
	} else {
		noise_reduction_label(nr_label(), false);
	}
	return nr_state;
}

void RIG_TRXAVR::set_noise_reduction_val(int val)
{
	if (nr_state == 0) return;
	if (nr_state == 1) _nrval1 = val;
	else _nrval2 = val;

	cmd.assign("RL").append(to_decimal(val, 2)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "SET_noise_reduction_val", cmd, "");
	sett("noise reduction val");
}

int  RIG_TRXAVR::get_noise_reduction_val()
{
	int nrval = 0;
	if (nr_state == 0) return 0;
	int val = progStatus.noise_reduction_val;
	cmd = rsp = "RL";
	cmd.append(";");

	get_trace(1, "get_noise_reduction_val");
	ret = wait_char(';', 5, 100, "GET noise reduction val", ASC);
	gett("");

	if (ret == 5) {
		size_t p = replystr.rfind(rsp);
		if (p == std::string::npos) {
			nrval = (nr_state == 1 ? _nrval1 : _nrval2);
			return nrval;
		}
		val = atoi(&replystr[p+2]);
	}

	if (nr_state == 1) _nrval1 = val;
	else _nrval2 = val;

	return val;
}


//====================== vfo controls

void RIG_TRXAVR::selectA()
{
	cmd = "FR0;FT0;";
	sendCommand(cmd);
	showresp(WARN, ASC, "Rx on A, Tx on A", cmd, "");
	inuse = onA;
}

void RIG_TRXAVR::selectB()
{
	cmd = "FR1;FT1;";
	sendCommand(cmd);
	showresp(WARN, ASC, "Rx on B, Tx on B", cmd, "");
	inuse = onB;
}

unsigned long long RIG_TRXAVR::get_vfoA ()
{
	cmd = "FA;";
	if (wait_char(';', 14, 100, "get vfo A", ASC) < 14) return A.freq;

	size_t p = replystr.rfind("FA");
	if (p != std::string::npos && (p + 12 < replystr.length())) {
		unsigned long long f = 0;
		for (size_t n = 2; n < 13; n++)
			f = f*10 + replystr[p+n] - '0';
		A.freq = f;
	}
	return A.freq;
}

void RIG_TRXAVR::set_vfoA (unsigned long long freq)
{
	A.freq = freq;
	cmd = "FA00000000000;";
	for (int i = 12; i > 1; i--) {
		cmd[i] += freq % 10;
		freq /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "set vfo A", cmd, "");
}

unsigned long long RIG_TRXAVR::get_vfoB ()
{
	cmd = "FB;";
	if (wait_char(';', 14, 100, "get vfo B", ASC) < 14) return B.freq;

	size_t p = replystr.rfind("FB");
	if (p != std::string::npos && (p + 12 < replystr.length())) {
		unsigned long long f = 0;
		for (size_t n = 2; n < 13; n++)
			f = f*10 + replystr[p+n] - '0';
		B.freq = f;
	}
	return B.freq;
}

void RIG_TRXAVR::set_vfoB (unsigned long long freq)
{
	B.freq = freq;
	cmd = "FB00000000000;";
	for (int i = 12; i > 1; i--) {
		cmd[i] += freq % 10;
		freq /= 10;
	}
	sendCommand(cmd);
	showresp(WARN, ASC, "set vfo B", cmd, "");
}

void RIG_TRXAVR::set_split(bool val) 
{
	split = val;
	if (val) cmd = "OS1;";
	else     cmd = "OS0;";
	sendCommand(cmd);
}

int RIG_TRXAVR::get_split()
{
	cmd = "OS;";
	wait_char(';', 4, 100, "get split", ASC);
	if (replystr.find("OS1;") != std::string::npos) return 1;
	return 0;
}

void RIG_TRXAVR::set_PTT_control(int val)
{
	if (val) cmd = "TX;"; //"TX1;";
	else     cmd = "RX;"; //"TX0;";
	sendCommand(cmd);
}

