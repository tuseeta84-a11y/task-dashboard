#include "PluginEditor.h"

using namespace juce;

namespace
{
    const Colour kPanel (0xff222c30), kPanelHi (0xff2b373c), kInk (0xffebe5d3), kMute (0xff8c9b9a),
                 kAmber (0xffffb13b), kMint (0xff8fe3b2), kRed (0xffff6252), kLcd (0xff0b110f);
    constexpr int W = 940, H = 600;
    constexpr int bigSize = 230;
}

KilnLookAndFeel::KilnLookAndFeel()
{
    setColour (ComboBox::textColourId, kMint);
    setColour (PopupMenu::backgroundColourId, Colour (0xff0f1517));
    setColour (PopupMenu::textColourId, kInk);
    setColour (PopupMenu::highlightedBackgroundColourId, kAmber);
    setColour (PopupMenu::highlightedTextColourId, Colour (0xff20180a));
    setColour (PopupMenu::headerTextColourId, kMute);
    setColour (TooltipWindow::backgroundColourId, kLcd);
    setColour (TooltipWindow::textColourId, kMint);
}

void KilnLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float start, float end, Slider& s)
{
    const auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
    const float d = jmin (b.getWidth(), b.getHeight()), r = d * 0.5f;
    const auto c = b.getCentre();
    const float a = start + pos * (end - start);
    const bool big = s.getName() == "loud";

    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillEllipse (c.x - r + 1.f, c.y - r + (big ? 8.f : 5.f), d, d);

    if (big)
    {
        g.setGradientFill (ColourGradient (Colour (0xff46555b), c.x, c.y - r * 0.3f, Colour (0xff121819), c.x, c.y + r, true));
        g.fillEllipse (c.x - r, c.y - r, d, d);
        g.setColour (Colour (0x14ffffff));
        for (int i = 0; i < 72; ++i)
        {
            const float t = MathConstants<float>::twoPi * (float) i / 72.f;
            g.drawLine (c.x + std::sin (t) * r * 0.8f, c.y - std::cos (t) * r * 0.8f, c.x + std::sin (t) * r, c.y - std::cos (t) * r, 1.5f);
        }
        g.setGradientFill (ColourGradient (Colour (0xff3b484e), c.x - r * 0.2f, c.y - r * 0.3f, Colour (0xff1a2124), c.x + r * 0.3f, c.y + r * 0.4f, true));
        g.fillEllipse (c.x - r * 0.56f, c.y - r * 0.56f, r * 1.12f, r * 1.12f);
    }
    else
    {
        g.setGradientFill (ColourGradient (Colour (0xfffaf5e6), c.x - r * 0.3f, c.y - r * 0.4f, Colour (0xffb7ad93), c.x + r * 0.3f, c.y + r, true));
        g.fillEllipse (c.x - r, c.y - r, d, d);
    }

    Path p;
    const float len = big ? r * 0.36f : r * 0.42f, thick = big ? 5.f : 3.f, inner = big ? r * 0.64f : r * 0.52f;
    p.addRoundedRectangle (-thick * 0.5f, -inner - len * 0.5f, thick, len, thick * 0.5f);
    g.setColour (big ? kAmber : Colour (0xff1c2528));
    g.fillPath (p, AffineTransform::rotation (a).translated (c.x, c.y));
}

void KilnLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool hover, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.f);
    const bool on = b.getToggleState();
    const bool bypass = b.getName() == "bypass";
    if (on)
    {
        g.setGradientFill (ColourGradient (bypass ? kRed : Colour (0xffe9e3d1), 0, r.getY(), bypass ? kRed.darker (0.3f) : Colour (0xffbfb79f), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 6.f);
        g.setColour (Colours::black); g.drawRoundedRectangle (r.expanded (1.f), 7.f, 1.5f);
    }
    else
    {
        g.setGradientFill (ColourGradient (hover ? Colour (0xff36464d) : Colour (0xff2c3a40), 0, r.getY(), Colour (0xff161e21), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 6.f);
        g.setColour (Colour (0xff0a0f11)); g.drawRoundedRectangle (r, 6.f, 1.f);
    }
    // LED bar under the key
    if (b.getProperties().contains ("led"))
    {
        auto led = Rectangle<float> (r.getX() + r.getWidth() * 0.15f, r.getBottom() + 6.f, r.getWidth() * 0.7f, 5.f);
        g.setColour (on ? kAmber : Colour (0xff121a1d)); g.fillRoundedRectangle (led, 2.5f);
    }
}

void KilnLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    const bool on = b.getToggleState();
    g.setColour (on ? (b.getName() == "bypass" ? Colours::white : Colour (0xff1c2528)) : kInk);
    g.setFont (FontOptions (b.getHeight() < 28 ? 13.f : 17.f, Font::bold));
    g.drawFittedText (b.getButtonText().toUpperCase(), b.getLocalBounds(), Justification::centred, 1);
}

void KilnLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox&)
{
    g.setColour (kLcd); g.fillRoundedRectangle (0, 0, (float) w, (float) h, 4.f);
    g.setColour (Colour (0xff2c3a40)); g.drawRoundedRectangle (0.5f, 0.5f, (float) w - 1.f, (float) h - 1.f, 4.f, 1.f);
}

KilnEditor::KilnEditor (KilnProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (W, H);

    auto addKnob = [this] (Slider& s, const char* id, const char* name, const char* suffix)
    {
        s.setName (name);
        s.setSliderStyle (Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
        s.setPopupDisplayEnabled (true, true, this);
        s.setTextValueSuffix (suffix);
        s.setVelocityBasedMode (false);
        s.setMouseDragSensitivity (220);
        addAndMakeVisible (s);
        sliderAtt.push_back (std::make_unique<SA> (proc.apvts, id, s));
    };
    addKnob (sIn, "in", "in", " dB");   addKnob (sOut, "out", "out", " dB");
    addKnob (sLow, "low", "low", " dB"); addKnob (sMid, "mid", "mid", " dB"); addKnob (sHigh, "high", "high", " dB");
    addKnob (sLoud, "loud", "loud", ""); addKnob (sWide, "wide", "wide", "");
    sWide.textFromValueFunction = [] (double v) { return String (roundToInt (v * 100)) + "%"; };

    auto addToggle = [this] (TextButton& b, const char* id, bool led)
    {
        b.setClickingTogglesState (true);
        if (led) b.getProperties().set ("led", true);
        addAndMakeVisible (b);
        buttonAtt.push_back (std::make_unique<BA> (proc.apvts, id, b));
    };
    addToggle (bMulti, "multi", true); addToggle (bSmooth, "smooth", true); addToggle (bCalm, "calm", true);
    addToggle (bX2, "x2", false); addToggle (bBypass, "bypass", false);
    bBypass.setName ("bypass");

    auto addChoice = [this] (TextButton& b, const char* id, int idx, bool led)
    {
        if (led) b.getProperties().set ("led", true);
        b.onClick = [this, id, idx, &b] { setChoice (id, getChoice (id) == idx && std::string (id) == "color" ? 0 : idx); };
        addAndMakeVisible (b);
    };
    addChoice (bThick, "color", 1, true); addChoice (bClean, "color", 2, true); addChoice (bTape, "color", 3, true);
    addChoice (bH, "band", 0, false); addChoice (bM, "band", 1, false); addChoice (bL, "band", 2, false);
    addChoice (mBoost, "mode", 0, false); addChoice (mTrue, "mode", 1, false); addChoice (mUnity, "mode", 2, false);

    int id = 1; String group;
    for (auto& pr : KilnProcessor::presets())
    {
        if (group != pr.group) { group = pr.group; presetBox.addSectionHeading (group); }
        presetBox.addItem (pr.name, id++);
    }
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);
    prevPre.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + proc.getNumPrograms() - 1) % proc.getNumPrograms()); };
    nextPre.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % proc.getNumPrograms()); };
    addAndMakeVisible (prevPre); addAndMakeVisible (nextPre);

    startTimerHz (30);
    timerCallback();
}

KilnEditor::~KilnEditor() { setLookAndFeel (nullptr); }

void KilnEditor::setChoice (const char* id, int index)
{
    if (auto* p = proc.apvts.getParameter (id)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 ((float) index)); p->endChangeGesture(); }
}
int KilnEditor::getChoice (const char* id) const { return (int) proc.apvts.getRawParameterValue (id)->load(); }

void KilnEditor::timerCallback()
{
    const int color = getChoice ("color"), band = getChoice ("band"), mode = getChoice ("mode");
    bThick.setToggleState (color == 1, dontSendNotification); bClean.setToggleState (color == 2, dontSendNotification); bTape.setToggleState (color == 3, dontSendNotification);
    bH.setToggleState (band == 0, dontSendNotification); bM.setToggleState (band == 1, dontSendNotification); bL.setToggleState (band == 2, dontSendNotification);
    mBoost.setToggleState (mode == 0, dontSendNotification); mTrue.setToggleState (mode == 1, dontSendNotification); mUnity.setToggleState (mode == 2, dontSendNotification);
    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1) presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    grSmooth += (proc.grDb.load() - grSmooth) * 0.3f;
    repaint();
}

void KilnEditor::resized()
{
    sIn.setBounds (30, 20, 70, 70);   sOut.setBounds (840, 20, 70, 70);
    mBoost.setBounds (720, 22, 100, 22); mTrue.setBounds (720, 46, 100, 22); mUnity.setBounds (720, 70, 100, 22);
    sMid.setBounds (95, 135, 100, 100); bH.setBounds (205, 150, 30, 22); bM.setBounds (205, 175, 30, 22); bL.setBounds (205, 200, 30, 22);
    sLow.setBounds (55, 275, 90, 90);   sHigh.setBounds (185, 275, 90, 90);
    sLoud.setBounds (470 - bigSize / 2, 255 - bigSize / 2, bigSize, bigSize);
    sWide.setBounds (700, 190, 120, 120);

    const int kw = 110, gap = 18, y = 440, h = 44;
    int x = (W - (7 * kw + 6 * gap)) / 2;
    bThick.setBounds (x, y, kw, h); x += kw + gap;
    bClean.setBounds (x, y, kw, h); x += kw + gap;
    bMulti.setBounds (x, y, kw, h); x += kw + gap;
    bBypass.setBounds (x + 15, y - 8, kw - 30, h + 16); x += kw + gap;
    bSmooth.setBounds (x, y, kw, h); x += kw + gap;
    bCalm.setBounds (x, y, kw - 38, h); bX2.setBounds (x + kw - 38, y, 38, h); x += kw + gap;
    bTape.setBounds (x, y, kw, h);

    prevPre.setBounds (370, 520, 32, 32); presetBox.setBounds (406, 520, 220, 32); nextPre.setBounds (630, 520, 32, 32);
}

void KilnEditor::paint (Graphics& g)
{
    g.setGradientFill (ColourGradient (Colour (0xff2c393e), W / 2.f, 0.f, Colour (0xff1c2528), W / 2.f, (float) H, false));
    g.fillAll();

    g.setFont (FontOptions (44.f, Font::bold));
    g.setColour (kInk);
    g.drawText ("Kiln", W / 2 - 120, 18, 120, 50, Justification::centredRight);
    g.setColour (kAmber);
    g.drawText ("One", W / 2, 18, 140, 50, Justification::centredLeft);
    g.setColour (kMute); g.setFont (FontOptions (12.f, Font::plain));
    g.drawText ("O N E - K N O B   M A S T E R I N G", W / 2 - 160, 66, 320, 16, Justification::centred);

    // labels
    g.setFont (FontOptions (14.f, Font::bold)); g.setColour (kInk);
    auto label = [&g] (Component& c, const char* t) { g.drawText (String (t).toUpperCase(), c.getX() - 10, c.getBottom() - 4, c.getWidth() + 20, 18, Justification::centred); };
    label (sIn, "In"); label (sOut, "Out"); label (sMid, "Mid"); label (sLow, "Low"); label (sHigh, "High"); label (sWide, "Wide");
    g.setFont (FontOptions (20.f, Font::bold));
    g.drawText ("L O U D", sLoud.getX(), sLoud.getBottom() + 4, sLoud.getWidth(), 24, Justification::centred);

    // ring around LOUD: value ticks and gain-reduction ticks
    const auto c = sLoud.getBounds().toFloat().getCentre();
    const float kr = bigSize * 0.5f - 4.f;
    const int N = 41, lit = roundToInt ((float) sLoud.valueToProportionOfLength (sLoud.getValue()) * (N - 1));
    for (int i = 0; i < N; ++i)
    {
        const float a = degreesToRadians (-135.f + (float) i * 270.f / (N - 1));
        const float r1 = kr + 12.f, r2 = kr + (i % 5 == 0 ? 24.f : 20.f);
        g.setColour (i <= lit && sLoud.getValue() > 0 ? kAmber : Colour (0xff2c3a40));
        g.drawLine (c.x + std::sin (a) * r1, c.y - std::cos (a) * r1, c.x + std::sin (a) * r2, c.y - std::cos (a) * r2, i % 5 == 0 ? 3.f : 2.f);
    }
    const int M = 24, glit = roundToInt (jlimit (0.f, 1.f, grSmooth / 18.f) * M);
    for (int j = 0; j < M; ++j)
    {
        const float a = degreesToRadians (135.f - (float) j * 270.f / (M - 1));
        g.setColour (j < glit ? kRed : Colour (0xff1f2a2f));
        g.drawLine (c.x + std::sin (a) * (kr + 29.f), c.y - std::cos (a) * (kr + 29.f), c.x + std::sin (a) * (kr + 34.f), c.y - std::cos (a) * (kr + 34.f), 2.4f);
    }

    // LCD strip with meters
    auto lcd = Rectangle<float> (30.f, 508.f, 880.f, 58.f);
    g.setColour (kLcd); g.fillRoundedRectangle (lcd, 6.f);
    const float pk[2] = { proc.peakL.load(), proc.peakR.load() };
    for (int r = 0; r < 2; ++r)
    {
        const float dbv = Decibels::gainToDecibels (jmax (pk[r], 1.0e-5f));
        const int n = 30, litSeg = roundToInt (jlimit (0.f, 1.f, (dbv + 42.f) / 42.f) * n);
        for (int i = 0; i < n; ++i)
        {
            const float d = -42.f + (float) i / (n - 1) * 42.f;
            g.setColour (i < litSeg ? (d > -1.f ? kRed : d > -9.f ? kAmber : kMint) : Colour (0xff17221e));
            g.fillRect (44.f + (float) i * 10.f, 520.f + (float) r * 16.f, 8.f, 12.f);
        }
    }
    g.setFont (FontOptions (13.f, Font::plain)); g.setColour (kMint);
    g.drawText ("GR " + String (grSmooth, 1) + " dB", 720, 520, 170, 16, Justification::centredRight);
    g.drawText ("PK " + String (Decibels::gainToDecibels (jmax (pk[0], pk[1], 1.0e-5f)), 1) + " dB", 720, 536, 170, 16, Justification::centredRight);
}
