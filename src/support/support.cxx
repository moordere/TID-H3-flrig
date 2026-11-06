// ----------------------------------------------------------------------------
// Copyright (C) 2014-2011
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

#include "config.h"
#include "compat.h" // Must precede all FL includes

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>

#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Widget.H>
#include <FL/Enumerations.H>

#include "icons.h"
#include "support.h"
#include "debug.h"
#include "gettext.h"
#include "rig_io.h"
#include "dialogs.h"
#include "rigbase.h"
#include "ptt.h"
#include "socket_io.h"
#include "ui.h"
#include "tod_clock.h"
#include "rig.h"
#include "rigs.h"
#include "K3_ui.h"
#include "KX3_ui.h"
#include "K4_ui.h"
#include "rigpanel.h"
#include "tod_clock.h"
#include "trace.h"
#include "cwio.h"
#include "cwioUI.h"
#include "fsk.h"
#include "fskioUI.h"
#include "xml_server.h"
#include "gpio_ptt.h"
#include "cmedia.h"
#include "tmate2.h"

//void initTabs();

rigbase *selrig = rigs[0];

extern bool test;
void init_notch_control();

int freqval = 0;

XCVR_STATE vfoA		= XCVR_STATE(14070000, 0, 0, UI);
XCVR_STATE vfoB		= XCVR_STATE(7070000, 0, 0, UI);
XCVR_STATE *vfo		= &vfoA;

//XCVR_STATE xmlvfo	= XCVR_STATE(14070000, 0, 0, UI);

XCVR_STATE xcvr_vfoA, xcvr_vfoB;

enum {VOL, MIC, PWR, SQL, IFSH, NOTCH, RFGAIN, NR, NB };

//std::queue<VFOQUEUE> srvc_reqs;

// Add alpha-tag to XCVR_STATE;
struct ATAG_XCVR_STATE {
	unsigned long long freq;
	int  imode;
	int  iBW;
	int  src;
	char alpha_tag[ATAGSIZE];
};
ATAG_XCVR_STATE oplist[LISTSIZE];

int  numinlist = 0;
std::vector<std::string> rigmodes_;

Cserial *RigSerial;
Cserial *AuxSerial;
Cserial *SepSerial;

bool using_buttons = false;

int meter_image = SWR_IMAGE;

bool xcvr_online = false;

// meter values passed to display functions

double smtrval = 0;
double pwrval = 0;
double swrval = 0;
double alcval = 0;
double iddval = 0;
double vmtrval = 0;

//======================================================================
// slider change processing
//======================================================================
int inhibit_nb_level = 0;
int inhibit_volume = 0;
int inhibit_pbt = 0;
int inhibit_shift = 0;
int inhibit_nr = 0;
int inhibit_notch = 0;
int inhibit_power = 0;
int inhibit_mic = 0;
int inhibit_rfgain = 0;
int inhibit_squelch = 0;

struct SLIDER {
enum {NOTCH, SHIFT, INNER, OUTER, LOCK, VOLUME, MIC, POWER, SQUELCH, RFGAIN, NB_LEVEL, NR, NR_VAL};
	int  fnc;
	int  val;
};

//======================================================================
// loop for serial i/o thread
// runs continuously until program is closed
// only accesses the serial port if it has been successfully opened
//======================================================================

bool bypass_serial_thread_loop = true;
bool run_serial_thread = true;

bool PTT = false;
bool TUNE = false;

int  powerlevel = 0;

std::string printXCVR_STATE(XCVR_STATE data)
{
	try {
		std::stringstream str;

		str << data.freq;
		str << ", " << selrig->modes_.at(data.imode);
		if (selrig->has_dsp_controls) {
			std::vector<std::string>& dsplo = selrig->lotable(data.imode);
			std::vector<std::string>& dsphi = selrig->hitable(data.imode);
			if (data.iBW > 256) {
				str << ", " << dsplo.at(data.iBW & 0x7F);
				str << ", " << dsphi.at((data.iBW >> 8) & 0x7F);
			}
		} else {
			std::vector<std::string>& bwt = selrig->bwtable(data.imode);
			str << ", " << bwt.at(data.iBW);
		}
		str << std::endl;
		return str.str();

	} catch (const std::exception& e) {
		LOG_ERROR("%s", e.what());
	}
	return "";
}

std::string print_ab()
{
	std::string s;
	s.assign("VFO-A: ");
	s.append(printXCVR_STATE(vfoA));
	s.append("VFO-B: ");
	s.append(printXCVR_STATE(vfoB));
	return s;
}

const char *print(XCVR_STATE data)
{
	static std::string prstr;
	static char str[1024];

	try {
		std::vector<std::string>& bwt = selrig->bwtable(data.imode);
		std::vector<std::string>& dsplo = selrig->lotable(data.imode);
		std::vector<std::string>& dsphi = selrig->hitable(data.imode);

		str[0] = 0;
		prstr.clear();
		snprintf(
			str, sizeof(str), "\
Data Source: %s\n\
  freq ........... %llu\n\
  mode ........... %d [%s]\n",
				data.src == XML ? "XML" : data.src == UI ? "UI" :
					data.src == SRVR ? "SRVR" : "RIG",
				data.freq,
				data.imode,
				selrig->modes_.at(data.imode).c_str());
		prstr.assign(str);
		str[0] = 0;
		if (selrig->has_FILTER) {
			snprintf(str, sizeof(str), "\
  filter ......... %s\n\
  bwt index ...... %2d, [%s] [%s]\n",
				selrig->FILT(selrig->get_FILT(data.imode)),
					data.iBW,
					(data.iBW > 256 && selrig->has_dsp_controls) ?
						dsplo.at(data.iBW & 0x7F).c_str() : bwt.at(data.iBW).c_str(),
					(data.iBW > 256 && selrig->has_dsp_controls) ?
						dsphi.at((data.iBW >> 8) & 0x7F).c_str() : ""
				);
			} else {
				snprintf(str, sizeof(str), "\
  filter ......... %s\n\
  bwt index ...... %2d, [%s]\n",
					selrig->FILT(selrig->get_FILT(data.imode)),
					data.iBW,
					bwt.at(data.iBW).c_str() );
			}
		prstr.append(str);
		str[0] = 0;
		snprintf( str, sizeof(str), "\
  split .......... %4d, power_control . %4.1f, volume_control  %4d\n\
  attenuator ..... %4d, preamp ........ %4d, rf gain ....... %4d\n\
  if_shift ....... %4d, shift val ..... %4d\n\
  auto notch ..... %4d, notch ......... %4d, notch value ... %4d\n\
  noise .......... %4d, nr ............ %4d, nr val ........ %4d\n\
  mic gain ....... %4d, agc level ..... %4d, squelch ....... %4d\n\
  compression .... %4d, compON ........ %4d\n",
			data.split,
			data.power_control,
			data.volume_control,
			data.attenuator,
			data.preamp,
			data.rf_gain,
			data.if_shift,
			data.shift_val,
			data.auto_notch,
			data.notch,
			data.notch_val,
			data.noise,
			data.nr,
			data.nr_val,
			data.mic_gain,
			data.agc_level,
			data.squelch,
			data.compression,
			data.compON
		);
		prstr.append(str);
		return prstr.c_str();
	} catch (const std::exception& e) {
		LOG_ERROR("%s", e.what());
	}
	return "";
}

std::string print_all()
{
	std::string s;
	s.assign("\
=======================================================================\n");
	s.append(print(vfoA));
	s.append("\
-----------------------------------------------------------------------\n");
	s.append(print(vfoB));
	return s;
}

void read_info()
{
	trace(1,"read_info()");
	selrig->get_info();
}

void TRACED(update_vfoAorB, void *d)
	trace(1,"update_vfoAorB()");
	if (selrig->inuse == onB) {
		vfoB.src = RIG;
		vfoB.freq = selrig->get_vfoB();
		vfoB.imode =selrig->get_modeB();
		vfoB.iBW = selrig->get_bwB();
		xml_B.update_bw( vfoB.iBW );
		xml_B.update_mode( vfoB.imode );
		xml_B.update_freq( vfoB.freq );
	} else {
		vfoA.src = RIG;
		vfoA.freq =selrig->get_vfoA();
		vfoA.imode =selrig->get_modeA();
		vfoA.iBW = selrig->get_bwA();
		xml_A.update_bw( vfoA.iBW );
		xml_A.update_mode( vfoA.imode );
		xml_A.update_freq( vfoA.freq );
	}
	updateUI((void*)0);
}

void read_vfo()
{
	if (xcvr_name == rig_K3.name_) {
		read_K3_vfo();
		return;
	}

// transceiver changed ?
	trace(1,"read_vfo()");
	unsigned long long  freq;

	static int current_vfo = onNIL;
	int chkvfo = selrig->get_vfoAorB();
	if (current_vfo != chkvfo) {
		current_vfo = chkvfo;
		Fl::awake(updateUI);
	}
	if (current_vfo == onNIL)
		return;

	if (selrig->has_get_info)
		selrig->get_info();

	if (selrig->inuse == onA) { // vfo-A
		trace(2, "vfoA active", "get vfo A");
		freq = selrig->get_vfoA();
		vfoA.freq = freq;
		xml_A.update_freq(vfoA.freq);
		Fl::awake(setFreqDispA);
		vfo = &vfoA;
		xml_A.update_freq( vfoA.freq );
		if ( selrig->twovfos() ) {
			trace(2, "vfoA active", "get vfo B");
			freq = selrig->get_vfoB();
			vfoB.freq = freq;
			xml_B.update_freq( vfoB.freq );
			Fl::awake(setFreqDispB);
		}
	} else { // vfo-B
		trace(2, "vfoB active", "get vfo B");
		freq = selrig->get_vfoB();
		vfoB.freq = freq;
		xml_B.update_freq( vfoB.freq );
		Fl::awake(setFreqDispB);
		vfo = &vfoB;
		if ( selrig->twovfos() ) {
			trace(2, "vfoB active", "get vfo A");
			freq = selrig->get_vfoA();
			vfoA.freq = freq;
			Fl::awake(setFreqDispA);
			xml_A.update_freq( vfoA.freq );
		}
	}
	Fl::awake(updateTCI);
	Fl::awake(updateFLEX1500);
}

void update_ifshift(void *d);

void TRACED(updateUI, void *)

	set_Mode_BW_control(NULL);

	updateBandwidthControl(NULL);

	setFreqDispA();
	setFreqDispB();

	highlight_vfo(NULL);

	int min, max, step;
	selrig->get_if_min_max_step(min, max, step);
	if (sldrIFSHIFT) sldrIFSHIFT->minimum(min);
	if (sldrIFSHIFT) sldrIFSHIFT->maximum(max);
	if (sldrIFSHIFT) sldrIFSHIFT->step(step);
	if (sldrIFSHIFT) sldrIFSHIFT->redraw();
	if (spnrIFSHIFT) spnrIFSHIFT->minimum(min);
	if (spnrIFSHIFT) spnrIFSHIFT->maximum(max);
	if (spnrIFSHIFT) spnrIFSHIFT->step(step);
	if (spnrIFSHIFT) spnrIFSHIFT->redraw();
	update_ifshift((void *)0);

}

void TRACED(set_Mode_BW_control, void *)

	opMODE->index(vfo->imode);
	opMODE->redraw();

	selrig->bandwidths_ = selrig->bwtable(vfo->imode);
	opBW->clear();
	for (size_t i = 0; i < selrig->bandwidths_.size(); i++)
		opBW->add(selrig->bandwidths_.at(i).c_str());

	if (xcvr_name == rig_KX3.name_ || xcvr_name == rig_K4.name_)
		return;

	opBW->index(vfo->iBW);
	opBW->redraw();
	opBW->show();

// enables/disables the IF shift control, depending on the mode.
// the IF Shift function, is ONLY valid in CW modes, with the 870S.

	if (xcvr_name == rig_TS870S.name_) {
		if (vfo->imode == RIG_TS870S::tsCW || vfo->imode == RIG_TS870S::tsCWR) {
			btnIFsh->activate();
			if (sldrIFSHIFT) sldrIFSHIFT->activate();
			if (spnrIFSHIFT) spnrIFSHIFT->activate();
		} else {
			btnIFsh->deactivate();
			if (sldrIFSHIFT) sldrIFSHIFT->deactivate();
			if (spnrIFSHIFT) spnrIFSHIFT->deactivate();
		}
	}
}

void setFILTER(void *)
{
	if (selrig->has_FILTER) {
		if (selrig->inuse == onB)
			btnFILT->label(selrig->FILT(vfoB.filter));
		else
			btnFILT->label(selrig->FILT(vfoA.filter));
		btnFILT->redraw_label();
		trace(3, "Filter", (selrig->inuse == onB ? "B" : "A"), btnFILT->label());
	}
}

// mode and bandwidth

int last_mode = -1;

void read_mode()
{
	if (xcvr_name == rig_K3.name_) {
		read_K3_mode();
		return;
	}

	int nu_mode;
	if (selrig->inuse == onA) {
		nu_mode = selrig->get_modeA();
		if (last_mode != nu_mode) {
			vfoA.imode = vfo->imode = nu_mode;
			xml_A.update_mode(vfoA.imode);
			selrig->adjust_bandwidth(vfo->imode);
			Fl::awake(set_Mode_BW_control);
			vfoA.iBW = vfo->iBW = selrig->get_bwA();
			xml_A.update_bw(vfoA.iBW);
			Fl::awake(updateBandwidthControl);
			deb_trace(2, "read_mode(vfoA)", printXCVR_STATE(vfoA).c_str());
			last_mode = nu_mode;
		} else
			rig_trace(1, "read_mode(vfoA)");

		Fl::awake(updateTCI);
		Fl::awake(updateFLEX1500);

		if (selrig->twovfos()) {
			vfoB.imode =selrig->get_modeB();
			xml_B.update_mode(vfoB.imode);
			vfoB.filter = selrig->get_FILT(vfoB.imode);
		}

		vfoA.filter = selrig->get_FILT(vfoA.imode);
		Fl::awake(setFILTER);
	} else {
		nu_mode = selrig->get_modeB();
		if (last_mode != nu_mode) {
			vfoB.imode = vfo->imode = nu_mode;
			xml_B.update_mode(vfoB.imode);

			selrig->adjust_bandwidth(vfo->imode);
			xml_B.update_mode(vfoB.imode);
			Fl::awake(set_Mode_BW_control);

			vfoB.iBW = vfo->iBW = selrig->get_bwB();
			xml_B.update_bw(vfoB.iBW);
			Fl::awake(updateBandwidthControl);
			deb_trace(2, "read_mode(vfoB)", printXCVR_STATE(vfoB).c_str());
			last_mode = nu_mode;
		} else
			rig_trace(1, "read_mode(vfoB)");

		Fl::awake(updateTCI);
		Fl::awake(updateFLEX1500);

		if (selrig->twovfos()) {
			vfoA.imode = selrig->get_modeA();
			xml_A.update_mode(vfoA.imode);
			vfoA.filter = selrig->get_FILT(vfoA.imode);
		}

		vfoB.filter = selrig->get_FILT(vfoB.imode);
		Fl::awake(setFILTER);
	}
}

void TRACED(setBWControl, void *)
	if (selrig->has_dsp_controls) {
		if (vfo->iBW > 256) {
			opBW->hide();
			opDSP_hi->index((vfo->iBW >> 8) & 0x7F);
			opDSP_lo->index(vfo->iBW & 0xFF);
			if (opDSP_lo->visible()) {
				opDSP_lo->redraw();
				btnDSP->label(selrig->SL_label);
			} else {
				opDSP_hi->redraw();
				btnDSP->label(selrig->SH_label);
			}
			btnDSP->redraw_label();
			btnDSP->redraw();
			btnDSP->show();
		} else {
			opDSP_lo->index(0);
			opDSP_hi->index(0);
			opDSP_lo->hide();
			opDSP_hi->hide();
			btnDSP->hide();
//			if (vfo->iBW != opBW->index())
				opBW->index(vfo->iBW);
			opBW->show();
			opBW->redraw();
		}
	}
	else {
		opDSP_lo->hide();
		opDSP_hi->hide();
		btnDSP->hide();
		opBW_A->hide();
		opBW_B->hide();

		if (xcvr_name == rig_KX3.name_ || xcvr_name == rig_K4.name_)
			return;
//		if (vfo->iBW != opBW->index())
			opBW->index(vfo->iBW);
		opBW->show();
		opBW->redraw();
	}
}

void set_Kx_bandwidths(void *)
{
	opBW_A->index(vfoA.iBW);
	opBW_B->index(vfoB.iBW);
	opBW_A->redraw();
	opBW_B->redraw();
}

int lastbw = -1;
void TRACED(read_bandwidth)
	if (xcvr_name == rig_K2.name_ || xcvr_name == rig_K3.name_ ) {
		vfoA.iBW = vfo->iBW = selrig->get_bwA();//nu_BW;
		Fl::awake(setBWControl);
		vfoB.iBW = selrig->get_bwB();//nu_BW;
		xml_A.update_bw(vfoA.iBW);
		xml_B.update_bw(vfoB.iBW);
		return;
	}
	if (xcvr_name == rig_KX3.name_ || xcvr_name == rig_K4.name_) {
		vfoA.iBW = selrig->get_bwA();
		vfoB.iBW = selrig->get_bwB();
		xml_A.update_bw(vfoA.iBW);
		xml_B.update_bw(vfoB.iBW);

		Fl::awake(set_Kx_bandwidths);

		return;
	}

	std::stringstream s;
	if (selrig->inuse == onA) {
		vfoA.iBW = vfo->iBW = selrig->get_bwA();
		xml_A.update_bw(vfoA.iBW);
		s << "read_bandwidth(vfoA): " << printXCVR_STATE(vfoA);
	} else {
		vfoB.iBW = vfo->iBW = selrig->get_bwB();
		xml_B.update_bw(vfoB.iBW);
		s << "read_bandwidth(vfoB): " << printXCVR_STATE(vfoB);
	}
	rig_trace(1, s.str().c_str());
	if (lastbw != vfo->iBW) {
		deb_trace(1, s.str().c_str());
		lastbw = vfo->iBW;
	}

	Fl::awake(setBWControl);
	Fl::awake(updateTCI);
	Fl::awake(updateFLEX1500);
}

// read current signal level
void read_smeter()
{
	if (!selrig->has_smeter) return;
	int  sig;
	{
		trace(1,"read_smeter()");
		sig = selrig->get_smeter();
	}
	if (sig == -1) return;
	smtrval = sig;
	Fl::awake(updateSmeter);
}

void read_voltmeter()
{
	if (!selrig->has_voltmeter) return;
	double sig;
	{
		trace(1, "read voltage");
		sig = selrig->get_voltmeter();
	}
	vmtrval = sig;
	Fl::awake(updateVmeter);
}

int tunerval = 0;
void update_UI_TUNER(void *)
{
	btn_tune_on_off->value(tunerval);
	btn_tune_on_off->redraw();
}

//void read_tuner()
//{
//	if (!selrig->has_tune_control) return;

//	tunerval = selrig->get_tune();
//	if (tunerval != btn_tune_on_off->value()) {
//		std::stringstream s;
//		s << "tuner state: " << tunerval;
//		trace(1, s.str().c_str());
//		Fl::awake(update_UI_TUNER);
//	}
//}

// read power out
void read_power_out()
{
	if (!selrig->has_power_out) return;
	int sig;
	{
		trace(1,"read_power_out()");
		sig = selrig->get_power_out();
	}
	if (sig < 0) sig = 0;
	pwrval = sig;
	Fl::awake(updateFwdPwr);
}

// read swr
void read_swr()
{
	if (!selrig->has_swr_control) return;
	int sig;
	{
		trace(1,"read_swr()");
		sig = selrig->get_swr();
	}
	if (sig < 0) sig = 0;
	swrval = sig;
	Fl::awake(updateSWR);
}

// alc
void read_alc()
{
	if (!selrig->has_alc_control) {
		return;
	}
	int sig;
	{
		trace(1,"read_alc()");
		sig = selrig->get_alc();
	}
	if (sig < 0) sig = 0;
	alcval = sig;
	Fl::awake(updateALC);
}

// IDD
void read_idd()
{
	if (!selrig->has_idd_control) {
		return;
	}
	double sig;
	{
		trace(1,"read_idd()");
		sig = selrig->get_idd();
	}
	if (sig < 0) sig = 0;
	iddval = sig;
	Fl::awake(updateIDD);
}

// notch
void update_auto_notch(void *d)
{
	btnAutoNotch->value(progStatus.auto_notch);
}

void read_auto_notch()
{
	int val;
	if (!selrig->has_auto_notch) return;
	{
		trace(1,"read_auto_notch()");
		val = selrig->get_auto_notch();
	}
	if (val != progStatus.auto_notch) {
		progStatus.auto_notch = vfo->auto_notch = val;
		Fl::awake(update_auto_notch, (void*)0);
	}
}

// NOISE blanker
void update_noise(void *d)
{
	btnNOISE->label(selrig->nb_label());
	btnNOISE->value(progStatus.noise);
	btnNOISE->redraw_label();
	btnNOISE->redraw();
	sldr_nb_level->value(progStatus.nb_level);
	sldr_nb_level->redraw();
}

void read_noise()
{
	int on = 0, val = 0;
	if (inhibit_nb_level > 0) {
		inhibit_nb_level--;
		return;
	}
	{
		trace(1,"read_noise()");
		on = selrig->get_noise();
		val = selrig->get_nb_level();
//	}
//	if ((on != progStatus.noise) || (val != progStatus.nb_level)) {
		vfo->noise = progStatus.noise = on;
		vfo->nb_level = progStatus.nb_level = val;
		Fl::awake(update_noise, (void*)0);
	}
}

// compression
void update_compression(void *d)
{
	if (btnCompON) btnCompON->value(progStatus.compON);
	if (spnr_compression) spnr_compression->value(progStatus.compression);
}

void read_compression()
{
	int on = progStatus.compON;
	int val = progStatus.compression;
	if (selrig->has_compression || selrig->has_compON) {
		{
			trace(1,"read_compression()");
			selrig->get_compression( on, val );
		}
		if (on != progStatus.compON || val != progStatus.compression) {
			vfo->compression = progStatus.compression = val;
			vfo->compON = progStatus.compON = on;
			Fl::awake(update_compression, (void*)0);
		}
	}
}

// preamp - attenuator
void update_preamp(void *d)
{
	if (selrig->name_ == rig_FLEX1500.name_ ) {
		cbo_preamp->index(progStatus.preamp);
		cbo_preamp->redraw();
		return;
	}

	btnPreamp->value(progStatus.preamp > 0 ? 1 : 0);
	btnPreamp->label(selrig->pre_label());
	btnPreamp->redraw_label();
	btnPreamp->redraw();
}

void update_attenuator(void *d)
{
	if (selrig->name_ == rig_IC7610.name_ || selrig->name_ == rig_IC7760.name_) {
		ic7610att->index(progStatus.index_ic7610att);
		return;
	}

	btnAttenuator->value(progStatus.attenuator > 0 ? 1 : 0);
	btnAttenuator->label(selrig->att_label());
	btnAttenuator->redraw_label();
	btnAttenuator->redraw();
}

void read_preamp()
{
	int val;
	if (selrig->has_preamp_control) {
		trace(1,"read_preamp_att()  1");
		val = selrig->get_preamp();
		vfo->preamp = progStatus.preamp = val;
		Fl::awake(update_preamp, (void*)0);
	}
}

void read_att()
{
	int val;
	if (selrig->has_attenuator_control) {
		trace(1,"read_preamp_att()  2");
		val = selrig->get_attenuator();
		vfo->attenuator = progStatus.attenuator = val;
		Fl::awake(update_attenuator, (void*)0);
	}
}

// split
void update_split(void *d)
{
	if (xcvr_name == rig_FTdx1200.name_ ||
		xcvr_name == rig_TS480SAT.name_ || xcvr_name == rig_TS480HX.name_ ||
		xcvr_name == rig_TS590S.name_ || xcvr_name == rig_TS590SG.name_ ||
		xcvr_name == rig_TS890S.name_ ||
		xcvr_name == rig_TS2000.name_ || xcvr_name == rig_TS990.name_) {
		switch (progStatus.split) {
			case 0: btnSplit->value(0);
					highlight_vfo(NULL);
					break;
			case 1: btnSplit->value(1);
					highlight_vfo(NULL);
					break;
			case 2:	btnSplit->value(1);
					highlight_vfo(NULL);
					break;
			case 3: btnSplit->value(0);
					highlight_vfo(NULL);
					break;
		}
	} else
		btnSplit->value(progStatus.split);
	btnSplit->redraw();
}

void read_split()
{
	int val = progStatus.split;
	if (selrig->has_split) {
		val = selrig->get_split();
		vfo->split = progStatus.split = val;
		Fl::awake(update_split, (void*)0);
		std::ostringstream s;
		s << "read_split() " << (val ? "ON" : "OFF");
		trace(1, s.str().c_str());
	} else {
		vfo->split = progStatus.split;
	}
}

// volume
void update_volume(void *d)
{
	long *nr = (long *)d;
	if (spnrVOLUME) {
		spnrVOLUME->value(progStatus.volume);
		spnrVOLUME->activate();
		spnrVOLUME->redraw();
	}

	if (sldrVOLUME) {
		sldrVOLUME->value(progStatus.volume); // Set slider to last known value
		sldrVOLUME->activate();				  // activate it
		sldrVOLUME->redraw();
	}

	if (*nr) btnVol->value(1);			  // Button Lit
	else     btnVol->value(0);			  // Button Dark.
	btnVol->redraw();
}

long nlzero = 0L;
long nlone = 1L;

void read_volume()
{
	if (inhibit_volume > 0) {
		inhibit_volume--;
		return;
	}
	if (!selrig->has_volume_control) return;
	int vol;
	{
		trace(1,"read_volume()");
		vol = selrig->get_volume_control();
	}
	if (vol != progStatus.volume) {
		if (vol <= 1 && !btnVol->value()) return;
		if (xcvr_name == rig_K4.name_) {
			if (selrig->isOnA()) progStatus.volume_A = vol;
			else progStatus.volume_B = vol;
		} else
			progStatus.volume = vol;
		if (vol <= 1 && btnVol->value())
			Fl::awake(update_volume, (void*)&nlzero);
		else
			Fl::awake(update_volume, (void*)&nlone);
	}
}

// ifshift
void update_ifshift(void *d)
{
	btnIFsh->value(progStatus.shift);
	btnIFsh->redraw();
	if (sldrIFSHIFT) {
		sldrIFSHIFT->value(progStatus.shift_val);
		sldrIFSHIFT->redraw();
	}
	if (spnrIFSHIFT) {
		spnrIFSHIFT->value(progStatus.shift_val);
		spnrIFSHIFT->redraw();
	}
}

void update_pbt(void *)
{
	if (sldrINNER) {
		sldrINNER->value(progStatus.pbt_inner);
		sldrINNER->redraw();
	}
	if (sldrOUTER) {
		sldrOUTER->value(progStatus.pbt_outer);
		sldrOUTER->redraw();
	}
}

void read_pbt()
{
	progStatus.pbt_inner = selrig->get_pbt_inner();
	progStatus.pbt_outer = selrig->get_pbt_outer();
	Fl::awake(update_pbt, (void*)0);
}

void read_ifshift()
{
	int on = 0;
	int val = 0;

	if (inhibit_shift > 0) {
		inhibit_shift--;
		return;
		}
	if (!selrig->has_ifshift_control) return;

	trace(1,"read_if_shift()");
	on = selrig->get_if_shift(val);

	if (xcvr_name == rig_K3.name_ ||
		xcvr_name == rig_KX3.name_ ||
		xcvr_name == rig_K4.name_) {
		vfo->shift_val = progStatus.shift_val = val;
		Fl::awake(update_ifshift, (void*)0);
	} else if ((on != progStatus.shift) || (val != progStatus.shift_val)) {
		vfo->if_shift = progStatus.shift = on;
		vfo->shift_val = progStatus.shift_val = val;
		Fl::awake(update_ifshift, (void*)0);
	}
}

// noise reduction
void update_nr(void *d)
{
	btnNR->value(progStatus.noise_reduction);
	btnNR->redraw();
	if (sldrNR) {
		sldrNR->value(progStatus.noise_reduction_val);
		sldrNR->redraw();
	}
	if (spnrNR) {
		spnrNR->value(progStatus.noise_reduction_val);
		spnrNR->redraw();
	}
}

void read_nr()
{
	int on = 0;
	int val = 0;
	if (inhibit_nr > 0) {
		inhibit_nr--;
		return;
	}
	if (!selrig->has_noise_reduction) return;
	{
		trace(1,"read_nr()");
		on = selrig->get_noise_reduction();
		val = selrig->get_noise_reduction_val();
	}
	if ((on != progStatus.noise_reduction) || (val != progStatus.noise_reduction_val)) {
		vfo->nr = progStatus.noise_reduction = on;
		vfo->nr_val = progStatus.noise_reduction_val = val;
		Fl::awake(update_nr, (void*)0);
	}
}

// manual notch
void update_notch(void *d)
{
	btnNotch->value(progStatus.notch);
	btnNotch->redraw();
	if (sldrNOTCH) {
		sldrNOTCH->value(progStatus.notch_val);
		sldrNOTCH->redraw();
	}
	if (spnrNOTCH) {
		spnrNOTCH->value(progStatus.notch_val);
		spnrNOTCH->redraw();
	}
}

void read_notch()
{
	if (!selrig->has_notch_control) return;

	int on = progStatus.notch;
	int val = progStatus.notch_val;
	{
		trace(1,"read_notch()");
		on = selrig->get_notch(val);
	}

	if (on != progStatus.notch)
		vfo->notch = progStatus.notch = on;

	if (val != progStatus.notch_val)
		vfo->notch_val = progStatus.notch_val = val;

	Fl::awake(update_notch, (void*)0);
}

// power_control

void update_power_control(void *d)
{
	double min, max, step;

	if (selrig->has_power_control) {
		selrig->get_pc_min_max_step(min, max, step);

		if (sldrPOWER) {
			sldrPOWER->minimum(min);
			sldrPOWER->maximum(max);
			sldrPOWER->step(step);
		}

		if (spnrPOWER) {
			spnrPOWER->minimum(min);
			spnrPOWER->maximum(max);
			spnrPOWER->step(step);
		}
	}

	if (sldrPOWER) {
		sldrPOWER->value(progStatus.power_level);
		sldrPOWER->redraw();
	}

	if (spnrPOWER) {
		spnrPOWER->value(progStatus.power_level);
		spnrPOWER->redraw();
	}
}

void read_power_control()
{
	if (inhibit_power > 0) {
		inhibit_power--;
		return;
	}
	if (!selrig->has_power_control) return;

	trace(1,"read_power_control()");
	vfo->power_control = progStatus.power_level = selrig->get_power_control();

	Fl::awake(update_power_control, (void*)0);

}

// mic gain
void update_mic_gain(void *d)
{
	if (sldrMICGAIN) {
		sldrMICGAIN->value(progStatus.mic_gain);
		sldrMICGAIN->redraw();
	}
	if (spnrMICGAIN) {
		spnrMICGAIN->value(progStatus.mic_gain);
		spnrMICGAIN->redraw();
	}
}

void read_mic_gain()
{
	int val;
	if (inhibit_mic > 0) {
		inhibit_mic--;
		return;
	}
	if (!selrig->has_micgain_control) return;
	{
		trace(1,"read_mic_gain()");
		val = selrig->get_mic_gain();
	}
	if (val != progStatus.mic_gain || val != vfo->mic_gain || (sldrMICGAIN && val != sldrMICGAIN->value())) {
		vfo->mic_gain = progStatus.mic_gain = val;
		Fl::awake(update_mic_gain, (void*)0);
	}
}

void read_agc()
{
	int val;
	if (!selrig->has_agc_control) return;
	trace(1,"read_agc()");
	val = selrig->get_agc();
	if (val != progStatus.agc_level) {
		vfo->agc_level = progStatus.agc_level = val;
		Fl::awake(setAGC);
	}
}

// rf gain
void update_rfgain(void *d)
{
	if (sldrRFGAIN) {
		sldrRFGAIN->value(progStatus.rfgain);
		sldrRFGAIN->redraw();
	}
	if (spnrRFGAIN) {
		spnrRFGAIN->value(progStatus.rfgain);
		spnrRFGAIN->redraw();
	}
}

void read_rfgain()
{
	int val;
	if (inhibit_rfgain > 0) {
		inhibit_rfgain--;
		return;
	}
	if (selrig->has_rf_control) {
		trace(1,"read_rfgain");
		val = selrig->get_rf_gain();
		if (val != progStatus.rfgain) {
			progStatus.rfgain = val;
			Fl::awake(update_rfgain, (void*)0);
		}
	}
	read_agc();
}

// squelch
void update_squelch(void *d)
{
	if (sldrSQUELCH) {
		sldrSQUELCH->value(progStatus.squelch);
		sldrSQUELCH->redraw();
	}
	if (spnrSQUELCH) {
		spnrSQUELCH->value(progStatus.squelch);
		spnrSQUELCH->redraw();
	}
}

void read_squelch()
{
	int val;
	if (inhibit_squelch > 0) {
		inhibit_squelch--;
		return;
	}
	if (!selrig->has_sql_control) return;
	{
		trace(1,"read_squelch()");
		val = selrig->get_squelch();
	}

	if (val != progStatus.squelch || val != vfo->squelch || (sldrSQUELCH && val != sldrSQUELCH->value())) {
		vfo->squelch = progStatus.squelch = val;
		Fl::awake(update_squelch, (void*)0);
	}
}

void show_meters( void *tx )
{
	if (tx == (void *)1) {
		sldrRcvSignal->hide();
		sldrRcvSignal->redraw();
		scaleSmeter->hide();
		sldrSWR->show();
		sldrSWR->redraw();
        if (selrig->name_ == rig_PowerSDR.name_) {
		    btnALC_IDD_SWR->image(meter_image == SWR_IMAGE ? image_swr : image_alc40db);
        }
        else {
			if (meter_image == IDD_IMAGE)  ;
//				btnALC_IDD_SWR->image(image_idd));
			else if (meter_image == ALC_IMAGE)
				btnALC_IDD_SWR->image(image_alc);
			else
				btnALC_IDD_SWR->image(image_swr);
        }
		btnALC_IDD_SWR->redraw();
		btnALC_IDD_SWR->show();
	}
	else {
		sldrSWR->hide();
		sldrSWR->redraw();
		sldrRcvSignal->show();
		sldrRcvSignal->redraw();
		btnALC_IDD_SWR->hide();
		scaleSmeter->show();
	}
}

void set_ptt(void *d)
{
	if (d == (void*)0) {
		btnPTT->value(0);
	} else {
		btnPTT->value(1);
	}
	show_meters(d);
}

void check_ptt()
{
	if (selrig->name_ == rig_FT817.name_ ||
		selrig->name_ == rig_FT817BB.name_ ||
		selrig->name_ == rig_FT818ND.name_ ) {
		return;
	}
	PTT = ptt_state();
	xml_A.update_ptt(PTT);
}

void check_break_in()
{
	if (selrig->has_cw_break_in)
		selrig->get_break_in();
}

//static bool resetrcv = true;
//static bool resetxmt = true;

// On early model Yaesu transceivers, the mode must be set before VFO,
// since mode changes can shift frequency.
//
// For example, might set freq to 7123.000, but then change mode from USB
// to DATA-U.  This mode shift would change the VFO to  7123.700, instead
// of the desired 7123.000.
//
// Setting VFO after the mode change will prevent this type of frequency
// shifting.
void yaesu891UpdateA(XCVR_STATE * newVfo)
{
	selrig->set_modeA(newVfo->imode);
	selrig->set_vfoA(newVfo->freq);
	selrig->set_bwA(newVfo->iBW);

	vfoA.imode = selrig->get_modeA();
	xml_A.update_mode(vfoA.imode);

	vfoA.freq = selrig->get_vfoA();
	xml_A.update_freq(vfoA.freq);

	vfoA.iBW = selrig->get_bwA();
	xml_A.update_bw(vfoA.iBW);
}

void yaesu891UpdateB(XCVR_STATE * newVfo)
{
	selrig->set_modeB(newVfo->imode);
	selrig->set_vfoB(newVfo->freq);
	selrig->set_bwB(newVfo->iBW);

	vfoB.imode = selrig->get_modeB();
	xml_B.update_mode(vfoB.imode);

	vfoB.freq = selrig->get_vfoB();
	xml_B.update_freq(vfoB.freq);

	vfoB.iBW = selrig->get_bwB();
	xml_B.update_bw(vfoB.iBW);
}

void FTdx10_UpdateA(XCVR_STATE * newVfo)
{
	selrig->set_modeA(newVfo->imode);
	selrig->set_vfoA(newVfo->freq);
	selrig->set_bwA(newVfo->iBW);
	vfoA.imode = selrig->get_modeA();
	vfoA.freq = selrig->get_vfoA();
	vfoA.iBW = selrig->get_bwA();

	xml_A.update_bw(vfoA.iBW);
	xml_A.update_freq(vfoA.freq);
	xml_A.update_mode(vfoA.imode);
}

void FTdx10_UpdateB(XCVR_STATE * newVfo)
{
	selrig->set_modeB(newVfo->imode);
	selrig->set_vfoB(newVfo->freq);
	selrig->set_bwB(newVfo->iBW);
	vfoB.imode = selrig->get_modeB();
	vfoB.freq = selrig->get_vfoB();
	vfoB.iBW = selrig->get_bwB();

	xml_B.update_bw(vfoB.iBW);
	xml_B.update_freq(vfoB.freq);
	xml_B.update_mode(vfoB.imode);
}

void serviceXCVR(VFOQUEUE nuvals)
{
	guard_lock serial( &mutex_serial, std::string(__func__) );

	if (nuvals.change == ON || nuvals.change == OFF) { // PTT processing
		if (selrig->ICOMmainsub && selrig->inuse == onB) {  // disallowed operation
			Fl::awake(update_UI_PTT);
			return;
		}
		PTT = (nuvals.change == ON);
		if (nuvals.change == ON) trace(1,"ptt ON");
		else trace(1,"ptt OFF");
		rigPTT(PTT);
		{
			bool get = ptt_state();
			int cnt = 0;
			while ((get != PTT) && (cnt++ < 100)) {
				MilliSleep(10);
				get = ptt_state();
			}
			std::stringstream s;
			s << "ptt returned " << get << " in " << cnt * 10 << " msec";
			trace(1, s.str().c_str());
			Fl::awake(update_UI_PTT);
		}
		return;
	}

	switch (nuvals.change) {
		case vX:
			if (selrig->inuse == onB)
				serviceB(nuvals.vfo);
			else
				serviceA(nuvals.vfo);
			break;
		case vA:
			serviceA(nuvals.vfo);
			break;
		case vB:
			serviceB(nuvals.vfo);
			break;
		case sA: // select A
			selrig->selectA();
			vfo = &vfoA;
			if (selrig->name_ == rig_FTdx10.name_) {
				FTdx10_UpdateA(&vfoA);
			}
			if (selrig->name_ == rig_FT2000.name_) {
				progStatus.rfgain = selrig->get_rf_gain();
			}
			rig_trace(2, "case sA ", printXCVR_STATE(vfoA).c_str());
			Fl::awake(updateUI);
			break;
		case sB: // select B
			selrig->selectB();
			vfo = &vfoB;
			if (selrig->name_ == rig_FTdx10.name_) {
				FTdx10_UpdateB(&vfoB);
			}
			if (selrig->name_ == rig_FT2000.name_) {
				progStatus.rfgain = selrig->get_rf_gain();
			}
			rig_trace(2, "case sB ", printXCVR_STATE(vfoB).c_str());
			Fl::awake(updateUI);
			break;
		case sON: case sOFF:
			{
			int on = 0;
			if (nuvals.change == sON) on = 1;
			trace(1, (on ? "split ON" : "split OFF"));
			rig_trace(2, "case sB ", printXCVR_STATE(vfoB).c_str());
			if (selrig->can_split() || selrig->has_split_AB) {
				selrig->set_split(on);
				selrig->get_split();
				progStatus.split = on;
				Fl::awake(update_split, (void *)0);
				if (selrig->ICOMmainsub) {
					selrig->selectA();
					vfo = &vfoA;
				}
			}
			}
			break;
		case SWAP:
			trace(1, "execute swapab()");
			rig_trace(1, "execute swapab()");
			execute_swapAB();
			break;
		case A2B:
			trace(1, "execute A2B()");
			rig_trace(1, "execute A2B()");
			execute_A2B();
			break;
		case FA2FB:
			trace(1, "execute FA2FB");
			rig_trace(1, "execute FA2FB");
			execute_FA2FB();
			break;
		case FB2FA:
			trace(1, "execute FB2FA");
			rig_trace(1, "execute FB2FA");
			execute_FB2FA();
			break;
		default:
			trace(2, "default ", printXCVR_STATE(nuvals.vfo).c_str());
			if (selrig->inuse == onB) serviceB(nuvals.vfo);
			else serviceA(nuvals.vfo);
			break;
	}

	Fl::awake(updateUI);
}

void find_bandwidth(XCVR_STATE &nuvals)
{
	if (nuvals.iBW == 255) return;
	if (!selrig->has_bandwidth_control) {
		nuvals.iBW = 255;
		return;
	}
	if (nuvals.iBW > 65536) {
		nuvals.iBW /= 256;
		nuvals.iBW /= 256;
		size_t i = 0;
		int bwval = 0;
		for (i = 0; i < selrig->bandwidths_.size(); i++) {
			if ((bwval = atol(selrig->bandwidths_[i].c_str())) == nuvals.iBW)
				break;
			if (bwval > nuvals.iBW) {
				i--;
				break;
			}
		}
		if (i < 0) i = 0;
		if (i >= selrig->bandwidths_.size()) i = selrig->bandwidths_.size() - 1;
		nuvals.iBW = i;
	}
}

void serviceA(XCVR_STATE nuvals)
{
	if (nuvals.freq == 0) nuvals.freq = vfoA.freq;
	if (nuvals.imode == -1) nuvals.imode = vfoA.imode;
	if (nuvals.iBW == 255) nuvals.iBW = vfoA.iBW;

	if (selrig->inuse == onB) {
		if (selrig->can_change_alt_vfo) {
			trace(2, "B active, set alt vfo A", printXCVR_STATE(nuvals).c_str());
			rig_trace(2, "B active, set alt vfo A", printXCVR_STATE(nuvals).c_str());
			if (vfoA.imode != nuvals.imode) {
				if (selrig->name_ == rig_FT891.name_) {
					// Mode change on ft891 requires mode first, so set all values
					yaesu891UpdateA(&nuvals);
					vfoA = nuvals;
				} else {
					selrig->set_modeA(vfoA.imode = nuvals.imode);
					vfoA.imode = selrig->get_modeA();
					xml_A.update_mode(vfoA.imode);

					selrig->set_bwA(nuvals.iBW);
					vfoA.iBW = selrig->get_bwA();
					xml_A.update_bw(vfoA.iBW);

					selrig->set_vfoA(nuvals.freq);
					vfoA.freq = selrig->get_vfoA();
					xml_A.update_freq(vfoA.freq);
					Fl::awake(setFreqDispA);

					return;
				}
			}
			if (vfoA.iBW != nuvals.iBW) {
				selrig->set_bwA(nuvals.iBW);
				vfoA.iBW = selrig->get_bwA();
				xml_A.update_bw(vfoA.iBW);
			}
			if (vfoA.freq != nuvals.freq) {
				selrig->set_vfoA(nuvals.freq);
				vfoA.freq = selrig->get_vfoA();
				xml_A.update_freq(vfoA.freq);
			}
			vfoA = nuvals;
		} else if (xcvr_name != rig_TT550.name_) {
			trace(2, "B active, set vfo A", printXCVR_STATE(nuvals).c_str());
			rig_trace(2, "B active, set vfo A", printXCVR_STATE(nuvals).c_str());
			selrig->selectA();

			selrig->set_modeA(nuvals.imode);
			vfoA.imode = selrig->get_modeA();
			Fl::awake(updateUI);

			selrig->set_bwA(nuvals.iBW);
			vfoA.iBW = selrig->get_bwA();

			selrig->set_vfoA(nuvals.freq);
			vfoA.freq = selrig->get_vfoA();

			selrig->selectB();
			vfoA = nuvals;

			xml_A.update_freq(vfoA.freq);
			xml_A.update_mode(vfoA.imode);
			xml_A.update_bw(vfoA.iBW);
		}
		Fl::awake(setFreqDispA);
		return;
	}

	trace(2, "service VFO A", printXCVR_STATE(nuvals).c_str());

	if (nuvals.imode != -1)  {
		if (selrig->name_ == rig_FT891.name_) {
			// Mode change on ft891 can change frequency, so set all values
			yaesu891UpdateA(&nuvals);
			vfoA = nuvals;
			set_bandwidth_control();
		} else {
			selrig->set_modeA(vfoA.imode = nuvals.imode);
			vfoA.imode = selrig->get_modeA();
			xml_A.update_mode(vfoA.imode);

			selrig->set_bwA(nuvals.iBW);
			vfoA.iBW = selrig->get_bwA();
			xml_A.update_bw(vfoA.iBW);

			selrig->set_vfoA(nuvals.freq);
			vfoA.freq = selrig->get_vfoA();
			xml_A.update_freq(vfoA.freq);

			vfo = &vfoA;
			Fl::awake(set_Mode_BW_control);
			Fl::awake(setFreqDispA);
			return;
		}
	}
	if (vfoA.iBW != nuvals.iBW) {
		selrig->set_bwA(vfoA.iBW = nuvals.iBW);
		selrig->get_bwA();
		xml_A.update_bw(vfoA.iBW);
	}
	if (vfoA.freq != nuvals.freq) {
		trace(1, "change vfoA frequency");
		selrig->set_vfoA(vfoA.freq =nuvals.freq);
		selrig->get_vfoA();
		xml_A.update_freq( vfoA.freq );
	}
	vfo = &vfoA;

	Fl::awake(setFreqDispA);
}

void serviceB(XCVR_STATE nuvals)
{
	if (nuvals.freq == 0) nuvals.freq = vfoB.freq;
	if (nuvals.imode == -1) nuvals.imode = vfoB.imode;
	if (nuvals.iBW == 255) nuvals.iBW = vfoB.iBW;

	if (selrig->inuse == onA) {
		if (selrig->can_change_alt_vfo) {
			trace(2, "A active, set alt vfo B", printXCVR_STATE(nuvals).c_str());
			if (vfoB.imode != nuvals.imode) {
				if (selrig->name_ == rig_FT891.name_) {
					// Mode change on ft891 requires mode first, so set all values
					yaesu891UpdateB(&nuvals);
					vfoB = nuvals;
				} else {
					selrig->set_modeB(vfoB.imode = nuvals.imode);
					vfoB.imode = selrig->get_modeB();
					xml_B.update_mode(vfoB.imode);

					selrig->set_bwB(vfoB.iBW = nuvals.iBW);
					vfoB.iBW = selrig->get_bwB();
					xml_B.update_bw(vfoB.iBW);

					selrig->set_vfoB(vfoB.freq = nuvals.freq);
					vfoB.freq = selrig->get_vfoB();
					xml_B.update_freq(vfoB.freq);
					Fl::awake(setFreqDispB);
					return;
				}
			}
			if (vfoB.iBW != nuvals.iBW) {
				selrig->set_bwB(nuvals.iBW);
				selrig->get_bwB();
			}
			if (vfoB.freq != nuvals.freq) {
				selrig->set_vfoB(nuvals.freq);
				selrig->get_vfoB();
			}
			vfoB = nuvals;
		} else if (xcvr_name != rig_TT550.name_) {
			trace(2, "A active, set vfo B", printXCVR_STATE(nuvals).c_str());
			selrig->selectB();
			selrig->set_modeB(nuvals.imode);
			vfoB.imode = selrig->get_modeB();

			selrig->set_bwB(nuvals.iBW);
			vfoB.iBW = selrig->get_bwB();

			selrig->set_vfoB(nuvals.freq);
			vfoB.freq = selrig->get_vfoB();

			selrig->selectA();
			vfoB = nuvals;
		}
		Fl::awake(setFreqDispB);
		return;
	}

	trace(2, "service VFO B", printXCVR_STATE(nuvals).c_str());

	if (nuvals.imode != -1) {
		if (selrig->name_ == rig_FT891.name_) {
			// Mode change on ft891 can change frequency, so set all values
			yaesu891UpdateB(&nuvals);
			vfoB = nuvals;
			set_bandwidth_control();
		} else {
			selrig->set_modeB(vfoB.imode =nuvals.imode);
			vfoB.imode = selrig->get_modeB();
			selrig->set_bwB(nuvals.iBW);
			vfoB.iBW = selrig->get_bwB();
			selrig->set_vfoB(nuvals.freq);
			vfoB.freq = selrig->get_vfoB();
			Fl::awake(set_Mode_BW_control);
			Fl::awake(setFreqDispB);
			vfo = &vfoB;
			xml_B.update_mode( vfoB.imode );
			xml_B.update_bw( nuvals.iBW );
			xml_B.update_freq( nuvals.freq );
			return;
		}
	}

	if (vfoB.iBW != nuvals.iBW) {
		selrig->set_bwB(vfoB.iBW = nuvals.iBW);
		selrig->get_bwB();
		xml_B.update_bw( vfoB.iBW );
	}
	if (vfoB.freq != nuvals.freq) {
		selrig->set_vfoB(vfoB.freq = nuvals.freq);
		selrig->get_vfoB();
		xml_B.update_freq( vfoB.freq );
	}

	vfo = &vfoB;

	Fl::awake(setFreqDispB);

}

#define MAX_FAILURES 5

void serial_failed(void *)
{
	bypass_serial_thread_loop = true;
	RigSerial->ClosePort();

	box_xcvr_connect->color(FL_BACKGROUND2_COLOR);
	box_xcvr_connect->redraw();

	fl_alert2(_("\
Transceiver not responding!\n\n\
Check serial (COM) port connection\n\
Open menu Config/Setup/Transceiver\n\
Press 'Update' button, reselect port\n\
Check that Baud matches transceiver baud\n\n\
Press 'Init' button."));
}

struct POLL_PAIR {
	int *poll;
	void (*pollfunc)();
	std::string name;
};

POLL_PAIR RX_poll_group_1[] = {
	{&progStatus.poll_smeter, read_smeter, "SMETER"},
	{&progStatus.poll_frequency, read_vfo, "FREQ"},
	{NULL, NULL, ""}
};

POLL_PAIR RX_poll_group_2[] = {
	{&progStatus.poll_mode, read_mode, "MODE"},
	{&progStatus.poll_bandwidth, read_bandwidth, "BW"},
//	{&progStatus.poll_vfoAorB, read_vfoAorB, "A/B"},
	{NULL, NULL, ""}
};

POLL_PAIR RX_poll_group_3[] = {
	{&progStatus.poll_volume, read_volume, "volume"},
	{&progStatus.poll_micgain, read_mic_gain, "mic gain"},
	{&progStatus.poll_rfgain, read_rfgain, "rfgain"},
	{&progStatus.poll_power_control, read_power_control, "power"},
	{&progStatus.poll_ifshift, read_ifshift, "if shift"},
	{&progStatus.poll_notch, read_notch, "notch"},
	{&progStatus.poll_auto_notch, read_auto_notch, "auto notch"},
	{&progStatus.poll_voltage, read_voltmeter, "voltage"},
	{&progStatus.poll_pre_att, read_preamp, "preamp"},
	{&progStatus.poll_pre_att, read_att, "atten"},
	{&progStatus.poll_squelch, read_squelch, "squelch"},
	{&progStatus.poll_split, read_split, "split"},
	{&progStatus.poll_break_in, check_break_in, "break-in"},
	{&progStatus.poll_noise, read_noise, "noise"},
	{&progStatus.poll_nr, read_nr, "noise reduction"},
	{&progStatus.poll_compression, read_compression, "compression"},
	{&progStatus.poll_pbt, read_pbt, "pass band tunning"},
	{NULL, NULL}
};

POLL_PAIR TX_poll_pairs[] = {
	{&progStatus.poll_power_control, read_power_control, "power"},
	{&progStatus.poll_pout, read_power_out, "pout"},
	{&progStatus.poll_swr, read_swr, "swr"},
	{&progStatus.poll_alc, read_alc, "alc"},
	{&progStatus.poll_alc, read_idd, "idd"},
	{&progStatus.poll_voltage, read_voltmeter, "voltage"},
	{&progStatus.poll_split, read_split, "split"},
	{NULL, NULL, ""}
};

static int menu1 = 0, menu2 = 1;
static FILE *qcx_menus = (FILE *)0;

void read_menu()
{
	if (menu1 == 0) return;
	std::string menu_item;
	if (menu1 < 8) {
		menu_item = selrig->read_menu(menu1, menu2);
		if (!menu_item.empty()) {
			if (qcx_menus)
				fprintf(qcx_menus, "%s\n", menu_item.c_str());
			++menu2;
		} else {
			menu2 = 1;
			++menu1;
		}
		return;
	}
	if (qcx_menus)
		fclose(qcx_menus);
	qcx_menus = 0;
}

void read_menus()
{
	std::string qcx_menu_file;
	qcx_menu_file.assign(RigHomeDir).append("qcx_menus.txt");
	qcx_menus = fopen(qcx_menu_file.c_str(), "w");
	menu1 = menu2 = 1;
}

int read_tune()
{
	tunerval = selrig->get_tune();
	if (tunerval != btn_tune_on_off->value()) {
		std::stringstream s;
		s << "tuner state: " << tunerval;
		trace(1, s.str().c_str());
		Fl::awake(update_UI_TUNER);
	}
	if (tunerval == 2) return 1;
	return 0;
}

void * serial_thread_loop(void *d)
{
extern bool xmlrpc_pending;

	POLL_PAIR *rx_poll_group_1 = &RX_poll_group_1[0];
	POLL_PAIR *rx_poll_group_2 = &RX_poll_group_2[0];
	POLL_PAIR *rx_poll_group_3 = &RX_poll_group_3[0];
	POLL_PAIR *tx_polling = &TX_poll_pairs[0];
	bool isRX = false;

	for(;;) {

		MilliSleep(progStatus.serloop_timing);

		if (!run_serial_thread) {
			break;
		}

		if (bypass_serial_thread_loop || disable_polling->value()) {
			goto serial_bypass_loop;
		}

		if (xmlrpc_pending)
			goto serial_bypass_loop;

		if (RigSerial->failed() >= MAX_FAILURES) {
			Fl::awake(serial_failed);
		}

//		if (progStatus.poll_ptt) {
		{
			guard_lock lk( &mutex_serial, std::string(__func__).append(" ").append("ptt"));
			check_ptt();
		}
		if (xmlrpc_pending) goto serial_bypass_loop;

		if (PTT || (!PTT && read_tune()) ||
			cwio_process == SEND ||
			cwio_process == CALIBRATE ||
			cwio_process == KEYDOWN ) {
			if (isRX) {
				isRX = false;
				smtrval = 0;
				Fl::awake(update_UI_PTT);
			}
			Fl::awake(show_meters, (void *)1);

			if (progStatus.poll_frequency) {
				guard_lock lk( &mutex_serial,
					std::string(__func__).append(" ").append("vfo"));
				read_vfo();
			}
			if (xmlrpc_pending) goto serial_bypass_loop;

			while ( tx_polling->poll != NULL ) {
				Fl::awake();
				if (xmlrpc_pending ||
					!(PTT ||
					  (cwio_process == SEND) ||
					  (cwio_process == CALIBRATE) ||
					  (cwio_process == KEYDOWN) )) break;
				if (*tx_polling->poll)  {
					guard_lock lk( &mutex_serial,
						std::string(__func__).append(" ").append(tx_polling->name));
					(tx_polling->pollfunc)();
//					check_ptt();
				} else
					MilliSleep(10);
				++tx_polling;
			}
			if (tx_polling->poll == NULL)
				tx_polling = &TX_poll_pairs[0];
			if (xmlrpc_pending) goto serial_bypass_loop;

		} else {

			if (!isRX) {
				isRX = true;
				Fl::awake(update_UI_PTT);
				Fl::awake(zeroXmtMeters, 0);
			}
			Fl::awake(show_meters, (void *)0);

			while ( (rx_poll_group_1->poll != NULL) ) {
				Fl::awake();
				if (xmlrpc_pending || PTT) break;
				if (*rx_poll_group_1->poll) {
					guard_lock lk( &mutex_serial,
						std::string(__func__).append(" ").append(rx_poll_group_1->name));
					(rx_poll_group_1->pollfunc)();
//					check_ptt();
				} else
					MilliSleep(10);
				++rx_poll_group_1;
			}
			if ((rx_poll_group_1)->poll == NULL)
				rx_poll_group_1 = &RX_poll_group_1[0];
			if (xmlrpc_pending) goto serial_bypass_loop;

			while ( (rx_poll_group_2->poll != NULL) ) {
				Fl::awake();
				if (xmlrpc_pending || PTT) break;
				if (*rx_poll_group_2->poll) {
					guard_lock lk( &mutex_serial,
						std::string(__func__).append(" ").append(rx_poll_group_2->name));
					(rx_poll_group_2->pollfunc)();
//					check_ptt();
				} else
					MilliSleep(10);
				++rx_poll_group_2;
			}
			if ((rx_poll_group_2)->poll == NULL)
				rx_poll_group_2 = &RX_poll_group_2[0];
			if (xmlrpc_pending) goto serial_bypass_loop;

			while ( (rx_poll_group_3->poll != NULL) ) {
				Fl::awake();
				if (xmlrpc_pending || PTT) break;
				if (*rx_poll_group_3->poll) {
					guard_lock lk( &mutex_serial,
						std::string(__func__).append(" ").append(rx_poll_group_3->name));
					(rx_poll_group_3->pollfunc)();
//					check_ptt();
				} else
					MilliSleep(10);
				++rx_poll_group_3;
			}
			if ((rx_poll_group_3)->poll == NULL)
				rx_poll_group_3 = &RX_poll_group_3[0];
			if (xmlrpc_pending) goto serial_bypass_loop;

			if (menu1 < 9  && selrig->name_ == rig_QCXP.name_) {
				guard_lock lk( &mutex_serial,
					std::string(__func__).append(" ").append("QCX+"));
				read_menu();
			}

		}
serial_bypass_loop: ;
	}
	return NULL;

}

//=============================================================================

void setBW()
{
	guard_lock serlock( &mutex_serial, std::string(__func__) );
	if (selrig->inuse == onB) {
		vfo->iBW = vfoB.iBW = opBW->index();
		selrig->set_bwB(vfo->iBW);
	} else {
		vfo->iBW = vfoA.iBW = opBW->index();
		selrig->set_bwA(vfo->iBW);
	}
}

void setDSP()
{
	XCVR_STATE fm = *vfo;
	fm.src = UI;
	fm.iBW = ((opDSP_hi->index() << 8) | 0x8000) | (opDSP_lo->index() & 0xFF) ;
	serviceXCVR( VFOQUEUE((selrig->inuse == onB ? vB : vA), fm) );
}

void selectDSP()
{
	if (btnDSP->label()[0] == selrig->SL_label[0]) {
		btnDSP->label(selrig->SH_label);
		btnDSP->redraw_label();
		opDSP_lo->hide();
		opDSP_hi->show();
		opDSP_hi->redraw();
	} else {
		btnDSP->label(selrig->SL_label);
		btnDSP->redraw_label();
		opDSP_hi->hide();
		opDSP_lo->show();
		opDSP_lo->redraw();
	}
}

void selectFILT()
{
	guard_lock lock( &mutex_serial, std::string(__func__) );
	btnFILT->label(selrig->nextFILT());
	btnFILT->redraw_label();
	if (selrig->inuse == onB) {
		vfoB.iBW = vfo->iBW = selrig->get_bwB();
		xml_B.update_bw(vfoB.iBW);
	} else {
		vfoA.iBW = vfo->iBW = selrig->get_bwA();
		xml_A.update_bw(vfoA.iBW);
	}
	setBWControl(NULL);
}

void selectCENTER()
{
	if (xcvr_name == rig_K2.name_ ||
		xcvr_name == rig_K3.name_ ||
		xcvr_name == rig_KX3.name_ ||
		xcvr_name == rig_K4.name_) {
		return;
	}

	if (btnCENTER->label()[0] == 'C') {
		btnCENTER->label("W");
		opBW->show();
		opCENTER->hide();
	} else {
		btnCENTER->label("C");
		opBW->hide();
		opCENTER->index(progStatus.tci_center);
		opCENTER->show();
	}
	opBW->redraw();
	opCENTER->redraw();
	btnCENTER->redraw_label();
}

static int lastTCI_bw = -1;
static int lastTCI_inner = -1;
static int lastTCI_outer = -1;
static int lastTCI_mode = -1;

void updateTCI(void *d)
{
	if (!(	selrig->name_ == rig_tci_sundx.name_ ||
			selrig->name_ == rig_tci_sunpro.name_) )
		return;

	vfo->FilterInner = selrig->get_pbt_inner();
	vfo->FilterOuter = selrig->get_pbt_outer();

	if (lastTCI_mode != vfo->imode) {
		lastTCI_mode = vfo->imode;
		switch (vfo->imode) {
			case TCI_AM: case TCI_SAM: case TCI_DSB: case TCI_NFM:
				opFilterInner->minimum(-8000);	opFilterInner->maximum(0);
				opFilterOuter->minimum(0); 		opFilterOuter->maximum(8000);
				break;
			case TCI_LSB:
				opFilterInner->minimum(-4000);	opFilterInner->maximum(0);
				opFilterOuter->minimum(-4000); 	opFilterOuter->maximum(0);
				break;
			case TCI_USB:
				opFilterInner->minimum(0);	opFilterInner->maximum(4000);
				opFilterOuter->minimum(0);	opFilterOuter->maximum(4000);
				break;
			default: case TCI_CW:
				opFilterInner->minimum(-1000);	opFilterInner->maximum(0);
				opFilterOuter->minimum(0);	opFilterOuter->maximum(1000);
				break;
			case TCI_DIGL: case TCI_DIGU:
				opFilterInner->minimum(-4000);	opFilterInner->maximum(0);
				opFilterOuter->minimum(0); 		opFilterOuter->maximum(4000);
				break;
			case TCI_WFM:
				opFilterInner->minimum(-9000);	opFilterInner->maximum(0);
				opFilterOuter->minimum(0); 		opFilterOuter->maximum(9000);
				break;
			case TCI_DRM:
				opFilterInner->minimum(-5000);	opFilterInner->maximum(0);
				opFilterOuter->minimum(0); 		opFilterOuter->maximum(5000);
				break;
		}

		updateBandwidthControl((void *)0);
	}

	if (lastTCI_bw != vfo->iBW) opBW->index(lastTCI_bw = vfo->iBW);
	if (lastTCI_inner != vfo->FilterInner) opFilterInner->value(lastTCI_inner = vfo->FilterInner);
	if (lastTCI_outer != vfo->FilterOuter) opFilterOuter->value(lastTCI_outer = vfo->FilterOuter);

	std::string smode = opMODE->value();
	if (smode == "USB" || smode == "LSB") {
		btnCENTER->activate();
		btnCENTER->redraw();
		btnCENTER->show();
		if (btnCENTER->label()[0] == 'C') {
			opCENTER->index(progStatus.tci_center);
			opCENTER->show();
			opBW->hide();
		} else {
			opBW->show();
			opCENTER->hide();
		}
		opBW->resize(opDSP_lo->x(), opDSP_lo->y(), opDSP_lo->w(), opDSP_lo->h());
		opCENTER->resize(opDSP_lo->x(), opDSP_lo->y(), opDSP_lo->w(), opDSP_lo->h());
		opCENTER->redraw();
		opBW->redraw();
	} else {
		btnCENTER->label("W");
		btnCENTER->redraw_label();
		btnCENTER->deactivate();
		btnCENTER->show();
		opCENTER->hide();
		if (vfo->iBW != opBW->index())
			opBW->index(vfo->iBW);
		opBW->resize(opDSP_lo->x(), opDSP_lo->y(), opDSP_lo->w(), opDSP_lo->h());
		opBW->redraw();
		opBW->show();
	}
} 

void updateFLEX1500(void *d)
{
	if (!(	selrig->name_ == rig_FLEX1500.name_) )
		return;

	btnCENTER->hide();
	opCENTER->hide();

	switch (vfo->imode) {
		case RIG_FLEX1500::AM :
		case RIG_FLEX1500::SAM :
		case RIG_FLEX1500::DSB :
		case RIG_FLEX1500::FM :
			sldrINNER->minimum(-8000); sldrINNER->maximum(0); sldrINNER->step(10);
			sldrOUTER->minimum(0); sldrOUTER->maximum(8000); sldrOUTER->step(10);
			break;
		case RIG_FLEX1500::LSB :
			sldrINNER->minimum(-4000); sldrINNER->maximum(0); sldrINNER->step(10);
			sldrOUTER->minimum(-4000); sldrOUTER->maximum(0); sldrOUTER->step(10);
			break;
		case RIG_FLEX1500::USB :
			sldrINNER->minimum(0); sldrINNER->maximum(4000); sldrINNER->step(10);
			sldrOUTER->minimum(0); sldrOUTER->maximum(4000); sldrOUTER->step(10);
			break;
		default: case RIG_FLEX1500::CWL :
			sldrINNER->minimum(-1500); sldrINNER->maximum(0); sldrINNER->step(5);
			sldrOUTER->minimum(-1500); sldrOUTER->maximum(0); sldrOUTER->step(5);
			break;
		case RIG_FLEX1500::CWU:
			sldrINNER->minimum(0); sldrINNER->maximum(1500); sldrINNER->step(5);
			sldrOUTER->minimum(0); sldrOUTER->maximum(1500); sldrOUTER->step(5);
			break;
		case RIG_FLEX1500::DIGL: case RIG_FLEX1500::DIGU:
			sldrINNER->minimum(0); sldrINNER->maximum(1500); sldrINNER->step(10);
			sldrOUTER->minimum(0); sldrOUTER->maximum(1500); sldrOUTER->step(10);
			break;
		case RIG_FLEX1500::DRM:
			sldrINNER->minimum(-5000); sldrINNER->maximum(0); sldrINNER->step(10);
			sldrOUTER->minimum(0); sldrOUTER->maximum(5000); sldrOUTER->step(10);
			break;
	}

	std::string sBW = opBW->value();
	if (sBW.find("Var") != std::string::npos) {
		sldrINNER->activate();
		sldrOUTER->activate();
	} else {
		sldrINNER->deactivate();
		sldrOUTER->deactivate();
	}

	updateBandwidthControl((void *)0);

} 

// set_bandwidth_control updates iBW and then posts the call for
// the UI thread to updateBandwidthControl
// changes to the UI cannot come from any thread other than the
// main FL thread!  Doing otherwise can cause what appears to be
// random program crashes.

void set_bandwidth_control()
{
	if (!selrig->has_bandwidth_control) return;

//	if (selrig->name_ != rig_K3.name_) {
//		vfo->iBW = selrig->def_bandwidth(vfo->imode);
//		if (vfo->iBW < 256) {
//			vfo->iBW = selrig->def_bandwidth(vfo->imode);
//		}
//	}
	if (selrig->inuse == onB) {
		vfoB.iBW = vfo->iBW = selrig->get_bwB();
		xml_B.update_bw(vfoB.iBW);
	} else {
		vfoA.iBW = vfo->iBW = selrig->get_bwA();
		xml_A.update_bw(vfoA.iBW);
	}

	Fl::awake(updateBandwidthControl);
}

void TRACED ( updateBandwidthControl, void *d )
	if (//xcvr_name == rig_K3.name_ ||
		xcvr_name == rig_KX3.name_
		|| xcvr_name == rig_K4.name_
		) {
		return;
	}
	if (selrig->has_dsp_controls) {
		if (vfo->iBW > 256) {
			opDSP_lo->clear();
			opDSP_hi->clear();
			try {
				for (size_t i = 0; i < selrig->dsp_SL.size(); i++)
					opDSP_lo->add(selrig->dsp_SL.at(i).c_str());
				for (size_t i = 0; i < selrig->dsp_SH.size(); i++)
					opDSP_hi->add(selrig->dsp_SH.at(i).c_str());
			} catch (const std::exception& e) {
				LOG_ERROR("%s", e.what());
			}
			opBW->hide();
			opDSP_lo->index(vfo->iBW & 0xFF);
			opDSP_lo->hide();
			opDSP_hi->index((vfo->iBW >> 8) & 0x7F);
			btnDSP->label(selrig->SH_label);
			opDSP_hi->show();
			opDSP_hi->redraw();
			btnDSP->show();
			btnDSP->redraw();
			btnFILT->hide();
		} else  {
			opDSP_lo->hide();
			opDSP_hi->hide();
			btnDSP->hide();
			btnFILT->hide();
//			opBW->clear();
			try {
				if (vfo->iBW != opBW->index())
					opBW->index(vfo->iBW);
			} catch (const std::exception& e) {
				LOG_ERROR("%s", e.what());
				opBW->index(0);
			}
			opBW->redraw();
			opBW->show();
		}
	}
	else if (selrig->has_bandwidth_control) {
		opDSP_lo->hide();
		opDSP_hi->hide();
		btnDSP->hide();
//		opBW->clear();
		try {
			if (vfo->iBW != opBW->index())
				opBW->index(vfo->iBW);
		} catch (const std::exception& e) {
			LOG_ERROR("%s", e.what());
			opBW->index(0);
		}
		opBW->redraw();

		if (selrig->has_FILTER) {
			btnFILT->show();
			btnFILT->label(
				selrig->FILT(
					selrig->inuse == onB ? vfoB.filter : vfoA.filter));
			btnFILT->redraw_label();
			opBW->resize(opDSP_lo->x(), opDSP_lo->y(), opDSP_lo->w(), opDSP_lo->h());
			opBW->redraw();
		} else {
			opBW->show();
		}
	}
	else { // no BW, no DSP controls
		opBW->index(0);
		opBW->hide();
		btnFILT->hide();
		opDSP_lo->index(0);
		opDSP_hi->index(0);
		btnDSP->hide();
		opDSP_lo->hide();
		opDSP_hi->hide();
	}

//	if (selrig->name_ == rig_FLEX1500.name_) {
//		std::string sBW = opBW->value();
//		if (sBW.find("Var") != std::string::npos) {
//			sldrINNER->activate();
//			sldrOUTER->activate();
//		} else {
//			sldrINNER->deactivate();
//			sldrOUTER->deactivate();
//		}
//		sldrINNER->redraw();
//		sldrOUTER->redraw();
//	}
}

void setMode()
{
	guard_lock serlock( &mutex_serial, std::string(__func__) );
	if (selrig->inuse == onB) {
		vfo->imode = vfoB.imode =opMODE->index();
		selrig->set_modeB(vfoB.imode);
		xml_B.update_mode(vfoB.imode);

		vfo->iBW = vfoB.iBW = selrig->def_bandwidth(vfo->imode);
		selrig->set_bwB(vfo->iBW);
		xml_B.update_bw( vfoB.iBW );
	} else {
		vfo->imode = vfoA.imode = opMODE->index();
		selrig->set_modeA(vfo->imode);
		xml_A.update_mode(vfoA.imode);

		vfo->iBW = vfoA.iBW = selrig->def_bandwidth(vfo->imode);
		selrig->set_bwA(vfo->iBW);
		xml_A.update_bw( vfoA.iBW );
	}
	set_Mode_BW_control(NULL);
}

void sortList() {
	if (numinlist < 2) return;
	bool swap = false;
	int comp = 0;
	ATAG_XCVR_STATE temp, *A, *B;
	for (int i = 0; i < numinlist - 1; i++) {
		A = &oplist[i];
		for (int j = i + 1; j < numinlist; j++) {
			B = &oplist[j];
			swap = false;
			if (progStatus.mem_sortby == "FREQ") {
				if (A->freq > B->freq) swap = true;
				else if (A->freq == B->freq) {
					if (A->imode > B->imode) swap = true;
					else if (A->imode == B->imode) {
						if (A->iBW > B->iBW) swap = true;
						else {
							comp = strcasecmp(A->alpha_tag, B->alpha_tag);
							if (comp > 0) swap = true;
						}
					}
				}
			} else if (progStatus.mem_sortby == "BW") {
				if (A->iBW > B->iBW) swap = true;
				else if (A->iBW == B->iBW) {
					if (A->freq > B->freq) swap = true;
					else if (A->freq == B->freq) {
						if (A->imode > B->imode) swap = true;
						else if (A->imode == B->imode) {
							comp = strcasecmp(A->alpha_tag, B->alpha_tag);
							if (comp > 0) swap = true;
						}
					}
				}
			} else if (progStatus.mem_sortby == "MODE") {
				if (A->imode > B->imode) swap = true;
				else if (A->imode == B->imode) {
					if (A->freq > B->freq) swap = true;
					else if (A->freq == B->freq) {
						if (A->iBW > B->iBW) swap = true;
						else if (A->iBW == B->iBW) {
							comp = strcasecmp(A->alpha_tag, B->alpha_tag);
							if (comp > 0) swap = true;
						}
					}
				}
			} else {
				comp = strcasecmp(A->alpha_tag, B->alpha_tag);
				if (comp > 0) swap = true;
				else if (comp == 0) {
					if (A->freq > B->freq) swap = true;
					else if (A->freq == B->freq) {
					if (A->imode > B->imode) swap = true;
						else if (A->imode == B->imode) {
							if (A->iBW > B->imode) swap = true;
						}
					}
				}
			}
			if (swap) {
				temp = oplist[i];
				oplist[i] = oplist[j];
				oplist[j] = temp;
			}
		}
	}
}

void clearList() {
	if (!numinlist) return;
	for (int i = 0; i < LISTSIZE; i++) {
		oplist[i].freq = 0;
		oplist[i].imode = USB;
		oplist[i].iBW = 0;
		memset(oplist[i].alpha_tag, 0, ATAGSIZE);
	}
	FreqSelect->clear();
	numinlist = 0;
	inAlphaTag->value("");
}

void updateSelect() {
	char szline[1000];
	char szatag[ATAGSIZE];
	int i;
	FreqSelect->clear();
	if (!numinlist) return;
	sortList();

	for (int n = 0; n < numinlist; n++) {
		memset(szline, 0, sizeof(szline));
		memset(szatag, 0, sizeof(szatag));
		for (i = 0; i < ATAGSIZE; i++) {
			szatag[i] = oplist[n].alpha_tag[i];
			if (szatag[i] == '\n') szatag[i] = ' ';
		}
		
		snprintf(szline, sizeof(szline),
"@F%d@S%d@r%.3f\t\
@F%d@S%d@.|\t\
@F%d@S%d@r%s\t\
@F%d@S%d@.|\t\
@F%d@S%d@r%s\t\
@F%d@S%d@.|\t\
@F%d@S%d@.%s",
			progStatus.memfontnbr, progStatus.memfontsize, oplist[n].freq / 1000.0,
			progStatus.memfontnbr, progStatus.memfontsize, 
			progStatus.memfontnbr, progStatus.memfontsize, selrig->get_bwname_(oplist[n].iBW, oplist[n].imode),
			progStatus.memfontnbr, progStatus.memfontsize, 
			progStatus.memfontnbr, progStatus.memfontsize, selrig->get_modename_(oplist[n].imode),
			progStatus.memfontnbr, progStatus.memfontsize, 
			progStatus.memfontnbr, progStatus.memfontsize, szatag );

		FreqSelect->add (szline);
	}
	inAlphaTag->value("");
}

void addtoList(unsigned long long val, int imode, int iBW) {
	if (numinlist < LISTSIZE) {
		oplist[numinlist].imode = imode;
		oplist[numinlist].freq = val;
		oplist[numinlist].iBW = iBW;
		memset(oplist[numinlist].alpha_tag, 0, ATAGSIZE);
		numinlist++;
	}
}

void readFile() {
	std::ifstream iList(defFileName.c_str());
	if (!iList) {
		fl_message ("Could not open %s", defFileName.c_str());
		return;
	}
	clearList();
	int i = 0, mode, bw;
	unsigned long long freq;
	while (!iList.eof()) {
		freq = 0ULL; mode = -1;
		iList >> freq >> mode >> bw;
		if (freq && (mode > -1)) {
			oplist[i].freq = freq;
			oplist[i].imode = mode;
			oplist[i].iBW = (bw == -1 ? 0 : bw);
			memset(oplist[i].alpha_tag, 0, ATAGSIZE);
			i++;
		}
	}
	iList.close();
	numinlist = i;
	updateSelect();
}

void readTagFile() {
	std::ifstream iList(defFileName.c_str());
	if (!iList) {
		fl_message ("Could not open %s", defFileName.c_str());
		return;
	}
	clearList();
	int i = 0, mode, bw;
	unsigned long long freq;
	std::string atag;
	char ca[ATAGSIZE + 60];
	while (!iList.eof()) {
		freq = 0ULL; mode = -1;
		atag.clear();
		memset(ca, 0, sizeof(ca));
		iList >> freq >> mode >> bw;
		iList.getline(ca, sizeof(ca) - 1);
		atag = ca;
		if (freq && (mode > -1)) {
			oplist[i].freq = freq;
			oplist[i].imode = mode;
			oplist[i].iBW = (bw == -1 ? 0 : bw);
// trim leading, trailing spaces and double quotes
			atag = lt_trim(atag);
			snprintf(oplist[i].alpha_tag, ATAGSIZE, "%s", atag.c_str());
			i++;
		}
	}
	iList.close();
	numinlist = i;
	updateSelect();
}

void buildlist() {
	std::string tmpFN, orgFN;
// check for new Memory-Alpha-Tag file
	defFileName = RigHomeDir;
	defFileName.append(selrig->name_);
	defFileName.append(".mat");
	FILE *fh = fopen(defFileName.c_str(), "r");
	if (fh != NULL) {
		fclose (fh);
		readTagFile();
		return;
	}
// else only read original file to make new MAT file
	orgFN = RigHomeDir;
	orgFN.append(selrig->name_);
	orgFN.append(".arv");
	fh = fopen(orgFN.c_str(), "r");
	if (fh != NULL) {
		fclose (fh);
		tmpFN = defFileName;
		defFileName = orgFN;
		readFile();
		defFileName = tmpFN;
		return;
	}
	clearList();
}

// flrig front panel changed

void movFreqA(Fl_Widget *, void *) {
	guard_lock serial( &mutex_serial, std::string(__func__) );
	if (!selrig->can_change_alt_vfo  && selrig->inuse == onB) {
		selrig->selectA();
		vfoA.freq =FreqDispA->value();
		selrig->set_vfoA(vfoA.freq);
		selrig->selectB();
	} else {
		vfoA.freq =FreqDispA->value();
		selrig->set_vfoA(vfoA.freq);
	}
	xml_A.update_freq( vfoA.freq );
}

void movFreqB(Fl_Widget *, void *) {
	guard_lock serial( &mutex_serial, std::string(__func__) );
	if (!selrig->can_change_alt_vfo  && selrig->inuse == onA) {
		selrig->selectB();
		vfoB.freq = FreqDispB->value();
		selrig->set_vfoB(vfoB.freq);
		selrig->selectA();
	} else {
		vfoB.freq = FreqDispB->value();
		selrig->set_vfoB(vfoB.freq);
	}
	xml_B.update_freq( vfoB.freq );
}

void execute_swapAB()
{
	if (selrig->canswap()) {
		if (selrig->ICOMmainsub) {
			selrig->swapAB();
			XCVR_STATE temp = vfoA;
			vfoA = vfoB;
			vfoB = temp;
			selrig->selectA();
			vfo = &vfoA;
		} else if (selrig->ICOMrig) {
			selrig->swapAB();
			if (selrig->inuse == onB) {
				selrig->selectA();
				vfo = &vfoA;
			} else {
				selrig->selectB();
				vfo = &vfoB;
			}
		} else if (selrig->name_ == rig_FT891.name_) {
			selrig->swapAB();
		} else if (selrig->name_ == rig_FTdx10.name_ ) {
			selrig->swapAB();
			XCVR_STATE temp = vfoB;
			vfoB = vfoA;
			vfoA = temp;
			if (selrig->inuse == onB)
				vfo = &vfoB;
			else
				vfo = &vfoA;
		} else {
			selrig->swapAB();
			XCVR_STATE temp = vfoB;
			vfoB = vfoA;
			vfoA = temp;
			if (selrig->inuse == onB) {
				selrig->selectB();
				vfo = &vfoB;
			} else {
				selrig->selectA();
				vfo = &vfoA;
			}
		}
	} else {
		if (selrig->inuse == onB) {
			XCVR_STATE vfotemp = vfoA;
			selrig->selectA();
			vfoA = vfoB;
			selrig->set_modeA(vfoA.imode);
			selrig->set_vfoA(vfoA.freq);
			selrig->set_bwA(vfoA.iBW);
			vfoA.freq = selrig->get_vfoA();
			vfoA.imode = selrig->get_modeA();
			vfoA.iBW = selrig->get_bwA();
			xml_A.update_freq(vfoA.freq);
			xml_A.update_mode(vfoA.imode);
			xml_A.update_bw(vfoA.iBW);

			selrig->selectB();
			vfoB = vfotemp;
			selrig->set_modeB(vfoB.imode);
			selrig->set_vfoB(vfoB.freq);
			selrig->set_bwB(vfoB.iBW);
			vfoB.freq = selrig->get_vfoB();
			vfoB.imode = selrig->get_modeB();
			vfoB.iBW = selrig->get_bwB();
			xml_B.update_freq(vfoB.freq);
			xml_B.update_mode(vfoB.imode);
			xml_B.update_bw(vfoB.iBW);
			vfo = &vfoB;
		} else {
			XCVR_STATE vfotemp = vfoB;
			selrig->selectB();
			vfoB = vfoA;
			selrig->set_modeB(vfoB.imode);
			selrig->set_vfoB(vfoB.freq);
			selrig->set_bwB(vfoB.iBW);
			vfoB.freq = selrig->get_vfoB();
			vfoB.imode = selrig->get_modeB();
			vfoB.iBW = selrig->get_bwB();
			xml_B.update_freq(vfoB.freq);
			xml_B.update_mode(vfoB.imode);
			xml_B.update_bw(vfoB.iBW);

			selrig->selectA();
			vfoA = vfotemp;
			selrig->set_modeA(vfoA.imode);
			selrig->set_vfoA(vfoA.freq);
			selrig->set_bwA(vfoA.iBW);
			vfoA.freq = selrig->get_vfoA();
			vfoA.imode = selrig->get_modeA();
			vfoA.iBW = selrig->get_bwA();
			xml_A.update_freq(vfoA.freq);
			xml_A.update_mode(vfoA.imode);
			xml_A.update_bw(vfoA.iBW);
vfo = &vfoA;
		}
	}
	Fl::awake(updateUI);
}

void cbAswapB()
{
	VFOQUEUE xcvr;
	if (Fl::event_button() == FL_RIGHT_MOUSE)
		xcvr.change = A2B;
	else {
		if ((Fl::event_state() & FL_SHIFT) == FL_SHIFT)
			xcvr.change = FA2FB;
		else if ((Fl::event_state() & FL_CTRL) == FL_CTRL)
			xcvr.change = FB2FA;
		else
			xcvr.change = SWAP;
	}
	serviceXCVR(xcvr);
}

void execute_A2B()
{
	if (xcvr_name == rig_K3.name_) {
		K3_A2B();
	} else if (xcvr_name == rig_KX3.name_ || xcvr_name == rig_K4.name_) {
		cb_KX3_A2B();
	} else if (xcvr_name == rig_K2.name_) {
		trace(1,"execute A2B() 1");
		vfoB = vfoA;
		selrig->set_vfoB(vfoB.freq);
		selrig->get_vfoB();
		FreqDispB->value(vfoB.freq);
	}
	if (selrig->ICOMmainsub) {
		trace(1, "execute A2B() 2");
		selrig->selectA();
		selrig->A2B();
		vfoB = vfoA;
		vfo = &vfoA;
	} else if (selrig->has_a2b) {
		trace(1,"execute A2B() 3");
		selrig->A2B();
		if (selrig->inuse == onB) {
			vfoA = vfoB;
			FreqDispA->value(vfoA.freq);
		} else {
			vfoB = vfoA;
			FreqDispB->value(vfoB.freq);
		}
	} else {
		trace(1,"execute A2B() 4");
		if (selrig->inuse == onB) {
			vfoA = vfoB;
			if (selrig->name_ == rig_FT891.name_) {
				yaesu891UpdateA(&vfoA);
			} else {
				selrig->set_vfoA(vfoA.freq);
				selrig->set_modeA(vfoA.imode);
				selrig->set_bwA(vfoA.iBW);
				vfoA.freq = selrig->get_vfoA();
				vfoA.imode = selrig->get_modeA();
				vfoA.iBW = selrig->get_bwA();
				xml_A.update_freq(vfoA.freq);
				xml_A.update_mode(vfoA.imode);
				xml_A.update_bw(vfoA.iBW);
			}
			FreqDispA->value(vfoA.freq);
		} else {
			vfoB = vfoA;
			if (selrig->name_ == rig_FT891.name_) {
				yaesu891UpdateB(&vfoB);
			} else {
				selrig->set_vfoB(vfoB.freq);
				selrig->set_modeB(vfoB.imode);
				selrig->set_bwB(vfoB.iBW);
				vfoB.freq = selrig->get_vfoB();
				vfoB.imode = selrig->get_modeB();
				vfoB.iBW = selrig->get_bwB();
				xml_B.update_freq(vfoB.freq);
				xml_B.update_mode(vfoB.imode);
				xml_B.update_bw(vfoB.iBW);
			}
			FreqDispB->value(vfoB.freq);
		}
	}
	Fl::awake(updateUI);
}

void execute_FA2FB()
{
	vfoB.freq = vfoA.freq;
	xml_B.update_freq( vfoB.freq );
	selrig->set_vfoB(vfoB.freq);
	FreqDispB->value(vfoB.freq);
	Fl::awake(updateUI);
}

void execute_FB2FA()
{
	vfoA.freq =vfoB.freq;
	selrig->set_vfoA(vfoA.freq);
	FreqDispA->value(vfoA.freq);
	Fl::awake(updateUI);
	xml_A.update_freq( vfoA.freq );
}

void highlight_vfo(void *d)
{
	Fl_Color norm_fg = fl_rgb_color(progStatus.fg_red, progStatus.fg_green, progStatus.fg_blue);
	Fl_Color norm_bg = fl_rgb_color(progStatus.bg_red, progStatus.bg_green, progStatus.bg_blue);
	Fl_Color dim_bg = fl_color_average( norm_bg, FL_BLACK, 0.75);
	FreqDispA->value(vfoA.freq);
	FreqDispB->value(vfoB.freq);
	if (selrig->inuse == onB) {
		FreqDispA->SetCOLORS( norm_fg, dim_bg );
		FreqDispB->SetCOLORS( norm_fg, norm_bg );
		if (selrig->name_ == rig_tci_sundx.name_ || selrig->name_ == rig_tci_sunpro.name_) {
			btnA->value(1);
		} else {
			btnA->value(0);
			btnB->value(1);
		}
		FreqDispB->activate();
		if (selrig->can_change_alt_vfo)
			FreqDispA->activate();
		else
			FreqDispA->deactivate();
	} else {
		FreqDispA->SetCOLORS( norm_fg, norm_bg );
		FreqDispB->SetCOLORS( norm_fg, dim_bg);
		if (selrig->name_ == rig_tci_sundx.name_ || selrig->name_ == rig_tci_sunpro.name_) {
			btnA->value(0);
		} else {
			btnA->value(1);
			btnB->value(0);
		}
		FreqDispA->activate();
		if (selrig->can_change_alt_vfo)
			FreqDispB->activate();
		else
			FreqDispB->deactivate();
	}
	FreqDispA->redraw();
	FreqDispB->redraw();
	btnA->redraw();
	btnB->redraw();
	Fl::check();
}

void cb_set_split(int val)
{
	progStatus.split = val;
	VFOQUEUE xcvr_split;
	if (val) xcvr_split.change = sON;
	else       xcvr_split.change = sOFF;
	serviceXCVR(xcvr_split);
	trace(1, (val ? "cb_set_split(ON)" : "cb_set_split(OFF)"));
}

void cb_selectA()
{
	serviceXCVR(VFOQUEUE(sA, vfoA));
	trace(2, "cb_selectA() ", printXCVR_STATE(vfoA).c_str());
	return;
}

void cb_selectB()
{
	serviceXCVR(VFOQUEUE(sB, vfoB));
	trace(2, "cb_selectB() ", printXCVR_STATE(vfoB).c_str());
	return;
}

void setLower()
{
}

void setUpper()
{
}

void selectFreq() {
	int n = FreqSelect->value(); // This is the number of the selected line; not the line's value.
	if (!n) return;

	XCVR_STATE fm;
	fm.freq  = oplist[n-1].freq;
	fm.imode = oplist[n-1].imode;
	fm.iBW   = oplist[n-1].iBW;
	fm.src   = UI;

	guard_lock serial( &mutex_serial, std::string(__func__) );
	if (selrig->inuse == onB) {

		vfo->freq = vfoB.freq = fm.freq;
		selrig->set_vfoB(vfoB.freq);
		xml_B.update_freq(vfoB.freq);
		Fl::awake(setFreqDispB);

		vfo->imode = vfoB.imode = fm.imode;
		selrig->set_modeB(vfoB.imode);
		xml_B.update_mode(vfoB.imode);
		Fl::awake(set_Mode_BW_control);

		vfo->iBW = vfoB.iBW = fm.iBW;
		selrig->set_bwB(vfoB.iBW);
		xml_B.update_bw(vfoB.iBW);

	} else {

		vfo->freq = vfoA.freq = fm.freq;
		selrig->set_vfoA(fm.freq);
		xml_A.update_freq(vfoA.freq);
		Fl::awake(setFreqDispA);

		vfo->imode = vfoA.imode = fm.imode;
		selrig->set_modeA(fm.imode);
		xml_A.update_mode(vfoA.imode);
		Fl::awake(set_Mode_BW_control);

		vfo->iBW = vfoA.iBW = fm.iBW;
		selrig->set_bwA(vfo->iBW);
		xml_A.update_bw(vfoA.iBW);

	}

}

#include <FL/names.h>
void select_and_close()
{
	int key = Fl::event_key();
	int btn = Fl::event_button();

	if (FreqSelect->value() <= 0) return;

	if ((btn == FL_LEFT_MOUSE && Fl::event_clicks()) ||
		 btn == FL_RIGHT_MOUSE ||
		 key == FL_Enter ||
		 key == FL_Left) {
		inAlphaTag->value(oplist[FreqSelect->value() - 1].alpha_tag);
		selectFreq();
		Fl::focus(FreqSelect);
		return;
	}

	if (btn == FL_LEFT_MOUSE || key == FL_Up || key == FL_Down) {
		inAlphaTag->value(oplist[FreqSelect->value() - 1].alpha_tag);
		Fl::focus(FreqSelect);
		return;
	}

	if (key == FL_Right) {
		addFreq();
		FreqSelect->select(1, 1);
		inAlphaTag->value(oplist[0].alpha_tag);
		Fl::focus(FreqSelect);
		return;
	}

	if (key == FL_Delete) {
		long n = FreqSelect->value();
		delFreq();
		FreqSelect->select(n, 1);
		inAlphaTag->value(oplist[n-1].alpha_tag);
		Fl::focus(FreqSelect);
		return;
	}
}

void delFreq() {
	if (FreqSelect->value()) {
		long n = FreqSelect->value() - 1;
		for (int i = n; i < numinlist; i ++)
			oplist[i] = oplist[i+1];
		if (numinlist) {
			oplist[numinlist - 1].imode = USB;
			oplist[numinlist - 1].freq = 0;
			oplist[numinlist - 1].iBW = 0;
			memset(oplist[numinlist - 1].alpha_tag, 0, ATAGSIZE);
			numinlist--;
		}
		updateSelect();
	}
}

void addFreq() {
	if (selrig->inuse == onB) {
		unsigned long long freq = FreqDispB->value();
		if (!freq) return;
		int mode = opMODE->index();
		int bw;
		if (btnDSP->visible())
			bw = ((opDSP_hi->index() << 8) | 0x8000) | (opDSP_lo->index() & 0xFF) ;
		else if (opBW_B->visible())
			bw = opBW_B->index();
		else
			bw = opBW->index();
		for (int n = 0; n < numinlist; n++)
			if (freq == oplist[n].freq && mode == oplist[n].imode) {
				oplist[n].iBW = bw;
				updateSelect();	// update list
				return;
			}
		addtoList(freq, mode, bw);
		updateSelect();
		FreqDispB->visual_beep();
	} else {
		unsigned long long freq = FreqDispA->value();
		if (!freq) return;
		int mode = opMODE->index();
		int bw;
		if (btnDSP->visible())
			bw = ((opDSP_hi->index() << 8) | 0x8000) | (opDSP_lo->index() & 0xFF) ;
		else if (opBW_A->visible())
			bw = opBW_A->index();
		else
			bw = opBW->index();
		for (int n = 0; n < numinlist; n++)
			if (freq == oplist[n].freq && mode == oplist[n].imode) {
				oplist[n].iBW = bw;
				updateSelect();	// update list
				return;
			}
		addtoList(freq, mode, bw);
		updateSelect();
		FreqDispA->visual_beep();
		}
}

void cbRIT()
{
	trace(1, "cbRIT()");
	if (selrig->has_rit  && cntRIT) {
		guard_lock serial_lock( &mutex_serial, std::string(__func__) );
		selrig->setRit((int)cntRIT->value());
	}
}

void cbXIT()
{
	trace(1, "cbXIT()");
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	selrig->setXit((int)cntXIT->value());
}

void cbBFO()
{
	if (selrig->has_bfo) {
		trace(1, "cbBFO()");
		guard_lock serial_lock( &mutex_serial, std::string(__func__) );
		selrig->setBfo((int)cntBFO->value());
	}
}

void cbAttenuator()
{
	trace(1, "cbAttenuator()");

	{
		guard_lock serial_lock( &mutex_serial, std::string(__func__) );
		selrig->set_attenuator ( progStatus.attenuator = selrig->next_attenuator() );

		if (xcvr_name == rig_K4.name_)
			selrig->get_attenuator();
	}
	btnAttenuator->value( progStatus.attenuator > 0 ? 1 : 0);
	btnAttenuator->label( selrig->att_label() );
	btnAttenuator->redraw_label();
	btnAttenuator->redraw();

	return;
}

void setAttControl(void *d)
{
	size_t val = reinterpret_cast<size_t>(d);
	btnAttenuator->value(val);
}

void cbPreamp()
{
	trace(1, "cbPreamp()");

	if (selrig->name_ == rig_FLEX1500.name_) {
		selrig->set_preamp(progStatus.preamp = cbo_preamp->index());
		return;
	}

	{
		guard_lock serial_lock( &mutex_serial, std::string(__func__) );
		selrig->set_preamp ( progStatus.preamp = selrig->next_preamp() );
		if (xcvr_name == rig_K4.name_)
			selrig->get_preamp();
	}

	btnPreamp->value( progStatus.preamp > 0 ? 1 : 0);
	btnPreamp->label( selrig->pre_label() );
	btnPreamp->redraw_label();
	btnPreamp->redraw();

	return;
}

void setPreampControl(void *d)
{
	size_t val = reinterpret_cast<size_t>(d);
	btnPreamp->value(val);
}

void cbAN()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cbAN()");
	progStatus.auto_notch = btnAutoNotch->value();
	selrig->set_auto_notch(progStatus.auto_notch);
}

void cbbtnNotch()
{
	if (!selrig->has_notch_control) return;

	int val = 0, cnt = 0;
	{
		guard_lock serial_lock( &mutex_serial, std::string(__func__) );
		progStatus.notch = btnNotch->value();
		selrig->set_notch(progStatus.notch, progStatus.notch_val);
	}

	MilliSleep(progStatus.serial_post_write_delay);

	while ((selrig->get_notch(val) != progStatus.notch) && (cnt++ < 10)) {
		MilliSleep(progStatus.serial_post_write_delay);
		Fl::awake();
	}
}

void setNotch()
{
	if (!selrig->has_notch_control) return;

	trace(1, "setNotch()");

	int ev = Fl::event();
	if (ev == FL_LEAVE || ev == FL_ENTER) return;

	guard_lock lock( &mutex_serial, std::string(__func__) );

	if (sldrNOTCH) {
		progStatus.notch_val = sldrNOTCH->value();
	} else {
		progStatus.notch_val = spnrNOTCH->value();
	}

	selrig->set_notch(progStatus.notch, progStatus.notch_val);
}

// called from xml_io thread
// xcvr updated in xml_io / xml_server code
// this only updates the dialog controls

void setNotchControl(void *d)
{
	if (sldrNOTCH) sldrNOTCH->value(progStatus.notch_val);
	if (spnrNOTCH) spnrNOTCH->value(progStatus.notch_val);
	btnNotch->value(progStatus.notch);
}

void adjust_if_shift_control(void *d)
{
	if (sldrIFSHIFT) sldrIFSHIFT->minimum(selrig->if_shift_min);
	if (sldrIFSHIFT) sldrIFSHIFT->maximum(selrig->if_shift_max);
	if (sldrIFSHIFT) sldrIFSHIFT->step(selrig->if_shift_step);
	if (sldrIFSHIFT) sldrIFSHIFT->value(selrig->if_shift_mid);
	if (sldrIFSHIFT) sldrIFSHIFT->redraw();
	if (spnrIFSHIFT) spnrIFSHIFT->minimum(selrig->if_shift_min);
	if (spnrIFSHIFT) spnrIFSHIFT->maximum(selrig->if_shift_max);
	if (spnrIFSHIFT) spnrIFSHIFT->step(selrig->if_shift_step);
	if (spnrIFSHIFT) spnrIFSHIFT->value(selrig->if_shift_mid);
	if (spnrIFSHIFT) spnrIFSHIFT->redraw();
	btnIFsh->value(0);
	btnIFsh->redraw();
}

void setIFshiftButton(void *d)
{
	bool b = (bool)d;
	if (b && !btnIFsh->value()) {
		btnIFsh->value(1);
	}
	else if (!b && btnIFsh->value()) {
		btnIFsh->value(0);
		if (sldrIFSHIFT) sldrIFSHIFT->value( selrig->if_shift_mid );
		if (spnrIFSHIFT) spnrIFSHIFT->value( selrig->if_shift_mid );
	}
}

void setIFshiftControl(void *d)
{
	int val = *(reinterpret_cast<int *>(d));
	if (sldrIFSHIFT) {
		if (sldrIFSHIFT->value() != val)
			sldrIFSHIFT->value(val);
	}
	if (spnrIFSHIFT) {
		if (spnrIFSHIFT->value() != val)
			spnrIFSHIFT->value(val);
	}
	btnIFsh->value( val != selrig->if_shift_mid );
}

void setIFshift()
{
	trace(1, "setIFshift()");

	int ev = Fl::event();
	if (ev == FL_LEAVE || ev == FL_ENTER) return;
	if (ev == FL_DRAG || ev == FL_PUSH) {
		inhibit_shift = 1;
		return;
	}

	int btn = 0, set = 0;

	btn = btnIFsh->value();
	progStatus.shift = btn;

	if (sldrIFSHIFT) {
		set = sldrIFSHIFT->value();
	} else if (spnrIFSHIFT) {
		set = spnrIFSHIFT->value();
	}
	progStatus.shift_val = set;


	guard_lock lock( &mutex_serial, std::string(__func__) );
	if (xcvr_name == rig_TS990.name_) {
		if (progStatus.shift)
			selrig->set_monitor(1);
		else
			selrig->set_monitor(0);
	}
	selrig->set_if_shift(progStatus.shift_val);
}

void cbIFsh()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "setIFsh()");

	int btn, set, cnt = 0;
	if (sldrIFSHIFT) {
		set = sldrIFSHIFT->value();
		btn = btnIFsh->value();
	} else {
		set = spnrIFSHIFT->value();
		btn = btnIFsh->value();
	}
	if (btn == 0) set = 0;

	selrig->set_if_shift(set);
	MilliSleep(50);
	int val, on;
	on = selrig->get_if_shift(val);
	while ((on != btn) && (cnt++ < 10)) {
		MilliSleep(progStatus.serial_post_write_delay);
		on = selrig->get_if_shift(val);
		Fl::awake();
	}
}

void setLOCK()
{
	progStatus.pbt_lock = btnLOCK->value();
	if (progStatus.pbt_lock) {
		guard_lock serial_lock( &mutex_serial, std::string(__func__) );
		progStatus.pbt_outer = progStatus.pbt_inner;
		sldrOUTER->value(progStatus.pbt_outer);
		selrig->set_pbt_outer(progStatus.pbt_outer);
		sldrOUTER->redraw();
	}
}

void setINNER()
{
	if (selrig->name_ == rig_FLEX1500.name_) {
		progStatus.pbt_inner = sldrINNER->value();
		selrig->set_pbt( progStatus.pbt_inner, progStatus.pbt_outer);
		return;
	}

	progStatus.pbt_inner = sldrINNER->value();
	if (progStatus.pbt_inner == 0) {
		sldrINNER->value(1); sldrINNER->redraw();
		sldrINNER->value(0); sldrINNER->redraw();
	}
	if (progStatus.pbt_lock) {
		progStatus.pbt_outer = progStatus.pbt_inner;
		if (progStatus.pbt_outer == 0) {
			sldrOUTER->value(1);
			sldrOUTER->redraw();
		}
		sldrOUTER->value(progStatus.pbt_outer);
		sldrOUTER->redraw();
	}

	int ev = Fl::event();
	if (ev == FL_LEAVE || ev == FL_ENTER) return;
	if (ev == FL_DRAG || ev == FL_PUSH) {
		inhibit_pbt = 1;
		return;
	}

	guard_lock lock( &mutex_serial, std::string(__func__) );
	selrig->set_pbt_inner(progStatus.pbt_inner);
	selrig->get_pbt_inner();
	if (progStatus.pbt_lock) {
		selrig->set_pbt_outer(progStatus.pbt_outer);
		selrig->get_pbt_outer();
	}
}

void setOUTER()
{
	if (selrig->name_ == rig_FLEX1500.name_) {
		progStatus.pbt_outer = sldrOUTER->value();
		selrig->set_pbt( progStatus.pbt_inner, progStatus.pbt_outer);
		return;
	}

	progStatus.pbt_outer = sldrOUTER->value();
	if (progStatus.pbt_outer == 0) {
		sldrOUTER->value(1); sldrOUTER->redraw();
		sldrOUTER->value(0); sldrOUTER->redraw();
	}
	if (progStatus.pbt_lock) {
		progStatus.pbt_inner = progStatus.pbt_outer;
		if (progStatus.pbt_inner == 0) {
			sldrINNER->value(1);
			sldrINNER->redraw();
		}
		sldrINNER->value(progStatus.pbt_inner);
		sldrINNER->redraw();
	}

	int ev = Fl::event();
	if (ev == FL_LEAVE || ev == FL_ENTER) return;
	if (ev == FL_DRAG || ev == FL_PUSH) {
		inhibit_pbt = 1;
		return;
	}

	guard_lock lock( &mutex_serial, std::string(__func__) );
	selrig->set_pbt_outer(progStatus.pbt_outer);
	selrig->get_pbt_outer();
	if (progStatus.pbt_lock) {
		selrig->set_pbt_inner(progStatus.pbt_inner);
		selrig->get_pbt_inner();
	}
}

void setCLRPBT()
{
	if (selrig->name_ == rig_FLEX1500.name_)
		return;

	progStatus.pbt_inner = progStatus.pbt_outer = 0;

	sldrOUTER->value(0);
	sldrOUTER->redraw();

	sldrINNER->value(0);
	sldrINNER->redraw();

	guard_lock lock( &mutex_serial, std::string(__func__) );
	selrig->set_pbt_outer(progStatus.pbt_outer);
	selrig->set_pbt_inner(progStatus.pbt_inner);
}

//----------------------------------------------------------------------
// these only apply to the IC7610
//----------------------------------------------------------------------

void digi_sel_on_off()
{
	selrig->set_digi_sel(progStatus.digi_sel_on_off);
}

void set_ic7610_digi_sel_on_off(void *)
{
	ic7610digi_sel_on_off->value(progStatus.digi_sel_on_off);
	ic7610digi_sel_on_off->redraw();
}

void digi_sel_val()
{
	selrig->set_digi_val(progStatus.digi_sel_val);
}

void set_ic7610_digi_sel_val(void *)
{
	ic7610_digi_sel_val->value(progStatus.digi_sel_val);
	ic7610_digi_sel_val->redraw();
}

void dual_watch()
{
	selrig->set_dual_watch(progStatus.dual_watch);
}

void set_ic7610_dual_watch(void *)
{
	ic7610dual_watch->value(progStatus.dual_watch);
	ic7610dual_watch->redraw();
}

void index_att()
{
	selrig->set_index_att(progStatus.index_ic7610att);
}

//void set_ic7610_index_att(void *)
//{
//	ic7610att->index(progStatus.index_ic7610att);
//}


//----------------------------------------------------------------------

void cbEventLog()
{
	debug::show();
}

void setVolume() // UI call
{
	trace(1, "setVolume()");
	int ev = Fl::event();
	if (ev == FL_LEAVE || ev == FL_ENTER) return;
	int set;
	if (spnrVOLUME) set = spnrVOLUME->value();
	else set = sldrVOLUME->value();
	if (xcvr_name == rig_K4.name_) {
		if (selrig->isOnA()) progStatus.volume_A = set;
		else progStatus.volume_B = set;
	} else
		progStatus.volume = set;

	if (btnVol->value() == 0) return;

	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	selrig->set_volume_control(set);
}

void setVolumeControl(void* d) // called by xml_server
{
	trace(1, "setVolumeControl()");
	if (sldrVOLUME) sldrVOLUME->value(progStatus.volume);
	if (spnrVOLUME) spnrVOLUME->value(progStatus.volume);
}

void cbMute()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cbMute()");

	int set = 0, get, cnt = 0;
	if (btnVol->value() == 0) {
		if (spnrVOLUME) spnrVOLUME->deactivate();
		if (sldrVOLUME) sldrVOLUME->deactivate();
	} else {
		if (spnrVOLUME) {
			spnrVOLUME->activate();
			set = spnrVOLUME->value();
		}
		if (sldrVOLUME) {
			sldrVOLUME->activate();
			set = sldrVOLUME->value();
		}
	}
	selrig->set_volume_control(set);
	MilliSleep(50);
	get = selrig->get_volume_control();
	while (get != set && cnt++ < 10) {
		MilliSleep(progStatus.serial_post_write_delay);
		get = selrig->get_volume_control();
		Fl::awake();
	}
	progStatus.volume = set;
}

void setMicGain()
{
	int ev = Fl::event();
	if (ev == FL_LEAVE || ev == FL_ENTER) return;
	if (ev == FL_DRAG || ev == FL_PUSH) {
		inhibit_mic = 1;
		return;
	}
	std::stringstream str;
	str << "setMicGain(), ev=" << ev << ", inhibit_mic=" << inhibit_mic;
	trace(1, str.str().c_str());


	int set = 0;

	if (sldrMICGAIN) set = sldrMICGAIN->value();
	if (spnrMICGAIN) set = spnrMICGAIN->value();

	progStatus.mic_gain = set;

	guard_lock lock( &mutex_serial, std::string(__func__) );
	selrig->set_mic_gain(set);
}

void setMicGainControl(void* d)
{
	trace(1, "setMicGainControl()");
	if (sldrMICGAIN) sldrMICGAIN->value(progStatus.mic_gain);
	if (spnrMICGAIN) spnrMICGAIN->value(progStatus.mic_gain);
}

static int img = -1;

void set_power_controlImage(double pwr)
{
	if ((progStatus.pwr_scale == 9 && pwr <= 5.0) || (progStatus.pwr_scale == 0)) {
		if (img != 1) {
			img = 1;
			scalePower->image(image_p5);
			sldrFwdPwr->maximum(5.0);
			sldrFwdPwr->minimum(0.0);
			scalePower->redraw();

			mtr_PWR->image(image_p5);
			mtr_PWR->redraw();
			sigbar_PWR->maximum(5.0);
			sigbar_PWR->minimum(0.0);
			sigbar_PWR->redraw();
		}
	}
	else if ((progStatus.pwr_scale == 9 && pwr <= 10.0) || (progStatus.pwr_scale == 1)) {
		if (img != 2) {
			img = 2;
			scalePower->image(image_p10);
			sldrFwdPwr->maximum(10.0);
			sldrFwdPwr->minimum(0.0);
			scalePower->redraw();

			mtr_PWR->image(image_p10);
			mtr_PWR->redraw();
			sigbar_PWR->maximum(10.0);
			sigbar_PWR->minimum(0.0);
			sigbar_PWR->redraw();
		}
	}
	else if ((progStatus.pwr_scale == 9 && pwr <= 15.0) || (progStatus.pwr_scale == 2)) {
		if (img != 3) {
			img = 3;
			scalePower->image(image_p15);
			sldrFwdPwr->maximum(16.666);
			sldrFwdPwr->minimum(0.0);
			scalePower->redraw();

			mtr_PWR->image(image_p15);
			mtr_PWR->redraw();
			sigbar_PWR->maximum(16.666);
			sigbar_PWR->minimum(0.0);
			sigbar_PWR->redraw();
		}
	}
	else if ((progStatus.pwr_scale == 9 && pwr <= 20.0) || (progStatus.pwr_scale == 3)) {
		if (img != 4) {
			img = 4;
			scalePower->image(image_p20);
			sldrFwdPwr->maximum(20.0);
			sldrFwdPwr->minimum(0.0);
			scalePower->redraw();

			mtr_PWR->image(image_p20);
			mtr_PWR->redraw();
			sigbar_PWR->maximum(20.0);
			sigbar_PWR->minimum(0.0);
			sigbar_PWR->redraw();
		}
	}
	else if ((progStatus.pwr_scale == 9 && pwr <= 25.0) || (progStatus.pwr_scale == 4)) {
		if (img != 5) {
			img = 5;
			scalePower->image(image_p25);
			sldrFwdPwr->maximum(25.0);
			sldrFwdPwr->minimum(0.0);
			scalePower->redraw();

			mtr_PWR->image(image_p25);
			mtr_PWR->redraw();
			sigbar_PWR->maximum(25.0);
			sigbar_PWR->minimum(0.0);
			sigbar_PWR->redraw();
		}
	}
	else if ((progStatus.pwr_scale == 9 && pwr <= 50.0) || (progStatus.pwr_scale == 5)) {
		if (img != 6) {
			img = 6;
			scalePower->image(image_p50);
			sldrFwdPwr->maximum(50.0);
			sldrFwdPwr->minimum(0.0);
			scalePower->redraw();

			mtr_PWR->image(image_p50);
			mtr_PWR->redraw();
			sigbar_PWR->maximum(50.0);
			sigbar_PWR->minimum(0.0);
			sigbar_PWR->redraw();
		}
	}
	else if ((progStatus.pwr_scale == 9 && pwr <= 100.0) || (progStatus.pwr_scale == 6)) {
		if (img != 6) {
			img = 6;
			scalePower->image(image_p100);
			sldrFwdPwr->maximum(100.0);
			sldrFwdPwr->minimum(0.0);
			scalePower->redraw();

			mtr_PWR->image(image_p100);
			mtr_PWR->redraw();
			sigbar_PWR->maximum(100.0);
			sigbar_PWR->minimum(0.0);
			sigbar_PWR->redraw();
		}
	}
	else if ((progStatus.pwr_scale == 9 && pwr > 100.0) || (progStatus.pwr_scale == 7)) {
		if (img != 8) {
			img = 8;
			scalePower->image(image_p200);
			sldrFwdPwr->maximum(200.0);
			sldrFwdPwr->minimum(0.0);
			scalePower->redraw();

			mtr_PWR->image(image_p200);
			mtr_PWR->redraw();
			sigbar_PWR->maximum(200.0);
			sigbar_PWR->minimum(0.0);
			sigbar_PWR->redraw();
		}
	}
	else if ((progStatus.pwr_scale == 9 && pwr > 100.0) || (progStatus.pwr_scale == 8)) {
		if (img != 9) {
			img = 9;
			scalePower->image(image_p200log);
			sldrFwdPwr->maximum(200.0);
			sldrFwdPwr->minimum(0.0);
			scalePower->redraw();

			mtr_PWR->image(image_p200log);
			mtr_PWR->redraw();
			sigbar_PWR->maximum(200.0);
			sigbar_PWR->minimum(0.0);
			sigbar_PWR->redraw();
		}
	}
	return;
}

void set_init_power_control();

void execute_setPower()
{
	double set = 0;

	if (spnrPOWER) set = progStatus.power_level = spnrPOWER->value();
	if (sldrPOWER) set = progStatus.power_level = sldrPOWER->value();

	double min, max, step;
	selrig->get_pc_min_max_step(min, max, step);

	if (xcvr_name == rig_K2.name_) {
		if (spnrPOWER) spnrPOWER->minimum(min);
		if (spnrPOWER) spnrPOWER->maximum(max);
		if (spnrPOWER) spnrPOWER->step(step);
		if (spnrPOWER) spnrPOWER->value(progStatus.power_level);
		if (spnrPOWER) spnrPOWER->redraw();
		if (sldrPOWER) sldrPOWER->minimum(min);
		if (sldrPOWER) sldrPOWER->maximum(max);
		if (sldrPOWER) sldrPOWER->step(step);
		if (sldrPOWER) sldrPOWER->value(progStatus.power_level);
		if (sldrPOWER) sldrPOWER->redraw();
	}

	if (progStatus.enable_power_limit && (set > progStatus.power_limit * max / 100)) {
		set = progStatus.power_limit * max / 100;
		if (spnrPOWER) spnrPOWER->value(set);
		if (sldrPOWER) sldrPOWER->value(set);
	}

	guard_lock lock( &mutex_serial, std::string(__func__) );
	selrig->set_power_control(set);
	set_init_power_control();
}

void setPower()
{
	int ev = Fl::event();
	if (ev == FL_LEAVE || ev == FL_ENTER) return;
	if (ev == FL_DRAG || ev == FL_PUSH) {
		inhibit_power = 1;
		return;
	}
	std::stringstream str;
	str << "setPower(), ev=" << ev << ", inhibit_power=" << inhibit_power;
	trace(1, str.str().c_str());
	execute_setPower();
}

void cbTune()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );

	if (progStatus.enable_AM_tune) {
// find AM mode in modes_ vector
		int mAM = -1;
		rigmodes_ = selrig->modes_;
		for (size_t n = 0; n < rigmodes_.size(); n++) {
			if (rigmodes_[n].find("AM") != std::string::npos) {
				mAM = n;
				break;
			}
		}
		if (mAM == -1) return;

		double pwr_level = selrig->get_power_control();
		int md = vfo->imode;
		if (selrig->inuse == onA)
			selrig->set_modeA(mAM);
		else
			selrig->set_modeB(mAM);

		selrig->set_power_control( progStatus.tune_percent_power * sldrPOWER->maximum() / 100.0 );

		selrig->set_PTT_control(1);
		for (int n = 0; n < progStatus.tune_seconds * 20; n++) {
			MilliSleep(50);
			Fl::awake();
		}
		selrig->set_PTT_control(0);

		selrig->set_power_control( pwr_level );

		if (selrig->inuse == onA) {
			selrig->set_modeA(md);
		} else {
			selrig->set_modeB(md);
		}
	} else  {
		trace(1, "cbTune()");
		selrig->tune_rig(2);
	}
}

void cb_tune_on_off()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_tune_on_off()");
	selrig->tune_rig(btn_tune_on_off->value());
}

int chkptt()
{
	if (progStatus.serial_catptt) {
		return selrig->get_PTT();
	} else if (progStatus.serial_dtrptt) {
		return RigSerial->getPTT();
	} else if (progStatus.serial_rtsptt) {
		return RigSerial->getPTT();
	} else if (SepSerial->IsOpen() && progStatus.sep_dtrptt) {
		return SepSerial->getPTT();
	} else if (SepSerial->IsOpen() && progStatus.sep_rtsptt) {
		return SepSerial->getPTT();
	} else if (progStatus.gpio_ptt) {
		return get_gpio();
	} else if (progStatus.cmedia_ptt) {
		return get_cmedia();
	}
	return 0;
}

void doPTT(int on)
{
	guard_lock serlck( &mutex_serial, std::string(__func__) );

	PTT = on;
	rigPTT(on);
	btnPTT->value(on);
	MilliSleep(progStatus.serial_post_write_delay);
	for (int n = 0; n < 100; n++) {
		if (on == chkptt()) break;
		MilliSleep(progStatus.serial_post_write_delay);
		Fl::awake();
	}
	return;
}

void setSQUELCH()
{
	trace(1, "setSQUELCH()");

	int ev = Fl::event();
	if (ev == FL_LEAVE || ev == FL_ENTER) return;
	if (ev == FL_DRAG || ev == FL_PUSH) {
		inhibit_squelch = 1;
		return;
	}
	int set = 0;
	if (sldrSQUELCH) set = sldrSQUELCH->value();
	if (spnrSQUELCH) set = spnrSQUELCH->value();

	progStatus.squelch = set;
	guard_lock lock( &mutex_serial, std::string(__func__) );
	selrig->set_squelch(set);
}

int agcwas = 0;
void redrawAGC()
{
	const char *lbl = selrig->agc_label();
	int val = progStatus.agc_level;

	if (!selrig->has_rf_control) {
		sldrSQUELCH->label("");
		sldrSQUELCH->redraw_label();
	}
	btnAGC->label(lbl);
	btnAGC->redraw_label();

	if (xcvr_name == rig_FT450D.name_) {
		switch (val) {
			case 0 : btnAGC->selection_color(FL_BACKGROUND_COLOR);  // off
					 break;
			case 1 : btnAGC->selection_color(FL_RED); // fast
					 break;
			case 2 : btnAGC->selection_color(FL_YELLOW); // slow
					 break;
			case 3 : btnAGC->selection_color(FL_GREEN); // auto
					 break;
		}
		btnAGC->redraw();
	}

	if (xcvr_name == rig_FT991A.name_) {
		switch (val) {
			case 0 : btnAGC->selection_color(FL_BACKGROUND_COLOR);  // off
					 break;
			case 1 : btnAGC->selection_color(FL_RED); // fast
					 break;
			case 2 : btnAGC->selection_color(FL_YELLOW); // medium
					 break;
			case 3 : btnAGC->selection_color(FL_GREEN); // slow
					 break;
			case 4 : btnAGC->selection_color(FL_WHITE); // auto
		}
		btnAGC->redraw();
	}

	int rignbr = 0;
	if (xcvr_name == rig_IC7200.name_) rignbr = 1;
	if (xcvr_name == rig_IC7300.name_) rignbr = 2;
	if (rignbr) {
		if (val == 0) btnAGC->selection_color(FL_BACKGROUND_COLOR);  // off
		if (val == 1) btnAGC->selection_color(
			rignbr == 1 ? FL_GREEN : FL_RED); // fast
		if (val == 2) btnAGC->selection_color(
			rignbr == 1 ? FL_RED : FL_YELLOW);  // med / slow
		if (val == 3) btnAGC->selection_color(FL_GREEN); // slow
		btnAGC->value(val > 0);
	} else {
		if (val == 0)
			btnAGC->value(0);
		else
			btnAGC->value(1);
	}

	if (agcwas != val) {
		agcwas = val;
	}

	btnAGC->redraw();
}

void setAGC(void *)
{
	if (!selrig->has_agc_control) return;
	redrawAGC();
}

void cbAGC()
{
	if (!selrig->has_agc_control) return;
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cbAGC()");
	progStatus.agc_level = selrig->incr_agc();
	redrawAGC();
}

void setRFGAIN()
{
	trace(1, "setRFGAIN()");

	int ev = Fl::event();
	if (ev == FL_LEAVE || ev == FL_ENTER) return;
	if (ev == FL_DRAG || ev == FL_PUSH) {
		inhibit_rfgain = 1;
		return;
	}

	int set = 0;

	if (spnrRFGAIN) set = spnrRFGAIN->value();
	if (sldrRFGAIN) set = sldrRFGAIN->value();

	progStatus.rfgain = set;

	guard_lock lock( &mutex_serial, std::string(__func__) );
	selrig->set_rf_gain(set);
}

void setRFGAINControl(void* d)
{
	trace(1, "setRFGAINControl()");
	if (sldrRFGAIN) sldrRFGAIN->value(progStatus.rfgain);
	if (spnrRFGAIN) spnrRFGAIN->value(progStatus.rfgain);
}

void updateIDD(void *)
{
	if (iddval <= 5.0) {
		mtr_IDD->image(image_idd5);
		sigbar_IDD->maximum(5);
	} else if (iddval <= 25.0) {
		mtr_IDD->image(image_idd25);
		sigbar_IDD->maximum(25);
	} else {
		mtr_IDD->image(image_idd50);
		sigbar_IDD->maximum(50);
	}
	mtr_IDD->redraw();
	sigbar_IDD->value(iddval);
	sigbar_IDD->redraw();

	if (meter_image == IDD_IMAGE) {
		sldrRcvSignal->hide();
		sldrSWR->hide();
		sldrALC->hide();
		btnALC_IDD_SWR->redraw_label();
		btnALC_IDD_SWR->redraw();
		sldrIDD->redraw();
		sldrIDD->show();
		if (iddval <= 5.0) {
			btnALC_IDD_SWR->image(image_idd5);
			sldrIDD->value(iddval);
			sldrIDD->maximum(5);
		} else if (iddval <= 25.0) {
			btnALC_IDD_SWR->image(image_idd25);
			sldrIDD->value(iddval);
			sldrIDD->maximum(25);
		} else {
			btnALC_IDD_SWR->image(image_idd50);
			sldrIDD->value(iddval);
			sldrIDD->maximum(50);
		}
		sigbar_IDD->value(iddval);
		sigbar_IDD->redraw();
	}
//	 else if (meter_image == SWR_IMAGE) {
//		sldrRcvSignal->hide();
//		sldrALC->hide();
//		sldrIDD->hide();
//		sldrSWR->show();
//	} else {
//		sldrRcvSignal->hide();
//		sldrSWR->hide();
//		sldrIDD->hide();
//		sldrALC->show();
//	}
}

void updateALC(void *)
{
	sigbar_ALC->value(alcval);
	sigbar_ALC->redraw();

	if (meter_image != ALC_IMAGE) return;
	sldrRcvSignal->hide();
	sldrSWR->hide();
	sldrIDD->hide();
	sldrALC->show();
	sldrALC->value(alcval);
	sldrALC->redraw();
}

void updateSWR(void *)
{
	sigbar_SWR->value(swrval);
	sigbar_SWR->redraw();

	if (meter_image != SWR_IMAGE) return;
	if (selrig->has_swr_control) {
		sldrRcvSignal->hide();
		sldrALC->hide();
		sldrIDD->hide();
		sldrSWR->show();
	}
	sldrSWR->value(swrval);
	sldrSWR->redraw();
}

void updateFwdPwr(void *)
{
	double power = pwrval;

	power /= selrig->power_scale();

	if (selrig->has_power_control)
		set_power_controlImage(power);

	if (selrig->has_power_out) {
		sldrVoltage->hide();
		scaleVoltage->hide();
		sldrFwdPwr->show();
		scalePower->show();

		if  ((progStatus.pwr_scale == 9 && pwrval > 100.0) || (progStatus.pwr_scale == 8)) {

			power = 200 * sqrt( pwrval / 200.0 );

		}

		sigbar_PWR->value(power);
		sldrFwdPwr->value(power);

	}

}

void updateSquelch(void *d)
{
	if (sldrSQUELCH) sldrSQUELCH->value(reinterpret_cast<size_t>(d));
	if (sldrSQUELCH) sldrSQUELCH->redraw();
	if (spnrSQUELCH) spnrSQUELCH->value(reinterpret_cast<size_t>(d));
	if (spnrSQUELCH) spnrSQUELCH->redraw();
}

void updateRFgain(void *d)
{
	if (spnrRFGAIN) {
		spnrRFGAIN->value(reinterpret_cast<size_t>(d));
		spnrRFGAIN->redraw();
	}
	if (sldrRFGAIN) {
		sldrRFGAIN->value(reinterpret_cast<size_t>(d));
		sldrRFGAIN->redraw();
	}
}

void zeroXmtMeters(void *d)
{
	pwrval = 0; updateFwdPwr();
	alcval = 0; updateALC();
	swrval = 0; updateSWR();
	iddval = 0; updateIDD();
	sldrFwdPwr->clear();
	sldrALC->clear();
	sldrSWR->clear();
	sigbar_IDD->value(0);
	sigbar_IDD->redraw();
	sldrIDD->value(0);
	sldrIDD->redraw();
}

void setFreqDispA(void *d)
{
	FreqDispA->value(vfoA.freq);
	FreqDispA->redraw();
}

void setFreqDispB(void *d)
{
	FreqDispB->value(vfoB.freq);
	FreqDispB->redraw();
}

void updateSmeter(void *)
{
	if (!sldrRcvSignal->visible()) {
		sldrRcvSignal->show();
		sldrFwdPwr->hide();
		sldrALC->hide();
		sldrSWR->hide();
	}
	sldrRcvSignal->value(smtrval);
	sldrRcvSignal->redraw();
	sigbar_SMETER->value(smtrval);
	sigbar_SMETER->redraw();
}

void updateVmeter(void *)
{
	if (!progStatus.display_voltmeter || !selrig->has_voltmeter) {
		sldrVoltage->hide();
		scaleVoltage->hide();
		if (selrig->has_power_out) {
			sldrFwdPwr->show();
			scalePower->show();
		}
	} else {
		if (PTT) {
			if (selrig->has_power_out) {
				sldrVoltage->hide();
				scaleVoltage->hide();
				sldrFwdPwr->show();
				scalePower->show();
			}
		} else {
			if (selrig->has_power_out) {
				sldrFwdPwr->hide();
				scalePower->hide();
				sldrVoltage->show();
				scaleVoltage->show();
			}
		}
	}
	if (vmtrval == -1) return;

	sldrVoltage->value(vmtrval);
	sldrVoltage->redraw();

	sigbar_VOLTS->value(vmtrval);
	sigbar_VOLTS->redraw();
}

void TRACED(saveFreqList)

	std::string atag;

	if (!numinlist) return;

	backup_files(defFileName);

	std::ofstream oList(defFileName.c_str());
	if (!oList) {
		fl_message ("Could not write to %s", defFileName.c_str());
		return;
	}
	for (int i = 0; i < numinlist; i++) {
		atag = oplist[i].alpha_tag;
		oList << oplist[i].freq << " " << oplist[i].imode << " " << oplist[i].iBW << " \"" << atag.c_str() << "\"" << std::endl;

	}
	oList.close();
}

void TRACED(setPTT, void *d)

	size_t set = reinterpret_cast<size_t>(d);
	size_t chk = 0;

	guard_lock serlck( &mutex_serial, std::string(__func__) );
	chk = chkptt();
	rigPTT(set);
	MilliSleep(progStatus.serial_post_write_delay);
	for (int n = 0; n < 100; n++) {
		chk = chkptt();
		if (set == chk) break;
		MilliSleep(10);
	}
	return;
}

void update_progress(int val)
{
	progress->value(val);
	Fl::check();
}

void TRACED(send_st_ex_command, std::string command)

	std::string cmd = "";
	if (command.find("x") != std::string::npos) { // hex std::strings
		size_t p = 0;
		unsigned int val;
		while (( p = command.find("x", p)) != std::string::npos) {
			sscanf(&command[p+1], "%x", &val);
			cmd += (unsigned char) val;
			p += 3;
		}
	} else
		cmd = command;
	sendCommand(cmd, 0);
}

#include "timeops.h"
void synchronize( void *) {

	time_t now;
	time(&now);
	struct tm *tm_time;
	static char sztm[20];

	if (progStatus.sync_gmt) {
		tm_time = gmtime(&now);
		strftime(sztm, sizeof(sztm), "%H:%M:%S Z", tm_time);
	} else {
		tm_time = localtime(&now);
		strftime(sztm, sizeof(sztm), "%H:%M:%S", tm_time);
	}

	if (strncmp(&sztm[6], "00", 2) == 0) {
		guard_lock serial_lock( &mutex_serial, std::string(__func__) );
		static char szdate[20];
		strftime(szdate, sizeof(szdate), "%Y%m%d", tm_time);
		selrig->sync_clock(sztm);
		selrig->sync_date(szdate);
		txt_xcvr_synch->value("--SYNC'D--");
		return;
	}
	txt_xcvr_synch->value(sztm);
	Fl::repeat_timeout(0.05, synchronize);
}

void TRACED(synchronize_now)
	Fl::remove_timeout(synchronize);
	Fl::add_timeout(0, synchronize);
}

void TRACED(start_commands)

	if (!progStatus.cmd_on_start1.empty()) send_st_ex_command(progStatus.cmd_on_start1);
	if (!progStatus.cmd_on_start2.empty()) send_st_ex_command(progStatus.cmd_on_start2);
	if (!progStatus.cmd_on_start3.empty()) send_st_ex_command(progStatus.cmd_on_start3);
	if (!progStatus.cmd_on_start4.empty()) send_st_ex_command(progStatus.cmd_on_start4);
	if (progStatus.sync_clock && selrig->can_synch_clock)
		Fl::add_timeout(0, synchronize);
}

void TRACED(exit_commands)

	if (!progStatus.cmd_on_exit1.empty()) send_st_ex_command(progStatus.cmd_on_exit1);
	if (!progStatus.cmd_on_exit2.empty()) send_st_ex_command(progStatus.cmd_on_exit2);
	if (!progStatus.cmd_on_exit3.empty()) send_st_ex_command(progStatus.cmd_on_exit3);
	if (!progStatus.cmd_on_exit4.empty()) send_st_ex_command(progStatus.cmd_on_exit4);
}

void TRACED(close_UI)

	{
		guard_lock serial_lock( &mutex_serial, std::string(__func__) );
		trace(1, "close_UI()");
		run_serial_thread = false;
	}
	pthread_join(*serial_thread, NULL);

// xcvr auto off
	if (selrig->has_xcvr_auto_on_off && progStatus.xcvr_auto_off)
		selrig->set_xcvr_auto_off();

	// close down the serial port
	RigSerial->ClosePort();

	debug::stop();

	Fl_Double_Window *widgets[] = {
		dlgDisplayConfig,
		dlgXcvrConfig,
		dlgMemoryDialog,
		dlgColorsDialog,
		meters_dialog,
		tracewindow,
		cwio_keyer_dialog,
		cwio_editor,
		cwio_configure,
		cwlog_viewer,
		FSK_keyer_dialog,
		FSK_editor,
		FSK_configure,
		meter_filters,
		meter_scale_dialog,
		tabs_dialog,
		config_trace_dialog,
		mainwindow };
	for (size_t n = 0; n < sizeof(widgets) / sizeof(*widgets); n++) {
		if (widgets[n]) widgets[n]->hide();
	}
}

void TRACED(closeRig)

	trace(1, "closeRig()");
	if (selrig->io_class == TCI) {
		restore_xcvr_vals();
		selrig->shutdown();
	}
	else if (xcvr_online) {
		restore_xcvr_vals();
		selrig->shutdown();
	}
	xcvr_online = false;
}

void TRACED(cbExit)

	if (progStatus.show_tabs) show_controls();

	main_group->hide();
	main_group->redraw();

	grpInitializing->size(mainwindow->w(), mainwindow->h() - grpInitializing->y());
	grpInitializing->show();
	grpInitializing->redraw();

	progress->label("Closing");
	progress->redraw_label();

	progress->position(grpInitializing->w()/4, grpInitializing->y() + grpInitializing->h()/2);

	update_progress(0);

	progStatus.freq_A = vfoA.freq;
	progStatus.imode_A = vfoA.imode;
	progStatus.iBW_A = vfoA.iBW;

	progStatus.freq_B = vfoB.freq;
	progStatus.imode_B = vfoB.imode;
	progStatus.iBW_B = vfoB.iBW;

	if (selrig->has_FILTER)
		progStatus.filters = selrig->get_FILTERS();

	progStatus.spkr_on = btnVol->value();

	saveFreqList();

	cwlog_close();

	if (spnrPOWER) progStatus.power_level = spnrPOWER->value();
	if (spnrVOLUME) progStatus.volume = spnrVOLUME->value();
	if (spnrRFGAIN) progStatus.rfgain = spnrRFGAIN->value();
	if (spnrMICGAIN) progStatus.mic_gain = spnrMICGAIN->value();
	if (spnrNOTCH) progStatus.notch_val = spnrNOTCH->value();
	if (spnrIFSHIFT) progStatus.shift_val = spnrIFSHIFT->value();
	if (spnrNR) progStatus.noise_reduction_val = spnrNR->value();

	if (sldrPOWER) progStatus.power_level = sldrPOWER->value();
	if (sldrVOLUME) progStatus.volume = sldrVOLUME->value();
	if (sldrRFGAIN) progStatus.rfgain = sldrRFGAIN->value();
	if (sldrMICGAIN) progStatus.mic_gain = sldrMICGAIN->value();
	if (sldrNOTCH) progStatus.notch_val = sldrNOTCH->value();
	if (sldrIFSHIFT) progStatus.shift_val = sldrIFSHIFT->value();
	if (sldrNR) progStatus.noise_reduction_val = sldrNR->value();

	progStatus.notch = btnNotch->value();
	progStatus.shift = btnIFsh->value();
	progStatus.noise_reduction = btnNR->value();
	progStatus.noise = btnNOISE->value();
	progStatus.attenuator = btnAttenuator->value();
	progStatus.preamp = btnPreamp->value();
	progStatus.auto_notch = btnAutoNotch->value();

	progStatus.bandwidths = selrig->get_BANDWIDTHS();

	progStatus.saveLastState();
	closeRig();

	stop_cwio_thread();

	exit_server();

	close_UI();

}

void cbALC_IDD_SWR()
{
	switch (meter_image) {
		case ALC_IMAGE:
			if (selrig->has_idd_control) {
				btnALC_IDD_SWR->image(image_idd25);
				meter_image = IDD_IMAGE;
				sldrIDD->show();
				{
					guard_lock serial_lock( &mutex_serial, std::string(__func__) );
					trace(1, "cbALC_IDD_SWR()  2");
					selrig->select_idd();
				}
				break;
			}
		case IDD_IMAGE:
			if (selrig->has_swr_control) {
				btnALC_IDD_SWR->image(image_swr);
				meter_image = SWR_IMAGE;
				sldrSWR->show();
				{
					guard_lock serial_lock( &mutex_serial, std::string(__func__) );
					trace(1, "cbALC_IDD_SWR()  2");
					selrig->select_swr();
				}
				break;
			}
		case SWR_IMAGE:
		default:
			if (selrig->has_alc_control) {
                if (selrig->name_ == rig_PowerSDR.name_) {
					btnALC_IDD_SWR->image(image_alc40db);
                }
                else {
					btnALC_IDD_SWR->image(image_alc);
                }
				meter_image = ALC_IMAGE;
				sldrALC->show();
				{
					guard_lock serial_lock( &mutex_serial, std::string(__func__) );
					trace(1, "cbALC_IDD_SWR()  1");
					selrig->select_alc();
				}
			}
	}
	btnALC_IDD_SWR->redraw();
}

// trim leading and trailing whitspace and double quote
const std::string lt_trim(const std::string& pstring, const std::string& pWhitespace)
{
	size_t beginStr, endStr, range;
	beginStr = pstring.find_first_not_of(pWhitespace);
	if (beginStr == std::string::npos) return "";	// no content
	endStr = pstring.find_last_not_of(pWhitespace);
	range = endStr - beginStr + 1;

	return pstring.substr(beginStr, range);
}

void editAlphaTag()
{
	int indx;
	std::string atag;
	if (FreqSelect->value() < 1) {
		inAlphaTag->value("");
		return;	// no memory selected
	}
	indx = FreqSelect->value() - 1;
	atag = inAlphaTag->value();
// delete leading, trailing spaces
	atag = lt_trim(atag);
	memset(oplist[indx].alpha_tag, 0, ATAGSIZE);
	snprintf(oplist[indx].alpha_tag, ATAGSIZE, "%s", atag.c_str());
// update browser list
	updateSelect();
	FreqSelect->value(indx + 1);
	inAlphaTag->value(oplist[indx].alpha_tag);
}

//----------------------------------------------------------------------
// button label and label state changes
// Note that an additional level of service request is made to insure
// that the main thread is actually changing the widget

// noise reduction
static std::string nrlabel;
static bool nrstate;

void do_nr_label(void *)
{
	btnNR->value(nrstate ? 1 : 0);
	btnNR->label(nrlabel.c_str());
	btnNR->redraw_label();
	btnNR->redraw();
}

void noise_reduction_label(const char *l, int on)
{
	nrlabel = l;
	progStatus.noise_reduction = nrstate = on;
	Fl::awake(do_nr_label);
}

// noise blanker
static std::string nblabel;
static bool nbstate;

void do_nb_label(void *)
{
	btnNOISE->value(nbstate ? 1 : 0);
	btnNOISE->label(nblabel.c_str());
	btnNOISE->redraw_label();
	btnNOISE->redraw();
}

void noise_blanker_label(const char * l, int on)
{
	nblabel = l;
	progStatus.noise = nbstate = on;
	Fl::awake(do_nb_label);
}

// break-in label
static std::string bkin_label_;

void do_bkin_label(void *)
{
	btnBreakIn->label(bkin_label_.c_str());
	btnBreakIn->redraw_label();
	btnBreakIn->redraw();
}

void break_in_label(const char *l)
{
	bkin_label_ = l;

	Fl::awake(do_bkin_label);
}

// autonotch label
static std::string auto_notch_label_;
static bool auto_notch_state_;

void do_auto_notch_label(void *)
{
	btnAutoNotch->value(auto_notch_state_ ? 1 : 0);
	btnAutoNotch->label(auto_notch_label_.c_str());
	btnAutoNotch->redraw_label();
}

void auto_notch_label(const char * l, int on)
{
	auto_notch_label_ = l;
	auto_notch_state_ = on;
	progStatus.auto_notch = on;
	Fl::awake(do_auto_notch_label);
}

void cbAuxPort()
{
	AuxSerial->setRTS(progStatus.aux_rts);
	AuxSerial->setDTR(progStatus.aux_dtr);
}

void cb_agc_level()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_agc_level()");
	selrig->set_agc_level();
}

void cb_cw_wpm()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_cw_wpm()");
	selrig->set_cw_wpm();
}

void cb_cw_vol()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_cw_vol()");
	selrig->set_cw_vol();
}

void cb_cw_spot()
{
	int ret;
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_cw_spot()");
	ret = selrig->set_cw_spot();
	if (!ret) btnSpot->value(0);
}

void cb_cw_spot_tone()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_cw_spot_tone()");
	selrig->set_cw_spot_tone();
}

void CW_Zero()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "CW_Zero");
	selrig->zero_in();
}

void cb_vox_gain()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_vox_gain()");
	selrig->set_vox_gain();
}

void cb_vox_anti()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_vox_anti()");
	selrig->set_vox_anti();
}

void cb_vox_hang()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_vox_hang()");
	selrig->set_vox_hang();
}

void cb_vox_onoff()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_vox_onoff()");
	selrig->set_vox_onoff();
}

void cb_vox_on_dataport()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_dataport()");
	selrig->set_vox_on_dataport();
}

void cb_compression()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_compression");
	selrig->set_compression(progStatus.compON, progStatus.compression);
}

void cb_auto_notch()
{
	progStatus.auto_notch = btnAutoNotch->value();
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_autonotch()");
	selrig->set_auto_notch(progStatus.auto_notch);
}

void cb_vfo_adj()
{
	if (xcvr_name == rig_TT550.name_)
		progStatus.vfo_adj = spnr_tt550_vfo_adj->value();
	else
		progStatus.vfo_adj = spnr_vfo_adj->value();
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_vfo_adj()");
	selrig->setVfoAdj(progStatus.vfo_adj);
}

void cb_line_out()
{
}

void cb_bpf_center()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_bpf_center()");
	selrig->set_if_shift(selrig->pbt);
}

void cb_special()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_special()");
	selrig->set_special(btnSpecial->value());
}

void cbNoise()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cbNoise()");

	int btn, get, cnt = 0;

	btn = progStatus.noise = btnNOISE->value();

	selrig->set_noise(btn);

	MilliSleep(50);
	get = selrig->get_noise();
	while ((get != btn) && (cnt++ < 10)) {
		MilliSleep(progStatus.serial_post_write_delay);
		get = selrig->get_noise();
		Fl::awake();
	}

	vfo->noise = progStatus.noise;
	vfo->nb_level = progStatus.nb_level;
	update_noise( (void*)0 );
}

void cb_nb_level()
{
	if (!selrig->has_nb_level) return;
	int set = 0;

	trace(1, "cb_nb_level()");

	int ev = Fl::event();
	if (ev == FL_LEAVE || ev == FL_ENTER) return;
	if (ev == FL_DRAG || ev == FL_PUSH) {
		inhibit_nb_level = 1;
		return;
	}
	set = sldr_nb_level->value();
	guard_lock lock( &mutex_serial, std::string(__func__) );
	selrig->set_nb_level(set);
}

void cbNR()
{
	if (!selrig->has_noise_reduction_control) return;
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cbNR()");

	int btn = 0, set = 0, get, cnt = 0;
	btn = btnNR->value();
	set = sldrNR ? sldrNR->value() : spnrNR->value();

	if (xcvr_name == rig_TS2000.name_) {
		if (btn != -1) { // pia
			if (selrig->noise_reduction_level() == 0) {
				selrig->set_noise_reduction(1);
				selrig->set_noise_reduction_val(selrig->nrval1());
				progStatus.noise_reduction = 1;
				progStatus.noise_reduction_val = selrig->nrval1();
			} else if (selrig->currmode() != RIG_TS2000::FM &&
					  selrig->noise_reduction_level() == 1) {
				selrig->set_noise_reduction(2);
				selrig->set_noise_reduction_val(selrig->nrval2());
				progStatus.noise_reduction = 2;
				progStatus.noise_reduction_val = selrig->nrval2();
			} else
				selrig->set_noise_reduction(0);
		} else {
			progStatus.noise_reduction_val = set;
			selrig->set_noise_reduction_val(set);
		}
	} else { // not TS2000

		progStatus.noise_reduction = btn;

		selrig->set_noise_reduction(btn);
		MilliSleep(50);

		get = selrig->get_noise_reduction();

		while ((get != btn) && (cnt++ < 10)) {
			MilliSleep(progStatus.serial_post_write_delay);
			get = selrig->get_noise_reduction();
			Fl::awake();
		}
		progStatus.noise_reduction_val = set;
		selrig->set_noise_reduction_val(set);
		MilliSleep(50);
		get = selrig->get_noise_reduction_val();
		cnt = 0;
		while ((get != set) && (cnt++ < 10)) {
			MilliSleep(progStatus.serial_post_write_delay);
			get = selrig->get_noise_reduction_val();
			Fl::awake();
		}
	}

}

void setNR()
{
	if (!selrig->has_noise_reduction_control) return;
	trace(1, "setNR()");

	int btn = 0, set = 0;

	int ev = Fl::event();
	if (ev == FL_LEAVE || ev == FL_ENTER) return;
	if (ev == FL_DRAG || ev == FL_PUSH) {
		inhibit_nr = 1;
		return;
	}

	if (xcvr_name == rig_TS2000.name_ ||
		xcvr_name == rig_TS590S.name_ ||
		xcvr_name == rig_TS590SG.name_ ||
		xcvr_name == rig_TS890S.name_ ||
		xcvr_name == rig_TS990.name_) {
		if (sldrNR) {
			set = sldrNR->value();
			btn = -1;
		}
		if (spnrNR) {
			set = spnrNR->value();
			btn = -1;
		}
	} else {
		if (sldrNR) {
			set = sldrNR->value();
			btn = btnNR->value();
		}
		if (spnrNR) {
			set = spnrNR->value();
			btn = btnNR->value();
		}
	}

	guard_lock lock( &mutex_serial, std::string(__func__) );
	selrig->set_noise_reduction_val(set);
	selrig->set_noise_reduction(btn);

}

void cb_spot()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_spot()");
	selrig->set_cw_spot();
}

void cb_enable_keyer()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_enable_keyer()");
	selrig->enable_keyer();
}

void cb_set_break_in()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_set_break_in()");
	selrig->set_break_in();
}

void cb_cw_weight()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_cw_weight()");
	selrig->set_cw_weight();
}

void cb_cw_qsk()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_cw_qsk()");
	selrig->set_cw_qsk();
}

void cb_cw_delay()
{
	guard_lock serial_lock( &mutex_serial, std::string(__func__) );
	trace(1, "cb_cw_delay()");
	selrig->set_cw_delay();
}

void set_band_label(int band)
{
	switch (band) {
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
		case 5:
			break;
		case 6:
			break;
		case 7:
			break;
		case 8:
			break;
		case 9:
			break;
		case 10:
			break;
		case 11:
			break;
		case 12:
			break;
		case 13:
			break;
	}
}

void set_band(int band)
{
	switch (band) {
		case 1: progStatus.f160 = vfo->freq;
				progStatus.m160 = vfo->imode;
				progStatus.txT_160 = choice_FT8n_tTONE->value();
				progStatus.rxT_160 = choice_FT8n_rTONE->value();
				progStatus.offset_160 = FMoffset->index();
				progStatus.oF_160 = FMoff_freq->value();
				break; // 160 meters
		case 2: progStatus.f80 = vfo->freq;
				progStatus.m80 = vfo->imode;
				progStatus.txT_80 = choice_FT8n_tTONE->value();
				progStatus.rxT_80 = choice_FT8n_rTONE->value();
				progStatus.offset_80 = FMoffset->index();
				progStatus.oF_80 = FMoff_freq->value();
				break; // 80 meters
		case 3: progStatus.f40 = vfo->freq;
				progStatus.m40 = vfo->imode;
				progStatus.txT_40 = choice_FT8n_tTONE->value();
				progStatus.rxT_40 = choice_FT8n_rTONE->value();
				progStatus.offset_40 = FMoffset->index();
				progStatus.oF_40 = FMoff_freq->value();
				break; // 40 meters
		case 4: progStatus.f30 = vfo->freq;
				progStatus.m30 = vfo->imode;
				progStatus.txT_30 = choice_FT8n_tTONE->value();
				progStatus.rxT_30 = choice_FT8n_rTONE->value();
				progStatus.offset_30 = FMoffset->index();
				progStatus.oF_30 = FMoff_freq->value();
				break; // 30 meters
		case 5: progStatus.f20 = vfo->freq;
				progStatus.m20 = vfo->imode;
				progStatus.txT_20 = choice_FT8n_tTONE->value();
				progStatus.rxT_20 = choice_FT8n_rTONE->value();
				progStatus.offset_20 = FMoffset->index();
				progStatus.oF_20 = FMoff_freq->value();
				break; // 20 meters
		case 6: progStatus.f17 = vfo->freq;
				progStatus.m17 = vfo->imode;
				progStatus.txT_17 = choice_FT8n_tTONE->value();
				progStatus.rxT_17 = choice_FT8n_rTONE->value();
				progStatus.offset_17 = FMoffset->index();
				progStatus.oF_17 = FMoff_freq->value();
				break; // 17 meters
		case 7: progStatus.f15 = vfo->freq;
				progStatus.m15 = vfo->imode;
				progStatus.txT_15 = choice_FT8n_tTONE->value();
				progStatus.rxT_15 = choice_FT8n_rTONE->value();
				progStatus.offset_15 = FMoffset->index();
				progStatus.oF_15 = FMoff_freq->value();
				break; // 15 meters
		case 8: progStatus.f12 = vfo->freq;
				progStatus.m12 = vfo->imode;
				progStatus.txT_12 = choice_FT8n_tTONE->value();
				progStatus.rxT_12 = choice_FT8n_rTONE->value();
				progStatus.offset_12 = FMoffset->index();
				progStatus.oF_12 = FMoff_freq->value();
				break; // 12 meters
		case 9: progStatus.f10 = vfo->freq;
				progStatus.m10 = vfo->imode;
				progStatus.txT_10 = choice_FT8n_tTONE->value();
				progStatus.rxT_10 = choice_FT8n_rTONE->value();
				progStatus.offset_10 = FMoffset->index();
				progStatus.oF_10 = FMoff_freq->value();
				break; // 10 meters
		case 10:progStatus.f6 = vfo->freq;
				progStatus.m6 = vfo->imode;
				progStatus.txT_6 = choice_FT8n_tTONE->value();
				progStatus.rxT_6 = choice_FT8n_rTONE->value();
				progStatus.offset_6 = FMoffset->index();
				progStatus.oF_6 = FMoff_freq->value();
				break; // 6 meters
		case 11:progStatus.f2 = vfo->freq;
				progStatus.m2 = vfo->imode;
				progStatus.txT_2 = choice_FT8n_tTONE->value();
				progStatus.rxT_2 = choice_FT8n_rTONE->value();
				progStatus.offset_2 = FMoffset->index();
				progStatus.oF_2 = FMoff_freq->value();
				break; // 2 meters
		case 12:progStatus.f70 = vfo->freq;
				progStatus.m70 = vfo->imode;
				progStatus.txT_70 = choice_FT8n_tTONE->value();
				progStatus.rxT_70 = choice_FT8n_rTONE->value();
				progStatus.offset_70 = FMoffset->index();
				progStatus.oF_70 = FMoff_freq->value();
				break; // 70 cent'
		case 13:progStatus.f12G = vfo->freq;
				progStatus.m12G = vfo->imode;
				progStatus.txT_12G = choice_FT8n_tTONE->value();
				progStatus.rxT_12G = choice_FT8n_rTONE->value();
				progStatus.offset_12G = FMoffset->index();
				progStatus.oF_12G = FMoff_freq->value();
				break; // 1.2 GHz cent'
	}
}


void updateCTCSS(int band)
{
	switch (band) {
		case 1: choice_FT8n_tTONE->value(progStatus.txT_160);
				choice_FT8n_rTONE->value(progStatus.txT_160);
				FMoffset->index(progStatus.offset_160);
				FMoff_freq->value(progStatus.oF_160 );
				break; // 160 meters
		case 2: choice_FT8n_tTONE->value(progStatus.txT_80);
				choice_FT8n_rTONE->value(progStatus.txT_80);
				FMoffset->index(progStatus.offset_80);
				FMoff_freq->value(progStatus.oF_80 );
				break; // 80 meters
		case 3: choice_FT8n_tTONE->value(progStatus.txT_40);
				choice_FT8n_rTONE->value(progStatus.txT_40);
				FMoffset->index(progStatus.offset_40);
				FMoff_freq->value(progStatus.oF_40 );
				break; // 40 meters
		case 4: choice_FT8n_tTONE->value(progStatus.txT_30);
				choice_FT8n_rTONE->value(progStatus.txT_30);
				FMoffset->index(progStatus.offset_30);
				FMoff_freq->value(progStatus.oF_30 );
				break; // 30 meters
		case 5: choice_FT8n_tTONE->value(progStatus.txT_20);
				choice_FT8n_rTONE->value(progStatus.txT_20);
				FMoffset->index(progStatus.offset_20);
				FMoff_freq->value(progStatus.oF_20 );
				break; // 20 meters
		case 6: choice_FT8n_tTONE->value(progStatus.txT_17);
				choice_FT8n_rTONE->value(progStatus.txT_17);
				FMoffset->index(progStatus.offset_17);
				FMoff_freq->value(progStatus.oF_17 );
				break; // 17 meters
		case 7: choice_FT8n_tTONE->value(progStatus.txT_15);
				choice_FT8n_rTONE->value(progStatus.txT_15);
				FMoffset->index(progStatus.offset_15);
				FMoff_freq->value(progStatus.oF_15 );
				break; // 15 meters
		case 8: choice_FT8n_tTONE->value(progStatus.txT_12);
				choice_FT8n_rTONE->value(progStatus.txT_12);
				FMoffset->index(progStatus.offset_12);
				FMoff_freq->value(progStatus.oF_12 );
				break; // 12 meters
		case 9: choice_FT8n_tTONE->value(progStatus.txT_10);
				choice_FT8n_rTONE->value(progStatus.txT_10);
				FMoffset->index(progStatus.offset_10);
				FMoff_freq->value(progStatus.oF_10 );
				break; // 10 meters
		case 10:choice_FT8n_tTONE->value(progStatus.txT_6);
				choice_FT8n_rTONE->value(progStatus.txT_6);
				FMoffset->index(progStatus.offset_6);
				FMoff_freq->value(progStatus.oF_6 );
				break; // 6 meters
		case 11:choice_FT8n_tTONE->value(progStatus.txT_2);
				choice_FT8n_rTONE->value(progStatus.txT_2);
				FMoffset->index(progStatus.offset_2);
				FMoff_freq->value(progStatus.oF_2 );
				break; // 2 meters
		case 12:choice_FT8n_tTONE->value(progStatus.txT_70);
				choice_FT8n_rTONE->value(progStatus.txT_70);
				FMoffset->index(progStatus.offset_70);
				FMoff_freq->value(progStatus.oF_70 );
				progStatus.m70 = vfo->imode;
				break; // 70 cent'
	}
}

void cbBandSelect(int band)
{
	guard_lock gl_serial( &mutex_serial, std::string(__func__) );

	if (xcvr_name == rig_FT857D.name_ || xcvr_name == rig_FT897D.name_ ) {
		if (Fl::event_button() == FL_LEFT_MOUSE) {
			selrig->set_band_selection(band);
			updateCTCSS(band);
			if (band > 8) { // 10, 6, 2, 70, 1.2G
				int tTONE = PL_tones[choice_FT8n_tTONE->value()];
				int rTONE = PL_tones[choice_FT8n_rTONE->value()];
				selrig->set_tones(tTONE, rTONE);
				selrig->set_offset(FMoffset->index(), FMoff_freq->value());
			}
		} else if (Fl::event_button() == FL_RIGHT_MOUSE)
			set_band(band);
		return;
	}

	if (Fl::event_button() == FL_RIGHT_MOUSE) {
		selrig->rTONE = choice_rTONE->value();
		selrig->tTONE = choice_tTONE->value();
		selrig->set_band_selection(band);
		return;
	}

	selrig->get_band_selection(band);

// get freqmdbw
	if (selrig->inuse == onA) {
		vfoA.freq =selrig->get_vfoA();
		xml_A.update_freq(vfoA.freq);
		if (selrig->has_mode_control) {
			vfoA.imode = selrig->get_modeA();
			xml_A.update_mode(vfoA.imode);
		}
		if (selrig->has_bandwidth_control) {
			selrig->adjust_bandwidth(vfoA.imode);
			vfoA.iBW = selrig->get_bwA();
			xml_A.update_bw(vfoA.iBW);
		}
		vfo = &vfoA;
	} else {
		vfoB.freq = selrig->get_vfoB();
		xml_B.update_freq(vfoB.freq);
		if (selrig->has_mode_control) {
			vfoB.imode = selrig->get_modeB();
			xml_B.update_mode(vfoB.imode);
		}
		if (selrig->has_bandwidth_control) {
			selrig->adjust_bandwidth(vfoB.imode);
			vfoB.iBW = selrig->get_bwB();
			xml_B.update_bw(vfoB.iBW);
		}
		vfo = &vfoB;
	}
// local display freqmdbw
	if (selrig->has_mode_control) {
		set_Mode_BW_control(NULL);
	}
	if (selrig->has_bandwidth_control) {
		set_bandwidth_control();
		updateBandwidthControl();
	}
	if (selrig->inuse == onA) { FreqDispA->value(vfo->freq); FreqDispA->redraw(); }
	else       { FreqDispB->value(vfo->freq); FreqDispB->redraw(); }

	if (selrig->CIV && (selrig->name_ != rig_IC7200.name_)) {
		choice_tTONE->value(selrig->tTONE);
		choice_tTONE->redraw();
		choice_rTONE->value(selrig->rTONE);
		choice_rTONE->redraw();
	}

}

void enable_yaesu_bandselect(int btn_num, bool enable)
{
	switch (btn_num) {
		case 1:
		case 9:
			break;
		case 10:	// 6m
			if (enable) btn_yaesu_select_10->show();
			else btn_yaesu_select_10->hide();
			break;
		case 11:	// GEN
			if (enable) btn_yaesu_select_11->show();
			else btn_yaesu_select_11->hide();
			break;
		case 13:
			if (enable) op_yaesu_select60->show();
			else op_yaesu_select60->hide();
			break;
		default:
			break;
	}
}


float interpolate(float raw, const std::vector< std::vector< float > > & table) {
    if (table.empty() || table[0].size() != 2) return 0; // Error value

    // Handle cases where raw is outside the range of the table
    if (raw <= table.front()[0]) return table.front()[1];
    if (raw >= table.back()[0]) return table.back()[1];

    // Find the appropriate segment in the table
    for (size_t i = 0; i < table.size() - 1; ++i) {
        if (raw >= table[i][0] && raw <= table[i + 1][0]) {
            // Linear interpolation
            float slope = (table[i + 1][1] - table[i][1]) / (table[i + 1][0] - table[i][0]);
            return table[i][1] + slope * (raw - table[i][0]);
        }
    }

    return 0; // Error value
}
