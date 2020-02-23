cmake_minimum_required(VERSION 3.9)
project(psa_test CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_32BIT "Build in 32-bit mode")
if (BUILD_32BIT)
    set(TARGET_M "-m32")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TARGET_M}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${TARGET_M}")

add_executable(psa "psa_openrct2.cpp" "MemoryStream.cpp" "IStream.cpp" "Guard.cpp" "String.cpp" "Console.cpp" "UTF8.cpp" "main.cpp")

# Don't bother doing find_package(), it probably won't give correct results for 32-bit anyway
target_link_libraries(psa z icuuc icudata benchmark)
