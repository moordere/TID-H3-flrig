// ----------------------------------------------------------------------------
// Copyright (C) 2023
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

#ifndef _SmartSDR_H
#define _SmartSDR_H

#include "kenwood/KENWOOD.h"

class RIG_SmartSDR : public KENWOOD {
private:
	int FilterInner;
	int FilterOuter;

public:

enum SmartSDRMODES { LSB, USB, CWL, CWU, FM, AM, DIGU, DIGL, SAM, NFM, DFM, RTTY };

	RIG_SmartSDR();
	~RIG_SmartSDR(){}

	void initialize();
	void shutdown();

	const char * get_bwname_(int n, int md);

	int get_smeter();

	unsigned long long get_vfoA();
	void set_vfoA(unsigned long long);
	unsigned long long get_vfoB();
	void set_vfoB(unsigned long long);

	void selectA();
	void selectB();

	void set_power_control(double val);
	double get_power_control();

	int set_widths(int val);

	void set_modeA(int val);
	int get_modeA();

	void set_modeB(int val);
	int get_modeB();

	int adjust_bandwidth(int val);
	int def_bandwidth(int val);

	int get_bw(int mode);

	void set_bwA(int val);
	int get_bwA();

	void set_bwB(int val);
	int get_bwB();

	int get_modetype(int n);

	void set_rf_gain(int val);
	int  get_rf_gain();
	void get_rf_min_max_step(int &min, int &max, int &step);

	void set_mic_gain(int val);
	int get_mic_gain();
	void get_mic_min_max_step(int &min, int &max, int &step);

	void set_PTT_control(int val);
	int get_PTT();

	std::vector<std::string>& bwtable(int);

};


#endif
