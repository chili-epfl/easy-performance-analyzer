cmake_minimum_required(VERSION 2.8)
project(easy-performance-analyzer)

#Options
option(WITH_SAMPLES "Build samples" ON)

#Print options
message(STATUS "")
message(STATUS "Options:")
message(STATUS "    WITH_SAMPLES:   " ${WITH_SAMPLES})
message(STATUS "")

#Main lib
add_library(ezp STATIC src/ezp.cpp)
set_target_properties(ezp PROPERTIES COMPILE_FLAGS "-O3 -fPIC")
if(DEFINED ANDROID)
    target_link_libraries(ezp log)
else()
    target_link_libraries(ezp pthread)
endif()
install(TARGETS ezp ARCHIVE DESTINATION lib)
install(FILES src/ezp.hpp DESTINATION include)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/ CACHE PATH "Output directory for binaries")

#Samples
if(WITH_SAMPLES)
    add_subdirectory(samples)
endif()

