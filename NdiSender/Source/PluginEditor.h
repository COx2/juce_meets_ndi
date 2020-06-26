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
class NdiSenderAudioProcessorEditor : public juce::AudioProcessorEditor
                                    , juce::Timer
{
public:
    NdiSenderAudioProcessorEditor (NdiSenderAudioProcessor&);
    ~NdiSenderAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    //==============================================================================
    virtual void timerCallback() override;

    //==============================================================================
    void updateCameraList();
    void cameraChanged();
    void cameraDeviceOpenResult(juce::CameraDevice* device, const String& error);
    void takeSnapshot();
    // This is called by the camera device when a new image arrives
    void imageReceived(const Image& image);

private:
    //==============================================================================
    NdiSenderAudioProcessor& audioProcessor;

    std::unique_ptr<juce::CameraDevice> cameraDevice;
    std::unique_ptr<juce::Component> cameraPreviewComp;

    juce::ComboBox cameraSelectorComboBox{ "Camera" };
    juce::TextButton snapshotButton{ "Take a snapshot" };
    juce::Label ndiName;


#ifdef JUCE_OPENGL
    juce::OpenGLContext openGLContext;
#endif //  JUCE_OPENGL

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NdiSenderAudioProcessorEditor)
};
