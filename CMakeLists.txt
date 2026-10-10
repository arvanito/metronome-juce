cmake_minimum_required(VERSION 3.22)

project(MetronomeJUCE VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE is downloaded at configure time (pinned to a release tag).
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.3
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

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
    VERSION ${PROJECT_VERSION})

target_sources(Metronome PRIVATE
    ${CORE_SOURCES}
    Source/Main.cpp
    Source/ui/MainComponent.cpp
    Source/features/Features.cpp
    Source/features/BeatDisplay.cpp
    Source/features/Tempo.cpp
    Source/features/Transport.cpp
    Source/features/Meter.cpp
    Source/features/Sound.cpp
    Source/features/SpeedTrainer.cpp
    Source/features/Presets.cpp)

target_compile_definitions(Metronome PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Metronome,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Metronome,JUCE_VERSION>")

target_link_libraries(Metronome
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_basics
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
