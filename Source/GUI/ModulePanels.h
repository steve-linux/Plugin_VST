#pragma once

#include "Widgets.h"

class ChannelStripProcessor;

/** Common frame for one processing module row. */
class ModulePanel : public juce::Component
{
public:
    static constexpr int leftWidth = 236;
    static constexpr int knobW = 62;
    static constexpr int knobX = 250;

    ModulePanel (ChannelStripProcessor& p, const juce::String& titleText, juce::Colour c);
    void paint (juce::Graphics&) override;
    virtual void updateMeters() {}

protected:
    void placeKnobs (std::initializer_list<Knob*> knobs, int x, int gap = 0);
    void addOnButton (const char* paramId);

    ChannelStripProcessor& proc;
    APVTS& state;
    juce::String title;
    juce::Colour colour;
    std::unique_ptr<FlatToggle> onButton;
    std::vector<std::unique_ptr<Knob>> knobs;

    Knob* knob (const char* id, const juce::String& name, bool bipolar = false);
};

//==============================================================================
class InputPanel : public ModulePanel
{
public:
    explicit InputPanel (ChannelStripProcessor&);
    void resized() override;
    void paint (juce::Graphics&) override;
    void updateMeters() override;

private:
    FlatToggle hpfButton { "HPF ON", "HPF OFF", Theme::input };
    juce::ComboBox slope;
    std::unique_ptr<APVTS::ComboBoxAttachment> slopeAtt;
    LedMeter inMeter { { -48, -36, -24, -18, -12, -6, -1 },
                       { Theme::ledGreen, Theme::ledGreen, Theme::ledGreen, Theme::ledGreen, Theme::ledAmber, Theme::ledAmber, Theme::ledRed } };
    float peakHold = -120.0f;
};

//==============================================================================
class PreampPanel : public ModulePanel
{
public:
    explicit PreampPanel (ChannelStripProcessor&);
    void resized() override;
    void paint (juce::Graphics&) override;
    void updateMeters() override;

private:
    ModelSelector model;
    LedMeter satMeter { { 5, 20, 35, 50, 65, 80 }, { Theme::preamp } };
};

//==============================================================================
class GatePanel : public ModulePanel
{
public:
    explicit GatePanel (ChannelStripProcessor&);
    void resized() override;
    void paint (juce::Graphics&) override;
    void updateMeters() override;

private:
    LedMeter openLed { { 0.5f }, { Theme::gate } };
    LedMeter grMeter { { 1, 3, 6, 10, 15, 20, 30 }, { Theme::gate } };
    TransferDisplay transfer { Theme::gate };
    GrHistory history { Theme::gate };
};

//==============================================================================
class EqPanel : public ModulePanel
{
public:
    explicit EqPanel (ChannelStripProcessor&);
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    juce::ComboBox position;
    std::unique_ptr<APVTS::ComboBoxAttachment> positionAtt;
};

//==============================================================================
class CompPanel : public ModulePanel
{
public:
    explicit CompPanel (ChannelStripProcessor&);
    void resized() override;
    void paint (juce::Graphics&) override;
    void updateMeters() override;

private:
    ModelSelector model;
    FlatToggle autoRelease { "AUTO RELEASE", "AUTO RELEASE", Theme::comp };
    FlatToggle autoMakeup  { "AUTO MAKEUP",  "AUTO MAKEUP",  Theme::comp };
    LedMeter grMeter { { 0.5f, 1, 2, 3, 4, 6, 8, 10, 12, 15 },
                       { Theme::comp } };
    TransferDisplay transfer { Theme::comp };
    GrHistory history { Theme::comp };
};

//==============================================================================
class LimiterPanel : public ModulePanel
{
public:
    explicit LimiterPanel (ChannelStripProcessor&);
    void resized() override;
    void paint (juce::Graphics&) override;
    void updateMeters() override;

private:
    LedMeter grMeter { { 0.2f, 0.5f, 1, 2, 3, 4, 6, 9 }, { Theme::limiter } };
    ModelSelector mode;
    FlatButton resetButton { "RESET INTEGRATED" };
    LedMeter targetMeter { { -12, -6, -3, -1, 1, 3, 6 },
                           { Theme::ledGreen, Theme::ledGreen, Theme::ledGreen, Theme::ledGreen, Theme::ledAmber, Theme::ledRed, Theme::ledRed } };
    float momentary = -100, shortTerm = -100, integrated = -100, inputM = -100;
};
