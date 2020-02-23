cmake_minimum_required(VERSION 3.9)
project(psa_test CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_32BIT "Build in 32-bit mode")
if (BUILD_32BIT)
    set(TARGET_M "-m32")
    set(OPENRCT2_EXE "${CMAKE_SOURCE_DIR}/openrct2.exe")
    add_custom_command(
        OUTPUT openrct2_text
        COMMAND dd if="${OPENRCT2_EXE}" of="${CMAKE_BINARY_DIR}/openrct2_text" bs=4096 skip=1 count=1187
        DEPENDS ${OPENRCT2_EXE}
    )
    add_custom_command(
        OUTPUT openrct2_data
        COMMAND dd if="${OPENRCT2_EXE}" of="${CMAKE_BINARY_DIR}/openrct2_data" bs=4096 skip=1188 count=318
        COMMAND dd if=/dev/zero of="${CMAKE_BINARY_DIR}/openrct2_data" bs=4096 seek=318 count=2630 conv=notrunc
        COMMAND dd if="${OPENRCT2_EXE}" of="${CMAKE_BINARY_DIR}/openrct2_data" bs=4096 skip=1506 seek=2948 count=1 conv=notrunc
        DEPENDS ${OPENRCT2_EXE}
    )
    add_custom_target(segfiles DEPENDS openrct2_text openrct2_data)
    set(OBJ_FORMAT "elf32-i386")
    set(LINKER_SCRIPT "ld_script_i386.xc")
    if (APPLE)
        set(RCT2_SEGMENT_LINKER_FLAGS "-sectcreate rct2_text __text ${CMAKE_BINARY_DIR}/openrct2_text -sectcreate rct2_data __data ${CMAKE_BINARY_DIR}/openrct2_data -segaddr rct2_data 0x8a4000 -segprot rct2_data rwx rwx -segaddr rct2_text 0x401000 -segprot rct2_text rwx rwx -segaddr __TEXT 0x2000000 -read_only_relocs suppress")
    else ()
        # For Linux we have to use objcopy to wrap regular binaries into a linkable
        # format. We use specific section names which are then referenced in a
        # bespoke linker script so they can be placed at predefined VMAs.
        add_custom_command(
            OUTPUT openrct2_text_section.o
            COMMAND objcopy --input binary --output ${OBJ_FORMAT} --binary-architecture i386 openrct2_text openrct2_text_section.o --rename-section .data=.rct2_text,contents,alloc,load,readonly,code
            DEPENDS segfiles
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        add_custom_command(
            OUTPUT openrct2_data_section.o
            COMMAND objcopy --input binary --output ${OBJ_FORMAT} --binary-architecture i386 openrct2_data openrct2_data_section.o --rename-section .data=.rct2_data,contents,alloc,load,readonly,data
            DEPENDS segfiles
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        add_custom_target(linkable_sections DEPENDS openrct2_text_section.o openrct2_data_section.o)
        set_source_files_properties(
            openrct2_text_section.o openrct2_data_section.o
            PROPERTIES
            EXTERNAL_OBJECT true
            GENERATED true
        )
        # can't use GLOB here, as the files don't exist yet at cmake-time
        set(RCT2_SECTIONS "${CMAKE_BINARY_DIR}/openrct2_data_section.o" "${CMAKE_BINARY_DIR}/openrct2_text_section.o")
        set(RCT2_SEGMENT_LINKER_FLAGS "-Wl,-T,\"${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT}\"")
    endif ()
    set(ADDRESSES_CPP "Addresses.cpp")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TARGET_M}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${TARGET_M}")

add_executable(psa "psa_openrct2.cpp" "MemoryStream.cpp" "IStream.cpp" "Guard.cpp" "String.cpp" "Console.cpp" "UTF8.cpp" ${ADDRESSES_CPP} "main.cpp" ${RCT2_SECTIONS})

if (BUILD_32BIT)
    set_target_properties(psa PROPERTIES LINK_FLAGS ${RCT2_SEGMENT_LINKER_FLAGS})
endif()

# Don't bother doing find_package(), it probably won't give correct results for 32-bit anyway
target_link_libraries(psa z icuuc icudata benchmark)
