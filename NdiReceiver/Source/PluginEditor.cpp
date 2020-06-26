/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NdiReceiverAudioProcessorEditor::NdiReceiverAudioProcessorEditor (NdiReceiverAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    ndiFindButton.setButtonText("Find NDI source");
    ndiFindButton.onClick = [&]()
    {
        ndiSourceList.setEnabled(false);

        const std::function<juce::ThreadPoolJob::JobStatus()> findJob = [&]()
        {
            ndiSources = audioProcessor.getNdiEngine().find();

            const std::function<void()> updateList = [&]()
            {
                ndiSourceList.clear(juce::dontSendNotification);

                juce::StringArray items;
                for (int idx = 0; idx < ndiSources.size(); ++idx)
                {
                    auto& source = ndiSources.getReference(idx);
                    items.add(source.NdiName);
                }
                ndiSourceList.addItemList(items, 1);
                ndiSourceList.setEnabled(true);
            };
            juce::MessageManager::getInstance()->callAsync(updateList);

            return juce::ThreadPoolJob::JobStatus::jobHasFinished;
        };
        threadPool.addJob(findJob);
    };
    addAndMakeVisible(ndiFindButton);

    ndiSourceList.onChange = [&]()
    {
    };
    addAndMakeVisible(ndiSourceList);


    ndiConnectButton.setButtonText("Connect");
    ndiConnectButton.onClick = [&]()
    {
        if (ndiSourceList.getSelectedId() > 0)
        {
            auto src_idx = ndiSourceList.getSelectedItemIndex();
            const std::function<juce::ThreadPoolJob::JobStatus()> coonectJob = [&, src_idx]()
            {
                if (audioProcessor.getNdiEngine().isReceiving())
                {
                    audioProcessor.getNdiEngine().stopReceive();
                    audioProcessor.getNdiEngine().disconnect();
                }

                audioProcessor.getNdiEngine().connect(src_idx);
                audioProcessor.getNdiEngine().startReceive();

                return juce::ThreadPoolJob::JobStatus::jobHasFinished;
            };
            threadPool.addJob(coonectJob);
        }
    };
    addAndMakeVisible(ndiConnectButton);


    ndiDisconnectButton.setButtonText("Disconnect");
    ndiDisconnectButton.onClick = [&]()
    {
        const std::function<juce::ThreadPoolJob::JobStatus()> discoonectJob = [&]()
        {
            audioProcessor.getNdiEngine().stopReceive();
            audioProcessor.getNdiEngine().disconnect();

            return juce::ThreadPoolJob::JobStatus::jobHasFinished;
        };
        threadPool.addJob(discoonectJob);
    };
    addAndMakeVisible(ndiDisconnectButton);

    setSize(820, 600);

    startTimerHz(120);

#ifdef JUCE_OPENGL
    openGLContext.attachTo(*getTopLevelComponent());
#endif // JUCE_OPENGL
}

NdiReceiverAudioProcessorEditor::~NdiReceiverAudioProcessorEditor()
{
#ifdef JUCE_OPENGL
    openGLContext.detach();
#endif // JUCE_OPENGL
}

//==============================================================================
void NdiReceiverAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    juce::Rectangle<int> video_area{ 20, 100, 780, 480 };
    g.setColour(juce::Colours::black);
    g.fillRect(video_area);

    if (audioProcessor.getNdiEngine().videoCache.pop(currentImage) > 0)
    {
        timeupCounter = 0;
    }
    else
    {
        timeupCounter++;
    }

    if (timeupCounter > 60)
    {
        currentImage = juce::Image();
    }

    g.drawImage(currentImage,
        video_area.toFloat(),
        juce::RectanglePlacement::Flags::centred);
}

void NdiReceiverAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    ndiFindButton.setBounds(20, 20, 180, 60);
    ndiSourceList.setBounds(220, 20, 180, 60);
    ndiConnectButton.setBounds(420, 20, 180, 60);
    ndiDisconnectButton.setBounds(620, 20, 180, 60);
}

void NdiReceiverAudioProcessorEditor::timerCallback()
{
    repaint();
}
