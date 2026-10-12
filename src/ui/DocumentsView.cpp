#include "DocumentsView.h"
#include "Async.h"

#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <sstream>

namespace kc::ui {

DocumentsView::DocumentsView(kc::Client& client)
    : Gtk::Box(Gtk::Orientation::VERTICAL, 8), client_(client) {
    set_margin(12);

    upload_button_.signal_clicked().connect(sigc::mem_fun(*this, &DocumentsView::on_upload_clicked));
    refresh_button_.signal_clicked().connect(sigc::mem_fun(*this, &DocumentsView::refresh));
    toolbar_.append(upload_button_);
    toolbar_.append(refresh_button_);
    append(toolbar_);

    status_label_.set_halign(Gtk::Align::START);
    append(status_label_);

    list_scroller_.set_child(documents_list_);
    list_scroller_.set_vexpand(true);
    append(list_scroller_);
}

void DocumentsView::refresh() {
    show_status("Loading...");
    kc::Client* client = &client_;

    run_async<kc::DocumentsResponse>(
        [client]() { return client->list_documents(); },
        [this](kc::DocumentsResponse* result, std::exception_ptr error) {
            if (error) {
                try {
                    std::rethrow_exception(error);
                } catch (const std::exception& e) {
                    show_status(std::string("Could not load documents: ") + e.what());
                }
                return;
            }
            render_documents(*result);
        });
}

void DocumentsView::render_documents(const kc::DocumentsResponse& docs) {
    while (auto* row = documents_list_.get_row_at_index(0)) {
        documents_list_.remove(*row);
    }

    for (const auto& source : docs.sources) {
        auto row_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
        row_box->set_margin(6);

        auto label = Gtk::make_managed<Gtk::Label>(source);
        label->set_hexpand(true);
        label->set_halign(Gtk::Align::START);
        row_box->append(*label);

        auto delete_button = Gtk::make_managed<Gtk::Button>("Delete");
        delete_button->signal_clicked().connect(
            [this, source]() { on_delete_clicked(source); });
        row_box->append(*delete_button);

        documents_list_.append(*row_box);
    }

    std::ostringstream status;
    status << docs.sources.size() << " document(s), " << docs.chunks << " chunk(s) total";
    show_status(status.str());
}

void DocumentsView::on_upload_clicked() {
    file_chooser_ = Gtk::FileChooserNative::create(
        "Select a document to upload", Gtk::FileChooser::Action::OPEN, "Upload", "Cancel");
    file_chooser_->signal_response().connect(sigc::mem_fun(*this, &DocumentsView::on_file_chosen));
    file_chooser_->show();
}

void DocumentsView::on_file_chosen(int response_id) {
    if (response_id != GTK_RESPONSE_ACCEPT) return;

    auto file = file_chooser_->get_file();
    if (!file) return;
    std::string path = file->get_path();

    show_status("Uploading " + path + "...");
    kc::Client* client = &client_;

    run_async<kc::IngestResponse>(
        [client, path]() { return client->ingest_file(path); },
        [this](kc::IngestResponse* result, std::exception_ptr error) {
            if (error) {
                try {
                    std::rethrow_exception(error);
                } catch (const std::exception& e) {
                    show_status(std::string("Upload failed: ") + e.what());
                }
                return;
            }
            show_status("Ingested " + result->source + " (" + std::to_string(result->chunks) + " chunks)");
            refresh();
        });
}

void DocumentsView::on_delete_clicked(const std::string& source) {
    show_status("Deleting " + source + "...");
    kc::Client* client = &client_;

    run_async_void(
        [client, source]() { client->delete_document(source); },
        [this](std::exception_ptr error) {
            if (error) {
                try {
                    std::rethrow_exception(error);
                } catch (const std::exception& e) {
                    show_status(std::string("Delete failed: ") + e.what());
                }
                return;
            }
            refresh();
        });
}

void DocumentsView::show_status(const std::string& message) {
    status_label_.set_text(message);
}

} // namespace kc::ui
