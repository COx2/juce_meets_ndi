/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NdiSenderAudioProcessorEditor::NdiSenderAudioProcessorEditor (NdiSenderAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    addAndMakeVisible(cameraSelectorComboBox);
    updateCameraList();
    cameraSelectorComboBox.setSelectedId(1);
    cameraSelectorComboBox.onChange = [this]
    {
        cameraChanged();
    };

    setSize (820, 600);

    startTimerHz(120);

#ifdef JUCE_OPENGL
    openGLContext.attachTo(*getTopLevelComponent());
#endif // JUCE_OPENGL
}

NdiSenderAudioProcessorEditor::~NdiSenderAudioProcessorEditor()
{
#ifdef JUCE_OPENGL
    openGLContext.detach();
#endif // JUCE_OPENGL
}

//==============================================================================
void NdiSenderAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    const auto video_area = juce::Rectangle<int>(20, 100, 780, 480);
    g.setColour(juce::Colours::black);
    g.fillRect(video_area);
}

void NdiSenderAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(5);

    auto top = area.removeFromTop(25);
    cameraSelectorComboBox.setBounds(top.removeFromLeft(250));

    area.removeFromTop(4);
    top = area.removeFromTop(25);

    snapshotButton.changeWidthToFitText(24);
    snapshotButton.setBounds(top.removeFromLeft(snapshotButton.getWidth()));
    top.removeFromLeft(4);

    area.removeFromTop(4);
    const auto previewArea = juce::Rectangle<int>(20, 100, 780, 480);

    if (cameraPreviewComp.get() != nullptr)
        cameraPreviewComp->setBounds(previewArea);
}

void NdiSenderAudioProcessorEditor::timerCallback()
{
    takeSnapshot();
}

void NdiSenderAudioProcessorEditor::updateCameraList()
{
    cameraSelectorComboBox.clear();
    cameraSelectorComboBox.addItem("No camera", 1);
    cameraSelectorComboBox.addSeparator();

    auto camera_list = juce::CameraDevice::getAvailableDevices();

    for (int i = 0; i < camera_list.size(); ++i)
    {
        cameraSelectorComboBox.addItem(camera_list[i], i + 2);
    }
}

void NdiSenderAudioProcessorEditor::cameraChanged()
{
    cameraDevice.reset();
    cameraPreviewComp.reset();

    if (cameraSelectorComboBox.getSelectedId() > 1)
    {
        cameraDeviceOpenResult(CameraDevice::openDevice(cameraSelectorComboBox.getSelectedId() - 2), {});
    }
    else
    {
        resized();
    }
}

void NdiSenderAudioProcessorEditor::cameraDeviceOpenResult(juce::CameraDevice* device, const String& error)
{
    cameraDevice.reset(device);

    if(cameraDevice.get() != nullptr)
    {
        cameraPreviewComp.reset(cameraDevice->createViewerComponent());
        addAndMakeVisible(cameraPreviewComp.get());
    }
    else
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Camera open failed","Camera open failed, reason: " + error);
    }

    resized();
}

void NdiSenderAudioProcessorEditor::takeSnapshot()
{
    if (cameraDevice.get() != nullptr)
    {
        SafePointer<NdiSenderAudioProcessorEditor> safeThis(this);
        cameraDevice->takeStillPicture([safeThis](const Image& image) mutable
            {
                safeThis->imageReceived(image);
            });
    }
}

void NdiSenderAudioProcessorEditor::imageReceived(const Image& image)
{
    if (!image.isValid())
        return;

    audioProcessor.getNdiEngine().videoCache.push(image);
}
