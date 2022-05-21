project(sloth2)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -lpulse") #FIXME use proper cmake find
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_library(utils utils.cc)
add_library(pulse_backend pulse_backend.cc)

add_executable(test test.cc)

target_link_libraries(pulse_backend utils)
target_link_libraries(test pulse_backend)