#pragma once

#include "../core/Settings.h"

namespace metro
{
/**
 * The app's state on the message thread: settings, trainer settings and whether it is
 * playing. Every update is sanitised; listeners are told synchronously after a change.
 */
class AppState
{
public:
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void appStateChanged() = 0;
    };

    const Settings& settings() const { return current; }
    const TrainerSettings& trainer() const { return trainerSettings; }
    bool isPlaying() const { return playing; }

    template <typename Fn>
    void updateSettings (Fn&& fn)
    {
        auto next = current;
        fn (next);
        next = sanitize (next);
        if (next == current)
            return;
        current = next;
        notify();
    }

    template <typename Fn>
    void updateTrainer (Fn&& fn)
    {
        auto next = trainerSettings;
        fn (next);
        next = sanitize (next);
        if (next == trainerSettings)
            return;
        trainerSettings = next;
        notify();
    }

    void setPlaying (bool shouldPlay)
    {
        if (shouldPlay == playing)
            return;
        playing = shouldPlay;
        notify();
    }

    void addListener (Listener* l) { listeners.add (l); }
    void removeListener (Listener* l) { listeners.remove (l); }

private:
    void notify() { listeners.call ([] (Listener& l) { l.appStateChanged(); }); }

    Settings current;
    TrainerSettings trainerSettings;
    bool playing = false;
    juce::ListenerList<Listener> listeners;
};
} // namespace metro
