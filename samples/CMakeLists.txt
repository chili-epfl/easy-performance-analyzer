include_directories(../src)

add_executable(real-time src/real-time.cpp)
set_target_properties(real-time PROPERTIES
    COMPILE_FLAGS "-O3"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(offline src/offline.cpp)
set_target_properties(offline PROPERTIES
    COMPILE_FLAGS "-O3"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(compiler-optimization-O0 src/compiler-optimization.cpp)
set_target_properties(compiler-optimization-O0 PROPERTIES
    COMPILE_FLAGS "-O0"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(compiler-optimization-O3 src/compiler-optimization.cpp)
set_target_properties(compiler-optimization-O3 PROPERTIES
    COMPILE_FLAGS "-O3"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(multithreaded src/multithreaded.cpp)
set_target_properties(multithreaded PROPERTIES
    COMPILE_FLAGS "-O3"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(instrumentation-performance-real-time src/instrumentation-performance.cpp)
set_target_properties(instrumentation-performance-real-time PROPERTIES
    COMPILE_FLAGS "-O3 -DEZP_SAMPLE_REALTIME"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(instrumentation-performance-smoothed src/instrumentation-performance.cpp)
set_target_properties(instrumentation-performance-smoothed PROPERTIES
    COMPILE_FLAGS "-O3 -DEZP_SAMPLE_SMOOTHED"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(instrumentation-performance-offline src/instrumentation-performance.cpp)
set_target_properties(instrumentation-performance-offline PROPERTIES
    COMPILE_FLAGS "-O3"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(external-control src/external-control.cpp)
set_target_properties(external-control PROPERTIES
    COMPILE_FLAGS "-O3"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

target_link_libraries(real-time                                 ezp)
target_link_libraries(offline                                   ezp)
target_link_libraries(compiler-optimization-O0                  ezp)
target_link_libraries(compiler-optimization-O3                  ezp)
target_link_libraries(multithreaded                             ezp)
target_link_libraries(instrumentation-performance-real-time     ezp)
target_link_libraries(instrumentation-performance-smoothed      ezp)
target_link_libraries(instrumentation-performance-offline       ezp)
target_link_libraries(external-control                          ezp)
if(NOT DEFINED ANDROID)
    target_link_libraries(multithreaded                         pthread)
endif()

