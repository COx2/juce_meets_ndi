/*
  ==============================================================================

    RingBuffer.h
    Created: 14 Jun 2020 5:15:35pm
    Author:  Tatsuya Shiozawa

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

template <typename SampleType>
class RingBuffer
{
public:
    static constexpr size_t order = 24;
    static constexpr size_t bufferSize = 1U << order;
    static constexpr int channelSize = 2;

    RingBuffer()
        : sampleRate(0), numChannels(0)
    {
        internalBuffer.setSize(channelSize, bufferSize);
    }

    void push(const juce::AudioBuffer<SampleType>& inputBuffer)
    {
        int start1, size1, start2, size2;

        abstractFifo.prepareToWrite(inputBuffer.getNumSamples(), start1, size1, start2, size2);

        const int min_ch_idx = juce::jmin(inputBuffer.getNumChannels(), internalBuffer.getNumChannels());

        if (size1 > 0)
        {
            for(int ch_idx = 0; ch_idx < min_ch_idx; ++ch_idx)
            {
                juce::FloatVectorOperations::copy(internalBuffer.getWritePointer(ch_idx) + start1
                    , inputBuffer.getReadPointer(ch_idx)
                    , size1);
            }
        }

        if(size2 > 0)
        {
            for (int ch_idx = 0; ch_idx < min_ch_idx; ++ch_idx)
            {
                juce::FloatVectorOperations::copy(internalBuffer.getWritePointer(ch_idx) + start2
                    , inputBuffer.getReadPointer(ch_idx)
                    , size2);
            }
        }

        abstractFifo.finishedWrite(size1 + size2);
    }

    void pop(juce::AudioBuffer<SampleType>& outputBuffer)
    {
        int start1, size1, start2, size2;

        abstractFifo.prepareToRead(outputBuffer.getNumSamples(), start1, size1, start2, size2);

        const int min_ch_idx = juce::jmin(outputBuffer.getNumChannels(), internalBuffer.getNumChannels());

        if (size1 > 0)
        {
            for (int ch_idx = 0; ch_idx < min_ch_idx; ++ch_idx)
            {
                juce::FloatVectorOperations::copy(outputBuffer.getWritePointer(ch_idx)
                    , internalBuffer.getReadPointer(ch_idx) + start1
                    , size1);
            }
        }

        if (size2 > 0)
        {
            for (int ch_idx = 0; ch_idx < min_ch_idx; ++ch_idx)
            {
                juce::FloatVectorOperations::copy(outputBuffer.getWritePointer(ch_idx)
                    , internalBuffer.getReadPointer(ch_idx) + start2
                    , size2);
            }
        }

        abstractFifo.finishedRead(size1 + size2);
    }

    bool isReady() const
    {
        return abstractFifo.getNumReady() != 0;
    }

    int sampleRate;
    int numChannels;

private:
    juce::AudioBuffer<SampleType> internalBuffer;
    juce::AbstractFifo abstractFifo{ bufferSize };

};
