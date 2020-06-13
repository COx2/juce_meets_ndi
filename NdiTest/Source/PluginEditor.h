/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include "NdiWrapper.h"

//==============================================================================
/**
*/
class NdiTestAudioProcessorEditor  : public juce::AudioProcessorEditor
                                    , public juce::MessageListener
                                    , public juce::Timer
{
public:
    NdiTestAudioProcessorEditor (NdiTestAudioProcessor&);
    ~NdiTestAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    virtual void handleMessage(const juce::Message& message) override;
    virtual void timerCallback() override;

    //==============================================================================
    NdiTestAudioProcessor& audioProcessor;

    NdiWrapper ndiWrapper;
    juce::Array<NdiWrapper::NdiSource> ndiSources;

    juce::TextButton ndiFindButton;
    juce::ComboBox ndiSourceList;
    juce::TextButton ndiConnectButton;
    juce::TextButton ndiDisconnectButton;

    juce::ThreadPool threadPool;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NdiTestAudioProcessorEditor)
};
