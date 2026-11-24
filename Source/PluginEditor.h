#pragma once

#include "PluginProcessor.h"
#include "GUI/ModulePanels.h"
#include "GUI/Analyzer.h"

/** Fixed-size layout (1200 x 780) scaled uniformly to the window size. */
class ChannelStripContent : public juce::Component
{
public:
    static constexpr int baseWidth = 1200, baseHeight = 780;

    explicit ChannelStripContent (ChannelStripProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void updateMeters();

private:
    void refreshPresetBox();

    ChannelStripProcessor& proc;

    juce::ComboBox presetBox, oversampling;
    std::unique_ptr<APVTS::ComboBoxAttachment> oversamplingAtt;

    InputPanel input;
    PreampPanel preamp;
    GatePanel gate;
    EqPanel eq;
    CompPanel comp;
    LimiterPanel limiter;
    Analyzer analyzer;
};

class ChannelStripEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ChannelStripEditor (ChannelStripProcessor&);
    ~ChannelStripEditor() override;

    void resized() override;

private:
    void timerCallback() override { content.updateMeters(); }

    ChannelStripLookAndFeel lnf;
    ChannelStripContent content;
};
