// ----------------------------------------------------------------------------
// Copyright (C) 2026
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
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Light_Button.H>

#include "config.h"
#include "compat.h" // Must precede all FL includes

#include "gettext.h"
#include "rigpanel.h"
#include "images.h"
#include "rig.h"
#include "rigs.h"
#include "status.h"
#include "support.h"
#include "socket_io.h"
#include "hspinner.h"
#include "ui.h"
//#include "cwioUI.h"
//#include "cwio.h"
//#include "fsk.h"
//#include "fskioUI.h"

#include "fileselect.h"

#include "rigpanel.h"
#include "status.h"
#include "trace.h"

#include "dialogs.h"
#include "cmedia.h"
#include "tmate2.h"
#include "rigs.h"
#include "xml_server.h"

#include "xmlrpc_rig.h"
#include "XmlRpc.h"

Fl_Double_Window*	tracewindow = (Fl_Double_Window *)0;
	Fl_Text_Display*	tracedisplay = (Fl_Text_Display *)0;
	Fl_Text_Buffer*		tracebuffer = (Fl_Text_Buffer*)0;
	Fl_Button*			btn_view_trace_config = (Fl_Button *)0;
	Fl_Button*			btn_cleartrace = (Fl_Button *)0;
	Fl_Light_Button*	btn_record_trace = (Fl_Light_Button *)0;
	Fl_Light_Button*	btn_pausetrace = (Fl_Light_Button *)0;

Fl_Double_Window*	config_trace_dialog = (Fl_Double_Window *)0;
	Fl_Check_Button *btn_trace = (Fl_Check_Button *)0;
	Fl_Check_Button *btn_xmltrace = (Fl_Check_Button *)0;
	Fl_Check_Button *btn_rigtrace = (Fl_Check_Button *)0;
	Fl_Check_Button *btn_gettrace = (Fl_Check_Button *)0;
	Fl_Check_Button *btn_settrace = (Fl_Check_Button *)0;
	Fl_Check_Button *btn_debugtrace = (Fl_Check_Button *)0;
	Fl_Check_Button *btn_rpctrace = (Fl_Check_Button *)0;
	Fl_Check_Button *btn_serialtrace = (Fl_Check_Button *)0;
	Fl_Check_Button *btn_lock_trace = (Fl_Check_Button *)0;
	Fl_Check_Button *btn_start_stop_trace = (Fl_Check_Button *)0;
	Fl_ComboBox *selectlevel = (Fl_ComboBox *)0;
	Fl_Button *btn_viewtrace = (Fl_Button *)0;

void view_trace()
{
	if (!tracewindow) make_trace_window();
	tracewindow->show();
}

void open_config_trace_dialog()
{
	if (!config_trace_dialog) make_config_trace_dialog();
	config_trace_dialog->show();
}

static void cb_btn_viewtrace(Fl_Button *, void *) {
	view_trace();
}

static void cb_btn_trace(Fl_Check_Button *, void *) {
	progStatus.trace = btn_trace->value();
}

static void cb_btn_rigtrace(Fl_Check_Button *, void *) {
	progStatus.rigtrace = btn_rigtrace->value();
}

static void cb_btn_gettrace(Fl_Check_Button *, void *) {
	progStatus.gettrace = btn_gettrace->value();
}

static void cb_btn_settrace(Fl_Check_Button *, void *) {
	progStatus.settrace = btn_settrace->value();
}

static void cb_btn_xmltrace(Fl_Check_Button *, void *) {
	progStatus.xmltrace = btn_xmltrace->value();
}

static void cb_btn_debugtrace(Fl_Check_Button *, void *) {
	progStatus.debugtrace = btn_debugtrace->value();
}

static void cb_btn_rpctrace(Fl_Check_Button *, void *) {
	progStatus.rpctrace = btn_rpctrace->value();
}

static void cb_btn_serialtrace(Fl_Check_Button *, void *) {
	progStatus.serialtrace = btn_serialtrace->value();
}

static void cb_btn_lock_trace(Fl_Check_Button *, void *) {
	progStatus.locktrace = btn_lock_trace->value();
}

static void cb_btn_start_stop_trace(Fl_Check_Button *, void *) {
	progStatus.start_stop_trace = btn_start_stop_trace->value();
}

static void cb_selectlevel(Fl_ComboBox *, void *) {
	progStatus.rpc_level = selectlevel->index();
	XmlRpc::setVerbosity(progStatus.rpc_level);
}


void make_config_trace_dialog()
{
	config_trace_dialog = new Fl_Double_Window(100, 100, 475, 210, "Configure trace");

	btn_trace = new Fl_Check_Button(10, 20, 80, 20, _("Trace support code"));
	btn_trace->value(progStatus.trace);
	btn_trace->callback((Fl_Callback*)cb_btn_trace);
	btn_trace->tooltip(_("Enable trace support"));

	btn_debugtrace = new Fl_Check_Button(10, 50, 80, 20, _("Trace debug code"));
	btn_debugtrace->value(progStatus.debugtrace);
	btn_debugtrace->callback((Fl_Callback*)cb_btn_debugtrace);
	btn_debugtrace->tooltip(_("Display debug output on trace view"));

	btn_rigtrace = new Fl_Check_Button(10, 80, 80, 20, _("Trace rig class code"));
	btn_rigtrace->value(progStatus.rigtrace);
	btn_rigtrace->callback((Fl_Callback*)cb_btn_rigtrace);
	btn_rigtrace->tooltip(_("Enable trace of rig methods"));

	btn_gettrace = new Fl_Check_Button(10, 110, 80, 20, _("Trace rig class get code"));
	btn_gettrace->value(progStatus.gettrace);
	btn_gettrace->callback((Fl_Callback*)cb_btn_gettrace);
	btn_gettrace->tooltip(_("Enable trace of rig get methods"));

	btn_settrace = new Fl_Check_Button(10, 140, 80, 20, _("Trace rig class set code"));
	btn_settrace->value(progStatus.settrace);
	btn_settrace->callback((Fl_Callback*)cb_btn_settrace);
	btn_settrace->tooltip(_("Enable trace of rig set methods"));

	btn_lock_trace = new Fl_Check_Button(10, 170, 80, 20, _("Trace guard lock"));
	btn_lock_trace->value(progStatus.locktrace);
	btn_lock_trace->callback((Fl_Callback*)cb_btn_lock_trace);
	btn_lock_trace->tooltip(_("Enable trace of pthread locking/unlocking"));

	btn_xmltrace = new Fl_Check_Button(240, 20, 80, 20, _("Trace xml_server code"));
	btn_xmltrace->value(progStatus.xmltrace);
	btn_xmltrace->callback((Fl_Callback*)cb_btn_xmltrace);
	btn_xmltrace->tooltip(_("Enable trace of xmlrpc functions"));

	btn_rpctrace = new Fl_Check_Button(240, 50, 80, 20, _("Trace xmlrpcpp code"));
	btn_rpctrace->value(progStatus.rpctrace);
	btn_rpctrace->callback((Fl_Callback*)cb_btn_rpctrace);
	btn_rpctrace->tooltip(_("Enable trace of XmlRpc methods"));

	btn_serialtrace = new Fl_Check_Button(240, 80, 80, 20, _("Trace serial code"));
	btn_serialtrace->value(progStatus.serialtrace);
	btn_serialtrace->callback((Fl_Callback*)cb_btn_serialtrace);
	btn_serialtrace->tooltip(_("Enable trace of serial i/o"));

	btn_start_stop_trace = new Fl_Check_Button(240, 110, 80, 20, _("Trace start/stop code"));
	btn_start_stop_trace->value(progStatus.start_stop_trace);
	btn_start_stop_trace->callback((Fl_Callback*)cb_btn_start_stop_trace);
	btn_start_stop_trace->tooltip(_("Enable trace of start/stop operations"));

	selectlevel = new Fl_ComboBox(240, 140, 80, 20, _("XmlRpc trace level"));
	selectlevel->add("0|1|2|3|4");
	selectlevel->align(FL_ALIGN_RIGHT);
	selectlevel->index(progStatus.rpc_level);
	selectlevel->tooltip(_("0 = off ... 4 maximum depth"));
	selectlevel->readonly();
	selectlevel->callback((Fl_Callback*)cb_selectlevel);

	btn_viewtrace = new Fl_Button(config_trace_dialog->w() - 91, config_trace_dialog->h() - 30, 85, 24, _("View Trace"));
	btn_viewtrace->callback((Fl_Callback*)cb_btn_viewtrace);

	config_trace_dialog->end();

}
