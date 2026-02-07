cmake_minimum_required(VERSION 3.10)
project(spamfile LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

add_executable(spamfile src/main.cpp)

target_link_options(spamfile PRIVATE -mwindows -static)