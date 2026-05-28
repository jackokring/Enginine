/*==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================*/

#include "PluginProcessor.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "PluginEditor.h"

#define keysHeight 100
#define margin 8

void EnginineAudioProcessorEditor::knob(juce::Slider& slider,
                                             std::function<void()> lambda,
                                             juce::AudioParameterFloat* para,
                                             juce::SliderParameterAttachment*& pa)
{
  addAndMakeVisible (slider);
  slider.setLookAndFeel(&lookAndFeel);
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
    setLookAndFeel(&lookAndFeel);
    setSize (888, 561);

    // alter look and feel of knobs
    lookAndFeel.setColour(juce::Slider::thumbColourId, juce::Colours::red);

    addAndMakeVisible(keyboard);
    keyboard.setMidiChannel(1);// int
    keyboard.setMidiChannelsToDisplay(1);// bit-mask

    // presets
    knob(presetSlider, [this] {
      *audioProcessor.savePreset = (int)presetSlider.getValue();
    }, audioProcessor.savePreset, presetPA);
    presetSlider.setTextBoxIsEditable(false);

    knob(volumeSlider, [this] {
      *audioProcessor.volume = volumeSlider.getValue();
    }, audioProcessor.volume, volumePA);
}

EnginineAudioProcessorEditor::~EnginineAudioProcessorEditor()
{
    // MUST delete ALL parameter attachments
    delete presetPA;
    delete volumePA;

    // apparently it needs it to deallocate lookAndFeel
    setLookAndFeel(nullptr);
}

juce::Colour EnginineAudioProcessorEditor::UIColour(juce::LookAndFeel_V4::ColourScheme::UIColour colour)
{
    return lookAndFeel.getCurrentColourScheme().getUIColour(colour);
}

//==============================================================================
void EnginineAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (UIColour(juce::LookAndFeel_V4::ColourScheme::windowBackground));

    g.setFont (juce::FontOptions (13.0f));

    auto area = getLocalBounds();
    area.removeFromBottom(keysHeight);
    area = area.reduced(margin);
    auto cWidth = area.getWidth() / 9.0f;
    auto cHeight = area.getHeight() / 3.0f;
    auto xOff = area.getX();
    auto yOff = area.getY();

    for(int x = 0; x < 9; ++x) for(int y = 0; y < 3; ++y) {
      auto name = sLayout[y][x];
      g.setColour (UIColour(juce::LookAndFeel_V4::ColourScheme::defaultText));
      g.drawFittedText(name, x * cWidth + xOff, y * cHeight + yOff,
          cWidth, 15, juce::Justification::centredBottom, 1);
      //bounds
      g.setColour (UIColour(juce::LookAndFeel_V4::ColourScheme::outline));
      g.drawRect(x * cWidth + xOff + 3, y * cHeight + 20 + yOff,
          cWidth - 6, cHeight - 20);
    }
}

void EnginineAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto area = getLocalBounds();
    keyboard.setBounds(area.removeFromBottom(keysHeight));
    area = area.reduced(margin);
    auto cWidth = area.getWidth() / 9.0f;
    auto cHeight = area.getHeight() / 3.0f;
    auto xOff = area.getX();
    auto yOff = area.getY();

    for(int x = 0; x < 9; ++x) for(int y = 0; y < 3; ++y) {
      if(layout[y][x] != nullptr) layout[y][x]->setBounds(x * cWidth + 5 + xOff,
          y * cHeight + 25 + yOff, cWidth - 10, cHeight - 30);
    }
}
