#include "MainWindow.h"
#include "Async.h"

#include <filesystem>
#include <fstream>
#include <glibmm/miscutils.h>
#include <gtkmm/label.h>

namespace kc::ui {

namespace {

std::filesystem::path config_path() {
    std::filesystem::path dir = std::filesystem::path(Glib::get_user_config_dir()) / "knowledge-copilot-desktop";
    std::filesystem::create_directories(dir);
    return dir / "config.txt";
}

} // namespace

MainWindow::MainWindow()
    : client_("http://localhost:8000"),
      ask_view_(client_),
      documents_view_(client_) {
    set_title("Knowledge Copilot Desktop");
    set_default_size(760, 560);

    base_url_entry_.set_placeholder_text("http://localhost:8000");
    base_url_entry_.set_hexpand(true);
    api_key_entry_.set_placeholder_text("API key (optional)");
    api_key_entry_.set_visibility(false);
    api_key_entry_.set_hexpand(true);
    connect_button_.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_connect_clicked));

    settings_bar_.set_margin(8);
    settings_bar_.append(base_url_entry_);
    settings_bar_.append(api_key_entry_);
    settings_bar_.append(connect_button_);
    root_.append(settings_bar_);

    connection_status_.set_halign(Gtk::Align::START);
    connection_status_.set_margin_start(8);
    connection_status_.set_margin_bottom(4);
    root_.append(connection_status_);

    tabs_.append_page(ask_view_, "Ask");
    tabs_.append_page(documents_view_, "Documents");
    tabs_.set_vexpand(true);
    root_.append(tabs_);

    set_child(root_);

    load_settings();
    on_connect_clicked();
}

MainWindow::~MainWindow() { save_settings(); }

void MainWindow::on_connect_clicked() {
    std::string url = base_url_entry_.get_text();
    if (url.empty()) url = "http://localhost:8000";
    client_.set_base_url(url);
    client_.set_api_key(api_key_entry_.get_text());

    connection_status_.set_text("Connecting...");
    kc::Client* client = &client_;

    run_async<kc::HealthResponse>(
        [client]() { return client->health(); },
        [this](kc::HealthResponse* result, std::exception_ptr error) {
            if (error) {
                try {
                    std::rethrow_exception(error);
                } catch (const std::exception& e) {
                    connection_status_.set_text(std::string("Not connected: ") + e.what());
                }
                return;
            }
            connection_status_.set_text(
                "Connected — " + result->llm_provider + "/" + result->llm_model + " · " +
                std::to_string(result->vector_store_chunks) + " chunks indexed" +
                (result->auth_required ? " · API key required" : ""));
            save_settings();
            documents_view_.refresh();
        });
}

void MainWindow::load_settings() {
    std::ifstream in(config_path());
    if (!in) return;
    std::string url, key;
    std::getline(in, url);
    std::getline(in, key);
    base_url_entry_.set_text(url);
    api_key_entry_.set_text(key);
}

void MainWindow::save_settings() {
    std::ofstream out(config_path());
    out << base_url_entry_.get_text() << "\n" << api_key_entry_.get_text() << "\n";
}

} // namespace kc::ui
