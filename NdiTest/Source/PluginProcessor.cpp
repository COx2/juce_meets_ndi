/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NdiTestAudioProcessor::NdiTestAudioProcessor()
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
}

NdiTestAudioProcessor::~NdiTestAudioProcessor()
{
    if (getNdiEngine().isReceiving())
    {
        getNdiEngine().stopReceive();
        getNdiEngine().disconnect();
    }
}

//==============================================================================
const juce::String NdiTestAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NdiTestAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool NdiTestAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool NdiTestAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double NdiTestAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NdiTestAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int NdiTestAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NdiTestAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String NdiTestAudioProcessor::getProgramName (int index)
{
    return {};
}

void NdiTestAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void NdiTestAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    deviceSampleRate = sampleRate;
    deviceMaxBufferSize = samplesPerBlock;

    for(int i = 0; i < getTotalNumOutputChannels(); ++i)
    {
        auto ip = new juce::LagrangeInterpolator();
        ip->reset();
        interPolators_ndi_to_device.add(ip);
    }
}

void NdiTestAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NdiTestAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void NdiTestAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    if (getNdiEngine().audioCache.isReady())
    {
        const auto retrieve_ratio = (double)(getNdiEngine().audioCache.sampleRate) / getSampleRate();
        juce::AudioBuffer<float> retrieveBuffer(getNdiEngine().audioCache.numChannels, buffer.getNumSamples() * retrieve_ratio);
        getNdiEngine().audioCache.pop(retrieveBuffer);

        resamplingBuffer_ndi_to_device.reset(new juce::AudioBuffer<float>(buffer.getNumChannels(), buffer.getNumSamples()));

        const float actual_ratio_revert = (float)retrieveBuffer.getNumSamples() / (float)resamplingBuffer_ndi_to_device->getNumSamples();

        for(int ch_idx = 0; ch_idx < totalNumOutputChannels; ++ch_idx)
        {
            interPolators_ndi_to_device.getUnchecked(ch_idx)->process(
                actual_ratio_revert, retrieveBuffer.getReadPointer(ch_idx),
                resamplingBuffer_ndi_to_device->getWritePointer(ch_idx), resamplingBuffer_ndi_to_device->getNumSamples()
            );
        }

        buffer.makeCopyOf((*resamplingBuffer_ndi_to_device.get()), false);
    }
}

//==============================================================================
bool NdiTestAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* NdiTestAudioProcessor::createEditor()
{
    return new NdiTestAudioProcessorEditor (*this);
}

//==============================================================================
void NdiTestAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void NdiTestAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NdiTestAudioProcessor();
}
