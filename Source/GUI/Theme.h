#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Theme
{
    inline const juce::Colour background  { 0xff141619 };
    inline const juce::Colour panel       { 0xff1b1e22 };
    inline const juce::Colour panelLeft   { 0xff1f2227 };
    inline const juce::Colour border      { 0xff2c3036 };
    inline const juce::Colour control     { 0xff2a2e34 };
    inline const juce::Colour text        { 0xffe8e8e8 };
    inline const juce::Colour textDim     { 0xff8b929b };
    inline const juce::Colour ledOff      { 0xff2e3238 };

    inline const juce::Colour input   { 0xff9aa7bd };
    inline const juce::Colour preamp  { 0xfff0a030 };
    inline const juce::Colour gate    { 0xff3ec07a };
    inline const juce::Colour eq      { 0xff4a90e2 };
    inline const juce::Colour comp    { 0xfff2643c };
    inline const juce::Colour limiter { 0xffe2405a };

    inline const juce::Colour ledGreen { 0xff3ad65a };
    inline const juce::Colour ledAmber { 0xffe0a020 };
    inline const juce::Colour ledRed   { 0xffe23a3a };

    inline juce::Font font (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    }
}

class ChannelStripLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChannelStripLookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, Theme::control);
        setColour (juce::ComboBox::outlineColourId, Theme::border);
        setColour (juce::ComboBox::textColourId, Theme::text);
        setColour (juce::ComboBox::arrowColourId, Theme::text);
        setColour (juce::PopupMenu::backgroundColourId, Theme::panel);
        setColour (juce::PopupMenu::textColourId, Theme::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::control.brighter (0.2f));
        setColour (juce::TextButton::buttonColourId, Theme::control);
        setColour (juce::TextButton::textColourOffId, Theme::text);
        setColour (juce::Label::textColourId, Theme::text);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                           float startAngle, float endAngle, juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float arcRadius = radius - 2.0f;
        const float angle = startAngle + pos * (endAngle - startAngle);
        const auto colour = slider.findColour (juce::Slider::rotarySliderFillColourId);

        // track
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
        g.setColour (Theme::border.brighter (0.1f));
        g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // value arc (from centre for bipolar parameters)
        const bool bipolar = slider.getProperties().getWithDefault ("bipolar", false);
        const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
        g.setColour (colour);
        g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // body
        const float bodyR = arcRadius - 5.0f;
        juce::ColourGradient grad (juce::Colour (0xff3a3f46), centre.x, centre.y - bodyR,
                                   juce::Colour (0xff16181b), centre.x, centre.y + bodyR, false);
        g.setGradientFill (grad);
        g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

        // pointer
        juce::Path p;
        p.addRoundedRectangle (-1.25f, -bodyR + 2.0f, 2.5f, bodyR * 0.6f, 1.0f);
        g.setColour (Theme::text);
        g.fillPath (p, juce::AffineTransform::rotation (angle).translated (centre));
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override
    {
        const auto r = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (0.5f);
        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (r, 2.0f, 1.0f);

        juce::Path arrow;
        const float ax = (float) width - 14.0f, ay = (float) height * 0.5f;
        arrow.addTriangle (ax - 5.0f, ay - 3.0f, ax + 5.0f, ay - 3.0f, ax, ay + 4.0f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.fillPath (arrow);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (4, 1, box.getWidth() - 24, box.getHeight() - 2);
        label.setFont (Theme::font (13.0f, true));
        label.setJustificationType (juce::Justification::centred);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return Theme::font (13.0f, true); }
    juce::Font getPopupMenuFont() override { return Theme::font (14.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return Theme::font (12.0f); }
};
