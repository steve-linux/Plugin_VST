#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

/** Downward expander / gate with range control. Stereo-linked. */
class Gate
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        env = -120.0f;
        grDb = 0.0f;
        detCoeff = std::exp (-1.0f / (0.002f * (float) fs)); // 2 ms detector release
    }

    void setParams (float threshDb, float ratioIn, float attackMs, float releaseMs, float rangeDb)
    {
        thresh = threshDb;
        ratio = ratioIn;
        range = rangeDb;
        attCoeff = std::exp (-1.0f / (attackMs * 0.001f * (float) fs));
        relCoeff = std::exp (-1.0f / (releaseMs * 0.001f * (float) fs));
    }

    /** Static curve: output level for a given input level (dB). */
    static float transfer (float inDb, float threshDb, float ratio, float rangeDb)
    {
        if (inDb >= threshDb) return inDb;
        const float gr = juce::jmax ((inDb - threshDb) * (ratio - 1.0f), rangeDb);
        return inDb + gr;
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numCh = juce::jmin (2, buffer.getNumChannels());
        const int n = buffer.getNumSamples();
        float maxGr = 0.0f, maxIn = -120.0f;

        for (int i = 0; i < n; ++i)
        {
            float peak = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
                peak = juce::jmax (peak, std::abs (buffer.getSample (ch, i)));

            const float inDb = juce::Decibels::gainToDecibels (peak, -120.0f);
            env = inDb > env ? inDb : inDb + detCoeff * (env - inDb);

            const float target = transfer (env, thresh, ratio, range) - env; // <= 0
            // Opening (gain rising) uses attack, closing uses release.
            const float c = target > grDb ? attCoeff : relCoeff;
            grDb = target + c * (grDb - target);

            const float g = juce::Decibels::decibelsToGain (grDb);
            for (int ch = 0; ch < numCh; ++ch)
                buffer.setSample (ch, i, buffer.getSample (ch, i) * g);

            maxGr = juce::jmin (maxGr, grDb);
            maxIn = juce::jmax (maxIn, env);
        }

        meterGr.store (maxGr);
        meterIn.store (maxIn);
    }

    std::atomic<float> meterGr { 0.0f }, meterIn { -120.0f };

private:
    double fs = 44100.0;
    float thresh = -55.0f, ratio = 3.0f, range = -30.0f;
    float attCoeff = 0.0f, relCoeff = 0.0f, detCoeff = 0.0f;
    float env = -120.0f, grDb = 0.0f;
};
