// ----------------------------------------------------------------------------
// Copyright (C) 2023
//              David Freese, W1HKJ
//              Dominik, DB1RUF
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
// 20230106

#ifndef TMD710_H
#define TMD710_H

#include "rigbase.h"

class RIG_TMD710 : public rigbase {
public:
	RIG_TMD710();
	~RIG_TMD710(){}

	void initialize();
	bool check();

	unsigned long long get_vfoA();
	unsigned long long get_vfoB();
	void set_vfoA(unsigned long long);
	void set_vfoB(unsigned long long);
	void set_modeA(int val);
	void set_modeB(int val);
	int  get_modeA();
	int  get_modeB();
	int  get_modetype(int);

	int  adjust_bandwidth(int val) { return 0; }

	void selectA();
	void selectB();
	int  get_vfoAorB();

	void set_PTT_control(int val);
	int  get_PTT();

	double get_power_control();
	void set_power_control(double val);
	void get_pc_min_max_step(double &min, double &max, double &step) {
		min = 5; max = 50; step = 5; }

	void set_squelch(int val);
	int  get_squelch();
	void get_squelch_min_max_step(int &min, int &max, int &step) {
		min = 0; max = 31; step = 1; }

};

#endif
