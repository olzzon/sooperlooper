/*
** Copyright (C) 2004 Jesse Chappell <jesse@essej.net>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**  
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**  
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**  
*/

#include <wx/wx.h>
#include <wx/file.h>
#include <wx/filename.h>

#include <iostream>
#include <cstring>
#include <cmath>

#include "main_panel.hpp"
#include "keyboard_target.hpp"
#include "looper_panel.hpp"
#include "pix_button.hpp"
#include "loop_control.hpp"
#include "time_panel.hpp"
#include "slider_bar.hpp"
#include "choice_box.hpp"
#include "check_box.hpp"
#include "spin_box.hpp"

#include "pixmap_includes.hpp"

#include <midi_bind.hpp>

using namespace SooperLooper;
using namespace SooperLooperGui;
using namespace std;


enum {
	ID_UndoButton = 8000,
	ID_RedoButton,
	ID_RecordButton,
	ID_OverdubButton,
	ID_MultiplyButton,
	ID_MuteButton,
	ID_PauseButton,
	ID_SoloButton,
	ID_LoadButton,
	ID_SaveButton,
	
	ID_ThreshControl,
	ID_FeedbackControl,
	ID_DryControl,
	ID_WetControl,
	ID_ScratchControl,
	ID_RateControl,
	ID_StretchControl,
	ID_PitchControl,
	ID_InputGainControl,

	ID_InputLatency,
	ID_OutputLatency,
	ID_TriggerLatency,
	
	ID_QuantizeCheck,
	ID_QuantizeChoice,
	ID_RoundCheck,
	ID_SyncCheck,
	ID_UseFeedbackPlayCheck,
	ID_TempoStretchCheck,
	ID_PlaySyncCheck,
	ID_UseMainInCheck,
	ID_Panner,
	ID_PrefaderCheck,
	ID_NameText,

	ID_FlashTimer

};

enum {
	FlashRate = 200
};


BEGIN_EVENT_TABLE(LooperPanel, wxPanel)

	EVT_TIMER(ID_FlashTimer, LooperPanel::on_flash_timer)
	EVT_TEXT_ENTER (ID_NameText, LooperPanel::on_text_event)

END_EVENT_TABLE()

	LooperPanel::LooperPanel(MainPanel * mainpan, LoopControl * control, wxWindow * parent, wxWindowID id, const wxPoint& pos, const wxSize& size)
	: wxPanel(parent, id, pos, size), _loop_control(control), _index(0), _last_state(LooperStateUnknown), _tap_val(1.0f)
{
	_mainpanel = mainpan;
	_learning = false;
	_scratch_pressed = false;
	_last_state = LooperStateUnknown;
	_chan_count = 0;
	_panners = 0;
	_has_discrete_io = false;
	_waiting = 0;
	_flashing_button = 0;

	_flash_timer = new wxTimer(this, ID_FlashTimer);
	
	init();
}

LooperPanel::~LooperPanel()
{

}


void
LooperPanel::init()
{
	SetBackgroundColour (*wxBLACK);
	SetThemeEnabled(false);

	wxBoxSizer * mainSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer * mainVSizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer * colsizer = new wxBoxSizer(wxVERTICAL);

	// These are used when more than one column is needed within one of the 4 columns 
	wxBoxSizer * subColSizer = new wxBoxSizer(wxVERTICAL);	
	wxBoxSizer * subRowSizer = new wxBoxSizer(wxHORIZONTAL);

	SliderBar *slider;
	wxFont sliderFont = *wxSMALL_FONT;
	wxSize sliderMinSize(200, 40);


	// add selbar
	_bgcolor.Set(0,0,0);
	_selbgcolor.Set(244, 255, 158);
	_learnbgcolor.Set(134, 80, 158);
	_barGreen.Set(10, 200, 10);

	_leftSelbar = new wxPanel(this, -1, wxDefaultPosition, wxSize(4,-1));
	_leftSelbar->SetThemeEnabled(false);
	_leftSelbar->SetBackgroundColour (_bgcolor);


	_rightSelbar = new wxPanel(this, -1, wxDefaultPosition, wxSize(4,-1));
	_rightSelbar->SetThemeEnabled(false);
	_rightSelbar->SetBackgroundColour (_bgcolor);

	_topSelbar = new wxPanel(this, -1, wxDefaultPosition, wxSize(-1,4));
	_topSelbar->SetThemeEnabled(false);
	_topSelbar->SetBackgroundColour (_bgcolor);

	_bottomSelbar = new wxPanel(this, -1, wxDefaultPosition, wxSize(-1,4));
	_bottomSelbar->SetThemeEnabled(false);
	_bottomSelbar->SetBackgroundColour (_bgcolor);

	
	mainVSizer->Add (_topSelbar, 0, wxEXPAND|wxBOTTOM|wxLEFT, 0);
	mainSizer->Add (_leftSelbar, 0, wxEXPAND|wxBOTTOM|wxLEFT, 0);


	// create all buttons first, then add them to sizers
	// must do this because the bitmaps need to be loaded
	// before adding to sizer
	create_buttons();
	
	int edgegap = 0;

	// ****** 1.Row
	colsizer = new wxBoxSizer(wxVERTICAL);
	subRowSizer = new wxBoxSizer(wxHORIZONTAL);
	
	// ****** 1.SubRow
	subColSizer = new wxBoxSizer(wxVERTICAL);
 	subColSizer->Add (_undo_button, 0, wxTOP, 3);
 	subColSizer->Add (_redo_button, 0, wxTOP, 3);
	subRowSizer->Add (subColSizer, 0, wxTop, 0);

	// ****** 2.SubRow
	subColSizer = new wxBoxSizer(wxVERTICAL);	

	subColSizer->Add (_record_button, 0, wxLEFT | wxTOP, 3);
	subColSizer->Add (_overdub_button, 0, wxLEFT | wxTOP | wxBOTTOM, 3);
	subColSizer->Add (_multiply_button, 0, wxLEFT, 3);
	subRowSizer->Add (subColSizer, 0, wxTop, 0);

	colsizer->Add (subRowSizer, 0, wxTop| wxBOTTOM, 5);

	
	// ******** 1.full row - Input gain + meter
	wxBoxSizer * inthresh_sizer = new wxBoxSizer(wxHORIZONTAL);

	_in_gain_control = slider = new SliderBar(this, ID_InputGainControl, 0.0f, 1.0f, 0.0f, true, wxDefaultPosition, wxSize(100, 40));
	slider->set_units(wxT(""));
	slider->set_label(wxT("In Gain"));
	slider->set_show_indicator_bar (false);
	slider->set_scale_mode(SliderBar::ZeroGainMode);
	slider->set_style (SliderBar::FromLeftStyle);
	slider->SetFont(sliderFont);
	slider->value_changed.connect (sigc::bind(mem_fun (*this, &LooperPanel::slider_events), (int) slider->GetId()));
	slider->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::control_bind_events), (int) slider->GetId()));
	inthresh_sizer->Add (slider, 0, wxLEFT|wxBOTTOM, 0);
	
	_thresh_control = slider = new SliderBar(this, ID_ThreshControl, 0.0f, 1.0f, 0.0f, true, wxDefaultPosition, wxSize(100, 40));
	slider->set_units(wxT(""));
	slider->set_label(wxT("Thresh"));
	slider->set_show_indicator_bar (true);
	slider->set_scale_mode(SliderBar::ZeroGainMode);
	slider->set_style (SliderBar::FromLeftStyle);
	slider->SetFont(sliderFont);
	slider->value_changed.connect (sigc::bind(mem_fun (*this, &LooperPanel::slider_events), (int) slider->GetId()));
	slider->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::control_bind_events), (int) slider->GetId()));
	inthresh_sizer->Add (slider, 0, wxLEFT|wxBOTTOM, 3);
	
	colsizer->Add (inthresh_sizer, 1, wxLEFT|wxBOTTOM, 0);
	
	// Add to main sizer:
	mainSizer->Add (colsizer, 0, wxEXPAND|wxBOTTOM, 5);


	// **************** 2.Row   time area
	colsizer = new wxBoxSizer(wxVERTICAL);

	_time_panel = new TimePanel(_loop_control, this, -1);
	_time_panel->set_index (_index);
	
	colsizer->Add (_time_panel, 0, wxLEFT, 5);
	
	_botpansizer = new wxBoxSizer(wxHORIZONTAL);
	// Position
	_loop_position = slider = new SliderBar(this, ID_ScratchControl, 0.0f, 1.0f, 0.0f, true, wxDefaultPosition, wxSize(210, 80));
	slider->set_units(wxT(""));
	slider->set_label(wxT("Position"));
	slider->set_style (SliderBar::FromLeftStyle);
	slider->set_decimal_digits (3);
	slider->set_show_value(false);
	slider->set_indicator_bar_color(wxColour(255, 255, 0));
	slider->set_show_indicator_bar (true);
	slider->SetFont(sliderFont);
	slider->value_changed.connect (sigc::bind(mem_fun (*this, &LooperPanel::slider_events), (int) slider->GetId()));
	slider->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::control_bind_events), (int) slider->GetId()));

	_botpansizer->Add (slider, 1, wxTOP, 20);

	colsizer->Add (_botpansizer, 1, wxBOTTOM | wxLEFT, 4);
	
	
	mainSizer->Add (colsizer, 0, wxEXPAND|wxLEFT | wxRIGHT | wxBOTTOM, 5);


	// ***** 3.Row

	colsizer = new wxBoxSizer(wxVERTICAL);
	subRowSizer = new wxBoxSizer(wxHORIZONTAL);

	// ******* 3.Row - 1.Sub Row:
	subColSizer = new wxBoxSizer(wxVERTICAL);	
	
	_sync_check = new CheckBox(this, ID_SyncCheck, wxT("sync"), true, wxDefaultPosition, wxSize(100, 24));
	_sync_check->SetFont(sliderFont);
	_sync_check->SetToolTip(wxT("sync operations to quantize source"));
	_sync_check->value_changed.connect (sigc::bind(mem_fun (*this, &LooperPanel::check_events), wxT("sync")));
	_sync_check->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::control_bind_events), (int) _sync_check->GetId()));
	subColSizer->Add (_sync_check, 1, wxLEFT, 0);

	subRowSizer = new wxBoxSizer(wxHORIZONTAL);
	_play_sync_check = new CheckBox(this, ID_PlaySyncCheck, wxT("play sync"), true, wxDefaultPosition, wxSize(100, 24));
	_play_sync_check->SetFont(sliderFont);
	_play_sync_check->SetToolTip(wxT("sync playback auto-triggering to quantized sync source"));
	_play_sync_check->value_changed.connect (sigc::bind(mem_fun (*this, &LooperPanel::check_events), wxT("playback_sync")));
	_play_sync_check->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::control_bind_events), (int) _play_sync_check->GetId()));
	subColSizer->Add (_play_sync_check, 1, wxLEFT, 0);
    
	_prefader_check = new CheckBox(this, ID_PrefaderCheck, wxT("prefader"), true, wxDefaultPosition, wxSize(100, 18));
	_prefader_check->SetFont(sliderFont);
	_prefader_check->SetToolTip(wxT("discrete outputs are pre-fader"));
	_prefader_check->value_changed.connect (sigc::bind(mem_fun (*this, &LooperPanel::check_events), wxT("discrete_prefader")));
	_prefader_check->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::control_bind_events), (int) _prefader_check->GetId()));
	subColSizer->Add (_prefader_check, 1, wxLEFT, 0);

  	_play_feed_check = new CheckBox(this, ID_UseFeedbackPlayCheck, wxT("p. feedb"), true, wxDefaultPosition, wxSize(100, 18));
	_play_feed_check->SetFont(sliderFont);
	_play_feed_check->SetToolTip(wxT("enable feedback during playback"));
	_play_feed_check->value_changed.connect (sigc::bind(mem_fun (*this, &LooperPanel::check_events), wxT("use_feedback_play")));
	_play_feed_check->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::control_bind_events), (int) _play_feed_check->GetId()));
	subColSizer->Add (_play_feed_check, 1, wxLEFT, 0);

	_tempo_stretch_check = new CheckBox(this, ID_TempoStretchCheck, wxT("t. stretch"), true, wxDefaultPosition, wxSize(100, 18));
	_tempo_stretch_check->SetFont(sliderFont);
	_tempo_stretch_check->SetToolTip(wxT("enable automatic timestretch when tempo changes"));
	_tempo_stretch_check->value_changed.connect (sigc::bind(mem_fun (*this, &LooperPanel::check_events), wxT("tempo_stretch")));
	_tempo_stretch_check->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::control_bind_events), (int) _tempo_stretch_check->GetId()));
	subColSizer->Add (_tempo_stretch_check, 1, wxLEFT, 0);

	subRowSizer->Add (subColSizer, 0, wxLEFT, 0);

	// ********** 3 Row - 2.SubRow	
	subColSizer = new wxBoxSizer(wxVERTICAL);	
  	
	_name_text = new wxTextCtrl(this, ID_NameText, wxT(""), wxDefaultPosition, wxSize(200, 24), wxTE_PROCESS_ENTER|wxTE_LEFT, wxDefaultValidator, wxT("KeyAware"));
	_name_text->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
	_name_text->SetToolTip(wxT("loop name"));
	_name_text->SetFont(sliderFont);
	subColSizer->Add (_name_text, 1, wxLEFT | wxTop, 0);

	// In Mon & Panners:
	_toppansizer = new wxBoxSizer(wxHORIZONTAL);
	subColSizer->Add (_toppansizer, 1, wxTOP|wxLEFT, 3);
	// panners are added later
	
	subRowSizer->Add (subColSizer, 1, wxTop, 5);
	colsizer->Add (subRowSizer, 1, wxLEFT | wxTop, 5);

	// ***** Full width 3rd Row
	// Output meter
	_wet_control = slider = new SliderBar(this, ID_WetControl, 0.0f, 1.0f, 1.0f, true, wxDefaultPosition, wxSize(200, 40));
	slider->set_units(wxT("dB"));
	slider->set_label(wxT("Output Level"));
	slider->set_show_indicator_bar (true);
	slider->set_scale_mode(SliderBar::ZeroGainMode);
	slider->set_style (SliderBar::FromLeftStyle);
	slider->SetFont(sliderFont);
	slider->value_changed.connect (sigc::bind(mem_fun (*this, &LooperPanel::slider_events), (int) slider->GetId()));
	slider->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::control_bind_events), (int) slider->GetId()));

	colsizer->Add (slider, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

	// Add 3th row to mainsize:
	mainSizer->Add (colsizer, 1, wxLEFT, 5);


	//****** 4.Row

	// Mute, Solo & Pause
	colsizer = new wxBoxSizer(wxVERTICAL);

	colsizer->Add (_mute_button, 0, wxTOP | wxRIGHT, 3);
	colsizer->Add (muteButton, 0, wxTOP | wxRIGHT, 3);
	colsizer->Add (_solo_button, 0, wxTOP | wxRIGHT, 3);
	colsizer->Add (_pause_button, 0, wxTOP | wxRIGHT, 3);
	colsizer->Add (_load_button, 0, wxTOP, 3);
	colsizer->Add (_save_button, 0, wxTOP, 3);


	mainSizer->Add (colsizer, 0, wxEXPAND | wxBOTTOM | wxRIGHT, 5);


	// Add final things:
	mainSizer->Add (_rightSelbar, 0, wxEXPAND|wxLEFT, 0);
	mainVSizer->Add (mainSizer, 1, wxEXPAND, 0);
	mainVSizer->Add (_bottomSelbar, 0, wxEXPAND|wxLEFT, 0);
	
	bind_events();
		
	this->SetAutoLayout( true );     // tell dialog to use sizer
	this->SetSizer( mainVSizer );      // actually set the sizer
	mainVSizer->Fit( this );            // set size to minimum size as calculated by the sizer
	mainVSizer->SetSizeHints( this );   // set size hints to honour mininum size

}

void
LooperPanel::post_init()
{
	// now we have channel count
	SliderBar * slider;
	wxFont sliderFont = *wxSMALL_FONT;
	
	_panners = new SliderBar*[_chan_count];
	int barwidth = _chan_count <= 4 ? 50 : 30;

	// without discrete i/o mains are the only option
	float val;
	if (_loop_control->get_value(_index, wxT("has_discrete_io"), val) && val != 0.0f)
	{
		_has_discrete_io = true;

		// dry is only meaningful with discrete io
		_dry_control = slider = new SliderBar(this, ID_DryControl, 0.0f, 1.0f, 1.0f, true, wxDefaultPosition, wxSize(100, 40));
		slider->set_units(wxT("dB"));
		slider->set_label(wxT("In Mon"));
		slider->set_scale_mode(SliderBar::ZeroGainMode);
		slider->set_style (SliderBar::FromLeftStyle);
		slider->SetFont(sliderFont);
		slider->value_changed.connect (sigc::bind(mem_fun (*this, &LooperPanel::slider_events), (int) slider->GetId()));
		slider->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::control_bind_events), (int) slider->GetId()));
		_toppansizer->Add (slider, 1, wxEXPAND, 0);

	}
	else {
		_has_discrete_io = false;
		_dry_control = 0;
	}
	
	
	for (int i=0; i < _chan_count; ++i)
	{
		float defval = 0.5f;
		if (_chan_count == 2) {
			defval = (i == 0) ? 0.0f : 1.0f;
		}
		
		_panners[i] = slider =  new SliderBar(this, ID_Panner, 0.0f, 1.0f, defval, true, wxDefaultPosition, wxSize(barwidth,40));
		slider->set_units(wxT(""));
		if (_chan_count > 1) {
			slider->set_label(wxString::Format(wxT("Pan %d"), i+1));
		}
		else {
			slider->set_label(wxString::Format(wxT("Pan")));
		}
		slider->set_style (SliderBar::CenterStyle);
		slider->set_decimal_digits (3);
		slider->set_show_value (false);
		slider->SetFont(sliderFont);
		slider->value_changed.connect (sigc::bind(mem_fun (*this, &LooperPanel::pan_events), (int) i));
		slider->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::pan_bind_events), (int) i));

		if (!_has_discrete_io) {
			_toppansizer->Add (slider, 1, (i==0) ? wxEXPAND : wxEXPAND|wxLEFT, 2);
		}
		else if (_chan_count <= 2 || i < (int) ceil(_chan_count*0.5)) {
			_toppansizer->Add (slider, 0, wxEXPAND|wxLEFT, 2);
		}
		else {
			_botpansizer->Add (slider, 0, wxEXPAND|wxLEFT, 2);
		}

	}

	_toppansizer->Layout();
	_botpansizer->Layout();
	GetSizer()->Layout();
	Refresh(false);
}

void
LooperPanel::set_selected (bool flag)
{
	if (flag) {

		_leftSelbar->SetBackgroundColour (_selbgcolor);
		_rightSelbar->SetBackgroundColour (_selbgcolor);
		_bottomSelbar->SetBackgroundColour (_selbgcolor);
		_topSelbar->SetBackgroundColour (_selbgcolor);
		//this->SetBackgroundColour (_selbgcolor);
	}
	else {
		_leftSelbar->SetBackgroundColour (_bgcolor);
		_rightSelbar->SetBackgroundColour (_bgcolor);
		_topSelbar->SetBackgroundColour (_bgcolor);
		_bottomSelbar->SetBackgroundColour (_bgcolor);
		//this->SetBackgroundColour (_bgcolor);
	}

	_leftSelbar->Refresh();
	_rightSelbar->Refresh();
	_bottomSelbar->Refresh();
	_topSelbar->Refresh();
}

void
LooperPanel::set_index(int ind)
{
	_index = ind;
	_time_panel->set_index (_index);
}


void
LooperPanel::bind_events()
{
	_undo_button->pressed.connect (sigc::bind(mem_fun (*this, &LooperPanel::pressed_events), wxString(wxT("undo"))));
	_undo_button->released.connect (sigc::bind(mem_fun (*this, &LooperPanel::released_events), wxString(wxT("undo"))));
	_undo_button->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::button_bind_events), wxString(wxT("undo"))));

	_redo_button->pressed.connect (sigc::bind(mem_fun (*this, &LooperPanel::pressed_events), wxString(wxT("redo"))));
	_redo_button->released.connect (sigc::bind(mem_fun (*this, &LooperPanel::released_events), wxString(wxT("redo"))));
	_redo_button->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::button_bind_events), wxString(wxT("redo"))));

	_record_button->pressed.connect (sigc::bind(mem_fun (*this, &LooperPanel::pressed_events), wxString(wxT("record"))));
	_record_button->released.connect (sigc::bind(mem_fun (*this, &LooperPanel::released_events), wxString(wxT("record"))));
	_record_button->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::button_bind_events), wxString(wxT("record"))));

	_overdub_button->pressed.connect (sigc::bind(mem_fun (*this, &LooperPanel::pressed_events), wxString(wxT("overdub"))));
	_overdub_button->released.connect (sigc::bind(mem_fun (*this, &LooperPanel::released_events), wxString(wxT("overdub"))));
	_overdub_button->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::button_bind_events), wxString(wxT("overdub"))));

	_multiply_button->pressed.connect (sigc::bind(mem_fun (*this, &LooperPanel::pressed_events), wxString(wxT("multiply"))));
	_multiply_button->released.connect (sigc::bind(mem_fun (*this, &LooperPanel::released_events), wxString(wxT("multiply"))));
	_multiply_button->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::button_bind_events), wxString(wxT("multiply"))));

	_mute_button->pressed.connect (sigc::bind(mem_fun (*this, &LooperPanel::pressed_events), wxString(wxT("mute"))));
	_mute_button->released.connect (sigc::bind(mem_fun (*this, &LooperPanel::released_events), wxString(wxT("mute"))));
	_mute_button->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::button_bind_events), wxString(wxT("mute"))));

	_pause_button->pressed.connect (sigc::bind(mem_fun (*this, &LooperPanel::pressed_events), wxString(wxT("pause"))));
	_pause_button->released.connect (sigc::bind(mem_fun (*this, &LooperPanel::released_events), wxString(wxT("pause"))));
	_pause_button->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::button_bind_events), wxString(wxT("pause"))));

	_solo_button->pressed.connect (sigc::bind(mem_fun (*this, &LooperPanel::pressed_events), wxString(wxT("solo"))));
	_solo_button->released.connect (sigc::bind(mem_fun (*this, &LooperPanel::released_events), wxString(wxT("solo"))));
	_solo_button->bind_request.connect (sigc::bind(mem_fun (*this, &LooperPanel::button_bind_events), wxString(wxT("solo"))));

	_loop_position->pressed.connect (sigc::bind(mem_fun (*this, &LooperPanel::scratch_events), wxString(wxT("scratch_press"))));
	_loop_position->released.connect (sigc::bind(mem_fun (*this, &LooperPanel::scratch_events), wxString(wxT("scratch_release"))));

	_save_button->clicked.connect (sigc::bind(mem_fun (*this, &LooperPanel::clicked_events), wxString(wxT("save"))));
	_load_button->clicked.connect (sigc::bind(mem_fun (*this, &LooperPanel::clicked_events), wxString(wxT("load"))));

	_loop_control->MidiBindingChanged.connect (mem_fun (*this, &LooperPanel::got_binding_changed));
	_loop_control->MidiLearnCancelled.connect (mem_fun (*this, &LooperPanel::got_learn_canceled));
	
}

void LooperPanel::create_buttons()
{
 	_undo_button = new PixButton(this, ID_UndoButton);
 	_redo_button = new PixButton(this, ID_RedoButton);
 	_record_button = new PixButton(this, ID_RecordButton);
 	_overdub_button = new PixButton(this, ID_OverdubButton);
 	_multiply_button = new PixButton(this, ID_MultiplyButton);
	_load_button = new PixButton(this, ID_LoadButton, false);
 	_save_button = new PixButton(this, ID_SaveButton, false);
	 _mute_button = new PixButton(this, ID_MuteButton);
 	muteButton = CreateButton("mute", "Mute", this);
 	_pause_button = new PixButton(this, ID_PauseButton);
 	_solo_button = new PixButton(this, ID_SoloButton);

	
	// load them all up manually
	_undo_button->set_normal_bitmap (wxBitmap(undo_normal));
	_undo_button->set_selected_bitmap (wxBitmap(undo_selected));
	_undo_button->set_focus_bitmap (wxBitmap(undo_focus));
	_undo_button->set_disabled_bitmap (wxBitmap(undo_disabled));
	_undo_button->set_active_bitmap (wxBitmap(undo_active));
	
	_redo_button->set_normal_bitmap (wxBitmap(redo_normal));
	_redo_button->set_selected_bitmap (wxBitmap(redo_selected));
	_redo_button->set_focus_bitmap (wxBitmap(redo_focus));
	_redo_button->set_disabled_bitmap (wxBitmap(redo_disabled));
	_redo_button->set_active_bitmap (wxBitmap(redo_active));

	_record_button->set_normal_bitmap (wxBitmap(record_normal));
	_record_button->set_selected_bitmap (wxBitmap(record_selected));
	_record_button->set_focus_bitmap (wxBitmap(record_focus));
	_record_button->set_disabled_bitmap (wxBitmap(record_disabled));
	_record_button->set_active_bitmap (wxBitmap(record_active));

	_overdub_button->set_normal_bitmap (wxBitmap(overdub_normal));
	_overdub_button->set_selected_bitmap (wxBitmap(overdub_selected));
	_overdub_button->set_focus_bitmap (wxBitmap(overdub_focus));
	_overdub_button->set_disabled_bitmap (wxBitmap(overdub_disabled));
	_overdub_button->set_active_bitmap (wxBitmap(overdub_active));

	_multiply_button->set_normal_bitmap (wxBitmap(multiply_normal));
	_multiply_button->set_selected_bitmap (wxBitmap(multiply_selected));
	_multiply_button->set_focus_bitmap (wxBitmap(multiply_focus));
	_multiply_button->set_disabled_bitmap (wxBitmap(multiply_disabled));
	_multiply_button->set_active_bitmap (wxBitmap(multiply_active));

	_mute_button->set_normal_bitmap (wxBitmap(mute_normal));
	_mute_button->set_selected_bitmap (wxBitmap(mute_selected));
	_mute_button->set_focus_bitmap (wxBitmap(mute_focus));
	_mute_button->set_disabled_bitmap (wxBitmap(mute_disabled));
	_mute_button->set_active_bitmap (wxBitmap(mute_active));

	_pause_button->set_normal_bitmap (wxBitmap(pause_normal));
	_pause_button->set_selected_bitmap (wxBitmap(pause_selected));
	_pause_button->set_focus_bitmap (wxBitmap(pause_focus));
	_pause_button->set_disabled_bitmap (wxBitmap(pause_disabled));
	_pause_button->set_active_bitmap (wxBitmap(pause_active));

	_solo_button->set_normal_bitmap (wxBitmap(solo_normal));
	_solo_button->set_selected_bitmap (wxBitmap(solo_selected));
	_solo_button->set_focus_bitmap (wxBitmap(solo_focus));
	_solo_button->set_disabled_bitmap (wxBitmap(solo_disabled));
	_solo_button->set_active_bitmap (wxBitmap(solo_active));

	_load_button->set_normal_bitmap (wxBitmap(load_normal));
	_load_button->set_selected_bitmap (wxBitmap(load_selected));
	_load_button->set_focus_bitmap (wxBitmap(load_focus));
	_load_button->set_disabled_bitmap (wxBitmap(load_disabled));
	_load_button->set_active_bitmap (wxBitmap(load_active));

	_save_button->set_normal_bitmap (wxBitmap(save_normal));
	_save_button->set_selected_bitmap (wxBitmap(save_selected));
	_save_button->set_focus_bitmap (wxBitmap(save_focus));
	_save_button->set_disabled_bitmap (wxBitmap(save_disabled));
	_save_button->set_active_bitmap (wxBitmap(save_active));	
}



void
LooperPanel::update_controls()
{
	// get recent controls from loop control
	float val;

	// first see if we have channel count yet
	if (_chan_count == 0 && _loop_control->is_updated(_index, wxT("channel_count"))
	    && _loop_control->is_updated(_index, wxT("has_discrete_io")))
	{
		_loop_control->get_value(_index, wxT("channel_count"), val);
		_chan_count = (int) val;
		// do post_init
		post_init();
	}
	
	if (_loop_control->is_updated(_index, wxT("feedback"))) {
		_loop_control->get_value(_index, wxT("feedback"), val);
		//_feedback_control->set_value ((val * 100.0f));
	}
	if (_loop_control->is_updated(_index, wxT("input_gain"))) {
		_loop_control->get_value(_index, wxT("input_gain"), val);
		_in_gain_control->set_value (val);
	}
	if (_loop_control->is_updated(_index, wxT("rec_thresh"))) {
		_loop_control->get_value(_index, wxT("rec_thresh"), val);
		_thresh_control->set_value (val);
	}

	if (_has_discrete_io) {
		if (_loop_control->is_updated(_index, wxT("in_peak_meter"))) {
			_loop_control->get_value(_index, wxT("in_peak_meter"), val);
			_thresh_control->set_indicator_value (val);
		}

		if (_loop_control->is_updated(_index, wxT("dry"))) {
			_loop_control->get_value(_index, wxT("dry"), val);
			_dry_control->set_value (val);
		}
	}
	else {
		if (_loop_control->is_updated(_index, wxT("in_peak_meter"))) {
			_loop_control->get_value(_index, wxT("in_peak_meter"), val);
			_thresh_control->set_indicator_value (val);
		}
	}

	if (_loop_control->is_updated(_index, wxT("out_peak_meter"))) {
		_loop_control->get_value(_index, wxT("out_peak_meter"), val);
		_wet_control->set_indicator_value (val);
	}
	if (_loop_control->is_updated(_index, wxT("wet"))) {
		_loop_control->get_value(_index, wxT("wet"), val);
		_wet_control->set_value (val);
	}
	if (_loop_control->is_updated(_index, wxT("scratch_pos"))) {
		_loop_control->get_value(_index, wxT("scratch_pos"), val);
		_loop_position->set_value (val);
		_loop_position->set_indicator_value (val);
	}
	if (_loop_control->is_updated(_index, wxT("sync"))) {
		_loop_control->get_value(_index, wxT("sync"), val);
		_sync_check->set_value (val > 0.0f);
	}
	if (_loop_control->is_updated(_index, wxT("playback_sync"))) {
		_loop_control->get_value(_index, wxT("playback_sync"), val);
		_play_sync_check->set_value (val > 0.0f);
	}
	if (_loop_control->is_updated(_index, wxT("use_feedback_play"))) {
		_loop_control->get_value(_index, wxT("use_feedback_play"), val);
		_play_feed_check->set_value (val > 0.0f);
	}
	if (_loop_control->is_updated(_index, wxT("tempo_stretch"))) {
		_loop_control->get_value(_index, wxT("tempo_stretch"), val);
		_tempo_stretch_check->set_value (val > 0.0f);
	}
	if (_loop_control->is_updated(_index, wxT("discrete_prefader"))) {
		_loop_control->get_value(_index, wxT("discrete_prefader"), val);
		_prefader_check->set_value (val > 0.0f);
	}
    
	if (_loop_control->is_updated(_index, wxT("is_soloed"))) {
		_loop_control->get_value(_index, wxT("is_soloed"), val);
		_solo_button->set_active(val > 0.0f);
	}

	if (_loop_control->is_updated(_index, wxT("name"))) {
		wxString prop;
		_loop_control->get_property(_index, wxT("name"), prop);
		if (prop.IsEmpty()) {
			wxString tmpname = wxString::Format(wxT("LOOP %d"), _index+1);
			_name_text->SetValue(tmpname);
		} else {
			_name_text->SetValue(prop);
		}
	}

	for (int i=0; i < _chan_count; ++i) {
		wxString panstr = wxString::Format(wxT("pan_%d"), i+1);
		if (_loop_control->is_updated(_index, panstr)) {
			_loop_control->get_value(_index, panstr, val);
			_panners[i]->set_value (val);
		}
	}

	
	bool state_updated = _loop_control->is_updated(_index, wxT("state"));
	bool pos_updated = _loop_control->is_updated(_index, wxT("loop_pos"));
	bool waiting_updated = _loop_control->is_updated(_index, wxT("waiting"));

	if (_time_panel->update_time()) {
		_time_panel->Refresh(false);
	}

	if (state_updated || waiting_updated) {
		update_state();
	}

	if (pos_updated) {
		float looplen;
		_loop_control->get_value(_index, wxT("loop_len"), looplen);
		_loop_control->get_value(_index, wxT("loop_pos"), val);
		_loop_position->set_indicator_value (val / looplen);
	}
}


void
LooperPanel::update_state()
{
	wxString statestr, nstatestr;
	LooperState state, nextstate;
	float val;
	float soloed = false;

	_loop_control->get_state(_index, state, statestr);
	_loop_control->get_next_state(_index, nextstate, nstatestr);
	_loop_control->get_value(_index, wxT("waiting"), val);
	_loop_control->get_value(_index, wxT("is_soloed"), soloed);
	_waiting = (val > 0.0f) ? true : false;

	if (!_waiting && _flashing_button) {
		// clear flashing
		_flashing_button->set_active(false);
		_flashing_button = 0;
	}
	
	// set not active for all state buttons
	switch(_last_state) {
	case LooperStateRecording:
	case LooperStateWaitStop:
		_record_button->set_active(false);
		break;
	case LooperStateOverdubbing:
		_overdub_button->set_active(false);
		break;
	case LooperStateMultiplying:
		_multiply_button->set_active(false);
		break;
	case LooperStateMuted:
	case LooperStateOffMuted:
		_mute_button->set_active(false);
		SetButtonState(muteButton, ButtonState::Normal);
		break;
	case LooperStatePaused:
		_pause_button->set_active(false);
		break;
	default:
		break;
	}

	
	switch(state) {
	case LooperStateRecording:
	case LooperStateWaitStop:
		_record_button->set_active(true);
		_flashing_button = _record_button;
		break;
	case LooperStateWaitStart:
		_flashing_button = _record_button;
		break;
	case LooperStateOverdubbing:
		_overdub_button->set_active(true);
		_flashing_button = _overdub_button;
		break;
	case LooperStateMultiplying:
		_multiply_button->set_active(true);
		_flashing_button = _multiply_button;
		break;
	case LooperStateMuted:
	case LooperStateOffMuted:
		_mute_button->set_active(true);
		SetButtonState(muteButton, ButtonState::Active);
		_flashing_button = _mute_button;
		break;
	case LooperStatePaused:
		_pause_button->set_active(true);
		_flashing_button = _pause_button;
		break;
	default:
		break;
	}

	
	if (_waiting) {
		if (nextstate != LooperStateUnknown) {
			// reset flashing button to use
			switch(nextstate) {
			case LooperStateRecording:
			case LooperStateWaitStart:
			case LooperStateWaitStop:
				_flashing_button = _record_button;
				break;
			case LooperStateOverdubbing:
				_flashing_button = _overdub_button;
				break;
			case LooperStateMultiplying:
				_flashing_button = _multiply_button;
				break;
			case LooperStateMuted:
			case LooperStateOffMuted:
				if (state == LooperStatePlaying)
					_flashing_button = _mute_button;
				break;
			case LooperStatePlaying:
				if( state == LooperStateMuted) {
					_flashing_button = _mute_button;
				}
				break;
			default:
				break;
			}
			      
		}
		
		// make sure flash time is going
		if (!_flash_timer->IsRunning()) {
			_flash_timer->Start ((int)FlashRate);
		}
	}
	else {

		if (_flash_timer->IsRunning()) {
			_flash_timer->Stop();
			_solo_button->set_active(soloed);
		}

	}
	
	_last_state = state;
}

void
LooperPanel::on_flash_timer (wxTimerEvent &ev)
{
	// toggle the active state of current flash button

	if (_flashing_button) {
		_flashing_button->set_active (!_flashing_button->get_active());
	}
}

void
LooperPanel::on_text_event (wxCommandEvent &ev)
{
	if (ev.GetEventType() == wxEVT_COMMAND_TEXT_ENTER) {
		cerr << "Got text event" << endl;

		// commit change
		_loop_control->post_property_change(_index, wxT("name"), _name_text->GetValue());

		_time_panel->SetFocus();
	}
}


void
LooperPanel::pressed_events (int button, wxString cmd)
{
	_loop_control->post_down_event (_index, cmd);
}

void
LooperPanel::released_events (int button, wxString cmd)
{
	
	if (button == PixButton::MiddleButton) {
		// force up
		_loop_control->post_up_event (_index, cmd, true);
	}
	else {
		_loop_control->post_up_event (_index, cmd);
	}
}


void
LooperPanel::scratch_events (wxString cmd)
{
	if (_last_state == LooperStateRecording) return;
	
	if (cmd == wxT("scratch_press") && (_last_state != LooperStateScratching)) {
		// toggle scratch on
		_scratch_pressed = true;
		_loop_control->post_down_event (_index, wxT("scratch"));
	}
	else if (_scratch_pressed) {
		// toggle scratch off
		_loop_control->post_down_event (_index, wxT("scratch"));
		_scratch_pressed = false;
	}
}

void
LooperPanel::button_bind_events (wxString cmd)
{
	MidiBindInfo info;
	char cmdbuf[100];
	
	info.channel = 0;
	info.type = "n";
	snprintf(cmdbuf, sizeof(cmdbuf), "%s", (const char *) cmd.ToAscii());
	info.control = cmdbuf;

	if (cmd == wxT("delay_trigger")) {
		info.command = "set";
	} else {
		info.command = "note"; // should this be something else?
	}
	info.instance = _index;
	info.lbound = 0.0f;
	info.ubound = 1.0f;

	start_learning (info);
}

void
LooperPanel::rate_bind_events (float val)
{
	MidiBindInfo info;
	
	info.channel = 0;
	info.type = "n";
	info.control = "rate";

	info.command = "set";
	info.instance = _index;
	info.lbound = val;
	info.ubound = val;

	start_learning (info);
}


void
LooperPanel::delay_button_press_event (int button)
{
	_tap_val *= -1.0f;
	post_control_event (wxString(wxT("delay_trigger")), _tap_val);
}

void
LooperPanel::delay_button_release_event (int button)
{
	if (button == PixButton::MiddleButton) {
		_tap_val *= -1.0f;
		post_control_event (wxString(wxT("delay_trigger")), _tap_val);
	}
}

void
LooperPanel::rate_button_event (int button, float rate)
{
	post_control_event (wxString(wxT("rate")), rate);
}

void
LooperPanel::clicked_events (int button, wxString cmd)
{
	if (cmd == wxT("save"))
	{
		wxString filename = _mainpanel->do_file_selector (wxT("Choose file to save loop"),
											      wxT("wav"), wxT("WAVE files (*.wav)|*.wav;*.WAV;*.Wav"),  wxFD_SAVE|wxFD_CHANGE_DIR|wxFD_OVERWRITE_PROMPT);
		
		if ( !filename.empty() )
		{
			// add .wav if there isn't one already
			if (filename.size() <= 4 || (filename.size() > 4 && filename.substr(filename.size() - 4, 4) != wxT(".wav"))) {
				filename += wxT(".wav");
			}
			// todo: specify format
			_loop_control->post_save_loop (_index, filename);
		}
	}
	else if (cmd == wxT("load"))
	{
		wxString filename = _mainpanel->do_file_selector(wxT("Choose file to open"), wxT(""), wxT("Audio files (*.wav,*.aif)|*.wav;*.WAV;*.Wav;*.aif;*.aiff;*.AIF;*.AIFF|All files (*.*)|*.*"), wxFD_OPEN|wxFD_CHANGE_DIR);
		
		if ( !filename.empty() )
		{
			_loop_control->post_load_loop (_index, filename);
		}
	}
}


void
LooperPanel::slider_events(float val, int id)
{
	wxString ctrl;
	
	switch(id)
	{
	case ID_ThreshControl:
		ctrl = wxT("rec_thresh");
		val = val;
		break;
	case ID_InputGainControl:
		ctrl = wxT("input_gain");
		val = _in_gain_control->get_value();
		break;
	case ID_DryControl:
		ctrl = wxT("dry");
		val = _dry_control->get_value();
		break;
	case ID_WetControl:
		ctrl = wxT("wet");
		val = _wet_control->get_value();
		break;
	case ID_ScratchControl:
		ctrl = wxT("scratch_pos");
		val = _loop_position->get_value();
		break;
	case ID_OutputLatency:
		ctrl = wxT("output_latency");
		cerr << "outlat " << val << endl;
		break;
	case ID_InputLatency:
		ctrl = wxT("input_latency");
		cerr << "inlat " << val << endl;
		break;
	case ID_TriggerLatency:
		ctrl = wxT("trigger_latency");
		cerr << "triglat " << val << endl;
		break;
	default:
		break;
	}

	if (!ctrl.empty()) {
		post_control_event (ctrl, val);
	}
}

void
LooperPanel::control_bind_events(int id)
{
	MidiBindInfo info;
	bool donothing = false;

	info.channel = 0;
	info.type = "cc";
	info.command = "set";
	info.instance = _index;
	info.lbound = 0.0f;
	info.ubound = 1.0f;

	
	switch(id)
	{
	case ID_ThreshControl:
		info.control = "rec_thresh";
		info.style = MidiBindInfo::GainStyle;
		break;
	case ID_FeedbackControl:
		info.control = "feedback";
		info.style = MidiBindInfo::NormalStyle;
		break;
	case ID_DryControl:
		info.control = "dry";
		info.style = MidiBindInfo::GainStyle;
		break;
	case ID_InputGainControl:
		info.control = "input_gain";
		info.style = MidiBindInfo::GainStyle;
		break;
	case ID_WetControl:
		info.control = "wet";
		info.command = "set";
		info.style = MidiBindInfo::GainStyle;
		break;
	case ID_ScratchControl:
		info.control = "scratch_pos";
		info.style = MidiBindInfo::NormalStyle;
		break;
	case ID_RateControl:
		info.control = "rate";
		info.style = MidiBindInfo::NormalStyle;
		info.lbound = 0.25;
		info.ubound = 4.0;
		break;
	case ID_StretchControl:
		info.control = "stretch_ratio";
		info.style = MidiBindInfo::NormalStyle;
		info.lbound = 0.5;
		info.ubound = 4.0;
		break;
	case ID_PitchControl:
		info.control = "pitch_shift";
		info.style = MidiBindInfo::IntegerStyle;
		info.lbound = -12;
		info.ubound = 12;
		break;
	case ID_SyncCheck:
		info.control = "sync";
		info.style = MidiBindInfo::NormalStyle;
		break;
	case ID_PlaySyncCheck:
		info.control = "playback_sync";
		info.style = MidiBindInfo::NormalStyle;
		break;
    case ID_PrefaderCheck:
    	info.control = "discrete_prefader";
    	info.style = MidiBindInfo::NormalStyle;
    	break;
	case ID_UseFeedbackPlayCheck:
		info.control = "use_feedback_play";
		info.style = MidiBindInfo::NormalStyle;
		break;
	case ID_TempoStretchCheck:
		info.control = "tempo_stretch";
		info.style = MidiBindInfo::NormalStyle;
		break;
	default:
		donothing = true;
		break;
	}

	if (!donothing) {
		start_learning(info);
	}
}

void LooperPanel::pan_events(float val, int chan)
{
	wxString ctrl = wxString::Format(wxT("pan_%d"), chan+1);

	post_control_event (ctrl, val);
}
       
void LooperPanel::pan_bind_events(int chan)
{
	MidiBindInfo info;
	char cmdbuf[20];
	
	info.channel = 0;
	info.type = "cc";
	info.command = "set";
	info.instance = _index;
	info.lbound = 0.0f;
	info.ubound = 1.0f;
	info.style = MidiBindInfo::NormalStyle;

	snprintf (cmdbuf, sizeof(cmdbuf), "pan_%d", chan+1);
	info.control = cmdbuf;

	start_learning(info);
}


void LooperPanel::start_learning(MidiBindInfo & info)
{
	if (!_learning) {
		_learning = true;

		_leftSelbar->SetBackgroundColour (_learnbgcolor);
		_rightSelbar->SetBackgroundColour (_learnbgcolor);
		_topSelbar->SetBackgroundColour (_learnbgcolor);
		_bottomSelbar->SetBackgroundColour (_learnbgcolor);
		_leftSelbar->Refresh();
		_rightSelbar->Refresh();
		_topSelbar->Refresh();
		_bottomSelbar->Refresh();
		
	}

	_loop_control->learn_midi_binding(info, true);
}


void
LooperPanel::got_binding_changed(SooperLooper::MidiBindInfo & info)
{
	if (_learning) {

		_leftSelbar->SetBackgroundColour (_bgcolor);
		_rightSelbar->SetBackgroundColour (_bgcolor);
		_topSelbar->SetBackgroundColour (_bgcolor);
		_bottomSelbar->SetBackgroundColour (_bgcolor);
		_leftSelbar->Refresh();
		_rightSelbar->Refresh();
		_topSelbar->Refresh();
		_bottomSelbar->Refresh();
		_learning = false;
	}
}

void
LooperPanel::got_learn_canceled()
{
	if (_learning) {
		_leftSelbar->SetBackgroundColour (_bgcolor);
		_rightSelbar->SetBackgroundColour (_bgcolor);
		_topSelbar->SetBackgroundColour (_bgcolor);
		_bottomSelbar->SetBackgroundColour (_bgcolor);
		_leftSelbar->Refresh();
		_rightSelbar->Refresh();
		_topSelbar->Refresh();
		_bottomSelbar->Refresh();
		_learning = false;
	}
}


void
LooperPanel::check_events(bool val, wxString which)
{
	post_control_event (which, val ? 1.0f: 0.0f);
}

void LooperPanel::on_quantize_change (int index, wxString strval)
{
	// 0 is none, 1 is cycle, 2 is eighth, 3 is loop

	post_control_event (wxT("quantize"), (float) index);

}


void
LooperPanel::post_control_event (wxString ctrl, float val)
{
	_loop_control->post_ctrl_change (_index, ctrl, val);
}


wxButton* LooperPanel::CreateButton(const wxString& buttonName, const wxString& buttonLabel, wxWindow* parent, wxWindowID id, bool midiBindable) {
    wxButton* button = new wxButton(parent, id, buttonLabel);
    
    // Connect to existing event system using wxEVT_BUTTON
    button->Bind(wxEVT_BUTTON, [this, buttonName, button](wxCommandEvent& event) {
		clicked_events(button->GetId(), buttonName.ToStdString());
    });

	button->Bind(wxEVT_LEFT_DOWN, [this, buttonName, button](wxMouseEvent& event) {
        pressed_events(button->GetId(), buttonName.ToStdString());
        event.Skip();
    });

    // Use EVT_LEFT_UP for release events
    button->Bind(wxEVT_LEFT_UP, [this, buttonName, button](wxMouseEvent& event) {
        released_events(button->GetId(), buttonName.ToStdString());
        event.Skip();
    });

	if (midiBindable) {
	// Right click for bind menu, should be similar to original PixButton
    // button->Bind(wxEVT_RIGHT_UP, [this, buttonName](wxMouseEvent& event) {
    //     wxMenu menu;
	// use startlearning?? or something like this?
    //     menu.Append(ID_BindMidi, wxT("Learn MIDI binding"));
    //     PopupMenu(&menu);
    //     event.Skip();
    // });
	}

    return button;
}

void LooperPanel::SetButtonState(wxButton* button, ButtonState state) {
    switch (state) {
        case Normal:
            button->Enable(true);
            button->SetBackgroundColour(wxNullColour);
            break;
        case Active:
            button->Enable(true);
            button->SetBackgroundColour(wxColour(255, 0, 0));  // or whatever color indicates selection
            break;
        case Disabled:
            button->Enable(false);
            break;
    }
    button->Refresh();
}
