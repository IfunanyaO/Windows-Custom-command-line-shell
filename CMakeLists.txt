cmake_minimum_required(VERSION 3.10)
project(MyShell C)

set(CMAKE_C_STANDARD 99)

# Add source files (since you only have library.c, you will need to use it here)
add_executable(MyShell library.c)
