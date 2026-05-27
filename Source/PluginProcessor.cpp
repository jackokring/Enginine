/*==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "juce_core/juce_core.h"

template <typename ValueT>
juce::NormalisableRange<ValueT> logRange (ValueT min, ValueT max)
{
    ValueT rng{ std::log (max / min) };
    return { min, max,
        [=](ValueT min, ValueT, ValueT v) { return std::exp (v * rng) * min; },
        [=](ValueT min, ValueT, ValueT v) { return std::log (v / min) / rng; }
    };
}

//==============================================================================
EnginineAudioProcessor::EnginineAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    auto decimals = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (auto x, auto) { return juce::String(floor(x * 1000) / 1000); });
    auto linpow = juce::NormalisableRange<float>(0.0f, 100.0f, 0.0f, 0.5f);
    auto hearing = logRange(20.0f, 20000.0f);

    addParameter (
        volume = new juce::AudioParameterFloat (
            { "volume", 1 }, // parameter ID, version
            "Volume", // parameter name
            linpow, // parameter range
            50.0f, // default value
            decimals.withLabel(" %") // restrictions on print
        )
    );
}

EnginineAudioProcessor::~EnginineAudioProcessor()
{
}

//==============================================================================
const juce::String EnginineAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EnginineAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EnginineAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EnginineAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EnginineAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================
int EnginineAudioProcessor::getNumPrograms()
{
    return 128;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EnginineAudioProcessor::getCurrentProgram()
{
    return currentPreset;
}

void EnginineAudioProcessor::setCurrentProgram (int index)
{
    for(int x = 0; x < 7; ++x) for(int y = 0; y < 3; ++y) {
        if(layout[y][x] != nullptr) presets[currentPreset][y][x] = **layout[y][x];
    }
    currentPreset = index;

    for(int x = 0; x < 7; ++x) for(int y = 0; y < 3; ++y) {
        if(layout[y][x] != nullptr)  **layout[y][x] = presets[currentPreset][y][x];
    }
}

const juce::String EnginineAudioProcessor::getProgramName (int index)
{
    return juce::String(luanames[currentPreset]);
}

void EnginineAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    // Not today
}

//==============================================================================
void EnginineAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    previousVolume = *volume;
}

void EnginineAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EnginineAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void EnginineAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    {
        keyState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);
        buffer.applyGainRamp (0, buffer.getNumSamples(), previousVolume, *volume);
        previousVolume = *volume;
    }
}

//==============================================================================
bool EnginineAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EnginineAudioProcessor::createEditor()
{
    return new EnginineAudioProcessorEditor (*this);
}

//==============================================================================
void EnginineAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    std::unique_ptr<juce::XmlElement> xml (new juce::XmlElement ("EnginineState"));
    xml->setAttribute ("volume", (double) *volume);
    for(int p = 0; p < 128; ++p) for(int x = 0; x < 7; ++x) for(int y = 0; y < 3; ++y) {
        if(layout[y][x] != nullptr)
            xml->setAttribute("p" + juce::String(p * 27 + y * 7 + x), (double) presets[p][y][x]);
    }
    copyXmlToBinary (*xml, destData);
}

void EnginineAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml.get() != nullptr) {
        if (xml->hasTagName ("EnginineState")) {
            *volume = (float) xml->getDoubleAttribute ("volume", *volume);
            for(int p = 0; p < 128; ++p) for(int x = 0; x < 7; ++x) for(int y = 0; y < 3; ++y) {
                if(layout[y][x] != nullptr) presets[p][y][x] =
                    (float)xml->getDoubleAttribute("p" + juce::String(p * 27 + y * 7 + x), presets[p][y][x]);
            }
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EnginineAudioProcessor();
}
