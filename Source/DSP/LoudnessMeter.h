#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <array>
#include "Biquad.h"

/** ITU-R BS.1770-4 / EBU R128 loudness meter: momentary (400 ms),
    short-term (3 s) and gated integrated loudness. Allocation-free. */
class LoudnessMeter
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        // Pre-filter (high shelf) and RLB high-pass, recomputed for any sample rate.
        {
            const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
            const double K = std::tan (BiquadCoeffs::pi * f0 / fs);
            const double Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416);
            const double a0 = 1.0 + K / Q + K * K;
            stage1.c = { (Vh + Vb * K / Q + K * K) / a0, 2.0 * (K * K - Vh) / a0, (Vh - Vb * K / Q + K * K) / a0,
                         2.0 * (K * K - 1.0) / a0, (1.0 - K / Q + K * K) / a0 };
        }
        {
            const double f0 = 38.13547087602444, Q = 0.5003270373238773;
            const double K = std::tan (BiquadCoeffs::pi * f0 / fs);
            const double a0 = 1.0 + K / Q + K * K;
            stage2.c = { 1.0, -2.0, 1.0, 2.0 * (K * K - 1.0) / a0, (1.0 - K / Q + K * K) / a0 };
        }
        blockLen = juce::jmax (1, (int) std::round (fs * 0.1));
        reset();
    }

    void reset()
    {
        stage1.reset(); stage2.reset();
        blocks.fill (0.0);
        blockAcc = 0.0; blockCount = 0; blocksSeen = 0; writeIdx = 0;
        resetIntegrated();
        momentary.store (-100.0f); shortTerm.store (-100.0f);
    }

    void resetIntegrated()
    {
        histCount.fill (0);
        histEnergy.fill (0.0);
        integrated.store (-100.0f);
    }

    void requestResetIntegrated() { resetRequested.store (true); }

    void process (const juce::AudioBuffer<float>& buffer)
    {
        if (resetRequested.exchange (false))
            resetIntegrated();

        const int numCh = juce::jmin (2, buffer.getNumChannels());
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            double sum = 0.0;
            for (int ch = 0; ch < numCh; ++ch)
            {
                const double y = stage2.process (stage1.process (buffer.getSample (ch, i), ch), ch);
                sum += y * y;
            }
            if (numCh == 1) sum *= 2.0; // mono played on two speakers counts as stereo

            blockAcc += sum;
            if (++blockCount >= blockLen)
                finishBlock();
        }
    }

    std::atomic<float> momentary { -100.0f }, shortTerm { -100.0f }, integrated { -100.0f };

private:
    static constexpr int numBlocks = 30;      // 30 x 100 ms = 3 s
    static constexpr int histBins = 800;      // -70 .. +10 LUFS in 0.1 dB steps

    static double toLufs (double meanSquare) { return meanSquare > 0.0 ? -0.691 + 10.0 * std::log10 (meanSquare) : -100.0; }

    void finishBlock()
    {
        blocks[(size_t) writeIdx] = blockAcc / (double) blockLen;
        writeIdx = (writeIdx + 1) % numBlocks;
        blockAcc = 0.0;
        blockCount = 0;
        blocksSeen = juce::jmin (blocksSeen + 1, 1 << 30);

        auto meanOfLast = [this] (int count)
        {
            double s = 0.0;
            for (int k = 1; k <= count; ++k)
                s += blocks[(size_t) ((writeIdx - k + numBlocks) % numBlocks)];
            return s / count;
        };

        if (blocksSeen >= 4)
        {
            const double m = meanOfLast (4);
            const double lm = toLufs (m);
            momentary.store ((float) lm);

            // Gating block (400 ms, 75 % overlap) -> histogram for integrated loudness
            if (lm > -70.0)
            {
                const int bin = juce::jlimit (0, histBins - 1, (int) ((lm + 70.0) * 10.0));
                histCount[(size_t) bin]++;
                histEnergy[(size_t) bin] += m;
            }
            updateIntegrated();
        }

        shortTerm.store ((float) toLufs (meanOfLast (juce::jmin (blocksSeen, numBlocks))));
    }

    void updateIntegrated()
    {
        double e = 0.0; long long c = 0;
        for (int b = 0; b < histBins; ++b) { e += histEnergy[(size_t) b]; c += histCount[(size_t) b]; }
        if (c == 0) { integrated.store (-100.0f); return; }

        const double relGate = toLufs (e / (double) c) - 10.0;
        const int firstBin = juce::jlimit (0, histBins, (int) std::ceil ((relGate + 70.0) * 10.0));
        e = 0.0; c = 0;
        for (int b = firstBin; b < histBins; ++b) { e += histEnergy[(size_t) b]; c += histCount[(size_t) b]; }
        integrated.store (c > 0 ? (float) toLufs (e / (double) c) : -100.0f);
    }

    double fs = 48000.0;
    Biquad<2> stage1, stage2;
    std::array<double, numBlocks> blocks {};
    std::array<long long, histBins> histCount {};
    std::array<double, histBins> histEnergy {};
    double blockAcc = 0.0;
    int blockLen = 4800, blockCount = 0, blocksSeen = 0, writeIdx = 0;
    std::atomic<bool> resetRequested { false };
};
