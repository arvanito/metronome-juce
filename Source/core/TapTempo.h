#pragma once

#include <deque>
#include <optional>

namespace metro
{
/** Averages the intervals between recent taps. Time is passed in, so it is easy to test. */
class TapTempo
{
public:
    explicit TapTempo (double resetAfterMs = 2000.0, size_t maxTaps = 8)
        : resetAfterMs (resetAfterMs), maxTaps (maxTaps) {}

    /** Registers a tap and returns the measured BPM, or nothing until there are two taps. */
    std::optional<double> tap (double nowMs)
    {
        if (! taps.empty() && nowMs - taps.back() > resetAfterMs)
            taps.clear();

        taps.push_back (nowMs);
        if (taps.size() > maxTaps)
            taps.pop_front();

        if (taps.size() < 2)
            return std::nullopt;

        const auto avgMs = (taps.back() - taps.front()) / (double) (taps.size() - 1);
        return 60000.0 / avgMs;
    }

    void reset() { taps.clear(); }

private:
    double resetAfterMs;
    size_t maxTaps;
    std::deque<double> taps;
};
} // namespace metro
