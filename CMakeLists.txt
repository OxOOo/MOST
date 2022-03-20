project(most)
cmake_minimum_required(VERSION 2.8)

add_compile_options(-O3 -Wall -g)
add_compile_options(-mavx -msse4 -msse4.1 -msse4.2 -msse -msse2 -msse3)
add_compile_options(-funroll-loops -fno-stack-protector -fwhole-program)
add_compile_options(-fcse-skip-blocks -funsafe-loop-optimizations)
set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++ -static-libgcc")

add_executable(main src/main.cpp)
target_link_libraries(main pthread)
