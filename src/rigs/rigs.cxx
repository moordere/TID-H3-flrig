// ----------------------------------------------------------------------------
// Copyright (C) 2014, 2020
//              David Freese, W1HKJ
//              Michael Black W9MDB
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

//=============================================================================
// Reference Transceiver unique drivers

#include "rigs.h"
#include "util.h"
#include "debug.h"
#include "rig_io.h"

rigbase			rig_null;

RIG_AOR5K		rig_AOR5K;
RIG_FDMDUO		rig_FDMDUO;
RIG_FT100D		rig_FT100D;
RIG_FT450		rig_FT450;
RIG_FT450D		rig_FT450D;
RIG_FT710		rig_FT710;
RIG_FT736R		rig_FT736R;
RIG_FT747		rig_FT747;
RIG_FT747GX		rig_FT747GX;
RIG_FT757GX2	rig_FT757GX2;
RIG_FT767		rig_FT767;
RIG_FT817		rig_FT817;
RIG_FT817BB		rig_FT817BB;
RIG_FT818ND		rig_FT818ND;
RIG_FT847		rig_FT847;
RIG_FT857D		rig_FT857D;
RIG_FT890		rig_FT890;
RIG_FT897D		rig_FT897D;
RIG_FT891		rig_FT891;
RIG_FT900		rig_FT900;
RIG_FT920		rig_FT920;
RIG_FT950		rig_FT950;
RIG_FT990		rig_FT990;
RIG_FT990A		rig_FT990A;
RIG_FT991		rig_FT991;
RIG_FT991A		rig_FT991A;
RIG_FT1000		rig_FT1000;
RIG_FT1000MP	rig_FT1000MP;
RIG_FT1000MP_A	rig_FT1000MP_A;
RIG_FT2000		rig_FT2000;
RIG_FTdx10		rig_FTdx10;
RIG_FTdx101D	rig_FTdx101D;
RIG_FTdx101MP	rig_FTdx101MP;
RIG_FTdx1200	rig_FTdx1200;
RIG_FTdx3000	rig_FTdx3000;
RIG_FT5000		rig_FT5000;
RIG_FTdx9000	rig_FTdx9000;
RIG_FTX_1		rig_FTX_1;
RIG_IC703		rig_IC703;
RIG_IC705		rig_IC705;
RIG_IC706MKIIG	rig_IC706MKIIG;
RIG_IC718		rig_IC718;
RIG_IC728		rig_IC728;
RIG_IC735		rig_IC735;
RIG_IC746		rig_IC746;
RIG_IC746PRO	rig_IC746PRO;
RIG_IC751		rig_IC751;
RIG_IC756		rig_IC756;
RIG_IC756PRO	rig_IC756PRO;
RIG_IC756PRO2	rig_IC756PRO2;
RIG_IC756PRO3	rig_IC756PRO3;
RIG_IC7000		rig_IC7000;
RIG_IC7100		rig_IC7100;
RIG_IC7410		rig_IC7410;
RIG_IC7200		rig_IC7200;
RIG_IC7300		rig_IC7300;
RIG_IC7600		rig_IC7600;
RIG_IC7610		rig_IC7610;
RIG_IC7700		rig_IC7700;
RIG_IC7800		rig_IC7800;
RIG_IC7851		rig_IC7851;
RIG_IC9100		rig_IC9100;
RIG_IC9700		rig_IC9700;
RIG_IC910H		rig_IC910H;
RIG_ICF8101		rig_ICF8101;
RIG_ICR71		rig_ICR71;
RIG_K2			rig_K2;
RIG_K3			rig_K3;
RIG_KX3			rig_KX3;
RIG_KX2			rig_KX2;
RIG_K4			rig_K4;
RIG_PCR1000		rig_PCR1000;
RIG_RAY152		rig_RAY152;
RIG_TMD710		rig_TMD710;
RIG_TRXAVR		rig_TRXAVR;
RIG_TS140		rig_TS140;
RIG_TS440		rig_TS440;
RIG_TS450S		rig_TS450S;
RIG_TS480HX		rig_TS480HX;
RIG_TS480SAT	rig_TS480SAT;
RIG_TS570		rig_TS570;
RIG_TS590S		rig_TS590S;
RIG_TS590SG		rig_TS590SG;
RIG_TS790		rig_TS790;
RIG_TS850		rig_TS850;
RIG_TS890S		rig_TS890S;
RIG_TS950		rig_TS950;
RIG_TS870S		rig_TS870S;
RIG_TS940S		rig_TS940S;
RIG_TS990		rig_TS990;
RIG_TS2000		rig_TS2000;
RIG_TT516		rig_TT516;
RIG_TT535		rig_TT535;
RIG_TT538		rig_TT538;
RIG_TT550		rig_TT550;
RIG_TT563		rig_TT563;
RIG_TT566		rig_TT566;
RIG_TT588		rig_TT588;
RIG_TT599		rig_TT599;
RIG_XI5105		rig_XI5105;
RIG_Xiegu_G90	rig_XIG90;
RIG_G90v4		rig_G90v4;
RIG_X6100		rig_X6100;
RIG_X6200		rig_X6200;
RIG_PowerSDR	rig_PowerSDR;
RIG_FLEX1500	rig_FLEX1500;
RIG_TX500		rig_TX500;
RIG_QCXP		rig_QCXP;
RIG_QDX			rig_qdx;
RIG_QMX			rig_qmx;
RIG_SDR2_PRO	rig_sdr2;
RIG_TCI_SUNDX	rig_tci_sundx;
RIG_TCI_SUNPRO	rig_tci_sunpro;
RIG_TRUSDX		rig_trusdx;
RIG_SmartSDR    rig_smartsdr;
RIG_IC7760	rig_IC7760;

rigbase *rigs[] = {
	&rig_null,		// 0
	&rig_FDMDUO,	// 1
	&rig_FT100D,	// 2
	&rig_FT450,		// 3
	&rig_FT450D,	// 4
	&rig_FT710,		// 5
	&rig_FT736R,	// 6
	&rig_FT747,		// 7
	&rig_FT747GX,		// 7a
	&rig_FT757GX2,	// 8
	&rig_FT767,		// 9
	&rig_FT817,		// 10
	&rig_FT817BB,	// 11
	&rig_FT818ND,	// 12
	&rig_FT847,		// 13
	&rig_FT857D,	// 14
	&rig_FT890,		// 15
	&rig_FT897D,	// 16
	&rig_FT891,		// 17
	&rig_FT900,		// 18
	&rig_FT920,		// 19
	&rig_FT950,		// 20
	&rig_FT990,		// 21
	&rig_FT990A,	// 22
	&rig_FT991,		// 23
	&rig_FT991A,	// 24
	&rig_FT1000,	// 25
	&rig_FT1000MP,	// 26
	&rig_FT1000MP_A,// 27
	&rig_FT2000,	// 28
	&rig_FTdx10,	// 29
	&rig_FTdx101D,	// 30
	&rig_FTdx101MP,	// 31
	&rig_FTdx1200,	// 32
	&rig_FTdx3000,	// 33
	&rig_FT5000,	// 34
	&rig_FTdx9000,	// 35
	&rig_FTX_1,		// 36
	&rig_IC703,		// 37
	&rig_IC705,		// 38
	&rig_IC706MKIIG,// 39
	&rig_IC718,		// 40
	&rig_IC728,		// 41
	&rig_IC735,		// 42
	&rig_IC746,		// 43
	&rig_IC746PRO,	// 44
	&rig_IC751,		// 45
	&rig_IC756,		// 46
	&rig_IC756PRO,	// 47
	&rig_IC756PRO2,	// 48
	&rig_IC756PRO3,	// 49
	&rig_IC7000,	// 50
	&rig_IC7100,	// 51
	&rig_IC7200,	// 52
	&rig_IC7300,	// 53
	&rig_IC7410,	// 54
	&rig_IC7600,	// 55
	&rig_IC7610,	// 56
	&rig_IC7700,	// 57
	&rig_IC7800,	// 58
	&rig_IC7851,	// 59
	&rig_IC9100,	// 60
	&rig_IC9700,	// 61
	&rig_IC910H,	// 62
	&rig_ICF8101,	// 63
	&rig_ICR71,		// 64
	&rig_K2,		// 65
	&rig_K3,		// 66
	&rig_KX3,		// 67
	&rig_KX2,		// 68
	&rig_K4,		// 69
	&rig_PCR1000,	// 70
	&rig_RAY152,	// 71
	&rig_TMD710,	// 72
	&rig_TRXAVR,	// 73
	&rig_TS140,		// 74
	&rig_TS440,		// 75
	&rig_TS450S,	// 76
	&rig_TS480HX,	// 77
	&rig_TS480SAT,	// 78
	&rig_TS570,		// 79
	&rig_TS590S,	// 80
	&rig_TS590SG,	// 81
	&rig_TS790,		// 82
	&rig_TS850,		// 83
	&rig_TS870S,	// 84
	&rig_TS890S,	// 85
	&rig_TS940S,	// 86
	&rig_TS950,		// 87
	&rig_TS990,		// 88
	&rig_TS2000,	// 89
	&rig_TT516,		// 90
	&rig_TT535,		// 91
	&rig_TT538,		// 92
	&rig_TT550,		// 93
	&rig_TT563,		// 94
	&rig_TT566,		// 95
	&rig_TT588,		// 96
	&rig_TT599,		// 97
	&rig_AOR5K,		// 98
	&rig_XI5105,	// 99
	&rig_XIG90,		// 100
	&rig_G90v4,		// 101
	&rig_X6100,		// 102
	&rig_X6200,		// 103
	&rig_PowerSDR,	// 104
	&rig_FLEX1500,	// 105
	&rig_TX500,		// 106
	&rig_QCXP,		// 107
	&rig_qdx,		// 108
	&rig_qmx,		// 109
	&rig_sdr2,		// 110
	&rig_tci_sundx,	// 111
	&rig_tci_sunpro,// 112
	&rig_trusdx,	// 113
	&rig_smartsdr,  // 114
	&rig_IC7760,	// 115
	NULL

};

//=============================================================================
