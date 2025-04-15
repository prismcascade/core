cmake_minimum_required(VERSION 3.8)
project(PrismCascadeCore LANGUAGES CXX)

# cpp ファイルを再帰的に得る
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS *.cpp)

# 静的ライブラリ （必要なら SHARED に変更）
add_library(prismcascade STATIC ${LIB_SOURCES})

# public な include directory に設定
target_include_directories(prismcascade PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
