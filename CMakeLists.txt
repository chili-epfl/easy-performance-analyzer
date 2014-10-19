cmake_minimum_required(VERSION 2.8)
project(easy-profiler)

#Options
option(WITH_SAMPLES "Build samples" ON)

#Print options
message(STATUS "")
message(STATUS "Options:")
message(STATUS "    WITH_SAMPLES:   " ${WITH_SAMPLES})
message(STATUS "")

#Main lib
add_library(easyprofiler STATIC src/EasyProfiler.cpp)
set_target_properties(easyprofiler PROPERTIES COMPILE_FLAGS "-O3")
if(DEFINED ANDROID)
    target_link_libraries(easyprofiler log)
endif()
install(TARGETS easyprofiler ARCHIVE DESTINATION lib)
install(FILES src/EasyProfiler.hpp DESTINATION include)

#Samples
if(WITH_SAMPLES)
    add_subdirectory(samples)
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/samples/bin/ CACHE PATH "Output directory for sample binaries")
endif()

