// ----------------------------------------------------------------------------
// Copyright (C) 2020
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
#include <FL/Fl_Native_File_Chooser.H>

#include "gpio.h"
#include "gpio_ptt.h"

Fl_Input *gpio_ptt_dev=(Fl_Input *)0;
Fl_Button *btn_select_gpio_ptt=(Fl_Button *)0;
Fl_Int_Input *gpio_ptt_line=(Fl_Int_Input *)0;
Fl_Check_Button *btn_enable_gpio_ptt=(Fl_Check_Button *)0;

Fl_Button *btn_select_gpio_cw=(Fl_Button *)0;
Fl_Input *gpio_cw_dev=(Fl_Input *)0;
Fl_Int_Input *gpio_cw_line=(Fl_Int_Input *)0;
Fl_Check_Button *btn_enable_gpio_cw=(Fl_Check_Button *)0;

static void cb_gpio_ptt_dev(Fl_Input* o, void*) {
  progStatus.gpio_ptt_device = o->value();
  progStatus.enable_gpio_ptt = false;
  btn_enable_gpio_ptt->value(0);
}

static void cb_btn_select_gpio_ptt(Fl_Button*, void*) {
  Fl_Native_File_Chooser fnfc;
  fnfc.title("Select GPIO PTT device");
  fnfc.type(Fl_Native_File_Chooser::BROWSE_FILE);
  fnfc.filter("GPIO devices\t*gpio*\n");
  fnfc.directory("/dev");           // default directory to use
  // Show native chooser
  switch ( fnfc.show() ) {
    case -1: break; // ERROR
    case  1: break; // CANCEL
    default: {
      gpio_ptt_dev->value(fnfc.filename());
      progStatus.enable_gpio_ptt = false;
      progStatus.gpio_ptt_device = fnfc.filename();
      btn_enable_gpio_ptt->value(0);
      break; // FILE CHOSEN
    }
  }
}

static void cb_gpio_ptt_line(Fl_Int_Input* o, void*) {
  progStatus.gpio_ptt_line = atoi(o->value());
  progStatus.enable_gpio_ptt = false;
  btn_enable_gpio_ptt->value(0);
}

static void cb_btn_enable_gpio_ptt(Fl_Check_Button* o, void*) {
  progStatus.enable_gpio_ptt = o->value();
}

static void cb_gpio_cw_dev(Fl_Input* o, void*) {
  progStatus.gpio_cw_device = o->value();
  progStatus.enable_gpio_cw = false;
  btn_enable_gpio_cw->value(0);
}

static void cb_btn_select_gpio_cw(Fl_Button*, void*) {
  Fl_Native_File_Chooser fnfc;
  fnfc.title("Select GPIO CW device");
  fnfc.type(Fl_Native_File_Chooser::BROWSE_FILE);
  fnfc.filter("GPIO devices\t*gpio*\n");
  fnfc.directory("/dev");           // default directory to use
  // Show native chooser
  switch ( fnfc.show() ) {
    case -1: break; // ERROR
    case  1: break; // CANCEL
    default: {
      gpio_cw_dev->value(fnfc.filename());
      progStatus.enable_gpio_ptt = false;
      progStatus.gpio_cw_device = fnfc.filename();
      btn_enable_gpio_cw->value(0);
      break; // FILE CHOSEN
    }
  }
}

static void cb_gpio_cw_line(Fl_Int_Input* o, void*) {
  progStatus.gpio_cw_line = atoi(o->value());
  progStatus.enable_gpio_cw = false;
  btn_enable_gpio_cw->value(0);
}

static void cb_btn_enable_gpio_cw(Fl_Check_Button* o, void*) {
  progStatus.enable_gpio_cw = o->value();
}

Fl_Group *createGPIO(int X, int Y, int W, int H, const char *label)
{
	static char linenbr[20];

	Fl_Group *tab = new Fl_Group(X, Y, W, H, label);

		gpio_ptt_dev = new Fl_Input(X + 35, Y + 40, 200 , 22, gettext("GPIO PTT device"));
		gpio_ptt_dev->tooltip(gettext("Select PTT device file"));
		gpio_ptt_dev->callback((Fl_Callback*)cb_gpio_ptt_dev);
		gpio_ptt_dev->align(Fl_Align(FL_ALIGN_TOP_LEFT));
		gpio_ptt_dev->value(progStatus.gpio_ptt_device.c_str());

		btn_select_gpio_ptt = new Fl_Button(X + 240, Y + 40, 60, 22, gettext("Select"));
		btn_select_gpio_ptt->callback((Fl_Callback*)cb_btn_select_gpio_ptt);

		gpio_ptt_line = new Fl_Int_Input(X + 35, Y + 90, 90, 22, gettext("On Line"));
		gpio_ptt_line->tooltip(gettext("Enter GPIO line for PTT"));
		gpio_ptt_line->type(2);
		gpio_ptt_line->align(Fl_Align(FL_ALIGN_TOP_LEFT));
		gpio_ptt_line->callback((Fl_Callback*)cb_gpio_ptt_line);

		snprintf(linenbr, sizeof(linenbr), "%d", progStatus.gpio_ptt_line);
		gpio_ptt_line->value(linenbr);

		btn_enable_gpio_ptt = new Fl_Check_Button(X + 240, Y + 90, 90, 22, gettext("Enable PTT"));
		btn_enable_gpio_ptt->tooltip(gettext("Set device & line, then enable"));
		btn_enable_gpio_ptt->down_box(FL_DOWN_BOX);
		btn_enable_gpio_ptt->callback((Fl_Callback*)cb_btn_enable_gpio_ptt);
		btn_enable_gpio_ptt->value(progStatus.enable_gpio_ptt);

//----------------------------------------------------------------------

		gpio_cw_dev = new Fl_Input(X + 35, Y + 150, 200, 22, gettext("GPIO CW device"));
		gpio_cw_dev->tooltip(gettext("Select PTT device file"));
		gpio_cw_dev->callback((Fl_Callback*)cb_gpio_cw_dev);
		gpio_cw_dev->align(Fl_Align(FL_ALIGN_TOP_LEFT));
		gpio_cw_dev->value(progStatus.gpio_cw_device.c_str());

		btn_select_gpio_cw = new Fl_Button(X + 240, Y + 150, 60, 22, gettext("Select"));
		btn_select_gpio_cw->callback((Fl_Callback*)cb_btn_select_gpio_cw);

		gpio_cw_line = new Fl_Int_Input(X + 35, Y + 195, 90, 22, gettext("On Line"));
		gpio_cw_line->tooltip(gettext("Enter GPIO line for CW"));
		gpio_cw_line->type(2);
		gpio_cw_line->align(Fl_Align(FL_ALIGN_TOP_LEFT));
		gpio_cw_line->callback((Fl_Callback*)cb_gpio_cw_line);

		snprintf(linenbr, sizeof(linenbr), "%d", progStatus.gpio_cw_line);
		gpio_cw_line->value(linenbr);

		btn_enable_gpio_cw = new Fl_Check_Button(X + 240, Y + 195, 90, 22, gettext("Enable GPIO CW"));
		btn_enable_gpio_cw->tooltip(gettext("Set device & line, then enable"));
		btn_enable_gpio_cw->down_box(FL_DOWN_BOX);
		btn_enable_gpio_cw->callback((Fl_Callback*)cb_btn_enable_gpio_cw);
		btn_enable_gpio_cw->value(progStatus.enable_gpio_cw);

	tab->end();

  return tab;
}
