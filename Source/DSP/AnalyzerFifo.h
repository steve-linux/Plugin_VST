#pragma once

#include <juce_core/juce_core.h>
#include <array>

/** Lock-free single-producer / single-consumer FIFO of mono samples
    from the audio thread to the GUI analyzer. */
class AnalyzerFifo
{
public:
    static constexpr int capacity = 1 << 15;

    void push (const float* data, int num) noexcept
    {
        int s1, n1, s2, n2;
        fifo.prepareToWrite (num, s1, n1, s2, n2);
        if (n1 > 0) std::copy (data, data + n1, buffer.begin() + s1);
        if (n2 > 0) std::copy (data + n1, data + n1 + n2, buffer.begin() + s2);
        fifo.finishedWrite (n1 + n2);
    }

    /** Pulls up to maxNum samples; returns how many were read. */
    int pull (float* dest, int maxNum) noexcept
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (juce::jmin (maxNum, fifo.getNumReady()), s1, n1, s2, n2);
        if (n1 > 0) std::copy (buffer.begin() + s1, buffer.begin() + s1 + n1, dest);
        if (n2 > 0) std::copy (buffer.begin() + s2, buffer.begin() + s2 + n2, dest + n1);
        fifo.finishedRead (n1 + n2);
        return n1 + n2;
    }

private:
    juce::AbstractFifo fifo { capacity };
    std::array<float, capacity> buffer {};
};
