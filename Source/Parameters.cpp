#include "Parameters.h"

namespace
{
    using APF = juce::AudioParameterFloat;
    using APB = juce::AudioParameterBool;
    using APC = juce::AudioParameterChoice;
    using Range = juce::NormalisableRange<float>;

    juce::String fmt (float v, int decimals, const juce::String& unit)
    {
        return juce::String (v, decimals) + (unit.isEmpty() ? "" : " " + unit);
    }

    juce::String fmtDb (float v, int)   { return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " dB"; }
    juce::String fmtHz (float v, int)
    {
        if (v >= 1000.0f) return juce::String (v / 1000.0f, v >= 10000.0f ? 1 : 2) + " kHz";
        return juce::String (juce::roundToInt (v)) + " Hz";
    }
    juce::String fmtMs (float v, int)
    {
        if (v >= 1000.0f) return juce::String (v / 1000.0f, 2) + " s";
        return fmt (v, v < 10.0f ? 2 : (v < 100.0f ? 1 : 0), "ms");
    }
    juce::String fmtPct (float v, int)   { return juce::String (juce::roundToInt (v)) + " %"; }
    juce::String fmtRatio (float v, int) { return juce::String (v, 1) + " : 1"; }
    juce::String fmtQ (float v, int)     { return juce::String (v, 2); }
    juce::String fmtLufs (float v, int)  { return juce::String (v, 1) + " LUFS"; }

    Range freqRange (float lo, float hi) { Range r (lo, hi, 0.0f); r.setSkewForCentre (std::sqrt (lo * hi)); return r; }
    Range timeRange (float lo, float hi, float centre) { Range r (lo, hi, 0.0f); r.setSkewForCentre (centre); return r; }

    template <typename Fn>
    std::unique_ptr<APF> makeFloat (const char* id, const juce::String& name, Range range, float def, Fn formatter)
    {
        return std::make_unique<APF> (juce::ParameterID { id, 1 }, name, range, def,
                                      juce::AudioParameterFloatAttributes().withStringFromValueFunction (formatter));
    }

    std::unique_ptr<APB> makeBool (const char* id, const juce::String& name, bool def)
    {
        return std::make_unique<APB> (juce::ParameterID { id, 1 }, name, def);
    }

    std::unique_ptr<APC> makeChoice (const char* id, const juce::String& name, const juce::StringArray& c, int def)
    {
        return std::make_unique<APC> (juce::ParameterID { id, 1 }, name, c, def);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace ParamIDs;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (makeChoice (oversampling, "Oversampling", Choices::oversampling, 2));

    // Input / HPF
    layout.add (makeFloat (inTrim, "Input Trim", Range (-24.0f, 24.0f, 0.1f), 0.0f, fmtDb));
    layout.add (makeBool  (hpfOn, "HPF On", true));
    layout.add (makeFloat (hpfFreq, "HPF Freq", freqRange (20.0f, 500.0f), 80.0f, fmtHz));
    layout.add (makeChoice (hpfSlope, "HPF Slope", Choices::hpfSlope, 1));

    // Preamp
    layout.add (makeBool  (preOn, "Preamp On", true));
    layout.add (makeChoice (preModel, "Preamp Model", Choices::preModel, 0));
    layout.add (makeFloat (preDrive, "Preamp Drive", Range (0.0f, 100.0f, 0.1f), 45.0f, fmtPct));
    layout.add (makeFloat (preColor, "Preamp Color", Range (0.0f, 100.0f, 0.1f), 100.0f, fmtPct));
    layout.add (makeFloat (preOut, "Preamp Out", Range (-24.0f, 24.0f, 0.1f), 0.0f, fmtDb));

    // Gate
    layout.add (makeBool  (gateOn, "Gate On", false));
    layout.add (makeFloat (gateThresh, "Gate Threshold", Range (-80.0f, 0.0f, 0.1f), -55.0f, fmtDb));
    layout.add (makeFloat (gateRatio, "Gate Ratio", timeRange (1.0f, 20.0f, 4.0f), 3.0f, fmtRatio));
    layout.add (makeFloat (gateAttack, "Gate Attack", timeRange (0.05f, 50.0f, 2.0f), 0.5f, fmtMs));
    layout.add (makeFloat (gateRelease, "Gate Release", timeRange (5.0f, 2000.0f, 150.0f), 120.0f, fmtMs));
    layout.add (makeFloat (gateRange, "Gate Range", Range (-80.0f, 0.0f, 0.1f), -30.0f, fmtDb));

    // EQ
    layout.add (makeBool  (eqOn, "EQ On", true));
    layout.add (makeChoice (eqPos, "EQ Position", Choices::eqPos, 0));
    layout.add (makeFloat (lfFreq, "LF Freq", freqRange (20.0f, 500.0f), 80.0f, fmtHz));
    layout.add (makeFloat (lfGain, "LF Gain", Range (-18.0f, 18.0f, 0.1f), 0.0f, fmtDb));
    layout.add (makeFloat (lmFreq, "LM Freq", freqRange (80.0f, 2500.0f), 400.0f, fmtHz));
    layout.add (makeFloat (lmGain, "LM Gain", Range (-18.0f, 18.0f, 0.1f), 0.0f, fmtDb));
    layout.add (makeFloat (lmQ, "LM Q", timeRange (0.2f, 10.0f, 1.0f), 1.0f, fmtQ));
    layout.add (makeFloat (hmFreq, "HM Freq", freqRange (500.0f, 12000.0f), 3000.0f, fmtHz));
    layout.add (makeFloat (hmGain, "HM Gain", Range (-18.0f, 18.0f, 0.1f), 0.0f, fmtDb));
    layout.add (makeFloat (hmQ, "HM Q", timeRange (0.2f, 10.0f, 1.0f), 1.0f, fmtQ));
    layout.add (makeFloat (hfFreq, "HF Freq", freqRange (1500.0f, 20000.0f), 10000.0f, fmtHz));
    layout.add (makeFloat (hfGain, "HF Gain", Range (-18.0f, 18.0f, 0.1f), 0.0f, fmtDb));

    // Compressor
    layout.add (makeBool  (compOn, "Comp On", true));
    layout.add (makeChoice (compModel, "Comp Model", Choices::compModel, 0));
    layout.add (makeFloat (compThresh, "Comp Threshold", Range (-60.0f, 0.0f, 0.1f), -28.0f, fmtDb));
    layout.add (makeFloat (compRatio, "Comp Ratio", timeRange (1.0f, 20.0f, 4.0f), 4.0f, fmtRatio));
    layout.add (makeFloat (compAttack, "Comp Attack", timeRange (0.1f, 100.0f, 10.0f), 10.0f, fmtMs));
    layout.add (makeFloat (compRelease, "Comp Release", timeRange (10.0f, 2000.0f, 150.0f), 150.0f, fmtMs));
    layout.add (makeFloat (compKnee, "Comp Knee", Range (0.0f, 24.0f, 0.1f), 6.0f, fmtDb));
    layout.add (makeFloat (compMakeup, "Comp Makeup", Range (-12.0f, 24.0f, 0.1f), 0.0f, fmtDb));
    layout.add (makeFloat (compMix, "Comp Mix", Range (0.0f, 100.0f, 0.1f), 100.0f, fmtPct));
    layout.add (makeFloat (compScHpf, "Comp SC HPF", freqRange (20.0f, 500.0f), 60.0f, fmtHz));
    layout.add (makeBool  (compAutoRel, "Comp Auto Release", false));
    layout.add (makeBool  (compAutoMakeup, "Comp Auto Makeup", false));

    // Limiter
    layout.add (makeBool  (limOn, "Limiter On", true));
    layout.add (makeChoice (limMode, "Limiter Mode", Choices::limMode, 1));
    layout.add (makeFloat (limCeiling, "Limiter Ceiling", Range (-12.0f, 0.0f, 0.1f), -0.3f, fmtDb));
    layout.add (makeFloat (limRelease, "Limiter Release", timeRange (1.0f, 1000.0f, 60.0f), 60.0f, fmtMs));
    layout.add (makeFloat (limOutput, "Output", Range (-24.0f, 24.0f, 0.1f), 0.0f, fmtDb));
    layout.add (makeFloat (limTarget, "LUFS Target", Range (-30.0f, -6.0f, 0.1f), -16.0f, fmtLufs));

    return layout;
}
