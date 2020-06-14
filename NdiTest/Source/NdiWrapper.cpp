/*
  ==============================================================================

    NdiWrapper.cpp
    Created: 13 Jun 2020 4:37:33pm
    Author:  Tatsuya Shiozawa

  ==============================================================================
*/

#include "NdiWrapper.h"
#include <Processing.NDI.Lib.h>

class NdiWrapper::Impl
{
public:
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

	static void YUVfromRGB(double& Y, double& U, double& V, const double R, const double G, const double B)
	{
		Y = 0.257 * R + 0.504 * G + 0.098 * B + 16;
		U = -0.148 * R - 0.291 * G + 0.439 * B + 128;
		V = 0.439 * R - 0.368 * G - 0.071 * B + 128;
	}

	static void RGBfromYUV(double& R, double& G, double& B, double Y, double U, double V)
	{
		Y -= 16;
		U -= 128;
		V -= 128;
		R = 1.164 * Y + 1.596 * V;
		G = 1.164 * Y - 0.392 * U - 0.813 * V;
		B = 1.164 * Y + 2.017 * U;
	}

	static juce::Colour GetColourFromYCbCr(int y, int cb, int cr, int a)
	{
		double Y = (double)y;
		double Cb = (double)cb;
		double Cr = (double)cr;

		int r = (int)(Y + 1.40200 * (Cr - 0x80));
		int g = (int)(Y - 0.34414 * (Cb - 0x80) - 0.71414 * (Cr - 0x80));
		int b = (int)(Y + 1.77200 * (Cb - 0x80));

		r = juce::jmax(0, juce::jmin(255, r));
		g = juce::jmax(0, juce::jmin(255, g));
		b = juce::jmax(0, juce::jmin(255, b));

		return juce::Colour::fromRGBA(r, g, b, a);
	}

	static juce::Colour GetColourFromYUV(int y, int u, int v, int a)
	{
		double Y = (double)y;
		double U = (double)u;
		double V = (double)v;

		int r = (int)(1.164 * (Y - 16) + 1.596 * (V - 128));
		int g = (int)(1.164 * (Y - 16) - 0.813 * (V - 128) - 0.391 * (U - 128));
		int b = (int)(1.164 * (Y - 16) + 2.018 * (U - 128));

		r = juce::jmax(0, juce::jmin(255, r));
		g = juce::jmax(0, juce::jmin(255, g));
		b = juce::jmax(0, juce::jmin(255, b));

		return juce::Colour::fromRGBA(r, g, b, a);
	}

	static void convertVideoFrame(NdiVideoFrame& videoFrame, NDIlib_video_frame_v2_t& srcFrame)
	{
		videoFrame.xres = srcFrame.xres;
		videoFrame.yres = srcFrame.yres;
		videoFrame.timecode = srcFrame.timecode;
		videoFrame.timestamp = srcFrame.timestamp;

		juce::Image image(juce::Image::PixelFormat::ARGB, srcFrame.xres, srcFrame.yres, true);

		switch (srcFrame.FourCC)
		{
		case NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_RGBA:
		    {
			    for (int y_idx = 0; y_idx < srcFrame.yres; ++y_idx)
			    {
				    for (int x_idx = 0; x_idx < srcFrame.xres; ++x_idx)
				    {
						const int pix_idx = x_idx + y_idx * srcFrame.xres;
					    const int fourcc_idx = pix_idx * 4;
					    juce::Colour col = juce::Colour::fromRGBA(srcFrame.p_data[fourcc_idx + 0], srcFrame.p_data[fourcc_idx + 1], srcFrame.p_data[fourcc_idx + 2], srcFrame.p_data[fourcc_idx + 3]);
					    image.setPixelAt(x_idx, y_idx, col);
				    }
			    }
		    }
		    break;
		case NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_RGBX:
		    {
			    for (int y_idx = 0; y_idx < srcFrame.yres; ++y_idx)
			    {
				    for (int x_idx = 0; x_idx < srcFrame.xres; ++x_idx)
				    {
					    const int pix_idx = x_idx + y_idx * srcFrame.xres;
						const int fourcc_idx = pix_idx * 3;
					    juce::Colour col = juce::Colour::fromRGBA(srcFrame.p_data[fourcc_idx + 0], srcFrame.p_data[fourcc_idx + 1], srcFrame.p_data[fourcc_idx + 2], 255);
					    image.setPixelAt(x_idx, y_idx, col);
				    }
			    }
		    }
		    break;
		case NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_BGRA:
		    {
			    for (int y_idx = 0; y_idx < srcFrame.yres; ++y_idx)
			    {
				    for (int x_idx = 0; x_idx < srcFrame.xres; ++x_idx)
				    {
						const int pix_idx = x_idx + y_idx * srcFrame.xres;
						const int fourcc_idx = pix_idx * 4;
					    juce::Colour col = juce::Colour::fromRGBA(srcFrame.p_data[fourcc_idx + 2], srcFrame.p_data[fourcc_idx + 1], srcFrame.p_data[fourcc_idx + 0], srcFrame.p_data[fourcc_idx + 3]);
					    image.setPixelAt(x_idx, y_idx, col);
				    }
			    }
		    }
		    break;
		case NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_BGRX:
		    {
			    for (int y_idx = 0; y_idx < srcFrame.yres; ++y_idx)
			    {
				    for (int x_idx = 0; x_idx < srcFrame.xres; ++x_idx)
				    {
						const int pix_idx = x_idx + y_idx * srcFrame.xres;
						const int fourcc_idx = pix_idx * 3;
					    juce::Colour col = juce::Colour::fromRGBA(srcFrame.p_data[fourcc_idx + 2], srcFrame.p_data[fourcc_idx + 1], srcFrame.p_data[fourcc_idx + 0], 255);
					    image.setPixelAt(x_idx, y_idx, col);
				    }
			    }
		    }
		    break;
		case NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_UYVY:
		    {
			    for (int y_idx = 0; y_idx < srcFrame.yres; ++y_idx)
			    {
				    for (int x_idx = 0; x_idx < srcFrame.xres; x_idx+=2)
				    {
						const int pix_idx = x_idx + y_idx * srcFrame.xres;
						const int fourcc_idx = pix_idx * 2;
						const int u0 = srcFrame.p_data[fourcc_idx + 0];
						const int y0 = srcFrame.p_data[fourcc_idx + 1];
						const int v0 = srcFrame.p_data[fourcc_idx + 2];
						const int y1 = srcFrame.p_data[fourcc_idx + 3];
						juce::Colour col0 = GetColourFromYUV(y0, u0, v0, 255);
						image.setPixelAt(x_idx, y_idx, col0);

						juce::Colour col1 = GetColourFromYUV(y1, u0, v0, 255);
						image.setPixelAt(x_idx + 1, y_idx, col1);
				    }
			    }
		    }
		    break;
		case NDIlib_FourCC_video_type_e::NDIlib_FourCC_video_type_UYVA:
		    {
			    for (int y_idx = 0; y_idx < srcFrame.yres; ++y_idx)
			    {
				    for (int x_idx = 0; x_idx < srcFrame.xres; ++x_idx)
				    {
						const int pix_idx = x_idx + y_idx * srcFrame.xres;
						const int fourcc_idx = pix_idx * 3;
					    const int y = srcFrame.p_data[fourcc_idx + 0];
					    const int cb = srcFrame.p_data[fourcc_idx + 1] & 0xf0 >> 4;
					    const int cr = srcFrame.p_data[fourcc_idx + 1] & 0x0f;
					    const int a = srcFrame.p_data[fourcc_idx + 2];
					    juce::Colour col = GetColourFromYCbCr(y, cb, cr, a);
					    image.setPixelAt(x_idx, y_idx, col);
				    }
			    }
		    }
		    break;
		default:
			break;
		}

		videoFrame.image = image;
	}


	NdiWrapper::NdiFrame getFrame()
    {
		NdiWrapper::NdiFrame result_frame;

		// The descriptors
		NDIlib_video_frame_v2_t video_frame;
		NDIlib_audio_frame_v2_t audio_frame;

		switch (NDIlib_recv_capture_v2(pNdiReciever, &video_frame, &audio_frame, nullptr, timeOutMsec))
		{	// No data
		case NDIlib_frame_type_e::NDIlib_frame_type_none:
			//DBG("No data received.");
			result_frame.type = NdiFrameType::kNone;
			break;

			// Video data
		case NDIlib_frame_type_e::NDIlib_frame_type_video:
			//DBG("Video data received (" << video_frame.xres << "x" << video_frame.yres <<" ).");
			result_frame.type = NdiFrameType::kVideo;
			convertVideoFrame(result_frame.video, video_frame);

			NDIlib_recv_free_video_v2(pNdiReciever, &video_frame);
			break;

			// Audio data
		case NDIlib_frame_type_e::NDIlib_frame_type_audio:
			//DBG("Audio data received (" << audio_frame.no_samples <<" samples).");
			result_frame.type = NdiFrameType::kAudio;

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

