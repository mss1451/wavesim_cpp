#include "window_handler.h"
#include <gtkmm.h>
#include <gtkmm/application.h>
#include <iostream>

int main(int argc, char *argv[]) {
  RefPtr<Gtk::Application> app =
      Gtk::Application::create(argc, argv, "org.mss.wavesim");

  RefPtr<Gtk::Builder> builder =
      Gtk::Builder::create_from_file("./data/ui/wave_simulator.glade");

  Gtk::Window *wnd = nullptr;
  builder->get_widget("mainWindow", wnd);

  WindowHandler windowHandler(builder);

  // Shows the window and returns when it is closed.
  return app->run(*wnd);
}
