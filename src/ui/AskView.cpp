#include "AskView.h"
#include "Async.h"

#include <gtkmm/expander.h>
#include <gtkmm/label.h>
#include <sstream>

namespace kc::ui {

AskView::AskView(kc::Client& client)
    : Gtk::Box(Gtk::Orientation::VERTICAL, 8), client_(client) {
    set_margin(12);

    question_entry_.set_placeholder_text("Ask a question about your documents...");
    question_entry_.set_hexpand(true);
    question_entry_.signal_activate().connect(sigc::mem_fun(*this, &AskView::on_ask_clicked));

    top_k_spin_.set_range(1, 10);
    top_k_spin_.set_increments(1, 1);
    top_k_spin_.set_value(4);
    top_k_spin_.set_tooltip_text("Number of source passages to retrieve");

    ask_button_.signal_clicked().connect(sigc::mem_fun(*this, &AskView::on_ask_clicked));

    input_row_.append(question_entry_);
    input_row_.append(top_k_spin_);
    input_row_.append(ask_button_);
    append(input_row_);

    status_label_.set_halign(Gtk::Align::START);
    status_label_.set_wrap(true);
    append(status_label_);

    answer_view_.set_editable(false);
    answer_view_.set_wrap_mode(Gtk::WrapMode::WORD_CHAR);
    answer_view_.set_vexpand(false);
    answer_scroller_.set_child(answer_view_);
    answer_scroller_.set_min_content_height(120);
    append(answer_scroller_);

    citations_header_.set_text("Sources");
    citations_header_.set_halign(Gtk::Align::START);
    append(citations_header_);

    citations_scroller_.set_child(citations_list_);
    citations_scroller_.set_vexpand(true);
    append(citations_scroller_);
}

void AskView::on_ask_clicked() {
    std::string question = question_entry_.get_text();
    if (question.empty()) {
        status_label_.set_text("Type a question first.");
        return;
    }

    ask_button_.set_sensitive(false);
    status_label_.set_text("Asking...");

    int top_k = static_cast<int>(top_k_spin_.get_value());
    kc::Client* client = &client_;

    run_async<kc::AskResponse>(
        [client, question, top_k]() { return client->ask(question, top_k); },
        [this](kc::AskResponse* result, std::exception_ptr error) {
            ask_button_.set_sensitive(true);
            if (error) {
                try {
                    std::rethrow_exception(error);
                } catch (const kc::ApiError& e) {
                    show_error("API error " + std::to_string(e.status_code) + ": " + e.what());
                } catch (const kc::NetworkError& e) {
                    show_error(std::string("Network error: ") + e.what());
                } catch (const std::exception& e) {
                    show_error(e.what());
                }
                return;
            }
            show_result(*result);
        });
}

void AskView::show_result(const kc::AskResponse& result) {
    answer_view_.get_buffer()->set_text(result.answer);

    std::ostringstream status;
    status << (result.grounded ? "Grounded" : "Not grounded") << " · " << result.model << " · "
           << result.citations.size() << " source(s) · "
           << result.usage.prompt_tokens + result.usage.completion_tokens << " tokens";
    status_label_.set_text(status.str());

    render_citations(result);
}

void AskView::render_citations(const kc::AskResponse& result) {
    while (auto* row = citations_list_.get_row_at_index(0)) {
        citations_list_.remove(*row);
    }

    for (const auto& citation : result.citations) {
        auto expander = Gtk::make_managed<Gtk::Expander>();
        std::ostringstream title;
        title << "[" << citation.n << "] " << citation.source;
        if (!citation.heading.empty()) title << " — " << citation.heading;
        expander->set_label(title.str());

        auto label = Gtk::make_managed<Gtk::Label>(citation.text);
        label->set_wrap(true);
        label->set_halign(Gtk::Align::START);
        label->set_margin(8);
        expander->set_child(*label);

        citations_list_.append(*expander);
    }
}

void AskView::show_error(const std::string& message) {
    status_label_.set_text(message);
}

} // namespace kc::ui
