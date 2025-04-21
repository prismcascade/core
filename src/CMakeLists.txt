# ---------- 収集 ----------
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/prism/*.hpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp")

# ---------- ライブラリ ----------
add_library(prismcascade STATIC ${LIB_SOURCES})
add_library(prismcascade::core ALIAS prismcascade)

target_include_directories(prismcascade
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/prism>
    $<INSTALL_INTERFACE:include/prism>)

target_compile_features(prismcascade PUBLIC cxx_std_20)

# MSVC で UTF‑8 ソースを明示
target_compile_options(prismcascade PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
