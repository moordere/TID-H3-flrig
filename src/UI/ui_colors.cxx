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

#include "dialogs.h"

Fl_Group* freq_meters_group = (Fl_Group *)0;
Fl_Group* freq_group = (Fl_Group *)0;
Fl_Group* meter_group = (Fl_Group *)0;

Fl_Box *lblTest = (Fl_Box *)0;
Fl_Button *prefFont = (Fl_Button *)0;
Fl_Button *prefForeground = (Fl_Button *)0;
Fl_Button *btnBacklight = (Fl_Button *)0;
Fl_Button *btnFreqMeters_default = (Fl_Button *)0;

Fl_Wheel_Value_Slider *sldrColors = (Fl_Wheel_Value_Slider *)0;
Fl_Button *pref_slider_background = (Fl_Button *)0;
Fl_Button *pref_slider_select = (Fl_Button *)0;
Fl_Button *prefslider_defaults = (Fl_Button *)0;
Fl_Button *btnReset = (Fl_Button *)0;
Fl_Button *btnCancel = (Fl_Button *)0;
Fl_Return_Button *btnOkDisplayDialog = (Fl_Return_Button *)0;
Fl_Light_Button *btn_lighted = (Fl_Light_Button *)0;
Fl_Button *btn_lighted_default = (Fl_Button *)0;

Fl_Button *btn_tab_color = (Fl_Button *)0;
Fl_Button *btn_tab_defcolor = (Fl_Button *)0;

Fl_Button *btn_change_SMETER = (Fl_Button *)0;
Fl_Button *btn_change_PWR    = (Fl_Button *)0;
Fl_Button *btn_change_SWR    = (Fl_Button *)0;
Fl_Button *btn_change_ALC    = (Fl_Button *)0;
Fl_Button *btn_change_IDD    = (Fl_Button *)0;
Fl_Button *btn_change_VOLTS  = (Fl_Button *)0;

Fl_SigBar *color_SMETER = (Fl_SigBar *)0;
Fl_SigBar *color_PWR    = (Fl_SigBar *)0;
Fl_SigBar *color_SWR    = (Fl_SigBar *)0;
Fl_SigBar *color_ALC    = (Fl_SigBar *)0;
Fl_SigBar *color_IDD    = (Fl_SigBar *)0;
Fl_SigBar *color_VOLTS  = (Fl_SigBar *)0;

Fl_Tabs   *tabs_color = (Fl_Tabs *)0;
Fl_Group  *tab_color1 = (Fl_Group *)0;
Fl_Group  *tab_color2 = (Fl_Group *)0;

static void cb_prefFont(Fl_Button*, void*) {
	cbPrefFont();
}

static void cb_prefForeground(Fl_Button*, void*) {
	cbPrefForeground();
}

static void cb_btnBacklight(Fl_Button*, void*) {
	cbBacklightColor();
}

static void cb_btnFreqMeters_default(Fl_Button*, void*) {
	default_meters();
}

static void cb_btnSmeterColor(Fl_Button*, void*) {
	bool shift = (((Fl::event_state() & FL_SHIFT) == FL_SHIFT));
	if (shift) {
		uchar r = smeter_peak_red, g = smeter_peak_green, b = smeter_peak_blue;
		if (fl_color_chooser("Smeter peak color", r, g, b)) {
			smeter_peak_red = r; smeter_peak_green = g; smeter_peak_blue = b;
			color_SMETER->PeakColor( fl_rgb_color (r, g, b) );
			dlgColorsDialog->redraw();
		}
	} else {
		uchar r = smeterRed, g = smeterGreen, b = smeterBlue;
		if (fl_color_chooser("Smeter color", r, g, b)) {
			smeterRed = r; smeterGreen = g; smeterBlue = b;
			color_SMETER->color( fl_rgb_color (r, g, b) );
			dlgColorsDialog->redraw();
		}
	}
}

static void cb_btnSWRColor(Fl_Button*, void*) {
	bool shift = (((Fl::event_state() & FL_SHIFT) == FL_SHIFT));
	if (shift) {
		uchar r = swr_peak_red, g = swr_peak_green, b = swr_peak_blue;
		if (fl_color_chooser("SWR peak color", r, g, b)) {
			swr_peak_red = r; swr_peak_green = g; swr_peak_blue = b;
			color_SWR->PeakColor(fl_rgb_color (r, g, b));
			dlgColorsDialog->redraw();
		}
	} else {
		uchar r = swrRed, g = swrGreen, b = swrBlue;
		if (fl_color_chooser("SWR meter color", r, g, b)) {
			swrRed = r; swrGreen = g; swrBlue = b;
			color_SWR->color( fl_rgb_color (r, g, b) );
			dlgColorsDialog->redraw();
		}
	}
}

static void cb_btnPwrColor(Fl_Button*, void*) {
	bool shift = (((Fl::event_state() & FL_SHIFT) == FL_SHIFT));
	if (shift) {
		uchar r = pwr_peak_red, g = pwr_peak_green, b = pwr_peak_blue;
		if (fl_color_chooser("PWR peak color", r, g, b)) {
			pwr_peak_red = r; pwr_peak_green = g; pwr_peak_blue = b;
			color_PWR->PeakColor(fl_rgb_color (r, g, b));
			dlgColorsDialog->redraw();
		}
	} else {
		uchar r = pwrRed, g = pwrGreen, b = pwrBlue;
		if (fl_color_chooser("PWR meter color", r, g, b)) {
			pwrRed = r; pwrGreen = g; pwrBlue = b;
			color_PWR->color( fl_rgb_color (r, g, b) );
			dlgColorsDialog->redraw();
		}
	}
}

static void cb_btnALCColor(Fl_Button*, void*) {
	bool shift = (((Fl::event_state() & FL_SHIFT) == FL_SHIFT));
	if (shift) {
		uchar r = alc_peak_red, g = alc_peak_green, b = alc_peak_blue;
		if (fl_color_chooser("VOLTS peak color", r, g, b)) {
			alc_peak_red = r; alc_peak_green = g; alc_peak_blue = b;
			color_ALC->PeakColor(fl_rgb_color (r, g, b));
			dlgColorsDialog->redraw();
		}
	} else {
		uchar r = alcRed, g = alcGreen, b = alcBlue;
		if (fl_color_chooser("ALC meter color", r, g, b)) {
			alcRed = r; alcGreen = g; alcBlue = b;
			color_ALC->color( fl_rgb_color (r, g, b) );
			dlgColorsDialog->redraw();
		}
	}
}

static void cb_btnIddColor(Fl_Button*, void*) {
	bool shift = (((Fl::event_state() & FL_SHIFT) == FL_SHIFT));
	if (shift) {
		uchar r = idd_peak_red, g = idd_peak_green, b = idd_peak_blue;
		if (fl_color_chooser("IDD peak color", r, g, b)) {
			idd_peak_red = r; idd_peak_green = g; idd_peak_blue = b;
			color_IDD->PeakColor(fl_rgb_color (r, g, b));
			dlgColorsDialog->redraw();
		}
	} else {
		uchar r = iddRed, g = iddGreen, b = iddBlue;
		if (fl_color_chooser("IDD meter color", r, g, b)) {
			iddRed = r; iddGreen = g; iddBlue = b;
			color_IDD->color( fl_rgb_color (r, g, b) );
			dlgColorsDialog->redraw();
		}
	}
}

static void cb_btnVoltColor(Fl_Button*, void*) {
	bool shift = (((Fl::event_state() & FL_SHIFT) == FL_SHIFT));
	if (shift) {
		uchar r = volts_peak_red, g = volts_peak_green, b = volts_peak_blue;
		if (fl_color_chooser("VOLTS peak color", r, g, b)) {
			volts_peak_red = r; volts_peak_green = g; volts_peak_blue = b;
			color_VOLTS->PeakColor(fl_rgb_color (r, g, b));
			dlgColorsDialog->redraw();
		}
	} else {
		uchar r = voltsRed, g = voltsGreen, b = voltsBlue;
		if (fl_color_chooser("VOLTS meter color", r, g, b)) {
			voltsRed = r; voltsGreen = g; voltsBlue = b;
			color_VOLTS->color( fl_rgb_color (r, g, b) );
			dlgColorsDialog->redraw();
		}
	}
}

static void cb_pref_slider_background(Fl_Button*, void*) {
	cb_slider_background();
}

static void cb_pref_slider_select(Fl_Button*, void*) {
	cb_slider_select();
}

static void cb_prefslider_defaults(Fl_Button*, void*) {
	cb_slider_defaults();
}

static void cb_btn_lighted(Fl_Light_Button*, void*) {
	cb_lighted_button();
}

static void cb_btn_lighted_default(Fl_Button*, void*) {
	cb_lighted_default();
}

static void cb_btn_tab_color(Fl_Button *, void*) {
	cb_tab_colors();
}

static void cb_btn_tab_defcolor(Fl_Button *, void*) {
	cb_tab_defaults();
}

static void cb_btnReset(Fl_Button*, void*) {
	cb_ResetColorsDialog();
}

static void cb_btnCancel(Fl_Button*, void*) {
	cbCancelColorDialog();
}

static void cb_btnOKColorDialog(Fl_Return_Button*, void*) {
	cbOKColorDialog();
}

Fl_Double_Window* ColorsDialog() {
	Fl_Double_Window* win = new Fl_Double_Window(SMALL_MAINW, 265, _("Color Selection"));

	freq_meters_group = new Fl_Group( 2, 2, 218, 260 );
		freq_meters_group->box(FL_ENGRAVED_FRAME);

		freq_group = new Fl_Group(
			freq_meters_group->x() + 4, freq_meters_group->y() + 4,
			210, 35);
			freq_group->box(FL_DOWN_BOX);

			lblTest = new Fl_Box(
				freq_group->x() + 2, freq_group->y() + 2, 206, 31, _("14070.000"));
			lblTest->box(FL_FLAT_BOX);
			lblTest->labelfont(progStatus.fontnbr);
			lblTest->labelsize(32);
		freq_group->end();

		meter_group = new Fl_Group(
			freq_group->x(), freq_group->y() + freq_group->h() + 2, freq_group->w(), 184);
			meter_group->box(FL_DOWN_BOX);
			meter_group->color(fl_rgb_color (progStatus.bg_red, progStatus.bg_green, progStatus.bg_blue));

			btn_change_SMETER = new Fl_Button(meter_group->x() + 2, meter_group->y() + 2, 206, 20);
			btn_change_SMETER->box(FL_FLAT_BOX);
			btn_change_SMETER->image(image_smeter);
			btn_change_SMETER->color(fl_rgb_color (progStatus.bg_red, progStatus.bg_green, progStatus.bg_blue) );
			btn_change_SMETER->labelcolor(fl_rgb_color( progStatus.fg_red, progStatus.fg_green, progStatus.fg_blue ));
			btn_change_SMETER->callback((Fl_Callback*)cb_btnSmeterColor);
			btn_change_SMETER->tooltip(_("click to change Smeter color\nSHIFT click to change peak color"));

			color_SMETER = new Fl_SigBar(meter_group->x() + 5, btn_change_SMETER->y() + btn_change_SMETER->h() + 2, 200, 6);
			color_SMETER->box(FL_FLAT_BOX);
			color_SMETER->labeltype(FL_NORMAL_LABEL);
			color_SMETER->labelfont(0);
			color_SMETER->labelsize(12);
			color_SMETER->labelcolor(FL_FOREGROUND_COLOR);
			color_SMETER->align(Fl_Align(FL_ALIGN_CENTER));
			color_SMETER->when(FL_WHEN_CHANGED);
			color_SMETER->hide();
			color_SMETER->color(fl_rgb_color (progStatus.smeterRed, progStatus.smeterGreen, progStatus.smeterBlue) );
			color_SMETER->PeakColor(fl_rgb_color(progStatus.smeter_peak_red, progStatus.smeter_peak_green, progStatus.smeter_peak_blue) );
			color_SMETER->selection_color(FL_BACKGROUND_COLOR);
			color_SMETER->show();

			color_PWR = new Fl_SigBar(meter_group->x() + 5, color_SMETER->y() + color_SMETER->h() + 2, 200, 6);
			color_PWR->box(FL_FLAT_BOX);
			color_PWR->labeltype(FL_NORMAL_LABEL);
			color_PWR->labelfont(0);
			color_PWR->labelsize(12);
			color_PWR->labelcolor(FL_FOREGROUND_COLOR);
			color_PWR->align(Fl_Align(FL_ALIGN_CENTER));
			color_PWR->when(FL_WHEN_CHANGED);
			color_PWR->hide();
			color_PWR->color(fl_rgb_color (progStatus.pwrRed, progStatus.pwrGreen, progStatus.pwrBlue) );
			color_PWR->PeakColor(fl_rgb_color(progStatus.pwr_peak_red, progStatus.pwr_peak_green, progStatus.pwr_peak_blue) );
			color_PWR->selection_color(FL_BACKGROUND_COLOR);
			color_PWR->show();

			btn_change_PWR = new Fl_Button(meter_group->x() + 2, color_PWR->y() + color_PWR->h() + 2, 206, 20);
			btn_change_PWR->box(FL_FLAT_BOX);
			btn_change_PWR->tooltip(_("click to change power color\nSHIFT click to change peak color"));
			btn_change_PWR->image(image_p100);
			btn_change_PWR->color(fl_rgb_color( progStatus.bg_red, progStatus.bg_green, progStatus.bg_blue ));
			btn_change_PWR->labelcolor(fl_rgb_color( progStatus.fg_red, progStatus.fg_green, progStatus.fg_blue ));
			btn_change_PWR->callback((Fl_Callback*)cb_btnPwrColor);

			btn_change_SWR = new Fl_Button(meter_group->x() + 2, btn_change_PWR->y() + btn_change_PWR->h() + 2, 206, 20);
			btn_change_SWR->box(FL_FLAT_BOX);
			btn_change_SWR->image(image_swr);
			btn_change_SWR->tooltip(_("click to change SWR color\nSHIFT click to change peak color"));
			btn_change_SWR->color(fl_rgb_color( progStatus.bg_red, progStatus.bg_green, progStatus.bg_blue ));
			btn_change_SWR->labelcolor(fl_rgb_color( progStatus.fg_red, progStatus.fg_green, progStatus.fg_blue ));
			btn_change_SWR->callback((Fl_Callback*)cb_btnSWRColor);

			color_SWR = new Fl_SigBar(meter_group->x() + 5, btn_change_SWR->y() + btn_change_SWR->h() + 2, 200, 6);
			color_SWR->box(FL_FLAT_BOX);
			color_SWR->labeltype(FL_NORMAL_LABEL);
			color_SWR->labelfont(0);
			color_SWR->labelsize(12);
			color_SWR->labelcolor(FL_FOREGROUND_COLOR);
			color_SWR->align(Fl_Align(FL_ALIGN_CENTER));
			color_SWR->when(FL_WHEN_CHANGED);
			color_SWR->hide();
			color_SWR->color(fl_rgb_color (progStatus.swrRed, progStatus.swrGreen, progStatus.swrBlue) );
			color_SWR->PeakColor(fl_rgb_color(progStatus.swr_peak_red, progStatus.swr_peak_green, progStatus.swr_peak_blue) );
			color_SWR->selection_color(FL_BACKGROUND_COLOR);
			color_SWR->show();

			btn_change_ALC = new Fl_Button(meter_group->x() + 2, color_SWR->y() + color_SWR->h() + 2, 206, 20);
			btn_change_ALC->box(FL_FLAT_BOX);
			if (selrig->name_ == rig_PowerSDR.name_) {
				btn_change_ALC->image(image_alc40db);
			}
			else {
				btn_change_ALC->image(image_alc);
			}
			btn_change_ALC->color(fl_rgb_color( progStatus.bg_red, progStatus.bg_green, progStatus.bg_blue ));
			btn_change_ALC->labelcolor(fl_rgb_color( progStatus.fg_red, progStatus.fg_green, progStatus.fg_blue ));
			btn_change_ALC->tooltip(_("click to change ALC color\nSHIFT click to change peak color"));
			btn_change_ALC->callback((Fl_Callback*)cb_btnALCColor);

			color_ALC = new Fl_SigBar(meter_group->x() + 5, btn_change_ALC->y() + btn_change_ALC->h() + 2, 200, 6);
			color_ALC->box(FL_FLAT_BOX);
			color_ALC->labeltype(FL_NORMAL_LABEL);
			color_ALC->labelfont(0);
			color_ALC->labelsize(12);
			color_ALC->labelcolor(FL_FOREGROUND_COLOR);
			color_ALC->align(Fl_Align(FL_ALIGN_CENTER));
			color_ALC->when(FL_WHEN_CHANGED);
			color_ALC->hide();
			color_ALC->color(fl_rgb_color (progStatus.alcRed, progStatus.alcGreen, progStatus.alcBlue) );
			color_ALC->PeakColor(fl_rgb_color(progStatus.alc_peak_red, progStatus.alc_peak_green, progStatus.alc_peak_blue) );
			color_ALC->selection_color(FL_BACKGROUND_COLOR);
			color_ALC->show();

			btn_change_IDD = new Fl_Button(meter_group->x() + 2, color_ALC->y() + color_ALC->h() + 2, 206, 20);
			btn_change_IDD->box(FL_FLAT_BOX);
			btn_change_IDD->image(image_idd25);
			btn_change_IDD->color(fl_rgb_color( progStatus.bg_red, progStatus.bg_green, progStatus.bg_blue ));
			btn_change_IDD->labelcolor(fl_rgb_color( progStatus.fg_red, progStatus.fg_green, progStatus.fg_blue ));
			btn_change_IDD->tooltip(_("click to change IDD color\nSHIFT click to change peak color"));
			btn_change_IDD->callback((Fl_Callback*)cb_btnIddColor);

			color_IDD = new Fl_SigBar(meter_group->x() + 5, btn_change_IDD->y() + btn_change_IDD->h() + 2, 200, 6);
			color_IDD->box(FL_FLAT_BOX);
			color_IDD->labeltype(FL_NORMAL_LABEL);
			color_IDD->labelfont(0);
			color_IDD->labelsize(12);
			color_IDD->labelcolor(FL_FOREGROUND_COLOR);
			color_IDD->align(Fl_Align(FL_ALIGN_CENTER));
			color_IDD->when(FL_WHEN_CHANGED);
			color_IDD->hide();
			color_IDD->color(fl_rgb_color (progStatus.iddRed, progStatus.iddGreen, progStatus.iddBlue) );
			color_IDD->PeakColor(fl_rgb_color(progStatus.idd_peak_red, progStatus.idd_peak_green, progStatus.idd_peak_blue) );
			color_IDD->selection_color(FL_BACKGROUND_COLOR);
			color_IDD->show();

			color_VOLTS = new Fl_SigBar(meter_group->x() + 5, color_IDD->y() + color_IDD->h() + 2, 200, 6);
			color_VOLTS->box(FL_FLAT_BOX);
			color_VOLTS->labeltype(FL_NORMAL_LABEL);
			color_VOLTS->labelfont(0);
			color_VOLTS->labelsize(12);
			color_VOLTS->labelcolor(FL_FOREGROUND_COLOR);
			color_VOLTS->align(Fl_Align(FL_ALIGN_CENTER));
			color_VOLTS->when(FL_WHEN_CHANGED);
			color_VOLTS->hide();
			color_VOLTS->color(fl_rgb_color (progStatus.voltsRed, progStatus.voltsGreen, progStatus.voltsBlue) );
			color_VOLTS->PeakColor(fl_rgb_color( progStatus.volts_peak_red, progStatus.volts_peak_green, progStatus.volts_peak_blue ));
			color_VOLTS->selection_color(FL_BACKGROUND_COLOR);
			color_VOLTS->show();

			btn_change_VOLTS = new Fl_Button(meter_group->x() + 2, color_VOLTS->y() + color_VOLTS->h() + 2, 206, 20);
			btn_change_VOLTS->box(FL_FLAT_BOX);
			btn_change_VOLTS->image(image_voltmeter);
			btn_change_VOLTS->color(fl_rgb_color( progStatus.bg_red, progStatus.bg_green, progStatus.bg_blue ));
			btn_change_VOLTS->labelcolor(fl_rgb_color( progStatus.fg_red, progStatus.fg_green, progStatus.fg_blue ));
			btn_change_VOLTS->tooltip(_("click to change VOLTS color\nSHIFT click to change peak color"));
			btn_change_VOLTS->callback((Fl_Callback*)cb_btnVoltColor);

			color_SMETER->minimum(0);
			color_SMETER->maximum(100);
			color_SMETER->aging(1);
			color_SMETER->avg(1);
			color_SMETER->value(45);

			color_PWR->minimum(0);
			color_PWR->maximum(100);
			color_PWR->aging(1);
			color_PWR->avg(1);
			color_PWR->value(35);

			color_SWR->minimum(0);
			color_SWR->maximum(100);
			color_SWR->aging(1);
			color_SWR->avg(1);
			color_SWR->value(25);

			color_VOLTS->minimum(0);
			color_VOLTS->maximum(16);
			color_VOLTS->aging(1);
			color_VOLTS->avg(1);
			color_VOLTS->value(12);

			color_ALC->minimum(0);
			color_ALC->maximum(5);
			color_ALC->aging(1);
			color_ALC->avg(1);
			color_ALC->value(1);

			color_IDD->minimum(0);
			color_IDD->maximum(25);
			color_IDD->aging(1);
			color_IDD->avg(1);
			color_IDD->value(14);

		meter_group->end();

		prefFont = new Fl_Button(
			meter_group->x(), meter_group->y() + meter_group->h() + 2,
			47, 20, _("Font"));
		prefFont->tooltip("Frequency Font");
		prefFont->callback((Fl_Callback*)cb_prefFont);

		prefForeground = new Fl_Button(
			prefFont->x() + prefFont->w() + 2, prefFont->y(),
			prefFont->w(), prefFont->h(), _("Color"));
		prefForeground->tooltip("Freq/Meters font Color");
		prefForeground->callback((Fl_Callback*)cb_prefForeground);

		btnBacklight = new Fl_Button(
			prefForeground->x() + prefForeground->w() + 2, prefForeground->y(),
			prefFont->w(), prefFont->h(), _("Bkgd"));
		btnBacklight->tooltip("Freq/Meters Background");
		btnBacklight->callback((Fl_Callback*)cb_btnBacklight);

		btnFreqMeters_default = new Fl_Button(
			btnBacklight->x() + btnBacklight->w() + 2, prefFont->y(),
			prefFont->w() + 14, prefFont->h(), _("Default"));
		btnFreqMeters_default->tooltip("Freq/Meters Defaults");
		btnFreqMeters_default->callback((Fl_Callback*)cb_btnFreqMeters_default);

	freq_meters_group->end();

	Fl_Group* su_grp5 = new Fl_Group(
		freq_meters_group->x() + freq_meters_group->w() + 2, freq_meters_group->y(),
		win->w() -(freq_meters_group->x() + freq_meters_group->w() + 4) , 60);
		su_grp5->box(FL_ENGRAVED_FRAME);

		sldrColors = new Fl_Wheel_Value_Slider(
			su_grp5->x() + 4, su_grp5->y() + 4, 
			su_grp5->w() - 8, 20);
		sldrColors->type(5);
		sldrColors->box(FL_THIN_DOWN_BOX);
		sldrColors->color(FL_BACKGROUND_COLOR);
		sldrColors->selection_color(FL_BACKGROUND_COLOR);
		sldrColors->labeltype(FL_NORMAL_LABEL);
		sldrColors->labelfont(0);
		sldrColors->labelsize(14);
		sldrColors->labelcolor(FL_FOREGROUND_COLOR);
		sldrColors->maximum(100);
		sldrColors->step(1);
		sldrColors->value(15);
		sldrColors->textsize(12);
		sldrColors->align(Fl_Align(FL_ALIGN_CENTER));
		sldrColors->when(FL_WHEN_CHANGED);
        sldrColors->reverse(true);

		pref_slider_background = new Fl_Button(
			sldrColors->x(), sldrColors->y() + sldrColors->h() + 6,
			60, 20, _("Bkgd"));
		pref_slider_background->tooltip(_("Background - normal"));
		pref_slider_background->callback((Fl_Callback*)cb_pref_slider_background);

		pref_slider_select = new Fl_Button(
			pref_slider_background->x() + pref_slider_background->w() + 2, 
			pref_slider_background->y(),
			60, 20, _("Button"));
		pref_slider_select->tooltip(_("Background - normal"));
		pref_slider_select->callback((Fl_Callback*)cb_pref_slider_select);

		prefslider_defaults = new Fl_Button(
			pref_slider_select->x() + pref_slider_select->w() + 2, 
			pref_slider_select->y(),
			60, 20, _("Default"));
		prefslider_defaults->tooltip(_("Background - selected"));
		prefslider_defaults->callback((Fl_Callback*)cb_prefslider_defaults);

	su_grp5->end();

	Fl_Group* su_grp6 = new Fl_Group(
		su_grp5->x(), su_grp5->y() + su_grp5->h() + 2, su_grp5->w(), 40);
		su_grp6->box(FL_ENGRAVED_FRAME);

		btn_lighted = new Fl_Light_Button(
			su_grp6->x() + 25, su_grp6->y() + 10, 
			60, 20, _("Lt Btn"));
		btn_lighted->callback((Fl_Callback*)cb_btn_lighted);

		btn_lighted_default = new Fl_Button(
			btn_lighted->x() + btn_lighted->w() + 2, btn_lighted->y(), 
			60, 20, _("Default"));
		btn_lighted_default->tooltip(_("Background - selected"));
		btn_lighted_default->callback((Fl_Callback*)cb_btn_lighted_default);

	su_grp6->end();

	Fl_Group* su_grp7 = new Fl_Group(
		su_grp6->x(), su_grp6->y() + su_grp6->h() + 2, su_grp6->w(), 80);
		su_grp7->box(FL_ENGRAVED_FRAME);

		Fl_Color tc = fl_rgb_color(progStatus.tab_red, progStatus.tab_green, progStatus.tab_blue);
		tabs_color = new Fl_Tabs(su_grp7->x() + 2, su_grp7->y() + 4, su_grp7->w() - 8, su_grp7->h() - 8);
			tabs_color->selection_color(tc);

			tab_color1 = new Fl_Group(tabs_color->x(), tabs_color->y() + 20, tabs_color->w(), tabs_color->h() - 20, _("Tab 1"));
				btn_tab_color = new Fl_Button(
					tab_color1->x() + 10, tab_color1->y() + 20, 80, 20, _("Tab Color"));
				btn_tab_color->callback((Fl_Callback *)cb_btn_tab_color);

				btn_tab_defcolor = new Fl_Button(
					btn_tab_color->x() + btn_tab_color->w() + 5, btn_tab_color->y(), 80, 20, _("Def Color"));
				btn_tab_defcolor->callback((Fl_Callback *)cb_btn_tab_defcolor);
			tab_color1->end();

			tab_color2 = new Fl_Group(tabs_color->x(), tabs_color->y() + 20, tabs_color->w(), tabs_color->h() - 20, _("Tab 2"));
			tab_color2->end();

		tabs_color->end();

	su_grp7->end();

	btnReset = new Fl_Button(
		su_grp7->x() + 2, prefFont->y(),
		60, 20, _("Reset"));
	btnReset->tooltip(_("Restore all flrig defaults"));
	btnReset->callback((Fl_Callback*)cb_btnReset);

	btnCancel = new Fl_Button(
		btnReset->x() + btnReset->w() + 4, btnReset->y(), 
		60, 20, _("Cancel"));
	btnCancel->tooltip(_("Discard current changes"));
	btnCancel->callback((Fl_Callback*)cb_btnCancel);

	btnOkDisplayDialog = new Fl_Return_Button(
		btnCancel->x() + btnCancel->w() + 4, btnReset->y(), 
		60, 20, _("OK"));
	btnOkDisplayDialog->tooltip(_("Use & Save Current Changes"));
	btnOkDisplayDialog->callback((Fl_Callback*)cb_btnOKColorDialog);

	win->end();

	win->position( progStatus.ddX, progStatus.ddY );

	return win;
}
