#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>

/** Look-ahead brickwall peak limiter.
    gain = boxFilter (release (peakHoldMin (targetGain))) with the audio delayed
    by the look-ahead length, which guarantees the ceiling is not exceeded. */
class Limiter
{
public:
    static constexpr double lookaheadMs = 1.5;

    void prepare (double sampleRate, int numChannels)
    {
        fs = sampleRate;
        L = juce::jmax (1, (int) std::round (lookaheadMs * 0.001 * fs));
        delay.assign ((size_t) (juce::jmax (1, numChannels) * L), 0.0f);
        box.assign ((size_t) L, 1.0f);
        numCh = juce::jmax (1, numChannels);
        reset();
    }

    void reset()
    {
        std::fill (delay.begin(), delay.end(), 0.0f);
        std::fill (box.begin(), box.end(), 1.0f);
        boxSum = (double) L;
        pos = 0; holdCount = 0; holdVal = 1.0f; relEnv = 1.0f;
    }

    int getLatencySamples() const { return L; }

    void setParams (float ceilingDb, float releaseMs)
    {
        ceiling = juce::Decibels::decibelsToGain (ceilingDb);
        relCoeff = std::exp (-1.0f / (juce::jmax (releaseMs, 0.1f) * 0.001f * (float) fs));
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int chs = juce::jmin (numCh, buffer.getNumChannels(), 2);
        const int n = buffer.getNumSamples();
        float maxGr = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            float peak = 0.0f;
            for (int ch = 0; ch < chs; ++ch)
                peak = juce::jmax (peak, std::abs (buffer.getSample (ch, i)));

            const float target = peak > ceiling ? ceiling / peak : 1.0f;

            // Peak-hold of the minimum gain for L samples
            if (target <= holdVal) { holdVal = target; holdCount = L; }
            else if (--holdCount <= 0) { holdVal = target; holdCount = 0; }

            // Release: instant down, exponential up
            relEnv = holdVal < relEnv ? holdVal : holdVal + relCoeff * (relEnv - holdVal);

            // Box filter over L samples
            boxSum += (double) relEnv - (double) box[(size_t) pos];
            box[(size_t) pos] = relEnv;
            const float g = juce::jmin (1.0f, (float) (boxSum / (double) L));

            for (int ch = 0; ch < chs; ++ch)
            {
                float& d = delay[(size_t) (ch * L + pos)];
                const float delayed = d;
                d = buffer.getSample (ch, i);
                buffer.setSample (ch, i, juce::jlimit (-ceiling, ceiling, delayed * g));
            }

            pos = (pos + 1) % L;
            maxGr = juce::jmin (maxGr, juce::Decibels::gainToDecibels (g, -60.0f));
        }

        meterGr.store (maxGr);
    }

    std::atomic<float> meterGr { 0.0f };

private:
    double fs = 44100.0;
    int L = 64, numCh = 2, pos = 0, holdCount = 0;
    float ceiling = 1.0f, relCoeff = 0.0f, holdVal = 1.0f, relEnv = 1.0f;
    double boxSum = 64.0;
    std::vector<float> delay, box;
};
