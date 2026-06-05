/*==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================*/

#include "PluginProcessor.h"
#include <BinaryData.h>
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
    // and an exceptional case for mod wheel
    icc[1] = &mod;

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
    // going to be ratio multiplied anyway elsewhere on the control path
    auto bpmRange = juce::NormalisableRange<float>(60.0f, 240.0f, 2.0f);

    //=============================================================================
    // parameters of the plugin
    addParameter(bend = new juce::AudioParameterFloat (
        { "bend", 1 }, // parameter ID, version
        "Pitch Bend", // parameter name
        octaveBend, // parameter range
        0.0f, // default value
        decimals.withLabel(" semi")
    ));

    addParameter(mod = new juce::AudioParameterFloat (
        { "mod", 1 }, // parameter ID, version
        "Mod Wheel", // parameter name
        linpow, // parameter range
        0.0f, // default value
        decimals.withLabel(" %")
    ));

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
            "MIDI Channel",
            nibble, // parameter range
            1.0f, // default value
            natural.withLabel(" chan") // restrictions on print
        )
    );

    addParameter(bpm = new juce::AudioParameterFloat (
        { "lfoBpm", 1 },
        "LFO BPM",
        bpmRange, // parameter range
        120.0f, // default value
        natural.withLabel(" BPM") // restrictions on print
    ));

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

// MIDI CC in sample acurate, not sure about the latency
// of VST parameter changes as likely block aligned
// I guess it's one for legacy MIDI CC
void EnginineAudioProcessor::onces() {
    // cache optimization of changed parameters
    frequencyMult = std::powf(2.0f, (*bend - 0.5f) * 2.0f);// +- 1 octave
    samplesPerMidiClock = getSampleRate() * 60.0f / ((*bpm) * 24.0f) * 4;// with 4x oversampling
}

void EnginineAudioProcessor::process(juce::dsp::AudioBlock<float> signal) {
    // dsp code

    // accumulate samples to clock internal LFO
    for(int i = 0; i < signal.getNumSamples(); ++i) {
        bool clk = (sampleAccumulator - samplesPerMidiClock) >= 0;
        if(clk) {
            midiClockContinue(true);
            sampleAccumulator -= samplesPerMidiClock;
        }
        ++sampleAccumulator;

        // now do the rest of the block dsp for a sample and two channels
#pragma GCC ivdep
        for (int chan = 0; chan < signal.getNumChannels(); ++chan) {
            float sample = signal.getSample(chan, i);

            // process a sample
        }
    }

    // post block workings

}

void EnginineAudioProcessor::sampleNotesAndHoldPedal(bool on) {

}

void EnginineAudioProcessor::midiClock(bool internal) {
    // no ifs, big butts
    int channel = (int)internal & 1;
    int increaseClock = (int)(clockRunning[channel]) & 1;
    int increaseSongPointer = ((int)(factorSix[channel] == 5) & 1) * increaseClock;
    increaseClock = increaseClock - 6 * increaseSongPointer;// auto to -1
    songPointer[channel] = (songPointer[channel] + increaseSongPointer) % 0x4000;
    factorSix[channel] = (factorSix[channel] + increaseClock);// avoids non power of 2 modulo %
}

void EnginineAudioProcessor::midiClockContinue(bool internal) {
    int channel = internal ? 1 : 0;
    factorSix[channel] = -1;// start from song position downbeat
    clockRunning[channel] = true;
}

void EnginineAudioProcessor::midiClockStart(bool internal) {
    int channel = internal ? 1 : 0;
    //if(!clockRunning[channel]) {
        songPointer[channel] = 0;
        factorSix[channel] = -1;
        clockRunning[channel] = true;
        //} // LFO sync an DAW
}

void EnginineAudioProcessor::midiClockStop(bool internal) {
    int channel = internal ? 1 : 0;
    clockRunning[channel] = false;
}

void EnginineAudioProcessor::midiSongPosition(bool internal, int position) {
    int channel = internal ? 1 : 0;
    songPointer[channel] = position &0x3fff;
}

void EnginineAudioProcessor::midi(juce::MidiMessage& msg) {
    // midi code
    if(msg.isForChannel(*midiChannel)) {
        // controller
        if(msg.isController()) {
            int controller = msg.getControllerNumber();
            int value = msg.getControllerValue();
            if(controller < 64) {
                auto parameter = icc[controller & 31];
                if(parameter == nullptr) {
                    // TODO: handle 6
                    return;
                }
                bool isMSB = controller < 32;
                // yes, it's a lot simpler than it appears
                float norm = (*parameter)->convertTo0to1(**parameter);// 0 to 1
                int n = floor(norm * 0x3fff);
                if(isMSB) {
                    // stepped 14-bit simulant
                    n = (value << 7) | value;
                } else {
                    n = (n & 0x3f80) | value;
                }
                norm = ((float)n) / 0x3fff;
                // this assign does the notify host thing
                // setValueNotifyingHost(float newValue) is implicit in this
                **parameter = (*parameter)->convertFrom0to1(norm);
                return;
            }
            if(controller < 96) {
                auto parameter = iccHighs[controller - 64];
                if(parameter == nullptr) {
                    return;
                }
                // stepped 14-bit simulant
                float norm = ((float)((value << 7) | value)) / 0x3fff;
                **parameter = (*parameter)->convertFrom0to1(norm);
                return;
            }
        }
        if(msg.isPitchWheel()) {
            int wheel = msg.getPitchWheelValue();
            float norm = (float)wheel / 0x3fff;
            *bend = bend->convertFrom0to1(norm);
            return;
        }
        // notes
        if(msg.isChannelPressure()) {
            int pressure = msg.getChannelPressureValue();
            channelPressure = ((float)((pressure << 7) | pressure)) / 0x3fff;
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
        if(msg.isSustainPedalOn()) {
            sustainPedal = true;
            return;
        }
        if(msg.isSustainPedalOff()) {
            sustainPedal = false;
            return;
        }
        if(msg.isSostenutoPedalOn()) {
            sampleNotesAndHoldPedal(true);
            return;
        }
        if(msg.isSostenutoPedalOff()) {
            sampleNotesAndHoldPedal(false);
            return;
        }
        if(msg.isSoftPedalOn()) {
            softPedal = true;
            return;
        }
        if(msg.isSoftPedalOff()) {
            softPedal = false;
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
            // rest bender
            *bend = 0.0f;
            // reset mod wheel (assumption of wild to nil)
            *mod = 0.0f;
            // channel pressure
            channelPressure = 0.0f;
            // pedal state
            sustainPedal = false;
            sampleNotesAndHoldPedal(false);
            softPedal = false;
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
            midiClock(false);
            return;
        }
        if(msg.isMidiStart()) {
            midiClockStart(false);
            return;
        }
        if(msg.isMidiStop()) {
            midiClockStop(false);
            return;
        }
        if(msg.isMidiContinue()) {
            midiClockContinue(false);
            return;
        }
        if(msg.isSongPositionPointer()) {
            midiSongPosition(false, msg.getSongPositionPointerMidiBeat());
            return;
        }
        if(msg.getRawData()[0] == 0xf3) {
            // Song Select
            int song = msg.getRawData()[1];
            // not sure if I'll use it
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

    // possible changes of cached parameter calcs
    onces();

    //over.reset();// how would it remember the last filter memory state?
    juce::dsp::AudioBlock<float> block(buffer);
    auto signal = over.processSamplesUp(block);
    //=======================================================================
    // current set to 4* oversample on signal
    auto num = signal.getNumSamples();// oversampling implicit in signal
    size_t begin = 0;
    for(auto m: midiMessages) {
        auto msg = m.getMessage();
        auto upto = m.samplePosition * 4;// oversampling not implicit in MIDI
        process(signal.getSubBlock(begin, upto - begin));
        begin = upto;
        midi(msg);
        // possible changes of cached parameter calcs
        onces();
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

    // now processed can add in all CC dragged GUI messages to thru chain
    // a bit latency bad, but the GUI would present most of the latency
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
