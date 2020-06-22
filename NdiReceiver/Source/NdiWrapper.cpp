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
#include "NdiAudioHelper.h"

#if JUCE_MAC
#include <dlfcn.h>
#endif

//==============================================================================
class NdiWrapper::Impl
{
public:
    //==============================================================================
    Impl()
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
        if (handle_ndi_lib)
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
        
        // Create a finder
        pNdiFinder = pNdiLib->NDIlib_find_create_v2(NULL);
        if (!pNdiFinder) return;
        
        // We now have at least one source, so we create a receiver to look at it.
        pNdiReceiver = pNdiLib->NDIlib_recv_create_v3(NULL);
        if (!pNdiReceiver) return;
#else
        // Not required, but "correct" (see the SDK documentation.
        if (!NDIlib_initialize()) return;

        // Create a finder
        pNdiFinder = NDIlib_find_create_v2();
        if (!pNdiFinder) return;

        // We now have at least one source, so we create a receiver to look at it.
        pNdiReceiver = NDIlib_recv_create_v3();
        if (!pNdiReceiver) return;
#endif
    }

    ~Impl()
    {
#if JUCE_MAC
        if(pNdiLib)
        {
            // Destroy the receiver
            pNdiLib->NDIlib_recv_destroy(pNdiReceiver);

            // Destroy the NDI finder. We needed to have access to the pointers to p_sources[0]
            pNdiLib->NDIlib_find_destroy(pNdiFinder);

            // Not required, but nice
            pNdiLib->NDIlib_destroy();
        }
#else
        // Destroy the receiver
        NDIlib_recv_destroy(pNdiReceiver);

        // Destroy the NDI finder. We needed to have access to the pointers to p_sources[0]
        NDIlib_find_destroy(pNdiFinder);

        // Not required, but nice
        NDIlib_destroy();
#endif
    }

    //==============================================================================
    juce::Array<NdiWrapper::NdiSource> find()
    {
        juce::Array<NdiWrapper::NdiSource> sources{};

        // Wait until there is one source
        uint32_t num_sources = 0;
        DBG("Looking for sources ...\n");
        
#if JUCE_MAC
        if(pNdiLib) pNdiLib->NDIlib_find_wait_for_sources(pNdiFinder, 1000/* One second */);
        if(pNdiLib) pNdiSources = pNdiLib->NDIlib_find_get_current_sources(pNdiFinder, &num_sources);
#else
        NDIlib_find_wait_for_sources(pNdiFinder, 1000/* One second */);
        pNdiSources = NDIlib_find_get_current_sources(pNdiFinder, &num_sources);
#endif

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
#if JUCE_MAC
        // Connect to our sources
        if(pNdiLib) pNdiLib->NDIlib_recv_connect(pNdiReceiver, pNdiSources + sourceIndex);
#else
        // Connect to our sources
        NDIlib_recv_connect(pNdiReceiver, pNdiSources + sourceIndex);
#endif
    }

    void disconnect() const
    {
#if JUCE_MAC
        // Disconnect with NULL source
        if(pNdiLib) pNdiLib->NDIlib_recv_connect(pNdiReceiver, NULL);
#else
        // Disconnect with NULL source
        NDIlib_recv_connect(pNdiReceiver, NULL);
#endif
    }

    NdiWrapper::NdiFrame getFrame()
    {
        const juce::ScopedLock frame_lock(lock);

        NdiWrapper::NdiFrame result_frame;

        // The descriptors
        NDIlib_video_frame_v2_t video_frame;
        NDIlib_audio_frame_v2_t audio_frame;
        NDIlib_frame_type_e frame_type = NDIlib_frame_type_e::NDIlib_frame_type_none;
#if JUCE_MAC
        if(pNdiLib) frame_type = pNdiLib->NDIlib_recv_capture_v2(pNdiReceiver, &video_frame, &audio_frame, nullptr, timeOutMsec);
#else
        frame_type = NDIlib_recv_capture_v2(pNdiReceiver, &video_frame, &audio_frame, nullptr, timeOutMsec);
#endif
        switch (frame_type)
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
#if JUCE_MAC
            if(pNdiLib) pNdiLib->NDIlib_recv_free_video_v2(pNdiReceiver, &video_frame);
#else
            NDIlib_recv_free_video_v2(pNdiReceiver, &video_frame);
#endif
            break;

            // Audio data
        case NDIlib_frame_type_e::NDIlib_frame_type_audio:
            //DBG("Audio data received (" << audio_frame.no_samples <<" samples).");
            result_frame.type = NdiFrameType::kAudio;
            NdiAudioHelper::convertAudioFrame(result_frame.audio, audio_frame);
#if JUCE_MAC
            if(pNdiLib) pNdiLib->NDIlib_recv_free_audio_v2(pNdiReceiver, &audio_frame);
#else
            NDIlib_recv_free_audio_v2(pNdiReceiver, &audio_frame);
#endif
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
    const NDIlib_v4* pNdiLib;
    NDIlib_find_instance_t pNdiFinder;
    NDIlib_recv_instance_t pNdiReceiver;
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
