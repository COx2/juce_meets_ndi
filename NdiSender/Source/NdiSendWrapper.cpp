/*
  ==============================================================================

    NdiWrapper.cpp
    Created: 13 Jun 2020 4:37:33pm
    Author:  Tatsuya Shiozawa

  ==============================================================================
*/

#include "NdiSendWrapper.h"
#include <Processing.NDI.Lib.h>
#include "NdiVideoHelper.h"
#include "NdiAudioHelper.h"

//==============================================================================
class NdiSendWrapper::Impl
{
public:
    //==============================================================================
    Impl()
        : uuid_dashed_str(uuid.toDashedString().toStdString())
    {
        // Not required, but "correct" (see the SDK documentation.
        if (!NDIlib_initialize()) return;

        // Create an NDI source that is called "My Video and Audio" and is clocked to the video.
        ndiSendDesc.p_ndi_name = uuid_dashed_str.c_str();
        ndiSendDesc.clock_audio = true;

        // We create the NDI sender
        pNdiSender = NDIlib_send_create(&ndiSendDesc);
        if (!pNdiSender) return;
    }

    ~Impl()
    {
        // Destroy the NDI sender
        NDIlib_send_destroy(pNdiSender);

        // Not required, but nice
        NDIlib_destroy();
    }

    //==============================================================================
    void sendFrame(const NdiSendWrapper::NdiFrame& frame) const
    {
        const juce::ScopedLock frame_lock(lock);

        switch (frame.type)
        {
            // Video data
        case NdiSendWrapper::NdiFrameType::kVideo:
            {
                // We are going to create a 1920x1080 interlaced frame at 29.97Hz.
                NDIlib_video_frame_v2_t NDI_video_frame;
                NDI_video_frame.xres = 1920;
                NDI_video_frame.yres = 1080;
                NDI_video_frame.FourCC = NDIlib_FourCC_type_UYVY;
                NDI_video_frame.p_data = (uint8_t*)malloc(1920 * 1080 * 2);
                NDI_video_frame.line_stride_in_bytes = 1920 * 2;

                NDIlib_send_send_video_v2(pNdiSender, &NDI_video_frame);

                // Free the data
                free((void*)NDI_video_frame.p_data);
            }
            break;

            // Audio data
        case NdiSendWrapper::NdiFrameType::kAudio:
            {
                // Create an audio buffer
                NDIlib_audio_frame_v2_t NDI_audio_frame;
                NDI_audio_frame.sample_rate = frame.audio.sample_rate;
                NDI_audio_frame.no_channels = frame.audio.no_channels;
                NDI_audio_frame.no_samples = frame.audio.no_samples;
                NDI_audio_frame.channel_stride_in_bytes = sizeof(float) * frame.audio.no_samples;
                NDI_audio_frame.p_data = (float*)malloc(sizeof(float) * frame.audio.no_samples * frame.audio.no_channels);
                for (int ch_idx = 0; ch_idx < frame.audio.no_channels; ++ch_idx)
                {
                    juce::FloatVectorOperations::copy(NDI_audio_frame.p_data + ch_idx * frame.audio.no_samples,
                        frame.audio.samples.getReadPointer(ch_idx),
                        frame.audio.no_samples);
                }

                NDI_audio_frame.timecode = frame.audio.timecode;
                NDI_audio_frame.timestamp = frame.audio.timestamp;

                NDI_audio_frame.p_metadata = NULL;

                NDIlib_send_send_audio_v2(pNdiSender, &NDI_audio_frame);

                // Free the data
                free((void*)NDI_audio_frame.p_data);
            }
            break;
            // No data
        case NdiSendWrapper::NdiFrameType::kNone:
            break;
        }
    }

    int getTimeOutMsec() const
    {
        return timeOutMsec;
    }

private:
    NDIlib_find_instance_t pNdiFinder;
    NDIlib_send_instance_t pNdiSender;
    NDIlib_send_create_t ndiSendDesc;

    juce::Uuid uuid;
    std::string uuid_dashed_str;
    juce::CriticalSection lock;

    const int timeOutMsec{ 5000 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Impl)
};

//==============================================================================
NdiSendWrapper::NdiSendWrapper()
{
    pImpl = std::make_unique<NdiSendWrapper::Impl>();
}

NdiSendWrapper::~NdiSendWrapper()
{
    pImpl.reset();
}

void NdiSendWrapper::startSend()
{
    frameUpdater = std::make_unique<FrameUpdater>(*this);
}

void NdiSendWrapper::stopSend()
{
    frameUpdater.reset();
}

bool NdiSendWrapper::isSending() const
{
    return frameUpdater.get() != nullptr;
}

void NdiSendWrapper::sendFrame(NdiFrame& frame) const
{
    pImpl->sendFrame(frame);
}

int NdiSendWrapper::getTimeOutMsec()
{
    return pImpl->getTimeOutMsec();
}

