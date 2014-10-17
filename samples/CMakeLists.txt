include_directories(../src)

add_executable(real-time src/real-time.cpp)
target_link_libraries(real-time easyprofiler)
set_target_properties(real-time PROPERTIES COMPILE_FLAGS "-O3")

add_executable(compiler-optimization-O0 src/compiler-optimization.cpp)
target_link_libraries(compiler-optimization-O0 easyprofiler)
set_target_properties(compiler-optimization-O0 PROPERTIES COMPILE_FLAGS "-O0")

add_executable(compiler-optimization-O3 src/compiler-optimization.cpp)
target_link_libraries(compiler-optimization-O3 easyprofiler)
set_target_properties(compiler-optimization-O3 PROPERTIES COMPILE_FLAGS "-O3")

