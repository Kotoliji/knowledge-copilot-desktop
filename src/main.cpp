#include "ui/MainWindow.h"
#include <gtkmm/application.h>

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create("dev.kotoliji.knowledge-copilot-desktop");
    return app->make_window_and_run<kc::ui::MainWindow>(argc, argv);
}
