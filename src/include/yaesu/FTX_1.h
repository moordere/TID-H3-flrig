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

#ifndef _FTX_1_H
#define _FTX_1_H

#include "rigbase.h"

class RIG_FTX_1 : public rigbase {
private:
	bool notch_on;
	int  m_60m_indx;
protected:
	int  preamp_level;
	int  atten_level;
public:
	RIG_FTX_1();
	~RIG_FTX_1(){}

	void initialize();
	void post_initialize();

	bool check();

	unsigned long long get_vfoA();
	void set_vfoA(unsigned long long);

	unsigned long long get_vfoB();
	void set_vfoB(unsigned long long);

	bool twovfos() { return true; }
	bool canswap() { return true; }

	void selectA();
	void selectB();

	void A2B();
	void B2A();
	void swapAB();

	bool can_split();
	void set_split(bool val);
	int  get_split();

	void set_modeA(int val);
	int  get_modeA();
	int  get_modetype(int n);

	void set_modeB(int val);
	int  get_modeB();

	void set_bwA(int val);
	int  get_bwA();

	void set_bwB(int val);
	int  get_bwB();

	const char * get_bwname_(int bw, int md);

	int  adjust_bandwidth(int val);
	int  def_bandwidth(int val);

	int  get_agc();
	int  incr_agc();
	const char *agc_label();
	int  agc_val();

	int  get_smeter();
	int  get_swr();
	int  get_alc();
	int  get_power_out();

	double get_power_control();
	void   set_power_control(double val);
	void   get_pc_min_max_step(double &min, double &max, double &step);

	void set_volume_control(int val);
	int  get_volume_control();

	void set_PTT_control(int val);
	int  get_PTT();

	void tune_rig(int how);
	int  get_tune();

	void set_attenuator(int val);
	int  get_attenuator();
	void set_preamp(int val);
	int  get_preamp();

	void set_if_shift(int val);
	bool get_if_shift(int &val);
	void get_if_min_max_step(int &min, int &max, int &step) {
		if_shift_min = min = -1200; if_shift_max = max = 1200; if_shift_step = step = 20; if_shift_mid = 0; }

	void set_notch(bool on, int val);
	bool get_notch(int &val);
	void get_notch_min_max_step(int &min, int &max, int &step) {
		min = 10; max = 3200; step = 10; }

	void set_auto_notch(int v);
	int  get_auto_notch();

	void set_noise(bool b); // noise reduction
	int  get_noise();
	void get_nb_min_max_step(int &min, int &max, int &step) {
		min = 1; max = 15; step = 1; }
	void set_nb_level(int val);
	int  get_nb_level();

	void set_noise_reduction_val(int val); // noise blanker
	int  get_noise_reduction_val();
	void get_nr_min_max_step(int &min, int &max, int &step) {
		min = 0; max = 10; step = 1; }

	void set_noise_reduction(int val);
	int  get_noise_reduction();


	void set_mic_gain(int val);
	int  get_mic_gain();
	void get_mic_min_max_step(int &min, int &max, int &step) {
		min = 0; max = 100; step = 1; }

	void set_rf_gain(int val);
	int  get_rf_gain();
	void get_rf_min_max_step(int &min, int &max, int &step) {
		min = 0; max = 30; step = 1; }

	void set_squelch(int val);
	int  get_squelch();
	void get_squelch_min_max_step(int &min, int &max, int &step) {
		min = 0; max = 100; step = 5; }
	
	std::vector<std::string>& bwtable(int);

	void set_vox_onoff();
	void set_vox_gain();
	void set_vox_anti();
	void set_vox_hang();
	void set_vox_on_dataport();

	void set_cw_weight();
	void set_cw_wpm();
	void get_cw_wpm_min_max(int &min, int &max) {
		min = 4; max = 60; }
	void enable_keyer();
	int  get_keyer();
	void set_cw_qsk();
	int  get_cw_qsk();
	void set_cw_delay();
	int  get_cw_delay();
	void set_cw_vol();
	int  get_cw_vol();

	bool set_cw_spot();
	void set_cw_spot_tone();
	void get_cw_spot_tone_min_max_step(int &min, int &max, int &step) {
		min = 300; max = 1050; step = 10; }

	void set_xcvr_auto_on();
	void set_xcvr_auto_off();

	void set_compression(int, int);
	void get_compression(int &on, int &val);
	void get_comp_min_max_step(int &min, int &max, int &step) {
		min = 0; max = 100; step = 5; }

	void get_band_selection(int v);

	void setVfoAdj(double v);
	double getVfoAdj();
	void get_vfoadj_min_max_step(double &min, double &max, double &step);

	void zero_in();
};


#endif
