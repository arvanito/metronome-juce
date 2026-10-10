#pragma once

#include "Types.h"

#include <juce_core/juce_core.h>

namespace metro
{
/** Clamps every field into its valid range. */
Settings sanitize (Settings s);
TrainerSettings sanitize (TrainerSettings t);

/** JSON-compatible representation, for saving settings and presets. */
juce::var toVar (const Settings& s);
juce::var toVar (const TrainerSettings& t);

/** Reads saved data; anything missing or invalid falls back to the defaults. */
Settings settingsFromVar (const juce::var& v);
TrainerSettings trainerFromVar (const juce::var& v);
} // namespace metro
