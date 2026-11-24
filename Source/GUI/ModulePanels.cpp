#include "ModulePanels.h"
#include "../PluginProcessor.h"

namespace
{
    void setupCombo (juce::ComboBox& box, APVTS& state, const char* id,
                     std::unique_ptr<APVTS::ComboBoxAttachment>& att)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (id)))
            box.addItemList (p->choices, 1);
        att = std::make_unique<APVTS::ComboBoxAttachment> (state, id, box);
    }

    juce::String lufsText (float v) { return v <= -99.0f ? juce::String ("-inf") : juce::String (v, 1); }
}

//==============================================================================
ModulePanel::ModulePanel (ChannelStripProcessor& p, const juce::String& titleText, juce::Colour c)
    : proc (p), state (p.apvts), title (titleText), colour (c)
{
}

Knob* ModulePanel::knob (const char* id, const juce::String& name, bool bipolar)
{
    knobs.push_back (std::make_unique<Knob> (state, id, name, colour, bipolar));
    addAndMakeVisible (*knobs.back());
    return knobs.back().get();
}

void ModulePanel::addOnButton (const char* paramId)
{
    onButton = std::make_unique<FlatToggle> ("ON", "OFF", colour);
    onButton->attach (state, paramId);
    addAndMakeVisible (*onButton);
}

void ModulePanel::placeKnobs (std::initializer_list<Knob*> list, int x, int gap)
{
    for (auto* k : list)
    {
        k->setBounds (x, 4, knobW, 80);
        x += knobW + gap;
    }
}

void ModulePanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    g.setColour (Theme::panel);
    g.fillRect (r);
    g.setColour (Theme::panelLeft);
    g.fillRect (r.withWidth (leftWidth));
    g.setColour (Theme::border);
    g.drawRect (r, 1);
    g.drawVerticalLine (leftWidth, 0.0f, (float) getHeight());

    g.setColour (colour);
    g.fillRect (0, 0, 4, getHeight());

    g.setColour (Theme::text);
    g.setFont (Theme::font (15.0f, true));
    g.drawText (title, 14, 6, 170, 20, juce::Justification::left);
}

//==============================================================================
InputPanel::InputPanel (ChannelStripProcessor& p) : ModulePanel (p, "INPUT / HPF", Theme::input)
{
    hpfButton.attach (state, ParamIDs::hpfOn);
    addAndMakeVisible (hpfButton);
    setupCombo (slope, state, ParamIDs::hpfSlope, slopeAtt);
    addAndMakeVisible (slope);
    addAndMakeVisible (inMeter);
    knob (ParamIDs::inTrim, "TRIM", true);
    knob (ParamIDs::hpfFreq, "HPF FREQ");
}

void InputPanel::resized()
{
    hpfButton.setBounds (14, 30, 100, 22);
    slope.setBounds (122, 30, 104, 22);
    inMeter.setBounds (14, 60, 150, 14);
    placeKnobs ({ knobs[0].get(), knobs[1].get() }, knobX);
}

void InputPanel::paint (juce::Graphics& g)
{
    ModulePanel::paint (g);
    g.setColour (Theme::textDim);
    g.setFont (Theme::font (11.0f));
    g.drawText ("IN", 180, 60, 40, 14, juce::Justification::left);
}

void InputPanel::updateMeters()
{
    const float pk = proc.inputPeakDb.load();
    peakHold = pk > peakHold ? pk : peakHold - 1.5f;
    inMeter.setValue (peakHold);
}

//==============================================================================
PreampPanel::PreampPanel (ChannelStripProcessor& p)
    : ModulePanel (p, "PREAMP", Theme::preamp), model (p.apvts, ParamIDs::preModel, Theme::preamp)
{
    addOnButton (ParamIDs::preOn);
    addAndMakeVisible (model);
    addAndMakeVisible (satMeter);
    knob (ParamIDs::preDrive, "DRIVE");
    knob (ParamIDs::preColor, "COLOR");
    knob (ParamIDs::preOut, "OUT", true);
}

void PreampPanel::resized()
{
    onButton->setBounds (178, 7, 48, 18);
    model.setBounds (14, 30, 212, 22);
    satMeter.setBounds (14, 60, 150, 14);
    placeKnobs ({ knobs[0].get(), knobs[1].get(), knobs[2].get() }, knobX);
}

void PreampPanel::paint (juce::Graphics& g)
{
    ModulePanel::paint (g);
    g.setColour (Theme::textDim);
    g.setFont (Theme::font (11.0f));
    g.drawText ("SAT", 180, 60, 40, 14, juce::Justification::left);
}

void PreampPanel::updateMeters()
{
    // Saturation amount indicator: drive weighted by how hot the input is.
    const bool on = state.getRawParameterValue (ParamIDs::preOn)->load() > 0.5f;
    const float drive = state.getRawParameterValue (ParamIDs::preDrive)->load();
    const float level = juce::jlimit (0.0f, 1.0f, (proc.inputPeakDb.load() + 40.0f) / 40.0f);
    satMeter.setValue (on ? drive * (0.4f + 0.6f * level) : 0.0f);
}

//==============================================================================
GatePanel::GatePanel (ChannelStripProcessor& p) : ModulePanel (p, "GATE / EXPANDER", Theme::gate)
{
    addOnButton (ParamIDs::gateOn);
    addAndMakeVisible (openLed);
    addAndMakeVisible (grMeter);
    addAndMakeVisible (transfer);
    addAndMakeVisible (history);

    knob (ParamIDs::gateThresh, "THRESH");
    knob (ParamIDs::gateRatio, "RATIO");
    knob (ParamIDs::gateAttack, "ATTACK");
    knob (ParamIDs::gateRelease, "RELEASE");
    knob (ParamIDs::gateRange, "RANGE");

    transfer.curve = [this] (float in)
    {
        return Gate::transfer (in, state.getRawParameterValue (ParamIDs::gateThresh)->load(),
                               state.getRawParameterValue (ParamIDs::gateRatio)->load(),
                               state.getRawParameterValue (ParamIDs::gateRange)->load());
    };
}

void GatePanel::resized()
{
    onButton->setBounds (178, 7, 48, 18);
    openLed.setBounds (14, 36, 12, 14);
    grMeter.setBounds (60, 36, 150, 14);
    placeKnobs ({ knobs[0].get(), knobs[1].get(), knobs[2].get(), knobs[3].get(), knobs[4].get() }, knobX);
    transfer.setBounds (880, 6, 80, getHeight() - 12);
    history.setBounds (966, 6, getWidth() - 972, getHeight() - 12);
}

void GatePanel::paint (juce::Graphics& g)
{
    ModulePanel::paint (g);
    g.setColour (Theme::textDim);
    g.setFont (Theme::font (11.0f));
    g.drawText ("OPEN", 8, 54, 40, 14, juce::Justification::left);
    g.drawText ("GR dB", 60, 54, 150, 14, juce::Justification::centred);
}

void GatePanel::updateMeters()
{
    const bool on = state.getRawParameterValue (ParamIDs::gateOn)->load() > 0.5f;
    const float gr = on ? proc.gate.meterGr.load() : 0.0f;
    openLed.setValue (on && gr > -1.0f ? 1.0f : 0.0f);
    grMeter.setValue (-gr);
    transfer.setInputLevel (on ? proc.gate.meterIn.load() : -120.0f);
    transfer.repaint();
    history.push (gr);
}

//==============================================================================
EqPanel::EqPanel (ChannelStripProcessor& p) : ModulePanel (p, "EQUALIZER", Theme::eq)
{
    addOnButton (ParamIDs::eqOn);
    setupCombo (position, state, ParamIDs::eqPos, positionAtt);
    addAndMakeVisible (position);

    knob (ParamIDs::lfFreq, "LF FREQ");
    knob (ParamIDs::lfGain, "LF GAIN", true);
    knob (ParamIDs::lmFreq, "LM FREQ");
    knob (ParamIDs::lmGain, "LM GAIN", true);
    knob (ParamIDs::lmQ, "LM Q");
    knob (ParamIDs::hmFreq, "HM FREQ");
    knob (ParamIDs::hmGain, "HM GAIN", true);
    knob (ParamIDs::hmQ, "HM Q");
    knob (ParamIDs::hfFreq, "HF FREQ");
    knob (ParamIDs::hfGain, "HF GAIN", true);
}

void EqPanel::resized()
{
    onButton->setBounds (178, 7, 48, 18);
    position.setBounds (14, 30, 212, 22);
    int x = knobX;
    placeKnobs ({ knobs[0].get(), knobs[1].get() }, x);                       x += 2 * knobW + 30;
    placeKnobs ({ knobs[2].get(), knobs[3].get(), knobs[4].get() }, x);       x += 3 * knobW + 30;
    placeKnobs ({ knobs[5].get(), knobs[6].get(), knobs[7].get() }, x);       x += 3 * knobW + 30;
    placeKnobs ({ knobs[8].get(), knobs[9].get() }, x);
}

void EqPanel::paint (juce::Graphics& g)
{
    ModulePanel::paint (g);
    g.setColour (Theme::textDim);
    g.setFont (Theme::font (11.0f));
    g.drawText ("response overlaid on the analyzer", 14, 60, 212, 14, juce::Justification::left);

    g.setColour (Theme::border.brighter (0.15f));
    int x = knobX + 2 * knobW + 15;
    g.drawVerticalLine (x, 10.0f, (float) getHeight() - 10.0f);
    x += 3 * knobW + 30;
    g.drawVerticalLine (x, 10.0f, (float) getHeight() - 10.0f);
    x += 3 * knobW + 30;
    g.drawVerticalLine (x, 10.0f, (float) getHeight() - 10.0f);
}

//==============================================================================
CompPanel::CompPanel (ChannelStripProcessor& p)
    : ModulePanel (p, "COMPRESSOR", Theme::comp), model (p.apvts, ParamIDs::compModel, Theme::comp)
{
    addOnButton (ParamIDs::compOn);
    addAndMakeVisible (model);
    autoRelease.attach (state, ParamIDs::compAutoRel);
    autoMakeup.attach (state, ParamIDs::compAutoMakeup);
    addAndMakeVisible (autoRelease);
    addAndMakeVisible (autoMakeup);
    addAndMakeVisible (grMeter);
    addAndMakeVisible (transfer);
    addAndMakeVisible (history);

    knob (ParamIDs::compThresh, "THRESH");
    knob (ParamIDs::compRatio, "RATIO");
    knob (ParamIDs::compAttack, "ATTACK");
    knob (ParamIDs::compRelease, "RELEASE");
    knob (ParamIDs::compKnee, "KNEE");
    knob (ParamIDs::compMakeup, "MAKEUP", true);
    knob (ParamIDs::compMix, "MIX");
    knob (ParamIDs::compScHpf, "SC HPF");

    transfer.curve = [this] (float in)
    {
        const float t = state.getRawParameterValue (ParamIDs::compThresh)->load();
        const float r = state.getRawParameterValue (ParamIDs::compRatio)->load();
        const float k = state.getRawParameterValue (ParamIDs::compKnee)->load();
        return in + Compressor::gainComputer (in, t, r, k);
    };
}

void CompPanel::resized()
{
    onButton->setBounds (178, 7, 48, 18);
    model.setBounds (14, 30, 212, 22);
    autoRelease.setBounds (14, 58, 104, 20);
    autoMakeup.setBounds (122, 58, 104, 20);
    placeKnobs ({ knobs[0].get(), knobs[1].get(), knobs[2].get(), knobs[3].get(),
                  knobs[4].get(), knobs[5].get(), knobs[6].get(), knobs[7].get() }, knobX);
    grMeter.setBounds (754, 30, 118, 12);
    transfer.setBounds (880, 6, 80, getHeight() - 12);
    history.setBounds (966, 6, getWidth() - 972, getHeight() - 12);
}

void CompPanel::paint (juce::Graphics& g)
{
    ModulePanel::paint (g);
    g.setColour (Theme::textDim);
    g.setFont (Theme::font (11.0f));
    g.drawText ("GR dB", 754, 46, 118, 14, juce::Justification::centred);
}

void CompPanel::updateMeters()
{
    const bool on = state.getRawParameterValue (ParamIDs::compOn)->load() > 0.5f;
    const float gr = on ? proc.compressor.meterGr.load() : 0.0f;
    grMeter.setValue (gr);
    transfer.setInputLevel (on ? proc.compressor.meterIn.load() : -120.0f);
    transfer.repaint();
    history.push (gr);
}

//==============================================================================
LimiterPanel::LimiterPanel (ChannelStripProcessor& p)
    : ModulePanel (p, "LIMITER / OUTPUT", Theme::limiter), mode (p.apvts, ParamIDs::limMode, Theme::limiter)
{
    addOnButton (ParamIDs::limOn);
    addAndMakeVisible (grMeter);
    addAndMakeVisible (mode);
    addAndMakeVisible (resetButton);
    addAndMakeVisible (targetMeter);
    resetButton.onClick = [this] { proc.resetIntegratedLoudness(); };

    knob (ParamIDs::limCeiling, "CEILING");
    knob (ParamIDs::limRelease, "RELEASE");
    knob (ParamIDs::limOutput, "OUTPUT", true);
    knob (ParamIDs::limTarget, "TARGET");
}

void LimiterPanel::resized()
{
    onButton->setBounds (178, 7, 48, 18);
    grMeter.setBounds (14, 40, 212, 14);
    placeKnobs ({ knobs[0].get(), knobs[1].get(), knobs[2].get(), knobs[3].get() }, knobX);
    mode.setBounds (530, 10, 150, 22);
    resetButton.setBounds (530, 38, 150, 20);
    targetMeter.setBounds (530, 64, 150, 14);
}

void LimiterPanel::paint (juce::Graphics& g)
{
    ModulePanel::paint (g);
    g.setColour (Theme::textDim);
    g.setFont (Theme::font (11.0f));
    g.drawText ("GR dB", 14, 58, 212, 14, juce::Justification::centred);

    g.setColour (Theme::border.brighter (0.15f));
    g.drawVerticalLine (700, 10.0f, (float) getHeight() - 10.0f);

    const float target = state.getRawParameterValue (ParamIDs::limTarget)->load();
    const struct { const char* label; float value; bool highlight; } cols[] = {
        { "MOMENTARY", momentary, false },
        { "SHORT-TERM", shortTerm, true },
        { "INTEGRATED", integrated, false },
        { "INPUT (M)", inputM, false },
    };

    const int x0 = 712, w = (getWidth() - x0 - 10) / 4;
    for (int i = 0; i < 4; ++i)
    {
        const int x = x0 + i * w;
        g.setColour (Theme::textDim);
        g.setFont (Theme::font (11.5f));
        g.drawText (cols[i].label, x, 8, w, 14, juce::Justification::centred);

        const bool onTarget = cols[i].highlight && std::abs (cols[i].value - target) <= 1.0f;
        g.setColour (onTarget ? Theme::ledGreen : Theme::text);
        g.setFont (Theme::font (22.0f, true));
        g.drawText (lufsText (cols[i].value), x, 28, w, 28, juce::Justification::centred);

        g.setColour (Theme::textDim);
        g.setFont (Theme::font (11.0f));
        g.drawText ("LUFS", x, 58, w, 14, juce::Justification::centred);
    }
}

void LimiterPanel::updateMeters()
{
    const bool on = state.getRawParameterValue (ParamIDs::limOn)->load() > 0.5f;
    grMeter.setValue (on ? -proc.limiter.meterGr.load() : 0.0f);

    momentary  = proc.outputLoudness.momentary.load();
    shortTerm  = proc.outputLoudness.shortTerm.load();
    integrated = proc.outputLoudness.integrated.load();
    inputM     = proc.inputLoudness.momentary.load();

    const float target = state.getRawParameterValue (ParamIDs::limTarget)->load();
    targetMeter.setValue (shortTerm <= -99.0f ? -1000.0f : shortTerm - target);
    repaint (700, 0, getWidth() - 700, getHeight());
}
