#pragma once

#include "../engine/MetronomeEngine.h"
#include "AppState.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <vector>

namespace metro
{
/** Everything a feature may use. Features never talk to each other directly. */
struct AppContext
{
    AppState& state;
    MetronomeEngine& engine;
    juce::PropertiesFile& properties;
    juce::AudioDeviceManager& devices;
};

/**
 * A self-contained piece of the app: a component that renders itself from the
 * shared state, writes changes back to it, and may react to heard ticks.
 */
class Feature : public juce::Component
{
public:
    explicit Feature (AppContext& c) : ctx (c) {}

    /** Height this feature wants at the given width. */
    virtual int preferredHeight (int width) const = 0;

    /** The shared state changed. */
    virtual void stateChanged() {}

    /** A tick has just reached the speakers. */
    virtual void tickHeard (const Tick&) {}

    /** Called about 60 times a second, for animations. */
    virtual void frame (double /*nowMs*/) {}

    /** A key was pressed anywhere in the window; return true if this feature handled it. */
    virtual bool shortcut (const juce::KeyPress&) { return false; }

    /** Set by the main component; call when preferredHeight() changes. */
    std::function<void()> requestLayout = [] {};

protected:
    AppContext& ctx;
};

/** The app's features, in display order. Add, remove or reorder features in Features.cpp. */
std::vector<std::unique_ptr<Feature>> createFeatures (AppContext& ctx);
} // namespace metro
