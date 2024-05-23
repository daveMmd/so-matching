cmake_minimum_required(VERSION 3.16)
project(so-matching)

set(CMAKE_CXX_STANDARD 11)

add_executable(so-matching main.cpp soengine.cpp soengine.h util.cpp util.h pigasus_hash_bitmap.cpp)