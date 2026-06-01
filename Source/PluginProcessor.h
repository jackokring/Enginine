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
    void loadPresets();
    void setStateInformation (const void* data, int sizeInBytes) override;

//private:
    //==============================================================================
    juce::MidiKeyboardState keyState;

    juce::AudioParameterFloat* savePreset;// not saved in presets
    int currentPreset;// apparently the DAW might not know how to set state correctly

    juce::AudioParameterFloat* volume;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothVol;

    juce::dsp::Oversampling<float> over;// the oversampler

    //==============================================================================
    // dump these to XML when a preset save happens
    juce::AudioParameterFloat** presetParas[3][9] = {
        { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &volume },
        { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr },
        { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }
    };

    //==============================================================================
    // MIDI things
    float presets[128][3][9] = {};//zeros

    // MIDI CC 14 bit controller numbers n and n+32
    // 2 - Carla Dry/Wet
    // 5 - Portamento (has duplicate on 84 [siren?])
    // 6 - NRPN, RPN Data Entry (kind of last selected drift [debug?])
    // 7 - Carla Volume
    // 8 - Carla Balance
    // So 27 left
    // parameter to MIDI CC
    int cc[3][9] = {
        { 0, 1, 3, 4, 9, 10, 11, 12, 13 },
        { 14, 15, 16, 17, 18, 19, 20, 21, 22 },
        { 23, 24, 25, 26, 27, 28, 29, 30, 31 }
    };

    // a null dump target for MIDI CC to parameter
    juce::AudioParameterFloat* noop;
    juce::AudioParameterFloat** icc[32];

    juce::AudioParameterFloat* midiChannel;
    float transpose = 0.0f;
    float tempoNoSync = 120.0f;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnginineAudioProcessor)
};
