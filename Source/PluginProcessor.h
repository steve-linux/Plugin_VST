#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "DSP/InputStage.h"
#include "DSP/Preamp.h"
#include "DSP/Gate.h"
#include "DSP/Equalizer.h"
#include "DSP/Compressor.h"
#include "DSP/Limiter.h"
#include "DSP/LoudnessMeter.h"
#include "DSP/AnalyzerFifo.h"

class ChannelStripProcessor : public juce::AudioProcessor
{
public:
    ChannelStripProcessor();
    ~ChannelStripProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // --- metering, read by the editor --------------------------------------
    std::atomic<float> inputPeakDb { -120.0f };
    std::atomic<float> limiterAutoGainDb { 0.0f };
    Gate gate;
    Compressor compressor;
    Limiter limiter;
    LoudnessMeter inputLoudness, outputLoudness;
    AnalyzerFifo analyzerFifo;

    void resetIntegratedLoudness() { outputLoudness.requestResetIntegrated(); inputLoudness.requestResetIntegrated(); }
    double getCurrentSampleRate() const { return currentSampleRate; }

private:
    void updateParameters();
    void updateLatency();

    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    bool  getBool (const char* id) const { return get (id) > 0.5f; }
    int   getInt (const char* id) const { return juce::roundToInt (get (id)); }

    InputStage inputStage;
    Preamp preamp;
    Equalizer equalizer;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGain { 1.0f }, autoGain { 1.0f };
    juce::AudioBuffer<float> monoScratch;

    double currentSampleRate = 44100.0;
    int currentProgram = 0;
    int lastOversampling = -1;
    float autoGainDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelStripProcessor)
};
