/*
  ==============================================================================

    NdiWrapper.h
    Created: 13 Jun 2020 4:37:33pm
    Author:  Tatsuya Shiozawa

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class NdiWrapper
{
    //==============================================================================
    class Impl;

    //==============================================================================
    class FrameUpdater : public juce::Thread
    {
    public:
        //==============================================================================
        FrameUpdater(NdiWrapper& owner_)
            : juce::Thread("NDI Frame Update Thread")
            , owner(owner_)
        {
            startThread();
        }

        ~FrameUpdater()
        {
            // Waiting time duration have to be longer than NDI receiver's time out msec.
            stopThread(owner.getTimeOutMsec());
        }

        //==============================================================================
        virtual void run() override
        {
            while(!threadShouldExit())
            {
                auto frame = owner.getFrame();
                if(frame.type == NdiFrameType::kVideo)
                {
                    owner.setCurrentVideoFrame(frame);
                }
                else if (frame.type == NdiFrameType::kAudio)
                {
                    owner.setCurrentAudioFrame(frame);
                }
            }

            DBG("Thread exited!!");
        }

    private:
        //==============================================================================
        NdiWrapper& owner;
        int interval{ 30 };

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
    };

    struct NdiFrame
    {
        NdiFrameType type;
        NdiAudioFrame audio;
        NdiVideoFrame video;
    };

    //==============================================================================
    NdiWrapper();
    ~NdiWrapper();

    //==============================================================================
    juce::Array<NdiSource> find() const;
    void connect(int sourceIndex);
    void disconnect();
    void startReceive();
    void stopReceive();
    bool isReceiving() const;
    NdiFrame getFrame();
    int getTimeOutMsec();

    //==============================================================================
    NdiFrame& getCurrentVideoFrame() { return currentVideoFrame; }
    void setCurrentVideoFrame(NdiFrame& frame) { currentVideoFrame = frame; }

    NdiFrame& getCurrentAudioFrame() { return currentAudioFrame; }
    void setCurrentAudioFrame(NdiFrame& frame) { currentAudioFrame = frame; }

private:
    //==============================================================================
    std::unique_ptr<Impl> pImpl;
    std::unique_ptr<FrameUpdater> frameUpdater;

    NdiFrame currentVideoFrame;
    NdiFrame currentAudioFrame;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NdiWrapper)
};