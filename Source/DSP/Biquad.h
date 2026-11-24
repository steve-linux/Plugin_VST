#pragma once

#include <cmath>
#include <complex>
#include <array>

/** Small RBJ-cookbook biquad (transposed direct form II), usable both on the
    audio thread (no allocation) and in the GUI to plot magnitude responses. */
struct BiquadCoeffs
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;

    static constexpr double pi = 3.14159265358979323846;

    static BiquadCoeffs identity() { return {}; }

    static BiquadCoeffs highPass (double fs, double f, double q)
    {
        const double w = 2.0 * pi * f / fs, c = std::cos (w), s = std::sin (w), alpha = s / (2.0 * q);
        const double a0 = 1.0 + alpha;
        return { (1.0 + c) * 0.5 / a0, -(1.0 + c) / a0, (1.0 + c) * 0.5 / a0, -2.0 * c / a0, (1.0 - alpha) / a0 };
    }

    static BiquadCoeffs lowPass (double fs, double f, double q)
    {
        const double w = 2.0 * pi * f / fs, c = std::cos (w), s = std::sin (w), alpha = s / (2.0 * q);
        const double a0 = 1.0 + alpha;
        return { (1.0 - c) * 0.5 / a0, (1.0 - c) / a0, (1.0 - c) * 0.5 / a0, -2.0 * c / a0, (1.0 - alpha) / a0 };
    }

    static BiquadCoeffs firstOrderHighPass (double fs, double f)
    {
        const double k = std::tan (pi * f / fs);
        const double a0 = k + 1.0;
        return { 1.0 / a0, -1.0 / a0, 0.0, (k - 1.0) / a0, 0.0 };
    }

    static BiquadCoeffs peak (double fs, double f, double q, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2.0 * pi * f / fs, c = std::cos (w), s = std::sin (w), alpha = s / (2.0 * q);
        const double a0 = 1.0 + alpha / A;
        return { (1.0 + alpha * A) / a0, -2.0 * c / a0, (1.0 - alpha * A) / a0, -2.0 * c / a0, (1.0 - alpha / A) / a0 };
    }

    static BiquadCoeffs lowShelf (double fs, double f, double gainDb, double q = 0.7071)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2.0 * pi * f / fs, c = std::cos (w), s = std::sin (w), alpha = s / (2.0 * q);
        const double sq = 2.0 * std::sqrt (A) * alpha;
        const double a0 = (A + 1.0) + (A - 1.0) * c + sq;
        return { A * ((A + 1.0) - (A - 1.0) * c + sq) / a0,
                 2.0 * A * ((A - 1.0) - (A + 1.0) * c) / a0,
                 A * ((A + 1.0) - (A - 1.0) * c - sq) / a0,
                 -2.0 * ((A - 1.0) + (A + 1.0) * c) / a0,
                 ((A + 1.0) + (A - 1.0) * c - sq) / a0 };
    }

    static BiquadCoeffs highShelf (double fs, double f, double gainDb, double q = 0.7071)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2.0 * pi * f / fs, c = std::cos (w), s = std::sin (w), alpha = s / (2.0 * q);
        const double sq = 2.0 * std::sqrt (A) * alpha;
        const double a0 = (A + 1.0) - (A - 1.0) * c + sq;
        return { A * ((A + 1.0) + (A - 1.0) * c + sq) / a0,
                 -2.0 * A * ((A - 1.0) + (A + 1.0) * c) / a0,
                 A * ((A + 1.0) + (A - 1.0) * c - sq) / a0,
                 2.0 * ((A - 1.0) - (A + 1.0) * c) / a0,
                 ((A + 1.0) - (A - 1.0) * c - sq) / a0 };
    }

    double magnitude (double f, double fs) const
    {
        const std::complex<double> z1 = std::polar (1.0, -2.0 * pi * f / fs);
        const std::complex<double> z2 = z1 * z1;
        return std::abs ((b0 + b1 * z1 + b2 * z2) / (1.0 + a1 * z1 + a2 * z2));
    }
};

template <int MaxChannels = 2>
struct Biquad
{
    BiquadCoeffs c;
    std::array<double, (size_t) MaxChannels> z1 {}, z2 {};

    void reset() { z1.fill (0.0); z2.fill (0.0); }

    inline float process (float xf, int ch) noexcept
    {
        const double x = xf;
        const auto c_ = (size_t) ch;
        const double y = c.b0 * x + z1[c_];
        z1[c_] = c.b1 * x - c.a1 * y + z2[c_];
        z2[c_] = c.b2 * x - c.a2 * y;
        return (float) y;
    }
};
