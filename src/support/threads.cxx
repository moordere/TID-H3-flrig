// ----------------------------------------------------------------------------
//      threads.cxx
//
// Copyright (C) 2014
//              Stelios Bounanos, M0GLD
//              David Freese, W1HKJ
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#include "threads.h"
#include "util.h"
#include "support.h"

/// This ensures that a mutex is always unlocked when leaving a function or block.

extern pthread_mutex_t mutex_replystr;
extern pthread_mutex_t command_mutex;
extern pthread_mutex_t mutex_serial;
extern pthread_mutex_t debug_mutex;
extern pthread_mutex_t mutex_rcv_socket;
extern pthread_mutex_t mutex_trace;

guard_lock::guard_lock(pthread_mutex_t* m, std::string h, long tout) : mutex(m) {

	how.clear();
	how = h;
	time_out = tout;
	start_time = zmsec();
	for (int i = 0; i < 20; i++) {
		if (pthread_mutex_trylock(mutex) == 0) {
			std::string szlock = name(mutex);
			szlock.append(" try lock ");
			if (!h.empty()) {
				how = h;
				szlock.append(", ").append(how);
			}
			lock_trace(1, szlock.c_str());
			return;
		}
		MilliSleep(50);
	}

	std::string szlock;;
	szlock.assign("lock FAILED ").append(name(mutex));
	if (!h.empty())
		szlock.append(", ").append(h);
	failure_trace(1, szlock.c_str());

}

guard_lock::~guard_lock(void) {

	char szlock[200];
	long now = zmsec();
	snprintf(szlock, sizeof(szlock), "%s locked for %lu msec", name(mutex), (now - start_time));
	lock_trace(1, szlock);

	if (now - start_time > time_out) {
		snprintf(szlock, sizeof(szlock), "%s [ %s ] LOCK TIME: %lu", name(mutex), how.c_str(), (now - start_time));
		failure_trace(1, szlock);
	}

	pthread_mutex_unlock(mutex);
}

const char * guard_lock::name(pthread_mutex_t *m) {
	if (m == &mutex_replystr) return "mutex_replystr";
	if (m == &command_mutex) return "command_mutex";
	if (m == &mutex_replystr) return "mutex_replystr";
	if (m == &mutex_serial) return "mutex_serial";
	if (m == &debug_mutex) return "debug_mutex";
	if (m == &mutex_rcv_socket) return "mutex_rcv_socket";
	if (m == &mutex_srvc_reqs) return "mutex_service_requests";
	if (m == &mutex_trace) return "mutex_trace";
	return "";
}

#ifndef __WIN_32_

int nano_sleep(const struct timespec *req, struct timespec *rem)
{
	return nanosleep(req, rem);
}

#else

int nano_sleep(const struct timespec *req, struct timespec *rem)
{
	if (unlikely(req->tv_nsec < 0 || req->tv_nsec < 0L || req->tv_nsec > 999999999L)) {
		errno = EINVAL;
		return -1;
	}
	Sleep(req->tv_sec * 1000 + req->tv_nsec / 1000000L);
	if (unlikely(rem)) {
		rem->tv_sec = 0;
		rem->tv_nsec = 0L;
	}
	return 0;
}

#endif
