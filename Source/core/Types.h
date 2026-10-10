#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace metro
{
/** How a beat is played: accented, normal, or silent. */
enum class Accent : uint8_t { strong, normal, mute };

/** What a single scheduled tick sounds like. */
enum class TickKind : uint8_t { accent, beat, sub, silent };

constexpr int maxBeats = 16;

using AccentPattern = std::array<Accent, maxBeats>;

/** Accent pattern with beat 1 accented and every other beat normal. */
constexpr AccentPattern defaultAccents()
{
    AccentPattern a {};
    a.fill (Accent::normal);
    a[0] = Accent::strong;
    return a;
}

struct Settings
{
    int bpm = 100;
    int beatsPerBar = 4;
    /** Bottom number of the time signature. Display only: BPM always counts this note value. */
    int beatUnit = 4;
    /** One entry per possible beat; only the first beatsPerBar are used. */
    AccentPattern accents = defaultAccents();
    /** Clicks per beat: 1 = beats only, 2 = eighths, 3 = triplets, 4 = sixteenths. */
    int subdivision = 1;
    /** Id of a sound in engine/Sounds.h. */
    std::string sound = "woodblock";
    /** 0..maxVolume: 1 is full clean level, above 1 is boost (see core/Volume.h). */
    double volume = 1.0;

    bool operator== (const Settings&) const = default;
};

struct TrainerSettings
{
    bool enabled = false;
    /** BPM added (or removed, if the target is lower) at each step. */
    int stepBpm = 5;
    /** Bars played between steps. */
    int everyBars = 4;
    /** Tempo at which the trainer stops changing the BPM. */
    int targetBpm = 140;

    bool operator== (const TrainerSettings&) const = default;
};
} // namespace metro
