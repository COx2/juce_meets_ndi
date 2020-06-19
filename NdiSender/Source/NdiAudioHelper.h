/*
  ==============================================================================

    NdiAudioHelper.h
    Created: 18 Jun 2020 1:13:59am
    Author:  Tatsuya Shiozawa

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <Processing.NDI.Lib.h>
#include "NdiSendWrapper.h"

class NdiAudioHelper
{
public:
    static void convertAudioFrame(NdiSendWrapper::NdiAudioFrame& audioFrame, NDIlib_audio_frame_v2_t& srcFrame)
    {
        audioFrame.sample_rate = srcFrame.sample_rate;
        audioFrame.no_channels = srcFrame.no_channels;
        audioFrame.no_samples = srcFrame.no_samples;
        audioFrame.timecode = srcFrame.timecode;
        audioFrame.timestamp = srcFrame.timestamp;
        audioFrame.channel_stride_in_bytes = srcFrame.channel_stride_in_bytes;

        audioFrame.samples.setSize(audioFrame.no_channels, audioFrame.no_samples);
        for (int ch_idx = 0; ch_idx < audioFrame.samples.getNumChannels(); ++ch_idx)
        {
            juce::FloatVectorOperations::copy(audioFrame.samples.getWritePointer(ch_idx)
                , (float*)(srcFrame.p_data) + ch_idx * audioFrame.no_samples
                , audioFrame.samples.getNumSamples());
        }
    }
};
