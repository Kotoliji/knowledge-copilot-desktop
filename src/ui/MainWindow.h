#pragma once

#include "../api/Client.h"
#include "AskView.h"
#include "DocumentsView.h"

#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/notebook.h>

namespace kc::ui {

class MainWindow : public Gtk::ApplicationWindow {
public:
    MainWindow();
    ~MainWindow() override;

private:
    void on_connect_clicked();
    void load_settings();
    void save_settings();

    kc::Client client_;

    Gtk::Box root_{Gtk::Orientation::VERTICAL, 0};
    Gtk::Box settings_bar_{Gtk::Orientation::HORIZONTAL, 6};
    Gtk::Entry base_url_entry_;
    Gtk::Entry api_key_entry_;
    Gtk::Button connect_button_{"Connect"};
    Gtk::Label connection_status_;

    Gtk::Notebook tabs_;
    AskView ask_view_;
    DocumentsView documents_view_;
};

} // namespace kc::ui
