/*
  ==============================================================================

    NdiVideoHelper.h
    Created: 14 Jun 2020 4:15:30pm
    Author:  Tatsuya Shiozawa

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <Processing.NDI.Lib.h>
#include "NdiSendWrapper.h"

class NdiVideoHelper
{
public:
    static void convertFromRGBToYUV(double& Y, double& U, double& V, const double R, const double G, const double B)
    {
        Y = 0.257 * R       + 0.504 * G     + 0.098 * B     + 16;
        U = -0.148 * R      - 0.291 * G     + 0.439 * B     + 128;
        V = 0.439 * R       - 0.368 * G     - 0.071 * B     + 128;
    }

    static void convertFromYUVToRGB(double& R, double& G, double& B, const double Y, const double U, const double V)
    {
        R = 1.164 * (Y - 16)                           + 1.596 * (V - 128);
        G = 1.164 * (Y - 16)    - 0.392 * (U - 128)    - 0.813 * (V - 128);
        B = 1.164 * (Y - 16)    + 2.017 * (U - 128);
    }

    static void convertFromBiRGBToUYVY(double& U, double& Ya, double& V, double& Yb
        , const double Ra, const double Ga, const double Ba
        , const double Rb, const double Gb, const double Bb)
    {
        Ya = ((257.0 * Ra) + (504 * Ga) + (98 * Ba)) / 1000.0 + 16;

        Yb = ((257.0 * Rb) + (504 * Gb) + (98 * Bb)) / 1000.0 + 16;

        U = (((439 * Ba) - (148 * Ra) - (291 * Ga))
            + ((439 * Bb) - (148 * Rb) - (291 * Gb))
            ) / 2.0 / 1000.0 + 128;

        V = (((439 * Ra) - (368 * Ga) - (71 * Ba))
            + ((439 * Rb) - (368 * Gb) - (71 * Bb))
            ) / 2.0 / 1000.0 + 128;
    }

    static juce::Colour getColourFromYCbCr(int y, int cb, int cr, int a)
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

    static juce::Colour getColourFromYUV(int y, int u, int v, int a)
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

    static void convertVideoFrame(NdiSendWrapper::NdiVideoFrame& videoFrame, const NDIlib_video_frame_v2_t& srcFrame)
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
                for (int x_idx = 0; x_idx < srcFrame.xres; x_idx += 2)
                {
                    const int pix_idx = x_idx + y_idx * srcFrame.xres;
                    const int fourcc_idx = pix_idx * 2;
                    const int u0 = srcFrame.p_data[fourcc_idx + 0];
                    const int y0 = srcFrame.p_data[fourcc_idx + 1];
                    const int v0 = srcFrame.p_data[fourcc_idx + 2];
                    const int y1 = srcFrame.p_data[fourcc_idx + 3];
                    juce::Colour col0 = getColourFromYUV(y0, u0, v0, 255);
                    image.setPixelAt(x_idx, y_idx, col0);

                    juce::Colour col1 = getColourFromYUV(y1, u0, v0, 255);
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
                    const int u = srcFrame.p_data[fourcc_idx + 1] & 0xf0 >> 4;
                    const int v = srcFrame.p_data[fourcc_idx + 1] & 0x0f;
                    const int a = srcFrame.p_data[fourcc_idx + 2];
                    juce::Colour col = getColourFromYCbCr(y, u, v, a);
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

    static void convertVideoFrame(NDIlib_video_frame_v2_t& destFrame, const NdiSendWrapper::NdiVideoFrame& videoFrame)
    {
        destFrame.FourCC = NDIlib_FourCC_type_UYVY;
        int color_data_size = 0;
        switch (destFrame.FourCC)
        {
        case NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_RGBA:
        case NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_RGBX:
        case NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_BGRA:
        case NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_BGRX:
            color_data_size = 4;
            break;
        case NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_UYVY:
            color_data_size = 2;
            break;
        case NDIlib_FourCC_video_type_e::NDIlib_FourCC_video_type_UYVA:
            color_data_size = 3;
            break;
        default:
            break;
        }

        destFrame.xres = videoFrame.xres;
        destFrame.yres = videoFrame.yres;
        destFrame.picture_aspect_ratio = (float)videoFrame.xres / (float)videoFrame.yres;

        destFrame.line_stride_in_bytes = destFrame.xres * color_data_size;

        destFrame.p_data = (uint8_t*)malloc(destFrame.xres * destFrame.yres * color_data_size);
        uint8_t* dest_ptr = destFrame.p_data;
        switch (destFrame.FourCC)
        {
        case NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_RGBA:
        case NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_RGBX:
        case NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_BGRA:
        case NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_BGRX:
            for (int y_idx = 0; y_idx < videoFrame.image.getHeight(); ++y_idx)
            {
                for (int x_idx = 0; x_idx < videoFrame.image.getWidth(); ++x_idx)
                {
                    auto col = videoFrame.image.getPixelAt(x_idx, y_idx);
                    *dest_ptr = col.getRed();   dest_ptr++;
                    *dest_ptr = col.getGreen(); dest_ptr++;
                    *dest_ptr = col.getBlue();  dest_ptr++;
                    *dest_ptr = col.getAlpha(); dest_ptr++;
                }
            }
            break;
        case NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_UYVY:
            for (int y_idx = 0; y_idx < videoFrame.image.getHeight(); ++y_idx)
            {
                for (int x_idx = 0; x_idx < videoFrame.image.getWidth(); x_idx += 2)
                {
                    auto col_a = videoFrame.image.getPixelAt(x_idx, y_idx);
                    auto col_b = videoFrame.image.getPixelAt(x_idx + 1, y_idx);
                    double u, ya, v, yb = 0.0;
                    NdiVideoHelper::convertFromBiRGBToUYVY(u, ya, v, yb
                        , col_a.getRed(), col_a.getGreen(), col_a.getBlue()
                        , col_b.getRed(), col_b.getGreen(), col_b.getBlue());
                    *dest_ptr = static_cast<uint8_t>(u);    dest_ptr++;
                    *dest_ptr = static_cast<uint8_t>(ya);   dest_ptr++;
                    *dest_ptr = static_cast<uint8_t>(v);    dest_ptr++;
                    *dest_ptr = static_cast<uint8_t>(yb);   dest_ptr++;
                }
            }
            break;
        case NDIlib_FourCC_video_type_e::NDIlib_FourCC_video_type_UYVA:
            break;
        default:
            break;
        }

        destFrame.frame_format_type = NDIlib_frame_format_type_e::NDIlib_frame_format_type_interleaved;
        destFrame.frame_rate_N = videoFrame.frame_rate_N;
        destFrame.frame_rate_D = videoFrame.frame_rate_D;

        destFrame.timecode = videoFrame.timecode;
        destFrame.timestamp = videoFrame.timestamp;

        destFrame.p_metadata = NULL;
    }
};
