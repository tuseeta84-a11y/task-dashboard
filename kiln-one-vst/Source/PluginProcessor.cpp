#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

namespace
{
    constexpr float kCeilBoost = -0.3f, kCeilTrue = -1.0f;
    float raw (AudioProcessorValueTreeState& s, const char* id) { return s.getRawParameterValue (id)->load(); }
}

AudioProcessorValueTreeState::ParameterLayout KilnProcessor::createLayout()
{
    using F = AudioParameterFloat; using C = AudioParameterChoice; using B = AudioParameterBool; using ID = ParameterID;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto dB = [] (float lo, float hi) { return NormalisableRange<float> (lo, hi, 0.1f); };
    l.add (std::make_unique<F> (ID { "in", 1 },   "In",   dB (-12, 12), 0.f));
    l.add (std::make_unique<F> (ID { "low", 1 },  "Low",  dB (-6, 6), 0.f));
    l.add (std::make_unique<F> (ID { "mid", 1 },  "Mid",  dB (-6, 6), 0.f));
    l.add (std::make_unique<F> (ID { "high", 1 }, "High", dB (-6, 6), 0.f));
    l.add (std::make_unique<C> (ID { "band", 1 }, "Mid Band", StringArray { "H", "M", "L" }, 1));
    l.add (std::make_unique<F> (ID { "loud", 1 }, "Loud", NormalisableRange<float> (0.f, 10.f, 0.01f), 4.f));
    l.add (std::make_unique<F> (ID { "wide", 1 }, "Wide", NormalisableRange<float> (0.f, 1.6f, 0.01f), 1.f));
    l.add (std::make_unique<F> (ID { "out", 1 },  "Out",  dB (-12, 6), 0.f));
    l.add (std::make_unique<C> (ID { "color", 1 }, "Colour", StringArray { "Off", "Thick", "Clean", "Tape" }, 0));
    l.add (std::make_unique<B> (ID { "multi", 1 },  "Multi", false));
    l.add (std::make_unique<B> (ID { "smooth", 1 }, "Smooth", false));
    l.add (std::make_unique<B> (ID { "calm", 1 },   "Calm", false));
    l.add (std::make_unique<B> (ID { "x2", 1 },     "Calm x2", false));
    l.add (std::make_unique<C> (ID { "mode", 1 }, "Output Mode", StringArray { "Boost", "True Peak", "Unity" }, 0));
    l.add (std::make_unique<B> (ID { "bypass", 1 }, "Bypass", false));
    return l;
}

KilnProcessor::KilnProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KILN", createLayout())
{
    bypassParam = dynamic_cast<AudioParameterBool*> (apvts.getParameter ("bypass"));
}

bool KilnProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == AudioChannelSet::stereo() && l.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void KilnProcessor::prepareToPlay (double rate, int blockSize)
{
    sr = rate;
    dsp::ProcessSpec spec { rate, (uint32) blockSize, 2 };
    oversampling.initProcessing ((size_t) blockSize);
    const int latency = (int) std::ceil (oversampling.getLatencyInSamples());
    setLatencySamples (latency);

    dryDelay.prepare (spec); dryDelay.setDelay ((float) latency);
    dryBuffer.setSize (2, blockSize);

    for (auto* x : { &xLowLP, &xLowHP, &xHighLP, &xHighHP }) { x->prepare (spec); }
    xLowLP.setType (dsp::LinkwitzRileyFilterType::lowpass);  xLowLP.setCutoffFrequency (200.f);
    xLowHP.setType (dsp::LinkwitzRileyFilterType::highpass); xLowHP.setCutoffFrequency (200.f);
    xHighLP.setType (dsp::LinkwitzRileyFilterType::lowpass);  xHighLP.setCutoffFrequency (2500.f);
    xHighHP.setType (dsp::LinkwitzRileyFilterType::highpass); xHighHP.setCutoffFrequency (2500.f);

    limiter.prepare (spec); limiter.setRelease (90.f);

    lastLow = lastMid = lastHigh = lastCalm = 1e9f; lastBand = lastColor = -1;
    for (int c = 0; c < 2; ++c)
    {
        for (auto* f : { &low[c], &mid[c], &high[c], &air[c], &calmPk[c], &calmSh[c], &tapeLp[c] }) f->reset();
        tapeLp[c].coefficients = Coeffs::makeLowPass (rate, jmin (13000.0, rate * 0.45), 0.5f);
    }
    single.env = 0; for (auto& b : bandComp) b.env = 0;
    inGain.reset (rate, 0.02);  inGain.setCurrentAndTargetValue (1.f);
    wetGain.reset (rate, 0.02); wetGain.setCurrentAndTargetValue (1.f);
    bypassMix.reset (rate, 0.02); bypassMix.setCurrentAndTargetValue (0.f);
    updateStaticFilters();
}

void KilnProcessor::updateStaticFilters()
{
    const float lo = raw (apvts, "low"), mi = raw (apvts, "mid"), hi = raw (apvts, "high");
    const int band = (int) raw (apvts, "band"), color = (int) raw (apvts, "color");
    const float calm = raw (apvts, "calm") > 0.5f ? (raw (apvts, "x2") > 0.5f ? 2.f : 1.f) : 0.f;
    const float nyq = (float) (sr * 0.45);

    if (lo != lastLow || color != lastColor)
        for (auto& f : low) f.coefficients = Coeffs::makeLowShelf (sr, 100.f, 0.7071f, Decibels::decibelsToGain (lo + (color == 1 ? 2.f : 0.f)));
    if (mi != lastMid || band != lastBand)
        for (auto& f : mid) f.coefficients = Coeffs::makePeakFilter (sr, band == 0 ? 3200.f : band == 1 ? 1100.f : 300.f, 0.8f, Decibels::decibelsToGain (mi));
    if (hi != lastHigh)
        for (auto& f : high) f.coefficients = Coeffs::makeHighShelf (sr, 8000.f, 0.7071f, Decibels::decibelsToGain (hi));
    if (color != lastColor)
        for (auto& f : air) f.coefficients = Coeffs::makeHighShelf (sr, jmin (12000.f, nyq), 0.7071f, Decibels::decibelsToGain (color == 2 ? 2.5f : 0.f));
    if (calm != lastCalm)
    {
        for (auto& f : calmPk) f.coefficients = Coeffs::makePeakFilter (sr, 3500.f, 0.9f, Decibels::decibelsToGain (-2.2f * calm));
        for (auto& f : calmSh) f.coefficients = Coeffs::makeHighShelf (sr, jmin (7000.f, nyq), 0.7071f, Decibels::decibelsToGain (-1.2f * calm));
    }
    lastLow = lo; lastMid = mi; lastHigh = hi; lastBand = band; lastColor = color; lastCalm = calm;
}

void KilnProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0) return;
    if (dryBuffer.getNumSamples() < n) dryBuffer.setSize (2, n, false, false, true);

    // dry copy, delayed to match the wet path latency
    for (int c = 0; c < 2; ++c)
    {
        dryBuffer.copyFrom (c, 0, buffer, c, 0, n);
        auto* d = dryBuffer.getWritePointer (c);
        for (int i = 0; i < n; ++i) { dryDelay.pushSample (c, d[i]); d[i] = dryDelay.popSample (c); }
    }

    updateStaticFilters();

    const float loud = raw (apvts, "loud"), wide = raw (apvts, "wide");
    const int color = (int) raw (apvts, "color"), mode = (int) raw (apvts, "mode");
    const bool multi = raw (apvts, "multi") > 0.5f, smooth = raw (apvts, "smooth") > 0.5f, bypass = raw (apvts, "bypass") > 0.5f;

    inGain.setTargetValue (Decibels::decibelsToGain (raw (apvts, "in")));
    bypassMix.setTargetValue (bypass ? 1.f : 0.f);

    // LOUD: threshold, ratio and makeup move together
    const float thr = -10.f - loud * 2.2f, ratio = 1.6f + loud * 0.5f;
    const float knee = smooth ? 24.f : 8.f, att = smooth ? 0.03f : 0.008f, rel = smooth ? 0.4f : 0.14f;
    single.set (thr, ratio, knee, att, rel, sr);
    bandComp[0].set (thr, ratio, knee, att * 1.8f, rel * 1.5f, sr);
    bandComp[1].set (thr, ratio, knee, att,        rel,        sr);
    bandComp[2].set (thr, ratio, knee, att * 0.5f, rel * 0.7f, sr);
    const float makeupDb = mode == 2 ? loud * 0.6f : loud * 2.f + (mode == 0 ? 2.f : 0.f);
    wetGain.setTargetValue (Decibels::decibelsToGain (makeupDb + raw (apvts, "out")));

    const float ceilDb = mode == 1 ? kCeilTrue : kCeilBoost;
    const float ceil = Decibels::decibelsToGain (ceilDb);
    limiter.setThreshold (ceilDb - 1.5f);

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    float* ch[2] = { L, R };

    // input gain and tone
    for (int i = 0; i < n; ++i)
    {
        const float g = inGain.getNextValue();
        for (int c = 0; c < 2; ++c)
        {
            float x = ch[c][i] * g;
            x = low[c].processSample (x);
            x = mid[c].processSample (x);
            x = high[c].processSample (x);
            x = air[c].processSample (x);
            x = calmPk[c].processSample (x);
            x = calmSh[c].processSample (x);
            ch[c][i] = x;
        }
    }

    // saturation at 2x oversampling (always run so latency never changes)
    {
        dsp::AudioBlock<float> block (buffer);
        auto up = oversampling.processSamplesUp (block);
        if (color == 1 || color == 3)
        {
            const float k = color == 1 ? 1.5f : 1.2f, bias = color == 1 ? 0.02f : 0.12f, off = std::tanh (k * bias);
            for (size_t c = 0; c < up.getNumChannels(); ++c)
            {
                auto* p = up.getChannelPointer (c);
                for (size_t i = 0; i < up.getNumSamples(); ++i) p[i] = (std::tanh (k * (p[i] + bias)) - off) / k;
            }
        }
        oversampling.processSamplesDown (block);
    }
    if (color == 3)
        for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) ch[c][i] = tapeLp[c].processSample (ch[c][i]);

    // width (mid/side), compression, gain
    float maxGr = 0.f;
    for (int i = 0; i < n; ++i)
    {
        float l = L[i], r = R[i];
        const float m = 0.5f * (l + r), s = 0.5f * (l - r) * wide;
        l = m + s; r = m - s;

        if (! multi)
        {
            const float g = single.gain (l, r);
            l *= g; r *= g;
            maxGr = jmax (maxGr, single.env);
        }
        else
        {
            float lo[2], rest[2], mi[2], hi[2];
            float in[2] = { l, r };
            for (int c = 0; c < 2; ++c)
            {
                lo[c]   = xLowLP.processSample (c, in[c]);
                rest[c] = xLowHP.processSample (c, in[c]);
                mi[c]   = xHighLP.processSample (c, rest[c]);
                hi[c]   = xHighHP.processSample (c, rest[c]);
            }
            const float g0 = bandComp[0].gain (lo[0], lo[1]), g1 = bandComp[1].gain (mi[0], mi[1]), g2 = bandComp[2].gain (hi[0], hi[1]);
            l = lo[0] * g0 + mi[0] * g1 + hi[0] * g2;
            r = lo[1] * g0 + mi[1] * g1 + hi[1] * g2;
            maxGr = jmax (maxGr, jmax (bandComp[0].env, jmax (bandComp[1].env, bandComp[2].env)));
        }
        const float w = wetGain.getNextValue();
        L[i] = l * w; R[i] = r * w;
    }

    // limiter then hard ceiling
    {
        dsp::AudioBlock<float> block (buffer);
        dsp::ProcessContextReplacing<float> ctx (block);
        limiter.process (ctx);
        for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) ch[c][i] = jlimit (-ceil, ceil, ch[c][i]);
    }

    // bypass crossfade, meters
    float pl = 0.f, pr = 0.f;
    const float* dl = dryBuffer.getReadPointer (0);
    const float* dr = dryBuffer.getReadPointer (1);
    for (int i = 0; i < n; ++i)
    {
        const float m = bypassMix.getNextValue();
        L[i] = L[i] * (1.f - m) + dl[i] * m;
        R[i] = R[i] * (1.f - m) + dr[i] * m;
        pl = jmax (pl, std::abs (L[i])); pr = jmax (pr, std::abs (R[i]));
    }
    grDb.store (bypass ? 0.f : maxGr);
    peakL.store (jmax (pl, peakL.load() * 0.9f));
    peakR.store (jmax (pr, peakR.load() * 0.9f));
}

AudioProcessorEditor* KilnProcessor::createEditor() { return new KilnEditor (*this); }

// ---- presets ----
const std::vector<KilnProcessor::Preset>& KilnProcessor::presets()
{
    static const std::vector<Preset> p = {
        { "Start", "Init", {} },
        { "Genre", "Hip-Hop", { { "loud", 7 }, { "low", 2.5f }, { "mid", -1.5f }, { "band", 2 }, { "high", 1 }, { "wide", 1.1f }, { "color", 1 }, { "multi", 1 } } },
        { "Genre", "Trap", { { "loud", 7.5f }, { "low", 3.5f }, { "mid", -2 }, { "band", 1 }, { "high", 1.5f }, { "wide", 1.2f }, { "color", 1 }, { "multi", 1 }, { "calm", 1 } } },
        { "Genre", "Pop", { { "loud", 6 }, { "low", 1 }, { "mid", 1 }, { "band", 0 }, { "high", 2 }, { "wide", 1.25f }, { "color", 2 }, { "smooth", 1 } } },
        { "Genre", "EDM", { { "loud", 8.5f }, { "low", 2 }, { "high", 2.5f }, { "wide", 1.4f }, { "color", 2 }, { "multi", 1 } } },
        { "Genre", "Rock", { { "loud", 6.5f }, { "low", 1 }, { "mid", 1.5f }, { "band", 1 }, { "high", 1 }, { "wide", 1.1f }, { "color", 3 } } },
        { "Genre", "Metal", { { "loud", 8 }, { "low", 1.5f }, { "mid", -2 }, { "band", 1 }, { "high", 2 }, { "wide", 1.2f }, { "color", 1 }, { "multi", 1 }, { "calm", 1 } } },
        { "Genre", "R&B", { { "loud", 5 }, { "low", 1.5f }, { "mid", 0.5f }, { "band", 0 }, { "high", 1 }, { "wide", 1.2f }, { "color", 1 }, { "smooth", 1 } } },
        { "Genre", "Jazz", { { "loud", 2.5f }, { "high", 0.5f }, { "wide", 1.1f }, { "color", 3 }, { "smooth", 1 }, { "mode", 1 } } },
        { "Genre", "Acoustic", { { "loud", 2 }, { "low", -0.5f }, { "mid", 0.5f }, { "band", 0 }, { "high", 1 }, { "wide", 1.05f }, { "color", 2 }, { "smooth", 1 }, { "mode", 2 } } },
        { "Genre", "Lo-fi", { { "loud", 4 }, { "low", 1 }, { "mid", 1 }, { "band", 1 }, { "high", -3 }, { "wide", 0.8f }, { "color", 3 }, { "smooth", 1 }, { "calm", 1 }, { "x2", 1 } } },
        { "Artist type", "Stadium Rocker", { { "loud", 7 }, { "low", 2 }, { "mid", 1 }, { "band", 1 }, { "high", 1.5f }, { "wide", 1.3f }, { "color", 3 }, { "multi", 1 } } },
        { "Artist type", "Bedroom Pop", { { "loud", 4.5f }, { "low", 1 }, { "high", 2 }, { "wide", 1.3f }, { "color", 2 }, { "smooth", 1 }, { "calm", 1 } } },
        { "Artist type", "Trap Producer", { { "loud", 8 }, { "low", 4 }, { "mid", -2 }, { "band", 1 }, { "high", 1 }, { "wide", 1.2f }, { "color", 1 }, { "multi", 1 } } },
        { "Artist type", "Singer-Songwriter", { { "loud", 3 }, { "mid", 1 }, { "band", 0 }, { "high", 1 }, { "wide", 1.05f }, { "color", 3 }, { "smooth", 1 }, { "mode", 1 } } },
        { "Artist type", "Club DJ", { { "loud", 9 }, { "low", 2.5f }, { "high", 2 }, { "wide", 1.35f }, { "color", 2 }, { "multi", 1 } } },
        { "Artist type", "Podcast Voice", { { "loud", 5 }, { "low", -1.5f }, { "mid", 2 }, { "band", 0 }, { "high", 0.5f }, { "wide", 0.8f }, { "color", 2 }, { "smooth", 1 }, { "calm", 1 }, { "mode", 1 } } },
    };
    return p;
}

int KilnProcessor::getNumPrograms() { return (int) presets().size(); }
const String KilnProcessor::getProgramName (int i) { return isPositiveAndBelow (i, getNumPrograms()) ? String (presets()[(size_t) i].name) : String(); }

void KilnProcessor::setCurrentProgram (int i)
{
    if (! isPositiveAndBelow (i, getNumPrograms())) return;
    currentProgram = i;
    for (auto* id : { "in", "low", "mid", "high", "band", "loud", "wide", "out", "color", "multi", "smooth", "calm", "x2", "mode" })
        if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->getDefaultValue());
    for (auto& kv : presets()[(size_t) i].values)
        if (auto* p = apvts.getParameter (kv.first)) p->setValueNotifyingHost (p->convertTo0to1 (kv.second));
}

void KilnProcessor::getStateInformation (MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void KilnProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = ValueTree::fromXml (*xml);
            currentProgram = jlimit (0, getNumPrograms() - 1, (int) state.getProperty ("program", 0));
            apvts.replaceState (state);
        }
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KilnProcessor(); }
