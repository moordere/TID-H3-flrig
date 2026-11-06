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

#include "qrp_labs/QMX.h"
#include "support.h"

static const char QMXname_[] = "QMX";

static std::vector<std::string>vQMXmodes_;
static const char *QMXmodes_[] = {
	"LSB",   "USB", "CW-U", "CW-L", "DIGIU", "DIGIL" };
static const char QMX_mode_type[] = { 'L', 'U' };
static const char *QMX_mode_str[] = {
	"MD1;", "MD2;", "MD3;", "MD7;", "MD6;", "MD9;", NULL };

static std::vector<std::string>vQMX_BW;
static const char *QMX_BW[] = {
  "50", "100", "150", "200", "250", "300", "400", "500", 
  "2500", "2700", "2900", "3200" };

static GUI rig_widgets[]= {
	{ (Fl_Widget *)btnVol,        2, 125,  50 }, // 0
	{ (Fl_Widget *)sldrVOLUME,   54, 125, 156 }, // 1
	{ (Fl_Widget *)sldrRFGAIN,   54, 145, 156 }, // 2
	{ (Fl_Widget *)NULL,          0,   0,   0 }
};

void RIG_QMX::initialize()
{
	VECTOR (vQMXmodes_, QMXmodes_);
	VECTOR (vQMX_BW, QMX_BW);

	modes_ = vQMXmodes_;
	bandwidths_ = vQMX_BW;

	rig_widgets[0].W = btnVol;
	rig_widgets[1].W = sldrVOLUME;
	rig_widgets[2].W = sldrRFGAIN;

//	setVfoAdj(progStatus.vfo_adj);

}

RIG_QMX::RIG_QMX() {

	name_ = QMXname_;
	modes_ = vQMXmodes_;
	bandwidths_ = vQMX_BW;

	widgets = rig_widgets;

	serial_baudrate = BR9600;
	stopbits = 1;
	serial_retries = 2;
	serial_rtscts  = false;
	serial_rtsplus = false;
	serial_dtrplus = false;
	serial_rtsptt  = false;
	serial_dtrptt  = false;

	serial_write_delay = 0;
	serial_post_write_delay = 0;
	serial_timeout = 50;

	serial_catptt  = true;

	B.imode = A.imode = 1;
	B.iBW = A.iBW = 6;
	A.freq = 7070000ULL;
	B.freq = 14070000ULL;

	has_smeter =
	has_swr_control =
	has_mode_control =
	has_bandwidth_control =
	has_extras =
	has_rf_control =
	has_volume_control =
	has_ptt_control =
	has_vfo_adj =
	has_vox_onoff =
	has_rit =
	has_split =
	can_change_alt_vfo =
	has_cw_vol = 
	has_cw_spot_tone = 
	has_power_out = true;

	can_synch_clock = true;

	precision = 1;
	ndigits = 8;

}

static int ret = 0;
static int split_ = 0;
static int rit_ = 0;
static int rit_stat_ = 0;

static char cmdstr[20];

inline void TRACE_SET(std::string func, std::string cmd )
{
//	std::cout << ztime() << "::" << func << " sent: " << cmd << std::endl;
}

inline void TRACE_CMD(std::string func, std::string cmd )
{
//	std::cout << ztime() << "::" << func << " sent: " << cmd << std::endl;
}

inline void TRACE_REP(std::string func, std::string replystr)
{
//	std::cout << ztime() << "::" << func << " rcvd: " << replystr << std::endl;
}

void RIG_QMX::shutdown()
{
}

bool RIG_QMX::check()
{
	cmd = "VN;";  // version number query; re VN1_05;
	get_trace(1, "get version");

	TRACE_CMD("check", cmd);

	wait_char(';', 8, 100, "get version", ASC);
	gett("");

//	if (replystr.find("VN") == std::string::npos) return 0;

	TRACE_REP("check", replystr);

	return 1;
}

unsigned long long RIG_QMX::get_vfoA ()
{
	cmd = "FA;";
	get_trace(1, "get vfoA");

	TRACE_CMD("get_vfoA", cmd);

	if (wait_char(';', 14, 100, "get vfo A", ASC) < 14) {

		TRACE_REP("get_vfoA", replystr);

		return A.freq;
	}
	gett("");

	size_t p = replystr.rfind("FA");

	if (p != std::string::npos) {
		sscanf(&replystr[p+2], "%llu", &A.freq);
	}

	TRACE_REP("get_vfoA", replystr);

	return A.freq;
}

void RIG_QMX::set_vfoA (unsigned long long freq)
{
	A.freq = freq;
	snprintf(cmdstr, sizeof(cmdstr), "FA%011llu;", A.freq);
	cmd = cmdstr;
	set_trace(1, "set vfoA");

	TRACE_CMD("set_vfoA", cmd);

	sendCommand(cmd);
	sett("");

	TRACE_REP("set_vfoA", replystr);

	showresp(WARN, ASC, "set vfo A", cmd, "");
}

unsigned long long RIG_QMX::get_vfoB ()
{
	cmd = "FB;";
	get_trace(1, "get vfoB");

	TRACE_CMD("get_vfoB", cmd);

	if (wait_char(';', 14, 100, "get vfo B", ASC) < 14) {

		TRACE_REP("get_vfoB", replystr);

		return B.freq;
	}
	gett("");

	size_t p = replystr.rfind("FB");

	if (p != std::string::npos) {
		sscanf(&replystr[p+2], "%llu", &B.freq);
	}

	TRACE_REP("get_vfoB", replystr);

	return B.freq;
}

void RIG_QMX::set_vfoB (unsigned long long freq)
{
	B.freq = freq;
	snprintf(cmdstr, sizeof(cmdstr), "FB%011llu;", B.freq);
	cmd = cmdstr;
	set_trace(1, "set vfoB");

	TRACE_CMD("set_vfoB", cmd);

	sendCommand(cmd);

	sett("");

	TRACE_REP("set_vfoB", replystr);

	showresp(WARN, ASC, "set vfo B", cmd, "");
}

void RIG_QMX::selectA()
{
	cmd = "FR0;FT0;";
	set_trace(1, "select A");

	TRACE_CMD("selectA", cmd);

	sendCommand(cmd);
	sett("");

	showresp(WARN, ASC, "Rx on A, Tx on A", cmd, "");
	inuse = onA;

	TRACE_REP("selectA", replystr);

}

void RIG_QMX::selectB()
{
	cmd = "FR1;FT1;";
	set_trace(1, "select B");

	TRACE_CMD("selectB", cmd);

	sendCommand(cmd);
	sett("");

	showresp(WARN, ASC, "Rx on B, Tx on B", cmd, "");
	inuse = onB;

	TRACE_REP("selectB", replystr);

}

void RIG_QMX::set_split(bool val) 
{
	if (val) cmd = "SP1;";
	else     cmd = "SP0;";
	set_trace(1, "set split on/off");

	TRACE_CMD("set_split", cmd);

	sendCommand(cmd);
	sett("");

	showresp(WARN, ASC, "set split", cmd, "");
	split_ = val;

	TRACE_REP("set_split", replystr);

}

bool RIG_QMX::can_split()
{
	return true;
}

int RIG_QMX::get_split()
{
	cmd = "SP;";
	get_trace(1, "get split");

	TRACE_CMD("get_split", cmd);

	ret = wait_char(';', 4, 100, "get split", ASC);
	gett("");

	split_ = replystr[2] == '1';

	TRACE_REP("get_split", replystr);

	return split_;
}

int RIG_QMX::def_bandwidth(int mode)
{
	switch (mode) {
		case 0: // USB
			return 8;
		case 1: // CW
		case 2: // CW-R
			return 6;
		case 3: // FSK-U
		case 4: // FSK-L
			return 7;
	}
	return 6;
}

void RIG_QMX::set_bwA(int val)
{
	char setcmd[50];

	if (A.imode == 2 || A.imode == 3)
		snprintf(setcmd, sizeof(setcmd), "MMCW|CW passband=%s;",
			(val == 0 ? "50" :
			(val == 1 ? "100" :
			(val == 2 ? "150" :
			(val == 3 ? "200" :
			(val == 4 ? "250" :
			(val == 5 ? "300" :
			(val == 6 ? "400" :
			(val == 7 ? "500" : "400" )))))))));

	else if (A.imode == 0 || A.imode == 1)
		snprintf(setcmd, sizeof(setcmd), "MMSSB|FILTER RX=%s;",
			(val == 8 ? "2500" : 
			(val == 9 ? "2700" :
			(val == 10 ? "2900" : 
			(val == 11 ? "3200" : "2700" )))));

	else return;

	cmd = setcmd;

	set_trace(1, "set bwA");
//	TRACE_SET("set_bwA", cmd);

	sendCommand(cmd);
}

int RIG_QMX::get_bwA()
{
	cmd = "FW;";
//	TRACE_CMD("get_bw", cmd);

	ret = wait_char(';', 7, 100, "get bw", ASC);

	gett("");

	if (replystr.find("0050;") != std::string::npos) return A.iBW = 0;
	if (replystr.find("0100;") != std::string::npos) return A.iBW = 1;
	if (replystr.find("0150;") != std::string::npos) return A.iBW = 2;
	if (replystr.find("0200;") != std::string::npos) return A.iBW = 3;
	if (replystr.find("0250;") != std::string::npos) return A.iBW = 4;
	if (replystr.find("0300;") != std::string::npos) return A.iBW = 5;
	if (replystr.find("0400;") != std::string::npos) return A.iBW = 6;
	if (replystr.find("0500;") != std::string::npos) return A.iBW = 7;
	if (replystr.find("2500;") != std::string::npos) return A.iBW = 8;
	if (replystr.find("2700;") != std::string::npos) return A.iBW = 9;
	if (replystr.find("2900;") != std::string::npos) return A.iBW = 10;
	if (replystr.find("3200;") != std::string::npos) return A.iBW = 11;

	return A.iBW = 6;
}

void RIG_QMX::set_bwB(int val)
{
	char setcmd[50];

	if (B.imode == 2 || B.imode == 3)
		snprintf(setcmd, sizeof(setcmd), "MMCW|CW passband=%s;",
			(val == 0 ? "50" :
			(val == 1 ? "100" :
			(val == 2 ? "150" :
			(val == 3 ? "200" :
			(val == 4 ? "250" :
			(val == 5 ? "300" :
			(val == 6 ? "400" :
			(val == 7 ? "500" : "400" )))))))));

	else if (B.imode == 0 || B.imode == 1)
		snprintf(setcmd, sizeof(setcmd), "MMSSB|FILTER RX=%s;",
			(val == 8 ? "2500" : 
			(val == 9 ? "2700" :
			(val == 10 ? "2900" : 
			(val == 11 ? "3200" : "2700" )))));

	else return;
	cmd = setcmd;

	set_trace(1, "set bwB");
	TRACE_SET("set_bwB", cmd);

	sendCommand(cmd);
}

int RIG_QMX::get_bwB()
{
	cmd = "FW;";
	TRACE_CMD("get_bw", cmd);
	ret = wait_char(';', 7, 100, "get bw", ASC);
	gett("");
	if (replystr.find("0050;") != std::string::npos) return B.iBW = 0;
	if (replystr.find("0100;") != std::string::npos) return B.iBW = 1;
	if (replystr.find("0150;") != std::string::npos) return B.iBW = 2;
	if (replystr.find("0200;") != std::string::npos) return B.iBW = 3;
	if (replystr.find("0250;") != std::string::npos) return B.iBW = 4;
	if (replystr.find("0300;") != std::string::npos) return B.iBW = 5;
	if (replystr.find("0400;") != std::string::npos) return B.iBW = 6;
	if (replystr.find("0500;") != std::string::npos) return B.iBW = 7;
	if (replystr.find("2500;") != std::string::npos) return B.iBW = 8;
	if (replystr.find("2700;") != std::string::npos) return B.iBW = 9;
	if (replystr.find("2900;") != std::string::npos) return B.iBW = 10;
	if (replystr.find("3200;") != std::string::npos) return B.iBW = 11;

	return B.iBW = 6;
}

// Tranceiver PTT on/off
void RIG_QMX::set_PTT_control(int val)
{
	if (val) cmd = "TQ1;";
	else     cmd = "TQ0;";
	set_trace(1, "set PTT");

	TRACE_CMD("set_PTT", cmd);

	sendCommand(cmd);
	sett("");

	showresp(WARN, ASC, "set PTT", cmd, "");
	ptt_ = val;

	TRACE_REP("set_PTT", replystr);

}

#include "cwio.h"

int RIG_QMX::get_PTT()
{
	if (cwio_process == SEND) return 1;

	cmd = "TQ;";
	get_trace(1, "get PTT");

	TRACE_CMD("get_PTT", cmd);

	ret = wait_char(';', 4, 100, "get PTT", ASC);
	gett("");

	TRACE_REP("get_PTT", replystr);

	if (ret < 4) return ptt_;
	return ptt_ = replystr[2] == '1';
}

/*======================================================================

IF; response

IFaaaaaaaaaaaXXXXXbbbbbcdXeefghjklmmX;
12345678901234567890123456789012345678
01234567890123456789012345678901234567 byte #
          1         2         3
                            ^ position 28
where:
	aaaaaaaaaaa => 11 digit decimal value of vfo frequency
	XXXXX => 5 spaces
	bbbbb => 5 digit RIT frequency, -0200 ... +0200
	c => 0 = rit OFF, 1 = rit ON
	d => xit off/on; always ZERO
	X    always ZERO
	ee => memory channel; always 00
	f => tx/rx; 0 = RX, 1 = TX
	g => mode; always 3 (CW)
	h => function; receive VFO; 0 = A, 1 = B
	j => scan off/on; always ZERO
	k => split off /on; 0 = Simplex, 1 = Split
	l => tone off /on; always ZERO
	m => tone number; always ZERO
	X => unused characters; always a SPACE
======================================================================*/ 

int RIG_QMX::get_IF()
{
	cmd = "IF;";
	get_trace(1, "get_PTT");

	TRACE_CMD("get_IF", cmd);

	ret = wait_char(';', 38, 100, "get VFO", ASC);
	gett("");

	if (ret < 38) return ptt_;

	rit_ = 0;
	for (int n = 22; n > 18; n--)
		rit_ = (10 * rit_) + (replystr[n] - '0');
	if (replystr[18] == '-') rit_ *= -1;
	rit_stat_ = (replystr[23] - '0');

	ptt_ = (replystr[28] == '1');

	split_ = (replystr[32] == '1');

	TRACE_REP("get_IF", replystr);

	return ptt_;
}

void RIG_QMX::set_modeA(int val)
{
	if (val < 0 || val > 5) return;
	cmd = QMX_mode_str[val];
	set_trace(1, "set mode A");

	TRACE_SET("set_modeA", cmd);

	sendCommand(cmd);
	sett("");
	showresp(WARN, ASC, "set mode A", cmd, "");
	A.imode = val;

	TRACE_REP("set_modeA", replystr);

}

int RIG_QMX::get_modeA()
{
	cmd = "MD;";
	get_trace(1, "get_modeA");

	TRACE_CMD("get_modeA", cmd);

	ret = wait_char(';', 4, 100, "get modeA", ASC);
	gett("");

	TRACE_REP("get_modeA", replystr);

	if (ret < 5) return A.imode;
	for (int i = 0; i < 6; i++)
		if (replystr.find(QMX_mode_str[i]) != std::string::npos)
			return A.imode = i;
	return A.imode;
}

void RIG_QMX::set_modeB(int val)
{
	if (val < 0 || val > 5) return;
	cmd = QMX_mode_str[val];
	set_trace(1, "set mode B");

	TRACE_SET("set_modeB", cmd);

	sendCommand(cmd);
	sett("");
	showresp(WARN, ASC, "set mode B", cmd, "");
	B.imode = val;

	TRACE_REP("set_modeB", replystr);
}

int RIG_QMX::get_modeB()
{
	cmd = "MD;";
	get_trace(1, "get_modeB");

	TRACE_CMD("get_modeB", cmd);

	ret = wait_char(';', 4, 100, "get modeB", ASC);
	gett("");

	TRACE_REP("get_modeB", replystr);

	if (ret < 5) return B.imode;
	for (int i = 0; i < 6; i++)
		if (replystr.find(QMX_mode_str[i]) != std::string::npos)
			return B.imode = i;
	return B.imode;
}

void RIG_QMX::setVfoAdj(double v)
{
	v += 25000000;

	if (v > 25001000) v = 25001000;
	if (v < 24999000) v = 24999000;

	char cmdstr[12];
	snprintf(cmdstr, sizeof(cmdstr), "Q0%08.0f;", v);
	cmd = cmdstr;
	set_trace(1, "set TCXO ref freq");

	TRACE_CMD("setVfoAdj", cmd);

	sendCommand(cmd);
	sett("");

	TRACE_REP("setVfoAdj", replystr);

}

double RIG_QMX::getVfoAdj()
{
	cmd = "Q0;";
	get_trace(1, "get TCXO ref freq");

	TRACE_CMD("getVfoAdj", cmd);

	ret = wait_char(';', 12, 100, "get TCXO ref freq", ASC);

	TRACE_REP("getVfoAdj", replystr);

	if (ret < 11) return vfo_;
	int vfo;
	sscanf( (&replystr[2]), "%d", &vfo);
	return vfo_ = vfo - 25000000;
}

void RIG_QMX::get_vfoadj_min_max_step(double &min, double &max, double &step)
{
	min = -1000; 
	max = 1000; 
	step = 1;
}

void RIG_QMX::set_vox_onoff()
{
	if (progStatus.vox_onoff) cmd = "Q31;";
	else                      cmd = "Q30;";
	set_trace(1, "set vox on/off");

	TRACE_CMD("set_vox_onoff", cmd);

	sendCommand(cmd);
	sett("");

	TRACE_REP("set_vox_onoff", replystr);

	showresp(WARN, ASC, "SET vox gain", cmd, replystr);
}

int RIG_QMX::get_vox_onoff()
{
	cmd = "Q3;";
	get_trace(1, "get vox onoff");

	TRACE_CMD("get_vox_onoff", cmd);

	ret = wait_char(';', 4, 100, "get vox on/off", ASC);
	gett("");

	TRACE_REP("get_vox_onoff", replystr);

	if (ret < 4) return progStatus.vox_onoff;
	progStatus.vox_onoff = (replystr[2] == '1');
	return progStatus.vox_onoff;
}

void RIG_QMX::set_volume_control(int val)
{
	char szval[20];
	snprintf(szval, sizeof(szval), "AG%03d", val * 4);
	cmd += szval;
	cmd += ';';
	set_trace(1, "set vol control");

	TRACE_CMD("set_vol", cmd);

	sendCommand(cmd);
	sett("");

	TRACE_REP("set_vol", replystr);

}

int RIG_QMX::get_volume_control()
{
	int val = progStatus.volume;
	cmd = "AG;";
	get_trace(1, "get vol control");

	TRACE_CMD("get_vol", cmd);

	ret = wait_char(';', 6, 100, "get vol", ASC);
	gett("");

	TRACE_REP("get_vol", replystr);

	size_t p = replystr.rfind("AG");
	if (p == std::string::npos) return val;
	sscanf(replystr.c_str(), "AG%d", &val);
	return val / 4;
}

void RIG_QMX::get_vol_min_max_step(int &min, int &max, int &step)
{
	min = 0;
	max = 100;
	step = 1;
}

void RIG_QMX::set_rf_gain(int val)
{
	cmd = "RG";
	cmd.append(to_decimal(val,3)).append(";");
	sendCommand(cmd);
	showresp(WARN, ASC, "set rf gain", cmd, "");
}

int  RIG_QMX::get_rf_gain()
{
	int val = progStatus.rfgain;
	cmd = "RG;";
	get_trace(1, "get_rf_gain");
	ret = wait_char(';', 6, 100, "get rf gain", ASC);
	gett("");
	if (ret < 6) return val;

	size_t p = replystr.rfind("RG");
	if (p != std::string::npos)
		val = fm_decimal(replystr.substr(p+2), 3);
	return val;
}

void RIG_QMX::get_rf_min_max_step(int &min, int &max, int &step)
{
	min = 0;
	max = 100;
	step = 1;
}

void RIG_QMX::setRit(int val)
{
	if (val >= 0) snprintf(cmdstr, sizeof(cmdstr), "RU%u;", val);
	else if (val < 0) snprintf(cmdstr, sizeof(cmdstr), "RD-%u;", abs(val));
	rit_ = val;
	cmd = cmdstr;
	set_trace(1, "set RIT");

	TRACE_CMD("setRit", cmd);

	sendCommand(cmd);
	sett("");

	TRACE_REP("setRit", replystr);

}

int RIG_QMX::getRit()
{
	get_IF();
	return rit_;
}

void RIG_QMX::set_cw_vol()
{
	char setcmd[50];
	snprintf(setcmd, sizeof(setcmd), "MMCW|SIDETONE VOLUME=%d;", progStatus.cw_vol);
	TRACE_SET( "set_cw_vol", setcmd );
	sendCommand(setcmd);
}

int RIG_QMX::get_cw_vol()
{
	cmd = "MMCW|SIDETONE VOLUME;";
	get_trace(1, "get_CW volume");
	ret = wait_char(';', 6, 100, "get cw vol", ASC);
	gett("");
	if (ret < 6) return progStatus.cw_vol;
	int vol = 0;
	sscanf(replystr.c_str(), "MM%d;", &vol);
	return vol;
}

static int qrp_power;

int RIG_QMX::get_power_out()
{
	cmd = "PC;";
	get_trace(1, "get_power_out");
	ret = wait_char(';', 5, 100, "get power out", ASC);
	gett("");
	if (ret < 5) return 25;
	int pwr = 40;
	sscanf(replystr.c_str(), "PC%d;", &pwr);
	if (pwr > qrp_power) qrp_power = pwr;
	else qrp_power *= 0.95;
	return qrp_power;
}

int RIG_QMX::get_smeter()
{
	cmd = "SM;";
	get_trace(1, "get_smeter");
	ret = wait_char(';', 6, 100, "get Smeter", ASC);
	gett("");
	if (ret < 6) return 50;
	int mtr = 50;
	sscanf(replystr.c_str(), "SM%d;", &mtr);
// gives approximately same scale reading as IC7300 in 400 Hz bandwidth
	return int(mtr * 0.5);
}

int qrp_swr = 100;

int RIG_QMX::get_swr()
{
	cmd = "Sw;";
	get_trace(1, "get_smeter");
	ret = wait_char(';', 6, 100, "get Smeter", ASC);
	gett("");
	if (ret < 6) return (qrp_swr - 1) * 50 / 200;
	int mtr = 0;
	sscanf(replystr.c_str(), "Sw%d;", &mtr);
// mtr ranges from 1.0 to infinity, 3.0 is mid scale
	if (mtr > qrp_swr) qrp_swr = mtr;
	else qrp_swr *= 0.95;
	return (qrp_swr- 1) * 50 / 200;
}

// ---------------------------------------------------------------------
// tm formated as HH:MM:SS
// ---------------------------------------------------------------------
void RIG_QMX::sync_clock(char *tm)
{
	cmd.assign("Tm");
	cmd += tm[0]; cmd += tm[1];
	cmd += tm[3]; cmd += tm[4];
	cmd += tm[6]; cmd += tm[7];
	cmd += ';';
	sendCommand(cmd);
	showresp(WARN, ASC, "sync_time", cmd, replystr);
	sett("sync_time");
}
