#pragma once

#include "Types.h"

namespace metro
{
/** Position of a tick: bars since start, beat within the bar, subdivision within the beat. */
struct Position
{
    int64_t bar = 0;
    int beat = 0;
    int sub = 0;

    bool operator== (const Position&) const = default;
};

constexpr Accent cycleAccent (Accent a)
{
    return a == Accent::strong ? Accent::normal : a == Accent::normal ? Accent::mute : Accent::strong;
}

constexpr TickKind tickKind (const AccentPattern& accents, int beat, int sub)
{
    if (sub > 0)
        return TickKind::sub;

    switch (beat >= 0 && beat < maxBeats ? accents[(size_t) beat] : Accent::normal)
    {
        case Accent::strong: return TickKind::accent;
        case Accent::normal: return TickKind::beat;
        case Accent::mute:   return TickKind::silent;
    }
    return TickKind::beat;
}

constexpr double secondsPerTick (double bpm, int subdivision)
{
    return 60.0 / bpm / subdivision;
}

/**
 * Moves to the next tick. Uses >= so that shrinking the bar or the subdivision
 * mid-bar wraps cleanly instead of running past the end.
 */
constexpr Position advance (Position pos, int beatsPerBar, int subdivision)
{
    if (++pos.sub >= subdivision)
    {
        pos.sub = 0;
        if (++pos.beat >= beatsPerBar)
        {
            pos.beat = 0;
            ++pos.bar;
        }
    }
    return pos;
}
} // namespace metro
