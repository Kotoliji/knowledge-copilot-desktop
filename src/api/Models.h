#pragma once

#include <string>
#include <vector>

namespace kc {

struct Usage {
    int prompt_tokens = 0;
    int completion_tokens = 0;
};

struct Citation {
    int n = 0;
    std::string source;
    std::string heading;
    double score = 0.0;
    std::string text;
};

struct AskResponse {
    std::string request_id;
    std::string answer;
    std::vector<Citation> citations;
    bool grounded = false;
    std::string prompt_version;
    std::string model;
    Usage usage;
    std::vector<std::string> retrieved;
};

struct TraceStep {
    int step = 0;
    std::string tool;
    std::string arguments_json; // kept as raw JSON text — arguments vary per tool
    std::string result;
    bool error = false;
};

struct AgentResponse {
    std::string request_id;
    std::string answer;
    std::vector<TraceStep> trace;
    int steps = 0;
    std::string prompt_version;
    std::string model;
    Usage usage;
    bool stopped_by_budget = false;
};

struct DocumentsResponse {
    std::vector<std::string> sources;
    int chunks = 0;
};

struct IngestResponse {
    std::string source;
    int chunks = 0;
    int total_chunks = 0;
};

struct HealthResponse {
    std::string status;
    std::string llm_provider;
    std::string llm_model;
    std::string embedding_provider;
    std::string embedding_model;
    int vector_store_chunks = 0;
    bool auth_required = false;
};

} // namespace kc
