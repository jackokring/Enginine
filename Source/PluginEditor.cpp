/*==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EnginineAudioProcessorEditor::EnginineAudioProcessorEditor (EnginineAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    keyboard(p.keyState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

    keyboard.setMidiChannel(1);// int
    keyboard.setMidiChannelsToDisplay(1);// bit-mask

    addAndMakeVisible(keyboard);
    addAndMakeVisible (volumeSlider);
    volumeSlider.setSliderStyle(juce::Slider::Rotary);
    volumeSlider.setTextValueSuffix (" Louds");
    volumeSlider.onValueChange = [this] {
      *audioProcessor.volume = volumeSlider.getValue();
    };
    volumePA = new juce::SliderParameterAttachment(*audioProcessor.volume, volumeSlider);
    addAndMakeVisible (volumeLabel);
    volumeLabel.setText ("Volume", juce::dontSendNotification);
    volumeLabel.attachToComponent (&volumeSlider, true);
}

EnginineAudioProcessorEditor::~EnginineAudioProcessorEditor()
{
    delete volumePA;
}

//==============================================================================
void EnginineAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    //g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void EnginineAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto area = getLocalBounds();
    auto keysHeight = 60;
    auto margin = 10;
    keyboard.setBounds(area.removeFromBottom(keysHeight));

    juce::Grid grid;
#define Cell juce::Grid::TrackInfo(juce::Grid::Fr(1))
#define Item juce::GridItem
    grid.templateRows = { Cell, Cell, Cell };
    grid.templateColumns = { Cell, Cell, Cell, Cell, Cell, Cell, Cell, Cell };
    grid.items = { Item(volumeSlider) };
    grid.performLayout(area.reduced(margin));
}
