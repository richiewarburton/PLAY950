cmake_minimum_required(VERSION 3.25)

project(PLAY950 VERSION 0.13.3 LANGUAGES C CXX OBJC OBJCXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS version")

set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES ON CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)
set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)
add_subdirectory(external/vst3sdk)

add_library(play950_formats STATIC
    source/formats/P9.cpp
    source/formats/P9.h
    source/formats/S9.cpp
    source/formats/S9.h
)
target_include_directories(play950_formats PUBLIC source)
target_compile_features(play950_formats PUBLIC cxx_std_20)

add_library(play950_audio STATIC
    source/audio/ProgramVoicePool.cpp
    source/audio/ProgramVoicePool.h
    source/audio/SampleVoicePool.cpp
    source/audio/SampleVoicePool.h
)
target_include_directories(play950_audio PUBLIC source)
target_compile_features(play950_audio PUBLIC cxx_std_20)
target_link_libraries(play950_audio PUBLIC play950_formats)

add_library(play950_state STATIC
    source/state/ProjectState.cpp
    source/state/ProjectState.h
)
target_include_directories(play950_state PUBLIC source)
target_compile_features(play950_state PUBLIC cxx_std_20)

add_library(play950_content STATIC
    source/content/ContentLoader.cpp
    source/content/ContentLoader.h
)
target_include_directories(play950_content PUBLIC source)
target_compile_features(play950_content PUBLIC cxx_std_20)
target_link_libraries(play950_content PUBLIC play950_formats)

add_library(play950_workflow STATIC
    source/workflow/ImageWorkflow.cpp
    source/workflow/ImageWorkflow.h
)
target_include_directories(play950_workflow PUBLIC source)
target_compile_features(play950_workflow PUBLIC cxx_std_20)

set(_PLAY950_LEGACY_AKAIUTIL "")
if(DEFINED CACHE{TRUE950_AKAIUTIL})
    get_property(_PLAY950_LEGACY_AKAIUTIL CACHE TRUE950_AKAIUTIL PROPERTY VALUE)
endif()
set(PLAY950_AKAIUTIL "${_PLAY950_LEGACY_AKAIUTIL}"
    CACHE FILEPATH "Optional trusted AKAI Util executable for comparison tests")
option(PLAY950_BUNDLE_AKAIUTIL
    "Bundle the vendored Universal AKAI Util helper" OFF)

add_executable(play950_akaiutil
    external/akaiutil/akaiutil_main.c
    external/akaiutil/akaiutil_tar.c
    external/akaiutil/akaiutil_file.c
    external/akaiutil/akaiutil_take.c
    external/akaiutil/akaiutil_wav.c
    external/akaiutil/akaiutil.c
    external/akaiutil/akaiutil_io.c
    external/akaiutil/commonlib.c
)
target_include_directories(play950_akaiutil PRIVATE external/akaiutil)
target_compile_options(play950_akaiutil PRIVATE -Wall -Wextra)
if(UNIX)
    target_link_libraries(play950_akaiutil PRIVATE m)
endif()

if(BUILD_TESTING)
    add_test(NAME play950.plugin.no_audio_input
        COMMAND "${CMAKE_COMMAND}"
                -DPROCESSOR_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/source/Play950Processor.cpp
                -P ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-no-audio-input.cmake)

    add_executable(play950_format_tests tests/S9Tests.cpp)
    target_link_libraries(play950_format_tests PRIVATE play950_formats)
    add_test(NAME play950.s9.unit COMMAND play950_format_tests)

    add_executable(play950_p9_tests tests/P9Tests.cpp)
    target_link_libraries(play950_p9_tests PRIVATE play950_formats)
    add_test(NAME play950.p9.unit COMMAND play950_p9_tests)

    add_executable(play950_audio_tests tests/SampleVoicePoolTests.cpp)
    target_link_libraries(play950_audio_tests PRIVATE play950_audio)
    add_test(NAME play950.audio.voice_pool COMMAND play950_audio_tests)

    add_executable(play950_program_audio_tests tests/ProgramVoicePoolTests.cpp)
    target_link_libraries(play950_program_audio_tests PRIVATE play950_audio)
    add_test(NAME play950.audio.program_voice_pool COMMAND play950_program_audio_tests)

    add_executable(play950_project_state_tests tests/ProjectStateTests.cpp)
    target_link_libraries(play950_project_state_tests PRIVATE play950_state)
    add_test(NAME play950.state.project COMMAND play950_project_state_tests)

    add_executable(play950_image_workflow_tests tests/ImageWorkflowTests.cpp)
    target_link_libraries(play950_image_workflow_tests PRIVATE play950_workflow)
    add_test(NAME play950.workflow.images COMMAND play950_image_workflow_tests)
endif()

set(PLAY950_UI_RESOURCES
    Resources/PLAY950Logo.png
    Resources/FIND950Logo.png
    Resources/EDIT950Logo.png
)

smtg_add_vst3plugin(PLAY950
    ${PLAY950_UI_RESOURCES}
    source/Play950Controller.cpp
    source/Play950Controller.h
    source/Play950Editor.h
    source/Play950Editor.mm
    source/Play950Factory.cpp
    source/Play950Ids.h
    source/Play950Messages.h
    source/Play950Processor.cpp
    source/Play950Processor.h
    source/Play950Version.h
)

set_source_files_properties(${PLAY950_UI_RESOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

target_compile_definitions(PLAY950 PRIVATE PLAY950_VERSION="${PROJECT_VERSION}")
target_link_libraries(PLAY950 PRIVATE sdk play950_audio play950_state play950_content play950_workflow)
set_source_files_properties(source/Play950Editor.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")

if(EXISTS "${PLAY950_AKAIUTIL}" AND NOT PLAY950_BUNDLE_AKAIUTIL)
    target_compile_definitions(PLAY950 PRIVATE
        PLAY950_AKAIUTIL_PATH="${PLAY950_AKAIUTIL}")
endif()
if(PLAY950_BUNDLE_AKAIUTIL)
    target_compile_definitions(PLAY950 PRIVATE PLAY950_BUNDLED_AKAIUTIL=1)
endif()
if(APPLE)
    target_sources(PLAY950 PRIVATE external/vst3sdk/public.sdk/source/main/macmain.cpp)
    target_link_libraries(PLAY950 PRIVATE "-framework Cocoa" "-framework QuartzCore"
        "-framework UniformTypeIdentifiers")
endif()

smtg_target_configure_version_file(PLAY950)

if(SMTG_MAC)
    smtg_target_set_bundle(PLAY950
        BUNDLE_IDENTIFIER "com.e45recordings.play950"
        COMPANY_NAME "e45recordings"
    )

    # SMTG_Bundle currently applies these standard CMake properties only when
    # using the Xcode generator. Keep command-line Make/Ninja bundles identical.
    set_target_properties(PLAY950 PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.e45recordings.play950"
        MACOSX_BUNDLE_COPYRIGHT "© 2026 e45recordings."
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_INFO_STRING "${PROJECT_VERSION}"
    )

    # The SDK creates moduleinfo.json after its Xcode 15 ad-hoc signing
    # workaround. Sign the completed bundle last so strict host scanners see a
    # valid seal containing the generated metadata.
    get_target_property(PLAY950_PLUGIN_PACKAGE_PATH PLAY950 SMTG_PLUGIN_PACKAGE_PATH)
    add_custom_command(TARGET PLAY950 POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${PLAY950_PLUGIN_PACKAGE_PATH}/Contents/Resources"
        COMMENT "Preparing PLAY950 interface resources"
        VERBATIM
    )
    foreach(PLAY950_UI_RESOURCE IN LISTS PLAY950_UI_RESOURCES)
        get_filename_component(PLAY950_UI_RESOURCE_NAME "${PLAY950_UI_RESOURCE}" NAME)
        add_custom_command(TARGET PLAY950 POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/${PLAY950_UI_RESOURCE}"
                    "${PLAY950_PLUGIN_PACKAGE_PATH}/Contents/Resources/${PLAY950_UI_RESOURCE_NAME}"
            COMMENT "Bundling ${PLAY950_UI_RESOURCE_NAME}"
            VERBATIM
        )
    endforeach()
    if(PLAY950_BUNDLE_AKAIUTIL)
        add_dependencies(PLAY950 play950_akaiutil)
        add_custom_command(TARGET PLAY950 POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                    "${PLAY950_PLUGIN_PACKAGE_PATH}/Contents/Resources"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:play950_akaiutil>"
                    "${PLAY950_PLUGIN_PACKAGE_PATH}/Contents/Resources/akaiutil"
            COMMAND chmod 755
                    "${PLAY950_PLUGIN_PACKAGE_PATH}/Contents/Resources/akaiutil"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/external/akaiutil/README.txt"
                    "${PLAY950_PLUGIN_PACKAGE_PATH}/Contents/Resources/AKAI-Util-NOTICE.txt"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/external/akaiutil/gpl-2.0.txt"
                    "${PLAY950_PLUGIN_PACKAGE_PATH}/Contents/Resources/AKAI-Util-GPL-2.0.txt"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/external/akaiutil/PLAY950-PROVENANCE.md"
                    "${PLAY950_PLUGIN_PACKAGE_PATH}/Contents/Resources/AKAI-Util-SOURCE.txt"
            COMMENT "Bundling AKAI Util and redistribution notice"
            VERBATIM
        )
    endif()
    add_custom_command(TARGET PLAY950 POST_BUILD
        COMMAND codesign --force --deep --sign "-" "${PLAY950_PLUGIN_PACKAGE_PATH}"
        COMMENT "Ad-hoc signing completed PLAY950 bundle"
        VERBATIM
    )
endif()
