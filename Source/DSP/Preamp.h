#pragma once

#include <juce_dsp/juce_dsp.h>
#include "Biquad.h"

/** Saturating preamp with several analogue-inspired models and oversampling. */
class Preamp
{
public:
    enum Model { britishClassA = 0, americanConsole, tube, tape };

    /** Static waveshaper shared by the DSP and the GUI harmonic estimate.
        drive01 / color01 are in [0, 1]. Input is expected pre-gained. */
    static float shape (int model, float x, float color01) noexcept
    {
        switch (model)
        {
            case britishClassA:
            {
                // Soft, mostly-even harmonics: asymmetric tanh with a 2nd-order bias.
                const float bias = 0.25f * color01;
                const float y = std::tanh (x + bias * x * x);
                return y - std::tanh (bias * 0.0f);
            }
            case americanConsole:
            {
                // Firmer knee, mostly odd harmonics.
                const float k = 2.5f + 2.0f * color01;
                return x / std::pow (1.0f + std::pow (std::abs (x), k), 1.0f / k);
            }
            case tube:
            {
                // Asymmetric triode-like curve: positive side saturates earlier.
                const float asym = 0.3f + 0.5f * color01;
                return x >= 0.0f ? std::tanh (x * (1.0f + asym)) / (1.0f + asym)
                                 : std::tanh (x * (1.0f - 0.5f * asym)) / (1.0f - 0.5f * asym);
            }
            default:
            {
                // Tape: gentle arctan compression.
                constexpr float twoOverPi = 0.63661977f;
                const float hardness = 1.0f + color01;
                return twoOverPi * std::atan (x * hardness * 1.5707963f) / hardness;
            }
        }
    }

    /** Pre-gain (linear) applied before the shaper for a drive in [0, 1]. */
    static float driveGain (float drive01) noexcept { return juce::Decibels::decibelsToGain (24.0f * drive01); }

    /** Rough level compensation so that drive mostly changes colour, not loudness. */
    static float compensation (float drive01) noexcept { return juce::Decibels::decibelsToGain (-14.0f * drive01); }

    void prepare (double sampleRate, int maxBlockSize, int numChannels)
    {
        fs = sampleRate;
        for (int i = 0; i < 3; ++i)
        {
            oversamplers[(size_t) i] = std::make_unique<juce::dsp::Oversampling<float>> (
                (size_t) numChannels, (size_t) (i + 1),
                juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
            oversamplers[(size_t) i]->initProcessing ((size_t) maxBlockSize);
        }
        drive.reset (sampleRate, 0.05);
        color.reset (sampleRate, 0.05);
        out.reset (sampleRate, 0.02);
        dcBlock.c = BiquadCoeffs::firstOrderHighPass (sampleRate, 10.0);
        dcBlock.reset();
        tilt.reset();
        lastModel = -1;
        lastColor = -1.0f;
    }

    void reset()
    {
        for (auto& o : oversamplers) if (o) o->reset();
        dcBlock.reset();
        tilt.reset();
    }

    /** 0 = 1x, 1 = 2x, 2 = 4x, 3 = 8x */
    void setOversampling (int index)
    {
        if (index != osIndex)
        {
            osIndex = index;
            if (osIndex > 0) oversamplers[(size_t) osIndex - 1]->reset();
        }
    }

    float getLatencySamples() const
    {
        return osIndex > 0 ? oversamplers[(size_t) osIndex - 1]->getLatencyInSamples() : 0.0f;
    }

    void setParams (int modelIndex, float drivePct, float colorPct, float outDb)
    {
        model = modelIndex;
        drive.setTargetValue (drivePct * 0.01f);
        color.setTargetValue (colorPct * 0.01f);
        out.setTargetValue (juce::Decibels::decibelsToGain (outDb));

        const float c = colorPct * 0.01f;
        if (model != lastModel || std::abs (c - lastColor) > 0.001f)
        {
            lastModel = model;
            lastColor = c;
            // Each model adds a subtle tonal tilt scaled by colour.
            switch (model)
            {
                case britishClassA:   tilt.c = BiquadCoeffs::lowShelf  (fs, 120.0, 1.5 * c); break;
                case americanConsole: tilt.c = BiquadCoeffs::peak      (fs, 2500.0, 0.8, 1.0 * c); break;
                case tube:            tilt.c = BiquadCoeffs::highShelf (fs, 9000.0, -1.5 * c); break;
                default:              tilt.c = BiquadCoeffs::highShelf (fs, 12000.0, -3.0 * c); break;
            }
        }
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numCh = juce::jmin (2, buffer.getNumChannels());
        const int n = buffer.getNumSamples();

        juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numCh, (size_t) n);

        // Per-sample smoothed values at base rate, held across the oversampled sub-samples.
        if (osIndex == 0)
        {
            for (int i = 0; i < n; ++i)
            {
                const float d = drive.getNextValue(), c = color.getNextValue();
                const float pre = driveGain (d), post = compensation (d);
                for (int ch = 0; ch < numCh; ++ch)
                {
                    auto* p = block.getChannelPointer ((size_t) ch);
                    p[i] = shape (model, p[i] * pre, c) * post;
                }
            }
        }
        else
        {
            auto& os = *oversamplers[(size_t) osIndex - 1];
            auto up = os.processSamplesUp (block);
            const int factor = (int) os.getOversamplingFactor();
            const int upN = (int) up.getNumSamples();

            for (int i = 0; i < upN; i += factor)
            {
                const float d = drive.getNextValue(), c = color.getNextValue();
                const float pre = driveGain (d), post = compensation (d);
                for (int ch = 0; ch < numCh; ++ch)
                {
                    auto* p = up.getChannelPointer ((size_t) ch);
                    for (int k = 0; k < factor; ++k)
                        p[i + k] = shape (model, p[i + k] * pre, c) * post;
                }
            }
            os.processSamplesDown (block);
        }

        for (int i = 0; i < n; ++i)
        {
            const float g = out.getNextValue();
            for (int ch = 0; ch < numCh; ++ch)
            {
                auto* p = block.getChannelPointer ((size_t) ch);
                p[i] = tilt.process (dcBlock.process (p[i], ch), ch) * g;
            }
        }
    }

    /** Fixed latency so the plugin reports a stable value: always the latency of
        the currently selected oversampler, rounded. */

private:
    double fs = 44100.0;
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3> oversamplers;
    int osIndex = 0, model = 0, lastModel = -1;
    float lastColor = -1.0f;
    juce::SmoothedValue<float> drive { 0.0f }, color { 0.0f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> out { 1.0f };
    Biquad<2> dcBlock, tilt;
};
