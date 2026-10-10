#pragma once

#include "../core/Rhythm.h"

namespace metro
{
struct Pattern
{
    double bpm = 120;
    int beatsPerBar = 4;
    int subdivision = 1;
    AccentPattern accents = defaultAccents();
};

struct Tick
{
    /** Absolute sample index at which the tick sounds. */
    int64_t sample = 0;
    Position pos;
    TickKind kind = TickKind::beat;
};

/**
 * Sample-accurate tick scheduler, driven from the audio callback. Each call to
 * process() reports every tick that falls inside the block, with its exact sample.
 * The next tick time is kept as a double so rounding never accumulates into drift.
 * It knows nothing about audio devices, so it is unit-testable.
 */
class Scheduler
{
public:
    void start (int64_t startSample)
    {
        pos = {};
        nextSample = (double) startSample;
        running = true;
    }

    void stop() { running = false; }
    bool isRunning() const { return running; }

    /**
     * @param getPattern  called before every tick, so changes apply immediately
     * @param onTick      receives each tick in [blockStart, blockStart + numSamples)
     * @param onBarStart  called with the number of completed bars before the first
     *                    tick of every bar after the first; may change the pattern
     */
    template <typename GetPattern, typename OnTick, typename OnBarStart>
    void process (int64_t blockStart, int numSamples, double sampleRate,
                  GetPattern&& getPattern, OnTick&& onTick, OnBarStart&& onBarStart)
    {
        if (! running)
            return;

        const auto blockEnd = (double) (blockStart + numSamples);
        // If we fell behind (e.g. the device restarted), resume now instead of bursting.
        if (nextSample < (double) blockStart)
            nextSample = (double) blockStart;

        while (nextSample < blockEnd)
        {
            if (pos.bar > 0 && pos.beat == 0 && pos.sub == 0)
                onBarStart (pos.bar);

            const Pattern p = getPattern();
            onTick (Tick { (int64_t) nextSample, pos, tickKind (p.accents, pos.beat, pos.sub) });
            nextSample += secondsPerTick (p.bpm, p.subdivision) * sampleRate;
            pos = advance (pos, p.beatsPerBar, p.subdivision);
        }
    }

private:
    Position pos;
    double nextSample = 0;
    bool running = false;
};
} // namespace metro
