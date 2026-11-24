#pragma once

#include <juce_dsp/juce_dsp.h>
#include "Theme.h"

class ChannelStripProcessor;

/** Output spectrum + EQ response + preamp harmonic estimate + waveform strip. */
class Analyzer : public juce::Component
{
public:
    explicit Analyzer (ChannelStripProcessor&);

    void update();   // called by the editor's timer
    void paint (juce::Graphics&) override;

private:
    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int numColumns = 600;
    static constexpr float minDb = -90.0f;
    static constexpr float eqRangeDb = 15.0f;

    void computeSpectrum();
    void updateHarmonics();
    float freqToX (float f, juce::Rectangle<float> r) const;

    ChannelStripProcessor& proc;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };

    std::vector<float> history = std::vector<float> ((size_t) fftSize, 0.0f);  // latest samples, ring
    int historyPos = 0;
    std::vector<float> fftData = std::vector<float> ((size_t) fftSize * 2, 0.0f);
    std::vector<float> spectrumDb = std::vector<float> ((size_t) fftSize / 2 + 1, minDb);
    std::vector<float> pullBuffer = std::vector<float> (32768, 0.0f);

    // waveform strip
    std::array<float, numColumns> wMin {}, wMax {};
    int wPos = 0, wCount = 0;
    float curMin = 0.0f, curMax = 0.0f;

    // harmonic estimate (H2..H5 relative to fundamental, dB)
    std::array<float, 4> harmonics { -120.0f, -120.0f, -120.0f, -120.0f };
    int lastModel = -1;
    float lastDrive = -1.0f, lastColor = -1.0f;
};
