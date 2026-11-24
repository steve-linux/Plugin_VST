cmake_minimum_required(VERSION 3.22)

project(ChannelStrip VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CS_BUILD_CLAP "Build the CLAP format (via clap-juce-extensions)" ON)
set(JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (optional, otherwise it is downloaded)")

include(FetchContent)

if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.9
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

set(CS_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND CS_FORMATS AU)
endif()

juce_add_plugin(ChannelStrip
    COMPANY_NAME "StefanoAudio"
    BUNDLE_ID "com.stefanoaudio.channelstrip"
    PLUGIN_MANUFACTURER_CODE Stfa
    PLUGIN_CODE Chst
    FORMATS ${CS_FORMATS}
    PRODUCT_NAME "Channel Strip"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Dynamics EQ
    AU_MAIN_TYPE kAudioUnitType_Effect)

target_sources(ChannelStrip PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp
    Source/GUI/ModulePanels.cpp
    Source/GUI/Analyzer.cpp)

target_include_directories(ChannelStrip PRIVATE Source)

target_compile_definitions(ChannelStrip PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(ChannelStrip
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(CS_BUILD_CLAP)
    FetchContent_Declare(clap-juce-extensions
        GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
        GIT_TAG main
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(clap-juce-extensions)
    clap_juce_extensions_plugin(TARGET ChannelStrip
        CLAP_ID "com.stefanoaudio.channelstrip"
        CLAP_FEATURES audio-effect equalizer compressor limiter mixing stereo)
endif()
