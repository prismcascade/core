cmake_minimum_required(VERSION 3.24)

# すべての *.cpp を列挙
file(GLOB PLUGIN_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(src ${PLUGIN_SOURCES})
    get_filename_component(fname ${src} NAME_WE)      # e.g. "twice_plugin"
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

