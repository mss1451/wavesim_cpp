#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <sys/sysinfo.h>
#include <gtkmm.h>
#include <gtkmm/window.h>
#include <gdkmm/window.h>
#include <cairomm/context.h>
#include <gdkmm/general.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include <iostream>
#include "wave_engine.h"
#include "INIReader.h"
#include "project_handler.h"

using namespace Gtk;
using namespace Glib;
using namespace WaveSimulation;

enum DrawType { Static, Mass };

class WindowHandler {
public:
  WindowHandler(RefPtr<Gtk::Builder>);
  virtual ~WindowHandler();

protected:
  Window *mainWindow;

  DrawingArea *drawingarea_main;

  RefPtr<Gdk::Cursor> cursorCrosshair, cursorBlank;

  AboutDialog *aboutdialog;

  Dialog *openProjectDialog, *saveProjectDialog;

  TreeView *treeview_projects_open, *treeview_projects_save;

  RefPtr<ListStore> liststore_projects;

  Entry *entry_project_name;

  TextView *textview_description;

  Label *label_info;

  Notebook *notebook1;

  SpinButton *spinbutton_size, *spinbutton_loss, *spinbutton_abs_peak,
      *spinbutton_abs_thick, *spinbutton_period, *spinbutton_phase,
      *spinbutton_amp, *spinbutton_locx1, *spinbutton_locy1, *spinbutton_locx2,
      *spinbutton_locy2, *spinbutton_move_period, *spinbutton_mass_pen,
      *spinbutton_primary_mass, *spinbutton_secondary_mass,
      *spinbutton_mass_min, *spinbutton_mass_max, *spinbutton_static_pen,
      *spinbutton_max_iterate, *spinbutton_num_co, *spinbutton_thread_sleep,
      *spinbutton_max_fps, *spinbutton_amp_mult_max, *spinbutton_dec_place;

  Switch *switch_shift_center, *switch_enable_absorb, *switch_edit_mass,
      *switch_mass_line_mode, *switch_osc_enabled, *switch_show_oscs,
      *switch_edit_static, *switch_static_line_mode, *switch_render,
      *switch_extreme_cont, *switch_ignore_num_scroll,
      *switch_ignore_list_scroll, *switch_ignore_scale_scroll;

  RadioButton *radiobutton_point, *radiobutton_line, *radiobutton_moving_point;

  Button *button_fill_mass, *button_clear_mass, *button_swap_mass,
      *button_fill_static, *button_clear_static, *button_set_to_cpu_cores,
      *button_delete;

  ColorButton *colorbutton_crest, *colorbutton_trough, *colorbutton_static;

  ComboBoxText *comboboxtext_static_op, *comboboxtext_sel_osc;

  Scale *scale_font_size, *scale_amp_mult;

  ImageMenuItem *imagemenuitem_quit, *imagemenuitem_manual,
      *imagemenuitem_about;

  ToolButton *toolbutton_refresh, *toolbutton_pause, *toolbutton_play,
      *toolbutton_new, *toolbutton_save, *toolbutton_open;

  // Signal handlers:
  bool on_drawing_area_button_pressed(GdkEventButton *, DrawingArea *);
  bool on_drawing_area_button_released(GdkEventButton *, DrawingArea *);
  bool on_main_window_button_pressed(GdkEventButton *, Window *);
  bool on_drawing_area_motion_notify(GdkEventMotion *, DrawingArea *);
  bool on_drawing_area_enter_notify(GdkEventCrossing *, DrawingArea *);
  bool on_drawing_area_leave_notify(GdkEventCrossing *, DrawingArea *);
  bool on_main_window_key_press(GdkEventKey *, Window *);
  bool on_main_window_delete(GdkEventAny *, Window *);
  bool on_drawing_area_scroll(GdkEventScroll *, DrawingArea *);
  bool on_scroll(GdkEventScroll *, Widget *);
  void on_activate(Widget *);

  void on_size_changed(SpinButton *);
  void on_loss_changed(SpinButton *);
  void on_abs_peak_changed(SpinButton *);
  void on_abs_thick_changed(SpinButton *);
  void on_shift_center_changed(StateType, Switch *);
  void on_enable_absorb_changed(StateType, Switch *);
  void on_sel_osc_changed(ComboBoxText *);
  void on_osc_enabled_changed(StateType, Switch *);
  void on_period_changed(SpinButton *);
  void on_phase_changed(SpinButton *);
  void on_amp_changed(SpinButton *);
  void on_locx1_changed(SpinButton *);
  void on_locy1_changed(SpinButton *);
  void on_locx2_changed(SpinButton *);
  void on_locy2_changed(SpinButton *);
  void on_move_period_changed(SpinButton *);
  void on_edit_mass_changed(StateType, Switch *);
  void on_mass_min_changed(SpinButton *);
  void on_mass_max_changed(SpinButton *);
  void on_edit_static_changed(StateType, Switch *);
  void on_max_iterate_changed(SpinButton *);
  void on_num_co_changed(SpinButton *);
  void on_thread_sleep_changed(SpinButton *);
  void on_render_changed(StateType, Switch *);
  void on_max_fps_changed(SpinButton *);
  void on_amp_mult_changed(Scale *);
  void on_amp_mult_max_changed(SpinButton *);
  void on_mass_pen_changed(SpinButton *);
  void on_static_pen_changed(SpinButton *);
  void on_extreme_cont_changed(StateType, Switch *);
  void on_crest_changed(ColorButton *);
  void on_trough_changed(ColorButton *);
  void on_static_color_changed(ColorButton *);
  void on_dec_place_changed(SpinButton *);
  void on_fill_mass_clicked(Button *);
  void on_clear_mass_clicked(Button *);
  void on_swap_mass_clicked(Button *);
  void on_fill_static_clicked(Button *);
  void on_set_to_cpu_cores_clicked(Button *);
  void on_clear_static_clicked(Button *);
  void on_open_clicked(ToolButton *);
  void on_save_clicked(ToolButton *);
  void on_new_clicked(ToolButton *);
  void on_refresh_clicked(ToolButton *);
  void on_pause_clicked(ToolButton *);
  void on_play_clicked(ToolButton *);
  void on_delete_clicked(Button *);

  void on_source_changed(RadioButton *);

  void on_projects_open_row_activated(const TreeModel::Path &, TreeViewColumn *,
                                      TreeView *treeView);
  void on_projects_save_row_activated(const TreeModel::Path &, TreeViewColumn *,
                                      TreeView *treeView);
  void on_projects_save_selection_changed(Glib::RefPtr<TreeSelection>);

  void on_drawing_area_size_allocate(Allocation &, DrawingArea *);

  bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr);
  void on_notification_from_render_thread();

  // Connections
  sigc::connection osc_enabled_changed_con, on_source_changed_con[3],
      on_period_changed_con, on_phase_changed_con, on_amp_changed_con,
      on_locx1_changed_con, on_locy1_changed_con, on_locx2_changed_con,
      on_locy2_changed_con, on_move_period_changed_con;

  sigc::connection size_changed_con, loss_changed_con, abs_peak_changed_con,
      abs_thick_changed_con, shift_center_changed_con,
      enable_absorb_changed_con;

  // Wave Engine related
  Dispatcher dispatcher;
  WaveEngine waveEngine;
  static void renderCallback(uint8_t *bitmap_data, unsigned long data_length,
                             void *extra_data);
  RefPtr<Gdk::Pixbuf> m_image;
  bool destroying = false;

  // Save/Load
  ProjectHandler projectHandler;
  ustring lastProjectName;

  bool pressing_left_mb = false;
  bool pressing_right_mb = false;
  bool pressing_middle_mb = false;

  // Mouse Positions for drawing lines
  Point mouse_loc, mouse_loc_old, mouse_press_loc;

  // Drawing cursor used to preview the shape and size of pen in real-time.
  bool draw_preview = false;

  // Is pointer in the drawing area?
  bool pointer_in_drawing_area = false;

  // Pool area
  Rectangle pool;

  // Oscillator indicators
  vector<ustring> osc_texts = {"1", "2", "3", "4", "5", "6", "7", "8", "9"};

  // Helper functions
  int showMsg(ustring msg, MessageType msgType, ButtonsType buttonsType,
              Window *transient);
  template <typename array, typename value>
  void iterateShape(array, value, Point p1, Point p2, int x, int y,
                    double radius, int size);
  void refresh_scene();
  void oscillator_helper(int width, int height, double x, double y);
  void draw_helper(int width, int height, double x, double y);
  template <typename array, typename value>
  void drawThickLine(array, value, unsigned int size, Point p1, Point p2,
                     double radius);
  template <typename array, typename value>
  void drawLine(array, value, unsigned int size, Point p1, Point p2);
  double vectorToAngle(Point p);
  template <typename widget, typename signal, typename callback>
  sigc::connection connectEvent(widget, signal, callback);
  void changeDigitCount(unsigned int);
  void updateSelectedOsc();
  void updatePoolGeneral();
  void updateCursor(bool arrow);
  void scanProjects(TreeView *, Dialog *);
  void deleteProject(TreeView *, Dialog *);
  void updateInfoLabel();
};

#endif // GTKMM_MAINWINDOW_H
