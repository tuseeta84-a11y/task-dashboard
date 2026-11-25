#pragma once
#include "PluginProcessor.h"

class KilnLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KilnLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool hover, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool hover, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::FontOptions (15.f, juce::Font::bold); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::FontOptions (15.f, juce::Font::bold); }
};

class KilnEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit KilnEditor (KilnProcessor&);
    ~KilnEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setChoice (const char* id, int index);
    int getChoice (const char* id) const;

    KilnProcessor& proc;
    KilnLookAndFeel laf;

    juce::Slider sIn, sOut, sLow, sMid, sHigh, sLoud, sWide;
    juce::TextButton bThick { "Thick" }, bClean { "Clean" }, bTape { "Tape" }, bMulti { "Multi" }, bSmooth { "Smooth" },
                     bCalm { "Calm" }, bX2 { "x2" }, bBypass { "Bypass" },
                     bH { "H" }, bM { "M" }, bL { "L" }, mBoost { "Boost" }, mTrue { "True Peak" }, mUnity { "Unity" },
                     prevPre { "<" }, nextPre { ">" };
    juce::ComboBox presetBox;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SA>> sliderAtt;
    std::vector<std::unique_ptr<BA>> buttonAtt;

    float grSmooth = 0.f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KilnEditor)
};
