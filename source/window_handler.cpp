#include "window_handler.h"
using namespace Gtk;
using namespace WaveSimulation;

WindowHandler::WindowHandler(Glib::RefPtr<Builder> builder) :
		dispatcher(), waveEngine(), projectHandler() {

	// Window
	builder->get_widget("mainWindow", mainWindow);
	mainWindow->add_events(Gdk::KEY_PRESS_MASK);
	mainWindow->add_events(Gdk::BUTTON_PRESS_MASK);
	connectEvent(mainWindow, mainWindow->signal_key_press_event(),
			&WindowHandler::on_main_window_key_press);
	connectEvent(mainWindow, mainWindow->signal_button_press_event(),
			&WindowHandler::on_main_window_button_pressed);

	connectEvent(mainWindow, mainWindow->signal_delete_event(),
			&WindowHandler::on_main_window_delete);

	// Drawing Area
	builder->get_widget("drawingarea_main", drawingarea_main);

	drawingarea_main->add_events(Gdk::POINTER_MOTION_MASK);
	drawingarea_main->add_events(Gdk::SCROLL_MASK);
	drawingarea_main->add_events(Gdk::BUTTON_PRESS_MASK);
	drawingarea_main->add_events(Gdk::BUTTON_RELEASE_MASK);
	drawingarea_main->add_events(Gdk::ENTER_NOTIFY_MASK);
	drawingarea_main->add_events(Gdk::LEAVE_NOTIFY_MASK);
	connectEvent(drawingarea_main,
			drawingarea_main->signal_button_press_event(),
			&WindowHandler::on_drawing_area_button_pressed);
	connectEvent(drawingarea_main,
			drawingarea_main->signal_button_release_event(),
			&WindowHandler::on_drawing_area_button_released);
	connectEvent(drawingarea_main, drawingarea_main->signal_scroll_event(),
			&WindowHandler::on_drawing_area_scroll);
	connectEvent(drawingarea_main,
			drawingarea_main->signal_motion_notify_event(),
			&WindowHandler::on_drawing_area_motion_notify);
	connectEvent(drawingarea_main,
			drawingarea_main->signal_enter_notify_event(),
			&WindowHandler::on_drawing_area_enter_notify);
	connectEvent(drawingarea_main,
			drawingarea_main->signal_leave_notify_event(),
			&WindowHandler::on_drawing_area_leave_notify);
	connectEvent(drawingarea_main, drawingarea_main->signal_size_allocate(),
			&WindowHandler::on_drawing_area_size_allocate);

	drawingarea_main->signal_draw().connect(
			sigc::mem_fun(*this, &WindowHandler::on_draw), false);

	// Cursors
	cursorCrosshair = Gdk::Cursor::create(Gdk::CursorType::CROSSHAIR);
	cursorBlank = Gdk::Cursor::create(Gdk::CursorType::BLANK_CURSOR);

	// About Dialog
	builder->get_widget("aboutdialog", aboutdialog);

	// Dialog
	builder->get_widget("openProjectDialog", openProjectDialog);

	builder->get_widget("saveProjectDialog", saveProjectDialog);

	// Tree View
	builder->get_widget("treeview_projects_open", treeview_projects_open);
	connectEvent(treeview_projects_open,
			treeview_projects_open->signal_row_activated(),
			&WindowHandler::on_projects_open_row_activated);

	builder->get_widget("treeview_projects_save", treeview_projects_save);
	connectEvent(treeview_projects_save,
			treeview_projects_save->signal_row_activated(),
			&WindowHandler::on_projects_save_row_activated);

	auto selection = treeview_projects_save->get_selection();
	connectEvent(selection, selection->signal_changed(),
			&WindowHandler::on_projects_save_selection_changed);

	// List Store
	liststore_projects = Glib::RefPtr<ListStore>::cast_static(
			treeview_projects_open->get_model());

	// Entry
	builder->get_widget("entry_project_name", entry_project_name);

	// Text View
	builder->get_widget("textview_description", textview_description);

	// Label
	builder->get_widget("label_info", label_info);

	// Notebook
	builder->get_widget("notebook1", notebook1);

	// Switch
	builder->get_widget("switch_shift_center", switch_shift_center);
	shift_center_changed_con = connectEvent(switch_shift_center,
			switch_shift_center->signal_state_changed(),
			&WindowHandler::on_shift_center_changed);

	builder->get_widget("switch_enable_absorb", switch_enable_absorb);
	enable_absorb_changed_con = connectEvent(switch_enable_absorb,
			switch_enable_absorb->signal_state_changed(),
			&WindowHandler::on_enable_absorb_changed);

	builder->get_widget("switch_osc_enabled", switch_osc_enabled);
	osc_enabled_changed_con = connectEvent(switch_osc_enabled,
			switch_osc_enabled->signal_state_changed(),
			&WindowHandler::on_osc_enabled_changed);

	builder->get_widget("switch_show_oscs", switch_show_oscs);

	builder->get_widget("switch_edit_mass", switch_edit_mass);
	connectEvent(switch_edit_mass, switch_edit_mass->signal_state_changed(),
			&WindowHandler::on_edit_mass_changed);

	builder->get_widget("switch_mass_line_mode", switch_mass_line_mode);

	builder->get_widget("switch_edit_static", switch_edit_static);
	connectEvent(switch_edit_static, switch_edit_static->signal_state_changed(),
			&WindowHandler::on_edit_static_changed);

	builder->get_widget("switch_static_line_mode", switch_static_line_mode);

	builder->get_widget("switch_render", switch_render);
	connectEvent(switch_render, switch_render->signal_state_changed(),
			&WindowHandler::on_render_changed);

	builder->get_widget("switch_extreme_cont", switch_extreme_cont);
	connectEvent(switch_extreme_cont,
			switch_extreme_cont->signal_state_changed(),
			&WindowHandler::on_extreme_cont_changed);

	builder->get_widget("switch_ignore_num_scroll", switch_ignore_num_scroll);

	builder->get_widget("switch_ignore_list_scroll", switch_ignore_list_scroll);

	builder->get_widget("switch_ignore_scale_scroll",
			switch_ignore_scale_scroll);

	// Radio Button
	builder->get_widget("radiobutton_point", radiobutton_point);
	on_source_changed_con[0] = connectEvent(radiobutton_point,
			radiobutton_point->signal_toggled(),
			&WindowHandler::on_source_changed);

	builder->get_widget("radiobutton_line", radiobutton_line);
	on_source_changed_con[1] = connectEvent(radiobutton_line,
			radiobutton_line->signal_toggled(),
			&WindowHandler::on_source_changed);

	builder->get_widget("radiobutton_moving_point", radiobutton_moving_point);
	on_source_changed_con[2] = connectEvent(radiobutton_moving_point,
			radiobutton_moving_point->signal_toggled(),
			&WindowHandler::on_source_changed);

	// Spin Button
	builder->get_widget("spinbutton_size", spinbutton_size);
	connectEvent(spinbutton_size, spinbutton_size->signal_scroll_event(),
			&WindowHandler::on_scroll);
	size_changed_con = connectEvent(spinbutton_size,
			spinbutton_size->signal_value_changed(),
			&WindowHandler::on_size_changed);

	builder->get_widget("spinbutton_loss", spinbutton_loss);
	connectEvent(spinbutton_loss, spinbutton_loss->signal_scroll_event(),
			&WindowHandler::on_scroll);
	loss_changed_con = connectEvent(spinbutton_loss,
			spinbutton_loss->signal_value_changed(),
			&WindowHandler::on_loss_changed);

	builder->get_widget("spinbutton_abs_peak", spinbutton_abs_peak);
	connectEvent(spinbutton_abs_peak,
			spinbutton_abs_peak->signal_scroll_event(),
			&WindowHandler::on_scroll);
	abs_peak_changed_con = connectEvent(spinbutton_abs_peak,
			spinbutton_abs_peak->signal_value_changed(),
			&WindowHandler::on_abs_peak_changed);

	builder->get_widget("spinbutton_abs_thick", spinbutton_abs_thick);
	connectEvent(spinbutton_abs_thick,
			spinbutton_abs_thick->signal_scroll_event(),
			&WindowHandler::on_scroll);
	abs_thick_changed_con = connectEvent(spinbutton_abs_thick,
			spinbutton_abs_thick->signal_value_changed(),
			&WindowHandler::on_abs_thick_changed);

	builder->get_widget("spinbutton_period", spinbutton_period);
	connectEvent(spinbutton_period, spinbutton_period->signal_scroll_event(),
			&WindowHandler::on_scroll);
	on_period_changed_con = connectEvent(spinbutton_period,
			spinbutton_period->signal_value_changed(),
			&WindowHandler::on_period_changed);

	builder->get_widget("spinbutton_phase", spinbutton_phase);
	connectEvent(spinbutton_phase, spinbutton_phase->signal_scroll_event(),
			&WindowHandler::on_scroll);
	on_phase_changed_con = connectEvent(spinbutton_phase,
			spinbutton_phase->signal_value_changed(),
			&WindowHandler::on_phase_changed);

	builder->get_widget("spinbutton_amp", spinbutton_amp);
	connectEvent(spinbutton_amp, spinbutton_amp->signal_scroll_event(),
			&WindowHandler::on_scroll);
	on_amp_changed_con = connectEvent(spinbutton_amp,
			spinbutton_amp->signal_value_changed(),
			&WindowHandler::on_amp_changed);

	builder->get_widget("spinbutton_locx1", spinbutton_locx1);
	connectEvent(spinbutton_locx1, spinbutton_locx1->signal_scroll_event(),
			&WindowHandler::on_scroll);
	on_locx1_changed_con = connectEvent(spinbutton_locx1,
			spinbutton_locx1->signal_value_changed(),
			&WindowHandler::on_locx1_changed);

	builder->get_widget("spinbutton_locy1", spinbutton_locy1);
	connectEvent(spinbutton_locy1, spinbutton_locy1->signal_scroll_event(),
			&WindowHandler::on_scroll);
	on_locy1_changed_con = connectEvent(spinbutton_locy1,
			spinbutton_locy1->signal_value_changed(),
			&WindowHandler::on_locy1_changed);

	builder->get_widget("spinbutton_locx2", spinbutton_locx2);
	connectEvent(spinbutton_locx2, spinbutton_locx2->signal_scroll_event(),
			&WindowHandler::on_scroll);
	on_locx2_changed_con = connectEvent(spinbutton_locx2,
			spinbutton_locx2->signal_value_changed(),
			&WindowHandler::on_locx2_changed);

	builder->get_widget("spinbutton_locy2", spinbutton_locy2);
	connectEvent(spinbutton_locy2, spinbutton_locy2->signal_scroll_event(),
			&WindowHandler::on_scroll);
	on_locy2_changed_con = connectEvent(spinbutton_locy2,
			spinbutton_locy2->signal_value_changed(),
			&WindowHandler::on_locy2_changed);

	builder->get_widget("spinbutton_move_period", spinbutton_move_period);
	connectEvent(spinbutton_move_period,
			spinbutton_move_period->signal_scroll_event(),
			&WindowHandler::on_scroll);
	on_move_period_changed_con = connectEvent(spinbutton_move_period,
			spinbutton_move_period->signal_value_changed(),
			&WindowHandler::on_move_period_changed);

	builder->get_widget("spinbutton_mass_pen", spinbutton_mass_pen);
	connectEvent(spinbutton_mass_pen,
			spinbutton_mass_pen->signal_scroll_event(),
			&WindowHandler::on_scroll);
	connectEvent(spinbutton_mass_pen,
			spinbutton_mass_pen->signal_value_changed(),
			&WindowHandler::on_mass_pen_changed);

	builder->get_widget("spinbutton_primary_mass", spinbutton_primary_mass);
	connectEvent(spinbutton_primary_mass,
			spinbutton_primary_mass->signal_scroll_event(),
			&WindowHandler::on_scroll);

	builder->get_widget("spinbutton_secondary_mass", spinbutton_secondary_mass);
	connectEvent(spinbutton_secondary_mass,
			spinbutton_secondary_mass->signal_scroll_event(),
			&WindowHandler::on_scroll);

	builder->get_widget("spinbutton_mass_min", spinbutton_mass_min);
	connectEvent(spinbutton_mass_min,
			spinbutton_mass_min->signal_scroll_event(),
			&WindowHandler::on_scroll);
	connectEvent(spinbutton_mass_min,
			spinbutton_mass_min->signal_value_changed(),
			&WindowHandler::on_mass_min_changed);

	builder->get_widget("spinbutton_mass_max", spinbutton_mass_max);
	connectEvent(spinbutton_mass_max,
			spinbutton_mass_max->signal_scroll_event(),
			&WindowHandler::on_scroll);
	connectEvent(spinbutton_mass_max,
			spinbutton_mass_max->signal_value_changed(),
			&WindowHandler::on_mass_max_changed);

	builder->get_widget("spinbutton_static_pen", spinbutton_static_pen);
	connectEvent(spinbutton_static_pen,
			spinbutton_static_pen->signal_scroll_event(),
			&WindowHandler::on_scroll);
	connectEvent(spinbutton_static_pen,
			spinbutton_static_pen->signal_value_changed(),
			&WindowHandler::on_static_pen_changed);

	builder->get_widget("spinbutton_max_iterate", spinbutton_max_iterate);
	connectEvent(spinbutton_max_iterate,
			spinbutton_max_iterate->signal_scroll_event(),
			&WindowHandler::on_scroll);
	connectEvent(spinbutton_max_iterate,
			spinbutton_max_iterate->signal_value_changed(),
			&WindowHandler::on_max_iterate_changed);

	builder->get_widget("spinbutton_num_co", spinbutton_num_co);
	connectEvent(spinbutton_num_co, spinbutton_num_co->signal_scroll_event(),
			&WindowHandler::on_scroll);
	connectEvent(spinbutton_num_co, spinbutton_num_co->signal_value_changed(),
			&WindowHandler::on_num_co_changed);

	builder->get_widget("spinbutton_thread_sleep", spinbutton_thread_sleep);
	connectEvent(spinbutton_thread_sleep,
			spinbutton_thread_sleep->signal_scroll_event(),
			&WindowHandler::on_scroll);
	connectEvent(spinbutton_thread_sleep,
			spinbutton_thread_sleep->signal_value_changed(),
			&WindowHandler::on_thread_sleep_changed);

	builder->get_widget("spinbutton_max_fps", spinbutton_max_fps);
	connectEvent(spinbutton_max_fps, spinbutton_max_fps->signal_scroll_event(),
			&WindowHandler::on_scroll);
	connectEvent(spinbutton_max_fps, spinbutton_max_fps->signal_value_changed(),
			&WindowHandler::on_max_fps_changed);

	builder->get_widget("spinbutton_amp_mult_max", spinbutton_amp_mult_max);
	connectEvent(spinbutton_amp_mult_max,
			spinbutton_amp_mult_max->signal_scroll_event(),
			&WindowHandler::on_scroll);
	connectEvent(spinbutton_amp_mult_max,
			spinbutton_amp_mult_max->signal_value_changed(),
			&WindowHandler::on_amp_mult_max_changed);

	builder->get_widget("spinbutton_dec_place", spinbutton_dec_place);
	connectEvent(spinbutton_dec_place,
			spinbutton_dec_place->signal_scroll_event(),
			&WindowHandler::on_scroll);
	connectEvent(spinbutton_dec_place,
			spinbutton_dec_place->signal_value_changed(),
			&WindowHandler::on_dec_place_changed);

	// Button
	builder->get_widget("button_fill_mass", button_fill_mass);
	connectEvent(button_fill_mass, button_fill_mass->signal_clicked(),
			&WindowHandler::on_fill_mass_clicked);

	builder->get_widget("button_clear_mass", button_clear_mass);
	connectEvent(button_clear_mass, button_clear_mass->signal_clicked(),
			&WindowHandler::on_clear_mass_clicked);

	builder->get_widget("button_swap_mass", button_swap_mass);
	connectEvent(button_swap_mass, button_swap_mass->signal_clicked(),
			&WindowHandler::on_swap_mass_clicked);

	builder->get_widget("button_fill_static", button_fill_static);
	connectEvent(button_fill_static, button_fill_static->signal_clicked(),
			&WindowHandler::on_fill_static_clicked);

	builder->get_widget("button_clear_static", button_clear_static);
	connectEvent(button_clear_static, button_clear_static->signal_clicked(),
			&WindowHandler::on_clear_static_clicked);

	builder->get_widget("button_set_to_cpu_cores", button_set_to_cpu_cores);
	connectEvent(button_set_to_cpu_cores,
			button_set_to_cpu_cores->signal_clicked(),
			&WindowHandler::on_set_to_cpu_cores_clicked);

	builder->get_widget("button_delete", button_delete);
	connectEvent(button_delete, button_delete->signal_clicked(),
			&WindowHandler::on_delete_clicked);

	// Color Button
	builder->get_widget("colorbutton_crest", colorbutton_crest);
	connectEvent(colorbutton_crest, colorbutton_crest->signal_color_set(),
			&WindowHandler::on_crest_changed);

	builder->get_widget("colorbutton_trough", colorbutton_trough);
	connectEvent(colorbutton_trough, colorbutton_trough->signal_color_set(),
			&WindowHandler::on_trough_changed);

	builder->get_widget("colorbutton_static", colorbutton_static);
	connectEvent(colorbutton_static, colorbutton_static->signal_color_set(),
			&WindowHandler::on_static_color_changed);

	// List Box
	builder->get_widget("comboboxtext_static_op", comboboxtext_static_op);
	connectEvent(comboboxtext_static_op,
			comboboxtext_static_op->signal_scroll_event(),
			&WindowHandler::on_scroll);

	builder->get_widget("comboboxtext_sel_osc", comboboxtext_sel_osc);
	connectEvent(comboboxtext_sel_osc, comboboxtext_sel_osc->signal_changed(),
			&WindowHandler::on_sel_osc_changed);
	connectEvent(comboboxtext_sel_osc,
			comboboxtext_sel_osc->signal_scroll_event(),
			&WindowHandler::on_scroll);

	// Scale
	builder->get_widget("scale_font_size", scale_font_size);
	connectEvent(scale_font_size, scale_font_size->signal_scroll_event(),
			&WindowHandler::on_scroll);

	builder->get_widget("scale_amp_mult", scale_amp_mult);
	connectEvent(scale_amp_mult, scale_amp_mult->signal_value_changed(),
			&WindowHandler::on_amp_mult_changed);
	connectEvent(scale_amp_mult, scale_amp_mult->signal_scroll_event(),
			&WindowHandler::on_scroll);

	// ImageMenuItem
	builder->get_widget("imagemenuitem_quit", imagemenuitem_quit);
	connectEvent(imagemenuitem_quit, imagemenuitem_quit->signal_activate(),
			&WindowHandler::on_activate);

	builder->get_widget("imagemenuitem_manual", imagemenuitem_manual);
	connectEvent(imagemenuitem_manual, imagemenuitem_manual->signal_activate(),
			&WindowHandler::on_activate);

	builder->get_widget("imagemenuitem_about", imagemenuitem_about);
	connectEvent(imagemenuitem_about, imagemenuitem_about->signal_activate(),
			&WindowHandler::on_activate);

	// ToolButton
	builder->get_widget("toolbutton_open", toolbutton_open);
	connectEvent(toolbutton_open, toolbutton_open->signal_clicked(),
			&WindowHandler::on_open_clicked);

	builder->get_widget("toolbutton_save", toolbutton_save);
	connectEvent(toolbutton_save, toolbutton_save->signal_clicked(),
			&WindowHandler::on_save_clicked);

	builder->get_widget("toolbutton_new", toolbutton_new);
	connectEvent(toolbutton_new, toolbutton_new->signal_clicked(),
			&WindowHandler::on_new_clicked);

	builder->get_widget("toolbutton_refresh", toolbutton_refresh);
	connectEvent(toolbutton_refresh, toolbutton_refresh->signal_clicked(),
			&WindowHandler::on_refresh_clicked);

	builder->get_widget("toolbutton_pause", toolbutton_pause);
	connectEvent(toolbutton_pause, toolbutton_pause->signal_clicked(),
			&WindowHandler::on_pause_clicked);

	builder->get_widget("toolbutton_play", toolbutton_play);
	connectEvent(toolbutton_play, toolbutton_play->signal_clicked(),
			&WindowHandler::on_play_clicked);

	// Check existence of the scene directory and create it if necessary
	lastProjectName = "default";
	if (projectHandler.setProjectsPath(projectHandler.getProjectsPath(), true)
			== OK) {
		// Attempt to load the default scene. Create one if it doesn't exist.
		string description;
		if (projectHandler.openProject("default", description, &waveEngine)
				!= OK) {
			std::cout << "Couldn't find the default scene. Creating one."
					<< std::endl;
			if (projectHandler.saveProject("default",
					textview_description->get_buffer()->get_text(false),
					&waveEngine) != OK) {
				std::cout << "Creation of a default scene failed." << std::endl;
			}
		}
		textview_description->get_buffer()->set_text(description);
	} else {
		std::cout << "Scene directory could neither be found nor created. "
				"Default scene won't be opened." << std::endl;
	}

	try {
		unsigned int sz = waveEngine.getSize();
		m_image = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, sz, sz);
	} catch (const Gdk::PixbufError& ex) {
		std::cerr << "PixbufError: " << ex.what() << std::endl;
	}

	if (m_image) {
		dispatcher.connect(
				sigc::mem_fun(*this,
						&WindowHandler::on_notification_from_render_thread));
	}

	// Retrieve settings
	INIReader inireader("./data/settings.ini");
	if (inireader.ParseError() < 0) {
		std::cout
				<< "Can't load the 'settings.ini'. Default settings will be applied."
				<< std::endl;
	}
	//else{

	spinbutton_size->set_value(waveEngine.getSize());
	spinbutton_loss->set_value(waveEngine.getLossRatio());
	spinbutton_abs_peak->set_value(waveEngine.getAbsorberLossRatio());
	spinbutton_abs_thick->set_value(waveEngine.getAbsorberThickness());
	switch_shift_center->set_active(waveEngine.getShiftParticlesEnabled());
	switch_enable_absorb->set_active(waveEngine.getAbsorberEnabled());

	switch_show_oscs->set_active(
			inireader.GetBoolean("pool_oscillators", "show_oscillators",
					switch_show_oscs->get_active()));

	scale_font_size->set_value(
			inireader.GetReal("pool_oscillators", "font_size",
					scale_font_size->get_value()));

	spinbutton_mass_pen->set_value(
			inireader.GetInteger("pool_mass", "pen_thickness",
					spinbutton_mass_pen->get_value()));

	switch_mass_line_mode->set_active(
			inireader.GetBoolean("pool_mass", "line_mode",
					switch_mass_line_mode->get_active()));

	spinbutton_primary_mass->set_value(
			inireader.GetReal("pool_mass", "primary_mass",
					spinbutton_primary_mass->get_value()));

	spinbutton_secondary_mass->set_value(
			inireader.GetReal("pool_mass", "secondary_mass",
					spinbutton_secondary_mass->get_value()));

	spinbutton_mass_min->set_value(
			inireader.GetReal("pool_mass", "mass_range_minimum",
					waveEngine.getMassMapRangeLow()));

	spinbutton_mass_max->set_value(
			inireader.GetReal("pool_mass", "mass_range_maximum",
					waveEngine.getMassMapRangeHigh()));

	spinbutton_static_pen->set_value(
			inireader.GetInteger("pool_static", "pen_thickness",
					spinbutton_static_pen->get_value()));

	switch_static_line_mode->set_active(
			inireader.GetBoolean("pool_static", "line_mode",
					switch_static_line_mode->get_active()));

	spinbutton_max_iterate->set_value(
			inireader.GetReal("threading", "iterations_per_second",
					waveEngine.getIterationsPerSecond()));

	spinbutton_num_co->set_value(
			inireader.GetInteger("threading", "number_of_co_threads",
					waveEngine.getNumberOfThreads()));

	spinbutton_thread_sleep->set_value(
			inireader.GetInteger("threading", "thread_sleep_duration",
					waveEngine.getThreadDelay()));

	switch_render->set_active(
			inireader.GetBoolean("rendering", "render",
					waveEngine.getRenderEnabled()));

	spinbutton_max_fps->set_value(
			inireader.GetReal("rendering", "frames_per_second",
					waveEngine.getFramesPerSecond()));

	scale_amp_mult->set_value(
			inireader.GetInteger("rendering", "amplitude_multiplier",
					waveEngine.getAmplitudeMultiplier()));

	spinbutton_amp_mult_max->set_value(
			inireader.GetInteger("rendering", "amplitude_multiplier_max",
					spinbutton_amp_mult_max->get_value()));

	switch_extreme_cont->set_active(
			inireader.GetBoolean("rendering", "extreme_contrast",
					waveEngine.getExtremeContrastEnabled()));

	Gdk::Color button_color;
	uint32_t color;
	color = inireader.GetInteger("rendering", "crest_color",
			waveEngine.getCrestColor().ToRGB32());
	button_color.set_rgb_p((double) ((uint8_t) color) / 255.0,
			(double) ((uint8_t) (color >> 8)) / 255.0,
			(double) ((uint8_t) (color >> 16)) / 255.0);
	colorbutton_crest->set_color(button_color);
	on_crest_changed(colorbutton_crest); // Setting the color won't trigger the event on its own.

	color = inireader.GetInteger("rendering", "trough_color",
			waveEngine.getTroughColor().ToRGB32());
	button_color.set_rgb_p((double) ((uint8_t) color) / 255.0,
			(double) ((uint8_t) (color >> 8)) / 255.0,
			(double) ((uint8_t) (color >> 16)) / 255.0);
	colorbutton_trough->set_color(button_color);
	on_trough_changed(colorbutton_trough); // Setting the color won't trigger the event on its own.

	color = inireader.GetInteger("rendering", "static_color",
			waveEngine.getStaticColor().ToRGB32());
	button_color.set_rgb_p((double) ((uint8_t) color) / 255.0,
			(double) ((uint8_t) (color >> 8)) / 255.0,
			(double) ((uint8_t) (color >> 16)) / 255.0);
	colorbutton_static->set_color(button_color);
	on_static_color_changed(colorbutton_static); // Setting the color won't trigger the event on its own.

	switch_ignore_num_scroll->set_active(
			inireader.GetBoolean("ui", "ignore_numeric_scroll",
					switch_ignore_num_scroll->get_active()));

	switch_ignore_list_scroll->set_active(
			inireader.GetBoolean("ui", "ignore_list_scroll",
					switch_ignore_list_scroll->get_active()));

	switch_ignore_scale_scroll->set_active(
			inireader.GetBoolean("ui", "ignore_scale_scroll",
					switch_ignore_scale_scroll->get_active()));

	spinbutton_dec_place->set_value(
			inireader.GetInteger("ui", "decimal_places",
					spinbutton_dec_place->get_value()));
	changeDigitCount(spinbutton_dec_place->get_value());

	//}

	// Wave Engine
	waveEngine.setRenderCallback(renderCallback, this);
	waveEngine.start();
}

bool WindowHandler::on_main_window_delete(GdkEventAny*arg, Window*window) {

	window->unset_focus();
	FILE * pFile = fopen("./data/settings.ini", "wb");
	if (!pFile) {
		std::cout << "Can't create the 'settings.ini'. Settings won't be saved."
				<< std::endl;
	} else {
		fprintf(pFile,
				";General settings to be loaded when a new scene is created.\n"
						";Settings those exist under 'Pool->General' and 'Oscillators' "
						"except 'Show Oscillators' can only be saved by saving the scene\n");

		fprintf(pFile, "[pool_oscillators]\n");
		fprintf(pFile, "show_oscillators=%d\n", switch_show_oscs->get_active());
		fprintf(pFile, "font_size=%f\n", scale_font_size->get_value());

		fprintf(pFile, "[pool_mass]\n");
		fprintf(pFile, "pen_thickness=%d\n",
				spinbutton_mass_pen->get_value_as_int());
		fprintf(pFile, "line_mode=%d\n", switch_mass_line_mode->get_active());
		fprintf(pFile, "primary_mass=%f\n",
				spinbutton_primary_mass->get_value());
		fprintf(pFile, "secondary_mass=%f\n",
				spinbutton_secondary_mass->get_value());
		fprintf(pFile, "mass_range_minimum=%f\n",
				spinbutton_mass_min->get_value());
		fprintf(pFile, "mass_range_maximum=%f\n",
				spinbutton_mass_max->get_value());

		fprintf(pFile, "[pool_static]\n");
		fprintf(pFile, "pen_thickness=%d\n",
				spinbutton_static_pen->get_value_as_int());
		fprintf(pFile, "line_mode=%d\n", switch_static_line_mode->get_active());

		fprintf(pFile, "[threading]\n");
		fprintf(pFile, "iterations_per_second=%f\n",
				spinbutton_max_iterate->get_value());
		fprintf(pFile, "number_of_co_threads=%d\n",
				spinbutton_num_co->get_value_as_int());
		fprintf(pFile, "thread_sleep_duration=%d\n",
				spinbutton_thread_sleep->get_value_as_int());

		fprintf(pFile, "[rendering]\n");
		fprintf(pFile, "render=%d\n", switch_render->get_active());
		fprintf(pFile, "frames_per_second=%f\n",
				spinbutton_max_fps->get_value());
		fprintf(pFile, "amplitude_multiplier=%d\n",
				(int) scale_amp_mult->get_value());
		fprintf(pFile, "amplitude_multiplier_max=%d\n",
				spinbutton_amp_mult_max->get_value_as_int());
		fprintf(pFile, "extreme_contrast=%d\n",
				switch_extreme_cont->get_active());
		Gdk::Color bcolor = colorbutton_crest->get_color();
		uint32_t color = (uint8_t) (bcolor.get_red_p() * 255.0)
				+ ((uint8_t) (bcolor.get_green_p() * 255.0) << 8)
				+ ((uint8_t) (bcolor.get_blue_p() * 255.0) << 16);
		fprintf(pFile, "crest_color=%d\n", color);
		bcolor = colorbutton_trough->get_color();
		color = (uint8_t) (bcolor.get_red_p() * 255.0)
				+ ((uint8_t) (bcolor.get_green_p() * 255.0) << 8)
				+ ((uint8_t) (bcolor.get_blue_p() * 255.0) << 16);
		fprintf(pFile, "trough_color=%d\n", color);
		bcolor = colorbutton_static->get_color();
		color = (uint8_t) (bcolor.get_red_p() * 255.0)
				+ ((uint8_t) (bcolor.get_green_p() * 255.0) << 8)
				+ ((uint8_t) (bcolor.get_blue_p() * 255.0) << 16);
		fprintf(pFile, "static_color=%d\n", color);

		fprintf(pFile, "[ui]\n");
		fprintf(pFile, "ignore_numeric_scroll=%d\n",
				switch_ignore_num_scroll->get_active());
		fprintf(pFile, "ignore_list_scroll=%d\n",
				switch_ignore_list_scroll->get_active());
		fprintf(pFile, "ignore_scale_scroll=%d\n",
				switch_ignore_scale_scroll->get_active());
		fprintf(pFile, "decimal_places=%d\n",
				spinbutton_dec_place->get_value_as_int());

		fclose(pFile);
	}

	return false;
}

bool WindowHandler::on_drawing_area_button_pressed(GdkEventButton*arg,
		DrawingArea*drawingArea) {
	mainWindow->unset_focus();
	mouse_loc = Point(arg->x, arg->y);
	mouse_loc_old = mouse_press_loc = mouse_loc;
	if (arg->type != GDK_TRIPLE_BUTTON_PRESS) {
		bool edit_mass = switch_edit_mass->get_active();
		bool edit_static = switch_edit_static->get_active();
		if (arg->type != GDK_DOUBLE_BUTTON_PRESS) {
			bool mass_line_mode = switch_mass_line_mode->get_active();
			bool static_line_mode = switch_static_line_mode->get_active();
			switch (arg->button) {
			case 1:
				pressing_left_mb = true;
				if (edit_mass || edit_static) {
					if ((edit_mass && !mass_line_mode)
							|| (edit_static && !static_line_mode))
						draw_helper(drawingArea->get_width(),
								drawingArea->get_height(), arg->x, arg->y);
				} else {
					if (!switch_osc_enabled->get_active())
						switch_osc_enabled->set_active(true);
					oscillator_helper(drawingArea->get_width(),
							drawingArea->get_height(), arg->x, arg->y);
				}
				break;
			case 2:
				pressing_middle_mb = true;
				break;
			case 3:
				pressing_right_mb = true;
				if (edit_mass || edit_static) {
					if ((edit_mass && !mass_line_mode)
							|| (edit_static && !static_line_mode))
						draw_helper(drawingArea->get_width(),
								drawingArea->get_height(), arg->x, arg->y);
				}

				else {

					oscillator_helper(drawingArea->get_width(),
							drawingArea->get_height(), arg->x, arg->y);
				}
				break;
			}
		} else if (!edit_mass && !edit_static) {
			switch (arg->button) {
			case 1:
				if (switch_osc_enabled->get_active())
					switch_osc_enabled->set_active(false);
				break;
			}
		}

	}

	return false;
}
bool WindowHandler::on_drawing_area_button_released(GdkEventButton*arg,
		DrawingArea*drawingArea) {

	bool mass_line_mode = switch_mass_line_mode->get_active();
	bool static_line_mode = switch_static_line_mode->get_active();
	bool edit_mass = switch_edit_mass->get_active();
	bool edit_static = switch_edit_static->get_active();
	if ((edit_mass && mass_line_mode) || (edit_static && static_line_mode)) {
		draw_helper(drawingArea->get_width(), drawingArea->get_height(), arg->x,
				arg->y);
	}

	switch (arg->button) {
	case 1:
		pressing_left_mb = false;
		break;
	case 2:
		pressing_middle_mb = false;
		break;
	case 3:
		pressing_right_mb = false;
		break;
	}
	return false;
}
bool WindowHandler::on_main_window_button_pressed(GdkEventButton*arg,
		Window*window) {
	if (arg->button == 1 && arg->type != GDK_DOUBLE_BUTTON_PRESS
			&& arg->type != GDK_DOUBLE_BUTTON_PRESS)
		window->unset_focus();
	return false;
}

bool WindowHandler::on_drawing_area_motion_notify(GdkEventMotion*arg,
		DrawingArea * drawingArea) {
	mouse_loc_old = mouse_loc;
	mouse_loc = Point(arg->x, arg->y);
	bool mass_line_mode = switch_mass_line_mode->get_active();
	bool static_line_mode = switch_static_line_mode->get_active();
	bool edit_mass = switch_edit_mass->get_active();
	bool edit_static = switch_edit_static->get_active();
	updateInfoLabel();
	if (pressing_left_mb || pressing_right_mb) {
		if (edit_mass || edit_static) {
			if ((edit_mass && !mass_line_mode)
					|| (edit_static && !static_line_mode))
				draw_helper(drawingArea->get_width(), drawingArea->get_height(),
						arg->x, arg->y);
		} else {

			oscillator_helper(drawingArea->get_width(),
					drawingArea->get_height(), arg->x, arg->y);
		}
	}
	//std::cout << "Pos: " << arg->x << ", " << arg->y << std::endl;
	return false;
}

bool WindowHandler::on_drawing_area_enter_notify(GdkEventCrossing*arg,
		DrawingArea*drawingArea) {
	updateCursor(false);
	draw_preview = true;
	pointer_in_drawing_area = true;
	return false;
}

bool WindowHandler::on_drawing_area_leave_notify(GdkEventCrossing*arg,
		DrawingArea*drawingArea) {
	draw_preview = false;
	pointer_in_drawing_area = false;
	updateInfoLabel();
	return false;
}

void WindowHandler::on_drawing_area_size_allocate(Allocation& allocation,
		DrawingArea*drawingArea) {
	double width = allocation.get_width();
	double height = allocation.get_height();
	double minwh = MIN(width, height);

	pool = Rectangle(width > minwh ? (width - height) / 2 : 0,
			height > minwh ? (height - width) / 2 : 0, minwh, minwh);
}

bool WindowHandler::on_main_window_key_press(GdkEventKey*arg, Window * window) {

	if (!(arg->state & GDK_CONTROL_MASK)) {
		Widget * focused = window->get_focus();
		if (focused == nullptr
				|| (typeid(*focused) != typeid(SpinButton)
						&& typeid(*focused) != typeid(TextView))) {
			switch (arg->keyval) {
			case GDK_KEY_1:
				comboboxtext_sel_osc->set_active(0);
				break;
			case GDK_KEY_2:
				comboboxtext_sel_osc->set_active(1);
				break;
			case GDK_KEY_3:
				comboboxtext_sel_osc->set_active(2);
				break;
			case GDK_KEY_4:
				comboboxtext_sel_osc->set_active(3);
				break;
			case GDK_KEY_5:
				comboboxtext_sel_osc->set_active(4);
				break;
			case GDK_KEY_6:
				comboboxtext_sel_osc->set_active(5);
				break;
			case GDK_KEY_7:
				comboboxtext_sel_osc->set_active(6);
				break;
			case GDK_KEY_8:
				comboboxtext_sel_osc->set_active(7);
				break;
			case GDK_KEY_9:
				comboboxtext_sel_osc->set_active(8);
				break;
			case GDK_KEY_space:
				refresh_scene();
				break;
			case GDK_KEY_s:
			case GDK_KEY_S:
				switch_edit_static->set_active(
						!switch_edit_static->get_active());
				break;
			case GDK_KEY_m:
			case GDK_KEY_M:
				switch_edit_mass->set_active(!switch_edit_mass->get_active());
				break;
			case GDK_KEY_o:
			case GDK_KEY_O:
				switch_edit_mass->set_active(false);
				switch_edit_static->set_active(false);
				break;
			case GDK_KEY_L:
			case GDK_KEY_l:
				if (switch_edit_mass->get_active())
					switch_mass_line_mode->set_active(
							!switch_mass_line_mode->get_active());
				else if (switch_edit_static->get_active())
					switch_static_line_mode->set_active(
							!switch_static_line_mode->get_active());
				break;
			case GDK_KEY_p:
			case GDK_KEY_P:
				if (toolbutton_play->is_visible())
					on_play_clicked(toolbutton_play);
				else if (toolbutton_pause->is_visible())
					on_pause_clicked(toolbutton_pause);
				break;
			}
		}
	} else {
		switch (arg->keyval) {
		case GDK_KEY_s:
		case GDK_KEY_S:
			on_save_clicked(toolbutton_save);
			break;
		case GDK_KEY_o:
		case GDK_KEY_O:
			on_open_clicked(toolbutton_open);
			break;
		case GDK_KEY_n:
		case GDK_KEY_N:
			on_new_clicked(toolbutton_new);
			break;
		}
	}

	return false;
}
bool WindowHandler::on_drawing_area_scroll(GdkEventScroll*arg,
		DrawingArea*drawingArea) {
	bool edit_mass = switch_edit_mass->get_active();
	bool edit_static = switch_edit_static->get_active();
	if (edit_static) {
		double incdec =
				spinbutton_static_pen->get_adjustment()->get_step_increment();
		if (arg->direction == GDK_SCROLL_DOWN)
			incdec *= -1;
		spinbutton_static_pen->set_value(
				spinbutton_static_pen->get_value() + incdec);
	} else if (edit_mass) {
		double incdec =
				spinbutton_mass_pen->get_adjustment()->get_step_increment();
		if (arg->direction == GDK_SCROLL_DOWN)
			incdec *= -1;
		spinbutton_mass_pen->set_value(
				spinbutton_mass_pen->get_value() + incdec);
	} else {
		int rowid = comboboxtext_sel_osc->get_active_row_number();
		double incdec =
				spinbutton_period->get_adjustment()->get_step_increment();
		if (arg->direction == GDK_SCROLL_DOWN)
			incdec *= -1;
		waveEngine.setOscillatorPeriod(rowid,
				waveEngine.getOscillatorPeriod(rowid) + incdec);
		updateSelectedOsc();
	}

	return false;
}

bool WindowHandler::on_scroll(GdkEventScroll*arg, Widget*widget) {
	SpinButton * spinButton = dynamic_cast<SpinButton*>(widget);
	ComboBoxText * comboBoxText = dynamic_cast<ComboBoxText*>(widget);
	Scale * scale = dynamic_cast<Scale*>(widget);
	if ((spinButton != nullptr && switch_ignore_num_scroll->get_active())
			|| (comboBoxText != nullptr
					&& switch_ignore_list_scroll->get_active())
			|| (scale != nullptr && switch_ignore_scale_scroll->get_active()))
		widget->signal_scroll_event().emission_stop();
	return false;
}

void WindowHandler::on_size_changed(SpinButton*spinButton) {
	unsigned int size = spinButton->get_value_as_int();
	waveEngine.setSize(size);
	m_image = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, size, size);
}
void WindowHandler::on_loss_changed(SpinButton*spinButton) {
	waveEngine.setLossRatio(spinButton->get_value());
}
void WindowHandler::on_abs_peak_changed(SpinButton*spinButton) {
	waveEngine.setAbsorberLossRatio(spinButton->get_value());
}
void WindowHandler::on_abs_thick_changed(SpinButton*spinButton) {
	waveEngine.setAbsorberThickness(spinButton->get_value_as_int());
}
void WindowHandler::on_shift_center_changed(StateType state, Switch*_switch) {
	waveEngine.setShiftParticlesEnabled(_switch->get_active());
}
void WindowHandler::on_enable_absorb_changed(StateType state, Switch*_switch) {
	waveEngine.setAbsorberEnabled(_switch->get_active());
}
void WindowHandler::on_sel_osc_changed(ComboBoxText*comboBoxText) {
	updateSelectedOsc();
}
void WindowHandler::on_osc_enabled_changed(StateType state, Switch*_switch) {
	waveEngine.setOscillatorEnabled(
			comboboxtext_sel_osc->get_active_row_number(),
			_switch->get_active());
}

void WindowHandler::on_period_changed(SpinButton*spinButton) {
	waveEngine.setOscillatorPeriod(
			comboboxtext_sel_osc->get_active_row_number(),
			spinButton->get_value());
}
void WindowHandler::on_phase_changed(SpinButton*spinButton) {
	waveEngine.setOscillatorPhase(comboboxtext_sel_osc->get_active_row_number(),
			spinButton->get_value());
}
void WindowHandler::on_amp_changed(SpinButton*spinButton) {
	waveEngine.setOscillatorAmplitude(
			comboboxtext_sel_osc->get_active_row_number(),
			spinButton->get_value());
}
void WindowHandler::on_locx1_changed(SpinButton*spinButton) {
	waveEngine.setOscillatorLocation(
			comboboxtext_sel_osc->get_active_row_number(), 0,
			Point(spinButton->get_value(), spinbutton_locy1->get_value()));
}
void WindowHandler::on_locy1_changed(SpinButton*spinButton) {
	waveEngine.setOscillatorLocation(
			comboboxtext_sel_osc->get_active_row_number(), 0,
			Point(spinbutton_locx1->get_value(), spinButton->get_value()));
}
void WindowHandler::on_locx2_changed(SpinButton*spinButton) {
	waveEngine.setOscillatorLocation(
			comboboxtext_sel_osc->get_active_row_number(), 1,
			Point(spinButton->get_value(), spinbutton_locy2->get_value()));
}
void WindowHandler::on_locy2_changed(SpinButton*spinButton) {
	waveEngine.setOscillatorLocation(
			comboboxtext_sel_osc->get_active_row_number(), 1,
			Point(spinbutton_locx2->get_value(), spinButton->get_value()));
}
void WindowHandler::on_move_period_changed(SpinButton*spinButton) {
	waveEngine.setOscillatorMovePeriod(
			comboboxtext_sel_osc->get_active_row_number(),
			spinButton->get_value());
}

void WindowHandler::on_edit_mass_changed(StateType state, Switch*_switch) {
	bool active = _switch->get_active();
	waveEngine.setShowMassMap(active);
	if (active && switch_edit_static->get_active())
		switch_edit_static->set_active(false);
	updateCursor(false);
}
void WindowHandler::on_mass_min_changed(SpinButton*spinButton) {
	waveEngine.setMassMapRangeLow(spinButton->get_value());
}
void WindowHandler::on_mass_max_changed(SpinButton*spinButton) {
	waveEngine.setMassMapRangeHigh(spinButton->get_value());
}
void WindowHandler::on_edit_static_changed(StateType state, Switch*_switch) {
	if (_switch->get_active() && switch_edit_mass->get_active())
		switch_edit_mass->set_active(false);
	updateCursor(false);
}
void WindowHandler::on_max_iterate_changed(SpinButton*spinButton) {
	waveEngine.setIterationsPerSecond(spinButton->get_value());
}
void WindowHandler::on_num_co_changed(SpinButton*spinButton) {
	waveEngine.setNumberOfThreads(spinButton->get_value_as_int());
}
void WindowHandler::on_thread_sleep_changed(SpinButton*spinButton) {
	waveEngine.setThreadDelay(spinButton->get_value_as_int());
}
void WindowHandler::on_render_changed(StateType state, Switch*_switch) {
	waveEngine.setRenderEnabled(_switch->get_active());
}
void WindowHandler::on_max_fps_changed(SpinButton*spinButton) {
	waveEngine.setFramesPerSecond(spinButton->get_value());
}
void WindowHandler::on_amp_mult_changed(Scale*scale) {
	waveEngine.setAmplitudeMultiplier((int) scale->get_value());
}
void WindowHandler::on_amp_mult_max_changed(SpinButton*spinButton) {
	double upper = spinButton->get_value();
	scale_amp_mult->get_adjustment()->set_upper(upper);
	scale_amp_mult->set_fill_level(upper);
}
void WindowHandler::on_mass_pen_changed(SpinButton*spinButton) {

}
void WindowHandler::on_static_pen_changed(SpinButton*spinButton) {

}
void WindowHandler::on_extreme_cont_changed(StateType state, Switch*_switch) {
	waveEngine.setExtremeContrastEnabled(_switch->get_active());
}
void WindowHandler::on_fill_mass_clicked(Button*button) {
	if (waveEngine.lock()) {
		double * pdm = (double *) waveEngine.getData(ParticleAttribute::Mass);
		if (pdm != nullptr) {
			unsigned int sizesize = waveEngine.getSize();
			sizesize *= sizesize;
			double primarymass = spinbutton_primary_mass->get_value();
			for (unsigned int i = 0; i < sizesize; i++) {
				pdm[i] = primarymass;
			}
		}
		waveEngine.unlock();
	}
}
void WindowHandler::on_clear_mass_clicked(Button*button) {
	if (waveEngine.lock()) {
		double * pdm = (double *) waveEngine.getData(ParticleAttribute::Mass);
		if (pdm != nullptr) {
			unsigned int sizesize = waveEngine.getSize();
			sizesize *= sizesize;
			double secondarymass = spinbutton_secondary_mass->get_value();
			for (unsigned int i = 0; i < sizesize; i++) {
				pdm[i] = secondarymass;
			}
		}
		waveEngine.unlock();
	}
}
void WindowHandler::on_swap_mass_clicked(Button*button) {
	double pmass = spinbutton_primary_mass->get_value();
	spinbutton_primary_mass->set_value(spinbutton_secondary_mass->get_value());
	spinbutton_secondary_mass->set_value(pmass);
}

void WindowHandler::on_fill_static_clicked(Button*button) {
	if (waveEngine.lock()) {
		bool * pds = (bool *) waveEngine.getData(Fixity);
		if (pds != nullptr) {
			unsigned int sizesize = waveEngine.getSize();
			sizesize *= sizesize;
			for (unsigned int i = 0; i < sizesize; i++) {
				pds[i] = true;
			}
		}
		waveEngine.unlock();
	}
}

void WindowHandler::on_set_to_cpu_cores_clicked(Button*button) {
	spinbutton_num_co->set_value(get_nprocs_conf());
}

void WindowHandler::on_clear_static_clicked(Button*button) {
	if (waveEngine.lock()) {
		bool * pds = (bool *) waveEngine.getData(Fixity);
		if (pds != nullptr) {
			unsigned int sizesize = waveEngine.getSize();
			sizesize *= sizesize;
			for (unsigned int i = 0; i < sizesize; i++) {
				pds[i] = false;
			}
		}
		waveEngine.unlock();
	}
}
void WindowHandler::on_crest_changed(ColorButton*colorButton) {
	Gdk::Color bcolor = colorButton->get_color();
	uint32_t color = (uint8_t) (bcolor.get_red_p() * 255.0)
			+ ((uint8_t) (bcolor.get_green_p() * 255.0) << 8)
			+ ((uint8_t) (bcolor.get_blue_p() * 255.0) << 16);
	waveEngine.setCrestColor(color);
}
void WindowHandler::on_trough_changed(ColorButton*colorButton) {
	Gdk::Color bcolor = colorButton->get_color();
	uint32_t color = (uint8_t) (bcolor.get_red_p() * 255.0)
			+ ((uint8_t) (bcolor.get_green_p() * 255.0) << 8)
			+ ((uint8_t) (bcolor.get_blue_p() * 255.0) << 16);
	waveEngine.setTroughColor(color);
}
void WindowHandler::on_static_color_changed(ColorButton*colorButton) {
	Gdk::Color bcolor = colorButton->get_color();
	uint32_t color = (uint8_t) (bcolor.get_red_p() * 255.0)
			+ ((uint8_t) (bcolor.get_green_p() * 255.0) << 8)
			+ ((uint8_t) (bcolor.get_blue_p() * 255.0) << 16);
	waveEngine.setStaticColor(color);
}
void WindowHandler::on_dec_place_changed(SpinButton*spinButton) {
	changeDigitCount(spinButton->get_value_as_int());
}

void WindowHandler::on_activate(Widget*widget) {
	ImageMenuItem * imageMenuItem = dynamic_cast<ImageMenuItem*>(widget);

	if (imageMenuItem != nullptr) {
		if (imageMenuItem == imagemenuitem_quit)
			mainWindow->close();
		else if (imageMenuItem == imagemenuitem_manual) {
			ustring cmd = "xdg-open ./data/help/wavesim_help.pdf";
			//helpWindow->show();
			if (showMsg(
					"The following command is going to be executed to view PDF file:\n\n"
							+ cmd
							+ "\n\nSupposedly, a third-party program will open the file. Proceed?",
					MessageType::MESSAGE_QUESTION, ButtonsType::BUTTONS_YES_NO,
					mainWindow) == GTK_RESPONSE_YES) {
				int ret = system(cmd.c_str());
				std::cout
						<< "system() is called for opening the help file. Returned "
						<< ret << std::endl;
			}
		} else if (imageMenuItem == imagemenuitem_about) {
			//if (aboutdialog->run() == GTK_RESPONSE_DELETE_EVENT) {
			aboutdialog->run();
			aboutdialog->close();
			//}
		}

	}
}

void WindowHandler::on_open_clicked(ToolButton*toolButton) {

	scanProjects(treeview_projects_open, openProjectDialog);

	while (openProjectDialog->run() == GTK_RESPONSE_OK) {
		ListStore::iterator selected;
		ustring name;
		selected = treeview_projects_open->get_selection()->get_selected();
		if (selected != nullptr) {
			(*selected).get_value<ustring>(0, name);
			string description;
			ProjectHandlerResult result = projectHandler.openProject(name,
					description, &waveEngine);
			if (result == OK) {
				textview_description->get_buffer()->set_text(description);
				lastProjectName = name;
				mainWindow->set_title(
						ustring("Wave Simulator - ") + lastProjectName);
				unsigned int size = waveEngine.getSize();
				m_image = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8,
						size, size);
				updateSelectedOsc();
				updatePoolGeneral();
				notebook1->set_current_page(4); // Notes
				std::cout << "open ok " << name << std::endl;
				break;
			} else if (result == NOT_FOUND) {
				showMsg("Project no longer exists.", MESSAGE_ERROR, BUTTONS_OK,
						openProjectDialog);
			} else if (result == ERROR) {
				showMsg("Error occurred while opening the project.",
						MESSAGE_ERROR, BUTTONS_OK, openProjectDialog);
			}

		} else {
			showMsg("Please select a project from the list.", MESSAGE_ERROR,
					BUTTONS_OK, openProjectDialog);
		}

	}
	openProjectDialog->close();

}

void WindowHandler::on_save_clicked(ToolButton*toolButton) {

	scanProjects(treeview_projects_save, saveProjectDialog);

	while (saveProjectDialog->run() == GTK_RESPONSE_OK) {
		ListStore::iterator selected;
		ustring name = entry_project_name->get_text();
		if (name.length() > 0) {
			if (name != "default"
					|| (name == "default"
							&& showMsg(
									"You are about to overwrite the default scene which"
											" is loaded every time the program starts. Save anyways?",
									MESSAGE_QUESTION, BUTTONS_YES_NO,
									saveProjectDialog) == GTK_RESPONSE_YES)) {

				ProjectHandlerResult result = projectHandler.saveProject(name,
						textview_description->get_buffer()->get_text(false),
						&waveEngine);
				if (result == OK) {
					lastProjectName = name;
					mainWindow->set_title(
							ustring("Wave Simulator - ") + lastProjectName);
					std::cout << "save ok " << name << std::endl;
					break;
				} else {
					showMsg("Error occurred while saving the project.",
							MESSAGE_ERROR, BUTTONS_OK, saveProjectDialog);
				}
			}

		} else {
			showMsg(
					"Please enter a name or select an existing project from the list.",
					MESSAGE_ERROR, BUTTONS_OK, saveProjectDialog);
		}

	}
	saveProjectDialog->close();
}

void WindowHandler::on_new_clicked(ToolButton*toolButton) {
	string description;
	ProjectHandlerResult result = projectHandler.openProject("default",
			description, &waveEngine);
	if (result == OK) {
		textview_description->get_buffer()->set_text(description);
		lastProjectName = "default";
		mainWindow->set_title("Wave Simulator");
		unsigned int size = waveEngine.getSize();
		m_image = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, size,
				size);
		updateSelectedOsc();
		updatePoolGeneral();
	} else if (result == NOT_FOUND) {
		showMsg("Default scene could not be found. "
				"Please restart the program to generate one.", MESSAGE_ERROR,
				BUTTONS_OK, mainWindow);
	} else {
		showMsg("Error occurred while opening the default scene.",
				MESSAGE_ERROR, BUTTONS_OK, mainWindow);
	}

}

void WindowHandler::on_refresh_clicked(ToolButton*toolButton) {
	refresh_scene();
}

void WindowHandler::on_pause_clicked(ToolButton*toolButton) {
	waveEngine.setCalculationEnabled(false);
	waveEngine.setPowerSaveMode(true);
	toolButton->set_visible(false);
	toolbutton_play->set_visible(true);
}

void WindowHandler::on_play_clicked(ToolButton*toolButton) {
	waveEngine.setCalculationEnabled(true);
	waveEngine.setPowerSaveMode(false);
	toolButton->set_visible(false);
	toolbutton_pause->set_visible(true);
}

void WindowHandler::on_delete_clicked(Button*button) {
	deleteProject(treeview_projects_open, openProjectDialog);
}

void WindowHandler::on_source_changed(RadioButton*radioButton) {
	bool radio_active = radioButton->get_active();
	if (radio_active) {

		if (radioButton == radiobutton_point)
			waveEngine.setOscillatorSource(
					comboboxtext_sel_osc->get_active_row_number(), PointSource);
		else if (radioButton == radiobutton_line)
			waveEngine.setOscillatorSource(
					comboboxtext_sel_osc->get_active_row_number(), LineSource);
		else if (radioButton == radiobutton_moving_point)
			waveEngine.setOscillatorSource(
					comboboxtext_sel_osc->get_active_row_number(),
					MovingPointSource);
	}
}

void WindowHandler::on_projects_open_row_activated(const TreeModel::Path&,
		TreeViewColumn*, TreeView * treeView) {
	openProjectDialog->response(GTK_RESPONSE_OK);
}

void WindowHandler::on_projects_save_row_activated(const TreeModel::Path&,
		TreeViewColumn*, TreeView * treeView) {
	saveProjectDialog->response(GTK_RESPONSE_OK);
}

void WindowHandler::on_projects_save_selection_changed(
		Glib::RefPtr<TreeSelection> selection) {
	ListStore::iterator i = selection->get_selected();
	if (i != nullptr) {
		ustring str;
		(*i)->get_value<ustring>(0, str);
		entry_project_name->set_text(str);
	}

}

void WindowHandler::refresh_scene() {
	unsigned int size = waveEngine.getSize();
	unsigned int sizesize = size * size;
	if (waveEngine.lock()) {
#if defined(WAVE_ENGINE_SPRING_MODEL)
		double * pdv = (double*) waveEngine.getData(
				ParticleAttribute::Velocity);
		double * pd = (double*) waveEngine.getData(ParticleAttribute::Height);
		if (pd != nullptr && pdv != nullptr) {
			for (unsigned int i = 0; i < sizesize; i++) {
				pd[i] = 0;
				pdv[i] = 0;
			}
		}
#elif defined(WAVE_ENGINE_X_MODEL)
#endif
		waveEngine.unlock();
	}
}
void WindowHandler::oscillator_helper(int width, int height, double x,
		double y) {
	double size = waveEngine.getSize();

	double xratio = (x - pool.x) / (pool.width);
	double yratio = (y - pool.y) / (pool.height);

	waveEngine.setOscillatorLocation(
			comboboxtext_sel_osc->get_active_row_number(),
			pressing_left_mb ? 0 : 1, Point(xratio * size, yratio * size));
	updateSelectedOsc();
}
void WindowHandler::draw_helper(int width, int height, double x, double y) {
	double size = waveEngine.getSize();

	bool mass_line_mode = switch_mass_line_mode->get_active();
	bool static_line_mode = switch_static_line_mode->get_active();
	bool edit_mass = switch_edit_mass->get_active();
	bool edit_static = switch_edit_static->get_active();
	Point m_loc = Point(((mouse_loc.x - pool.x) / (pool.width)) * size,
			((mouse_loc.y - pool.y) / (pool.height)) * size);
	Point m_loc_old;
	if ((edit_mass && mass_line_mode) || (edit_static && static_line_mode))
		m_loc_old = Point(((mouse_press_loc.x - pool.x) / (pool.width)) * size,
				((mouse_press_loc.y - pool.y) / (pool.height)) * size);
	else
		m_loc_old = Point(((mouse_loc_old.x - pool.x) / (pool.width)) * size,
				((mouse_loc_old.y - pool.y) / (pool.height)) * size);
	if (waveEngine.lock()) {
		if (edit_static)
			drawThickLine((bool *) waveEngine.getData(Fixity),
					pressing_right_mb
							^ (comboboxtext_static_op->get_active_row_number()
									== 0 ? true : false), size, m_loc_old,
					m_loc, spinbutton_static_pen->get_value() / 2);
		else if (edit_mass)
			drawThickLine(
					(double *) waveEngine.getData(ParticleAttribute::Mass),
					pressing_left_mb ?
							spinbutton_primary_mass->get_value() :
							spinbutton_secondary_mass->get_value(), size,
					m_loc_old, m_loc, spinbutton_mass_pen->get_value() / 2);
		waveEngine.unlock();
	}

}

int WindowHandler::showMsg(ustring msg, MessageType msgType,
		ButtonsType buttonsType, Window * transient) {
	MessageDialog md = MessageDialog(msg, true, msgType, buttonsType, true);
	md.set_transient_for(*transient);
	return md.run();
}
template<typename array, typename value> void WindowHandler::iterateShape(
		array a, value v, Point p1, Point p2, int x, int y, double radius,
		int size) {
	for (int sy = p1.y; sy < p2.y; sy++) {
		for (int sx = p1.x; sx < p2.x; sx++) {
			if (round(sqrt(pow(sx - x, 2) + pow(sy - y, 2))) <= radius)
				a[sx + sy * size] = v;
		}
	}
}

template<typename array, typename value>
void WindowHandler::drawThickLine(array arr, value val, unsigned int size,
		Point p1, Point p2, double radius) {
	double angle = vectorToAngle(p2 - p1);
	double sind = sin(angle);
	double cosd = cos(angle);
	for (double i = -radius + 0.5; i <= radius - 0.5; i += 0.5) {
		double xr = i * sind;
		double yr = i * cosd;
		double circle_eq = sqrt(1 - pow(i / radius, 2));
		double xpeak = radius * cosd * circle_eq;
		double ypeak = radius * sind * circle_eq;
		drawLine(arr, val, size, Point(p1.x + xr - xpeak, p1.y + yr + ypeak),
				Point(p2.x + xr + xpeak, p2.y + yr - ypeak));
	}

}
template<typename array, typename value>
void WindowHandler::drawLine(array arr, value val, unsigned int size, Point p1,
		Point p2) {
	double length = Point::dist(p1, p2);
	if (length == 0)
		return;
	//double maxval = max(abs(p2.x - p1.x), abs(p2.y - p1.y));
	double xoverl = (p2.x - p1.x) / length;
	double yoverl = (p2.y - p1.y) / length;
	//int iLength = (int) length;
	Point currentPoint;
	for (double i = 0; i < length; i += 0.5) {
		currentPoint = Point(p1.x + (xoverl * i), p1.y + (yoverl * i));
		if (currentPoint.x < size && currentPoint.x >= 0
				&& currentPoint.y < size && currentPoint.y >= 0)
			arr[(unsigned int) floor(currentPoint.x)
					+ size * (unsigned int) floor(currentPoint.y)] = val;
	}
}

double WindowHandler::vectorToAngle(Point p) {
	int region = 1; // Upper-right
	double length = Point::dist(Point(0, 0), p);
	if (length == 0)
		return 0;
	// Find which region 'p' is at.
	if (p.x < 0 && p.y <= 0)
		region = 2; // Upper-left
	else if (p.x < 0 && p.y > 0)
		region = 3; // Lower-left
	else if (p.x >= 0 && p.y > 0)
		region = 4; // Lower-Right
	double angle = asin(abs(p.y) / length);
	switch (region) {
	case 2:
		angle = M_PI - angle;
		break;
	case 3:
		angle += M_PI;
		break;
	case 4:
		angle = 2 * M_PI - angle;
		break;
	}
	return angle;
}

template<typename widget, typename signal, typename callback>
sigc::connection WindowHandler::connectEvent(widget w, signal s, callback c) {
	return s.connect(sigc::bind<widget>(sigc::mem_fun(*this, c), w), false);
}

void WindowHandler::changeDigitCount(unsigned int count) {
	spinbutton_loss->set_digits(count);
	spinbutton_abs_peak->set_digits(count);
	spinbutton_period->set_digits(count);
	spinbutton_phase->set_digits(count);
	spinbutton_amp->set_digits(count);
	spinbutton_move_period->set_digits(count);
	spinbutton_primary_mass->set_digits(count);
	spinbutton_secondary_mass->set_digits(count);
	spinbutton_mass_min->set_digits(count);
	spinbutton_mass_max->set_digits(count);
	spinbutton_max_iterate->set_digits(count);
	spinbutton_max_fps->set_digits(count);
}

void WindowHandler::updateSelectedOsc() {
	osc_enabled_changed_con.block(true);
	on_period_changed_con.block(true);
	on_phase_changed_con.block(true);
	on_amp_changed_con.block(true);
	on_locx1_changed_con.block(true);
	on_locy1_changed_con.block(true);
	on_locx2_changed_con.block(true);
	on_locy2_changed_con.block(true);
	on_move_period_changed_con.block(true);
	for (int i = 0; i < 3; i++)
		on_source_changed_con[i].block(true);
	unsigned int rowId = comboboxtext_sel_osc->get_active_row_number();
	switch_osc_enabled->set_active(waveEngine.getOscillatorEnabled(rowId));
	spinbutton_period->set_value(waveEngine.getOscillatorPeriod(rowId));
	spinbutton_phase->set_value(waveEngine.getOscillatorPhase(rowId));
	spinbutton_amp->set_value(waveEngine.getOscillatorAmplitude(rowId));
	Point p1 = waveEngine.getOscillatorLocation(rowId, 0), p2 =
			waveEngine.getOscillatorLocation(rowId, 1);
	spinbutton_locx1->set_value(p1.x);
	spinbutton_locy1->set_value(p1.y);
	spinbutton_locx2->set_value(p2.x);
	spinbutton_locy2->set_value(p2.y);
	spinbutton_move_period->set_value(
			waveEngine.getOscillatorMovePeriod(rowId));
	switch (waveEngine.getOscillatorSource(rowId)) {
	case PointSource:
		radiobutton_point->set_active(true);
		break;
	case LineSource:
		radiobutton_line->set_active(true);
		break;
	case MovingPointSource:
		radiobutton_moving_point->set_active(true);
		break;
	}
	osc_enabled_changed_con.block(false);
	on_period_changed_con.block(false);
	on_phase_changed_con.block(false);
	on_amp_changed_con.block(false);
	on_locx1_changed_con.block(false);
	on_locy1_changed_con.block(false);
	on_locx2_changed_con.block(false);
	on_locy2_changed_con.block(false);
	on_move_period_changed_con.block(false);
	for (int i = 0; i < 3; i++)
		on_source_changed_con[i].block(false);
}

void WindowHandler::updatePoolGeneral() {
	size_changed_con.block(true);
	loss_changed_con.block(true);
	abs_peak_changed_con.block(true);
	abs_thick_changed_con.block(true);
	shift_center_changed_con.block(true);
	enable_absorb_changed_con.block(true);
	spinbutton_size->set_value(waveEngine.getSize());
	spinbutton_loss->set_value(waveEngine.getLossRatio());
	spinbutton_abs_peak->set_value(waveEngine.getAbsorberLossRatio());
	spinbutton_abs_thick->set_value(waveEngine.getAbsorberThickness());
	switch_shift_center->set_active(waveEngine.getShiftParticlesEnabled());
	switch_enable_absorb->set_active(waveEngine.getAbsorberEnabled());
	size_changed_con.block(false);
	loss_changed_con.block(false);
	abs_peak_changed_con.block(false);
	abs_thick_changed_con.block(false);
	shift_center_changed_con.block(false);
	enable_absorb_changed_con.block(false);
}

void WindowHandler::updateCursor(bool arrow) {
	bool edit = switch_edit_mass->get_active()
			|| switch_edit_static->get_active();
	if (!edit) {
		drawingarea_main->get_parent_window()->set_cursor(cursorCrosshair);
	} else {
		drawingarea_main->get_parent_window()->set_cursor(cursorBlank);
	}
}

void WindowHandler::scanProjects(TreeView*treeView, Dialog * transient) {
	liststore_projects->clear();
	vector<string> projects;
	if (projectHandler.getProjectsList(projects) == OK) {
		for (string project : projects) {
			ListStore::Row i = *liststore_projects->append();
			i.set_value<ustring>(0, project);
			if (project == lastProjectName) {
				treeView->get_selection()->select(i);
			}
		}
	} else {
		showMsg("Error occurred while scanning the project folder.",
				MESSAGE_ERROR, BUTTONS_OK, transient);
		return;
	}
}

void WindowHandler::deleteProject(TreeView*treeView, Dialog * transient) {
	ListStore::iterator selected;
	ustring name;
	selected = treeView->get_selection()->get_selected();
	if (selected != nullptr) {
		(*selected).get_value<ustring>(0, name);
		ustring msg;
		if (name == "default")
			msg =
					"Delete the default scene if only it is in a bad condition."
							" Are you sure you want to delete it? This can't be undone!";
		else
			msg = "Are you sure you want to delete '" + name
					+ "'? This can't be undone!";
		if (showMsg(msg, MessageType::MESSAGE_QUESTION,
				ButtonsType::BUTTONS_YES_NO, transient) == GTK_RESPONSE_YES) {
			if (projectHandler.deleteProject(name) != OK)
				showMsg("'" + name + "' could not be deleted.",
						MessageType::MESSAGE_ERROR, ButtonsType::BUTTONS_OK,
						transient);
			else {
				scanProjects(treeView, transient);
			}
		}
	} else
		showMsg("Please select a project from the list.", MESSAGE_ERROR,
				BUTTONS_OK, transient);
}

void WindowHandler::updateInfoLabel() {
	if (pointer_in_drawing_area) {
		unsigned int sz = waveEngine.getSize();
		double locx = sz * (mouse_loc.x - pool.x) / pool.width;
		double locy = sz * (mouse_loc.y - pool.y) / pool.height;
		if (locx >= 0 && locx < sz && locy >= 0 && locy < sz) {
			char info[256];
			double mass = 0, loss = 0;
			int static_particle = 0;
			double height = 0;
			if (waveEngine.lock()) {
				unsigned int index = (int) locx + (int) locy * sz;
				mass =
						((double_t*) waveEngine.getData(ParticleAttribute::Mass))[index];
				static_particle = ((uint8_t*) waveEngine.getData(
						ParticleAttribute::Fixity))[index];
				loss =
						((double_t*) waveEngine.getData(ParticleAttribute::Loss))[index];
				height = ((double_t*) waveEngine.getData(
						ParticleAttribute::Height))[index];
				waveEngine.unlock();
				sprintf(info,
						"x = %d\ty = %d\tmass = %f\tloss = %f\theight = %f\tstatic = %d",
						(int) locx, (int) locy, mass, loss, height,
						static_particle);
				label_info->set_text(info);
				return;
			}
		}
	}
	label_info->set_text("");
}

void WindowHandler::renderCallback(uint8_t * bitmap_data,
		unsigned long data_length, void * extra_data) {
	WindowHandler * windowHandler = (WindowHandler *) extra_data;
	if (windowHandler->m_image && !windowHandler->destroying) {
		unsigned int height = windowHandler->m_image->get_height();
		unsigned int widthbyte = height
				* windowHandler->m_image->get_n_channels()
				* windowHandler->m_image->get_bits_per_sample() / 8;
		unsigned int rowsize = windowHandler->m_image->get_rowstride();
		uint8_t * address = windowHandler->m_image->get_pixels();
		for (unsigned int i = 0; i < height; i++) {
			memcpy(address + rowsize * i, bitmap_data + widthbyte * i,
					widthbyte);
		}

		windowHandler->dispatcher.emit();
	}
}

bool WindowHandler::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {

	Allocation allocation = drawingarea_main->get_allocation();
	const double width = allocation.get_width();
	const double height = allocation.get_height();

	if (!m_image)
		return false;
	double size = waveEngine.getSize();
	double scalex = (double) height / size;
	double scaley = (double) width / size;
	double scalecommon = scalex < scaley ? scalex : scaley;
	double newsize_w = scalecommon * size;
	double newsize_h = scalecommon * size;

	// Pool
	cr->save();

	cr->rectangle(pool.x, pool.y, pool.width, pool.height);
	cr->clip();

	cr->translate((width - newsize_w) / 2.0, (height - newsize_h) / 2.0);

	cr->scale(scalecommon, scalecommon);

	Gdk::Cairo::set_source_pixbuf(cr, m_image, 0, 0);
	//Cairo::RefPtr<Cairo::SurfacePattern>::cast_static(cr->get_source())->set_filter(Cairo::FILTER_FAST);
	cr->paint();

	cr->restore();

	bool edit_mass = switch_edit_mass->get_active();
	bool edit_static = switch_edit_static->get_active();
	if ((draw_preview || pressing_left_mb || pressing_right_mb)
			&& (edit_mass || edit_static)) {

		valarray<double> dash { 5, /* ink */
		5, /* skip */
		};
		double radius =
				edit_mass ?
						scalecommon * spinbutton_mass_pen->get_value() / 2 :
						scalecommon * spinbutton_static_pen->get_value() / 2;
		Cairo::Path * preview_path;
		if ((pressing_left_mb || pressing_right_mb)
				&& ((edit_mass && switch_mass_line_mode->get_active())
						|| (edit_static && switch_static_line_mode->get_active()))) {
			// Line Mode
			cr->save();
			double vector_angle = vectorToAngle(
					Point(mouse_loc.x - mouse_press_loc.x,
							mouse_loc.y - mouse_press_loc.y));
			cr->arc(mouse_press_loc.x, mouse_press_loc.y, radius,
			M_PI / 2 - vector_angle, 3 * M_PI / 2 - vector_angle);
			cr->arc(mouse_loc.x, mouse_loc.y, radius,
					3 * M_PI / 2 - vector_angle,
					M_PI / 2 - vector_angle);
			cr->close_path();
			preview_path = cr->copy_path();
			cr->begin_new_path();
			cr->restore();
		} else {
			// Dashed circle
			cr->save();
			cr->arc(mouse_loc.x, mouse_loc.y, radius, 0, M_PI * 2.0);
			preview_path = cr->copy_path();
			cr->begin_new_path();
			cr->restore();
		}

		// Draw the preview path
		cr->save();
		cr->rectangle(pool.x, pool.y, pool.width, pool.height);
		cr->clip();
		cr->set_dash(dash, 0);
		cr->set_source_rgb(1, 1, 1);
		cr->append_path(*preview_path);
		cr->stroke();
		cr->set_dash(dash, 5);
		cr->set_source_rgb(0, 0, 0);
		cr->append_path(*preview_path);
		cr->stroke();

		cr->restore();

		// Intersection lines
		cr->save();
		cr->append_path(*preview_path);
		cr->clip();
		cr->set_dash(dash, 0);
		cr->set_source_rgb(1, 1, 1);
		cr->rectangle(pool.x + 1, pool.y + 1, pool.width - 2, pool.height - 2);
		cr->stroke();
		cr->set_dash(dash, 5);
		cr->set_source_rgb(0, 0, 0);
		cr->rectangle(pool.x + 1, pool.y + 1, pool.width - 2, pool.height - 2);
		cr->stroke();
		cr->restore();

		free(preview_path);

		// Cursor
		cr->save();
		cr->rectangle(0, 0, pool.x, pool.height);
		cr->rectangle(pool.x + pool.width, 0, pool.x, pool.height);
		cr->clip();
		RefPtr<Gdk::Pixbuf> crosshair = cursorCrosshair->get_image();
		cr->translate(mouse_loc.x - crosshair->get_width() / 2,
				mouse_loc.y - crosshair->get_height() / 2);

		Gdk::Cairo::set_source_pixbuf(cr, crosshair, 0, 0);
		cr->paint();
		cr->restore();

	}

	if (switch_show_oscs->get_active()) {

		const double font_size = scale_font_size->get_value();
		for (int i = 0; i < MAX_NUMBER_OF_OSCILLATORS; i++) {
			if (waveEngine.getOscillatorEnabled(i)) {
				// Oscillator Text
				cr->save();
				cr->translate(pool.x, pool.y);
				Point osc_loc = waveEngine.getOscillatorRealLocation(i);
				Point osc_display_loc = Point(osc_loc.x * scalecommon,
						osc_loc.y * scalecommon);
				cr->set_source_rgb(1, 1, 1);
				cr->translate(osc_display_loc.x, osc_display_loc.y);
				cr->set_font_size(font_size);
				Cairo::TextExtents te;
				cr->get_text_extents(osc_texts[i], te);
				cr->translate(-te.width / 2, te.height / 2);
				cr->text_path(osc_texts[i]);
				cr->fill_preserve();
				cr->set_line_width(font_size / 20);
				cr->set_source_rgb(0, 0, 0);
				cr->stroke();
				cr->restore();
			}

		}

	}

	return false;
}

void WindowHandler::on_notification_from_render_thread() {

	drawingarea_main->get_window()->invalidate(false);
	updateInfoLabel();
}

WindowHandler::~WindowHandler() {
	destroying = true;
}
