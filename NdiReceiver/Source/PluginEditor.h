/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class NdiReceiverAudioProcessorEditor  : public juce::AudioProcessorEditor
                                        , public juce::Timer
{
public:
    NdiReceiverAudioProcessorEditor (NdiReceiverAudioProcessor&);
    ~NdiReceiverAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    virtual void timerCallback() override;

    //==============================================================================
    NdiReceiverAudioProcessor& audioProcessor;

    juce::Array<NdiWrapper::NdiSource> ndiSources;

    juce::TextButton ndiFindButton;
    juce::ComboBox ndiSourceList;
    juce::TextButton ndiConnectButton;
    juce::TextButton ndiDisconnectButton;

    juce::ThreadPool threadPool;

    juce::Image currentImage;
    int timeupCounter{ 0 };

#ifdef JUCE_OPENGL
    juce::OpenGLContext openGLContext;
#endif //  JUCE_OPENGL

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NdiReceiverAudioProcessorEditor)
};
