/*
  ==============================================================================

    NdiWrapper.cpp
    Created: 13 Jun 2020 4:37:33pm
    Author:  Tatsuya Shiozawa

  ==============================================================================
*/

#include "NdiWrapper.h"
#include <Processing.NDI.Lib.h>
#include "NdiVideoHelper.h"

//==============================================================================
class NdiWrapper::Impl
{
public:
    //==============================================================================
    Impl()
    {
        // Not required, but "correct" (see the SDK documentation.
        if (!NDIlib_initialize()) return;

        // Create a finder
        pNdiFinder = NDIlib_find_create_v2();
        if (!pNdiFinder) return;

        // We now have at least one source, so we create a receiver to look at it.
        pNdiReciever = NDIlib_recv_create_v3();
        if (!pNdiReciever) return;
    }

    ~Impl()
    {
        // Destroy the receiver
        NDIlib_recv_destroy(pNdiReciever);

        // Destroy the NDI finder. We needed to have access to the pointers to p_sources[0]
        NDIlib_find_destroy(pNdiFinder);

        // Not required, but nice
        NDIlib_destroy();
    }

    //==============================================================================
    juce::Array<NdiWrapper::NdiSource> find()
    {
        juce::Array<NdiWrapper::NdiSource> sources{};

        // Wait until there is one source
        uint32_t num_sources = 0;
        DBG("Looking for sources ...\n");
        NDIlib_find_wait_for_sources(pNdiFinder, 1000/* One second */);
        pNdiSources = NDIlib_find_get_current_sources(pNdiFinder, &num_sources);

        if (!num_sources)
        {
            return sources;
        }

        for(int src_idx = 0; src_idx < num_sources; ++src_idx)
        {
            NdiWrapper::NdiSource s { pNdiSources[src_idx].p_ndi_name, pNdiSources[src_idx].p_url_address, pNdiSources[src_idx].p_ip_address };
            sources.add(s);
        }

        return sources;
    }

    void connect(int sourceIndex) const
    {
        // Connect to our sources
        NDIlib_recv_connect(pNdiReciever, pNdiSources + sourceIndex);
    }

    void disconnect() const
    {
        // Disconnect with NULL source
        NDIlib_recv_connect(pNdiReciever, NULL);
    }

    static void convertAudioFrame(NdiWrapper::NdiAudioFrame& audioFrame, NDIlib_audio_frame_v2_t& srcFrame)
    {
        audioFrame.sample_rate = srcFrame.sample_rate;
        audioFrame.no_channels = srcFrame.no_channels;
        audioFrame.no_samples = srcFrame.no_samples;
        audioFrame.timecode = srcFrame.timecode;
        audioFrame.timestamp = srcFrame.timestamp;
        audioFrame.channel_stride_in_bytes = srcFrame.channel_stride_in_bytes;

        audioFrame.samples.setSize(audioFrame.no_channels, audioFrame.no_samples);
        for(int ch_idx = 0; ch_idx < audioFrame.samples.getNumChannels(); ++ch_idx)
        {
            juce::FloatVectorOperations::copy(audioFrame.samples.getWritePointer(ch_idx)
                , (float*)(srcFrame.p_data + ch_idx * audioFrame.channel_stride_in_bytes)
                , audioFrame.samples.getNumSamples());
        }
    }

    NdiWrapper::NdiFrame getFrame()
    {
        //const juce::ScopedLock frame_lock(lock);

        NdiWrapper::NdiFrame result_frame;

        // The descriptors
        NDIlib_video_frame_v2_t video_frame;
        NDIlib_audio_frame_v2_t audio_frame;

        switch (NDIlib_recv_capture_v2(pNdiReciever, &video_frame, &audio_frame, nullptr, timeOutMsec))
        {   // No data
        case NDIlib_frame_type_e::NDIlib_frame_type_none:
            //DBG("No data received.");
            result_frame.type = NdiFrameType::kNone;
            break;

            // Video data
        case NDIlib_frame_type_e::NDIlib_frame_type_video:
            //DBG("Video data received (" << video_frame.xres << "x" << video_frame.yres <<" ).");
            result_frame.type = NdiFrameType::kVideo;
            NdiVideoHelper::convertVideoFrame(result_frame.video, video_frame);
            NDIlib_recv_free_video_v2(pNdiReciever, &video_frame);
            break;

            // Audio data
        case NDIlib_frame_type_e::NDIlib_frame_type_audio:
            DBG("Audio data received (" << audio_frame.no_samples <<" samples).");
            result_frame.type = NdiFrameType::kAudio;
            convertAudioFrame(result_frame.audio, audio_frame);
            NDIlib_recv_free_audio_v2(pNdiReciever, &audio_frame);
            break;

        case NDIlib_frame_type_e::NDIlib_frame_type_error:
        case NDIlib_frame_type_e::NDIlib_frame_type_metadata:
        case NDIlib_frame_type_e::NDIlib_frame_type_status_change:
        case NDIlib_frame_type_e::NDIlib_frame_type_max:
            break;
        }

        return result_frame;
    }

    int getTimeOutMsec() const
    {
        return timeOutMsec;
    }

private:
    NDIlib_find_instance_t pNdiFinder;
    NDIlib_recv_instance_t pNdiReciever;
    const NDIlib_source_t* pNdiSources;

    juce::CriticalSection lock;

    const int timeOutMsec{ 5000 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Impl)
};

//==============================================================================
NdiWrapper::NdiWrapper()
{
    pImpl = std::make_unique<NdiWrapper::Impl>();
}

NdiWrapper::~NdiWrapper()
{
    pImpl.reset();
}

juce::Array<NdiWrapper::NdiSource> NdiWrapper::find() const
{
    return pImpl->find();
}

void NdiWrapper::connect(int sourceIndex)
{
    return pImpl->connect(sourceIndex);
}

void NdiWrapper::disconnect()
{
    return pImpl->disconnect();
}

NdiWrapper::NdiFrame NdiWrapper::getFrame()
{
    return pImpl->getFrame();
}

int NdiWrapper::getTimeOutMsec()
{
    return pImpl->getTimeOutMsec();
}

void NdiWrapper::startReceive()
{
    frameUpdater = std::make_unique<FrameUpdater>(*this);
}

void NdiWrapper::stopReceive()
{
    frameUpdater.reset();
}

bool NdiWrapper::isReceiving() const
{
    return frameUpdater.get() != nullptr;
}
