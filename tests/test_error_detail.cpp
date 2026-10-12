#include "../src/api/Client.h"
#include <catch2/catch_test_macros.hpp>

using namespace kc;

TEST_CASE("parse_error_detail reads a plain string detail", "[error]") {
    std::string json = R"({"detail": "Invalid or missing X-API-Key", "request_id": "r1"})";

    REQUIRE(parse_error_detail(json) == "Invalid or missing X-API-Key");
}

TEST_CASE("parse_error_detail joins a Pydantic validation error list", "[error]") {
    // FastAPI/Pydantic 422 errors return `detail` as a list of objects, not a string.
    std::string json = R"({
        "detail": [
            {"loc": ["body", "question"], "msg": "field required", "type": "value_error.missing"},
            {"loc": ["body", "top_k"], "msg": "ensure this value is less than or equal to 10", "type": "value_error"}
        ]
    })";

    std::string detail = parse_error_detail(json);

    REQUIRE(detail.find("field required") != std::string::npos);
    REQUIRE(detail.find("ensure this value is less than or equal to 10") != std::string::npos);
}

TEST_CASE("parse_error_detail falls back gracefully on non-JSON bodies", "[error]") {
    std::string html_error_page = "<html><body>502 Bad Gateway</body></html>";

    REQUIRE(parse_error_detail(html_error_page) == html_error_page);
}

TEST_CASE("parse_error_detail falls back on an empty body", "[error]") {
    REQUIRE(parse_error_detail("") == "request failed");
}

TEST_CASE("parse_error_detail falls back when there is no detail field", "[error]") {
    REQUIRE(parse_error_detail(R"({"status": "error"})") == "request failed");
}
