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
class AudioRingBuffer
{
public:
    static constexpr size_t order = 24;
    static constexpr size_t bufferSize = 1U << order;
    static constexpr int channelSize = 2;

    AudioRingBuffer()
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
                juce::FloatVectorOperations::copy(
                    internalBuffer.getWritePointer(ch_idx) + start1,
                    inputBuffer.getReadPointer(ch_idx),
                    size1);
            }
        }

        if(size2 > 0)
        {
            for (int ch_idx = 0; ch_idx < min_ch_idx; ++ch_idx)
            {
                juce::FloatVectorOperations::copy(
                    internalBuffer.getWritePointer(ch_idx) + start2,
                    inputBuffer.getReadPointer(ch_idx) + size1,
                    size2);
            }
        }

        abstractFifo.finishedWrite(size1 + size2);
    }

    int pop(juce::AudioBuffer<SampleType>& outputBuffer)
    {
        int start1, size1, start2, size2;

        abstractFifo.prepareToRead(outputBuffer.getNumSamples(), start1, size1, start2, size2);

        const int min_ch_idx = juce::jmin(outputBuffer.getNumChannels(), internalBuffer.getNumChannels());

        if (size1 > 0)
        {
            for (int ch_idx = 0; ch_idx < min_ch_idx; ++ch_idx)
            {
                juce::FloatVectorOperations::copy(
                    outputBuffer.getWritePointer(ch_idx),
                    internalBuffer.getReadPointer(ch_idx) + start1,
                    size1);
            }
        }

        if (size2 > 0)
        {
            for (int ch_idx = 0; ch_idx < min_ch_idx; ++ch_idx)
            {
                juce::FloatVectorOperations::copy(
                    outputBuffer.getWritePointer(ch_idx) + size1,
                    internalBuffer.getReadPointer(ch_idx) + start2,
                    size2);
            }
        }

        abstractFifo.finishedRead(size1 + size2);

        return size1 + size2;
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


class VideoRingBuffer
{
public:
    static constexpr size_t bufferSize = 5;

    VideoRingBuffer()
    {
        for(int i = 0; i < bufferSize; ++i)
        {
            imageBuffer.add(juce::Image());
        }
    }

    void push(const juce::Image& input)
    {
        int start1, size1, start2, size2;

        abstractFifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 > 0)
        {
            imageBuffer.getReference(start1) = input;
        }

        if (size2 > 0)
        {
            imageBuffer.getReference(start2) = input;
        }

        abstractFifo.finishedWrite(size1 + size2);
    }

    int pop(juce::Image& output)
    {
        int start1, size1, start2, size2;

        abstractFifo.prepareToRead(1, start1, size1, start2, size2);

        if (size1 > 0)
        {
            output = imageBuffer.getReference(start1);
        }

        if (size2 > 0)
        {
            output = imageBuffer.getReference(start2);
        }

        abstractFifo.finishedRead(size1 + size2);

        return size1 + size2;
    }

    bool isReady() const
    {
        return abstractFifo.getNumReady() != 0;
    }

private:
    juce::Array<juce::Image> imageBuffer;
    juce::AbstractFifo abstractFifo{ bufferSize };
};