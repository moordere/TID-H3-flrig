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

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "threads.h"
#include "ptt.h"
#include "debug.h"
#include "rig_io.h"
#include "rig.h"
#include "support.h"

//#include "gpio_ptt.h"
#include "cmedia.h"

#if USE_LIBGPIOD
#include "gpio_common.h"
#endif

// used for transceivers with a single vfo, called only by rigPTT
static XCVR_STATE fake_vfo;

static void showfreq(void *)
{
	FreqDispA->value(vfoA.freq);
}

static void fake_split(int on)
{
	if (on) {
		fake_vfo = vfoA;
		vfoA.freq = vfoB.freq;
		selrig->set_vfoA(vfoA.freq);
		Fl::awake(showfreq);
	} else {
		vfoA = fake_vfo;
		selrig->set_vfoA(vfoA.freq);
		Fl::awake(showfreq);
	}
}

extern void xmlrpc_ptt(int);

#if USE_LIBGPIOD

int ptt_gpio_num = GPIO_COMMON_UNKNOWN;

void open_gpio_ptt(void)
{
	if (!progStatus.enable_gpio_ptt || (progStatus.gpio_ptt_line == -1) || ptt_gpio_num != GPIO_COMMON_UNKNOWN) {
		return;
	}
	LOG_INFO("Opening GPIO line %d on device %s for PTT",
		progStatus.gpio_ptt_line,
		progStatus.gpio_ptt_device.c_str());
	ptt_gpio_num = gpio_common_open_line(progStatus.gpio_ptt_device.c_str(), progStatus.gpio_ptt_line, false);
	if (ptt_gpio_num == GPIO_COMMON_UNKNOWN) {
		LOG_ERROR("Failed to open GPIO line");
	} else {
		LOG_INFO("GPIO line opened successfully");
	}
	return;
}

void close_gpio_ptt(void)
{
	gpio_common_close(ptt_gpio_num);
	ptt_gpio_num = GPIO_COMMON_UNKNOWN;
}

void set_gpio_ptt(bool ptt)
{
	int ret;
	if (ptt_gpio_num == GPIO_COMMON_UNKNOWN)
		open_gpio_ptt();
	if (ptt_gpio_num != GPIO_COMMON_UNKNOWN) {
		LOG_INFO("Setting GPIO lines for PTT %s", ptt ? "ON" : "OFF");
		ret = gpio_common_set(ptt_gpio_num, ptt);
		if (ret < 0) {
			LOG_ERROR("Failed to set GPIO line");
		}
	}
}

#endif //USE_LIBGPIOD
int CW_ptt = 0;

void rigPTT(bool on)
{
	if (progStatus.xmlrpc_rig) {
		xmlrpc_ptt(on);
		return;
	}

	if (!on && progStatus.split && !selrig->can_split())
		fake_split(on);

	try {
		std::string smode = selrig->modes_[vfo->imode];

		if ((smode.find("CW") != std::string::npos) && progStatus.disable_CW_ptt) {
			CW_ptt = on;
			return;
		}

		if (smode.find("RTTY") != std::string::npos && on == false)
			wait_fskPTT();

		if (progStatus.serial_catptt == PTT_BOTH || progStatus.serial_catptt == PTT_SET) {
			selrig->set_PTT_control(on);
		} else if (progStatus.serial_dtrptt == PTT_BOTH || progStatus.serial_dtrptt == PTT_SET) {
			RigSerial->SetPTT(on);
//			selrig->set_PTT_control(on); 
		} else if (progStatus.serial_rtsptt == PTT_BOTH || progStatus.serial_rtsptt == PTT_SET) {
			RigSerial->SetPTT(on);
//			selrig->set_PTT_control(on);
		} else if (SepSerial->IsOpen() && (progStatus.sep_dtrptt == PTT_BOTH || progStatus.sep_dtrptt == PTT_SET)) {
			SepSerial->SetPTT(on);
		} else if (SepSerial->IsOpen() && (progStatus.sep_rtsptt == PTT_BOTH || progStatus.sep_rtsptt == PTT_SET)) {
			SepSerial->SetPTT(on);
	#if USE_LIBGPIOD
		} else if (progStatus.enable_gpio_ptt) {
			set_gpio_ptt(on);
	#endif
		} else if (progStatus.cmedia_ptt == PTT_BOTH || progStatus.cmedia_ptt == PTT_SET) {
			set_cmedia(on);
		} else
			throw ("No PTT i/o connected");
	} catch (const std::exception& e) {
		LOG_DEBUG("%s", e.what());
		failure_trace(1, e.what());
	} catch (const char *why) {
		LOG_DEBUG("%s", why);
		failure_trace(1, why);
	}
}

extern bool xml_ptt_state();

bool ptt_state()
{
	std::string smode = selrig->modes_[vfo->imode];

	bool PTT = false;
	if (progStatus.xmlrpc_rig) {
		PTT = xml_ptt_state();
	}
	else if ((smode.find("CW") != std::string::npos) && progStatus.disable_CW_ptt) {
		PTT = CW_ptt;
	}
	else if (progStatus.serial_catptt == PTT_BOTH || progStatus.serial_catptt == PTT_GET) {
		PTT = selrig->get_PTT();
	}
	else if (progStatus.serial_dtrptt == PTT_BOTH || progStatus.serial_dtrptt == PTT_GET) {
		PTT = selrig->get_PTT();
	}
	else if (progStatus.serial_rtsptt == PTT_BOTH || progStatus.serial_rtsptt == PTT_GET){
		PTT = selrig->get_PTT();
	}
	else if (SepSerial->IsOpen() && 
		(progStatus.sep_dtrptt == PTT_BOTH || progStatus.sep_dtrptt == PTT_GET)) {
		PTT = SepSerial->getPTT();
	}
	else if (SepSerial->IsOpen() && 
		(progStatus.sep_rtsptt == PTT_BOTH || progStatus.sep_rtsptt == PTT_GET)) {
		PTT = SepSerial->getPTT();
	}
#if USE_LIBGPIOD
	else if (progStatus.gpio_ptt == PTT_BOTH || progStatus.gpio_ptt == PTT_GET) {
		PTT = get_gpio();
	}
#endif
	else if (progStatus.cmedia_ptt == PTT_BOTH || progStatus.cmedia_ptt == PTT_GET) {
		PTT = get_cmedia();
	}

	return PTT;
}

