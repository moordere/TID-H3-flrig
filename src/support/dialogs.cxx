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

#include "config.h"
#include "compat.h" // Must precede all FL includes

#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Native_File_Chooser.H>

#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef __WIN32__
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#include <glob.h>
#endif

#include "dialogs.h"
#include "rigs.h"
#include "util.h"
#include "debug.h"
#include "serial.h"
#include "support.h"
#include "rigpanel.h"
#include "rigbase.h"
#include "font_browser.h"
#include "ui.h"
#include "status.h"
#include "rig.h"
#include "rigbase.h"
#include "socket_io.h"
#include "rigpanel.h"
#include "gettext.h"
#include "cwioUI.h"
#include "fskioUI.h"

Fl_Double_Window *dlgDisplayConfig = NULL;
Fl_Double_Window *dlgColorsDialog = NULL;
Fl_Double_Window *dlgXcvrConfig = NULL;
Fl_Double_Window *dlgMemoryDialog = NULL;
Fl_Double_Window *dlgControls = NULL;

Font_Browser     *fntbrowser = NULL;

Fl_Color flrig_def_color(int);

//======================================================================
// test comm ports
//======================================================================

void clear_combos()
{
	selectCommPort->clear();
	selectAuxPort->clear();
	selectSepPTTPort->clear();
	selectCommPort->add("NONE");
	selectCommPort->add("xml_client");
	selectAuxPort->add("NONE");
	selectSepPTTPort->add("NONE");
	select_cwioPORT->add("NONE");
	select_fskioPORT->add("NONE");
}

void add_combos(const char *port)
{
	if (progStatus.serialtrace) {
		static char sztr[100];
		snprintf(sztr, sizeof(sztr), "Discovered %s", port);
		ser_trace(1, sztr);
	}
	selectCommPort->add(port);
	selectAuxPort->add(port);
	selectSepPTTPort->add(port);
	select_cwioPORT->add(port);
	select_fskioPORT->add(port);
}

void set_combo_value()
{
	add_combos(progStatus.xcvr_serial_port.c_str());
	add_combos(progStatus.sep_serial_port.c_str());
	add_combos(progStatus.aux_serial_port.c_str());
	add_combos(progStatus.cwioPORT.c_str());
	add_combos(progStatus.FSK_PORT.c_str());

	selectCommPort->value(progStatus.xcvr_serial_port.c_str());
	selectAuxPort->value(progStatus.aux_serial_port.c_str());
	selectSepPTTPort->value(progStatus.sep_serial_port.c_str());
	select_cwioPORT->value(progStatus.cwioPORT.c_str());
	select_fskioPORT->value(progStatus.FSK_PORT.c_str());
}

#if defined ( __WIN32__ )

void init_win_port_combos() {
	char lpTargetPath[1024]; // Buffer to store the target path of the COM port
	DWORD dwResult;

// Iterate through potential COM port names (COM1 to COM255)
	for (int i = 1; i <= 255; i++) {
		char szComName[10];
		snprintf(szComName, sizeof(szComName), "COM%d", i);

// Query the device for its target path
		dwResult = QueryDosDeviceA(szComName, lpTargetPath, sizeof(lpTargetPath));

// If QueryDosDevice returns a non-zero value, it means the device exists
		if (dwResult != 0) add_combos( szComName );
	}
}

#else 

#if defined (__APPLE__)

#  include <CoreFoundation/CoreFoundation.h>
#  include <IOKit/IOKitLib.h>
#  include <IOKit/serial/IOSerialKeys.h>
#  include <IOKit/IOBSD.h>

void init_mac_port_combos() {

	const char* tty_fmt[] = {
		"/dev/cu.*",
		"/dev/tty.*"
	};

// Create a matching dictionary: 
//   This dictionary specifies the criteria for the serial ports you are 
//   looking for. You typically match on kIOSerialBSDServiceValue.

	CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOSerialBSDServiceValue);

// Get an iterator for matching services:
//   Use IOServiceGetMatchingServices to find all services that match your dictionary.

	io_iterator_t serialPortIterator;
	kern_return_t kernResult = IOServiceGetMatchingServices
									(kIOMasterPortDefault,
									matchingDict,
									&serialPortIterator);
	if (KERN_SUCCESS != kernResult) {
		LOG_ERROR( "KERNAL ERROR" );
		return;
	}

// Iterate through the services: Loop through the serialPortIterator to 
//   get individual serial port devices.

	io_object_t serialPortService;
	while ((serialPortService = IOIteratorNext(serialPortIterator))) {
		// Get the callout device path (e.g., /dev/cu.usbserial-XXXX)
		CFStringRef calloutPath = (CFStringRef)IORegistryEntryCreateCFProperty(
									 serialPortService,
									 CFSTR(kIOCalloutDeviceKey),
									 kCFAllocatorDefault, 0);
		if (calloutPath) {

// Convert CFStringRef to C string and print or store

			char pathBuffer[1024];
			if ( CFStringGetCString(
					calloutPath,
					pathBuffer,
					sizeof(pathBuffer),
					kCFStringEncodingUTF8)) {
				add_combos( pathBuffer );
			}
			CFRelease(calloutPath);
		}
		IOObjectRelease(serialPortService); // Release the service object
	}

// Release the iterator.

	IOObjectRelease(serialPortIterator);

}

//end defined ( __APPLE__ )

# else

# if defined ( __OpenBSD__ ) || defined ( __NetBSD__ )

void init_bsd_port_combos() {

#  define PATH_MAX 1024

	struct stat st;

	char ttyname[PATH_MAX + 1];

#if definted ( __NetBSD__ )
	const char* tty_fmt[] = {
		"/dev/tty%2.2u"
	};
#else
	const char* tty_fmt[] = {
		"/dev/ttyd%u",
		"/dev/ttyU%u",
	};
#endif

#  define TTY_MAX 4

	glob_t gbuf;
	glob("/dev/serial/by-id/*", 0, NULL, &gbuf);
	for (size_t j = 0; j < gbuf.gl_pathc; j++) {
		if ( !(stat(gbuf.gl_pathv[j], &st) == 0 && S_ISCHR(st.st_mode)) ||
		     strstr(gbuf.gl_pathv[j], "modem") )
			continue;
		add_combos( gbuf.gl_pathv[j] );
	}
	globfree(&gbuf);

	for (size_t i = 0; i < sizeof(tty_fmt)/sizeof(*tty_fmt); i++) {
		for (unsigned j = 0; j < TTY_MAX; j++) {
			snprintf(ttyname, sizeof(ttyname), tty_fmt[i], j);
			if ( !(stat(ttyname, &st) == 0 && S_ISCHR(st.st_mode)) )
				continue;
			add_combos( ttyname );
		}

}

# else // must be good old Linux

void init_linux_port_combos() {

	struct stat st;
	glob_t gbuf;

	glob("/dev/serial/by-id/*", 0, NULL, &gbuf);
	for (size_t j = 0; j < gbuf.gl_pathc; j++) {
		if ( !(stat(gbuf.gl_pathv[j], &st) == 0 && S_ISCHR(st.st_mode)) ||
		     strstr(gbuf.gl_pathv[j], "modem") )
			continue;
		add_combos( gbuf.gl_pathv[j] );
	}
	globfree(&gbuf);

	const char* tty_fmt[] = {
		"/dev/ttyS%u",
		"/dev/ttyUSB%u",
		"/dev/usb/ttyUSB%u",
		"/dev/ttyACM%u",
		"/dev/usb/ttyACM%u",
		"/dev/rfcomm%u",
		"/dev/pts/%u",
		"/opt/vttyS%u"
	};
	char ttyname[512];

	for (size_t i = 0; i < sizeof(tty_fmt)/sizeof(*tty_fmt); i++) {
		for (unsigned j = 0; j < 8; j++) {
			snprintf(ttyname, sizeof(ttyname), tty_fmt[i], j);
			if ( !(stat(ttyname, &st) == 0 && S_ISCHR(st.st_mode)) )
				continue;
			add_combos ( ttyname );
		}
	}
}

#  endif
# endif
#endif

void init_port_combos()
{
	clear_combos();

#if defined (__WIN32__)
	init_win_port_combos();
#else
# if defined (__APPLE__)
	init_mac_port_combos();
# else
#  if defined(__OpenBSD__) || defined(__NetBSD__) 
	init_bsd_port_combos();
#  else
	init_linux_port_combos();
#  endif
# endif
#endif

}

//======================================================================

void cbCIVdefault()
{
	char hexstr[8];
	rigbase *srig = (rigbase *)(selectRig->data());
	xcvr_name = srig->name_;
	LOG_INFO("picked %s", xcvr_name.c_str());

	snprintf(hexstr, sizeof(hexstr), "0x%02X", srig->defaultCIV);
	txtCIV->value(hexstr);
	progStatus.CIV = srig->defaultCIV;
	srig->adjustCIV(progStatus.CIV);
}

void cbCIV()
{
	int picked = selectRig->index();
	int adr = 0;
	rigbase *srig = rigs[picked];
	sscanf(txtCIV->value(), "0x%2X", &adr);
	progStatus.CIV = adr;
	srig->adjustCIV(progStatus.CIV);
}

void cbUSBaudio()
{
	progStatus.USBaudio = btnUSBaudio->value();
}

void configXcvr()
{

	selectCommPort->value(progStatus.xcvr_serial_port.c_str());
	selectAuxPort->value(progStatus.aux_serial_port.c_str());
	selectSepPTTPort->value(progStatus.sep_serial_port.c_str());

	if (selrig->CIV) {
		char hexstr[8];
		snprintf(hexstr, sizeof(hexstr), "0x%02X", selrig->CIV);
		txtCIV->value(hexstr);
		txtCIV->activate();
		btnCIVdefault->activate();
		if (xcvr_name == rig_IC7200.name_ ||
			xcvr_name == rig_IC7300.name_ ||
			xcvr_name == rig_IC7600.name_ ) {
			btnUSBaudio->value(progStatus.USBaudio);
			btnUSBaudio->activate();
		} else
			btnUSBaudio->deactivate();
	} else {
		txtCIV->value("");
		txtCIV->deactivate();
		btnCIVdefault->deactivate();
		btnUSBaudio->value(0);
		btnUSBaudio->deactivate();
	}

	select_tab(_("Xcvr"));
}

void open_poll_tab()
{
	select_tab(_("Poll"));
}

void open_trace_tab()
{
	select_tab(_("Trace"));
}

void open_commands_tab()
{
	select_tab(_("Commands"));
}

void open_restore_tab()
{
	select_tab(_("Restore"));
}

void open_send_command_tab()
{
	select_tab(_("Send"));
}

void open_tcpip_tab()
{
	select_tab(_("TCPIP & TCI"));
}

void open_cmedia_tab()
{
	select_tab(_("PTT-Cmedia"));
}

void open_tmate2_tab()
{
	select_tab(_("TMATE-2"));
}

void open_ptt_tab()
{
	select_tab(_("PTT-Generic"));
}

void open_gpio_tab()
{
	select_tab(_("GPIO PTT & CW"));
}

void open_other_tab()
{
	select_tab(_("Other"));
}

void open_server_tab()
{
	select_tab(_("Server"));
}

void open_client_tab()
{
	select_tab(_("Client"));
}

void createXcvrDialog()
{
	dlgXcvrConfig = XcvrDialog();

	init_port_combos();

	mnuBaudrate->clear();
	for (int i = 0; szBaudRates[i] != NULL; i++)
		mnuBaudrate->add(szBaudRates[i]);

	cbo_tt550_agc_level->add("slow");
	cbo_tt550_agc_level->add("med");
	cbo_tt550_agc_level->add("fast");
	cbo_tt550_agc_level->index(progStatus.tt550_agc_level);

	cbo_tt550_nb_level->add("NONE");
	cbo_tt550_nb_level->add("1");
	cbo_tt550_nb_level->add("2");
	cbo_tt550_nb_level->add("3");
	cbo_tt550_nb_level->add("4");
	cbo_tt550_nb_level->add("5");
	cbo_tt550_nb_level->add("6");
	cbo_tt550_nb_level->add("7");
	cbo_tt550_nb_level->index(progStatus.tt550_nb_level);

	initRigCombo();
}

// Frequency display font / colors
Fl_Font selfont;

void cbFreqControlFontBrowser(Fl_Widget*, void*) {
	selfont = fntbrowser->fontNumber();
	lblTest->labelfont(selfont);
	dlgDisplayConfig->redraw();
	dlgColorsDialog->redraw();
	fntbrowser->hide();
}

void cbPrefFont()
{
#if FLRIG_FLTK_API_MINOR >3
	fntbrowser->screen_num( mainwindow->screen_num() );
#endif
	fntbrowser->fontNumber(progStatus.fontnbr);
	fntbrowser->callback(cbFreqControlFontBrowser);
	fntbrowser->show();
}

uchar fg_red, fg_green, fg_blue;
uchar bg_red, bg_green, bg_blue;

uchar smeterRed = 0, smeterGreen = 180, smeterBlue = 0,
	  pwrRed = 180, pwrGreen = 0, pwrBlue = 0,
	  alcRed = 148, alcGreen = 0, alcBlue = 148,
	  swrRed = 148, swrGreen = 0, swrBlue = 148,
	  iddRed = 0, iddGreen = 0, iddBlue = 128,
	  voltsRed = 0, voltsGreen = 0, voltsBlue = 128;

uchar smeter_peak_red = 255, smeter_peak_green = 0, smeter_peak_blue = 0,
	  pwr_peak_red = 255, pwr_peak_green = 0, pwr_peak_blue = 0,
	  alc_peak_red = 255, alc_peak_green = 0, alc_peak_blue = 0,
	  swr_peak_red = 255, swr_peak_green = 0, swr_peak_blue = 0,
	  idd_peak_red = 255, idd_peak_green = 0, idd_peak_blue = 0,
	  volts_peak_red = 255, volts_peak_green = 0, volts_peak_blue = 0;

static Fl_Color bgclr;
static Fl_Color fgclr;

static Fl_Color fgsys;
static uchar fg_sys_red, fg_sys_green, fg_sys_blue;

static Fl_Color bgsys;
static uchar bg_sys_red, bg_sys_green, bg_sys_blue;

static Fl_Color bg2sys;
static uchar bg2_sys_red, bg2_sys_green, bg2_sys_blue;

static Fl_Color bg_slider;
static uchar bg_slider_red, bg_slider_green, bg_slider_blue;

static Fl_Color btn_slider;
static uchar btn_slider_red, btn_slider_green, btn_slider_blue;

static Fl_Color btn_lt_color;
static uchar btn_lt_color_red, btn_lt_color_green, btn_lt_color_blue;

static Fl_Color tab_color;
static uchar tab_red, tab_green, tab_blue;

void cb_lighted_button()
{
	uchar r = btn_lt_color_red, g = btn_lt_color_green, b = btn_lt_color_blue;
	if (fl_color_chooser("Foreground color", r, g, b)) {
		btn_lt_color_red = r; btn_lt_color_green = g; btn_lt_color_blue = b;
		btn_lt_color = fl_rgb_color(r, g, b);
		btn_lighted->selection_color(btn_lt_color);
		btn_lighted->value(1);
		btn_lighted->redraw();
	}
}

void cb_lighted_default()
{
	btn_lt_color = flrig_def_color(FL_YELLOW);
	btn_lt_color_red = ((btn_lt_color >> 24) & 0xFF);
	btn_lt_color_green = ((btn_lt_color >> 16) & 0xFF);
	btn_lt_color_blue = ((btn_lt_color >> 8) & 0xFF);
	btn_lighted->selection_color(btn_lt_color);
	btn_lighted->value(1);
	btn_lighted->redraw();
}

void cb_change_hrd_button()
{
	progStatus.hrd_buttons = !progStatus.hrd_buttons;
	FreqDispA->set_hrd(progStatus.hrd_buttons);
	FreqDispB->set_hrd(progStatus.hrd_buttons);
}

void set_sliders_when()
{
	if (sldrSQUELCH)
		sldrSQUELCH->when(progStatus.sliders_button);
	if (sldrMICGAIN)
		sldrMICGAIN->when(progStatus.sliders_button);
	if (sldrIFSHIFT)
		sldrIFSHIFT->when(progStatus.sliders_button);
	if (sldrNR)
		sldrNR->when(progStatus.sliders_button);
	if (sldrNOTCH)
		sldrNOTCH->when(progStatus.sliders_button);
	if (sldrRFGAIN)
		sldrRFGAIN->when(progStatus.sliders_button);
	if (sldrINNER)
		sldrINNER->when(progStatus.sliders_button);
	if (sldrOUTER)
		sldrOUTER->when(progStatus.sliders_button);
	if (sldrPOWER)
		sldrPOWER->when(progStatus.sliders_button);
	if (sldrVOLUME)
		sldrVOLUME->when(progStatus.sliders_button);
	if (ic7610_digi_sel_val)
		ic7610_digi_sel_val->when(progStatus.sliders_button);
	if (sldr_nb_level)
		sldr_nb_level->when(progStatus.sliders_button);
}

void cb_change_sliders_button()
{
	if (progStatus.sliders_button == FL_WHEN_CHANGED)
		progStatus.sliders_button = FL_WHEN_RELEASE;
	else
		progStatus.sliders_button = FL_WHEN_CHANGED;
	set_sliders_when();
}

void cb_slider_defaults()
{
	bg_slider_red = 232;
	bg_slider_green = 255;
	bg_slider_blue = 232;

	btn_slider_red = 0;
	btn_slider_green = 0;
	btn_slider_blue = 128;

	bg_slider = fl_rgb_color( 232, 255, 232);
	btn_slider = fl_rgb_color( 0, 0, 128);

	sldrColors->color(bg_slider);
	sldrColors->selection_color(btn_slider);
	sldrColors->redraw();
}

void cb_slider_background()
{
	uchar r = bg_slider_red, g = bg_slider_green, b = bg_slider_blue;
	if (fl_color_chooser("Foreground color", r, g, b)) {
		bg_slider_red = r; bg_slider_green = g; bg_slider_blue = b;
		bg_slider = fl_rgb_color(r, g, b);
		sldrColors->color(bg_slider);
		sldrColors->selection_color(btn_slider);
		sldrColors->redraw();
	}
}

void cb_slider_select()
{
	uchar r = btn_slider_red, g = btn_slider_green, b = btn_slider_blue;
	if (fl_color_chooser("Foreground color", r, g, b)) {
		btn_slider_red = r; btn_slider_green = g; btn_slider_blue = b;
		btn_slider = fl_rgb_color(r, g, b);
		sldrColors->color(bg_slider);
		sldrColors->selection_color(btn_slider);
		sldrColors->redraw();
	}
}

void redraw_dialogs()
{
	Fl_Double_Window *wlist[] = {
		mainwindow, tabs_dialog,
		cwio_keyer_dialog, cwio_editor, cwio_configure,
		FSK_keyer_dialog, FSK_configure,
		meter_scale_dialog, meters_dialog,
		dlgDisplayConfig, dlgColorsDialog,
		dlgMemoryDialog, meter_filters,
		dlgXcvrConfig,
		NULL };

//	Fl_Color bgc = fl_rgb_color( progStatus.bg_red, progStatus.bg_green, progStatus.bg_blue );
	for (size_t n = 0; n < sizeof(wlist) / sizeof(*wlist); n++) {
		if (wlist[n]) {
//			wlist[n]->color(bgc);
			wlist[n]->redraw();
		}
	}

	tab_tree->item_labelbgcolor(bgsys);
	tab_tree->item_labelfgcolor(fgsys);
}

void cb_sys_defaults()
{
	bgsys = flrig_def_color(FL_BACKGROUND_COLOR);
	bg_sys_red = ((bgsys >> 24) & 0xFF);
	bg_sys_green = ((bgsys >> 16) & 0xFF);
	bg_sys_blue = ((bgsys >> 8) & 0xFF);

	bg2sys = flrig_def_color(FL_BACKGROUND2_COLOR);
	bg2_sys_red = ((bg2sys) >> 24 & 0xFF);
	bg2_sys_green = ((bg2sys) >> 16 & 0xFF);
	bg2_sys_blue = ((bg2sys) >> 8 & 0xFF);

	fgsys = flrig_def_color(FL_FOREGROUND_COLOR);
	fg_sys_red = (fgsys >> 24) & 0xFF;
	fg_sys_green = (fgsys >> 16) & 0xFF;
	fg_sys_blue = (fgsys >> 8) & 0xFF;

	Fl::background(bg_sys_red, bg_sys_green, bg_sys_blue);
	Fl::background2(bg2_sys_red, bg2_sys_green, bg2_sys_blue);
	Fl::foreground(fg_sys_red, fg_sys_green, fg_sys_blue);

	redraw_dialogs();
}

void set_system_colors()
{
	fgsys = fl_rgb_color( progStatus.fg_sys_red, progStatus.fg_sys_green, progStatus.fg_sys_red );
	Fl::foreground( progStatus.fg_sys_red, progStatus.fg_sys_green, progStatus.fg_sys_red );
	bgsys = fl_rgb_color(  progStatus.bg_sys_red, progStatus.bg_sys_green, progStatus.bg_sys_red );
	Fl::background(   progStatus.bg_sys_red, progStatus.bg_sys_green, progStatus.bg_sys_red );
	bg2sys = fl_rgb_color(   progStatus.bg2_sys_red, progStatus.bg2_sys_green, progStatus.bg2_sys_red );
	Fl::background2 (   progStatus.bg2_sys_red, progStatus.bg2_sys_green, progStatus.bg2_sys_red );
	redraw_dialogs();
}

void cb_sys_foreground()
{
	uchar r = progStatus.fg_sys_red, g = progStatus.fg_sys_green, b = progStatus.fg_sys_blue;
	if (fl_color_chooser("Foreground color", r, g, b)) {
		progStatus.fg_sys_red = fg_sys_red = r;
		progStatus.fg_sys_green = fg_sys_green = g;
		progStatus.fg_sys_blue = fg_sys_blue = b;
		fgsys = fl_rgb_color(r, g, b);
		Fl::foreground(r, g, b);
		redraw_dialogs();
	}
}

void cb_sys_background()
{
	uchar r = progStatus.bg_sys_red, g = progStatus.bg_sys_green, b = progStatus.bg_sys_blue;
	if (fl_color_chooser("Background color", r, g, b)) {
		progStatus.bg_sys_red = bg_sys_red = r;
		progStatus.bg_sys_green = bg_sys_green = g;
		progStatus.bg_sys_blue = bg_sys_blue = b;
		bgsys = fl_rgb_color(r, g, b);
		Fl::background(r, g, b);
		redraw_dialogs();
	}
}

void cb_sys_background2()
{
	uchar r = progStatus.bg2_sys_red, g = progStatus.bg2_sys_green, b = progStatus.bg2_sys_blue;
	if (fl_color_chooser("Background2 color", r, g, b)) {
		progStatus.bg2_sys_red = bg2_sys_red = r;
		progStatus.bg2_sys_green = bg2_sys_green = g;
		progStatus.bg2_sys_blue = bg2_sys_blue = b;
		bg2sys = fl_rgb_color(r, g, b);
		Fl::background2(r, g, b);
// void Fl::background2 ( uchar r, uchar g, uchar b )
// Changes the alternative background color.
// This color is used as a background by Fl_Input and other text widgets.
// This call may change fl_color(FL_FOREGROUND_COLOR) if it does not provide 
// sufficient contrast to FL_BACKGROUND2_COLOR.
// we want to preserve the user selection.
		Fl::background(progStatus.bg_sys_red, progStatus.bg_sys_green, progStatus.bg_sys_blue);
		Fl::foreground(progStatus.fg_sys_red, progStatus.fg_sys_green, progStatus.fg_sys_blue);
		redraw_dialogs();
	}
}

void cbBacklightColor()
{
	uchar r = bg_red, g = bg_green, b = bg_blue;
	if (fl_color_chooser("Background color", r, g, b)) {
		bg_red = r; bg_green = g; bg_blue = b;
		bgclr = fl_rgb_color(r, g, b);
		lblTest->color(bgclr);

		color_SMETER->color( fl_rgb_color (smeterRed, smeterGreen, smeterBlue), bgclr );
		color_PWR->color(fl_rgb_color (pwrRed, pwrGreen, pwrBlue), bgclr);
		color_SWR->color(fl_rgb_color (swrRed, swrGreen, swrBlue), bgclr);
		color_VOLTS->color(fl_rgb_color (voltsRed, voltsGreen, voltsBlue), bgclr);
		color_ALC->color(fl_rgb_color ( alcRed, alcGreen, alcBlue), bgclr);
		color_IDD->color(fl_rgb_color ( iddRed, iddGreen, iddBlue), bgclr);

		color_SMETER->PeakColor( fl_rgb_color (smeter_peak_red, smeter_peak_green, smeter_peak_blue) );
		color_PWR->PeakColor(fl_rgb_color (pwr_peak_red, pwr_peak_green, pwr_peak_blue) );
		color_SWR->PeakColor(fl_rgb_color (swr_peak_red, swr_peak_green, swr_peak_blue) );
		color_VOLTS->PeakColor(fl_rgb_color (volts_peak_red, volts_peak_green, volts_peak_blue));
		color_ALC->PeakColor(fl_rgb_color ( alc_peak_red, alc_peak_green, alc_peak_blue) );
		color_IDD->PeakColor(fl_rgb_color ( idd_peak_red, idd_peak_green, idd_peak_blue) );

		btn_change_SMETER->color(bgclr);
		btn_change_PWR->color(bgclr);
		btn_change_SWR->color(bgclr);
		btn_change_ALC->color(bgclr);
		btn_change_IDD->color(bgclr);
		btn_change_VOLTS->color(bgclr);
		meter_group->color(bgclr);

		dlgColorsDialog->redraw();
	}
}

void cbPrefForeground()
{
	uchar r = fg_red, g = fg_green, b = fg_blue;
	if (fl_color_chooser("Foreground color", r, g, b)) {
		fg_red = r; fg_green = g; fg_blue = b;
		fgclr = fl_rgb_color(r, g, b);
		lblTest->labelcolor(fgclr);

		btn_change_SMETER->labelcolor(fgclr);
		btn_change_PWR->labelcolor(fgclr);
		btn_change_SWR->labelcolor(fgclr);
		btn_change_ALC->labelcolor(fgclr);
		btn_change_IDD->labelcolor(fgclr);
		btn_change_VOLTS->labelcolor(fgclr);

		meter_group->labelcolor(fgclr);
		dlgColorsDialog->redraw();
	}
}

void default_freq_display()
{
	lblTest->labelfont(progStatus.fontnbr);
	lblTest->labelcolor(fl_rgb_color(progStatus.fg_red, progStatus.fg_green, progStatus.fg_blue));
}

void default_meters()
{
	Fl_Color c;
	bg_red = 232; bg_green = 255; bg_blue = 232;
	bgclr = fl_rgb_color( bg_red, bg_green, bg_blue);
		lblTest->color(bgclr);
		color_SMETER->color( fl_rgb_color (smeterRed, smeterGreen, smeterBlue), bgclr );
		color_PWR->color(fl_rgb_color (pwrRed, pwrGreen, pwrBlue), bgclr);
		color_SWR->color(fl_rgb_color (swrRed, swrGreen, swrBlue), bgclr);
		btn_change_SMETER->color(bgclr);
		btn_change_PWR->color(bgclr);
		btn_change_SWR->color(bgclr);
		btn_change_ALC->color(bgclr);
		btn_change_IDD->color(bgclr);
		btn_change_VOLTS->color(bgclr);
		meter_group->color(bgclr);

	fg_red = 0; fg_green = 0; fg_blue = 0;
	fgclr = (Fl_Color)0;
		lblTest->labelcolor(fgclr);
		btn_change_SMETER->labelcolor(fgclr);
		btn_change_PWR->labelcolor(fgclr);
		btn_change_SWR->labelcolor(fgclr);
		btn_change_ALC->labelcolor(fgclr);
		btn_change_IDD->labelcolor(fgclr);
		btn_change_VOLTS->labelcolor(fgclr);
		meter_group->labelcolor(fgclr);
	smeterRed = 0; smeterGreen = 180; smeterBlue = 0;
		c = fl_rgb_color (smeterRed, smeterGreen, smeterBlue);
		color_SMETER->color(c, bgclr );
	pwrRed = 180; pwrGreen = 0; pwrBlue = 0;
		c = fl_rgb_color( pwrRed, pwrGreen, pwrBlue );
		color_PWR->color(c, bgclr);
	alcRed = swrRed = 148; alcGreen = swrGreen = 0; alcBlue = swrBlue = 148;
		c = fl_rgb_color(swrRed, swrGreen, swrBlue);
		color_SWR->color(c, bgclr);
		color_ALC->color(c, bgclr);
	iddRed = voltsRed = 128; iddGreen = voltsGreen = 0; iddBlue = voltsBlue = 0;
		c = fl_rgb_color(voltsRed, voltsGreen, voltsBlue);
		color_VOLTS->color(c, bgclr);
		color_IDD->color(c, bgclr);
	smeter_peak_red = 255; smeter_peak_green = 0; smeter_peak_blue = 0;
		c = fl_rgb_color( smeter_peak_red, smeter_peak_green, smeter_peak_blue );
		color_SMETER->PeakColor(c);
		color_PWR->PeakColor(c);
		color_SWR->PeakColor(c);
		color_VOLTS->PeakColor(c);
		color_IDD->PeakColor(c);
		color_ALC->PeakColor(c);

	dlgColorsDialog->redraw();
}

void set_tab_colors()
{
	Fl_Color tab_color = fl_rgb_color( progStatus.tab_red, progStatus.tab_green, progStatus.tab_blue);
	if (tabsGeneric) {
		tabsGeneric->selection_color(tab_color);
		tabsGeneric->redraw();
	}
	if (tabs550) {
		tabs550->selection_color(tab_color);
		tabs550->redraw();
	}
	if (tabCmds) {
		tabCmds->selection_color(tab_color);
		tabCmds->redraw();
	}
}

void cb_tab_defaults()
{
	tab_red = tab_green = tab_blue = 230;
	tabs_color->selection_color(fl_rgb_color( 230, 230, 230 ));
}

void cb_tab_colors()
{
	uchar r = tab_red, g = tab_green, b = tab_blue;
	if (fl_color_chooser("TAB color", r, g, b)) {
		tab_red = r;
		tab_green = g;
		tab_blue = b;
		tab_color = fl_rgb_color(r, g, b);
		tabs_color->selection_color(tab_color);
		tabs_color->redraw();
	}
}

// called to transfer color configuration to operating dialogs

void setColors()
{

	Fl::background( progStatus.bg_sys_red, progStatus.bg_sys_green, progStatus.bg_sys_blue );
	Fl::background2( progStatus.bg2_sys_red, progStatus.bg2_sys_green, progStatus.bg2_sys_blue );
	Fl::foreground( progStatus.fg_sys_red, progStatus.fg_sys_green, progStatus.fg_sys_blue );

	FreqDispA->font(selfont);
	FreqDispB->font(selfont);

	Fl_Color bgclr = fl_rgb_color (progStatus.bg_red, progStatus.bg_green, progStatus.bg_blue);
	Fl_Color fgclr = fl_rgb_color (progStatus.fg_red, progStatus.fg_green, progStatus.fg_blue);

	if (selrig->inuse == onB) {
		FreqDispB->SetCOLORS( fl_rgb_color(progStatus.fg_red, progStatus.fg_green, progStatus.fg_blue), bgclr);
		FreqDispA->SetCOLORS(
			fl_rgb_color(progStatus.fg_red, progStatus.fg_green, progStatus.fg_blue),
			fl_color_average(bgclr, FL_BLACK, 0.87));
	} else {
		FreqDispA->SetCOLORS( fl_rgb_color(progStatus.fg_red, progStatus.fg_green, progStatus.fg_blue), bgclr);
		FreqDispB->SetCOLORS(
			fl_rgb_color(progStatus.fg_red, progStatus.fg_green, progStatus.fg_blue),
			fl_color_average(bgclr, FL_BLACK, 0.87));
	}

	grpMeters->color(bgclr);

	meter_fill_box->color(bgclr);

	meters_dialog->color(bgclr);

	scaleSmeter->color(bgclr);
	scaleSmeter->labelcolor(fgclr);
	scaleSmeter->redraw_label();
	scaleSmeter->redraw();

	mtr_SMETER->color(bgclr);
	mtr_SMETER->labelcolor(fgclr);
	mtr_SMETER->redraw();
	mtr_SMETER->redraw_label();

	scalePower->color(bgclr);
	scalePower->labelcolor(fgclr);
	scalePower->redraw_label();
	scalePower->redraw();

	mtr_PWR->color(bgclr);
	mtr_PWR->labelcolor(fgclr);
	mtr_PWR->redraw_label();
	mtr_PWR->redraw();

	scaleVoltage->color(bgclr);
	scaleVoltage->labelcolor(fgclr);
	scaleVoltage->redraw_label();
	scaleVoltage->redraw();

	mtr_VOLTS->color(bgclr);
	mtr_VOLTS->labelcolor(fgclr);
	mtr_VOLTS->redraw_label();
	mtr_VOLTS->redraw();

	btnALC_IDD_SWR->color(bgclr);
	btnALC_IDD_SWR->labelcolor(fgclr);
	btnALC_IDD_SWR->redraw_label();
	btnALC_IDD_SWR->redraw();

	mtr_SWR->color(bgclr);
	mtr_SWR->labelcolor(fgclr);
	mtr_SWR->redraw_label();
	mtr_SWR->redraw();

	mtr_ALC->color(bgclr);
	mtr_ALC->labelcolor(fgclr);
	mtr_ALC->redraw_label();
	mtr_ALC->redraw();

	mtr_IDD->color(bgclr);
	mtr_IDD->labelcolor(fgclr);
	mtr_IDD->redraw_label();
	mtr_IDD->redraw();

	sldrRcvSignal->color(fl_rgb_color (progStatus.smeterRed, progStatus.smeterGreen, progStatus.smeterBlue), bgclr);
	sldrRcvSignal->PeakColor(fl_rgb_color(progStatus.smeter_peak_red, progStatus.smeter_peak_green, progStatus.smeter_peak_blue));
	sldrRcvSignal->redraw();

	sldrFwdPwr->color(fl_rgb_color (progStatus.pwrRed, progStatus.pwrGreen, progStatus.pwrBlue), bgclr);
	sldrFwdPwr->PeakColor(fl_rgb_color(progStatus.pwr_peak_red, progStatus.pwr_peak_green, progStatus.pwr_peak_blue));
	sldrFwdPwr->redraw();

	sldrSWR->color(fl_rgb_color (progStatus.swrRed, progStatus.swrGreen, progStatus.swrBlue), bgclr);
	sldrSWR->PeakColor(fl_rgb_color(progStatus.swr_peak_red, progStatus.swr_peak_green, progStatus.swr_peak_blue));
	sldrSWR->redraw();

	sldrALC->color(fl_rgb_color (progStatus.alcRed, progStatus.alcGreen, progStatus.alcBlue), bgclr);
	sldrALC->PeakColor(fl_rgb_color(progStatus.alc_peak_red, progStatus.alc_peak_green, progStatus.alc_peak_blue));
	sldrALC->redraw();

	sldrIDD->color(fl_rgb_color (progStatus.iddRed, progStatus.iddGreen, progStatus.iddBlue), bgclr);
	sldrIDD->PeakColor(fl_rgb_color(progStatus.idd_peak_red, progStatus.idd_peak_green, progStatus.idd_peak_blue));
	sldrIDD->redraw();

	sldrVoltage->color(fl_rgb_color (progStatus.voltsRed, progStatus.voltsGreen, progStatus.voltsBlue), bgclr);
	sldrVoltage->PeakColor(fl_rgb_color(progStatus.volts_peak_red, progStatus.volts_peak_green, progStatus.volts_peak_blue));
	sldrVoltage->redraw();

	grpMeters->redraw();

	sigbar_SMETER->color(fl_rgb_color (progStatus.smeterRed, progStatus.smeterGreen, progStatus.smeterBlue), bgclr);
	sigbar_SMETER->PeakColor(fl_rgb_color(progStatus.smeter_peak_red, progStatus.smeter_peak_green, progStatus.smeter_peak_blue));
	sigbar_SMETER->redraw();

	sigbar_PWR->color(fl_rgb_color (progStatus.pwrRed, progStatus.pwrGreen, progStatus.pwrBlue), bgclr);
	sigbar_PWR->PeakColor(fl_rgb_color(progStatus.pwr_peak_red, progStatus.pwr_peak_green, progStatus.pwr_peak_blue));
	sigbar_PWR->redraw();

	sigbar_SWR->color(fl_rgb_color (progStatus.swrRed, progStatus.swrGreen, progStatus.swrBlue), bgclr);
	sigbar_SWR->PeakColor(fl_rgb_color(progStatus.swr_peak_red, progStatus.swr_peak_green, progStatus.swr_peak_blue));
	sigbar_SWR->redraw();

	sigbar_ALC->color(fl_rgb_color (progStatus.alcRed, progStatus.alcGreen, progStatus.alcBlue), bgclr);
	sigbar_ALC->PeakColor(fl_rgb_color(progStatus.alc_peak_red, progStatus.alc_peak_green, progStatus.alc_peak_blue));
	sigbar_ALC->redraw();

	sigbar_IDD->color(fl_rgb_color (progStatus.iddRed, progStatus.iddGreen, progStatus.iddBlue), bgclr);
	sigbar_IDD->PeakColor(fl_rgb_color(progStatus.idd_peak_red, progStatus.idd_peak_green, progStatus.idd_peak_blue));
	sigbar_IDD->redraw();

	sigbar_VOLTS->color(fl_rgb_color (progStatus.voltsRed, progStatus.voltsGreen, progStatus.voltsBlue), bgclr);
	sigbar_VOLTS->PeakColor(fl_rgb_color(progStatus.volts_peak_red, progStatus.volts_peak_green, progStatus.volts_peak_blue));
	sigbar_VOLTS->redraw();

	set_tab_colors();

	Fl_Color btn_lt_color = fl_rgb_color( progStatus.lighted_btn_red, progStatus.lighted_btn_green, progStatus.lighted_btn_blue);

	if (btnVol)				btnVol->selection_color(btn_lt_color);
	if (btnNR)				btnNR->selection_color(btn_lt_color);
	if (btnIFsh)			btnIFsh->selection_color(btn_lt_color);
	if (btnNotch)			btnNotch->selection_color(btn_lt_color);
	if (btnA)				btnA->selection_color(btn_lt_color);
	if (btnB)				btnB->selection_color(btn_lt_color);
	if (btnSplit)			btnSplit->selection_color(btn_lt_color);
	if (btnAttenuator)		btnAttenuator->selection_color(btn_lt_color);
	if (btnPreamp)			btnPreamp->selection_color(btn_lt_color);
	if (btnNOISE)			btnNOISE->selection_color(btn_lt_color);
	if (btnAutoNotch)		btnAutoNotch->selection_color(btn_lt_color);
	if (btnTune)			btnTune->selection_color(btn_lt_color);
	if (btn_tune_on_off)	btn_tune_on_off->selection_color(btn_lt_color);
	if (btnPTT)				btnPTT->selection_color(btn_lt_color);
	if (btnLOCK)			btnLOCK->selection_color(btn_lt_color);
	if (btnAuxRTS)			btnAuxRTS->selection_color(btn_lt_color);
	if (btnAuxDTR)			btnAuxDTR->selection_color(btn_lt_color);
	if (btnSpot)			btnSpot->selection_color(btn_lt_color);
	if (btn_vox)			btn_vox->selection_color(btn_lt_color);
	if (btnCompON)			btnCompON->selection_color(btn_lt_color);
	if (btnPOWER)			btnPOWER->selection_color(btn_lt_color);

	if (btn_tt550_vox)		btn_tt550_vox->selection_color(btn_lt_color);
	if (btn_tt550_CompON)	btn_tt550_CompON->selection_color(btn_lt_color);

	Fl_Color bg_slider = fl_rgb_color(progStatus.slider_red, progStatus.slider_green, progStatus.slider_blue);
	Fl_Color btn_slider = fl_rgb_color(progStatus.slider_btn_red, progStatus.slider_btn_green, progStatus.slider_btn_blue);

	if (sldrVOLUME)			sldrVOLUME->color(bg_slider);
	if (sldrVOLUME)			sldrVOLUME->selection_color(btn_slider);
	if (sldrRFGAIN)			sldrRFGAIN->color(bg_slider);
	if (sldrRFGAIN)			sldrRFGAIN->selection_color(btn_slider);
	if (sldrSQUELCH)		sldrSQUELCH->color(bg_slider);
	if (sldrSQUELCH)		sldrSQUELCH->selection_color(btn_slider);
	if (sldrNR)				sldrNR->color(bg_slider);
	if (sldrNR)				sldrNR->selection_color(btn_slider);
	if (sldrIFSHIFT)		sldrIFSHIFT->color(bg_slider);
	if (sldrIFSHIFT)		sldrIFSHIFT->selection_color(btn_slider);
	if (sldrINNER)			sldrINNER->color(bg_slider);
	if (sldrINNER)			sldrINNER->selection_color(btn_slider);
	if (sldrOUTER)			sldrOUTER->color(bg_slider);
	if (sldrOUTER)			sldrOUTER->selection_color(btn_slider);
	if (sldrNOTCH)			sldrNOTCH->color(bg_slider);
	if (sldrNOTCH)			sldrNOTCH->selection_color(btn_slider);
	if (sldrMICGAIN)		sldrMICGAIN->color(bg_slider);
	if (sldrMICGAIN)		sldrMICGAIN->selection_color(btn_slider);
	if (sldrPOWER)			sldrPOWER->color(bg_slider);
	if (sldrPOWER)			sldrPOWER->selection_color(btn_slider);

	if (spnrPOWER)			spnrPOWER->color(bg_slider);
	if (spnrVOLUME)			spnrVOLUME->color(bg_slider);

	setDisplayColors();
	update_memory_dialog_colors();

	redraw_dialogs();
}

void update_colors()
{
	progStatus.fg_red = fg_red;
	progStatus.fg_green = fg_green;
	progStatus.fg_blue = fg_blue;

	progStatus.bg_red = bg_red;
	progStatus.bg_green = bg_green;
	progStatus.bg_blue = bg_blue;

	progStatus.swrRed = swrRed;
	progStatus.swrGreen = swrGreen;
	progStatus.swrBlue = swrBlue;

	progStatus.pwrRed = pwrRed;
	progStatus.pwrGreen = pwrGreen;
	progStatus.pwrBlue = pwrBlue;

	progStatus.smeterRed = smeterRed;
	progStatus.smeterGreen = smeterGreen;
	progStatus.smeterBlue = smeterBlue;

	progStatus.alcRed = alcRed;
	progStatus.alcGreen = alcGreen;
	progStatus.alcBlue = alcBlue;

	progStatus.iddRed = iddRed;
	progStatus.iddGreen = iddGreen;
	progStatus.iddBlue = iddBlue;

	progStatus.voltsRed = voltsRed;
	progStatus.voltsGreen = voltsGreen;
	progStatus.voltsBlue = voltsBlue;

	progStatus.smeter_peak_red = smeter_peak_red;
	progStatus.smeter_peak_green = smeter_peak_green;
	progStatus.smeter_peak_blue = smeter_peak_blue;

	progStatus.alc_peak_red = alc_peak_red;
	progStatus.alc_peak_green = alc_peak_green;
	progStatus.alc_peak_blue = alc_peak_blue;

	progStatus.swr_peak_red = swr_peak_red;
	progStatus.swr_peak_green = swr_peak_green;
	progStatus.swr_peak_blue = swr_peak_blue;

	progStatus.pwr_peak_red = pwr_peak_red;
	progStatus.pwr_peak_green = pwr_peak_green;
	progStatus.pwr_peak_blue = pwr_peak_blue;

	progStatus.idd_peak_red = idd_peak_red;
	progStatus.idd_peak_green = idd_peak_green;
	progStatus.idd_peak_blue = idd_peak_blue;

	progStatus.volts_peak_red = volts_peak_red;
	progStatus.volts_peak_green = volts_peak_green;
	progStatus.volts_peak_blue = volts_peak_blue;

	progStatus.fontnbr = selfont;

	progStatus.fg_sys_red = fg_sys_red;
	progStatus.fg_sys_green = fg_sys_green;
	progStatus.fg_sys_blue = fg_sys_blue;

	progStatus.bg_sys_red = bg_sys_red;
	progStatus.bg_sys_green = bg_sys_green;
	progStatus.bg_sys_blue = bg_sys_blue;

	progStatus.bg2_sys_red = bg2_sys_red;
	progStatus.bg2_sys_green = bg2_sys_green;
	progStatus.bg2_sys_blue = bg2_sys_blue;

	progStatus.slider_red = bg_slider_red;
	progStatus.slider_green = bg_slider_green;
	progStatus.slider_blue = bg_slider_blue;

	progStatus.slider_btn_red = btn_slider_red;
	progStatus.slider_btn_green = btn_slider_green;
	progStatus.slider_btn_blue = btn_slider_blue;

	progStatus.lighted_btn_red = btn_lt_color_red;
	progStatus.lighted_btn_green = btn_lt_color_green;
	progStatus.lighted_btn_blue = btn_lt_color_blue;

	progStatus.tab_red = tab_red;
	progStatus.tab_green = tab_green;
	progStatus.tab_blue = tab_blue;

	setColors();

}


void cb_ResetColorsDialog()
{
	cb_sys_defaults();
	cb_lighted_default();
	cb_slider_defaults();
	default_meters();
	cb_tab_defaults();
	default_freq_display();
}

void cbOKColorDialog()
{
	update_colors();
	setColors();
	dlgColorsDialog->hide();
}

void cbOKDisplayDialog()
{
	dlgDisplayConfig->hide();
}

void cbCancelColorDialog()
{
	dlgColorsDialog->hide();
}

void setUIscheme()
{
	mnuScheme->value(mnuScheme->find_item(progStatus.ui_scheme.c_str()));
#if FLRIG_FLTK_API_MINOR >3
	dlgDisplayConfig->screen_num( mainwindow->screen_num() );
#endif
	dlgDisplayConfig->show();
}

// called when opening the configure colors dialog

void setDisplayColors()
{
	if (dlgDisplayConfig == NULL || dlgColorsDialog == NULL)
		return;

	swrRed = progStatus.swrRed;
	swrGreen = progStatus.swrGreen;
	swrBlue = progStatus.swrBlue;

	pwrRed = progStatus.pwrRed;
	pwrGreen = progStatus.pwrGreen;
	pwrBlue = progStatus.pwrBlue;

	smeterRed = progStatus.smeterRed;
	smeterGreen = progStatus.smeterGreen;
	smeterBlue = progStatus.smeterBlue;

	smeter_peak_red = progStatus.smeter_peak_red;
	smeter_peak_green = progStatus.smeter_peak_green;
	smeter_peak_blue = progStatus.smeter_peak_blue;

	iddRed = progStatus.iddRed;
	iddGreen = progStatus.iddGreen;
	iddBlue = progStatus.iddBlue;

	alcRed = progStatus.alcRed;
	alcGreen = progStatus.alcGreen;
	alcBlue = progStatus.alcBlue;

	voltsRed = progStatus.voltsRed;
	voltsGreen = progStatus.voltsGreen;
	voltsBlue = progStatus.voltsBlue;

	fg_red = progStatus.fg_red;
	fg_green = progStatus.fg_green;
	fg_blue = progStatus.fg_blue;

	bg_red = progStatus.bg_red;
	bg_green = progStatus.bg_green;
	bg_blue = progStatus.bg_blue;

	bgclr = fl_rgb_color(bg_red, bg_green, bg_blue);
	fgclr = fl_rgb_color(fg_red, fg_green, fg_blue);

	fg_sys_red = progStatus.fg_sys_red;
	fg_sys_green = progStatus.fg_sys_green;
	fg_sys_blue = progStatus.fg_sys_blue;

	bg_sys_red = progStatus.bg_sys_red;
	bg_sys_green = progStatus.bg_sys_green;
	bg_sys_blue = progStatus.bg_sys_blue;

	bg2_sys_red = progStatus.bg2_sys_red;
	bg2_sys_green = progStatus.bg2_sys_green;
	bg2_sys_blue = progStatus.bg2_sys_blue;

	bg_slider_red = progStatus.slider_red;
	bg_slider_green = progStatus.slider_green;
	bg_slider_blue = progStatus.slider_blue;

	btn_slider_red = progStatus.slider_btn_red;
	btn_slider_green = progStatus.slider_btn_green;
	btn_slider_blue = progStatus.slider_btn_blue;

	tab_red = progStatus.tab_red;
	tab_green = progStatus.tab_green;
	tab_blue = progStatus.tab_blue;

	sldrColors->color(fl_rgb_color(bg_slider_red, bg_slider_green, bg_slider_blue));
	sldrColors->selection_color(fl_rgb_color(btn_slider_red, btn_slider_green, btn_slider_blue));

	btn_lt_color_red = progStatus.lighted_btn_red;
	btn_lt_color_green = progStatus.lighted_btn_green;
	btn_lt_color_blue = progStatus.lighted_btn_blue;

	lblTest->labelcolor(fl_rgb_color(fg_red, fg_green, fg_blue));
	lblTest->color(bgclr);

	btn_change_SMETER->color(bgclr);
	btn_change_SMETER->labelcolor(fgclr);
	btn_change_PWR->color(bgclr);
	btn_change_PWR->labelcolor(fgclr);
	btn_change_SWR->color(bgclr);
	btn_change_SWR->labelcolor(fgclr);
	btn_change_VOLTS->color(bgclr);
	btn_change_VOLTS->labelcolor(fgclr);
	btn_change_IDD->color(bgclr);
	btn_change_IDD->labelcolor(fgclr);
	btn_change_ALC->color(bgclr);
	btn_change_ALC->labelcolor(fgclr);

	meter_group->color(bgclr);
	meter_group->labelcolor(fgclr);

	color_SMETER->color(
		fl_rgb_color (smeterRed, smeterGreen, smeterBlue),
		bgclr );
	color_PWR->color(
		fl_rgb_color (pwrRed, pwrGreen, pwrBlue),
		bgclr );
	color_SWR->color(
		fl_rgb_color (swrRed, swrGreen, swrBlue),
		bgclr );
	color_ALC->color(
		fl_rgb_color (alcRed, alcGreen, alcBlue),
		bgclr );
	color_IDD->color(
		fl_rgb_color (iddRed, iddGreen, iddBlue),
		bgclr );
	color_VOLTS->color(
		fl_rgb_color (voltsRed, voltsGreen, voltsBlue),
		bgclr );

	color_SMETER->minimum(0);
	color_SMETER->maximum(100);
	color_SMETER->value(45);

	color_PWR->minimum(0);
	color_PWR->maximum(100);
	color_PWR->value(35);

	color_SWR->minimum(0);
	color_SWR->maximum(100);
	color_SWR->value(25);

	color_VOLTS->minimum(0);
	color_VOLTS->maximum(16);
	color_VOLTS->value(12);

	color_ALC->minimum(0);
	color_ALC->maximum(5);
	color_ALC->value(1);

	color_IDD->minimum(0);
	color_IDD->maximum(25);
	color_IDD->value(14);

	btn_lt_color = fl_rgb_color( btn_lt_color_red, btn_lt_color_green, btn_lt_color_blue);
	btn_slider = fl_rgb_color( btn_slider_red, btn_slider_green, btn_slider_blue);
	bg_slider = fl_rgb_color(bg_slider_red, bg_slider_green, bg_slider_blue);

	btn_lighted->value(1);
	btn_lighted->selection_color(btn_lt_color);

	sldrColors->color(bg_slider);
	sldrColors->selection_color(btn_slider);

	tabs_color->selection_color( fl_rgb_color( tab_red, tab_green, tab_blue ) );

}

void open_colors_dialog() {
	setDisplayColors();
#if FLRIG_FLTK_API_MINOR >3
	dlgColorsDialog->screen_num( mainwindow->screen_num() );
#endif
	dlgColorsDialog->show();
}

void cbCloseMemory()
{
	dlgMemoryDialog->hide();
}

void openMemoryDialog()
{
	if (dlgMemoryDialog == NULL)
		return;
#if FLRIG_FLTK_API_MINOR >3
	dlgMemoryDialog->screen_num( mainwindow->screen_num() );
#endif
	dlgMemoryDialog->show();
}


void showme(char ch)
{
	return;
//	std::cout << "Schema " << ch << std::endl;
//	std::cout << "Fl::h() : " << Fl::h() << std::endl;
//	std::cout << " X: " << mainwindow->x() << ",  Y: " << mainwindow->y() << ",  W: " << mainwindow->w() << ",  H: " << mainwindow->h() << std::endl;
//	std::cout << "sX:" << progStatus.mainX << ", sY: " << progStatus.mainY << ", sW: " << progStatus.mainW << ", sH: " << progStatus.mainH << std::endl;
}

static bool restore_position = false;

void show_controls()
{
	Fl_Widget * vtab = (Fl_Widget *)0;
	if (tabsGeneric && !progStatus.visible_tab.empty()) {
		Fl_Widget * const *vtabs = tabsGeneric->array();
		int ntabs = tabsGeneric->children();
		for (int n = 0; n < ntabs; n++) {
			if (progStatus.visible_tab == vtabs[n]->label()) {
				vtab = vtabs[n];
			}
			vtabs[n]->redraw();
		}
	}

	int X = mainwindow->x(),
		Y = mainwindow->y(),
		W = mainwindow->w(),
		H = mainwindow->h();

	switch (progStatus.UIsize) {
		case touch_ui : {
			if (selrig->name_ == rig_TT550.name_) {
				tabs550->show();
				tabsGeneric->hide();
				tabs550->redraw();
			} else {
				tabs550->hide();
				tabsGeneric->show();
				tabsGeneric->redraw();
			}
			mainwindow->redraw();
			break;
		}
		case wide_ui : {
			if (selrig->name_ == rig_TT550.name_) {
				tabsGeneric->hide();
				tabs550->show();
				tabs550->redraw();
			} else {
				tabs550->hide();
				tabsGeneric->show();
				tabsGeneric->redraw();
			}
			if (progStatus.embed_tabs) { // embedded
				if ((progStatus.show_tabs && !progStatus.first_use) ||
					(!progStatus.show_tabs && progStatus.first_use)) {

					H = WIDE_MAINH + WIDE_MENUH;
					mainwindow->resize( X, Y, W, H );
					mainwindow->size_range(WIDE_MAINW, H, 0, H);

					tabs->hide();
					progStatus.show_tabs = false;
					mainwindow->redraw();
				} else {
					H = WIDE_MENUH + WIDE_MAINH + WIDE_TABSH;

					mainwindow->resize( X, Y, W, H);
					mainwindow->size_range(WIDE_MAINW, H, 0, H);

					tabs->add(grpTABS);
					grpTABS->resize(
						tabs->x(), tabs->y(),
						tabs->w(), tabs->h());
					grpTABS->show();
					tabs->show();
					progStatus.show_tabs = true;

					mainwindow->redraw();
				}
			} else {
				if ((progStatus.show_tabs && !progStatus.first_use) || (!progStatus.show_tabs && progStatus.first_use)) {

					tabs_dialog->hide();

					progStatus.show_tabs = false;

				} else if ((!progStatus.show_tabs && !progStatus.first_use) || (progStatus.show_tabs && progStatus.first_use)) {

					static int X, Y, W, H, dH;
					X = mainwindow->x();
					Y = mainwindow->y();
					W = mainwindow->w();
					H = WIDE_MAINH + WIDE_MENUH;

					dH = mainwindow->decorated_h();
					tabs_dialog->resize( X, Y + dH, W, tabs_dialog->h() );
					grpTABS->resize(0, 0, W, tabs_dialog->h());

					tabs_dialog->add(grpTABS);
					tabs_dialog->show();
					tabs_dialog->redraw();
					progStatus.show_tabs = true;

					mainwindow->resize( X, Y, W, H);
					mainwindow->size_range(WIDE_MAINW, H, 0, H);

					mainwindow->redraw();
				}
			}
			break;
		}
		case small_ui : {
			if (selrig->name_ == rig_TT550.name_) {
				tabsGeneric->hide();
				tabs550->show();
				tabs550->redraw();
				break;
			} else {
				tabs550->hide();
				tabsGeneric->show();
				tabsGeneric->redraw();
			}
			if (progStatus.embed_tabs) { // embedded
				if (progStatus.show_tabs) {
showme('A');
					H -= grpTABS->h();
					mainwindow->size(W, H);
					mainwindow->size_range( W, H, W, H);

					tabs_dialog->add(grpTABS);

					if (restore_position) {
						mainwindow->position(X, Y + grpTABS->h());
						restore_position = false;
					} else {
						mainwindow->position(X, Y);
					}
					progStatus.show_tabs = false;

				} else {
showme('B');
					mainwindow->add(grpTABS);
					grpTABS->position(0, H);
					grpTABS->show();

					H += grpTABS->h();
					mainwindow->size(W, H);
					mainwindow->size_range( W, H, W, H);

					if (mainwindow->y() + mainwindow->decorated_h() >= Fl::h()) {
						mainwindow->position(X, Y - grpTABS->h());
						restore_position = true;
					} else {
						mainwindow->position(X, Y);
					}
					progStatus.show_tabs = true;
				}
			} else {
				if (progStatus.first_use) {
					X = progStatus.mainX;
					Y = progStatus.mainY;
				}
				if (progStatus.show_tabs) {
					tabs_dialog->hide();
showme('C');
					mainwindow->resize( X, Y, W, H);
					mainwindow->size_range( W, H, W, H );
					progStatus.show_tabs = false;

				} else {
					int dH = mainwindow->decorated_h();
showme('D');
#if FLRIG_FLTK_API_MINOR >3
	tabs_dialog->screen_num( mainwindow->screen_num() );
#endif
					if ((Y + dH + tabs_dialog->h()) > Fl::h()) {
						tabs_dialog->resize( X, Y + mainwindow->h() - mainwindow->decorated_h() - tabs_dialog->decorated_h(), W, tabs_dialog->h() );
					} else {
						tabs_dialog->resize( X, Y + dH, W, tabs_dialog->h() );
					}
					grpTABS->resize(0, 0, W, tabs_dialog->h());
					if (vtab != (Fl_Widget *)0) tabsGeneric->value(vtab);
					tabs_dialog->add(grpTABS);
					tabs_dialog->show();
					tabs_dialog->redraw();
					progStatus.show_tabs = true;
					mainwindow->resize(X, Y, W, H);
					int sH = mainwindow->h();
					mainwindow->size_range( W, sH, W, sH);
				}
			}

			break;
		}
		default :
			break;
	}

	if (tabsGeneric) {
		if (vtab != (Fl_Widget *)0)
			tabsGeneric->value(vtab);
		else
			progStatus.visible_tab = (tabsGeneric->value())->label();
	}

	progStatus.first_use = false;
}

// a replica of the default color map used by Fltk

static unsigned flrig_cmap[256] = {
	0x00000000,
	0xff000000,
	0x00ff0000,
	0xffff0000,
	0x0000ff00,
	0xff00ff00,
	0x00ffff00,
	0xffffff00,
	0x55555500,
	0xc6717100,
	0x71c67100,
	0x8e8e3800,
	0x7171c600,
	0x8e388e00,
	0x388e8e00,
	0x00008000,
	0xa8a89800,
	0xe8e8d800,
	0x68685800,
	0x98a8a800,
	0xd8e8e800,
	0x58686800,
	0x9c9ca800,
	0xdcdce800,
	0x5c5c6800,
	0x9ca89c00,
	0xdce8dc00,
	0x5c685c00,
	0x90909000,
	0xc0c0c000,
	0x50505000,
	0xa0a0a000,
	0x00000000,
	0x0d0d0d00,
	0x1a1a1a00,
	0x26262600,
	0x31313100,
	0x3d3d3d00,
	0x48484800,
	0x55555500,
	0x5f5f5f00,
	0x6a6a6a00,
	0x75757500,
	0x80808000,
	0x8a8a8a00,
	0x95959500,
	0xa0a0a000,
	0xaaaaaa00,
	0xb5b5b500,
	0xc0c0c000,
	0xcbcbcb00,
	0xd5d5d500,
	0xe0e0e000,
	0xeaeaea00,
	0xf5f5f500,
	0xffffff00,
	0x00000000,
	0x00240000,
	0x00480000,
	0x006d0000,
	0x00910000,
	0x00b60000,
	0x00da0000,
	0x00ff0000,
	0x3f000000,
	0x3f240000,
	0x3f480000,
	0x3f6d0000,
	0x3f910000,
	0x3fb60000,
	0x3fda0000,
	0x3fff0000,
	0x7f000000,
	0x7f240000,
	0x7f480000,
	0x7f6d0000,
	0x7f910000,
	0x7fb60000,
	0x7fda0000,
	0x7fff0000,
	0xbf000000,
	0xbf240000,
	0xbf480000,
	0xbf6d0000,
	0xbf910000,
	0xbfb60000,
	0xbfda0000,
	0xbfff0000,
	0xff000000,
	0xff240000,
	0xff480000,
	0xff6d0000,
	0xff910000,
	0xffb60000,
	0xffda0000,
	0xffff0000,
	0x00003f00,
	0x00243f00,
	0x00483f00,
	0x006d3f00,
	0x00913f00,
	0x00b63f00,
	0x00da3f00,
	0x00ff3f00,
	0x3f003f00,
	0x3f243f00,
	0x3f483f00,
	0x3f6d3f00,
	0x3f913f00,
	0x3fb63f00,
	0x3fda3f00,
	0x3fff3f00,
	0x7f003f00,
	0x7f243f00,
	0x7f483f00,
	0x7f6d3f00,
	0x7f913f00,
	0x7fb63f00,
	0x7fda3f00,
	0x7fff3f00,
	0xbf003f00,
	0xbf243f00,
	0xbf483f00,
	0xbf6d3f00,
	0xbf913f00,
	0xbfb63f00,
	0xbfda3f00,
	0xbfff3f00,
	0xff003f00,
	0xff243f00,
	0xff483f00,
	0xff6d3f00,
	0xff913f00,
	0xffb63f00,
	0xffda3f00,
	0xffff3f00,
	0x00007f00,
	0x00247f00,
	0x00487f00,
	0x006d7f00,
	0x00917f00,
	0x00b67f00,
	0x00da7f00,
	0x00ff7f00,
	0x3f007f00,
	0x3f247f00,
	0x3f487f00,
	0x3f6d7f00,
	0x3f917f00,
	0x3fb67f00,
	0x3fda7f00,
	0x3fff7f00,
	0x7f007f00,
	0x7f247f00,
	0x7f487f00,
	0x7f6d7f00,
	0x7f917f00,
	0x7fb67f00,
	0x7fda7f00,
	0x7fff7f00,
	0xbf007f00,
	0xbf247f00,
	0xbf487f00,
	0xbf6d7f00,
	0xbf917f00,
	0xbfb67f00,
	0xbfda7f00,
	0xbfff7f00,
	0xff007f00,
	0xff247f00,
	0xff487f00,
	0xff6d7f00,
	0xff917f00,
	0xffb67f00,
	0xffda7f00,
	0xffff7f00,
	0x0000bf00,
	0x0024bf00,
	0x0048bf00,
	0x006dbf00,
	0x0091bf00,
	0x00b6bf00,
	0x00dabf00,
	0x00ffbf00,
	0x3f00bf00,
	0x3f24bf00,
	0x3f48bf00,
	0x3f6dbf00,
	0x3f91bf00,
	0x3fb6bf00,
	0x3fdabf00,
	0x3fffbf00,
	0x7f00bf00,
	0x7f24bf00,
	0x7f48bf00,
	0x7f6dbf00,
	0x7f91bf00,
	0x7fb6bf00,
	0x7fdabf00,
	0x7fffbf00,
	0xbf00bf00,
	0xbf24bf00,
	0xbf48bf00,
	0xbf6dbf00,
	0xbf91bf00,
	0xbfb6bf00,
	0xbfdabf00,
	0xbfffbf00,
	0xff00bf00,
	0xff24bf00,
	0xff48bf00,
	0xff6dbf00,
	0xff91bf00,
	0xffb6bf00,
	0xffdabf00,
	0xffffbf00,
	0x0000ff00,
	0x0024ff00,
	0x0048ff00,
	0x006dff00,
	0x0091ff00,
	0x00b6ff00,
	0x00daff00,
	0x00ffff00,
	0x3f00ff00,
	0x3f24ff00,
	0x3f48ff00,
	0x3f6dff00,
	0x3f91ff00,
	0x3fb6ff00,
	0x3fdaff00,
	0x3fffff00,
	0x7f00ff00,
	0x7f24ff00,
	0x7f48ff00,
	0x7f6dff00,
	0x7f91ff00,
	0x7fb6ff00,
	0x7fdaff00,
	0x7fffff00,
	0xbf00ff00,
	0xbf24ff00,
	0xbf48ff00,
	0xbf6dff00,
	0xbf91ff00,
	0xbfb6ff00,
	0xbfdaff00,
	0xbfffff00,
	0xff00ff00,
	0xff24ff00,
	0xff48ff00,
	0xff6dff00,
	0xff91ff00,
	0xffb6ff00,
	0xffdaff00,
	0xffffff00
};

Fl_Color flrig_def_color(int n)
{
	if ( n > 255 ) n = 255;
	if (n < 0) n = 0;
	return (Fl_Color)flrig_cmap[n];
}

extern std::string print_all();

void cb_send_command(std::string command, Fl_Output *resp, bool expect)
{
	if (command == "PRINT") {
		fl_alert2("%s", print_all().c_str());
//#ifndef __WIN32__
//		std::cout << print_all();
//#endif
		return;
	}
	if (command.empty()) return;
	bool usehex = false;

	std::string cmd = "";
	if (command.find("x") != std::string::npos) { // hex std::strings
		size_t p = 0;
		usehex = true;
		unsigned int val;
		while (( p = command.find("x", p)) != std::string::npos) {
			sscanf(&command[p+1], "%x", &val);
			cmd += (unsigned char) val;
			p += 3;
		}
	} else
		cmd = command;

	if (resp) {
		resp->value("");
		resp->redraw();
	}

// lock out polling loops until done
//	guard_lock lock1(&mutex_srvc_reqs, std::string(__func__) );

	guard_lock lock2(&mutex_serial, std::string(__func__) );

	selrig->cmd = cmd;

	test_trace(3, __func__, " cmd: ", str2hex(selrig->cmd.c_str(), selrig->cmd.length()));

	if (expect) {
		if (selrig->read_N > 0) {
			selrig->waitN( selrig->read_N, 100, __func__, HEX );
			respstr = selrig->replystr;
		}
		else {
			sendCommand(cmd);
			waitResponse(progStatus.serial_timeout);
		}
		std::string retstr = usehex ?
			str2hex(respstr.c_str(), respstr.length()) :
			respstr;

		test_trace(2, "response: ", retstr.c_str());

		if (resp) {
			resp->value(retstr.c_str());
			resp->redraw();
		}
	} else
		sendCommand(cmd);

}

// =====================================================================
// logbook support code
// =====================================================================
bool cwlog_editing = false;
int  cwlog_edit_nbr = 0;

bool cwlog_changed = false;
bool cwlog_is_open = false;

void cwlog_set_edit(bool on)
{
	cwlog_editing = on;
	if (on) {
		btn_cwlog_edit_entry->label("Delete");
		btn_cwlog_edit_entry->redraw_label();
		btn_cwlog_clear_qso->label("Cancel");
		btn_cwlog_clear_qso->redraw_label();
	} else {
		btn_cwlog_edit_entry->label("Edit");
		btn_cwlog_edit_entry->redraw_label();
		btn_cwlog_clear_qso->label("Clear");
		btn_cwlog_clear_qso->redraw_label();
	}
}

bool cwlog_compare( int &dir, int fld, std::string &s1, std::string &s2) {
	size_t p1 = 0, p2 = 0;
	for (int n = 0; n < fld; n++) {
		p1 = s1.find('\t', p1 + 1);
		p2 = s2.find('\t', p2 + 1);
	}
	if (dir == 1) return (s2.substr(p2) < s1.substr(p1));
	return (s2.substr(p2) > s1.substr(p1));
}

bool cwlog_freq_compare( int &dir, int fld, std::string &s1, std::string &s2) {
	size_t p1 = 0, p2 = 0;
	for (int n = 0; n < fld; n++) {
		p1 = s1.find('\t', p1 + 1);
		p2 = s2.find('\t', p2 + 1);
	}
	float f1 = atof(s1.substr(p1).c_str());
	float f2 = atof(s2.substr(p2).c_str());

	if (dir == 1) return (f2 < f1);
	return (f2 > f1);
}

static int dtdir = 1;
static int dtpos = 0;
void cwlog_sort_by_datetime() {
	if (cwlog_editing) return;
	size_t nbr = brwsr_cwlog_entries->size();
	if (nbr == 0) return;
	std::string entries[nbr];
	for (size_t n = 0; n < nbr; n++) entries[n] = brwsr_cwlog_entries->text(n+1);
	std::string temp;
	if (nbr > 1) {
		for (size_t n = 0; n < nbr - 1; n++) {
			for (size_t j = n + 1; j < nbr; j++) {
				if (cwlog_compare (dtdir, dtpos, entries[n], entries[j])) {
					temp = entries[j];
					entries[j] = entries[n];
					entries[n] = temp;
				}
			}
		}
	}
	brwsr_cwlog_entries->clear();
	for (size_t i = 0; i < nbr; i++)
		brwsr_cwlog_entries->add(entries[i].c_str());
	brwsr_cwlog_entries->redraw();
	if (dtdir == 1) dtdir = -1;
	else dtdir = 1;
}

static int freqdir = 1;
static int freqpos = 2;
void cwlog_sort_by_freq() {
	if (cwlog_editing) return;
	size_t nbr = brwsr_cwlog_entries->size();
	if (nbr == 0) return;
	std::string entries[nbr];
	for (size_t n = 0; n < nbr; n++) entries[n] = brwsr_cwlog_entries->text(n+1);
	std::string temp;
	if (nbr > 1) {
		for (size_t n = 0; n < nbr - 1; n++) {
			for (size_t j = n + 1; j < nbr; j++) {
				if (cwlog_freq_compare (freqdir, freqpos, entries[n], entries[j])) {
					temp = entries[j];
					entries[j] = entries[n];
					entries[n] = temp;
				}
			}
		}
	}
	brwsr_cwlog_entries->clear();
	for (size_t i = 0; i < nbr; i++)
		brwsr_cwlog_entries->add(entries[i].c_str());
	brwsr_cwlog_entries->redraw();
	if (freqdir == 1) freqdir = -1;
	else freqdir = 1;
}

static int calldir = 1;
static int callpos = 3;
void cwlog_sort_by_call() {
	if (cwlog_editing) return;
	size_t nbr = brwsr_cwlog_entries->size();
	if (nbr == 0) return;
	std::string entries[nbr];
	for (size_t n = 0; n < nbr; n++) entries[n] = brwsr_cwlog_entries->text(n+1);
	std::string temp;
	if (nbr > 1) {
		for (size_t n = 0; n < nbr - 1; n++) {
			for (size_t j = n + 1; j < nbr; j++) {
				if (cwlog_compare (calldir, callpos, entries[n], entries[j])) {
					temp = entries[j];
					entries[j] = entries[n];
					entries[n] = temp;
				}
			}
		}
	}
	brwsr_cwlog_entries->clear();
	for (size_t i = 0; i < nbr; i++)
		brwsr_cwlog_entries->add(entries[i].c_str());
	brwsr_cwlog_entries->redraw();
	if (calldir == 1) calldir = -1;
	else calldir = 1;
}

static int nbrdir = 1;
static int nbrpos = 7;
void cwlog_sort_by_nbr() {
	if (cwlog_editing) return;
	size_t nbr = brwsr_cwlog_entries->size();
	if (nbr == 0) return;
	std::string entries[nbr];
	for (size_t n = 0; n < nbr; n++) entries[n] = brwsr_cwlog_entries->text(n+1);
	std::string temp;
	if (nbr > 1) {
		for (size_t n = 0; n < nbr - 2; n++) {
			for (size_t j = n + 1; j < nbr - 1; j++) {
				if (cwlog_compare (nbrdir, nbrpos, entries[n], entries[j])) {
					temp = entries[j];
					entries[j] = entries[n];
					entries[n] = temp;
				}
			}
		}
	}
	brwsr_cwlog_entries->clear();
	for (size_t i = 0; i < nbr; i++)
		brwsr_cwlog_entries->add(entries[i].c_str());
	brwsr_cwlog_entries->redraw();
	if (nbrdir == 1) nbrdir = -1;
	else nbrdir = 1;
}

void cwlog_clear_qso()
{
	cw_qso_date->value("");
	cw_qso_time->value("");
	cw_freq->value("");
	cw_op_call->value("");
	cw_op_name->value("");
	cw_rst_in->value("");
	cw_rst_out->value("");
	cw_xchg_in->value("");

	if (cwlog_editing)
		cwlog_set_edit(false);

}

void cwlog_save_qso()
{
	char line[256];
	if (cwlog_editing) {
		snprintf(line, sizeof(line),
			"%s\t%s\t%s\t%s\t%s\t%s\t%s\t%05d\t%s",
			cw_qso_date->value(),
			cw_qso_time->value(),
			cw_freq->value(),
			cw_op_call->value(),
			cw_op_name->value(),
			cw_rst_in->value(),
			cw_rst_out->value(),
			(int)cw_log_nbr->value(),
			cw_xchg_in->value());
		brwsr_cwlog_entries->insert(cwlog_edit_nbr, line);
		brwsr_cwlog_entries->remove(cwlog_edit_nbr + 1);
		brwsr_cwlog_entries->select(cwlog_edit_nbr);
		cwlog_set_edit(false);
	} else {
		snprintf(line, sizeof(line),
			"%s\t%s\t%s\t%s\t%s\t%s\t%s\t%05d\t%s",
			cw_qso_date->value(),
			cw_qso_time->value(),
			cw_freq->value(),
			cw_op_call->value(),
			cw_op_name->value(),
			cw_rst_in->value(),
			cw_rst_out->value(),
			(int)cw_log_nbr->value(),
			cw_xchg_in->value());
		brwsr_cwlog_entries->add(line);
	}
	cwlog_changed = true;
}

void cwlog_delete_entry()
{
	brwsr_cwlog_entries->remove(cwlog_edit_nbr);
	brwsr_cwlog_entries->select(cwlog_edit_nbr, false);
	brwsr_cwlog_entries->redraw();
	cwlog_clear_qso();
	cwlog_changed = true;
}

void cwlog_edit_entry()
{
	if (cwlog_editing) {
		cwlog_delete_entry();
		return;
	}

	cwlog_edit_nbr = brwsr_cwlog_entries->value();
	if (!cwlog_edit_nbr) return;

	cwlog_clear_qso();
	size_t ptr = 0;
	std::string entry = brwsr_cwlog_entries->text(cwlog_edit_nbr);

	ptr = entry.find('\t');
	cw_qso_date->value(entry.substr(0, ptr).c_str());
	entry.erase(0, ptr+1);

	ptr = entry.find('\t');
	cw_qso_time->value(entry.substr(0,ptr).c_str());
	entry.erase(0, ptr+1);

	ptr = entry.find('\t');
	cw_freq->value(entry.substr(0,ptr).c_str());
	entry.erase(0, ptr+1);

	ptr = entry.find('\t');
	cw_op_call->value(entry.substr(0,ptr).c_str());
	entry.erase(0, ptr+1);

	ptr = entry.find('\t');
	cw_op_name->value(entry.substr(0,ptr).c_str());
	entry.erase(0, ptr+1);

	ptr = entry.find('\t');
	cw_rst_in->value(entry.substr(0,ptr).c_str());
	entry.erase(0, ptr+1);

	ptr = entry.find('\t');
	cw_rst_out->value(entry.substr(0,ptr).c_str());
	entry.erase(0, ptr+1);

	ptr = entry.find('\t');
	cw_log_nbr->value(atoi(entry.substr(0,ptr).c_str()));
	entry.erase(0, ptr+1);

	ptr = entry.find('\t');
	cw_xchg_in->value(entry.substr(0,ptr).c_str());
	entry.erase(0, ptr+1);

	cwlog_set_edit(true);
}

void cwlog_view()
{
	if (!cwlog_viewer) {
		cwlog_viewer = new_cwlogbook_dialog();
		if (!progStatus.cw_log_name.empty()) {
			txt_cwlog_file->value(progStatus.cw_log_name.c_str());
			cwlog_load();
		} else
			cwlog_open();
	}
#if FLRIG_FLTK_API_MINOR >3
	cwlog_viewer->screen_num( mainwindow->screen_num() );
#endif
	cwlog_viewer->show();
}

void cwlog_save()
{
	if (progStatus.cw_log_name.empty())
		return;
	std::ofstream oLog(progStatus.cw_log_name.c_str());
	if (!oLog) {
		fl_message ("Could not write to %s", progStatus.cw_log_name.c_str());
		return;
	}
	size_t n = brwsr_cwlog_entries->size();
	std::string oline;
	for (size_t i = 1; i <= n; i++) {
		oline = brwsr_cwlog_entries->text(i);
		if (oline.empty()) continue;
		oLog << oline << std::endl;
	}
	oLog.close();
	cwlog_changed = false;
}

void cwlog_load()
{
	std::ifstream iLog(progStatus.cw_log_name.c_str());
	if (!iLog) return;
	brwsr_cwlog_entries->clear();
	char line[256];
	std::string sline;
	while (!iLog.eof()) {
		memset(line, 0, 256);
		iLog.getline(line, 256);
		sline = line;
		if (!sline.empty())
			brwsr_cwlog_entries->add(sline.c_str());
	}
	iLog.close();
	brwsr_cwlog_entries->redraw();
	cwlog_is_open = true;
}

void cwlog_save_as()
{
// Create and post the local native file chooser
	Fl_Native_File_Chooser fnfc;
	fnfc.title("Save As log file");
	fnfc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	fnfc.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM);
	fnfc.filter("CW Log\t*.txt");
// default directory to use
	fnfc.directory(RigHomeDir.c_str());
	fnfc.preset_file(progStatus.cw_log_name.c_str());
// Show native chooser
	switch ( fnfc.show() ) {
		case -1:
			fl_message ("ERROR: %s", fnfc.errmsg());
			return; // ERROR
		case 1:
			return; // CANCEL
		default:
			progStatus.cw_log_name = fnfc.filename();
			txt_cwlog_file->value(progStatus.cw_log_name.c_str());
	}
	cwlog_save();
}

void cwlog_open()
{
	if (cwlog_is_open && cwlog_changed)
		cwlog_save();

// Create and post the local native file chooser
	Fl_Native_File_Chooser fnfc;
	fnfc.title("Select log file");
	fnfc.type(Fl_Native_File_Chooser::BROWSE_FILE);
	fnfc.filter("CW Log\t*.txt");
// default directory to use
	fnfc.directory(RigHomeDir.c_str());
// Show native chooser
	switch ( fnfc.show() ) {
		case -1:
			fl_message ("ERROR: %s", fnfc.errmsg());
			return; // ERROR
		case 1:
			return; // CANCEL
		default:
			progStatus.cw_log_name = fnfc.filename();
			txt_cwlog_file->value(progStatus.cw_log_name.c_str());
			txt_cwlog_file->redraw();
			cwlog_load();
	}
}

void cwlog_new()
{
	if (cwlog_is_open && cwlog_changed)
		cwlog_save();
	brwsr_cwlog_entries->clear();
	brwsr_cwlog_entries->redraw();
	progStatus.cw_log_name.clear();
	txt_cwlog_file->value(progStatus.cw_log_name.c_str());
	txt_cwlog_file->redraw();

	Fl_Native_File_Chooser fnfc;
	fnfc.title("Create new log file");
	fnfc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	fnfc.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM);
	fnfc.filter("CW Log\t*.txt");
// default directory to use
	fnfc.directory(RigHomeDir.c_str());
	fnfc.preset_file("cwlog.txt");
// Show native chooser
	switch ( fnfc.show() ) {
		case -1:
			fl_message ("ERROR: %s", fnfc.errmsg());
			return; // ERROR
		case 1:
			return; // CANCEL
		default:
			progStatus.cw_log_name = fnfc.filename();
			txt_cwlog_file->value(progStatus.cw_log_name.c_str());
	}
}

void cwlog_close()
{
	if (cwlog_is_open && cwlog_changed)
		cwlog_save();
}

void cwlog_export_adif()
{
// Create and post the local native file chooser
	Fl_Native_File_Chooser fnfc;
	fnfc.title("Export to ADIF file");
	fnfc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	fnfc.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM);
	fnfc.filter("ADIF Log\t*.{adi,adif}");
// default directory to use
	fnfc.directory(RigHomeDir.c_str());
// Show native chooser
	switch ( fnfc.show() ) {
		case -1:
			fl_message ("ERROR: %s", fnfc.errmsg());
			return; // ERROR
		case 1:
			return; // CANCEL
		default:
			break;
	}

	std::string export_fname = fnfc.filename();
	std::ofstream oExport(export_fname.c_str());
	if (!oExport) {
		fl_message ("Could not write to %s", export_fname.c_str());
		return;
	}

	std::string logline,
				cw_qso_date, cw_qso_time,
				cw_freq,
				cw_op_call,
				cw_op_name,
				cw_rst_in, cw_rst_out,
				cw_log_nbr,
				qso_notes;

	size_t ptr = std::string::npos;
	size_t n = brwsr_cwlog_entries->size();
	for (size_t i = 1; i <= n; i++) {
		logline = brwsr_cwlog_entries->text(i);
		if (logline.empty()) continue;

		ptr = logline.find('\t');
		cw_qso_date = logline.substr(0, ptr);
		logline.erase(0, ptr+1);

		ptr = logline.find('\t');
		cw_qso_time = logline.substr(0, ptr);
		logline.erase(0, ptr+1);

		ptr = logline.find('\t');
		cw_freq = logline.substr(0, ptr);
		logline.erase(0, ptr+1);

		ptr = logline.find('\t');
		cw_op_call = logline.substr(0, ptr);
		logline.erase(0, ptr+1);

		ptr = logline.find('\t');
		cw_op_name = logline.substr(0, ptr);
		logline.erase(0, ptr+1);

		ptr = logline.find('\t');
		cw_rst_in = logline.substr(0, ptr);
		logline.erase(0, ptr+1);

		ptr = logline.find('\t');
		cw_rst_out = logline.substr(0, ptr);
		logline.erase(0, ptr+1);

		ptr = logline.find('\t');
		cw_log_nbr = logline.substr(0, ptr);
		logline.erase(0, ptr+1);

		qso_notes = logline;

		oExport << "<QSO_DATE:" << cw_qso_date.length() << ">" << cw_qso_date
				<< "<TIME_ON:" << cw_qso_time.length() << ">" << cw_qso_time
				<< "<FREQ:" << cw_freq.length() << ">" << cw_freq
				<< "<MODE:2>CW"
				<< "<CALL:" << cw_op_call.length() << ">" << cw_op_call
				<< "<NAME:" << cw_op_name.length() << ">" << cw_op_name
				<< "<RST_RCVD:" << cw_rst_in.length() << ">" << cw_rst_in
				<< "<RST_SENT:" << cw_rst_out.length() << ">" << cw_rst_out
				<< "<STX:" << cw_log_nbr.length() << ">" << cw_log_nbr
				<< "<NOTES:" << qso_notes.length() << ">" << qso_notes
				<< "<EOR->" << std::endl;
	}
	oExport.close();
}

std::string cwlog_adif_extract( std::string FIELD, std::string line)
{
	size_t p1, p2;
	p1 = line.find(FIELD);
	if (p1 != std::string::npos)  {
		p1 = line.find(">", p1);
		if (p1 != std::string::npos) {
			p1++;
			p2 = line.find("<", p1);
			if (p2 != std::string::npos)
				return line.substr(p1, p2 - p1);
		}
	}
	return "";
}

void cwlog_import_adif()
{
	std::string import_fname;
	Fl_Native_File_Chooser fnfc;
	fnfc.title("Import from ADIF file");
	fnfc.type(Fl_Native_File_Chooser::BROWSE_FILE);
	fnfc.filter("ADIF Log\t*.{adi,adif}");
// default directory to use
	fnfc.directory(RigHomeDir.c_str());
// Show native chooser
	switch ( fnfc.show() ) {
		case -1:
			fl_message ("ERROR: %s", fnfc.errmsg());
			return; // ERROR
		case 1:
			return; // CANCEL
		default:
			break;
	}
	import_fname = fnfc.filename();
	std::ifstream iImport(import_fname.c_str());
	if (!iImport) return;

// current log
	size_t n = brwsr_cwlog_entries->size();
	size_t p;
	std::string fulllog;
	std::string teststr;
	for (size_t i = 1; i <= n; i++) {
		fulllog.append(brwsr_cwlog_entries->text(i)).append("\n");
	}

	char buff[512];
	std::string line, ldate, ltime, lfreq, lcall, lname, lrst_in, lrst_out, lnbr, lnotes, lbrwsr;
	while (!iImport.eof()) {
		iImport.getline(buff, 512);
		line = buff;
		if (cwlog_adif_extract("MODE", line) == "CW") {
			ldate = cwlog_adif_extract("QSO_DATE", line);
			ltime = cwlog_adif_extract("TIME_ON", line).substr(0,4);
			lfreq = cwlog_adif_extract("FREQ", line);
			lcall = cwlog_adif_extract("CALL", line);
			lname = cwlog_adif_extract("NAME", line);
			lrst_in = cwlog_adif_extract("RST_RCVD", line);
			lrst_out = cwlog_adif_extract("RST_SENT", line);
			lnbr = cwlog_adif_extract("STX", line);
			lnotes = cwlog_adif_extract("NOTES", line);
			lbrwsr.assign(ldate).append("\t");
			lbrwsr.append(ltime).append("\t");
			lbrwsr.append(lfreq).append("\t");
			lbrwsr.append(lcall).append("\t");
			teststr = lbrwsr;
			lbrwsr.append(lname).append("\t");
			lbrwsr.append(lrst_in).append("\t");
			lbrwsr.append(lrst_out).append("\t");
			lbrwsr.append(lnbr).append("\t");
			lbrwsr.append(lnotes);
			p = lbrwsr.find("\n");
			if (p != std::string::npos)
				lbrwsr.erase(p);
			if (fulllog.find(teststr) == std::string::npos &&
				!ldate.empty() &&
				!ltime.empty()) {
				fulllog.append(lbrwsr).append("\n");
				brwsr_cwlog_entries->add(lbrwsr.c_str());
			}
		}
	}
	cwlog_changed = true;
	iImport.close();
}

