#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include "Biquad.h"

/** Feed-forward compressor with three characters (VCA, FET, Opto),
    soft knee, sidechain HPF, parallel mix, auto release and auto makeup. */
class Compressor
{
public:
    enum Model { vca = 0, fet, opto };

    struct Settings
    {
        int model = vca;
        float thresh = -28, ratio = 4, attackMs = 10, releaseMs = 150, knee = 6,
              makeup = 0, mix = 100, scHpf = 60;
        bool autoRelease = false, autoMakeup = false;
    };

    /** Static gain computer (returns gain change in dB, <= 0). */
    static float gainComputer (float inDb, float thresh, float ratio, float knee)
    {
        const float over = inDb - thresh;
        const float slope = 1.0f / ratio - 1.0f;
        if (knee > 0.0f && 2.0f * std::abs (over) <= knee)
            return slope * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.0f * knee);
        return over > 0.0f ? slope * over : 0.0f;
    }

    static float autoMakeupDb (float thresh, float ratio, float knee)
    {
        return -0.5f * gainComputer (-12.0f, thresh, ratio, knee);
    }

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        scFilter.reset();
        rmsState = 0.0f;
        grFast = grSlow = 0.0f;
        lastScHpf = -1.0f;
        makeup.reset (sampleRate, 0.05);
        mix.reset (sampleRate, 0.05);
    }

    void setParams (const Settings& s)
    {
        st = s;

        float attack = s.attackMs, release = s.releaseMs;
        if (s.model == fet)  { attack *= 0.25f; release *= 0.7f; }
        if (s.model == opto) { attack = juce::jmax (attack, 5.0f) * 1.5f; }

        attCoeff  = timeCoeff (attack);
        relCoeff  = timeCoeff (release);
        relFast   = timeCoeff (40.0f);
        relSlow   = timeCoeff (juce::jmax (release * 4.0f, 600.0f));
        rmsCoeff  = timeCoeff (s.model == vca ? 5.0f : 1.0f);

        if (std::abs (s.scHpf - lastScHpf) > 1.0e-3f)
        {
            lastScHpf = s.scHpf;
            scFilter.c = BiquadCoeffs::highPass (fs, s.scHpf, 0.70710678);
        }

        const float mk = s.makeup + (s.autoMakeup ? autoMakeupDb (s.thresh, s.ratio, s.knee) : 0.0f);
        makeup.setTargetValue (juce::Decibels::decibelsToGain (mk));
        mix.setTargetValue (s.mix * 0.01f);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numCh = juce::jmin (2, buffer.getNumChannels());
        const int n = buffer.getNumSamples();
        float maxGr = 0.0f, maxIn = -120.0f;

        for (int i = 0; i < n; ++i)
        {
            // Sidechain detection (stereo linked, HPF'd)
            float det = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float s = scFilter.process (buffer.getSample (ch, i), ch);
                det = juce::jmax (det, s * s);
            }

            float levelDb;
            if (st.model == fet)
            {
                levelDb = juce::Decibels::gainToDecibels (std::sqrt (det), -120.0f); // peak
            }
            else
            {
                rmsState = det + rmsCoeff * (rmsState - det);
                levelDb = juce::Decibels::gainToDecibels (std::sqrt (rmsState) * 1.41421356f, -120.0f);
            }

            float ratio = st.ratio;
            if (st.model == opto) // opto: ratio softens at low levels (program dependent)
                ratio = 1.0f + (st.ratio - 1.0f) * juce::jlimit (0.3f, 1.0f, (levelDb - st.thresh + 12.0f) / 18.0f);

            const float target = -gainComputer (levelDb, st.thresh, ratio, st.knee); // >= 0 dB of reduction

            float gr;
            if (st.autoRelease || st.model == opto)
            {
                // Two-stage release: fast stage for transients, slow stage for sustained material.
                grFast = target > grFast ? target + attCoeff * (grFast - target) : target + relFast * (grFast - target);
                grSlow = target > grSlow ? target + attCoeff * (grSlow - target) : target + relSlow * (grSlow - target);
                gr = st.model == opto ? 0.4f * grFast + 0.6f * grSlow : 0.6f * grFast + 0.4f * grSlow;
            }
            else
            {
                const float c = target > grFast ? attCoeff : relCoeff;
                grFast = target + c * (grFast - target);
                gr = grFast;
            }

            const float g = juce::Decibels::decibelsToGain (-gr) * makeup.getNextValue();
            const float wet = mix.getNextValue();

            for (int ch = 0; ch < numCh; ++ch)
            {
                const float x = buffer.getSample (ch, i);
                float y = x * g;
                if (st.model == fet) // a touch of FET-style output colour
                    y = std::tanh (y * 1.05f) / 1.05f;
                buffer.setSample (ch, i, x + wet * (y - x));
            }

            maxGr = juce::jmax (maxGr, gr);
            maxIn = juce::jmax (maxIn, levelDb);
        }

        meterGr.store (maxGr);
        meterIn.store (maxIn);
    }

    std::atomic<float> meterGr { 0.0f }, meterIn { -120.0f };

private:
    float timeCoeff (float ms) const { return std::exp (-1.0f / (juce::jmax (ms, 0.01f) * 0.001f * (float) fs)); }

    double fs = 44100.0;
    Settings st;
    Biquad<2> scFilter;
    float lastScHpf = -1.0f;
    float attCoeff = 0, relCoeff = 0, relFast = 0, relSlow = 0, rmsCoeff = 0;
    float rmsState = 0, grFast = 0, grSlow = 0;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> makeup { 1.0f };
    juce::SmoothedValue<float> mix { 1.0f };
};
