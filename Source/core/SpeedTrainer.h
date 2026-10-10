#pragma once

#include "Types.h"

#include <algorithm>

namespace metro
{
/** True when barsCompleted bars have been played and it is time for a tempo step. */
constexpr bool shouldStep (int64_t barsCompleted, int everyBars)
{
    return barsCompleted > 0 && everyBars > 0 && barsCompleted % everyBars == 0;
}

/** Moves bpm one step towards the target without overshooting it. */
constexpr int trainerStep (int bpm, const TrainerSettings& t)
{
    if (bpm < t.targetBpm) return std::min (bpm + t.stepBpm, t.targetBpm);
    if (bpm > t.targetBpm) return std::max (bpm - t.stepBpm, t.targetBpm);
    return bpm;
}
} // namespace metro
