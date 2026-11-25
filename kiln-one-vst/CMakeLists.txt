cmake_minimum_required(VERSION 3.22)
project(KilnOne VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.4
  GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

option(KILN_INSTALL "Copy the VST3 into the system VST3 folder after building" OFF)

juce_add_plugin(KilnOne
  COMPANY_NAME "Kiln Audio"
  PLUGIN_MANUFACTURER_CODE Kiln
  PLUGIN_CODE KlnO
  FORMATS VST3 Standalone
  PRODUCT_NAME "Kiln One"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  VST3_CATEGORIES Fx Dynamics
  COPY_PLUGIN_AFTER_BUILD ${KILN_INSTALL})

target_sources(KilnOne PRIVATE
  Source/PluginProcessor.cpp Source/PluginProcessor.h
  Source/PluginEditor.cpp Source/PluginEditor.h)

target_compile_features(KilnOne PRIVATE cxx_std_17)
target_compile_definitions(KilnOne PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(KilnOne PRIVATE
  juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
