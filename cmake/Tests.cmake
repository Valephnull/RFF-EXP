option(RFF_BUILD_TESTS "Build RFF-EXP render-pool tests" OFF)
if (RFF_BUILD_TESTS)
    enable_testing()
    add_executable(RFFRenderPoolNetworkTest
            tests/RenderPoolNetworkTest.cpp
            src/rff2/renderpool/RenderPoolNetwork.cpp)
    target_include_directories(RFFRenderPoolNetworkTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(RFFRenderPoolNetworkTest PRIVATE OpenSSL::Crypto)
    if (WIN32)
        target_link_libraries(RFFRenderPoolNetworkTest PRIVATE ws2_32)
    endif ()
    add_test(NAME render_pool_network COMMAND RFFRenderPoolNetworkTest)
    add_executable(RFFParallelRenderStatePauseTest
            tests/ParallelRenderStatePauseTest.cpp
            src/rff2/parallel/ParallelRenderState.cpp)
    target_include_directories(RFFParallelRenderStatePauseTest PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/include)
    add_test(NAME parallel_render_pause COMMAND RFFParallelRenderStatePauseTest)

    add_executable(RFFDynamicMapValidationTest
            tests/RFFDynamicMapValidationTest.cpp
            src/rff2/io/RFFDynamicMapBinary.cpp
            src/rff2/io/RFFLocationBinary.cpp
            src/rff2/renderpool/RenderPoolJob.cpp)
    target_include_directories(RFFDynamicMapValidationTest PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/extern
            ${CMAKE_SOURCE_DIR}/extern/imgui)
    target_compile_options(RFFDynamicMapValidationTest PRIVATE
            $<$<CONFIG:Release>:-O3 -ffast-math -fno-associative-math>)
    add_test(NAME dynamic_map_validation COMMAND RFFDynamicMapValidationTest)

    add_executable(RFFSettingsIOTest
            tests/RFFSettingsIOTest.cpp
            src/rff2/io/RFFSettingsIO.cpp)
    target_include_directories(RFFSettingsIOTest PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/extern
            ${CMAKE_SOURCE_DIR}/extern/imgui)
    if (WIN32)
        target_link_libraries(RFFSettingsIOTest PRIVATE ${CLANG64_LIBS}/libgmp.a)
        target_link_libraries(RFFDynamicMapValidationTest PRIVATE ${CLANG64_LIBS}/libgmp.a)
    else()
        target_link_libraries(RFFSettingsIOTest PRIVATE ${GMP_STATIC})
        target_link_libraries(RFFDynamicMapValidationTest PRIVATE ${GMP_STATIC})
    endif()
    add_test(NAME settings_io_round_trip COMMAND RFFSettingsIOTest)

    add_executable(RFFGradientLoaderTest tests/GradientLoaderTest.cpp src/rff2/io/GradientLoader.cpp)
    target_include_directories(RFFGradientLoaderTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
    add_test(NAME gradient_import COMMAND RFFGradientLoaderTest)

    add_executable(RFFVideoExportValidationTest tests/VideoExportValidationTest.cpp)
    target_include_directories(RFFVideoExportValidationTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_options(RFFVideoExportValidationTest PRIVATE -O3 -ffast-math)
    add_test(NAME video_export_validation COMMAND RFFVideoExportValidationTest)

    # These tests use assert; Release builds must execute those checks too.
    foreach (testTarget RFFRenderPoolNetworkTest RFFParallelRenderStatePauseTest
                        RFFDynamicMapValidationTest RFFSettingsIOTest)
        target_compile_options(${testTarget} PRIVATE -UNDEBUG)
        target_compile_definitions(${testTarget} PRIVATE RFF_SIMD_CHUNK_SIZE=${RFF_SIMD_CHUNK_SIZE})
    endforeach ()

    if (WIN32)
        set_tests_properties(render_pool_network PROPERTIES
                ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CLANG64_DIRS}/bin")
    endif ()
endif ()
