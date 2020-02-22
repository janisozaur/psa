cmake_minimum_required(VERSION 3.9)
project(psa_test CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TARGET_M "-m32")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TARGET_M}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${TARGET_M}")

find_package(ZLIB REQUIRED)

add_library(psa "psa_openrct2.cpp" "MemoryStream.cpp" "main.cpp")
target_link_libraries(psa ZLIB::ZLIB)
