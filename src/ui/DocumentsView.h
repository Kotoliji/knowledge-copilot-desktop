#pragma once

#include "../api/Client.h"
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/filechoosernative.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/scrolledwindow.h>

namespace kc::ui {

// The "Documents" tab: what's currently ingested, plus upload/delete.
class DocumentsView : public Gtk::Box {
public:
    explicit DocumentsView(kc::Client& client);

    void refresh();

private:
    void on_upload_clicked();
    void on_file_chosen(int response_id);
    void on_delete_clicked(const std::string& source);
    void render_documents(const kc::DocumentsResponse& docs);
    void show_status(const std::string& message);

    kc::Client& client_;

    Gtk::Box toolbar_{Gtk::Orientation::HORIZONTAL, 6};
    Gtk::Button upload_button_{"Upload..."};
    Gtk::Button refresh_button_{"Refresh"};
    Gtk::Label status_label_;

    Gtk::ScrolledWindow list_scroller_;
    Gtk::ListBox documents_list_;

    Glib::RefPtr<Gtk::FileChooserNative> file_chooser_;
};

} // namespace kc::ui
