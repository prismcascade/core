cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0135 NEW)

include(FetchContent)

# ── GoogleTest ──────────────────────────────────────────────────────────────
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
  GIT_SHALLOW    TRUE
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# ── RapidCheck ─────────────────────────────────────────────────────────────
FetchContent_Declare(
  rapidcheck
  GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
  GIT_TAG        master                    # または特定コミット
  GIT_SHALLOW    TRUE
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(googletest rapidcheck)

# ① RapidCheck の gtest ラッパーを CMake に追加 ―――――――――――――――――――
#    extras/gtest に CMakeLists.txt があるのでサブディレクトリとして取り込む。
add_subdirectory(
  ${rapidcheck_SOURCE_DIR}/extras/gtest
  ${CMAKE_BINARY_DIR}/rapidcheck_gtest-build
)

# ② ここで target rapidcheck_gtest が定義される
# ---------------------------------------------------------------------------

# テスト・ソース収集
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(prism_tests ${TEST_SOURCES})

target_include_directories(prism_tests PRIVATE
    ${rapidcheck_SOURCE_DIR}/extras/gtest/include)

target_link_libraries(prism_tests
  PRIVATE
    prismcascade::core    # 本体
    Boost::boost
    gtest_main            # GoogleTest runner
    rapidcheck_gtest      # RapidCheck + gtest bridge（内部で rapidcheck をリンク）
)

target_compile_options(prism_tests PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

enable_testing()
add_test(NAME all COMMAND prism_tests)
