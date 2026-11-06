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

#ifndef _TRACE_H
#define _TRACE_H

#include <cstdio>
#include <string.h>

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Text_Display.H>

#include "rigpanel.h"
#include "status.h"

#include "xml_server.h"

#include "xmlrpc_rig.h"
#include "XmlRpc.h"

extern Fl_Double_Window*	tracewindow;
extern 	Fl_Text_Display*	tracedisplay;
extern 	Fl_Text_Buffer*		tracebuffer;
extern	Fl_Button*			btn_view_trace_config;
extern	Fl_Light_Button*	btn_record_trace;
extern 	Fl_Button*			btn_cleartrace;
extern 	Fl_Light_Button*	btn_pausetrace;

extern Fl_Double_Window*	config_trace_dialog;
extern 	Fl_Check_Button *btn_trace;
extern 	Fl_Check_Button *btn_xmltrace;
extern 	Fl_Check_Button *btn_rigtrace;
extern 	Fl_Check_Button *btn_gettrace;
extern 	Fl_Check_Button *btn_settrace;
extern 	Fl_Check_Button *btn_debugtrace;
extern 	Fl_Check_Button *btn_rpctrace;
extern 	Fl_Check_Button *btn_serialtrace;
extern 	Fl_Check_Button *btn_lock_trace;
extern 	Fl_Check_Button *btn_start_stop_trace;
extern 	Fl_ComboBox *selectlevel;
extern 	Fl_Button *btn_viewtrace;

extern void trace(int n, ...); // all args of type const char *
extern void xml_trace(int n, ...); // all args of type const char *
extern void rig_trace(int n, ...); // trace transceiver class methods
extern void get_trace(int n, ...); // trace get methods
extern void set_trace(int n, ...); // trace set methods
extern void ser_trace(int n, ...); // trace serial methods
extern void rpc_trace(int n, ...); // trace transceiver class methods
extern void deb_trace(int n, ...); // trace debug statements
extern void tci_trace(int n, ...);

extern bool activate_lock_trace;
extern void lock_trace(int n, ...); // trace lock/unlock statements
extern void failure_trace(int n, ...);
extern void test_trace(int n, ...);

extern void make_trace_window();
extern void view_trace();

extern void make_config_trace_dialog();
extern void open_config_trace_dialog();

#define getr(s)  get_trace(1, s);
#define setr(s)  set_trace(1, s);

#define gett(str) get_trace(5, str, "S: ", cmd.c_str(), " R: ", replystr.c_str())
#define sett(str) set_trace(5, str, "S: ", cmd.c_str(), " R: ", replystr.c_str())

#define getthex(str) { \
	static std::string hex1; \
	hex1 = str2hex(cmd.c_str(), cmd.length()); \
	static std::string hex2; \
	hex2 = str2hex(replystr.c_str(), replystr.length()); \
	get_trace(5, str, "S: ", hex1.c_str(), " R: ", hex2.c_str()); \
}

#define setthex(str) { \
	static std::string hex1; \
	hex1 = str2hex(cmd.c_str(), cmd.length()); \
	static std::string hex2; \
	hex2 = str2hex(replystr.c_str(), replystr.length()); \
	set_trace(5, str, "S: ", hex1.c_str(), " R: ", hex2.c_str()); \
}

#define seth() { \
	static std::string hex1; \
	hex1 = str2hex(cmd.c_str(), cmd.length()); \
	static std::string hex2; \
	hex2 = str2hex(replystr.c_str(), replystr.length()); \
	set_trace(4, "S: ", hex1.c_str(), " R: ", hex2.c_str()); \
}

#define geth() { \
	static std::string hex1; \
	hex1 = str2hex(cmd.c_str(), cmd.length()); \
	static std::string hex2; \
	hex2 = str2hex(replystr.c_str(), replystr.length()); \
	get_trace(4, "S: ", hex1.c_str(), " R: ", hex2.c_str()); \
}

#define getcr(str) { \
	static std::string s1; \
	s1 = cmd; \
	static std::string s2; \
	s2 = replystr; \
	size_t n = 0; \
	while (n < s1.length()) { \
		if (s1[n] == '\r') s1.replace(n, 1, "<cr>"); \
		n++; \
	} \
	n = 0; \
	while (n < s2.length()) { \
		if (s2[n] == '\r') s2.replace(n, 1, "<cr>"); \
		n++; \
	} \
	get_trace(5, str, "  ", s1.c_str(), " / ", s2.c_str()); \
}

#define setcr(str) { \
	static std::string s1; \
	s1 = cmd; \
	size_t n = 0; \
	while (n < s1.length()) { \
		if (s1[n] == '\r') s1.replace(n, 1, "<cr>"); \
		n++; \
	} \
	set_trace(3, str, "  ", s1.c_str()); \
}

#ifdef WITH_TRACED
#define TRACED(name, ...) name(__VA_ARGS__) { \
      static char tmsg[50]; \
      static unsigned trace_calls_##name = 0; \
      ++trace_calls_##name; \
      snprintf(tmsg, sizeof(tmsg), "[%2u] %s", trace_calls_##name, #name ); \
      get_trace(1, tmsg);
#else
#  define TRACED(name, ...) name(__VA_ARGS__) {
#endif

#endif
