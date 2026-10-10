#include "MainComponent.h"

namespace metro
{
namespace
{
    juce::PropertiesFile::Options propertiesOptions()
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "Metronome";
        o.folderName = "Metronome";
        o.filenameSuffix = ".settings";
        o.osxLibrarySubFolder = "Application Support";
        o.millisecondsBeforeSaving = 500;
        return o;
    }

    constexpr int maxContentWidth = 520;
    constexpr int gap = 16;
    constexpr int sideMargin = 16;
} // namespace

MainComponent::MainComponent()
    : ctx { state, engine, *[this]() -> juce::PropertiesFile* {
                appProperties.setStorageParameters (propertiesOptions());
                return appProperties.getUserSettings();
            }(),
            devices }
{
    setLookAndFeel (&theme);

    // Restore the last session's settings.
    const auto saved = juce::JSON::parse (ctx.properties.getValue ("state", "{}"));
    state.updateSettings ([&] (Settings& s) { s = settingsFromVar (saved["settings"]); });
    state.updateTrainer ([&] (TrainerSettings& t) { t = trainerFromVar (saved["trainer"]); });

    // Audio: default output device, stereo, no inputs.
    std::unique_ptr<juce::XmlElement> savedDevice = ctx.properties.getXmlValue ("audioDevice");
    devices.initialise (0, 2, savedDevice.get(), true);
    engine.apply (state.settings(), state.trainer());
    devices.addAudioCallback (&engine);

    features = createFeatures (ctx);
    for (auto& f : features)
    {
        f->requestLayout = [this] { layoutFeatures(); };
        content.addAndMakeVisible (*f);
    }

    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);

    state.addListener (this);
    setWantsKeyboardFocus (true);
    setSize (460, 900);
    startTimerHz (60);
}

MainComponent::~MainComponent()
{
    stopTimer();
    state.removeListener (this);
    devices.removeAudioCallback (&engine);
    if (auto xml = devices.createStateXml())
        ctx.properties.setValue ("audioDevice", xml.get());
    saveState();
    ctx.properties.saveIfNeeded();
    setLookAndFeel (nullptr);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);
}

void MainComponent::resized()
{
    viewport.setBounds (getLocalBounds());
    layoutFeatures();
}

void MainComponent::layoutFeatures()
{
    const auto width = std::min (maxContentWidth, viewport.getMaximumVisibleWidth()) - 2 * sideMargin;
    const auto x = (viewport.getMaximumVisibleWidth() - width) / 2;
    auto y = 20;
    for (auto& f : features)
    {
        f->setBounds (x, y, width, f->preferredHeight (width));
        y += f->getHeight() + gap;
    }
    content.setSize (viewport.getMaximumVisibleWidth(), y + 8);
}

bool MainComponent::keyPressed (const juce::KeyPress& key)
{
    for (auto& f : features)
        if (f->shortcut (key))
            return true;
    return false;
}

void MainComponent::appStateChanged()
{
    engine.apply (state.settings(), state.trainer());

    if (state.isPlaying() != wasPlaying)
    {
        wasPlaying = state.isPlaying();
        engine.setPlaying (wasPlaying);
        // Keep the screen on while playing (also stops the display sleeping on Android).
        juce::Desktop::setScreenSaverEnabled (! wasPlaying);
    }

    for (auto& f : features)
        f->stateChanged();
    saveState();
}

void MainComponent::saveState()
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("settings", toVar (state.settings()));
    o->setProperty ("trainer", toVar (state.trainer()));
    ctx.properties.setValue ("state", juce::JSON::toString (juce::var (o), true));
}

void MainComponent::timerCallback()
{
    if (const auto bpm = engine.takeTrainerBpm())
        state.updateSettings ([v = *bpm] (Settings& s) { s.bpm = v; });

    while (const auto tick = engine.popHeardTick())
        for (auto& f : features)
            f->tickHeard (*tick);

    const auto now = juce::Time::getMillisecondCounterHiRes();
    for (auto& f : features)
        f->frame (now);
}
} // namespace metro
