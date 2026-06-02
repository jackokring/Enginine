/*==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_gui_basics/juce_gui_basics.h"

//==============================================================================
/**
*/
class EnginineAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    EnginineAudioProcessorEditor (EnginineAudioProcessor&);
    ~EnginineAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void knob(juce::Slider& slider,
              std::function<void()> lambda,
              juce::AudioParameterFloat* para,
              juce::SliderParameterAttachment*& pa,
              bool editable = true, int defaultCC = -1,
              bool keepRotary = false);
    juce::Colour UIColour(juce::LookAndFeel_V4::ColourScheme::UIColour colour);
    void setUIColour(
        juce::LookAndFeel_V4::ColourScheme::UIColour colour, juce::Colour shade);

    // front panel slider layout
    juce::Slider* layout[3][9] = {
        { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &volumeSlider },
        { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr },
        { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }
    };

    // front panel slider parameter names
    // for some reason the joined knob does not like retrieving the parameter's name
    juce::String knobLabels[3][9] = {
        { "", "Hi Pass", "Boost", "Lo Pass", "Rez", "Warm", "Split", "Trim", "Volume" },
        { "", "", "", "", "", "", "", "", "" },
        { "", "", "", "", "", "", "", "", "" }
    };

    // MIDI CC in active don't do GUI CC output
    bool midiOut[32] = {
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false
    };
    // MIDI CC form 64 so access via midiOutHighs[cc - 64]
    bool midiOutHighs[32] = {
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false
    };
    bool bendOut = false;

//private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    EnginineAudioProcessor& audioProcessor;
    juce::LookAndFeel_V4 lookAndFeel;
    juce::MidiKeyboardComponent keyboard;
    juce::Image background;
    juce::TooltipWindow* tooltipWindow;

    //================================================================
    // sliders and PAs
    juce::Slider bendSlider;
    juce::SliderParameterAttachment *bendPA;
    juce::Slider modSlider;
    juce::SliderParameterAttachment *modPA;
    juce::Slider presetSlider;
    juce::SliderParameterAttachment *presetPA;
    juce::Slider volumeSlider;
    juce::SliderParameterAttachment *volumePA;
    juce::Slider midiChannelSlider;
    juce::SliderParameterAttachment *midiChannelPA;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnginineAudioProcessorEditor)
};
