#include "Analyzer.h"
#include "../PluginProcessor.h"

Analyzer::Analyzer (ChannelStripProcessor& p) : proc (p) {}

float Analyzer::freqToX (float f, juce::Rectangle<float> r) const
{
    const float lo = std::log10 (20.0f), hi = std::log10 (20000.0f);
    return r.getX() + r.getWidth() * (std::log10 (juce::jmax (f, 1.0f)) - lo) / (hi - lo);
}

void Analyzer::update()
{
    const int got = proc.analyzerFifo.pull (pullBuffer.data(), (int) pullBuffer.size());
    if (got > 0)
    {
        const double fs = proc.getCurrentSampleRate();
        const int perColumn = juce::jmax (1, (int) (fs * 6.0 / numColumns)); // ~6 s across the strip

        for (int i = 0; i < got; ++i)
        {
            const float s = pullBuffer[(size_t) i];
            history[(size_t) historyPos] = s;
            historyPos = (historyPos + 1) % fftSize;

            curMin = juce::jmin (curMin, s);
            curMax = juce::jmax (curMax, s);
            if (++wCount >= perColumn)
            {
                wMin[(size_t) wPos] = curMin;
                wMax[(size_t) wPos] = curMax;
                wPos = (wPos + 1) % numColumns;
                wCount = 0;
                curMin = curMax = 0.0f;
            }
        }
    }

    computeSpectrum();
    updateHarmonics();
    repaint();
}

void Analyzer::computeSpectrum()
{
    for (int i = 0; i < fftSize; ++i)
        fftData[(size_t) i] = history[(size_t) ((historyPos + i) % fftSize)];
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);

    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

    const float norm = 4.0f / (float) fftSize; // Hann: full-scale sine -> 0 dB
    for (size_t k = 0; k < spectrumDb.size(); ++k)
    {
        const float db = juce::jmax (minDb, juce::Decibels::gainToDecibels (fftData[k] * norm, minDb));
        // fast attack, slow decay
        spectrumDb[k] = db > spectrumDb[k] ? db : spectrumDb[k] + 0.25f * (db - spectrumDb[k]);
    }
}

void Analyzer::updateHarmonics()
{
    auto& s = proc.apvts;
    const int model = juce::roundToInt (s.getRawParameterValue (ParamIDs::preModel)->load());
    const float drive = s.getRawParameterValue (ParamIDs::preDrive)->load() * 0.01f;
    const float color = s.getRawParameterValue (ParamIDs::preColor)->load() * 0.01f;

    if (model == lastModel && std::abs (drive - lastDrive) < 1.0e-4f && std::abs (color - lastColor) < 1.0e-4f)
        return;
    lastModel = model; lastDrive = drive; lastColor = color;

    // Bin-centred test tone (~1 kHz at 48 kHz), -12 dBFS, through the static shaper.
    constexpr int bin = 85;
    std::vector<float> data ((size_t) fftSize * 2, 0.0f);
    const float pre = Preamp::driveGain (drive);
    for (int i = 0; i < fftSize; ++i)
    {
        const float x = 0.25f * std::sin (juce::MathConstants<float>::twoPi * (float) bin * (float) i / (float) fftSize);
        data[(size_t) i] = Preamp::shape (model, x * pre, color);
    }
    // Remove DC from asymmetric models before analysis.
    float mean = 0.0f;
    for (int i = 0; i < fftSize; ++i) mean += data[(size_t) i];
    mean /= (float) fftSize;
    for (int i = 0; i < fftSize; ++i) data[(size_t) i] -= mean;

    fft.performFrequencyOnlyForwardTransform (data.data(), true);
    const float fundamental = juce::jmax (1.0e-9f, data[(size_t) bin]);
    for (int h = 2; h <= 5; ++h)
        harmonics[(size_t) h - 2] = juce::Decibels::gainToDecibels (data[(size_t) (bin * h)] / fundamental, -120.0f);
}

void Analyzer::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (Theme::panel);
    g.fillRect (bounds);
    g.setColour (Theme::border);
    g.drawRect (bounds, 1.0f);

    g.setColour (Theme::text);
    g.setFont (Theme::font (14.0f, true));
    g.drawText ("SPECTRUM  +  EQ RESPONSE  +  WAVEFORM  (OUTPUT)", 12, 4, 600, 20, juce::Justification::left);

    auto area = bounds.reduced (12.0f, 0.0f).withTrimmedTop (26.0f).withTrimmedBottom (8.0f);
    auto wave = area.removeFromBottom (34.0f);
    area.removeFromBottom (6.0f);
    auto plot = area;

    g.setColour (juce::Colour (0xff101215));
    g.fillRect (plot);
    g.fillRect (wave);

    // grid
    auto dbToY = [&] (float db) { return plot.getY() + plot.getHeight() * (db / minDb); };
    g.setFont (Theme::font (10.5f));
    for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f })
    {
        const float x = freqToX (f, plot);
        g.setColour (Theme::border.withAlpha (0.8f));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        g.setColour (Theme::textDim);
        const juce::String label = f >= 1000.0f ? juce::String ((int) (f / 1000.0f)) + "k" : juce::String ((int) f);
        g.drawText (label, (int) x - 20, (int) plot.getBottom() - 14, 40, 12, juce::Justification::centred);
    }
    for (float db : { -30.0f, -60.0f })
    {
        g.setColour (Theme::border.withAlpha (0.8f));
        g.drawHorizontalLine ((int) dbToY (db), plot.getX(), plot.getRight());
        g.setColour (Theme::textDim);
        g.drawText (juce::String ((int) db), (int) plot.getX() + 3, (int) dbToY (db) - 13, 40, 12, juce::Justification::left);
    }
    g.drawText ("0 dB", (int) plot.getX() + 3, (int) plot.getY() + 2, 40, 12, juce::Justification::left);

    // spectrum: bars + stepped line, one step per ~6 px
    const double fs = proc.getCurrentSampleRate();
    const float binHz = (float) (fs / fftSize);
    const int step = 6;
    juce::Path line;
    bool started = false;
    for (int px = 0; px < (int) plot.getWidth(); px += step)
    {
        const float lo = std::log10 (20.0f), hi = std::log10 (20000.0f);
        const float f0 = std::pow (10.0f, lo + (hi - lo) * (float) px / plot.getWidth());
        const float f1 = std::pow (10.0f, lo + (hi - lo) * (float) (px + step) / plot.getWidth());
        int k0 = juce::jlimit (1, (int) spectrumDb.size() - 1, (int) (f0 / binHz));
        int k1 = juce::jlimit (k0, (int) spectrumDb.size() - 1, (int) (f1 / binHz));
        float db = minDb;
        for (int k = k0; k <= k1; ++k) db = juce::jmax (db, spectrumDb[(size_t) k]);

        const float x = plot.getX() + (float) px, y = dbToY (db);
        g.setColour (juce::Colour (0xff23272c));
        g.fillRect (x, y, (float) step - 1.0f, plot.getBottom() - y);
        if (! started) { line.startNewSubPath (x, y); started = true; }
        else line.lineTo (x, y);
        line.lineTo (x + (float) step, y);
    }
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.strokePath (line, juce::PathStrokeType (1.4f));

    // EQ response (+/-15 dB over the full height)
    auto& s = proc.apvts;
    if (s.getRawParameterValue (ParamIDs::eqOn)->load() > 0.5f)
    {
        Equalizer::Settings eq;
        eq.lfFreq = s.getRawParameterValue (ParamIDs::lfFreq)->load();
        eq.lfGain = s.getRawParameterValue (ParamIDs::lfGain)->load();
        eq.lmFreq = s.getRawParameterValue (ParamIDs::lmFreq)->load();
        eq.lmGain = s.getRawParameterValue (ParamIDs::lmGain)->load();
        eq.lmQ    = s.getRawParameterValue (ParamIDs::lmQ)->load();
        eq.hmFreq = s.getRawParameterValue (ParamIDs::hmFreq)->load();
        eq.hmGain = s.getRawParameterValue (ParamIDs::hmGain)->load();
        eq.hmQ    = s.getRawParameterValue (ParamIDs::hmQ)->load();
        eq.hfFreq = s.getRawParameterValue (ParamIDs::hfFreq)->load();
        eq.hfGain = s.getRawParameterValue (ParamIDs::hfGain)->load();
        const auto coeffs = Equalizer::makeCoeffs (eq, fs);

        juce::Path eqPath;
        for (int px = 0; px <= (int) plot.getWidth(); px += 2)
        {
            const float lo = std::log10 (20.0f), hi = std::log10 (20000.0f);
            const double f = std::pow (10.0, lo + (hi - lo) * (double) px / plot.getWidth());
            double mag = 1.0;
            for (auto& c : coeffs) mag *= c.magnitude (f, fs);
            const float db = juce::jlimit (-eqRangeDb, eqRangeDb, (float) juce::Decibels::gainToDecibels (mag));
            const float y = plot.getCentreY() - plot.getHeight() * 0.5f * db / eqRangeDb;
            if (px == 0) eqPath.startNewSubPath (plot.getX() + (float) px, y);
            else eqPath.lineTo (plot.getX() + (float) px, y);
        }
        g.setColour (Theme::eq);
        g.strokePath (eqPath, juce::PathStrokeType (1.8f));
        g.setFont (Theme::font (10.5f));
        g.drawText ("+15 dB EQ", (int) plot.getRight() - 70, (int) plot.getY() + 2, 66, 12, juce::Justification::right);
        g.drawText ("-15", (int) plot.getRight() - 30, (int) plot.getBottom() - 28, 26, 12, juce::Justification::right);
    }

    // Preamp harmonic estimate
    if (s.getRawParameterValue (ParamIDs::preOn)->load() > 0.5f)
    {
        const int model = juce::roundToInt (s.getRawParameterValue (ParamIDs::preModel)->load());
        g.setFont (Theme::font (11.0f));
        g.setColour (Theme::preamp.withAlpha (0.85f));
        g.drawText ("preamp harmonics (model estimate): " + Choices::preModel[model],
                    (int) plot.getRight() - 440, (int) plot.getY() + 2, 360, 12, juce::Justification::right);

        for (int h = 2; h <= 5; ++h)
        {
            const float db = harmonics[(size_t) h - 2];
            if (db < minDb + 3.0f) continue;
            const float x = freqToX (1000.0f * (float) h, plot), y = dbToY (db);
            g.setColour (Theme::preamp);
            g.drawLine (x, y, x, plot.getBottom(), 1.0f);
            g.fillEllipse (x - 3.0f, y - 3.0f, 6.0f, 6.0f);
            g.drawText ("H" + juce::String (h), (int) x - 12, (int) y - 16, 24, 12, juce::Justification::centred);
        }
    }

    // Waveform strip
    const float mid = wave.getCentreY(), half = wave.getHeight() * 0.5f - 2.0f;
    g.setColour (Theme::textDim.withAlpha (0.8f));
    for (int i = 0; i < numColumns; ++i)
    {
        const int idx = (wPos + i) % numColumns;
        const float x = wave.getX() + wave.getWidth() * (float) i / (float) numColumns;
        const float y0 = mid - juce::jlimit (-1.0f, 1.0f, wMax[(size_t) idx]) * half;
        const float y1 = mid - juce::jlimit (-1.0f, 1.0f, wMin[(size_t) idx]) * half;
        g.drawVerticalLine ((int) x, y0, juce::jmax (y0 + 1.0f, y1));
    }
}
