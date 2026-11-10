cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_RPATH "$ORIGIN")
if (NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
endif ()

# Keep existing installer/CI invocations working with upstream's build profiles.
if (NOT DEFINED RFF_BUILD_PROFILE)
    if (DEFINED RFF_NATIVE_OPTIMIZATIONS AND RFF_NATIVE_OPTIMIZATIONS)
        set(RFF_BUILD_PROFILE Native)
    elseif (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(RFF_BUILD_PROFILE "${CMAKE_BUILD_TYPE}")
    else ()
        set(RFF_BUILD_PROFILE Release)
    endif ()
endif ()
set(RFF_NATIVE_OPTIMIZATIONS OFF)
if (RFF_BUILD_PROFILE STREQUAL "Native")
    set(RFF_NATIVE_OPTIMIZATIONS ON)
endif ()
if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if (CMAKE_HOST_WIN32)
        set(CMAKE_TOOLCHAIN_FILE cmake/windows-x86_64.cmake)
    else ()
        set(CMAKE_TOOLCHAIN_FILE cmake/linux-x86_64.cmake)
    endif ()
endif ()
if (NOT DEFINED MSYS2_CLANG AND DEFINED ENV{MSYS2_ROOT})
    set(MSYS2_CLANG "$ENV{MSYS2_ROOT}/clang64")
endif ()

message("
═════════════════════════════════ TOOLCHAIN ═════════════════════════════════
")

include(cmake/Constants.cmake)
include(cmake/Help.cmake)
include(cmake/Macros.cmake)
include(cmake/Toolchain.cmake)
get_filename_component(CMAKE_TOOLCHAIN_FILE "${CMAKE_TOOLCHAIN_FILE}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")

message("
════════════════════════════ CONFIGURATION START ════════════════════════════
")

project(RFF)

include(cmake/Sources.cmake)
include(cmake/Definition.cmake)
include(cmake/BuildProfiles.cmake)
include(cmake/Dependencies.cmake)
include(cmake/Tests.cmake)



# detect system processor

if(${RFF_BUILD_PROFILE} STREQUAL "Native")
    set(CLANG_PROBE "${CMAKE_CURRENT_BINARY_DIR}/cpu_probe.cpp")
    file(WRITE "${CLANG_PROBE}" "int main() { return 0; }\n")

    execute_process(
            COMMAND
            ${CMAKE_CXX_COMPILER} -march=native -\#\#\# -c -x c++ ${CLANG_PROBE}
            OUTPUT_VARIABLE CLANG_STDOUT
            ERROR_VARIABLE CLANG_STDERR
            RESULT_VARIABLE CLANG_RESULT
    )

    set(CLANG_OUTPUT "${CLANG_STDOUT}\n${CLANG_STDERR}")

    string(REGEX MATCH
            "\"-target-cpu\" \"([^\"]+)\""
            CLANG_CPU_MATCH
            "${CLANG_OUTPUT}"
    )

    set(RFF_CPU_TARGET ${CMAKE_MATCH_1})
    file(REMOVE ${CLANG_PROBE})
else()
    set(RFF_CPU_TARGET "x86_64-generic")
endif()

# output dir
get_filename_component(RFF_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" ABSOLUTE)
message("
══════════════════════════════ BUILD PROFILE ════════════════════════════════

    TARGET
    Operating System  : ${CMAKE_SYSTEM_NAME}
    Target Processor  : ${RFF_CPU_TARGET}
    Build Type        : ${CMAKE_BUILD_TYPE}

    COMPILERS
    C Compiler        : ${CMAKE_C_COMPILER}
    C++ Compiler      : ${CMAKE_CXX_COMPILER}

    OUTPUT
    Runtime Directory : ${RFF_OUTPUT_DIR}

═════════════════════════════════════════════════════════════════════════════

"
)
