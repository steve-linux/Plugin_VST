#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Biquad.h"

/** Input trim + high-pass filter with selectable slope (6/12/18/24 dB/oct). */
class InputStage
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        trim.reset (sampleRate, 0.02);
        for (auto& f : filters) f.reset();
        lastFreq = -1.0f;
    }

    void setParams (float trimDb, bool hpfEnabled, float freq, int slopeIndex)
    {
        trim.setTargetValue (juce::Decibels::decibelsToGain (trimDb));
        hpfOn = hpfEnabled;

        if (std::abs (freq - lastFreq) > 1.0e-3f || slopeIndex != slope)
        {
            lastFreq = freq;
            slope = slopeIndex;

            switch (slope)
            {
                case 0: // 6 dB/oct
                    filters[0].c = BiquadCoeffs::firstOrderHighPass (fs, freq);
                    numStages = 1; break;
                case 1: // 12 dB/oct Butterworth
                    filters[0].c = BiquadCoeffs::highPass (fs, freq, 0.70710678);
                    numStages = 1; break;
                case 2: // 18 dB/oct: 1st order + 2nd order (Q = 1)
                    filters[0].c = BiquadCoeffs::firstOrderHighPass (fs, freq);
                    filters[1].c = BiquadCoeffs::highPass (fs, freq, 1.0);
                    numStages = 2; break;
                default: // 24 dB/oct Butterworth (two sections)
                    filters[0].c = BiquadCoeffs::highPass (fs, freq, 0.54119610);
                    filters[1].c = BiquadCoeffs::highPass (fs, freq, 1.30656296);
                    numStages = 2; break;
            }
        }
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numCh = juce::jmin (2, buffer.getNumChannels());
        const int n = buffer.getNumSamples();

        for (int i = 0; i < n; ++i)
        {
            const float g = trim.getNextValue();
            for (int ch = 0; ch < numCh; ++ch)
            {
                float x = buffer.getSample (ch, i) * g;
                if (hpfOn)
                    for (int s = 0; s < numStages; ++s)
                        x = filters[(size_t) s].process (x, ch);
                buffer.setSample (ch, i, x);
            }
        }
    }

private:
    double fs = 44100.0;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> trim { 1.0f };
    std::array<Biquad<2>, 2> filters;
    int numStages = 1, slope = -1;
    bool hpfOn = true;
    float lastFreq = -1.0f;
};
