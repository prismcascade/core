cmake_minimum_required(VERSION 3.21)

# ──────────────────────────────────────────────
# プロジェクト設定
# ──────────────────────────────────────────────
if(NOT CMAKE_CXX_COMPILER)
  find_program(_CLANG clang++)
  if(_CLANG)
    set(CMAKE_CXX_COMPILER "${_CLANG}" CACHE STRING "" FORCE)
  endif()
endif()

project(PrismCascade VERSION 4.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ──────────────────────────────────────────────
# オプション：テスト / サンプル
# ──────────────────────────────────────────────
option(PRISM_BUILD_TESTS   "Build unit/property tests" ON)
option(PRISM_BUILD_SAMPLES "Build CLI & sample plugins" ON)

if(NOT PROJECT_IS_TOP_LEVEL)
  set(PRISM_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
  set(PRISM_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
endif()

# ──────────────────────────────────────────────
# サブディレクトリ
# ──────────────────────────────────────────────
add_subdirectory(src)

if(PRISM_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(PRISM_BUILD_SAMPLES)
  add_subdirectory(cli)
  add_subdirectory(sample_plugins)
endif()

# ──────────────────────────────────────────────
# install / export
# ──────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS prismcascade
        EXPORT  PrismCascadeTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY include/prismcascade
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT  PrismCascadeTargets
        FILE     prismcascade-config.cmake
        NAMESPACE prismcascade::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/prismcascade)
