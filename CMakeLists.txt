cmake_minimum_required(VERSION 3.18)
project(sloth2)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra") #FIXME use proper cmake find
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_subdirectory(third_party/fftw-3.3.10)
add_subdirectory(third_party/SDL)
set(SDL_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/third_party/SDL/include)
set(SDL_TEST OFF)

add_library(wavelet2d ${CMAKE_CURRENT_LIST_DIR}/third_party/wavelet2d/wavelet2d.cpp)
target_include_directories(wavelet2d PUBLIC ${CMAKE_CURRENT_LIST_DIR}/third_party/fftw-3.3.10/api)
target_link_libraries(wavelet2d PUBLIC fftw3)

add_library(fft util/fft_handler.cpp util/dwt_handler.cpp)
target_include_directories(fft PUBLIC ${CMAKE_CURRENT_LIST_DIR}/third_party/fftw-3.3.10/api)
target_include_directories(fft PUBLIC ${CMAKE_CURRENT_LIST_DIR}/third_party/wavelet2d)
target_link_libraries(fft PUBLIC wavelet2d)

add_library(util util/ring_buffer.tcc util/math.tcc util/rolling_window.tcc)
add_library(sdl sdl/sdl_audio.tcc)
set_target_properties(util PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(sdl PROPERTIES LINKER_LANGUAGE CXX)

add_executable(sloth2 main.cpp)

target_link_libraries(sdl SDL2-static)
target_link_libraries(sloth2 util sdl fft)
