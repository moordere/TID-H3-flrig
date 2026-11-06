// ---------------------------------------------------------------------
//
//      xml_server.h, a part of flrig
//
// Copyright (C) 2014
//               Dave Freese, W1HKJ
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the program; if not, write to the
//
//  Free Software Foundation, Inc.
//  51 Franklin Street, Fifth Floor
//  Boston, MA  02110-1301 USA.
//
// ---------------------------------------------------------------------

#ifndef XML_SERVER_H
#define XML_SERVER_H

#include <fstream>
#include <vector>
#include <string>

#include <math.h>
#ifndef WIN32
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#endif

#include "status.h"
#include "tod_clock.h"

#include <FL/fl_show_colormap.H>
#include <FL/fl_ask.H>

extern int TOD_STALE;

struct XML_DATA {
	unsigned long long int  freq;
	unsigned long long int freq_tod;

	int mode;
	unsigned long long int mode_tod;

	int bw;
	unsigned long long int bw_tod;

	int ptt;
	unsigned long long int ptt_tod;

	unsigned long long int tod;

	XML_DATA() {
		ptt = freq = mode = bw = 0;
		ptt_tod = freq_tod = mode_tod = bw_tod = zmsec();
	}
	~XML_DATA() {};

	void update_freq(unsigned long long int f) {
		freq = f;
		freq_tod = zmsec();
	}
	bool freq_stale() {
		return int(zmsec() - freq_tod) > TOD_STALE;
	}

	void update_mode(int md) {
		mode = md;
		mode_tod = zmsec();
	}
	bool mode_stale() {
		return int(zmsec() - mode_tod) > TOD_STALE;
	}

	void update_bw(int val) {
		bw = val;
		bw_tod = zmsec();
	}
	bool bw_stale() {
		return int(zmsec() - bw_tod) > TOD_STALE;
	}

	void update_ptt(int val) {
		ptt = val;
		ptt_tod = zmsec();
	}
	bool ptt_stale() {
		return int(zmsec() - ptt_tod) > TOD_STALE;
	}

	void update(unsigned long long int f, int md, int val) {
		update_freq(f);
		update_mode(md);
		update_bw(val);
	}
};

extern XML_DATA xml_A;
extern XML_DATA xml_B;

extern void start_server(int port = 12345);
extern void exit_server();
extern void set_server_port(int port = 12345);

extern std::string print_xmlhelp();

#endif
