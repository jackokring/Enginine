/*==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================*/

#pragma once

#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include <JuceHeader.h>
#include <JucePluginDefines.h>
#include "presetnames.h"

//==============================================================================
/**
*/
class EnginineAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    EnginineAudioProcessor();
    ~EnginineAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

//private:
    //==============================================================================
    juce::MidiKeyboardState keyState;
    juce::AudioParameterFloat* savePreset;// not saved in presets
    int currentPreset;
    juce::AudioParameterFloat* volume;
    float previousVolume;

    //==============================================================================
    juce::AudioParameterFloat** layout[3][9] = {
        { &savePreset, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &volume },
        { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr },
        { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }
    };

    float presets[128][3][9] = {
        {//1
            {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
        },
        {//2
            {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
        },

    };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnginineAudioProcessor)
};
