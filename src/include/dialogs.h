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

#ifndef __DIALOGS_H__
#define __DIALOGS_H__

#include "config.h"
#include "compat.h" // Must precede all FL includes

#include <string>

#include "font_browser.h"
#include "support.h"

extern Fl_Double_Window *dlgDisplayConfig;
extern Fl_Double_Window *dlgColorsDialog;
extern Fl_Double_Window *dlgXcvrConfig;
extern Fl_Double_Window *dlgMemoryDialog;
extern Fl_Double_Window *dlgControls;

extern Font_Browser     *fntbrowser;

extern void init_port_combos();

extern bool cwlog_editing;

extern void cwlog_sort_by_call();
extern void cwlog_sort_by_datetime();
extern void cwlog_sort_by_nbr();
extern void cwlog_sort_by_freq();

extern void cwlog_clear_qso();
extern void cwlog_save_qso();

extern void cwlog_edit_entry();
extern void cwlog_delete_entry();
extern void cwlog_view();

extern void cwlog_close();
extern void cwlog_open();
extern void cwlog_new();

extern void cwlog_load();
extern void cwlog_save();
extern void cwlog_save_as();

extern void cwlog_export_adif();
extern void cwlog_import_adif();

extern void default_meters();


//----------------------------------------------------------------------
extern uchar fg_red, fg_green, fg_blue;
extern uchar bg_red, bg_green, bg_blue;

extern uchar smeterRed;
extern uchar smeterGreen;
extern uchar smeterBlue;
extern uchar pwrRed;
extern uchar pwrGreen;
extern uchar pwrBlue;
extern uchar alcRed;
extern uchar alcGreen;
extern uchar alcBlue;
extern uchar swrRed;
extern uchar swrGreen;
extern uchar swrBlue;
extern uchar iddRed;
extern uchar iddGreen;
extern uchar iddBlue;
extern uchar voltsRed;
extern uchar voltsGreen;
extern uchar voltsBlue;

extern uchar smeter_peak_red;
extern uchar smeter_peak_green;
extern uchar smeter_peak_blue;
extern uchar pwr_peak_red;
extern uchar pwr_peak_green;
extern uchar pwr_peak_blue;
extern uchar alc_peak_red;
extern uchar alc_peak_green;
extern uchar alc_peak_blue;
extern uchar swr_peak_red;
extern uchar swr_peak_green;
extern uchar swr_peak_blue;
extern uchar idd_peak_red;
extern uchar idd_peak_green;
extern uchar idd_peak_blue;
extern uchar volts_peak_red;
extern uchar volts_peak_green;
extern uchar volts_peak_blue;

extern void cbOKDisplayDialog();
extern void cbOKColorDialog();
extern void cbCancelColorDialog();
extern void cb_ResetColorsDialog();
extern void redraw_dialogs();

extern void update_colors();
extern void setColors();
extern void set_system_colors();
extern void open_colors_dialog();

extern void update_memory_dialog_colors();

//----------------------------------------------------------------------

#endif // __DIALOGS_H__
