#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs
{
    // Global
    inline constexpr const char* oversampling = "oversampling";

    // Input / HPF
    inline constexpr const char* inTrim    = "in_trim";
    inline constexpr const char* hpfOn     = "hpf_on";
    inline constexpr const char* hpfFreq   = "hpf_freq";
    inline constexpr const char* hpfSlope  = "hpf_slope";

    // Preamp
    inline constexpr const char* preOn     = "pre_on";
    inline constexpr const char* preModel  = "pre_model";
    inline constexpr const char* preDrive  = "pre_drive";
    inline constexpr const char* preColor  = "pre_color";
    inline constexpr const char* preOut    = "pre_out";

    // Gate / Expander
    inline constexpr const char* gateOn      = "gate_on";
    inline constexpr const char* gateThresh  = "gate_thresh";
    inline constexpr const char* gateRatio   = "gate_ratio";
    inline constexpr const char* gateAttack  = "gate_attack";
    inline constexpr const char* gateRelease = "gate_release";
    inline constexpr const char* gateRange   = "gate_range";

    // Equalizer
    inline constexpr const char* eqOn     = "eq_on";
    inline constexpr const char* eqPos    = "eq_pos";
    inline constexpr const char* lfFreq   = "eq_lf_freq";
    inline constexpr const char* lfGain   = "eq_lf_gain";
    inline constexpr const char* lmFreq   = "eq_lm_freq";
    inline constexpr const char* lmGain   = "eq_lm_gain";
    inline constexpr const char* lmQ      = "eq_lm_q";
    inline constexpr const char* hmFreq   = "eq_hm_freq";
    inline constexpr const char* hmGain   = "eq_hm_gain";
    inline constexpr const char* hmQ      = "eq_hm_q";
    inline constexpr const char* hfFreq   = "eq_hf_freq";
    inline constexpr const char* hfGain   = "eq_hf_gain";

    // Compressor
    inline constexpr const char* compOn        = "comp_on";
    inline constexpr const char* compModel     = "comp_model";
    inline constexpr const char* compThresh    = "comp_thresh";
    inline constexpr const char* compRatio     = "comp_ratio";
    inline constexpr const char* compAttack    = "comp_attack";
    inline constexpr const char* compRelease   = "comp_release";
    inline constexpr const char* compKnee      = "comp_knee";
    inline constexpr const char* compMakeup    = "comp_makeup";
    inline constexpr const char* compMix       = "comp_mix";
    inline constexpr const char* compScHpf     = "comp_sc_hpf";
    inline constexpr const char* compAutoRel   = "comp_auto_release";
    inline constexpr const char* compAutoMakeup= "comp_auto_makeup";

    // Limiter / Output
    inline constexpr const char* limOn      = "lim_on";
    inline constexpr const char* limMode    = "lim_mode";
    inline constexpr const char* limCeiling = "lim_ceiling";
    inline constexpr const char* limRelease = "lim_release";
    inline constexpr const char* limOutput  = "lim_output";
    inline constexpr const char* limTarget  = "lim_target";
}

namespace Choices
{
    inline const juce::StringArray oversampling { "1x", "2x", "4x", "8x" };
    inline const juce::StringArray hpfSlope     { "6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct" };
    inline const juce::StringArray preModel     { "British Class-A", "American Console", "Tube Warmth", "Tape" };
    inline const juce::StringArray eqPos        { "Pre-Comp", "Post-Comp" };
    inline const juce::StringArray compModel    { "VCA Clean", "FET Punch", "Opto Smooth" };
    inline const juce::StringArray limMode      { "Ceiling", "LUFS Target" };
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
