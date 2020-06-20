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
                // Create an video buffer
                NDIlib_video_frame_v2_t NDI_video_frame;
                NdiVideoHelper::convertVideoFrame(NDI_video_frame, frame.video);

                // Send data
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
                NdiAudioHelper::convertAudioFrame(NDI_audio_frame, frame.audio);

                // Send data
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

