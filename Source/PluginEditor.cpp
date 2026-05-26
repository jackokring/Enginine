/*==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#define keysHeight 100
#define margin 20

void EnginineAudioProcessorEditor::knob(juce::Slider& slider,
                                             std::function<void()> lambda,
                                             juce::AudioParameterFloat* para,
                                             juce::SliderParameterAttachment*& pa)
{
  addAndMakeVisible (slider);
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 15);
  slider.onValueChange = lambda;
  slider.setTextValueSuffix (para->getLabel());
  pa = new juce::SliderParameterAttachment(*para, slider);
}

//==============================================================================
EnginineAudioProcessorEditor::EnginineAudioProcessorEditor (EnginineAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    keyboard(p.keyState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (800, 600);

    // alter look and feel of knobs
    getLookAndFeel().setColour(juce::Slider::thumbColourId, juce::Colours::red);

    addAndMakeVisible(keyboard);
    keyboard.setMidiChannel(1);// int
    keyboard.setMidiChannelsToDisplay(1);// bit-mask

    knob(volumeSlider, [this] {
      *audioProcessor.volume = volumeSlider.getValue();
    }, audioProcessor.volume, volumePA);
}

EnginineAudioProcessorEditor::~EnginineAudioProcessorEditor()
{
    // MUST delete ALL parameter attachments
    delete volumePA;
}

//==============================================================================
void EnginineAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));

    auto area = getLocalBounds();
    area = area.removeFromBottom(keysHeight);
    area = area.reduced(margin);
    auto cWidth = area.getWidth() / 7;
    auto cHeight = area.getHeight() / 3;

    for(int x = 0; x < 7; ++x) for(int y = 0; y < 3; ++y) {
      auto name = sLayout[y][x];
      g.drawFittedText(name, x * cWidth, y * cHeight, cWidth, 15, juce::Justification::centredBottom, 1);
    }
}

void EnginineAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto area = getLocalBounds();
    keyboard.setBounds(area.removeFromBottom(keysHeight));
    area = area.reduced(margin);
    auto cWidth = area.getWidth() / 7;
    auto cHeight = area.getHeight() / 3;

    for(int x = 0; x < 7; ++x) for(int y = 0; y < 3; ++y) {
      if(layout[y][x] != nullptr) layout[y][x]->setBounds(x * cWidth, y * cHeight + 15, cWidth, cHeight - 15);
    }
}
