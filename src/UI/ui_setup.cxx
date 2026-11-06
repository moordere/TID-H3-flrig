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

Fl_Choice *mnuScheme = (Fl_Choice *)0;

Fl_Button *pref_sys_foreground = (Fl_Button *)0;
Fl_Button *pref_sys_background = (Fl_Button *)0;
Fl_Button *prefsys_background2 = (Fl_Button *)0;
Fl_Button *prefsys_defaults = (Fl_Button *)0;

Fl_Check_Button *btn_wide_ui = (Fl_Check_Button *)0;
Fl_Check_Button *btn_narrow_ui = (Fl_Check_Button *)0;
Fl_Check_Button *btn_touch_ui = (Fl_Check_Button *)0;
Fl_Check_Button *chk_hrd_button = (Fl_Check_Button *)0;
Fl_Check_Button *chk_sliders_button = (Fl_Check_Button *)0;

static void cb_btn_ui(Fl_Check_Button* o, void*) {
	if (o == btn_wide_ui) {
		progStatus.UIsize = wide_ui;
		btn_narrow_ui->value(0);
		btn_touch_ui->value(0);
	}
	else if (o == btn_narrow_ui) {
		progStatus.UIsize = small_ui;
		btn_wide_ui->value(0);
		btn_touch_ui->value(0);
	}
	else if (o == btn_touch_ui) {
		progStatus.UIsize = touch_ui;
		btn_wide_ui->value(0);
		btn_narrow_ui->value(0);
	}
	progStatus.UIchanged = true;
}

static void cb_mnuScheme(Fl_Choice* o, void*) {
	progStatus.ui_scheme = o->text();
	Fl::scheme(progStatus.ui_scheme.c_str());
}

static void cb_pref_sys_foreground(Fl_Button*, void*) {
	cb_sys_foreground();
}

static void cb_pref_sys_background(Fl_Button*, void*) {
	cb_sys_background();
}

static void cb_prefsys_background2(Fl_Button*, void*) {
	cb_sys_background2();
}

static void cb_prefsys_defaults(Fl_Button*, void*) {
	cb_sys_defaults();
}

static void cb_btnOkDisplayDialog(Fl_Return_Button*, void*) {
	cbOKDisplayDialog();
}

static void cb_hrd_button(Fl_Check_Button*, void*) {
	cb_change_hrd_button();
}

static void cb_sliders_button(Fl_Check_Button*, void*) {
	cb_change_sliders_button();
}

Fl_Double_Window* DisplayDialog() {
	Fl_Double_Window* w = new Fl_Double_Window(SMALL_MAINW, 140, _("Flrig User Interface"));

	Fl_Group* su_grp1 = new Fl_Group(2, 2, 420, 40, _("Main Dialog Aspect Ratio (change requires restart)"));
		su_grp1->box(FL_ENGRAVED_FRAME);
		su_grp1->align(Fl_Align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE));

		btn_narrow_ui = new Fl_Check_Button(
			su_grp1->w()/2 - 160, 20, 100, 18, _("Narrow UI"));
		btn_narrow_ui->callback((Fl_Callback*)cb_btn_ui);
		btn_narrow_ui->value(progStatus.UIsize == small_ui);

		btn_wide_ui = new Fl_Check_Button(
			btn_narrow_ui->x() + 110, 20, 100, 18, _("Wide UI"));
		btn_wide_ui->callback((Fl_Callback*)cb_btn_ui);
		btn_wide_ui->value(progStatus.UIsize == wide_ui);

		btn_touch_ui = new Fl_Check_Button(
			btn_wide_ui->x() + 110, 20, 100, 18, _("Touch UI"));
		btn_touch_ui->callback((Fl_Callback*)cb_btn_ui);
		btn_touch_ui->value(progStatus.UIsize == touch_ui);

	su_grp1->end();

	Fl_Group* su_grp_hrd = new Fl_Group(
		su_grp1->x(), su_grp1->y() + su_grp1->h(),
		su_grp1->w(), 30);
		su_grp_hrd->box(FL_ENGRAVED_FRAME);
		chk_hrd_button = new Fl_Check_Button(
			10, su_grp_hrd->y() + su_grp_hrd->h()/2 - 10,
			50, 20, _("Freq top/bottom left click"));
		chk_hrd_button->value(progStatus.hrd_buttons);
		chk_hrd_button->callback((Fl_Callback*)cb_hrd_button);
		chk_hrd_button->tooltip(_("Off - left/right click paradigm"));
		
		chk_sliders_button = new Fl_Check_Button(
			su_grp_hrd->w() / 2, su_grp_hrd->y() + su_grp_hrd->h()/2 - 10,
			50, 20, _("Xcvr follows slider drag"));
		chk_sliders_button->value(progStatus.hrd_buttons);
		chk_sliders_button->callback((Fl_Callback*)cb_sliders_button);
		chk_sliders_button->value(progStatus.sliders_button == FL_WHEN_CHANGED);
		chk_sliders_button->tooltip(_("ON  send all slider changes to xcvr\nOFF wait for slider control release"));

	su_grp_hrd->end();

	Fl_Group* su_grp4 = new Fl_Group(
		su_grp_hrd->x(), su_grp_hrd->y() + su_grp_hrd->h() + 2,
		su_grp_hrd->w() - 80, 65, _("System"));
		su_grp4->box(FL_ENGRAVED_FRAME);
		su_grp4->align(Fl_Align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE));

		mnuScheme = new Fl_Choice(
			su_grp4->x() + 3, su_grp4->y() + 20, 
			80, 22, _("UI"));
		mnuScheme->tooltip(_("Change application look and feel"));
		mnuScheme->down_box(FL_BORDER_BOX);
		mnuScheme->callback((Fl_Callback*)cb_mnuScheme);
		mnuScheme->align(Fl_Align(FL_ALIGN_RIGHT));
		mnuScheme->add("base");
		mnuScheme->add("gtk+");
		mnuScheme->add("plastic");
		mnuScheme->add("gleam");
		mnuScheme->value(mnuScheme->find_item(progStatus.ui_scheme.c_str()));

		pref_sys_foreground = new Fl_Button(
			mnuScheme->x() + mnuScheme->w() + 30, su_grp4->y() + 6, 
			60, 22, _("Fgnd"));
		pref_sys_foreground->tooltip(_("Label color"));
		pref_sys_foreground->callback((Fl_Callback*)cb_pref_sys_foreground);

		pref_sys_background = new Fl_Button(
			pref_sys_foreground->x() + pref_sys_foreground->w() + 2, pref_sys_foreground->y(),
			60, 22, _("Bkgd"));
		pref_sys_background->tooltip(_("Background - normal"));
		pref_sys_background->callback((Fl_Callback*)cb_pref_sys_background);

		prefsys_background2 = new Fl_Button(
			pref_sys_foreground->x(), pref_sys_foreground->y() + pref_sys_foreground->h() + 2,
			60, 22, _("Bkgd2"));
		prefsys_background2->tooltip(_("Controls background"));
		prefsys_background2->callback((Fl_Callback*)cb_prefsys_background2);

		prefsys_defaults = new Fl_Button(
			prefsys_background2->x() + prefsys_background2->w() + 2 , prefsys_background2->y(),
			60, 22, _("Default"));
		prefsys_defaults->tooltip(_("Default system colors"));
		prefsys_defaults->callback((Fl_Callback*)cb_prefsys_defaults);

	su_grp4->end();

	btnOkDisplayDialog = new Fl_Return_Button(
		su_grp_hrd->x() + su_grp_hrd->w() - 74, prefsys_defaults->y(), 
		70, 22, _("Done"));
	btnOkDisplayDialog->tooltip(_("Save Current Changes"));
	btnOkDisplayDialog->callback((Fl_Callback*)cb_btnOkDisplayDialog);

	w->end();

	w->position( progStatus.ddX, progStatus.ddY );

	return w;
}

