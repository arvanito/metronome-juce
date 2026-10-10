#pragma once

#include "../engine/MetronomeEngine.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace metro
{
/**
 * Hosts the web UI (the metronome web app, bundled into the binary) in a WebView and
 * connects it to the native audio engine. The page talks to us through JUCE's
 * "metronome" event in both directions; see web/src/engine/nativeEngine.ts.
 */
class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void resized() override;

private:
    void handleMessage (const juce::var& message);
    void send (juce::DynamicObject::Ptr message);
    void timerCallback() override;
    void showAudioSettings();
    std::optional<juce::WebBrowserComponent::Resource> serve (const juce::String& path);

    juce::ApplicationProperties appProperties;
    juce::AudioDeviceManager devices;
    MetronomeEngine engine;
    juce::ZipFile ui;
    juce::WebBrowserComponent browser;
    int64_t lastBars = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
} // namespace metro
