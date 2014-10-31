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
install(FILES src/ezp.hpp DESTINATION include)
install(TARGETS ezp ARCHIVE DESTINATION lib)

#Controller binary
add_executable(ezp_control src/ezp_control.cpp)
set_target_properties(ezp_control PROPERTIES COMPILE_FLAGS "-O3")
if(DEFINED ANDROID)
    target_link_libraries(ezp_control ezp log)
else()
    target_link_libraries(ezp_control ezp pthread)
endif()
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin/ CACHE PATH "Output directory for non-sample binaries")
if(NOT DEFINED ANDROID)
    install(TARGETS ezp_control RUNTIME DESTINATION bin)
endif()

#Samples
if(WITH_SAMPLES)
    add_subdirectory(samples)
endif()

