#pragma once

#include "../api/Client.h"
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/textview.h>

namespace kc::ui {

// The "Ask" tab: type a question, see the grounded answer and the source
// passages it was built from.
class AskView : public Gtk::Box {
public:
    explicit AskView(kc::Client& client);

private:
    void on_ask_clicked();
    void show_result(const kc::AskResponse& result);
    void show_error(const std::string& message);
    void render_citations(const kc::AskResponse& result);

    kc::Client& client_;

    Gtk::Box input_row_{Gtk::Orientation::HORIZONTAL, 6};
    Gtk::Entry question_entry_;
    Gtk::SpinButton top_k_spin_;
    Gtk::Button ask_button_{"Ask"};

    Gtk::Label status_label_;

    Gtk::ScrolledWindow answer_scroller_;
    Gtk::TextView answer_view_;

    Gtk::Label citations_header_;
    Gtk::ScrolledWindow citations_scroller_;
    Gtk::ListBox citations_list_;
};

} // namespace kc::ui
