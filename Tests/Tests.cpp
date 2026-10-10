// Unit tests for the pure logic and the engine's scheduler and sounds.
// Run the MetronomeTests target (or `ctest`); it exits non-zero on failure.

#include "../Source/core/Rhythm.h"
#include "../Source/core/Settings.h"
#include "../Source/core/SpeedTrainer.h"
#include "../Source/core/TapTempo.h"
#include "../Source/core/Tempo.h"
#include "../Source/core/Volume.h"
#include "../Source/engine/MetronomeEngine.h"
#include "../Source/engine/Scheduler.h"
#include "../Source/engine/Sounds.h"

#include <juce_core/juce_core.h>

#include <iostream>

namespace metro
{
namespace
{
    bool near (double a, double b, double tolerance = 1e-6) { return std::abs (a - b) <= tolerance; }

    class CoreTests final : public juce::UnitTest
    {
    public:
        CoreTests() : UnitTest ("Core", "metronome") {}

        void runTest() override
        {
            beginTest ("tempo");
            expectEquals (clampBpm (5), 20);
            expectEquals (clampBpm (999), 300);
            expectEquals (clampBpm (120.6), 121);
            expectEquals (juce::String (tempoMarking (50)), juce::String ("Largo"));
            expectEquals (juce::String (tempoMarking (120)), juce::String ("Allegro"));
            expectEquals (juce::String (tempoMarking (250)), juce::String ("Prestissimo"));

            beginTest ("accents");
            expect (cycleAccent (Accent::strong) == Accent::normal);
            expect (cycleAccent (Accent::normal) == Accent::mute);
            expect (cycleAccent (Accent::mute) == Accent::strong);
            auto accents = defaultAccents();
            accents[2] = Accent::mute;
            expect (tickKind (accents, 0, 0) == TickKind::accent);
            expect (tickKind (accents, 1, 0) == TickKind::beat);
            expect (tickKind (accents, 2, 0) == TickKind::silent);
            expect (tickKind (accents, 2, 1) == TickKind::sub);

            beginTest ("advance");
            expect (advance ({ 0, 0, 0 }, 4, 2) == Position { 0, 0, 1 });
            expect (advance ({ 0, 0, 1 }, 4, 2) == Position { 0, 1, 0 });
            expect (advance ({ 0, 3, 0 }, 4, 1) == Position { 1, 0, 0 });
            // Shrinking the subdivision or the bar mid-bar wraps cleanly.
            expect (advance ({ 0, 0, 3 }, 4, 2) == Position { 0, 1, 0 });
            expect (advance ({ 2, 5, 0 }, 3, 1) == Position { 3, 0, 0 });

            beginTest ("tap tempo");
            TapTempo tap;
            expect (! tap.tap (0).has_value());
            expect (near (*tap.tap (500), 120));
            expect (near (*tap.tap (1000), 120));
            expect (near (*tap.tap (1600), 60000.0 / (1600.0 / 3.0)));
            TapTempo tap2 (2000);
            tap2.tap (0);
            tap2.tap (1000);
            expect (! tap2.tap (5000).has_value()); // a pause starts over
            expect (near (*tap2.tap (5250), 240));

            beginTest ("speed trainer");
            const TrainerSettings t { true, 5, 4, 112 };
            expect (! shouldStep (0, 4));
            expect (! shouldStep (3, 4));
            expect (shouldStep (4, 4));
            expect (shouldStep (8, 4));
            expectEquals (trainerStep (100, t), 105);
            expectEquals (trainerStep (110, t), 112);
            expectEquals (trainerStep (112, t), 112);
            expectEquals (trainerStep (130, t), 125);

            beginTest ("volume");
            expectEquals (volumeToGain (0), 0.0);
            expectEquals (volumeToGain (0.5), 0.25);
            expectEquals (volumeToGain (1), 1.0);
            expect (near (20 * std::log10 (volumeToGain (1.25)), 2.5, 1e-9));
            expect (near (20 * std::log10 (volumeToGain (1.5)), 5.0, 1e-9));
            expectEquals (volumeToGain (5), volumeToGain (1.5));
            expectEquals (softLimit (0.5f), 0.5f);
            for (auto x : { 0.9f, 1.0f, 2.0f, 3.2f, 100.0f })
            {
                expect (softLimit (x) <= 1.0f);
                expectEquals (softLimit (-x), -softLimit (x));
            }
            expect (softLimit (1.5f) > softLimit (1.2f));

            beginTest ("settings");
            Settings junk;
            junk.bpm = 1000;
            junk.subdivision = 7;
            junk.volume = 3;
            junk.beatsPerBar = 40;
            const auto clean = sanitize (junk);
            expectEquals (clean.bpm, 300);
            expectEquals (clean.subdivision, 1);
            expectEquals (clean.volume, maxVolume);
            expectEquals (clean.beatsPerBar, maxBeats);

            Settings custom;
            custom.bpm = 87;
            custom.beatsPerBar = 7;
            custom.accents[3] = Accent::mute;
            custom.sound = "drums";
            expect (settingsFromVar (toVar (custom)) == custom, "settings survive a save/load round trip");
            expect (settingsFromVar (juce::var()) == Settings(), "missing data falls back to defaults");

            const TrainerSettings trainer { true, 3, 2, 150 };
            expect (trainerFromVar (toVar (trainer)) == trainer);
        }
    };

    class EngineTests final : public juce::UnitTest
    {
    public:
        EngineTests() : UnitTest ("Engine", "metronome") {}

        static std::vector<Tick> run (Scheduler& s, Pattern& pattern, int64_t totalSamples, int blockSize,
                                      std::function<void (int64_t)> onBar = {})
        {
            std::vector<Tick> ticks;
            s.start (0);
            for (int64_t block = 0; block < totalSamples; block += blockSize)
                s.process (block, blockSize, 48000.0,
                           [&] { return pattern; },
                           // The last block may run past the end; ignore ticks after it.
                           [&] (const Tick& t) { if (t.sample < totalSamples) ticks.push_back (t); },
                           [&] (int64_t bar) { if (onBar) onBar (bar); });
            return ticks;
        }

        void runTest() override
        {
            beginTest ("ticks land on exact samples, independent of block size");
            for (auto blockSize : { 32, 441, 512, 1024 })
            {
                Scheduler s;
                Pattern p { 120, 4, 1, defaultAccents() };
                const auto ticks = run (s, p, 48000 * 3, blockSize);
                expectEquals ((int) ticks.size(), 6);
                for (size_t i = 0; i < ticks.size(); ++i)
                    expectEquals (ticks[i].sample, (int64_t) (i * 24000));
                expect (ticks[0].kind == TickKind::accent && ticks[1].kind == TickKind::beat && ticks[4].kind == TickKind::accent);
                expect (ticks[4].pos == Position { 1, 0, 0 });
            }

            beginTest ("no drift over a long run");
            {
                Scheduler s;
                Pattern p { 137, 4, 3, defaultAccents() }; // a tick length that is not a whole number of samples
                const auto ticks = run (s, p, 48000 * 600, 480);
                const auto last = ticks.back();
                const auto expected = (double) (ticks.size() - 1) * 60.0 / 137.0 / 3.0 * 48000.0;
                expect (std::abs ((double) last.sample - expected) <= 1.0, "ten minutes in, still within one sample");
            }

            beginTest ("subdivisions");
            {
                Scheduler s;
                Pattern p { 60, 2, 3, defaultAccents() };
                const auto ticks = run (s, p, 48000, 256);
                expectEquals ((int) ticks.size(), 3);
                expect (ticks[1].kind == TickKind::sub && ticks[1].pos.sub == 1);
                expectEquals (ticks[1].sample, (int64_t) 16000);
            }

            beginTest ("onBarStart can change the tempo for the next bar");
            {
                Scheduler s;
                Pattern p { 60, 2, 1, defaultAccents() };
                std::vector<int64_t> bars;
                const auto ticks = run (s, p, 48000 * 3, 512, [&] (int64_t bar) { bars.push_back (bar); p.bpm = 120; });
                expectEquals (bars.front(), (int64_t) 1);
                // Bar 0 at 60 BPM (48000 samples apart), then 120 BPM (24000 apart).
                expectEquals (ticks[1].sample, (int64_t) 48000);
                expectEquals (ticks[2].sample, (int64_t) 96000);
                expectEquals (ticks[3].sample, (int64_t) 120000);
            }

            beginTest ("stopped scheduler is silent");
            {
                Scheduler s;
                s.start (0);
                s.stop();
                int count = 0;
                s.process (0, 48000, 48000.0, [] { return Pattern {}; }, [&] (const Tick&) { ++count; }, [] (int64_t) {});
                expectEquals (count, 0);
            }

            beginTest ("sounds are levelled, not clipping, and die away");
            for (const auto& sound : sounds())
                for (auto voice : { Voice::accent, Voice::beat, Voice::sub })
                {
                    const auto x = renderVoice (sound, voice, 48000);
                    const auto level = voiceLevels[(int) voice];
                    expect (x.size() > 100, sound.id);
                    float peak = 0;
                    for (auto v : x)
                        peak = std::max (peak, std::abs (v));
                    expect (peak <= level + 1e-6f, sound.id + " peaks above its level");
                    expect (peak > level * 0.3f, sound.id + " is much too quiet");
                    expect (std::abs (x.back()) < 0.05f, sound.id + " does not fade out");

                    // Not shrill: RMS of the first difference over RMS stays below 0.55 (~4 kHz centroid).
                    double e = 0, d = 0;
                    for (size_t i = 1; i < x.size(); ++i)
                    {
                        e += (double) x[i] * x[i];
                        d += (double) (x[i] - x[i - 1]) * (x[i] - x[i - 1]);
                    }
                    expect (std::sqrt (d / e) < 0.55, sound.id + " is too bright");
                }

            beginTest ("sound lookup");
            expectEquals (findSound ("drums"), 5);
            expectEquals (findSound ("no-such-sound"), 0);
        }
    };

    /** Minimal stand-in for a sound card, so the real engine can be driven without hardware. */
    class FakeDevice final : public juce::AudioIODevice
    {
    public:
        FakeDevice() : AudioIODevice ("Fake", "Fake") {}
        juce::StringArray getOutputChannelNames() override { return { "L", "R" }; }
        juce::StringArray getInputChannelNames() override { return {}; }
        juce::Array<double> getAvailableSampleRates() override { return { 48000.0 }; }
        juce::Array<int> getAvailableBufferSizes() override { return { blockSize }; }
        int getDefaultBufferSize() override { return blockSize; }
        juce::String open (const juce::BigInteger&, const juce::BigInteger&, double, int) override { return {}; }
        void close() override {}
        bool isOpen() override { return true; }
        void start (juce::AudioIODeviceCallback*) override {}
        void stop() override {}
        bool isPlaying() override { return true; }
        juce::String getLastError() override { return {}; }
        int getCurrentBufferSizeSamples() override { return blockSize; }
        double getCurrentSampleRate() override { return 48000.0; }
        int getCurrentBitDepth() override { return 32; }
        juce::BigInteger getActiveOutputChannels() const override { return 3; }
        juce::BigInteger getActiveInputChannels() const override { return 0; }
        int getOutputLatencyInSamples() override { return 0; }
        int getInputLatencyInSamples() override { return 0; }

        static constexpr int blockSize = 480;
    };

    class MetronomeEngineTests final : public juce::UnitTest
    {
    public:
        MetronomeEngineTests() : UnitTest ("MetronomeEngine", "metronome") {}

        void runTest() override
        {
            beginTest ("renders clicks at the right samples, on every channel");
            MetronomeEngine engine;
            Settings s;
            s.bpm = 120;
            s.sound = "softclick";
            engine.apply (s, {});
            FakeDevice device;
            engine.audioDeviceAboutToStart (&device);
            engine.setPlaying (true);

            constexpr int block = FakeDevice::blockSize;
            std::vector<float> left, right;
            juce::AudioBuffer<float> buffer (2, block);
            for (int b = 0; b < 100; ++b) // 1 second
            {
                engine.audioDeviceIOCallbackWithContext (nullptr, 0, buffer.getArrayOfWritePointers(), 2, block, {});
                left.insert (left.end(), buffer.getReadPointer (0), buffer.getReadPointer (0) + block);
                right.insert (right.end(), buffer.getReadPointer (1), buffer.getReadPointer (1) + block);
            }

            // At 120 BPM: clicks start at samples 0 and 24000, silence well before each.
            auto energy = [&] (int from, int to) {
                double e = 0;
                for (int i = from; i < to; ++i)
                    e += (double) left[(size_t) i] * left[(size_t) i];
                return e;
            };
            expect (energy (0, 480) > 0.1, "click at the start");
            expect (energy (20000, 23990) < 1e-9, "silence before the second beat");
            expect (energy (24000, 24480) > 0.05, "click on the second beat");
            expect (left == right, "both channels carry the same signal");

            float peak = 0;
            for (auto v : left)
                peak = std::max (peak, std::abs (v));
            expect (peak <= 1.0f, "never clips");

            beginTest ("reports heard ticks in order");
            std::vector<Tick> heard;
            while (auto t = engine.popHeardTick())
                heard.push_back (*t);
            expectEquals ((int) heard.size(), 2);
            expectEquals (heard[0].sample, (int64_t) 0);
            expectEquals (heard[1].sample, (int64_t) 24000);

            beginTest ("stops cleanly");
            engine.setPlaying (false);
            for (int b = 0; b < 10; ++b) // let the last click ring out
                engine.audioDeviceIOCallbackWithContext (nullptr, 0, buffer.getArrayOfWritePointers(), 2, block, {});
            engine.audioDeviceIOCallbackWithContext (nullptr, 0, buffer.getArrayOfWritePointers(), 2, block, {});
            expectEquals (buffer.getMagnitude (0, 0, block), 0.0f);
        }
    };

    CoreTests coreTests;
    EngineTests engineTests;
    MetronomeEngineTests metronomeEngineTests;
} // namespace
} // namespace metro

int main()
{
    // JUCE logs test results to the debugger by default; print them to the console instead.
    struct ConsoleRunner final : juce::UnitTestRunner
    {
        void logMessage (const juce::String& message) override { std::cout << message << std::endl; }
    };

    ConsoleRunner runner;
    runner.setAssertOnFailure (false);
    runner.runTestsInCategory ("metronome");

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult (i)->failures;
    return failures == 0 ? 0 : 1;
}
