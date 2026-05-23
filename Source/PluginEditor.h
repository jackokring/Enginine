/*==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

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
    void labelKnob(juce::Label& label, const juce::String& text);

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    EnginineAudioProcessor& audioProcessor;
    juce::MidiKeyboardComponent keyboard;
    juce::Slider volumeSlider;
    juce::Label volumeLabel;
    juce::SliderParameterAttachment *volumePA;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnginineAudioProcessorEditor)
};
