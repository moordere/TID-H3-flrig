
// ---------------------------------------------------------------------
// cwio.cxx  --  morse code modem
//
// Copyright (C) 2020
//		Dave Freese, W1HKJ
//
// This file is part of flrig
//
// flrig is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// flrig is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------


#include "config.h"

#include <cstring>
#include <string>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include <FL/Fl.H>

#include "status.h"
#include "util.h"
#include "threads.h"
#include "debug.h"
#include "tod_clock.h"
#include "cwio.h"
#include "cwioUI.h"
#include "support.h"
#include "status.h"

#ifdef __WIN32__
#include "mingw.h"
#endif

#define CWIO_DEBUG

Cserial *cwio_serial = 0;
cMorse  *morse = 0;

static pthread_t       cwio_pthread;
static pthread_cond_t  cwio_cond;
static pthread_mutex_t cwio_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cwio_text_mutex = PTHREAD_MUTEX_INITIALIZER;

int cwio_process = NONE;
bool cwio_thread_running = false;

static std::string new_text;
std::string cwio_text;

//======================================================================
// sub millisecond accurate sleep function
// sleep_time in seconds
//======================================================================
extern double monotonic_seconds();

//======================================================================
// QMX / QMX+ keyer commands
//======================================================================
void QMX_send_char(int c);
void set_QMX_keyer();
void QMX_sleep(double secs);
double QMX_now();

//======================================================================
// GPIO keyer command
//======================================================================
void send_gpio_CW(int c);


#ifdef CWIO_DEBUG
FILE *fcwio = (FILE *)0;
FILE *fcwio2 = (FILE *)0;
#endif

static double lasterr = 0;

int cw_sleep (double sleep_time)
{
	struct timespec tv1, tv2;
	double start_at = monotonic_seconds();
	double end_at = start_at + sleep_time;
	double delay = sleep_time - 0.010;

	tv1.tv_sec = (time_t) delay;
	tv1.tv_nsec = (long) ((delay - tv1.tv_sec) * 1e+9);
	tv2.tv_sec = 0;
	tv2.tv_nsec = 10;

#ifdef __WIN32__
	timeBeginPeriod(1);
	nano_sleep (&tv1, NULL);
	while ((lasterr = end_at - monotonic_seconds()) > 0)
		nano_sleep(&tv2, NULL);
	timeEndPeriod(1);
#else
	nano_sleep (&tv1, NULL);
	while (((lasterr = end_at - monotonic_seconds()) > 0))
		nano_sleep(&tv2, NULL);
#endif

#ifdef CWIO_DEBUG
	if (fcwio)
		fprintf(fcwio, "%f, %f\n", sleep_time, (end_at - lasterr - start_at));
#endif
	return 0;
}

void cwio_key(bool state)
{
	Cserial *port = cwio_serial;

	switch (progStatus.cwioSHARED) {
		case 1: port = RigSerial; break;
		case 2: port = AuxSerial; break;
		case 3: port = SepSerial; break;
		default: port = cwio_serial;
	}

	if (!port)
		return;

	if (!port->IsOpen())
		return;

	if (progStatus.cwioPTT)
		doPTT(state);

	if (progStatus.cwioKEYLINE == 2) {
		port->setDTR(progStatus.cwioINVERTED ? !state : state);
	} else if (progStatus.cwioKEYLINE == 1) {
		port->setRTS(progStatus.cwioINVERTED ? !state : state);
	}

	return;
}

double start_at2;

void send_cwkey(char c)
{

#if USE_LIBGPIOD
	if ((progStatus.gpio_cw_line != -1) && progStatus.enable_gpio_cw ) {
		return send_gpio_CW(c);
	}
#endif

	if (selrig->name_ == rig_qmx.name_) return QMX_send_char(c);

	start_at2 = monotonic_seconds();

	std::string code = "";
	double tc = 0;
	double tch = 0;
	double twd = 0;
	double xcvr_corr = 0;
	double comp = progStatus.cwio_comp * 1e-3;
	double requested = 0;
	Cserial *port = cwio_serial;

	switch (progStatus.cwioSHARED) {
		case 1: port = RigSerial; break;
		case 2: port = AuxSerial; break;
		case 3: port = SepSerial; break;
		default: port = cwio_serial;
	}

	if (!port)
		goto exit_send_cwkey;

	if (!port->IsOpen())
		goto exit_send_cwkey;

	tc = 1.2 / progStatus.cwioWPM;

	if (comp < tc) tc -= comp;
	tch = 3 * tc;
	twd = 4 * tc;

	xcvr_corr = progStatus.cwio_keycorr * 1e-3;
	if (xcvr_corr < -tc / 2) xcvr_corr = - tc / 2;
	else if (xcvr_corr > tc / 2) xcvr_corr = tc / 2;

	code = morse->tx_lookup(c);

	if (code.empty()) {// == ' ' || c == 0x0a) {
		requested += twd;
		cw_sleep(twd);
		goto exit_send_cwkey;
	}

	for (size_t n = 0; n < code.length(); n++) {
		if (cwio_process == END) {
			goto exit_send_cwkey;
		}
		if (progStatus.cwioKEYLINE == 2) {
			cwio_key(progStatus.cwioINVERTED ? 0 : 1);
		} else if (progStatus.cwioKEYLINE == 1) {
			cwio_key(progStatus.cwioINVERTED ? 0 : 1);
		}
		if (code[n] == '.') {
			requested += (tc + xcvr_corr);
			cw_sleep((tc + xcvr_corr));
		} else {
			requested += tch;
			cw_sleep(tch);
		}

		if (progStatus.cwioKEYLINE == 2) {
			cwio_key(progStatus.cwioINVERTED ? 1 : 0);
		} else if (progStatus.cwioKEYLINE == 1) {
			cwio_key(progStatus.cwioINVERTED ? 1 : 0);
		}
		if (n == code.length() -1) {
			requested += tch;
			cw_sleep(tch);
		} else {
			requested += (tc - xcvr_corr);
			cw_sleep((tc - xcvr_corr));
		}
	}

exit_send_cwkey:

#ifdef CWIO_DEBUG
	if (fcwio2) {
		double duration = monotonic_seconds() - start_at2;
		fprintf(fcwio2, "%f, %f, %f\n", duration, requested, duration - requested);
	}
#endif

	return;
}

void reset_cwioport()
{
	Cserial *port = cwio_serial;
	switch (progStatus.cwioSHARED) {
		case 1: port = RigSerial; break;
		case 2: port = AuxSerial; break;
		case 3: port = SepSerial; break;
		default: port = cwio_serial;
	}
	if (progStatus.cwioKEYLINE == 2) {
		port->setDTR(progStatus.cwioINVERTED ? 1 : 0);
	} else if (progStatus.cwioKEYLINE == 1) {
		port->setRTS(progStatus.cwioINVERTED ? 1 : 0);
	}
}

int open_cwkey()
{
	if (progStatus.cwioSHARED) {
		reset_cwioport();
		return 1;
	}
	if (!cwio_serial)
		cwio_serial = new Cserial;

	cwio_serial->Device(progStatus.cwioPORT);

	if (cwio_serial->OpenPort() == false) {
		LOG_ERROR("Cannot open serial port %s", cwio_serial->Device().c_str());
		cwio_serial = 0;
		return 0;
	}
	LOG_INFO("Opened %s for CW keyline control", cwio_serial->Device().c_str());

	reset_cwioport();

	return 1;
}

void close_cwkey()
{
	if (cwio_serial) {
		cwio_serial->ClosePort();
	}
}

static std::string snd;

void update_txt_to_send(void *)
{
	txt_to_send->clear();
	txt_to_send->buffer()->text(snd.c_str());
	txt_to_send->redraw();
}

void update_sent_text(void *loc)
{
	char ch[2];
	ch[1] = 0;
	ch[0] = *(char *)loc;
	if (ch[0] == ']') ch[0] = '\n';
	cw_sent_text->insert( ch );
	cw_sent_text->redraw();
}

void terminate_sending(void *)
{
	if (progStatus.cwioPTT)
		doPTT(0);
	btn_cwioSEND->value(0);
}

void sending_text()
{
	static char c = 0;
	if (progStatus.cwioPTT) {
		doPTT(1);
		MilliSleep(50);
	}
	while (cwio_process == SEND) {
		c = 0;
		snd = txt_to_send->buffer()->text();
		{
			guard_lock lck(&cwio_text_mutex, std::string(__func__) );
			if (!snd.empty()) {
				c = snd[0];
				snd.erase(0,1);
				Fl::awake(update_txt_to_send);
				Fl::awake(update_sent_text, &c);
			} else {
				if (!cwio_text.empty()) {
					c = cwio_text[0];
					cwio_text.erase(0,1);
				}
			}
		}
		if (c == ' ' || c == '\n')
			send_cwkey(' ');
		else if (!(morse->tx_lookup(c)).empty())
			send_cwkey(c);
		else if (c == ']') {
			cwio_process = END;
			snd.clear();
			Fl::awake(update_txt_to_send);
			Fl::awake(terminate_sending);
			MilliSleep(10);
		} else
			MilliSleep(50);
	}
}

void update_comp_value(void *)
{
	cnt_cwio_comp->value(progStatus.cwio_comp);
	cnt_cwio_comp->redraw();
	btn_cw_dtr_calibrate->value(0);
	btn_cw_dtr_calibrate->redraw();
}

void do_calibration()
{
	std::string paris = "PARIS ";
	std::string teststr;
	double start_time = 0;
	double end_time = 0;

	progStatus.cwio_comp = 0;

	for (int i = 0; i < progStatus.cwioWPM; i++)
		teststr.append(paris);

	txt_to_send->buffer()->text();

	start_time = monotonic_seconds();
	for (size_t n = 0; n < teststr.length(); n++) {
		send_cwkey(teststr[n]);
	}

	end_time = monotonic_seconds();
	double corr = 1000.0 * (end_time - start_time - 60.0) / (50.0 * progStatus.cwioWPM);

	progStatus.cwio_comp = corr;

	Fl::awake(update_comp_value);
}

//----------------------------------------------------------------------
// cwio thread
//----------------------------------------------------------------------

extern bool PRIORITY;

void *cwio_loop(void *)
{
	cwio_thread_running = true;
	cwio_process = NONE;

#if 0
if (PRIORITY) {
	char estr[200];
	std::string erfname = RigHomeDir;
	erfname.append("priority.txt");
	FILE *erfile = fopen(erfname.c_str(),"w");
#if 0
//#ifndef __WIN32__
	int erc = nice(-10);
	if (erc == -1)
		snprintf(estr, sizeof(estr), "%d: errno: %d, %s", __LINE__, errno, strerror(errno));
	else
		snprintf(estr, sizeof(estr), "%d: set thread priority to %d", __LINE__, erc);
	fprintf(erfile, "%s\n", estr);

#else

	// bump up the cwio thread priority
	pthread_attr_t tattr;
	if (pthread_attr_init(&tattr)) {
		LOG_ERROR("cwio thread fail (pthread_attr_init)");
		snprintf(estr, sizeof(estr), "%d: errno: %d, %s", __LINE__, errno, strerror(errno));
		fprintf(erfile, "%s\n", estr);
	}
	sched_param param;
	int sched;
	if (pthread_attr_getinheritsched(&tattr, &sched)) {
		LOG_ERROR("cwio thread fail (pthread_attr_getinheritsched)");
		snprintf(estr, sizeof(estr), "%d: errno: %d, %s", __LINE__, errno, strerror(errno));
		fprintf(erfile, "%s\n", estr);
	}
	sched = PTHREAD_EXPLICIT_SCHED;
	if (pthread_attr_setinheritsched(&tattr, sched)) {
		LOG_ERROR("cwio thread fail (pthread_attr_setinheritsched)");
		snprintf(estr, sizeof(estr), "%d: errno: %d, %s", __LINE__, errno, strerror(errno));
		fprintf(erfile, "%s\n", estr);
	}
	if (pthread_attr_getschedparam(&tattr, &param)) {
		LOG_ERROR("cwio thread fail (pthread_attr_getscheduparam)");
		snprintf(estr, sizeof(estr), "%d: errno: %d, %s", __LINE__, errno, strerror(errno));
		fprintf(erfile, "%s\n", estr);
	}

	snprintf(estr, sizeof(estr), "%d: priority old = %d", __LINE__, param.sched_priority);
	fprintf(erfile, "%s\n", estr);

	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	snprintf(estr, sizeof(estr), "%d: priority new = %d", __LINE__, param.sched_priority);
	fprintf(erfile, "%s\n", estr);

	if (pthread_attr_setschedparam(&tattr, &param)) {
		LOG_ERROR("cwio thread fail (pthread_attr_setscheduparam)");
		snprintf(estr, sizeof(estr), "%d: errno: %d, %s", __LINE__, errno, strerror(errno));
		fprintf(erfile, "%s\n", estr);
	}

	if (pthread_attr_getschedparam(&tattr, &param)) {
		LOG_ERROR("cwio thread fail (pthread_attr_getscheduparam)");
		snprintf(estr, sizeof(estr), "%d: errno: %d, %s", __LINE__, errno, strerror(errno));
		fprintf(erfile, "%s\n", estr);
	}

	snprintf(estr, sizeof(estr), "%d: priority set to %d", __LINE__, param.sched_priority);
	fprintf(erfile, "%s\n", estr);
#endif
	fclose(erfile);
} // PRIORITY

#endif

	while (1) {
		pthread_mutex_lock(&cwio_mutex);
		pthread_cond_wait(&cwio_cond, &cwio_mutex);
		pthread_mutex_unlock(&cwio_mutex);

		if (cwio_process == TERMINATE)
			return NULL;

		if (cwio_process == SEND)
			sending_text();

		if (cwio_process == CALIBRATE)
			do_calibration();
	}
	return NULL;
}

int start_cwio_thread()
{
	if(cwio_thread_running) return 0;

	if (!morse)
		morse = new cMorse;

	memset((void *) &cwio_pthread, 0, sizeof(cwio_pthread));
	memset((void *) &cwio_mutex,   0, sizeof(cwio_mutex));
	memset((void *) &cwio_cond,    0, sizeof(cwio_cond));

	if(pthread_cond_init(&cwio_cond, NULL)) {
		LOG_ERROR("cwio thread create fail (pthread_cond_init)");
		return 1;
	}

	if(pthread_mutex_init(&cwio_mutex, NULL)) {
		LOG_ERROR("cwio thread create fail (pthread_mutex_init)");
		return 1;
	}

	if (pthread_create(&cwio_pthread, NULL, cwio_loop, NULL) < 0) {
		pthread_mutex_destroy(&cwio_mutex);
		LOG_ERROR("cwio thread create fail (pthread_create)");
//		std::cout << __LINE__ << " : errno " << errno << ", " << strerror(errno) << std::endl;
		return 1;
	}

	LOG_INFO("started cwio thread");


	MilliSleep(50); // Give the CPU time to set 'cwio_thread_running'
	return 0;
}

void stop_cwio_thread()
{
	if(!cwio_thread_running) return;

	cwio_process = END;
	btn_cwioSEND->value(0);

	cwio_process = TERMINATE;
	pthread_cond_signal(&cwio_cond);

	pthread_join(cwio_pthread, NULL);

	LOG_INFO("%s", "cwio thread - stopped");

	pthread_mutex_destroy(&cwio_mutex);
	pthread_cond_destroy(&cwio_cond);

	memset((void *) &cwio_pthread, 0, sizeof(cwio_pthread));
	memset((void *) &cwio_mutex,   0, sizeof(cwio_mutex));

	cwio_thread_running = false;
	cwio_process = NONE;

	close_cwkey();

#ifdef CWIO_DEBUG
	if (fcwio) {
		fclose(fcwio);
		fcwio = 0;
	}
	if (fcwio2) {
		fclose(fcwio2);
		fcwio2 = 0;
	}
#endif

	if (cwio_serial) {
		delete cwio_serial;
		cwio_serial = 0;
	}
	if (morse) {
		delete morse;
		morse = 0;
	}

}

void add_cwio_msg(std::string txt)
{
	if (!cwio_thread_running) return;

// expand special character pairs
	size_t p;
	if (cw_op_call) {
		while (((p = txt.find("~C")) != std::string::npos) || ((p = txt.find("~c")) != std::string::npos))
			txt.replace(p, 2, cw_op_call->value());
	}
	if (cw_op_name) {
		while (((p = txt.find("~N")) != std::string::npos) || ((p = txt.find("~n")) != std::string::npos))
			txt.replace(p, 2, cw_op_name->value());
	}
	if (cw_rst_out){
		while (((p = txt.find("~R")) != std::string::npos) || ((p = txt.find("~r")) != std::string::npos))
			txt.replace(p, 2, cw_rst_out->value());
	}
	if (cw_log_nbr) {
		while ((p = txt.find("~#")) != std::string::npos) {
			char sznum[10];
			if (progStatus.cw_log_leading_zeros)
				snprintf(sznum, sizeof(sznum), "%04d", progStatus.cw_log_nbr);
			else
				snprintf(sznum, sizeof(sznum), "%d", progStatus.cw_log_nbr);
			if (progStatus.cw_log_cut_numbers) {
				for (size_t i = 1; i < strlen(sznum); i++) {
					if (sznum[i] == '0') sznum[i] = 'T';
					if (sznum[i] == '9') sznum[i] = 'N';
				}
			}
			txt.replace(p, 2, sznum);
		}
		while ((p = txt.find("~+")) != std::string::npos) {
			txt.replace(p, 2, "");
			progStatus.cw_log_nbr++;
			if (cw_log_nbr)
				cw_log_nbr->value(progStatus.cw_log_nbr);
		}
		while ((p = txt.find("~-")) != std::string::npos) {
			txt.replace(p, 2, "");
			progStatus.cw_log_nbr--;
			if (progStatus.cw_log_nbr < 0) progStatus.cw_log_nbr = 0;
			if (cw_log_nbr)
				cw_log_nbr->value(progStatus.cw_log_nbr);
		}
	}
	add_cwio(txt);
}

void add_cwio(std::string txt)
{
	guard_lock lck(&cwio_text_mutex, std::string(__func__) );
	new_text = txt_to_send->buffer()->text();
	new_text.append(txt);

	size_t pos = std::string::npos;
	if ((pos = new_text.find('[')) != std::string::npos) {
		while (pos != std::string::npos) {
			new_text.erase(pos,1);
			pos = new_text.find('[');
		}
		btn_cwioSEND->value(1);
		send_text(true);
	}
	if (new_text.empty()) return;
	txt_to_send->buffer()->text(new_text.c_str());
	txt_to_send->redraw();
	if (new_text[0] == ']') send_text(true);
}

void send_text(bool state)
{
	if (!cwio_thread_running) return;

	if (state) { //&& cwio_process != SEND) {
		cwio_process = SEND;
		pthread_cond_signal(&cwio_cond);
	} else {
		cwio_process = NONE;
	}
}
 
void key_state(bool key)
{
	if (!cwio_thread_running) return;

	cwio_key(key);
}

void cwio_new_text(std::string txt)
{
	cwio_text.append(txt);
	send_text(true);
}

void cwio_clear_text()
{
	txt_to_send->clear();
	txt_to_send->redraw();
}

void cwio_clear_sent_text()
{
	cw_sent_text->clear();
	cw_sent_text->redraw();
}

void msg_cb(int n)
{
}

void label_cb(int n)
{
}

void exec_msg(int n)
{
	if ((Fl::event_state() & FL_CTRL) == FL_CTRL) {
		for (int n = 0; n < 12; n++) {
			edit_label[n]->value(progStatus.cwio_labels[n].c_str());
			edit_msg[n]->value(progStatus.cwio_msgs[n].c_str());
		}
		cwio_editor->show();
		return;
	}
	add_cwio_msg(progStatus.cwio_msgs[n]);
}

void cancel_edit()
{
	cwio_editor->hide();
}

void apply_edit()
{
	for (int n = 0; n < 12; n++) {
		progStatus.cwio_labels[n] = edit_label[n]->value();
		progStatus.cwio_msgs[n] = edit_msg[n]->value();
		btn_msg[n]->label(progStatus.cwio_labels[n].c_str());
		btn_msg[n]->redraw_label();
	}
}

void done_edit()
{
	cwio_editor->hide();
}

// Alt-P pause transmit
// Alt-S start sending text
// F1 - F12 same as function-button mouse press

void control_function_keys()
{
	int key = Fl::event_key();
	int state = Fl::event_state();

	if (state & FL_ALT) {
		if (key == 'p') {
			btn_cwioSEND->value(0);
			btn_cwioSEND->redraw();
			send_text(false);
			return;
		}
		if (key == 's') {
			btn_cwioSEND->value(1);
			btn_cwioSEND->redraw();
			send_text(true);
			return;
		}
		if (key == 'c') {
			txt_to_send->clear();
			return;
		}
	}
	if ((key >= FL_F) && (key <= FL_F_Last)) {
		exec_msg( key - FL_F - 1);
	}
}

void calibrate_cwio()
{
	txt_to_send->clear();
	btn_cwioSEND->value(0);

	cwio_process = END;
	cw_sleep(0.050);

	cwio_process = CALIBRATE;
	pthread_cond_signal(&cwio_cond);
}


void open_cwio_config()
{
	if (progStatus.cwioSHARED == 1) {
		btn_cwioCAT->value(1); btn_cwioCAT->activate();
		btn_cwioAUX->value(0); btn_cwioAUX->deactivate();
		btn_cwioSEP->value(0); btn_cwioSEP->deactivate();
		select_cwioPORT->value("NONE"); select_cwioPORT->deactivate();
		btn_cwioCONNECT->value(0); btn_cwioCONNECT->deactivate();
	} else {
		btn_cwioCAT->activate();
		btn_cwioAUX->activate();
		btn_cwioSEP->activate();
		select_cwioPORT->activate();
		btn_cwioCONNECT->activate();
	}
	cwio_configure->show();
}

// Send using QMX Kenwood emulation of the QMX_ cat command

int QMX_wpm = 0;
bool use_QMX_keyer = false;

static cMorse *QMX_morse = 0;
static char lastQMX_char = 0;

void set_QMX_keyer();
void QMX_sleep(double secs);
double QMX_now();

double QMX_now()
{
	static struct timespec tp;

#if HAVE_CLOCK_GETTIME
	clock_gettime(CLOCK_MONOTONIC, &tp); 
#elif defined(__WIN32__)
	DWORD msec = GetTickCount();
	tp.tv_sec = msec / 1000;
	tp.tv_nsec = (msec % 1000) * 1000000;
#elif defined(__APPLE__)
	static mach_timebase_info_data_t info = { 0, 0 };
	if (unlikely(info.denom == 0))
		mach_timebase_info(&info);
	uint64_t t = mach_absolute_time() * info.numer / info.denom;
	tp.tv_sec = t / 1000000000;
	tp.tv_nsec = t % 1000000000;
#endif

	return 1.0 * tp.tv_sec + tp.tv_nsec * 1e-9;
}

void QMX_sleep(double secs)
{
	static struct timespec tv = { 0, 1000000L};
	static double end1 = 0;
	static double end2 = 0;
	static double t1 = 0;
	static double t2 = 0;
	static double t3 = 0;
	int loop1 = 0;
	int loop2 = 0;
	int n1 = secs*1e3;
#ifdef __WIN32__
	timeBeginPeriod(1);
#endif
	t1 = QMX_now();
	end2 = t1 + secs - 0.0001;
	end1 = end2 - 0.005;

	t2 = QMX_now();
	while (t2 < end1 && (++loop1 < n1)) {
		nano_sleep(&tv, NULL);
		t2 = QMX_now();
	}
	t3 = t2;
	while (t3 <= end2) {
		loop2++;
		t3 = QMX_now();
	}

#ifdef __WIN32__
	timeEndPeriod(1);
#endif

}

void set_QMX_keyer()
{
	char cmd[10];
	snprintf(cmd, sizeof(cmd), "KS%03d;", progStatus.cwioWPM);
	{
		guard_lock serial(&mutex_serial, std::string(__func__) );
		sendCommand(cmd);
	}
	QMX_sleep(0.050);
	QMX_wpm = progStatus.cwioWPM;
}

void QMX_send_char(int c)
{
	if (QMX_morse == 0) QMX_morse = new cMorse;

	if (QMX_wpm != progStatus.cwioWPM)
		set_QMX_keyer();

	float tc = 1.2 / progStatus.cwioWPM;

	c = toupper(c);
	if (c < ' ' || c > 'Z') c = ' ';

	int len = 4;
	if (lastQMX_char == ' ') len = 7;
	if (c != ' ')
		len = QMX_morse->tx_length(c);

	char cmd[10];
	snprintf(cmd, sizeof(cmd), "KY %c;", c);
	{
		guard_lock serial(&mutex_serial, std::string(__func__) );
		sendCommand(cmd);
	}
	QMX_sleep(tc * len);

	lastQMX_char = c;

}

//----------------------------------------------------------------------
// CW output on GPIO pin
//----------------------------------------------------------------------

#if USE_LIBGPIOD

#include <queue>
#include <fcntl.h>

#include "gpio_common.h"

static gpio_num_t		cw_gpio_num = GPIO_COMMON_UNKNOWN;

//----------------------------------------------------------------------
static void set_gpio_pin(bool key)
{
	int ret;

	ret = gpio_common_set(cw_gpio_num, key);

	if (ret < 0) {
		LOG_ERROR("Error setting GPIO");
	}

}

//----------------------------------------------------------------------

static double CW_gpio_now()
{
	static struct timespec tp;

#if HAVE_CLOCK_GETTIME
	clock_gettime(CLOCK_MONOTONIC, &tp); 
#elif defined(__WIN32__)
	DWORD msec = GetTickCount();
	return 1.0 * msec;
	tp.tv_sec = msec / 1000;
	tp.tv_nsec = (msec % 1000) * 1000000;
#elif defined(__APPLE__)
	static mach_timebase_info_data_t info = { 0, 0 };
	if (unlikely(info.denom == 0))
		mach_timebase_info(&info);
	uint64_t t = mach_absolute_time() * info.numer / info.denom;
	tp.tv_sec = t / 1000000000;
	tp.tv_nsec = t % 1000000000;
#endif
	return 1.0 * tp.tv_sec + tp.tv_nsec * 1e-9;

}

static void CW_gpio_bit(int bit, double msecs)
{
	static double secs;
	static struct timespec tv = { 0, 1000000L};
	static double end1 = 0;
	static double end2 = 0;
	static double t1 = 0;
#ifdef CW_gpio_TTEST
	static double t2 = 0;
#endif
	static double t3 = 0;
	static double t4 = 0;
	int loop1 = 0;
	int loop2 = 0;
	int n1 = msecs * 1e3;

	secs = msecs * 1e-3;

#ifdef __WIN32__
	timeBeginPeriod(1);
#endif

	t1 = CW_gpio_now();

	end2 = t1 + secs - 0.00001;

	set_gpio_pin(bit);

#ifdef CW_gpio_TTEST
	t2 = t3 = CW_gpio_now();
#else
	t3 = CW_gpio_now();
#endif
	end1 = end2 - 0.005;

	while (t3 < end1 && (++loop1 < n1)) {
		nano_sleep(&tv, NULL);
		t3 = CW_gpio_now();
	}

	t4 = t3;
	while (t4 <= end2) {
		loop2++;
		t4 = CW_gpio_now();
	}

#ifdef __WIN32__
	timeEndPeriod(1);
#endif

}

void send_gpio_CW(int c)
{
	if (!morse) {
		morse = new cMorse;
		morse->init();
	}

	if (c == 0x0d) {
		return;
	}

	float tc = 1200.0 / progStatus.cwioWPM;
	if (tc <= 0) tc = 1;

	if (c == 0x0a) c = ' ';

	if (c == ' ') {
		CW_gpio_bit(0, 4 * tc);
		return;
	}

	std::string code = morse->tx_lookup(c);
	if (code.empty()) {
		return;
	}

	double weight = progStatus.cw_weight;

	for (size_t n = 0; n < code.length(); n++) {
		if (code[n] == '.') {
			CW_gpio_bit(1, tc * (1 + weight));
		} else {
			CW_gpio_bit(1, 3*tc * (1 + weight));
		}
		if (n < code.length() -1) {
			CW_gpio_bit(0, tc * (1 - weight));
		} else {
			CW_gpio_bit(0, 3 * tc - tc * weight);
		}
	}
}

#endif // USE_LIBGPIOD
