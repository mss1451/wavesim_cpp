cmake_minimum_required(VERSION 3.10)

project(wavesim_cpp)

find_package(PkgConfig)

pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)

add_executable(
        wavesim_cpp
        source/main.cpp
        source/project_handler.cpp
        source/wave_engine.cpp
        source/INIReader.cpp
        source/ini.c
        source/zutil.c
        source/window_handler.cpp
)

target_link_libraries(wavesim_cpp ${GTKMM_LIBRARIES})
target_include_directories(wavesim_cpp PUBLIC ${GTKMM_INCLUDE_DIRS})
target_compile_options(wavesim_cpp PUBLIC ${GTKMM_CFLAGS_OTHER})