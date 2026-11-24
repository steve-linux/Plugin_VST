#pragma once

#include <vector>
#include <utility>
#include "Parameters.h"

/** Factory presets: every parameter not listed returns to its default. */
struct FactoryPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values; // real (denormalised) values; choices/bools as index
};

inline const std::vector<FactoryPreset>& getFactoryPresets()
{
    using namespace ParamIDs;
    static const std::vector<FactoryPreset> presets {
        { "Init", {} },
        { "Vocal - Clean Pop", {
            { hpfFreq, 90 }, { hpfSlope, 1 }, { preModel, 0 }, { preDrive, 30 }, { preColor, 70 },
            { lmFreq, 300 }, { lmGain, -2.0f }, { lmQ, 1.2f }, { hmFreq, 3500 }, { hmGain, 2.0f }, { hfFreq, 12000 }, { hfGain, 2.5f },
            { compModel, 0 }, { compThresh, -24 }, { compRatio, 3 }, { compAttack, 8 }, { compRelease, 120 }, { compMakeup, 4 },
            { limTarget, -16 } } },
        { "Vocal - Warm Opto", {
            { hpfFreq, 80 }, { preModel, 2 }, { preDrive, 40 }, { preColor, 80 },
            { lfFreq, 150 }, { lfGain, 1.0f }, { hmFreq, 2500 }, { hmGain, 1.5f },
            { compModel, 2 }, { compThresh, -26 }, { compRatio, 3 }, { compAttack, 15 }, { compRelease, 300 }, { compAutoMakeup, 1 },
            { limTarget, -16 } } },
        { "Drum Bus - Punch", {
            { hpfFreq, 30 }, { preModel, 1 }, { preDrive, 55 }, { preColor, 60 },
            { lfFreq, 70 }, { lfGain, 2.0f }, { lmFreq, 450 }, { lmGain, -2.5f }, { hfGain, 2.0f },
            { compModel, 1 }, { compThresh, -20 }, { compRatio, 4 }, { compAttack, 20 }, { compRelease, 80 }, { compMix, 60 }, { compMakeup, 3 },
            { limTarget, -12 } } },
        { "Kick - Tight", {
            { hpfFreq, 25 }, { gateOn, 1 }, { gateThresh, -40 }, { gateRatio, 10 }, { gateRelease, 80 }, { gateRange, -40 },
            { lfFreq, 60 }, { lfGain, 3.0f }, { lmFreq, 350 }, { lmGain, -4.0f }, { hmFreq, 4000 }, { hmGain, 3.0f },
            { compModel, 1 }, { compThresh, -18 }, { compRatio, 4 }, { compAttack, 25 }, { compRelease, 60 } } },
        { "Bass - Solid", {
            { hpfFreq, 35 }, { preModel, 3 }, { preDrive, 50 }, { preColor, 70 },
            { lfFreq, 90 }, { lfGain, 1.5f }, { lmFreq, 250 }, { lmGain, -2.0f }, { hmFreq, 900 }, { hmGain, 1.5f },
            { compModel, 2 }, { compThresh, -22 }, { compRatio, 4 }, { compAttack, 20 }, { compRelease, 200 }, { compAutoMakeup, 1 } } },
        { "Acoustic Guitar", {
            { hpfFreq, 100 }, { preModel, 0 }, { preDrive, 25 },
            { lmFreq, 250 }, { lmGain, -2.5f }, { hfFreq, 10000 }, { hfGain, 2.0f },
            { compThresh, -22 }, { compRatio, 2.5f }, { compAttack, 15 }, { compRelease, 150 }, { compAutoMakeup, 1 } } },
        { "Master - Gentle Glue", {
            { hpfFreq, 20 }, { hpfSlope, 0 }, { preModel, 3 }, { preDrive, 20 }, { preColor, 50 },
            { lfFreq, 60 }, { lfGain, 0.5f }, { hfFreq, 14000 }, { hfGain, 1.0f },
            { compModel, 0 }, { compThresh, -16 }, { compRatio, 2 }, { compAttack, 30 }, { compRelease, 200 }, { compKnee, 10 },
            { compAutoRel, 1 }, { compAutoMakeup, 1 }, { compScHpf, 90 },
            { limCeiling, -1.0f }, { limTarget, -14 } } },
    };
    return presets;
}
