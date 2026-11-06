// ----------------------------------------------------------------------------
// Copyright (C) 2026
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
//
// TIDRADIO TD-H3 running nicFW 2.5x firmware (nicsure)
//
// ----------------------------------------------------------------------------

#ifndef NICFW_H3_H
#define NICFW_H3_H

#include <pthread.h>

#include "rigbase.h"

class RIG_NICFW_H3 : public rigbase {
public:
	RIG_NICFW_H3();
	~RIG_NICFW_H3() { pthread_mutex_destroy(&io_mutex); }

	void initialize();
	bool check();

	unsigned long long get_vfoA();
	void set_vfoA(unsigned long long);
	unsigned long long get_vfoB();
	void set_vfoB(unsigned long long);

	void selectA();
	void selectB();
	int  get_vfoAorB();
	bool twovfos() { return true; }

	void set_modeA(int val);
	int  get_modeA();
	void set_modeB(int val);
	int  get_modeB();
	int  get_modetype(int n) { return 'U'; }

	void set_bwA(int val);
	int  get_bwA();
	void set_bwB(int val);
	int  get_bwB();
	int  adjust_bandwidth(int m) { return 0; }
	int  def_bandwidth(int m) { return 0; }
	std::vector<std::string> &bwtable(int m);

	void set_PTT_control(int val);
	int  get_PTT() { return ptt_; }

private:
	// decoded channelInfo record (32 bytes)
	struct vfo_info {
		bool               valid;
		unsigned long long rx_hz;
		unsigned long long tx_hz;
		int                modulation;  // 0 Auto, 1 FM, 2 AM, 3 USB
		int                bandwidth;   // 0 Wide, 1 Narrow
	};

	// the parts of the radio settings block used here
	struct radio_settings {
		bool valid;
		int  active_vfo;      // 0 = A, 1 = B
		int  mode[2];         // 0 = VFO (frequency) mode, 1 = channel mode
		int  group[2];        // channel group, 0 = all channels
		int  channel[2];      // channel shown when in channel mode
	};

	radio_settings rs;
	unsigned long long settings_time;
	unsigned long long select_time;
	int  radio_vfo;           // 0 = A, 1 = B as last known
	int  want_vfo;            // VFO selected by flrig
	bool select_pending;      // radio not yet switched to want_vfo
	pthread_mutex_t io_mutex; // keeps multi byte transactions intact

	void decode_vfo(const unsigned char *p, vfo_info &v);
	bool read_block(int block, unsigned char *data);
	bool read_settings(bool force = false);
	int  slot_for(int vfo);
	bool read_vfo(int vfo, vfo_info &v);
	void send_key(int key);
	void press_key(int key);
	bool enter_frequency(unsigned long long freq);
	void swap_vfo();
	void wake_keypad();
	void apply_select();
	void update_vfo(int vfo);
};

#endif
