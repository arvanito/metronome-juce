#pragma once

#include "../core/Types.h"
#include "Scheduler.h"
#include "Sounds.h"

#include <juce_audio_devices/juce_audio_devices.h>

#include <atomic>
#include <optional>

namespace metro
{
/**
 * Plays the metronome. The scheduler and the voice mixer run inside the audio callback,
 * so every click starts on an exact sample. The message thread talks to it only through
 * atomics and a lock-free FIFO: no locks or allocations happen on the audio thread.
 */
class MetronomeEngine : public juce::AudioIODeviceCallback
{
public:
    MetronomeEngine();

    // --- Message thread -------------------------------------------------------
    void apply (const Settings& settings, const TrainerSettings& trainer);
    void setPlaying (bool shouldPlay);

    /** If the speed trainer changed the tempo since the last call, returns the new BPM. */
    std::optional<int> takeTrainerBpm();

    /** Bars completed since playback started. */
    int64_t barsCompleted() const { return bars.load(); }

    /** Pops the next tick that has reached the speakers (adjusted for output latency). */
    std::optional<Tick> popHeardTick();

    // --- AudioIODeviceCallback -------------------------------------------------
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData, int numInputChannels,
                                           float* const* outputChannelData, int numOutputChannels,
                                           int numSamples, const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    struct ActiveVoice
    {
        const Samples* samples = nullptr;
        size_t position = 0;
        int delay = 0; // samples to wait within the current block before starting
    };

    Pattern currentPattern() const;
    void startVoice (TickKind kind, int offsetInBlock);
    void onBarStart (int64_t barsDone);

    // Shared state (written by the message thread unless noted).
    std::atomic<double> bpm { 100 };
    std::atomic<int> beatsPerBar { 4 };
    std::atomic<int> subdivision { 1 };
    std::atomic<uint32_t> accentBits { 0 }; // 2 bits per beat
    std::atomic<int> soundIndex { 0 };
    std::atomic<float> gain { 1.0f };
    std::atomic<bool> trainerEnabled { false };
    std::atomic<int> trainerStepBpm { 5 }, trainerEveryBars { 4 }, trainerTargetBpm { 140 };
    std::atomic<bool> startRequested { false }, stopRequested { false };

    // Written by the audio thread.
    std::atomic<int> trainerBpm { -1 };
    std::atomic<int64_t> bars { 0 };
    std::atomic<int64_t> samplesPlayed { 0 };
    std::atomic<int64_t> latencySamples { 0 };

    // Audio thread only.
    Scheduler scheduler;
    double sampleRate = 48000;
    std::vector<std::array<Samples, numVoices>> rendered; // [sound][voice], rebuilt when the device starts
    std::array<ActiveVoice, 16> voices {};

    // Ticks waiting to be shown, from the audio thread to the message thread.
    juce::AbstractFifo tickFifo { 256 };
    std::array<Tick, 256> tickBuffer {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MetronomeEngine)
};
} // namespace metro
