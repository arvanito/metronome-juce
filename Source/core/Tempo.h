#pragma once

#include <algorithm>
#include <cmath>

namespace metro
{
constexpr int minBpm = 20;
constexpr int maxBpm = 300;

inline int clampBpm (double bpm)
{
    return std::clamp ((int) std::lround (bpm), minBpm, maxBpm);
}

/** Traditional Italian tempo marking for a tempo. */
inline const char* tempoMarking (double bpm)
{
    struct Marking { double limit; const char* name; };
    static constexpr Marking markings[] {
        { 24, "Larghissimo" }, { 40, "Grave" },    { 60, "Largo" },  { 66, "Larghetto" },
        { 76, "Adagio" },      { 108, "Andante" }, { 120, "Moderato" }, { 156, "Allegro" },
        { 176, "Vivace" },     { 200, "Presto" },
    };
    for (const auto& m : markings)
        if (bpm < m.limit)
            return m.name;
    return "Prestissimo";
}
} // namespace metro
