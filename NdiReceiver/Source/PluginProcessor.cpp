/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NdiReceiverAudioProcessor::NdiReceiverAudioProcessor()
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

NdiReceiverAudioProcessor::~NdiReceiverAudioProcessor()
{
    if (getNdiEngine().isReceiving())
    {
        getNdiEngine().stopReceive();
        getNdiEngine().disconnect();
    }
}

//==============================================================================
const juce::String NdiReceiverAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NdiReceiverAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool NdiReceiverAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool NdiReceiverAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double NdiReceiverAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NdiReceiverAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int NdiReceiverAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NdiReceiverAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String NdiReceiverAudioProcessor::getProgramName (int index)
{
    return {};
}

void NdiReceiverAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void NdiReceiverAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    deviceSampleRate = sampleRate;
    deviceMaxBufferSize = samplesPerBlock;

    interPolators_NdiToDevice.clear();
    for (int i = 0; i < getTotalNumOutputChannels(); ++i)
    {
        auto ip = new juce::LagrangeInterpolator();
        ip->reset();
        interPolators_NdiToDevice.add(ip);
    }
}

void NdiReceiverAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NdiReceiverAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void NdiReceiverAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    const bool will_fade_in_this_frame = isLastRenderedSamplesShorten && getNdiEngine().audioCache.isReady();

    if (getNdiEngine().audioCache.isReady())
    {
        resamplingBuffer_NdiToDevice.reset(new juce::AudioBuffer<float>(buffer.getNumChannels(), buffer.getNumSamples()));

        const auto retrieve_ratio = (double)(getNdiEngine().audioCache.sampleRate) / getSampleRate();
        juce::AudioBuffer<float> retrieve_buffer(getNdiEngine().audioCache.numChannels, buffer.getNumSamples() * retrieve_ratio);
        retrieve_buffer.clear(0, retrieve_buffer.getNumSamples());
        const int actual_retrieved_num_samples = getNdiEngine().audioCache.pop(retrieve_buffer);

        // Apply fade out...
        // If actual retrieved sample size is less than retrieving buffer size, to reduce the noise with applying gain.
        if (actual_retrieved_num_samples < retrieve_buffer.getNumSamples())
        {
            retrieve_buffer.applyGainRamp(0, actual_retrieved_num_samples, 1.0f, 0.0f);
            isLastRenderedSamplesShorten = true;
        }
        else
        {
            isLastRenderedSamplesShorten = false;
        }

        // Apply fade in...
        if (will_fade_in_this_frame)
        {
            const int fade_sample_length = juce::jmin(256, juce::jmin(actual_retrieved_num_samples, retrieve_buffer.getNumSamples()));
            retrieve_buffer.applyGainRamp(0, fade_sample_length, 0.0f, 1.0f);
        }

        // Processing with re-sample...
        const float actual_ratio_revert = (float)retrieve_buffer.getNumSamples() / (float)resamplingBuffer_NdiToDevice->getNumSamples();

        for (int ch_idx = 0; ch_idx < interPolators_NdiToDevice.size(); ++ch_idx)
        {
            if(ch_idx < retrieve_buffer.getNumChannels())
            {
                interPolators_NdiToDevice.getUnchecked(ch_idx)->process(
                    actual_ratio_revert, retrieve_buffer.getReadPointer(ch_idx),
                    resamplingBuffer_NdiToDevice->getWritePointer(ch_idx), resamplingBuffer_NdiToDevice->getNumSamples()
                );
            }
            else
            {
                juce::AudioBuffer<float> dummy_buffer(1, retrieve_buffer.getNumSamples());
                dummy_buffer.clear();
                interPolators_NdiToDevice.getUnchecked(ch_idx)->process(
                    actual_ratio_revert, dummy_buffer.getReadPointer(0),
                    resamplingBuffer_NdiToDevice->getWritePointer(ch_idx), resamplingBuffer_NdiToDevice->getNumSamples()
                );
            }
        }

        // Copy buffer...
        for(int ch_idx = 0; ch_idx < juce::jmin(buffer.getNumChannels(), resamplingBuffer_NdiToDevice->getNumChannels()); ++ch_idx)
        {
            jassert(buffer.getNumSamples() == resamplingBuffer_NdiToDevice->getNumSamples());
            juce::FloatVectorOperations::copy(buffer.getWritePointer(ch_idx), resamplingBuffer_NdiToDevice->getReadPointer(ch_idx), buffer.getNumSamples());
        }
    }
    else
    {
        isLastRenderedSamplesShorten = true;
        buffer.clear(0, buffer.getNumSamples());
    }
}

//==============================================================================
bool NdiReceiverAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* NdiReceiverAudioProcessor::createEditor()
{
    return new NdiReceiverAudioProcessorEditor (*this);
}

//==============================================================================
void NdiReceiverAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void NdiReceiverAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NdiReceiverAudioProcessor();
}
