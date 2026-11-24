#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

ChannelStripProcessor::ChannelStripProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

bool ChannelStripProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void ChannelStripProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const int numCh = juce::jmax (1, getTotalNumOutputChannels());

    inputStage.prepare (sampleRate);
    preamp.prepare (sampleRate, samplesPerBlock, numCh);
    gate.prepare (sampleRate);
    equalizer.prepare (sampleRate);
    compressor.prepare (sampleRate);
    limiter.prepare (sampleRate, numCh);
    inputLoudness.prepare (sampleRate);
    outputLoudness.prepare (sampleRate);

    outputGain.reset (sampleRate, 0.02);
    autoGain.reset (sampleRate, 0.1);
    autoGainDb = 0.0f;
    monoScratch.setSize (1, samplesPerBlock);

    lastOversampling = -1;
    updateParameters();
    updateLatency();
}

void ChannelStripProcessor::updateLatency()
{
    // Constant latency: oversampling filter + limiter look-ahead (always reported,
    // so toggling the limiter does not shift timing in the host).
    setLatencySamples (juce::roundToInt (preamp.getLatencySamples()) + limiter.getLatencySamples());
}

void ChannelStripProcessor::updateParameters()
{
    using namespace ParamIDs;

    inputStage.setParams (get (inTrim), getBool (hpfOn), get (hpfFreq), getInt (hpfSlope));

    const int os = getInt (oversampling);
    preamp.setOversampling (os);
    if (os != lastOversampling)
    {
        lastOversampling = os;
        updateLatency();
    }
    preamp.setParams (getInt (preModel), get (preDrive), get (preColor), get (preOut));

    gate.setParams (get (gateThresh), get (gateRatio), get (gateAttack), get (gateRelease), get (gateRange));

    Equalizer::Settings eq;
    eq.lfFreq = get (lfFreq); eq.lfGain = get (lfGain);
    eq.lmFreq = get (lmFreq); eq.lmGain = get (lmGain); eq.lmQ = get (lmQ);
    eq.hmFreq = get (hmFreq); eq.hmGain = get (hmGain); eq.hmQ = get (hmQ);
    eq.hfFreq = get (hfFreq); eq.hfGain = get (hfGain);
    equalizer.setParams (eq);

    Compressor::Settings c;
    c.model = getInt (compModel);
    c.thresh = get (compThresh); c.ratio = get (compRatio);
    c.attackMs = get (compAttack); c.releaseMs = get (compRelease);
    c.knee = get (compKnee); c.makeup = get (compMakeup); c.mix = get (compMix);
    c.scHpf = get (compScHpf);
    c.autoRelease = getBool (compAutoRel); c.autoMakeup = getBool (compAutoMakeup);
    compressor.setParams (c);

    limiter.setParams (get (limCeiling), get (limRelease));
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (get (limOutput)));
}

void ChannelStripProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    using namespace ParamIDs;
    juce::ScopedNoDenormals noDenormals;

    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int n = buffer.getNumSamples();
    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, n);

    updateParameters();

    // Input metering (pre-processing)
    inputPeakDb.store (juce::Decibels::gainToDecibels (buffer.getMagnitude (0, n), -120.0f));
    inputLoudness.process (buffer);

    inputStage.process (buffer);

    if (getBool (preOn))  preamp.process (buffer);

    if (getBool (gateOn)) gate.process (buffer);
    else                  { gate.meterGr.store (0.0f); }

    const bool eqOnNow = getBool (eqOn);
    const bool eqPre = getInt (eqPos) == 0;

    if (eqOnNow && eqPre) equalizer.process (buffer);

    if (getBool (compOn)) compressor.process (buffer);
    else                  { compressor.meterGr.store (0.0f); }

    if (eqOnNow && ! eqPre) equalizer.process (buffer);

    // Limiter / output
    const bool limiterOn = getBool (limOn);
    if (limiterOn && getInt (limMode) == 1)
    {
        // LUFS target: slowly ride the gain into the limiter towards the target short-term loudness.
        const float st = outputLoudness.shortTerm.load();
        if (st > -60.0f)
        {
            const float err = get (limTarget) - st;
            const float maxStep = 1.0f * (float) n / (float) currentSampleRate; // max 1 dB/s
            autoGainDb = juce::jlimit (-18.0f, 18.0f, autoGainDb + juce::jlimit (-maxStep, maxStep, err * 0.5f * maxStep));
        }
    }
    else
    {
        autoGainDb = 0.0f;
    }
    autoGain.setTargetValue (juce::Decibels::decibelsToGain (autoGainDb));
    limiterAutoGainDb.store (autoGainDb);

    if (autoGain.isSmoothing() || std::abs (autoGainDb) > 0.001f)
        autoGain.applyGain (buffer, n);

    if (limiterOn) limiter.process (buffer);
    else           { limiter.meterGr.store (0.0f); }

    // Output gain applied before the limiter's ceiling would defeat it, so it comes after
    // and can only be used to trim the final level.
    outputGain.applyGain (buffer, n);

    outputLoudness.process (buffer);

    // Analyzer feed (mono sum)
    if (monoScratch.getNumSamples() < n)
        monoScratch.setSize (1, n, false, false, true);
    auto* mono = monoScratch.getWritePointer (0);
    const int chs = juce::jmin (2, numOut);
    for (int i = 0; i < n; ++i)
    {
        float s = 0.0f;
        for (int ch = 0; ch < chs; ++ch) s += buffer.getSample (ch, i);
        mono[i] = s / (float) chs;
    }
    analyzerFifo.push (mono, n);
}

//==============================================================================
int ChannelStripProcessor::getNumPrograms() { return (int) getFactoryPresets().size(); }

const juce::String ChannelStripProcessor::getProgramName (int index)
{
    const auto& p = getFactoryPresets();
    return juce::isPositiveAndBelow (index, (int) p.size()) ? juce::String (p[(size_t) index].name) : juce::String();
}

void ChannelStripProcessor::setCurrentProgram (int index)
{
    const auto& presets = getFactoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size()))
        return;

    currentProgram = index;

    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (rp->getParameterID() != ParamIDs::oversampling)
                rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& [id, value] : presets[(size_t) index].values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
}

void ChannelStripProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ChannelStripProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentProgram = state.getProperty ("program", 0);
            apvts.replaceState (state);
        }
}

juce::AudioProcessorEditor* ChannelStripProcessor::createEditor()
{
    return new ChannelStripEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChannelStripProcessor();
}
