/*
  ==============================================================================

    NdiWrapper.h
    Created: 13 Jun 2020 4:37:33pm
    Author:  Tatsuya Shiozawa

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "RingBuffer.h"

class NdiSendWrapper
{
    //==============================================================================
    class Impl;

    //==============================================================================
    class FrameUpdater : public juce::Thread
    {
    public:
        //==============================================================================
        FrameUpdater(NdiSendWrapper& owner_)
            : juce::Thread("NDI Frame Update Thread")
            , owner(owner_)
        {
            retrieveBuffer.setSize(channel_size, sample_size);

            startThread();
        }

        ~FrameUpdater()
        {
            // Waiting time duration have to be longer than NDI receiver's time out msec.
            stopThread(owner.getTimeOutMsec() + 1000);
        }

        //==============================================================================
        virtual void run() override
        {
            while(!threadShouldExit())
            {
                // Send audio...
                if(owner.audioCache.isReady())
                {
                    retrieveBuffer.clear();
                    const int actual_sample_size = owner.audioCache.pop(retrieveBuffer);

                    NdiFrame frame;
                    frame.type = NdiFrameType::kAudio;
                    frame.audio.sample_rate = owner.audioCache.sampleRate;
                    frame.audio.no_channels = owner.audioCache.numChannels;
                    frame.audio.no_samples = actual_sample_size;
                    frame.audio.p_data = NULL;
                    frame.audio.p_metadata = NULL;

                    frame.audio.samples = retrieveBuffer;

                    owner.sendFrame(frame);
                }
                
                // Send video...
                if (owner.videoCache.isReady())
                {

                    NdiFrame frame;
                    frame.type = NdiFrameType::kVideo;

                    owner.sendFrame(frame);
                }
            }

            DBG("Thread exited!!");
        }

    private:
        //==============================================================================
        NdiSendWrapper& owner;
        int interval{ 30 };

        const int channel_size = 16;
        const int sample_size = 1U << 11;
        juce::AudioBuffer<float> retrieveBuffer;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrameUpdater)
    };

public:
    //==============================================================================
    struct NdiSource
    {
        const juce::String NdiName;
        const juce::String UrlAddress;
        const juce::String IpAddress;

        JUCE_LEAK_DETECTOR(NdiSource)
    };

    enum NdiFrameType
    {
        kNone,
        kVideo,
        kAudio
    };

    struct NdiVideoFrame
    {
        int xres, yres;
        int frame_rate_N, frame_rate_D;
        float picture_aspect_ratio;
        int64_t timecode;
        int line_stride_in_bytes;
        int data_size_in_bytes;
        juce::Array<char> p_metadata;
        int64_t timestamp;

        juce::Image image;

        JUCE_LEAK_DETECTOR(NdiVideoFrame)
    };

    struct NdiAudioFrame
    {
        int sample_rate;
        int no_channels;
        int no_samples;
        int64_t timecode;
        float* p_data;
        int channel_stride_in_bytes;
        const char* p_metadata;
        int64_t timestamp;

        juce::AudioBuffer<float> samples;

        JUCE_LEAK_DETECTOR(NdiAudioFrame)
    };

    struct NdiFrame
    {
        NdiFrameType type;
        NdiAudioFrame audio;
        NdiVideoFrame video;

        JUCE_LEAK_DETECTOR(NdiFrame)
    };

    //==============================================================================
    NdiSendWrapper();
    ~NdiSendWrapper();

    //==============================================================================
    void startSend();
    void stopSend();
    bool isSending() const;
    void sendFrame(NdiFrame& frame) const;
    int getTimeOutMsec();

    //==============================================================================
    AudioRingBuffer<float> audioCache;
    VideoRingBuffer videoCache;

private:
    //==============================================================================
    std::unique_ptr<Impl> pImpl;
    std::unique_ptr<FrameUpdater> frameUpdater;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NdiSendWrapper)
};