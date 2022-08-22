cmake_minimum_required(VERSION 3.18)
project(sloth2)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -L/usr/include/SDL2 -lfftw3 -lm") #FIXME use proper cmake find
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(SDL2 REQUIRED)

add_library(util util/ring_buffer.tcc util/fft_handler.cpp util/math.tcc util/rolling_window.tcc)
add_library(sdl sdl/sdl_audio.tcc)
set_target_properties(util PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(sdl PROPERTIES LINKER_LANGUAGE CXX)

add_executable(sloth2 main.cpp)

target_link_libraries(sdl SDL2::SDL2main SDL2::SDL2)
target_link_libraries(sloth2 util sdl)
