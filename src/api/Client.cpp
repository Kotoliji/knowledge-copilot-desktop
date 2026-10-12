#include "Client.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <cstdio>
#include <stdexcept>

using json = nlohmann::json;

namespace kc {

namespace {

size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

// RAII wrapper so an exception thrown mid-request doesn't leak the handle.
struct CurlHandle {
    CURL* handle;
    CurlHandle() : handle(curl_easy_init()) {
        if (!handle) throw NetworkError("curl_easy_init failed");
    }
    ~CurlHandle() { curl_easy_cleanup(handle); }
};

} // namespace

Client::Client(std::string base_url, std::string api_key)
    : base_url_(std::move(base_url)), api_key_(std::move(api_key)) {}

void Client::set_base_url(std::string url) { base_url_ = std::move(url); }
void Client::set_api_key(std::string key) { api_key_ = std::move(key); }

void Client::raise_for_status(const HttpResponse& resp) const {
    if (resp.status >= 200 && resp.status < 300) return;
    throw ApiError(static_cast<int>(resp.status), parse_error_detail(resp.body));
}

Client::HttpResponse Client::http_get(const std::string& path) {
    CurlHandle c;
    std::string url = base_url_ + path;
    std::string body;

    struct curl_slist* headers = nullptr;
    if (!api_key_.empty()) {
        headers = curl_slist_append(headers, ("X-API-Key: " + api_key_).c_str());
    }

    curl_easy_setopt(c.handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c.handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(c.handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(c.handle, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(c.handle, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(c.handle);
    curl_slist_free_all(headers);
    if (res != CURLE_OK) throw NetworkError(curl_easy_strerror(res));

    long status = 0;
    curl_easy_getinfo(c.handle, CURLINFO_RESPONSE_CODE, &status);
    return {status, body};
}

Client::HttpResponse Client::http_post_json(const std::string& path, const std::string& json_body) {
    CurlHandle c;
    std::string url = base_url_ + path;
    std::string body;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!api_key_.empty()) {
        headers = curl_slist_append(headers, ("X-API-Key: " + api_key_).c_str());
    }

    curl_easy_setopt(c.handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c.handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(c.handle, CURLOPT_POSTFIELDS, json_body.c_str());
    curl_easy_setopt(c.handle, CURLOPT_POSTFIELDSIZE, static_cast<long>(json_body.size()));
    curl_easy_setopt(c.handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(c.handle, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(c.handle, CURLOPT_TIMEOUT, 60L);

    CURLcode res = curl_easy_perform(c.handle);
    curl_slist_free_all(headers);
    if (res != CURLE_OK) throw NetworkError(curl_easy_strerror(res));

    long status = 0;
    curl_easy_getinfo(c.handle, CURLINFO_RESPONSE_CODE, &status);
    return {status, body};
}

Client::HttpResponse Client::http_post_multipart_file(const std::string& path, const std::string& file_path) {
    CurlHandle c;
    std::string url = base_url_ + path;
    std::string body;

    curl_mime* mime = curl_mime_init(c.handle);
    curl_mimepart* part = curl_mime_addpart(mime);
    curl_mime_name(part, "file");
    CURLcode file_res = curl_mime_filedata(part, file_path.c_str());
    if (file_res != CURLE_OK) {
        curl_mime_free(mime);
        throw NetworkError("could not read file: " + file_path);
    }

    struct curl_slist* headers = nullptr;
    if (!api_key_.empty()) {
        headers = curl_slist_append(headers, ("X-API-Key: " + api_key_).c_str());
    }

    curl_easy_setopt(c.handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c.handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(c.handle, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(c.handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(c.handle, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(c.handle, CURLOPT_TIMEOUT, 120L);

    CURLcode res = curl_easy_perform(c.handle);
    curl_slist_free_all(headers);
    curl_mime_free(mime);
    if (res != CURLE_OK) throw NetworkError(curl_easy_strerror(res));

    long status = 0;
    curl_easy_getinfo(c.handle, CURLINFO_RESPONSE_CODE, &status);
    return {status, body};
}

Client::HttpResponse Client::http_delete(const std::string& path) {
    CurlHandle c;
    std::string url = base_url_ + path;
    std::string body;

    struct curl_slist* headers = nullptr;
    if (!api_key_.empty()) {
        headers = curl_slist_append(headers, ("X-API-Key: " + api_key_).c_str());
    }

    curl_easy_setopt(c.handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c.handle, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(c.handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(c.handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(c.handle, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(c.handle, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(c.handle);
    curl_slist_free_all(headers);
    if (res != CURLE_OK) throw NetworkError(curl_easy_strerror(res));

    long status = 0;
    curl_easy_getinfo(c.handle, CURLINFO_RESPONSE_CODE, &status);
    return {status, body};
}

HealthResponse Client::health() {
    auto resp = http_get("/health");
    raise_for_status(resp);
    return parse_health(resp.body);
}

AskResponse Client::ask(const std::string& question, int top_k, int prompt_version) {
    json body{{"question", question}};
    if (top_k > 0) body["top_k"] = top_k;
    if (prompt_version > 0) body["prompt_version"] = prompt_version;

    auto resp = http_post_json("/ask", body.dump());
    raise_for_status(resp);
    return parse_ask(resp.body);
}

DocumentsResponse Client::list_documents() {
    auto resp = http_get("/documents");
    raise_for_status(resp);
    return parse_documents(resp.body);
}

IngestResponse Client::ingest_file(const std::string& file_path) {
    auto resp = http_post_multipart_file("/ingest", file_path);
    raise_for_status(resp);
    return parse_ingest(resp.body);
}

void Client::delete_document(const std::string& source) {
    auto resp = http_delete("/documents/" + source);
    raise_for_status(resp);
}

// ---- Parsing (no network — unit tested directly against sample JSON) -------

std::string parse_error_detail(const std::string& json_text) {
    try {
        json j = json::parse(json_text);
        if (!j.contains("detail")) return "request failed";
        const auto& detail = j["detail"];
        if (detail.is_string()) return detail.get<std::string>();
        if (detail.is_array()) {
            // FastAPI/Pydantic validation errors: a list of {"msg": "...", ...}.
            std::string combined;
            for (const auto& item : detail) {
                if (!combined.empty()) combined += "; ";
                combined += item.value("msg", item.dump());
            }
            return combined.empty() ? "validation failed" : combined;
        }
        return detail.dump();
    } catch (const json::exception&) {
        return json_text.empty() ? "request failed" : json_text;
    }
}

HealthResponse parse_health(const std::string& json_text) {
    json j = json::parse(json_text);
    HealthResponse h;
    h.status = j.value("status", "");
    if (j.contains("llm")) {
        h.llm_provider = j["llm"].value("provider", "");
        h.llm_model = j["llm"].value("model", "");
    }
    if (j.contains("embeddings")) {
        h.embedding_provider = j["embeddings"].value("provider", "");
        h.embedding_model = j["embeddings"].value("model", "");
    }
    if (j.contains("vector_store")) {
        h.vector_store_chunks = j["vector_store"].value("chunks", 0);
    }
    h.auth_required = j.value("auth_required", false);
    return h;
}

static Usage parse_usage(const json& j) {
    Usage u;
    if (j.contains("usage")) {
        u.prompt_tokens = j["usage"].value("prompt_tokens", 0);
        u.completion_tokens = j["usage"].value("completion_tokens", 0);
    }
    return u;
}

AskResponse parse_ask(const std::string& json_text) {
    json j = json::parse(json_text);
    AskResponse a;
    a.request_id = j.value("request_id", "");
    a.answer = j.value("answer", "");
    a.grounded = j.value("grounded", false);
    a.prompt_version = j.value("prompt_version", "");
    a.model = j.value("model", "");
    a.usage = parse_usage(j);

    for (const auto& c : j.value("citations", json::array())) {
        Citation citation;
        citation.n = c.value("n", 0);
        citation.source = c.value("source", "");
        citation.heading = c.value("heading", "");
        citation.score = c.value("score", 0.0);
        citation.text = c.value("text", "");
        a.citations.push_back(std::move(citation));
    }
    for (const auto& r : j.value("retrieved", json::array())) {
        a.retrieved.push_back(r.get<std::string>());
    }
    return a;
}

AgentResponse parse_agent(const std::string& json_text) {
    json j = json::parse(json_text);
    AgentResponse a;
    a.request_id = j.value("request_id", "");
    a.answer = j.value("answer", "");
    a.steps = j.value("steps", 0);
    a.prompt_version = j.value("prompt_version", "");
    a.model = j.value("model", "");
    a.usage = parse_usage(j);
    a.stopped_by_budget = j.value("stopped_by_budget", false);

    for (const auto& t : j.value("trace", json::array())) {
        TraceStep step;
        step.step = t.value("step", 0);
        step.tool = t.value("tool", "");
        step.result = t.value("result", "");
        step.error = t.value("error", false);
        if (t.contains("arguments")) step.arguments_json = t["arguments"].dump();
        a.trace.push_back(std::move(step));
    }
    return a;
}

DocumentsResponse parse_documents(const std::string& json_text) {
    json j = json::parse(json_text);
    DocumentsResponse d;
    d.chunks = j.value("chunks", 0);
    for (const auto& s : j.value("sources", json::array())) {
        d.sources.push_back(s.get<std::string>());
    }
    return d;
}

IngestResponse parse_ingest(const std::string& json_text) {
    json j = json::parse(json_text);
    IngestResponse r;
    r.source = j.value("source", "");
    r.chunks = j.value("chunks", 0);
    r.total_chunks = j.value("total_chunks", 0);
    return r;
}

} // namespace kc
