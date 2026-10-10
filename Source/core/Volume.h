#pragma once

#include <algorithm>
#include <cmath>

namespace metro
{
/** Slider range: 0..1 is normal volume, 1..maxVolume is boost. */
constexpr double maxVolume = 1.5;
/** Boost at the top of the slider, in dB. */
constexpr double maxBoostDb = 5.0;

/**
 * Maps the volume slider (0..maxVolume) to a linear gain.
 * Below 100% the curve is squared, which feels even to the ear; above 100% each
 * step adds the same number of decibels, up to maxBoostDb.
 */
inline double volumeToGain (double volume)
{
    if (volume <= 1.0)
        return std::pow (std::max (0.0, volume), 2.0);
    const auto boost = std::min (volume, maxVolume) - 1.0;
    return std::pow (10.0, boost / (maxVolume - 1.0) * maxBoostDb / 20.0);
}

/**
 * Soft limiter: unchanged below the knee, then smoothly approaches ±1 so loud
 * (boosted) peaks are rounded off instead of clipping.
 */
inline float softLimit (float x)
{
    constexpr float knee = 0.8f;
    const auto a = std::abs (x);
    if (a <= knee)
        return x;
    return std::copysign (knee + (1.0f - knee) * std::tanh ((a - knee) / (1.0f - knee)), x);
}
} // namespace metro
