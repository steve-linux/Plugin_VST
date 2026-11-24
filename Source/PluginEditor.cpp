#include "PluginEditor.h"

ChannelStripContent::ChannelStripContent (ChannelStripProcessor& p)
    : proc (p), input (p), preamp (p), gate (p), eq (p), comp (p), limiter (p), analyzer (p)
{
    for (auto* c : std::initializer_list<juce::Component*> { &input, &preamp, &gate, &eq, &comp, &limiter, &analyzer })
        addAndMakeVisible (c);

    refreshPresetBox();
    presetBox.setTextWhenNothingSelected ("- select -");
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0) proc.setCurrentProgram (idx);
    };
    addAndMakeVisible (presetBox);

    oversampling.addItemList (Choices::oversampling, 1);
    oversamplingAtt = std::make_unique<APVTS::ComboBoxAttachment> (p.apvts, ParamIDs::oversampling, oversampling);
    addAndMakeVisible (oversampling);

    setSize (baseWidth, baseHeight);
}

void ChannelStripContent::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
}

void ChannelStripContent::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);

    g.setColour (Theme::text);
    g.setFont (Theme::font (22.0f, true));
    g.drawText ("CHANNEL STRIP", 16, 4, 240, 30, juce::Justification::left);

    g.setColour (Theme::textDim);
    g.setFont (Theme::font (12.0f));
    g.drawText ("FACTORY PRESET", 268, 9, 110, 20, juce::Justification::left);
    g.drawText (juce::String ("SIGNAL FLOW ") + juce::String::charToString ((juce::juce_wchar) 0x2193), 670, 9, 120, 20, juce::Justification::left);
    g.drawText ("OVERSAMPLING", 970, 9, 110, 20, juce::Justification::left);
}

void ChannelStripContent::resized()
{
    presetBox.setBounds (370, 7, 270, 24);
    oversampling.setBounds (1090, 7, 96, 24);

    const int x = 10, w = baseWidth - 20, rowH = 86;
    int y = 40;
    for (auto* c : std::initializer_list<juce::Component*> { &input, &preamp, &gate, &eq, &comp, &limiter })
    {
        c->setBounds (x, y, w, rowH);
        y += rowH + 1;
    }
    analyzer.setBounds (x, y + 4, w, baseHeight - y - 10);
}

void ChannelStripContent::updateMeters()
{
    for (auto* m : std::initializer_list<ModulePanel*> { &input, &preamp, &gate, &eq, &comp, &limiter })
        m->updateMeters();
    analyzer.update();

    const int prog = proc.getCurrentProgram();
    if (presetBox.getSelectedItemIndex() != prog)
        presetBox.setSelectedItemIndex (prog, juce::dontSendNotification);
}

//==============================================================================
ChannelStripEditor::ChannelStripEditor (ChannelStripProcessor& p)
    : AudioProcessorEditor (p), content (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);

    setResizable (true, true);
    setResizeLimits (ChannelStripContent::baseWidth * 6 / 10, ChannelStripContent::baseHeight * 6 / 10,
                     ChannelStripContent::baseWidth * 2, ChannelStripContent::baseHeight * 2);
    getConstrainer()->setFixedAspectRatio ((double) ChannelStripContent::baseWidth / ChannelStripContent::baseHeight);
    setSize (ChannelStripContent::baseWidth, ChannelStripContent::baseHeight);

    startTimerHz (30);
}

ChannelStripEditor::~ChannelStripEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ChannelStripEditor::resized()
{
    const float scale = (float) getWidth() / (float) ChannelStripContent::baseWidth;
    content.setTransform (juce::AffineTransform::scale (scale));
    content.setBounds (0, 0, ChannelStripContent::baseWidth, ChannelStripContent::baseHeight);
}
