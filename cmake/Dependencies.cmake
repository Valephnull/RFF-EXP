
if (WIN32)
    if ("${MSYS2_CLANG}" STREQUAL "")
        message(${RFF_HELP_MSYS2})
        message(FATAL_ERROR "Configuration aborted.")
    else ()
        set(CLANG64_DIRS "${MSYS2_CLANG}")
        set(CLANG64_LIBS "${MSYS2_CLANG}/lib")
    endif ()
endif ()


add_subdirectory(${CMAKE_SOURCE_DIR}/src/vulkan_helper)
find_package(OpenSSL REQUIRED)

target_link_libraries(RFF PRIVATE
        # Vulkan Helper
        vulkan_helper
        OpenSSL::Crypto
)

target_include_directories(RFF PRIVATE
        # extern sources
        ${CMAKE_SOURCE_DIR}/extern
        # clang include dirs
        ${CMAKE_SOURCE_DIR}/include
)
if (WIN32)
    if (EXISTS "${CLANG64_DIRS}/include/opencv5")
        set(RFF_OPENCV_INCLUDE_DIR "${CLANG64_DIRS}/include/opencv5")
    else ()
        set(RFF_OPENCV_INCLUDE_DIR "${CLANG64_DIRS}/include/opencv4")
    endif ()
    foreach (component core imgcodecs imgproc videoio highgui freetype)
        find_library(RFF_OPENCV_${component}
                NAMES opencv_${component} opencv_${component}4110
                PATHS "${CLANG64_LIBS}" NO_DEFAULT_PATH REQUIRED)
        target_link_libraries(RFF PRIVATE "${RFF_OPENCV_${component}}")
    endforeach ()
    target_link_libraries(
            RFF PRIVATE
            # GMP Linker
            ${CLANG64_LIBS}/libgmp.a
            # OpenCV Linkers
            # FFmpeg Linkers
            ${CLANG64_LIBS}/libavcodec.dll.a
            ${CLANG64_LIBS}/libavformat.dll.a
            ${CLANG64_LIBS}/libavutil.dll.a
            ws2_32
    )


    target_include_directories(RFF PRIVATE
            # clang include dirs
            ${CLANG64_DIRS}/include
            # opencv4 include dirs
            ${RFF_OPENCV_INCLUDE_DIR}
    )
elseif (APPLE)
    target_link_libraries(RFF PRIVATE "-framework Cocoa")
    # TODO non compilable
elseif (UNIX)

    macro(find_static_library OUT_VAR LIB_NAME)
        find_library(${OUT_VAR}
                NAMES lib${LIB_NAME}.a
                PATHS
                /usr/local/lib
                /usr/lib
                /usr/lib/x86_64-linux-gnu
                REQUIRED
        )
    endmacro()

    find_package(OpenCV REQUIRED COMPONENTS core imgcodecs imgproc videoio highgui freetype)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil)

    target_link_directories(RFF PRIVATE ${GTK3_LIBRARY_DIRS})
    target_compile_options(RFF PRIVATE ${GTK3_CFLAGS_OTHER})

    if(NOT DEFINED GMP_STATIC)
        message("GMP_STATIC is not set. finding gmp...")
        find_static_library(GMP_STATIC gmp)
    endif()
    message("GMP_STATIC path is ${GMP_STATIC}")

    target_link_libraries(RFF PRIVATE
            # GMP Linker
            ${GMP_STATIC}
            # gtk libraries
            ${GTK3_LIBRARIES}
            # OpenCV Linkers
            ${OpenCV_LIBS}
            # FFmpeg Linkers
            ${FFMPEG_LIBRARIES}
    )
    target_include_directories(RFF PRIVATE
            # opencv4 include dirs
            ${OpenCV_INCLUDE_DIRS}
            # gtk
            ${GTK3_INCLUDE_DIRS}
            # ffmpeg
            ${FFMPEG_INCLUDE_DIRS}
    )

    target_link_directories(RFF PRIVATE
            # ffmpeg
            ${FFMPEG_LIBRARY_DIRS}
    )

endif ()

include(FetchContent)
set(UPNPC_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(UPNPC_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(UPNPC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(UPNPC_BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
set(UPNPC_NO_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(miniupnpc
        GIT_REPOSITORY https://github.com/miniupnp/miniupnp.git
        GIT_TAG miniupnpc_2_3_3
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR miniupnpc)
FetchContent_MakeAvailable(miniupnpc)

FetchContent_Declare(
        nfd
        GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
        GIT_TAG master
)

FetchContent_MakeAvailable(nfd)

target_link_libraries(RFF PRIVATE nfd libminiupnpc-static)
