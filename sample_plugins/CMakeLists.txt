cmake_minimum_required(VERSION 3.24)

# すべての *.cpp を列挙
file(GLOB PLUGIN_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(src ${PLUGIN_SOURCES})
    get_filename_component(fname ${src} NAME_WE)      # "twice_plugin"
    add_library(${fname} SHARED ${src})

    target_include_directories(${fname} PRIVATE
       ${PROJECT_SOURCE_DIR}/include)

    target_compile_features(${fname} PRIVATE cxx_std_20)
    if(MSVC)
        target_compile_options(${fname} PRIVATE /utf-8)
        set_target_properties(${fname} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS YES)
    endif()

    # OS 共通で "fname.[dll|so]" に揃える
    set_target_properties(${fname} PROPERTIES PREFIX "" OUTPUT_NAME ${fname})
endforeach()



# # plugins/twice/CMakeLists.txt
# cmake_minimum_required(VERSION 3.24)
# project(twice_plugin LANGUAGES C CXX)

# # ───── 共有ライブラリ生成 ────────────────
# add_library(twice_plugin SHARED twice_plugin.cpp)

# # Windows で自動エクスポート（ EXPORT マクロでも動くが保険）
# if(MSVC)
#   set_target_properties(twice_plugin PROPERTIES
#     WINDOWS_EXPORT_ALL_SYMBOLS YES)
#   target_compile_options(twice_plugin PRIVATE /utf-8)
# endif()

# # インクルードパス（サブモジュールのコアヘッダ）
# target_include_directories(twice_plugin
#   PRIVATE
#     ${CMAKE_SOURCE_DIR}/include)      # prismcascade/api/plugin_abi.hpp など

# # 例：プラグイン自身は core ライブラリとリンクしない
# # target_link_libraries(twice_plugin PRIVATE prismcascade::core)

# # 出力名を OS 共通に
# set_target_properties(twice_plugin PROPERTIES
#   OUTPUT_NAME "twice_plugin"
#   PREFIX "")                   # "lib" プレフィクスを外す (Linuxでも)
