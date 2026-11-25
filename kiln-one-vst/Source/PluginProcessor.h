#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

// One-knob mastering: the LOUD parameter drives threshold, ratio and makeup gain together.
class KilnProcessor : public juce::AudioProcessor
{
public:
    KilnProcessor();
    ~KilnProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Kiln One"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    juce::AudioProcessorParameter* getBypassParameter() const override { return bypassParam; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    struct Preset { const char* group; const char* name; std::vector<std::pair<const char*, float>> values; };
    static const std::vector<Preset>& presets();

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> grDb { 0.f }, peakL { 0.f }, peakR { 0.f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // gain computer with soft knee, linked stereo detection
    struct Comp
    {
        float thr = -20.f, ratio = 2.f, knee = 6.f, attC = 0.f, relC = 0.f, env = 0.f;
        void set (float t, float r, float k, float attSec, float relSec, double sr)
        {
            thr = t; ratio = r; knee = k;
            attC = std::exp (-1.f / (float) (juce::jmax (attSec, 1.0e-4f) * sr));
            relC = std::exp (-1.f / (float) (juce::jmax (relSec, 1.0e-3f) * sr));
        }
        float gain (float l, float r)
        {
            const float lvl = juce::Decibels::gainToDecibels (juce::jmax (std::abs (l), std::abs (r)), -120.f);
            const float over = lvl - thr;
            float out = lvl;
            if (2.f * over > knee)              out = thr + over / ratio;
            else if (2.f * over > -knee)        out = lvl + (1.f / ratio - 1.f) * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.f * knee);
            const float gr = juce::jmax (0.f, lvl - out);
            const float c = gr > env ? attC : relC;
            env = c * env + (1.f - c) * gr;
            return juce::Decibels::decibelsToGain (-env);
        }
    };

    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    void updateStaticFilters();

    double sr = 44100.0;
    int currentProgram = 0;
    juce::AudioParameterBool* bypassParam = nullptr;

    Filter low[2], mid[2], high[2], air[2], calmPk[2], calmSh[2], tapeLp[2];
    juce::dsp::LinkwitzRileyFilter<float> xLowLP, xLowHP, xHighLP, xHighHP;
    Comp single, bandComp[3];
    juce::dsp::Oversampling<float> oversampling { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true };
    juce::dsp::Limiter<float> limiter;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 8192 };
    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float> inGain, wetGain, bypassMix;

    // cached parameter values so filter coefficients are only rebuilt on change
    float lastLow = 1e9f, lastMid = 1e9f, lastHigh = 1e9f, lastCalm = 1e9f;
    int lastBand = -1, lastColor = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KilnProcessor)
};
