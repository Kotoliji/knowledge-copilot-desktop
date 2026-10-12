#pragma once

#include <exception>
#include <functional>
#include <glibmm/dispatcher.h>
#include <memory>
#include <thread>

namespace kc::ui {

// Runs `work` on a background thread (so a slow HTTP call doesn't freeze the
// GTK main loop) and delivers the result back on the UI thread via
// Glib::Dispatcher, which is the supported way to cross from a worker thread
// into GTK. On success `on_done` gets a pointer to the result; on failure it
// gets the exception instead so the caller can show a real error message.
template <typename T>
void run_async(std::function<T()> work, std::function<void(T*, std::exception_ptr)> on_done) {
    struct State {
        Glib::Dispatcher dispatcher;
        T result{};
        std::exception_ptr error;
        std::thread worker;
    };
    auto state = std::make_shared<State>();

    state->dispatcher.connect([state, on_done]() mutable {
        if (state->error) {
            on_done(nullptr, state->error);
        } else {
            on_done(&state->result, nullptr);
        }
        state->worker.join();
    });

    state->worker = std::thread([state, work]() mutable {
        try {
            state->result = work();
        } catch (...) {
            state->error = std::current_exception();
        }
        state->dispatcher.emit();
    });
}

// Same as above for a void-returning operation (e.g. deleting a document).
inline void run_async_void(std::function<void()> work, std::function<void(std::exception_ptr)> on_done) {
    struct State {
        Glib::Dispatcher dispatcher;
        std::exception_ptr error;
        std::thread worker;
    };
    auto state = std::make_shared<State>();

    state->dispatcher.connect([state, on_done]() mutable {
        on_done(state->error);
        state->worker.join();
    });

    state->worker = std::thread([state, work]() mutable {
        try {
            work();
        } catch (...) {
            state->error = std::current_exception();
        }
        state->dispatcher.emit();
    });
}

} // namespace kc::ui
