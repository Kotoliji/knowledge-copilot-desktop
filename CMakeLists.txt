cmake_minimum_required(VERSION 3.22)
project(knowledge_copilot_desktop CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -Wextra)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

# ---- kc_api: HTTP client + JSON (un)marshalling, no GTK dependency --------
# Kept separate so the request/response logic can be unit tested without
# pulling in GTK, and so the test binary builds fast.
add_library(kc_api STATIC
    src/api/Client.cpp
)
target_include_directories(kc_api PUBLIC src)
target_link_libraries(kc_api PUBLIC CURL::libcurl nlohmann_json::nlohmann_json)

# ---- kc_desktop: the GTK application ---------------------------------------
add_executable(kc_desktop
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/AskView.cpp
    src/ui/DocumentsView.cpp
)
target_link_libraries(kc_desktop PRIVATE kc_api PkgConfig::GTKMM)

# ---- Tests ------------------------------------------------------------------
enable_testing()
find_package(Catch2 3 REQUIRED)

add_executable(kc_tests
    tests/test_parsing.cpp
    tests/test_error_detail.cpp
)
target_link_libraries(kc_tests PRIVATE kc_api Catch2::Catch2WithMain)

include(CTest)
add_test(NAME kc_tests COMMAND kc_tests)
