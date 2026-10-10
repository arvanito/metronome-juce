cmake_minimum_required(VERSION 3.22)

project(MetronomeJUCE VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE is downloaded at configure time (pinned to a release tag).
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.3
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

# The WebView2 SDK (NuGet package), which JUCE links statically on Windows. Downloading it
# here replaces the manual Install-Package step from JUCE's docs.
if(WIN32)
    set(WEBVIEW2_VERSION 1.0.3485.44)
    FetchContent_Declare(WebView2
        URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/${WEBVIEW2_VERSION}
        DOWNLOAD_NAME webview2.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/nuget/Microsoft.Web.WebView2.${WEBVIEW2_VERSION}")
    FetchContent_MakeAvailable(WebView2)
    set(JUCE_WEBVIEW2_PACKAGE_LOCATION "${FETCHCONTENT_BASE_DIR}/nuget")
endif()

# --- Web UI -------------------------------------------------------------------
# The UI is the metronome web app (git submodule in web/). It is built with npm and the
# result is zipped and embedded in the binary, then served to the WebView from memory.
find_program(NPM NAMES npm.cmd npm REQUIRED)
set(WEB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/web")
set(UI_DIR "${CMAKE_CURRENT_BINARY_DIR}/ui")
file(GLOB_RECURSE WEB_SOURCES CONFIGURE_DEPENDS
    "${WEB_DIR}/src/*" "${WEB_DIR}/public/*" "${WEB_DIR}/*.html" "${WEB_DIR}/vite.config.ts")

add_custom_command(OUTPUT "${UI_DIR}/npm-ci.stamp"
    COMMAND "${NPM}" ci --no-audit --no-fund
    COMMAND "${CMAKE_COMMAND}" -E touch "${UI_DIR}/npm-ci.stamp"
    WORKING_DIRECTORY "${WEB_DIR}"
    DEPENDS "${WEB_DIR}/package-lock.json"
    COMMENT "Installing web UI dependencies")

add_custom_command(OUTPUT "${UI_DIR}/ui.zip"
    COMMAND "${NPM}" run build -- --outDir "${UI_DIR}/dist" --emptyOutDir
    COMMAND "${CMAKE_COMMAND}" -E rm -f "${UI_DIR}/ui.zip"
    COMMAND "${CMAKE_COMMAND}" -E chdir "${UI_DIR}/dist" "${CMAKE_COMMAND}" -E tar cf "${UI_DIR}/ui.zip" --format=zip .
    WORKING_DIRECTORY "${WEB_DIR}"
    DEPENDS ${WEB_SOURCES} "${UI_DIR}/npm-ci.stamp"
    COMMENT "Building web UI")

juce_add_binary_data(MetronomeUI SOURCES "${UI_DIR}/ui.zip")

# Pure logic and the audio engine: shared by the app and the tests.
set(CORE_SOURCES
    Source/core/Settings.cpp
    Source/engine/Sounds.cpp
    Source/engine/MetronomeEngine.cpp)

# --- App ----------------------------------------------------------------------
juce_add_gui_app(Metronome
    PRODUCT_NAME "Metronome"
    COMPANY_NAME "arvanito"
    BUNDLE_ID "io.github.arvanito.metronome"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icon-512.png"
    NEEDS_WEBVIEW2 TRUE
    VERSION ${PROJECT_VERSION})

target_sources(Metronome PRIVATE
    ${CORE_SOURCES}
    Source/Main.cpp
    Source/ui/MainComponent.cpp)

target_compile_definitions(Metronome PRIVATE
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Metronome,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Metronome,JUCE_VERSION>")

target_link_libraries(Metronome
    PRIVATE
        MetronomeUI
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- Tests --------------------------------------------------------------------
juce_add_console_app(MetronomeTests PRODUCT_NAME "MetronomeTests")

target_sources(MetronomeTests PRIVATE ${CORE_SOURCES} Tests/Tests.cpp)

target_compile_definitions(MetronomeTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)

target_link_libraries(MetronomeTests
    PRIVATE
        juce::juce_audio_devices
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

enable_testing()
add_test(NAME MetronomeTests COMMAND MetronomeTests)
