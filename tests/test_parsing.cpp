#include "../src/api/Client.h"
#include <catch2/catch_test_macros.hpp>

using namespace kc;

TEST_CASE("parse_health reads provider info and chunk count", "[parsing]") {
    std::string json = R"({
        "status": "ok",
        "llm": {"provider": "openai", "model": "gpt-4o-mini"},
        "embeddings": {"provider": "openai", "model": "text-embedding-3-small", "dim": 1536},
        "vector_store": {"url": "http://qdrant:6333", "chunks": 42},
        "prompts": ["rag_answer", "agent_system"],
        "auth_required": true,
        "agent_actions": false
    })";

    HealthResponse h = parse_health(json);

    REQUIRE(h.status == "ok");
    REQUIRE(h.llm_provider == "openai");
    REQUIRE(h.llm_model == "gpt-4o-mini");
    REQUIRE(h.vector_store_chunks == 42);
    REQUIRE(h.auth_required == true);
}

TEST_CASE("parse_ask reads the answer, citations and usage", "[parsing]") {
    std::string json = R"({
        "request_id": "abc123",
        "answer": "The onboarding doc covers steps 1-5.",
        "citations": [
            {"n": 1, "source": "onboarding.md", "heading": "Steps", "score": 0.87, "text": "Step 1: ..."}
        ],
        "grounded": true,
        "prompt_version": "v3",
        "model": "gpt-4o-mini",
        "usage": {"prompt_tokens": 120, "completion_tokens": 40},
        "retrieved": ["onboarding.md", "faq.md"]
    })";

    AskResponse a = parse_ask(json);

    REQUIRE(a.request_id == "abc123");
    REQUIRE(a.grounded == true);
    REQUIRE(a.citations.size() == 1);
    REQUIRE(a.citations[0].n == 1);
    REQUIRE(a.citations[0].source == "onboarding.md");
    REQUIRE(a.citations[0].score == 0.87);
    REQUIRE(a.usage.prompt_tokens == 120);
    REQUIRE(a.usage.completion_tokens == 40);
    REQUIRE(a.retrieved.size() == 2);
}

TEST_CASE("parse_ask handles an ungrounded, no-citation answer", "[parsing]") {
    std::string json = R"({
        "request_id": "xyz",
        "answer": "I don't have information about that.",
        "citations": [],
        "grounded": false,
        "prompt_version": "v3",
        "model": "gpt-4o-mini",
        "usage": {"prompt_tokens": 50, "completion_tokens": 10},
        "retrieved": []
    })";

    AskResponse a = parse_ask(json);

    REQUIRE(a.grounded == false);
    REQUIRE(a.citations.empty());
    REQUIRE(a.retrieved.empty());
}

TEST_CASE("parse_agent reads the tool-call trace", "[parsing]") {
    std::string json = R"({
        "request_id": "agent-1",
        "answer": "Done.",
        "trace": [
            {"step": 1, "tool": "search_docs", "arguments": {"query": "refund policy"}, "result": "found 3", "error": false},
            {"step": 2, "tool": "create_ticket", "arguments": {"title": "Follow up"}, "result": "blocked", "error": true}
        ],
        "steps": 2,
        "prompt_version": "v1",
        "model": "gpt-4o-mini",
        "usage": {"prompt_tokens": 200, "completion_tokens": 80},
        "stopped_by_budget": false
    })";

    AgentResponse a = parse_agent(json);

    REQUIRE(a.steps == 2);
    REQUIRE(a.trace.size() == 2);
    REQUIRE(a.trace[0].tool == "search_docs");
    REQUIRE(a.trace[0].error == false);
    REQUIRE(a.trace[1].tool == "create_ticket");
    REQUIRE(a.trace[1].error == true);
    REQUIRE(a.stopped_by_budget == false);
}

TEST_CASE("parse_documents reads sources and chunk count", "[parsing]") {
    std::string json = R"({"sources": ["a.pdf", "b.md"], "chunks": 17})";

    DocumentsResponse d = parse_documents(json);

    REQUIRE(d.sources.size() == 2);
    REQUIRE(d.sources[0] == "a.pdf");
    REQUIRE(d.chunks == 17);
}

TEST_CASE("parse_documents handles an empty library", "[parsing]") {
    std::string json = R"({"sources": [], "chunks": 0})";

    DocumentsResponse d = parse_documents(json);

    REQUIRE(d.sources.empty());
    REQUIRE(d.chunks == 0);
}

TEST_CASE("parse_ingest reads the upload result", "[parsing]") {
    std::string json = R"({"source": "notes.txt", "chunks": 5, "total_chunks": 22})";

    IngestResponse r = parse_ingest(json);

    REQUIRE(r.source == "notes.txt");
    REQUIRE(r.chunks == 5);
    REQUIRE(r.total_chunks == 22);
}
