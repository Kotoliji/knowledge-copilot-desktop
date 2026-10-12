#pragma once

#include "Models.h"
#include <stdexcept>
#include <string>

namespace kc {

// Thrown for a non-2xx HTTP response. Carries the status code and whatever
// message the API put in its {"detail": "..."} body, falling back to a
// generic message when the body isn't JSON (e.g. a proxy error page).
class ApiError : public std::runtime_error {
public:
    ApiError(int status, const std::string& message)
        : std::runtime_error(message), status_code(status) {}

    int status_code;
};

// Thrown when the request never reached the server (DNS, connection refused,
// timeout) — distinct from ApiError so the UI can tell "server said no" from
// "couldn't reach the server" apart.
class NetworkError : public std::runtime_error {
public:
    explicit NetworkError(const std::string& message) : std::runtime_error(message) {}
};

// Talks to a Knowledge Copilot instance over HTTP. Every call is blocking —
// callers running this from a GUI thread should do so on a worker thread.
class Client {
public:
    Client(std::string base_url, std::string api_key = "");

    void set_base_url(std::string url);
    void set_api_key(std::string key);
    const std::string& base_url() const { return base_url_; }

    HealthResponse health();
    AskResponse ask(const std::string& question, int top_k = 0, int prompt_version = 0);
    DocumentsResponse list_documents();
    IngestResponse ingest_file(const std::string& file_path);
    void delete_document(const std::string& source);

private:
    std::string base_url_;
    std::string api_key_;

    struct HttpResponse {
        long status = 0;
        std::string body;
    };

    HttpResponse http_get(const std::string& path);
    HttpResponse http_post_json(const std::string& path, const std::string& json_body);
    HttpResponse http_post_multipart_file(const std::string& path, const std::string& file_path);
    HttpResponse http_delete(const std::string& path);
    void raise_for_status(const HttpResponse& resp) const;
};

// Pure JSON -> struct parsing, exposed separately from Client so the request
// *shape* can be unit tested without a live server or an HTTP mock.
HealthResponse parse_health(const std::string& json_text);
AskResponse parse_ask(const std::string& json_text);
AgentResponse parse_agent(const std::string& json_text);
DocumentsResponse parse_documents(const std::string& json_text);
IngestResponse parse_ingest(const std::string& json_text);

// Pulls {"detail": "..."} out of an error body, or a generic fallback.
std::string parse_error_detail(const std::string& json_text);

} // namespace kc
