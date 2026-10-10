#pragma once

#include "../core/Types.h"

#include <functional>
#include <string>
#include <vector>

namespace metro
{
/** The three sounding kinds of tick, used as array indices. */
enum class Voice : int { accent = 0, beat = 1, sub = 2 };
constexpr int numVoices = 3;

/** Peak level of each kind of tick relative to an accent. */
constexpr float voiceLevels[numVoices] { 1.0f, 0.7f, 0.4f };

using Samples = std::vector<float>;

/**
 * A sound pack. render() returns raw mono samples for one click; levelling and the
 * fade-out are applied by renderVoice(). Ported from the web app's sounds.ts.
 */
struct SoundDef
{
    std::string id;
    std::string label;
    std::string description;
    std::function<Samples (Voice, double sampleRate)> render;
};

/** All sound packs, in menu order. */
const std::vector<SoundDef>& sounds();

/** Index of the sound with this id, or 0 if there is none. */
int findSound (const std::string& id);

/** Renders one click, levels it by loudness (peak never above the voice level) and fades out the tail. */
Samples renderVoice (const SoundDef& sound, Voice voice, double sampleRate);
} // namespace metro
