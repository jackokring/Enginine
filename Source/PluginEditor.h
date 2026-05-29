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
              juce::SliderParameterAttachment*& pa);
    juce::Colour UIColour(juce::LookAndFeel_V4::ColourScheme::UIColour colour);

    juce::Slider* layout[3][9] = {
        { &presetSlider, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &volumeSlider },
        { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr },
        { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }
    };

    // for some reason the joined knob does not like retrieving the parameter's name
    juce::String sLayout[3][9] = {
        { "Save In", "", "", "", "", "", "", "", "Volume" },
        { "", "", "", "", "", "", "", "", "" },
        { "", "", "", "", "", "", "", "", "" }
    };

//private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    EnginineAudioProcessor& audioProcessor;
    juce::LookAndFeel_V4 lookAndFeel;
    juce::Slider presetSlider;
    juce::SliderParameterAttachment *presetPA;

    juce::MidiKeyboardComponent keyboard;
    juce::Image background;
    juce::Slider volumeSlider;
    juce::SliderParameterAttachment *volumePA;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnginineAudioProcessorEditor)
};
