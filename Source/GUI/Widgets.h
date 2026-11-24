#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"

using APVTS = juce::AudioProcessorValueTreeState;

//==============================================================================
/** Rotary knob with its name and current value printed underneath. */
class Knob : public juce::Component
{
public:
    Knob (APVTS& state, const juce::String& paramId, const juce::String& labelText, juce::Colour c, bool bipolar = false)
        : name (labelText), colour (c)
    {
        param = state.getParameter (paramId);
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setColour (juce::Slider::rotarySliderFillColourId, c);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        slider.getProperties().set ("bipolar", bipolar);
        slider.setPopupDisplayEnabled (false, false, nullptr);
        slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible (slider);
        attachment = std::make_unique<APVTS::SliderAttachment> (state, paramId, slider);
    }

    void resized() override
    {
        const int s = juce::jmin (getWidth(), 48);
        slider.setBounds ((getWidth() - s) / 2, 0, s, s);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (Theme::text);
        g.setFont (Theme::font (11.5f));
        g.drawText (name, 0, 49, getWidth(), 14, juce::Justification::centred);
        g.setColour (colour);
        g.setFont (Theme::font (12.5f));
        g.drawText (param != nullptr ? param->getCurrentValueAsText() : juce::String(), 0, 63, getWidth(), 15,
                    juce::Justification::centred);
    }

    juce::Slider slider;

private:
    juce::String name;
    juce::Colour colour;
    juce::RangedAudioParameter* param = nullptr;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};

//==============================================================================
/** Flat toggle button attached to a bool parameter (or free-standing). */
class FlatToggle : public juce::Button
{
public:
    FlatToggle (const juce::String& onText, const juce::String& offText, juce::Colour c)
        : juce::Button (onText), textOn (onText), textOff (offText), colour (c)
    {
        setClickingTogglesState (true);
    }

    void attach (APVTS& state, const juce::String& paramId)
    {
        attachment = std::make_unique<APVTS::ButtonAttachment> (state, paramId, *this);
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        const auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool on = getToggleState();
        auto fill = on ? colour : Theme::control;
        if (over) fill = fill.brighter (0.08f);
        if (down) fill = fill.darker (0.1f);
        g.setColour (fill);
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (on ? colour.brighter (0.3f) : Theme::border.brighter (0.2f));
        g.drawRoundedRectangle (r, 2.0f, 1.0f);
        g.setColour (on ? juce::Colour (0xff101214) : Theme::text);
        g.setFont (Theme::font (12.0f, true));
        g.drawText (on ? textOn : textOff, getLocalBounds(), juce::Justification::centred);
    }

private:
    juce::String textOn, textOff;
    juce::Colour colour;
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
};

//==============================================================================
/** Momentary flat button (non-toggling). */
class FlatButton : public juce::Button
{
public:
    explicit FlatButton (const juce::String& t) : juce::Button (t) {}

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        const auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (down ? Theme::control.brighter (0.2f) : (over ? Theme::control.brighter (0.1f) : Theme::control));
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (Theme::border.brighter (0.2f));
        g.drawRoundedRectangle (r, 2.0f, 1.0f);
        g.setColour (Theme::text);
        g.setFont (Theme::font (12.0f));
        g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred);
    }
};

//==============================================================================
/** "< [ combo ] >" selector attached to a choice parameter. */
class ModelSelector : public juce::Component
{
public:
    ModelSelector (APVTS& state, const juce::String& paramId, juce::Colour c) : colour (c)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (paramId)))
            box.addItemList (p->choices, 1);

        box.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
        box.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        box.setColour (juce::ComboBox::arrowColourId, c);
        box.setColour (juce::ComboBox::textColourId, Theme::text);
        addAndMakeVisible (box);
        attachment = std::make_unique<APVTS::ComboBoxAttachment> (state, paramId, box);

        for (auto* b : { &prev, &next })
        {
            b->setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            b->setColour (juce::TextButton::textColourOffId, c);
            b->setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
            addAndMakeVisible (*b);
        }
        prev.onClick = [this] { step (-1); };
        next.onClick = [this] { step (+1); };
    }

    void step (int delta)
    {
        const int num = box.getNumItems();
        if (num > 0)
            box.setSelectedItemIndex ((box.getSelectedItemIndex() + delta + num) % num);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        prev.setBounds (r.removeFromLeft (20));
        next.setBounds (r.removeFromRight (20));
        box.setBounds (r);
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (Theme::control.darker (0.3f));
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (colour);
        g.drawRoundedRectangle (r, 2.0f, 1.0f);
    }

    juce::ComboBox box;

private:
    juce::Colour colour;
    juce::TextButton prev { "<" }, next { ">" };
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
};

//==============================================================================
/** Row of round LEDs. Each LED lights when value >= its threshold. */
class LedMeter : public juce::Component
{
public:
    LedMeter (std::vector<float> thresholdsIn, std::vector<juce::Colour> coloursIn)
        : thresholds (std::move (thresholdsIn)), colours (std::move (coloursIn)) {}

    void setValue (float v)
    {
        if (std::abs (v - value) > 0.05f) { value = v; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        const int n = (int) thresholds.size();
        if (n == 0) return;
        const float d = juce::jmin ((float) getHeight() - 2.0f, 11.0f);
        const float step = ((float) getWidth() - d) / (float) juce::jmax (1, n - 1);
        for (int i = 0; i < n; ++i)
        {
            const bool lit = value >= thresholds[(size_t) i];
            const auto c = colours[(size_t) juce::jmin (i, (int) colours.size() - 1)];
            const float x = (float) i * step, y = ((float) getHeight() - d) * 0.5f;
            if (lit)
            {
                g.setColour (c.withAlpha (0.25f));
                g.fillEllipse (x - 2.0f, y - 2.0f, d + 4.0f, d + 4.0f);
            }
            g.setColour (lit ? c : Theme::ledOff);
            g.fillEllipse (x, y, d, d);
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.drawEllipse (x, y, d, d, 1.0f);
        }
    }

private:
    std::vector<float> thresholds;
    std::vector<juce::Colour> colours;
    float value = -1000.0f;
};

//==============================================================================
/** Static input/output curve with the current detector level as a dot. */
class TransferDisplay : public juce::Component
{
public:
    explicit TransferDisplay (juce::Colour c) : colour (c) {}

    std::function<float (float)> curve;   // in dB -> out dB
    static constexpr float minDb = -80.0f;

    void setInputLevel (float db)
    {
        if (std::abs (db - inputDb) > 0.2f) { inputDb = db; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff111316));
        g.fillRect (r);
        g.setColour (Theme::border);
        g.drawRect (r, 1.0f);

        g.setColour (Theme::textDim);
        g.setFont (Theme::font (10.5f));
        g.drawText ("IN/OUT", r.reduced (4.0f, 2.0f).removeFromTop (12.0f), juce::Justification::topLeft);

        const auto plot = r.reduced (2.0f).withTrimmedTop (14.0f);
        auto toX = [&] (float db) { return plot.getX() + plot.getWidth() * (db - minDb) / -minDb; };
        auto toY = [&] (float db) { return plot.getBottom() - plot.getHeight() * (juce::jlimit (minDb, 0.0f, db) - minDb) / -minDb; };

        if (! curve) return;

        // stepped bars under the curve
        const int bars = 24;
        const float bw = plot.getWidth() / (float) bars;
        for (int i = 0; i < bars; ++i)
        {
            const float inDb = minDb + (-minDb) * ((float) i + 0.5f) / (float) bars;
            const float y = toY (curve (inDb));
            g.setColour (colour.withAlpha (0.35f));
            g.fillRect (plot.getX() + (float) i * bw, y, bw - 1.0f, plot.getBottom() - y);
        }

        juce::Path p;
        for (int i = 0; i <= 64; ++i)
        {
            const float inDb = minDb + (-minDb) * (float) i / 64.0f;
            const float x = toX (inDb), y = toY (curve (inDb));
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (1.5f));

        // unity reference
        g.setColour (Theme::textDim.withAlpha (0.4f));
        g.drawLine (toX (minDb), toY (minDb), toX (0.0f), toY (0.0f), 0.7f);

        if (inputDb > minDb)
        {
            const float x = toX (inputDb), y = toY (curve (inputDb));
            g.setColour (juce::Colours::white);
            g.fillEllipse (x - 3.5f, y - 3.5f, 7.0f, 7.0f);
        }
    }

private:
    juce::Colour colour;
    float inputDb = -120.0f;
};

//==============================================================================
/** Scrolling gain-reduction history. */
class GrHistory : public juce::Component
{
public:
    explicit GrHistory (juce::Colour c) : colour (c) { history.fill (0.0f); }

    void push (float grDb)
    {
        history[(size_t) writePos] = juce::jlimit (0.0f, maxDb, std::abs (grDb));
        writePos = (writePos + 1) % (int) history.size();
        current = std::abs (grDb);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff111316));
        g.fillRect (r);
        g.setColour (Theme::border);
        g.drawRect (r, 1.0f);

        auto header = r.reduced (6.0f, 2.0f).removeFromTop (13.0f);
        g.setFont (Theme::font (10.5f));
        g.setColour (Theme::textDim);
        g.drawText ("GAIN REDUCTION", header, juce::Justification::left);
        g.drawText (juce::String (current, 1) + " dB", header, juce::Justification::right);

        const auto plot = r.reduced (2.0f).withTrimmedTop (15.0f);
        g.setColour (Theme::border.withAlpha (0.6f));
        for (int i = 1; i < 4; ++i)
            g.drawHorizontalLine ((int) (plot.getY() + plot.getHeight() * (float) i / 4.0f), plot.getX(), plot.getRight());

        const int n = (int) history.size();
        juce::Path p;
        for (int i = 0; i < n; ++i)
        {
            const float v = history[(size_t) ((writePos + i) % n)];
            const float x = plot.getX() + plot.getWidth() * (float) i / (float) (n - 1);
            const float y = plot.getY() + plot.getHeight() * v / maxDb;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        juce::Path fill (p);
        fill.lineTo (plot.getRight(), plot.getY());
        fill.lineTo (plot.getX(), plot.getY());
        fill.closeSubPath();
        g.setColour (colour.withAlpha (0.18f));
        g.fillPath (fill);
        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (1.3f));
    }

private:
    static constexpr float maxDb = 24.0f;
    juce::Colour colour;
    std::array<float, 200> history;
    int writePos = 0;
    float current = 0.0f;
};
