project(most)
cmake_minimum_required(VERSION 2.8)

add_compile_options(-O3 -Wall -g)
add_compile_options(-mavx -msse4 -msse4.1 -msse4.2 -msse -msse2 -msse3)
add_compile_options(-funroll-loops -fno-stack-protector -fwhole-program)
add_compile_options(-fcse-skip-blocks -funsafe-loop-optimizations)
set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++ -static-libgcc")

add_executable(download src/download.cpp)

add_executable(ultimate src/ultimate.cpp)
target_link_libraries(ultimate pthread)

add_executable(ultimate_single src/ultimate_single.cpp)
target_link_libraries(ultimate_single pthread)

add_executable(opt1 src/opt1.cpp)
target_link_libraries(opt1 pthread)
add_executable(opt12 src/opt12.cpp)
target_link_libraries(opt12 pthread)
add_executable(opt2 src/opt2.cpp)
target_link_libraries(opt2 pthread)
add_executable(opt3 src/opt3.cpp)
target_link_libraries(opt3 pthread)
add_executable(opt32 src/opt32.cpp)
target_link_libraries(opt32 pthread)
add_executable(opt4 src/opt4.cpp)
target_link_libraries(opt4 pthread)
add_executable(opt42 src/opt42.cpp)
target_link_libraries(opt42 pthread)

add_executable(test2 src/test2.cpp)
