#pragma once

#include "../engine/MetronomeEngine.h"
#include "AppState.h"
#include "Feature.h"
#include "Theme.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_data_structures/juce_data_structures.h>

namespace metro
{
/** Owns the audio device, engine and state, and lays the features out in a scrolling column. */
class MainComponent final : public juce::Component,
                            private AppState::Listener,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    void appStateChanged() override;
    void timerCallback() override;
    void layoutFeatures();
    void saveState();

    Theme theme;
    juce::ApplicationProperties appProperties;
    juce::AudioDeviceManager devices;
    MetronomeEngine engine;
    AppState state;
    AppContext ctx;

    juce::Viewport viewport;
    juce::Component content;
    std::vector<std::unique_ptr<Feature>> features;
    juce::TooltipWindow tooltips { this };
    bool wasPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
} // namespace metro
