#include "MainComponent.h"

#include "../core/Settings.h"

#include <BinaryData.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace metro
{
namespace
{
    constexpr auto eventId = "metronome";
    const juce::Colour background { 0xff14171f }; // matches the web app's dark theme

    juce::PropertiesFile::Options propertiesOptions()
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "Metronome";
        o.folderName = "Metronome";
        o.filenameSuffix = ".settings";
        o.osxLibrarySubFolder = "Application Support";
        return o;
    }

    juce::File appDataFolder()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Metronome");
    }

    const char* mimeType (const juce::String& path)
    {
        const auto ext = path.fromLastOccurrenceOf (".", false, false).toLowerCase();
        if (ext == "html") return "text/html";
        if (ext == "js") return "text/javascript";
        if (ext == "css") return "text/css";
        if (ext == "svg") return "image/svg+xml";
        if (ext == "png") return "image/png";
        if (ext == "json" || ext == "webmanifest") return "application/json";
        return "application/octet-stream";
    }

    const char* kindName (TickKind kind)
    {
        switch (kind)
        {
            case TickKind::accent: return "accent";
            case TickKind::beat:   return "beat";
            case TickKind::sub:    return "sub";
            case TickKind::silent: return "silent";
        }
        return "beat";
    }

    // The tuner reads the microphone from the page. WebView2 would ask "juce.backend wants to
    // use your microphone" like a website does; the page is our own UI, so grant it as a native
    // app would (Windows privacy settings still apply). JUCE has no option for browser
    // arguments, so they go through the variable WebView2 reads when it starts.
    void allowMediaWithoutPrompt()
    {
       #if JUCE_WINDOWS
        constexpr auto name = "WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS";
        const auto args = (juce::SystemStats::getEnvironmentVariable (name, {}) + " --use-fake-ui-for-media-stream").trim();
        _putenv_s (name, args.toRawUTF8());
       #endif
    }

    juce::WebBrowserComponent::Options browserOptions (std::function<void (const juce::var&)> onMessage,
                                                       juce::WebBrowserComponent::ResourceProvider provider)
    {
        allowMediaWithoutPrompt();
        using Options = juce::WebBrowserComponent::Options;
        return Options {}
            .withBackend (Options::Backend::webview2)
            .withWinWebView2Options (Options::WinWebView2 {}
                                         .withUserDataFolder (appDataFolder().getChildFile ("WebView2"))
                                         .withBackgroundColour (background)
                                         .withStatusBarDisabled())
            .withNativeIntegrationEnabled()
            .withKeepPageLoadedWhenBrowserIsHidden()
            .withEventListener (eventId, std::move (onMessage))
            .withResourceProvider (std::move (provider));
    }
} // namespace

MainComponent::MainComponent()
    : ui (std::make_unique<juce::MemoryInputStream> (BinaryData::ui_zip, BinaryData::ui_zipSize, false)),
      browser (browserOptions ([this] (const juce::var& m) { handleMessage (m); },
                               [this] (const juce::String& path) { return serve (path); }))
{
    appProperties.setStorageParameters (propertiesOptions());

    // Audio: the saved output device, or the default one; stereo, no inputs.
    auto savedDevice = appProperties.getUserSettings()->getXmlValue ("audioDevice");
    devices.initialise (0, 2, savedDevice.get(), true);
    devices.addAudioCallback (&engine);

    addAndMakeVisible (browser);
    browser.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setSize (460, 900);
    startTimerHz (60);
}

MainComponent::~MainComponent()
{
    stopTimer();
    devices.removeAudioCallback (&engine);
    if (auto xml = devices.createStateXml())
        appProperties.getUserSettings()->setValue ("audioDevice", xml.get());
    appProperties.saveIfNeeded();
}

void MainComponent::resized()
{
    browser.setBounds (getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource> MainComponent::serve (const juce::String& requested)
{
    auto path = requested.upToFirstOccurrenceOf ("?", false, false).trimCharactersAtStart ("/");
    if (path.isEmpty())
        path = "index.html";

    // The zip may store entries with or without a leading "./".
    const auto* entry = ui.getEntry (path);
    if (entry == nullptr)
        entry = ui.getEntry ("./" + path);
    if (entry == nullptr)
        return std::nullopt;

    std::unique_ptr<juce::InputStream> stream (ui.createStreamForEntry (*entry));
    if (stream == nullptr)
        return std::nullopt;

    juce::MemoryBlock block;
    stream->readIntoMemoryBlock (block);
    const auto* bytes = static_cast<const std::byte*> (block.getData());
    return juce::WebBrowserComponent::Resource { { bytes, bytes + block.getSize() }, mimeType (path) };
}

void MainComponent::handleMessage (const juce::var& message)
{
    const auto type = message["type"].toString();

    if (type == "update")
    {
        engine.apply (settingsFromVar (message["settings"]), trainerFromVar (message["trainer"]));
    }
    else if (type == "start" || type == "stop")
    {
        const auto playing = type == "start";
        lastBars = 0;
        engine.setPlaying (playing);
        // Keep the screen on while playing (also stops the display sleeping on Android).
        juce::Desktop::setScreenSaverEnabled (! playing);
    }
    else if (type == "audioSettings")
    {
        showAudioSettings();
    }
}

void MainComponent::send (juce::DynamicObject::Ptr message)
{
    browser.emitEventIfBrowserIsVisible (eventId, juce::var (message.get()));
}

void MainComponent::timerCallback()
{
    // Same order as the web engine: a trainer tempo step, then the bar, then heard ticks.
    if (const auto bpm = engine.takeTrainerBpm())
    {
        juce::DynamicObject::Ptr m = new juce::DynamicObject();
        m->setProperty ("type", "tempo");
        m->setProperty ("bpm", *bpm);
        send (m);
    }

    if (const auto bars = engine.barsCompleted(); bars != lastBars)
    {
        lastBars = bars;
        juce::DynamicObject::Ptr m = new juce::DynamicObject();
        m->setProperty ("type", "bar");
        m->setProperty ("bars", (juce::int64) bars);
        send (m);
    }

    while (const auto tick = engine.popHeardTick())
    {
        juce::DynamicObject::Ptr m = new juce::DynamicObject();
        m->setProperty ("type", "beat");
        m->setProperty ("bar", (juce::int64) tick->pos.bar);
        m->setProperty ("beat", tick->pos.beat);
        m->setProperty ("sub", tick->pos.sub);
        m->setProperty ("kind", kindName (tick->kind));
        send (m);
    }
}

void MainComponent::showAudioSettings()
{
    auto selector = std::make_unique<juce::AudioDeviceSelectorComponent> (devices, 0, 0, 1, 2, false, false, true, false);
    selector->setSize (480, 360);
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (selector.release());
    options.dialogTitle = "Audio settings";
    options.dialogBackgroundColour = juce::Colour (0xff1d212b);
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}
} // namespace metro
