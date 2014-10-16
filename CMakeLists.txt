cmake_minimum_required(VERSION 2.8)
project(easy-profiler)

add_definitions("-O3")

add_library(easyprofiler STATIC src/EasyProfiler.cpp)

if(DEFINED ANDROID)
    target_link_libraries(easyprofiler log)
endif()

install(TARGETS easyprofiler ARCHIVE DESTINATION lib)
install(FILES src/EasyProfiler.hpp DESTINATION include)

