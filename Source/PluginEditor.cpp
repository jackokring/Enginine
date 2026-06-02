/*==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================*/

#include "BinaryData.h"
#include "PluginProcessor.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "PluginEditor.h"

#define keysHeight 100
#define margin 8

void EnginineAudioProcessorEditor::knob(juce::Slider& slider,
                                             std::function<void()> lambda,
                                             juce::AudioParameterFloat* para,
                                             juce::SliderParameterAttachment*& pa,
                                             bool editable)
{
  addAndMakeVisible (slider);
  slider.setLookAndFeel(&lookAndFeel);
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 15);
  slider.setTextValueSuffix (para->getLabel());
  slider.setTextBoxIsEditable(editable);
  pa = new juce::SliderParameterAttachment(*para, slider);
  // tooltip setup
  auto name = slider.getName();
  for(int x = 0; x < 9; ++x) for(int y = 0; y < 3; ++y) {
      if(layout[y][x] == &slider) {
          auto ccIdx = audioProcessor.cc[x][y];
          auto ccNum = "CC " + juce::String(ccIdx);
          if(audioProcessor.presetParas[y][x] == nullptr) {
              ccNum = "(" + ccNum + ")";
          }
          name = name + " " + ccNum;
          slider.setTooltip(name);
          slider.onValueChange =
              [this, ccIdx, lambda, para]() {
                  // no generate MIDI out on automations
                  if(midiOut[ccIdx]) {
                      auto norm = para->convertTo0to1(*para);
                      int bytes = floor(norm * 0x3fff);
                      audioProcessor.midiOutLock.lock();
                      // MSB
                      audioProcessor.midiOutBuffer.addEvent(
                          juce::MidiMessage::controllerEvent(
                              *audioProcessor.midiChannel, ccIdx, bytes >> 7), 0);
                      // LSB
                      audioProcessor.midiOutBuffer.addEvent(
                          juce::MidiMessage::controllerEvent(
                              *audioProcessor.midiChannel, ccIdx + 32, bytes & 0x7f), 0);
                      audioProcessor.midiOutLock.unlock();
                  };
                  lambda();
              };
          // drag MIDI out generate
          slider.onDragStart =
              [this, ccIdx]() {
                  midiOut[ccIdx] = true;
              };
          slider.onDragEnd =
              [this, ccIdx]() {
                  midiOut[ccIdx] = false;
              };
          break;
      }
   }
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

    background = juce::ImageCache::getFromMemory(
        BinaryData::background_png, BinaryData::background_pngSize);

    // TODO: main colour scheme
    setUIColour(juce::LookAndFeel_V4::ColourScheme::windowBackground, juce::Colours::black);
    setUIColour(juce::LookAndFeel_V4::ColourScheme::widgetBackground, juce::Colours::black);
    setUIColour(juce::LookAndFeel_V4::ColourScheme::menuBackground, juce::Colours::black);
    setUIColour(juce::LookAndFeel_V4::ColourScheme::outline, juce::Colours::red);
    setUIColour(juce::LookAndFeel_V4::ColourScheme::defaultText, juce::Colours::cyan);
    setUIColour(juce::LookAndFeel_V4::ColourScheme::defaultFill, juce::Colours::cyan);
    setUIColour(juce::LookAndFeel_V4::ColourScheme::highlightedText, juce::Colours::yellow);
    setUIColour(juce::LookAndFeel_V4::ColourScheme::highlightedFill, juce::Colours::yellow);
    setUIColour(juce::LookAndFeel_V4::ColourScheme::menuText, juce::Colours::green);

    // TODO: alter look and feel of knobs
    lookAndFeel.setColour(juce::Slider::backgroundColourId, juce::Colours::black);
    // pointer colour
    lookAndFeel.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    lookAndFeel.setColour(juce::Slider::trackColourId, juce::Colours::red);
    // on colour
    lookAndFeel.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::green);
    // off colour
    lookAndFeel.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::red);
    lookAndFeel.setColour(juce::Slider::textBoxTextColourId, juce::Colours::cyan);
    lookAndFeel.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::black);
    lookAndFeel.setColour(juce::Slider::textBoxHighlightColourId, juce::Colours::blue);
    lookAndFeel.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::red);

    addAndMakeVisible(keyboard);

    // all made visible in the knod function

    // bend
    knob(bendSlider, [this] {
        *audioProcessor.bend = bendSlider.getValue();
        audioProcessor.frequencyMult = std::powf(2.0f, (bendSlider.getValue() - 0.5f) * 2.0f);// +- 1 octave

    }, audioProcessor.bend, bendPA, false);
    // and fix up special case
    bendSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bendSlider.onDragEnd = [this] {
        // spring back to zero on release
        bendSlider.setValue(0.0f, juce::dontSendNotification);
        audioProcessor.frequencyMult = 1.0f;
    };
    bendSlider.setTooltip("Pitch Bender");

    // MIDI channel
    knob(midiChannelSlider, [this] {
        int chan = (int)midiChannelSlider.getValue();
        keyboard.setMidiChannel(chan);// int
        keyboard.setMidiChannelsToDisplay(1 << (chan - 1));// bit-mask
        *audioProcessor.midiChannel = chan;
    }, audioProcessor.midiChannel, midiChannelPA, false);

    // presets
    knob(presetSlider, [this] {
        *audioProcessor.savePreset = (int)presetSlider.getValue();
    }, audioProcessor.savePreset, presetPA, false);

    knob(volumeSlider, [this] {
      *audioProcessor.volume = volumeSlider.getValue();
    }, audioProcessor.volume, volumePA);
}

EnginineAudioProcessorEditor::~EnginineAudioProcessorEditor()
{
    // MUST delete ALL parameter attachments
    delete presetPA;
    delete volumePA;
    delete bendPA;
    delete midiChannelPA;

    // apparently it needs it to deallocate lookAndFeel
    setLookAndFeel(nullptr);
}

juce::Colour EnginineAudioProcessorEditor::UIColour(juce::LookAndFeel_V4::ColourScheme::UIColour colour)
{
    return lookAndFeel.getCurrentColourScheme().getUIColour(colour);
}

void EnginineAudioProcessorEditor::setUIColour(
    juce::LookAndFeel_V4::ColourScheme::UIColour colour, juce::Colour shade)
{
    lookAndFeel.getCurrentColourScheme().setUIColour(colour, shade);
}

//==============================================================================
void EnginineAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll (UIColour(juce::LookAndFeel_V4::ColourScheme::windowBackground));
    g.drawImageWithin(background, 0, 0, getWidth(), getHeight(),
        juce::RectanglePlacement::stretchToFit);
    g.setFont (juce::FontOptions (13.0f));

    auto area = getLocalBounds();
    area.removeFromBottom(keysHeight);
    area = area.reduced(margin);
    auto cWidth = area.getWidth() / 9.0f;
    auto cHeight = area.getHeight() / 3.0f;
    auto xOff = area.getX();
    auto yOff = area.getY();

    for(int x = 0; x < 9; ++x) for(int y = 0; y < 3; ++y) {
      auto name = knobLabels[y][x];
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
    auto keyArea = area.removeFromBottom(keysHeight);
    auto miniArea = keyArea.removeFromLeft(200).reduced(margin);
    bendSlider.setBounds(miniArea);
    keyboard.setBounds(keyArea);
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
