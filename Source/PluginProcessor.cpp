/*==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================*/

#include "PluginProcessor.h"
#include "BinaryData.h"
#include "PluginEditor.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <cmath>

/*
template <typename ValueT>
juce::NormalisableRange<ValueT> logRange (ValueT min, ValueT max)
{
    ValueT rng{ std::log (max / min) };
    return { min, max,
        [=](ValueT min, ValueT, ValueT v) { return std::exp (v * rng) * min; },
        [=](ValueT min, ValueT, ValueT v) { return std::log (v / min) / rng; }
    };
}
*/

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
                       ),
#else
    :
#endif
    over(2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR),
    midiOutBuffer()
{
    // MIDI CC mappings
    // dynamic inverse for maintainance consistency
    for(int i = 0; i < 32; i++){
        icc[i] = nullptr;
    }
    for(int x = 0; x < 9; ++x) {
        for(int y = 0; y < 4; ++y) {
            // also makes preset non-saved not automatable by legacy MIDI CC
            // still automatable by host
            int ccIdx = cc[y][x];
            if(presetParas[y][x] != nullptr) {
                // MIDI CC to parameter mapping
                icc[ccIdx] = presetParas[y][x];
            }
        }
    }

    //=============================================================================
    // varoius parameter specifications for parameter typing
    // <float> used for <int> deadcode elimination and virtual
    // dispatch elimination for reduced I-cache pressure?

    // 3 decimal places
    auto decimals = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (auto x, auto) { return juce::String(floor(x * 1000) / 1000); });
    auto natural = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (auto x, auto) { return juce::String((int)floor(x)); });
    // preset list
    auto presetName = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction([this] (auto x, auto) {
          return getProgramName((int)x);
        });

    // varoius normalized ranges for parameter typing
    // power is signal squared so in linear power the range is skewed
    auto linpow = juce::NormalisableRange<float>(0.0f, 100.0f, 0.0f, 0.5f);
    auto preset128 = juce::NormalisableRange<float>(0.0f, 127.0f, 1.0f);
    auto nibble = juce::NormalisableRange<float>(1.0f, 16.0f, 1.0f);
    //auto hearing = logRange(20.0f, 20000.0f);
    auto octaveBend = juce::NormalisableRange<float>(-12.0f, 12.0f, 0.0f);

    //=============================================================================
    // parameters of the plugin
    bend = new juce::AudioParameterFloat (
        { "bend", 1 }, // parameter ID, version
        "Bend", // parameter name
        octaveBend, // parameter range
        0.0f, // default value
        decimals.withLabel(" semi")
    );

    addParameter (
        savePreset = new juce::AudioParameterFloat (
            { "savePreset", 1 }, // parameter ID, version
            "Save As", // parameter name
            preset128, // parameter range
            0.0f, // default value
            presetName // restrictions on print
        )
    );

    addParameter (
        volume = new juce::AudioParameterFloat (
            { "volume", 1 }, // parameter ID, version
            "Volume", // parameter name
            linpow, // parameter range
            50.0f, // default value
            decimals.withLabel(" %") // restrictions on print
        )
    );

    addParameter(
        midiChannel = new juce::AudioParameterFloat (
            { "midiChannel", 1 },
            "MIDI Chan",
            nibble, // parameter range
            1.0f, // default value
            natural // restrictions on print
        )
    );

    //=============================================================================
    // anything else last?
    loadPresets(); // load manufacturer presets
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
    return (int)currentPreset;
}

void EnginineAudioProcessor::setCurrentProgram (int index)
{
    // save to save in before loading new
    for(int x = 0; x < 9; ++x) for(int y = 0; y < 3; ++y) {
        if(presetParas[y][x] != nullptr) presets[(int)*savePreset][y][x] = **presetParas[y][x];
    }

    currentPreset = index;
    // load new preset values
    for(int x = 0; x < 9; ++x) for(int y = 0; y < 3; ++y) {
        if(presetParas[y][x] != nullptr) {
            auto para = *presetParas[y][x];
            // Normalization filter (code hardening)
            *para = para->getNormalisableRange().snapToLegalValue(presets[currentPreset][y][x]);
        }
    }
    *savePreset = index;
}

const juce::String EnginineAudioProcessor::getProgramName (int index)
{
    return juce::String(presetNames[index]);
}

void EnginineAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    // Not today
    juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
        "Program Name Change",
        "Sorry, " + getProgramName(index) + " is fixed and can't be changed to " + newName + ".");
}

//==============================================================================
void EnginineAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    smoothVol.setCurrentAndTargetValue(*volume * 0.01);
    smoothVol.reset(sampleRate, 0.05f); // 50ms ramp

    over.initProcessing(samplesPerBlock);
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

void EnginineAudioProcessor::process(juce::dsp::AudioBlock<float> signal) {
    // dsp code
}

void EnginineAudioProcessor::midi(juce::MidiMessage& msg) {
    // midi code
    if(msg.isForChannel(*midiChannel)) {
        // controller
        if(msg.isController()) {
            int controller = msg.getControllerNumber();
            if(controller < 64) {
                int value = msg.getControllerValue();
                auto parameter = icc[controller & 31];
                if(parameter == nullptr) {
                    // controller 5 etc?
                    return;
                }
                bool isMSB = controller < 32;
                // yes, it's a lot simpler than it appears
                float norm = (*parameter)->convertTo0to1(**parameter);// 0 to 1
                int n = floor(norm * 0x3fff);
                if(isMSB) {
                    n = (value << 7) | (n & 0x7f);
                } else {
                    n = (n & 0x3f80) | value;
                }
                norm = ((float)n) / 0x3fff;
                // this assign does the notify host thing
                // setValueNotifyingHost(float newValue) is implicit in this
                **parameter = (*parameter)->convertFrom0to1(norm);
                return;
            }
        }
        if(msg.isPitchWheel()) {
            int wheel = msg.getPitchWheelValue();
            float norm = (float)wheel / 0x3fff;
            *bend = bend->convertFrom0to1(norm);
            // speed cache as simplest solution
            frequencyMult = std::powf(2.0f, (norm - 0.5f) * 2.0f);// +- 1 octave
            return;
        }
        // notes
        if(msg.isChannelPressure()) {
            int pressure = msg.getChannelPressureValue();

            return;
        }
        if(msg.isAftertouch()) {
            int note = msg.getNoteNumber();
            int pressure = msg.getAfterTouchValue();

            return;
        }
        if(msg.isNoteOn()) {
            int note = msg.getNoteNumber();
            float velocity = msg.getFloatVelocity();

            return;
        }
        if(msg.isNoteOff()) {
            int note = msg.getNoteNumber();

            return;
        }
        // process kill notes
        if(msg.isAllNotesOff()) {
            return;
        }
        if(msg.isAllSoundOff()) {
            return;
        }
        if(msg.isResetAllControllers()) {
            for(int x = 0; x < 9; ++x) for(int y = 0; y < 3; ++y) {
                if(presetParas[y][x] != nullptr) {
                    auto para = *presetParas[y][x];
                    // Normalization filter (code hardening)
                    *para = para->getNormalisableRange().snapToLegalValue(presets[currentPreset][y][x]);
                }
            }
            return;
        }
        // preset change
        if(msg.isProgramChange()) {
            int program = msg.getProgramChangeNumber();
            setCurrentProgram(program);
            return;
        }
    } else {
        // clocking
        if(msg.isMidiClock()) {
            return;
        }
        if(msg.isMidiStart()) {
            return;
        }
        if(msg.isMidiStop()) {
            return;
        }
        if(msg.isMidiContinue()) {
            return;
        }
        if(msg.isSongPositionPointer()) {
            return;
        }
        // watchdog
        if(msg.isActiveSense()) {
            return;
        }
        // system exclusive
        if(msg.isSysEx()) {
            return;
        }
    }
}

void EnginineAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // disable denormals to avoid numerical instability in audio processing
    // I assume it's in the constructor to set a flag
    // I hope an -O3 won't break it, but then again fast, fast
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

    // Visuals, not sample acurate
    keyState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    over.reset();
    juce::dsp::AudioBlock<float> block(buffer);
    auto signal = over.processSamplesUp(block);
    //=======================================================================
    // current set to 4* oversample on signal
    auto num = signal.getNumSamples() * 4;// oversampling
    size_t begin = 0;
    for(auto m: midiMessages) {
        auto msg = m.getMessage();
        auto upto = m.samplePosition * 4;// oversampling
        process(signal.getSubBlock(begin, upto - begin));
        begin = upto;
        midi(msg);
    }
    process(signal.getSubBlock(begin, num - begin));

    //=======================================================================
    // downsample to output block and voulume set
    over.processSamplesDown(block);

    smoothVol.setTargetValue(*volume * 0.01);
    auto chans = buffer.getNumChannels();
    auto writes = buffer.getArrayOfWritePointers();
    for(int s = buffer.getNumSamples(); s > 0; --s) {
        for (int i = 0; i < chans; ++i) {
            writes[i][s - 1] *= smoothVol.getNextValue();
        }
    }

    if(midiOutLock.try_lock()) {
        for(auto m: midiOutBuffer) {
            // add drag drops at beginning of block as ...
            midiMessages.addEvent(m.getMessage(), 0);
        }
        // technically this might glitch a removal of a midi message
        // that was glitched sent in the iterator capture
        // thinning control messages?
        // hence a cheeky mutex
        // and maybe a small delay for some messages to be processed
        midiOutBuffer.clear();
        midiOutLock.unlock();
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

    // presets
    xml->setAttribute ("presetW", (double) *savePreset);
    xml->setAttribute ("presetR", (double) currentPreset);
    for(int p = 0; p < 128; ++p) for(int x = 0; x < 9; ++x) for(int y = 0; y < 3; ++y) {
        if(presetParas[y][x] != nullptr) {
            if(presetParas[y][x] != &savePreset) {
                xml->setAttribute("p" + juce::String(p * 27 + y * 9 + x), (double) presets[p][y][x]);
            } else {
                // savePreset layout, store preset index instead of value
                xml->setAttribute("p" + juce::String(p * 27 + y * 9 + x), (double) p);
            }
        }
    }
    copyXmlToBinary (*xml, destData);
}

void EnginineAudioProcessor::loadPresets()
{
    // load binary resource embedded presets file
    juce::MemoryInputStream stream(BinaryData::presets_zlib, BinaryData::presets_zlibSize, false);
    juce::GZIPDecompressorInputStream decompress(stream);// zlib format
    juce::MemoryBlock data;
    decompress.readIntoMemoryBlock(data);
    setStateInformation(data.getData(), data.getSize());
}

void EnginineAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml.get() != nullptr) {
        if (xml->hasTagName ("EnginineState")) {
            *volume = (float) xml->getDoubleAttribute ("volume", *volume);

            // presets
            *savePreset = (float)xml->getDoubleAttribute ("presetW", *savePreset);
            currentPreset = (float)xml->getDoubleAttribute ("presetR", currentPreset);
            for(int p = 0; p < 128; ++p) for(int x = 0; x < 9; ++x) for(int y = 0; y < 3; ++y) {
                if(presetParas[y][x] != nullptr) presets[p][y][x] =
                    (float)xml->getDoubleAttribute("p" + juce::String(p * 27 + y * 9 + x), presets[p][y][x]);
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
