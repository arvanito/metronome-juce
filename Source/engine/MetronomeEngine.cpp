#include "MetronomeEngine.h"

#include "../core/SpeedTrainer.h"
#include "../core/Volume.h"

namespace metro
{
namespace
{
    uint32_t packAccents (const AccentPattern& accents)
    {
        uint32_t bits = 0;
        for (size_t i = 0; i < accents.size(); ++i)
            bits |= (uint32_t) accents[i] << (2 * i);
        return bits;
    }

    AccentPattern unpackAccents (uint32_t bits)
    {
        AccentPattern accents {};
        for (size_t i = 0; i < accents.size(); ++i)
            accents[i] = (Accent) ((bits >> (2 * i)) & 3u);
        return accents;
    }
} // namespace

MetronomeEngine::MetronomeEngine()
{
    accentBits = packAccents (defaultAccents());
}

void MetronomeEngine::apply (const Settings& s, const TrainerSettings& t)
{
    bpm = (double) s.bpm;
    beatsPerBar = s.beatsPerBar;
    subdivision = s.subdivision;
    accentBits = packAccents (s.accents);
    soundIndex = findSound (s.sound);
    gain = (float) volumeToGain (s.volume);
    trainerEnabled = t.enabled;
    trainerStepBpm = t.stepBpm;
    trainerEveryBars = t.everyBars;
    trainerTargetBpm = t.targetBpm;
}

void MetronomeEngine::setPlaying (bool shouldPlay)
{
    if (shouldPlay)
    {
        bars = 0;
        startRequested = true;
    }
    else
    {
        stopRequested = true;
    }
}

std::optional<int> MetronomeEngine::takeTrainerBpm()
{
    const auto v = trainerBpm.exchange (-1);
    return v < 0 ? std::nullopt : std::optional<int> (v);
}

std::optional<Tick> MetronomeEngine::popHeardTick()
{
    const auto heardUpTo = samplesPlayed.load() - latencySamples.load();
    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    tickFifo.prepareToRead (1, start1, size1, start2, size2);
    if (size1 == 0)
        return std::nullopt;

    // Peek first: only consume the tick once it has actually been heard.
    const auto tick = tickBuffer[(size_t) start1];
    if (tick.sample > heardUpTo)
        return std::nullopt;

    tickFifo.finishedRead (1);
    return tick;
}

Pattern MetronomeEngine::currentPattern() const
{
    return { bpm.load(), beatsPerBar.load(), subdivision.load(), unpackAccents (accentBits.load()) };
}

void MetronomeEngine::onBarStart (int64_t barsDone)
{
    bars = barsDone;
    if (! trainerEnabled.load() || ! shouldStep (barsDone, trainerEveryBars.load()))
        return;

    const TrainerSettings t { true, trainerStepBpm.load(), trainerEveryBars.load(), trainerTargetBpm.load() };
    const auto next = trainerStep ((int) std::lround (bpm.load()), t);
    bpm = (double) next;
    trainerBpm = next; // picked up by the message thread to update the UI and settings
}

void MetronomeEngine::startVoice (TickKind kind, int offsetInBlock)
{
    if (kind == TickKind::silent || rendered.empty())
        return;

    const auto sound = (size_t) std::clamp (soundIndex.load(), 0, (int) rendered.size() - 1);
    const auto* samples = &rendered[sound][(size_t) kind]; // TickKind accent/beat/sub == Voice 0/1/2

    // Use a free slot, or steal the one that has played longest.
    auto* slot = &voices[0];
    for (auto& v : voices)
    {
        if (v.samples == nullptr) { slot = &v; break; }
        if (v.position > slot->position) slot = &v;
    }
    *slot = { samples, 0, offsetInBlock };
}

void MetronomeEngine::audioDeviceIOCallbackWithContext (const float* const*, int,
                                                        float* const* outputChannelData, int numOutputChannels,
                                                        int numSamples, const juce::AudioIODeviceCallbackContext&)
{
    const auto blockStart = samplesPlayed.load();

    if (startRequested.exchange (false))
    {
        for (auto& v : voices) v = {};
        scheduler.start (blockStart);
    }
    if (stopRequested.exchange (false))
        scheduler.stop(); // ringing clicks are allowed to finish

    scheduler.process (blockStart, numSamples, sampleRate,
        [this] { return currentPattern(); },
        [&] (const Tick& tick) {
            startVoice (tick.kind, (int) (tick.sample - blockStart));
            const auto scope = tickFifo.write (1);
            if (scope.blockSize1 > 0)
                tickBuffer[(size_t) scope.startIndex1] = tick;
        },
        [this] (int64_t barsDone) { onBarStart (barsDone); });

    // Mix into the first channel, then copy to the others.
    if (numOutputChannels > 0 && outputChannelData[0] != nullptr)
    {
        auto* out = outputChannelData[0];
        std::fill (out, out + numSamples, 0.0f);

        for (auto& v : voices)
        {
            if (v.samples == nullptr)
                continue;
            const auto& s = *v.samples;
            for (int i = std::min (v.delay, numSamples); i < numSamples && v.position < s.size(); ++i)
                out[i] += s[v.position++];
            v.delay = std::max (0, v.delay - numSamples);
            if (v.position >= s.size())
                v = {};
        }

        const auto g = gain.load();
        for (int i = 0; i < numSamples; ++i)
            out[i] = softLimit (out[i] * g);

        for (int ch = 1; ch < numOutputChannels; ++ch)
            if (outputChannelData[ch] != nullptr)
                std::copy (out, out + numSamples, outputChannelData[ch]);
    }

    samplesPlayed = blockStart + numSamples;
}

void MetronomeEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    sampleRate = device->getCurrentSampleRate();
    latencySamples = device->getOutputLatencyInSamples() + device->getCurrentBufferSizeSamples();

    // Render every sound at the device's sample rate (a few milliseconds of work).
    rendered.clear();
    for (const auto& sound : sounds())
        rendered.push_back ({ renderVoice (sound, Voice::accent, sampleRate),
                              renderVoice (sound, Voice::beat, sampleRate),
                              renderVoice (sound, Voice::sub, sampleRate) });
    for (auto& v : voices) v = {};
}

void MetronomeEngine::audioDeviceStopped()
{
    for (auto& v : voices) v = {};
}
} // namespace metro
