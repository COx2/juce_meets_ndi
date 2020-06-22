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

#if JUCE_MAC
#include <dlfcn.h>
#endif

//==============================================================================
class NdiSendWrapper::Impl
{
public:
    //==============================================================================
    Impl()
        : uuid_dashed_str(uuid.toDashedString().toStdString())
    {
#if JUCE_MAC
        std::string ndi_path;
        
        // ToDo
        const char* p_NDI_runtime_folder = std::getenv("NDI_RUNTIME_DIR_V4");
        if (p_NDI_runtime_folder)
        {
            ndi_path = p_NDI_runtime_folder;
            ndi_path += "/libndi.dylib";
        }
        else
        {
            ndi_path = "libndi.4.dylib"; // The standard versioning scheme on Linux based systems using sym links
        }
        
        // Try to load the library
        void* handle_ndi_lib = ::dlopen(ndi_path.c_str(), RTLD_LOCAL | RTLD_LAZY);
        
        // If handle is NULL, Re-try to load the library by absolute path.
        if(!handle_ndi_lib)
        {
            ndi_path = "/usr/local/lib/libndi.4.dylib";
            handle_ndi_lib = ::dlopen(ndi_path.c_str(), RTLD_LOCAL | RTLD_LAZY);
        }
        
        // The main NDI entry point for dynamic loading if we got the library
        const NDIlib_v4* (*funcPtr_NDIlib_v4_load)(void) = NULL;
        if(handle_ndi_lib)
        {
            *((void**)&funcPtr_NDIlib_v4_load) = ::dlsym(handle_ndi_lib, "NDIlib_v4_load");
        }
        
        if (!funcPtr_NDIlib_v4_load)
        {
            DBG("Please re-install the NewTek NDI Runtimes to use this application.");
            return;
        }
        
        // Lets get all of the DLL entry points
        pNdiLib = funcPtr_NDIlib_v4_load();
        
        // We can now run as usual
        if (!pNdiLib->NDIlib_initialize())
        {    // Cannot run NDI. Most likely because the CPU is not sufficient (see SDK documentation).
            // you can check this directly with a call to NDIlib_is_supported_CPU()
            DBG("Cannot run NDI.");
            return;
        }
        
        // Create an NDI source that is called "My Video and Audio" and is clocked to the video.
        ndiSendDesc.p_ndi_name = uuid_dashed_str.c_str();
        ndiSendDesc.clock_audio = true;

        // We create the NDI sender
        pNdiSender = pNdiLib->NDIlib_send_create(&ndiSendDesc);
        if (!pNdiSender) return;
#else
        // Not required, but "correct" (see the SDK documentation.
        if (!NDIlib_initialize()) return;

        // Create an NDI source that is called "My Video and Audio" and is clocked to the video.
        ndiSendDesc.p_ndi_name = uuid_dashed_str.c_str();
        ndiSendDesc.clock_audio = true;

        // We create the NDI sender
        pNdiSender = NDIlib_send_create(&ndiSendDesc);
        if (!pNdiSender) return;
#endif
    }

    ~Impl()
    {
#if JUCE_MAC
        if(pNdiLib)
        {
            // Destroy the NDI sender
            pNdiLib->NDIlib_send_destroy(pNdiSender);

            // Not required, but nice
            pNdiLib->NDIlib_destroy();
        }
#else
        // Destroy the NDI sender
        NDIlib_send_destroy(pNdiSender);

        // Not required, but nice
        NDIlib_destroy();
#endif
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
#if JUCE_MAC
                if(pNdiLib)
                {
                    // Send data
                    pNdiLib->NDIlib_send_send_video_v2(pNdiSender, &NDI_video_frame);
                }
#else
                // Send data
                NDIlib_send_send_video_v2(pNdiSender, &NDI_video_frame);
#endif
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

#if JUCE_MAC
                if(pNdiLib)
                {
                    // Send data
                    pNdiLib->NDIlib_send_send_audio_v2(pNdiSender, &NDI_audio_frame);
                }
#else
                // Send data
                NDIlib_send_send_audio_v2(pNdiSender, &NDI_audio_frame);
#endif
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
    const NDIlib_v4* pNdiLib;
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

