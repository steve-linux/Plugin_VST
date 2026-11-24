#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cstring>
#include "Biquad.h"

/** 4-band EQ: low shelf, two peaking mids, high shelf. */
class Equalizer
{
public:
    struct Settings
    {
        float lfFreq = 80, lfGain = 0, lmFreq = 400, lmGain = 0, lmQ = 1,
              hmFreq = 3000, hmGain = 0, hmQ = 1, hfFreq = 10000, hfGain = 0;

        bool operator== (const Settings& o) const
        {
            return std::memcmp (this, &o, sizeof (Settings)) == 0;
        }
    };

    static std::array<BiquadCoeffs, 4> makeCoeffs (const Settings& s, double fs)
    {
        const double nyq = fs * 0.49;
        return { BiquadCoeffs::lowShelf  (fs, juce::jmin ((double) s.lfFreq, nyq), s.lfGain),
                 BiquadCoeffs::peak      (fs, juce::jmin ((double) s.lmFreq, nyq), s.lmQ, s.lmGain),
                 BiquadCoeffs::peak      (fs, juce::jmin ((double) s.hmFreq, nyq), s.hmQ, s.hmGain),
                 BiquadCoeffs::highShelf (fs, juce::jmin ((double) s.hfFreq, nyq), s.hfGain) };
    }

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        for (auto& b : bands) b.reset();
        dirty = true;
    }

    void setParams (const Settings& s)
    {
        if (dirty || ! (s == settings))
        {
            settings = s;
            dirty = false;
            auto c = makeCoeffs (s, fs);
            for (size_t i = 0; i < 4; ++i)
                bands[i].c = c[i];
        }
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numCh = juce::jmin (2, buffer.getNumChannels());
        const bool active[4] = { std::abs (settings.lfGain) > 0.01f, std::abs (settings.lmGain) > 0.01f,
                                 std::abs (settings.hmGain) > 0.01f, std::abs (settings.hfGain) > 0.01f };

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* p = buffer.getWritePointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float x = p[i];
                for (size_t b = 0; b < 4; ++b)
                    if (active[b]) x = bands[b].process (x, ch);
                p[i] = x;
            }
        }
    }

private:
    double fs = 44100.0;
    Settings settings;
    bool dirty = true;
    std::array<Biquad<2>, 4> bands;
};
