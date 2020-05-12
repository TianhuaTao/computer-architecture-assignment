cmake_minimum_required(VERSION 3.15)
project(Tomasulo)

set(CMAKE_CXX_STANDARD 11)

add_executable(Tomasulo main.cpp Instruction.cpp Instruction.h Tomasulo.cpp Tomasulo.h Logger.cpp Logger.h)