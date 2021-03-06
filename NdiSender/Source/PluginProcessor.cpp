/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NdiSenderAudioProcessor::NdiSenderAudioProcessor()
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
    getNdiEngine().startSend();
}

NdiSenderAudioProcessor::~NdiSenderAudioProcessor()
{
    getNdiEngine().stopSend();
}

//==============================================================================
const juce::String NdiSenderAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NdiSenderAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool NdiSenderAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool NdiSenderAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double NdiSenderAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NdiSenderAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int NdiSenderAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NdiSenderAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String NdiSenderAudioProcessor::getProgramName (int index)
{
    return {};
}

void NdiSenderAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void NdiSenderAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    ndiWrapper.audioCache.numChannels = getTotalNumOutputChannels();
    ndiWrapper.audioCache.sampleRate = sampleRate;
    ndiWrapper.audioCache.reset();
}

void NdiSenderAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NdiSenderAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
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

void NdiSenderAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    ndiWrapper.audioCache.numChannels = buffer.getNumChannels();
    ndiWrapper.audioCache.sampleRate = static_cast<int>(getSampleRate());
    ndiWrapper.audioCache.push(buffer);
}

//==============================================================================
bool NdiSenderAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* NdiSenderAudioProcessor::createEditor()
{
    return new NdiSenderAudioProcessorEditor (*this);
}

//==============================================================================
void NdiSenderAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void NdiSenderAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NdiSenderAudioProcessor();
}
