cmake_minimum_required( VERSION 3.0 )

project( remoter )

add_definitions( -g -ggdb )

include_directories( ./serialport )
add_subdirectory( ./serialport )

add_executable( remoter main.cpp keymap.cpp keymap.hpp )

target_link_libraries( remoter serialport )
