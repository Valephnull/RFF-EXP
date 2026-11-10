
# build profile is not defined
if (NOT DEFINED RFF_BUILD_PROFILE)
    message("${RFF_HELP_BUILD}")
    message(FATAL_ERROR "Build profile is not specified!")
endif ()

set(VALID_PROFILES Debug Release Native RelWithDebInfo)
list(FIND VALID_PROFILES "${RFF_BUILD_PROFILE}" PROFILE_INDEX)

# invalid build profile
if(PROFILE_INDEX EQUAL -1)
    message("${RFF_HELP_BUILD}")
    message(FATAL_ERROR "Invalid build profile: ${RFF_BUILD_PROFILE}")
endif()

# toolchain file is not defined
if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message("${RFF_HELP_BUILD}")
    message(FATAL_ERROR "Toolchain file not specified!")
endif ()

# toolchain file not found
get_filename_component(RFF_TOOLCHAIN_PATH "${CMAKE_TOOLCHAIN_FILE}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
if (NOT EXISTS "${RFF_TOOLCHAIN_PATH}")
    message("${RFF_HELP_BUILD}")
    message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE not found!")
endif ()


set(VALID_TOOLCHAINS "cmake/windows-x86_64-cross.cmake" "cmake/windows-x86_64.cmake" "cmake/linux-x86_64.cmake")
file(RELATIVE_PATH RFF_TOOLCHAIN_NAME "${CMAKE_SOURCE_DIR}" "${RFF_TOOLCHAIN_PATH}")
list(FIND VALID_TOOLCHAINS "${RFF_TOOLCHAIN_NAME}" TOOLCHAIN_INDEX)

# invalid toolchain file
if(TOOLCHAIN_INDEX EQUAL -1)
    message("${RFF_HELP_BUILD}")
    message(FATAL_ERROR "Invalid toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
endif()


message("
    The toolchain file is ${CMAKE_TOOLCHAIN_FILE}
    The build profile is ${RFF_BUILD_PROFILE}
")
