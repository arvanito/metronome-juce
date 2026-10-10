#include "Sounds.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
#include <utility>

namespace metro
{
namespace
{
    // -----------------------------------------------------------------------
    // Synthesis helpers. The sounds keep most of their energy below ~3 kHz and avoid
    // instant attacks, which is what makes metronome clicks sound harsh.

    constexpr double tau = 6.283185307179586;

    template <typename Fn>
    Samples synth (double sr, double duration, Fn&& fn)
    {
        Samples out ((size_t) std::ceil (duration * sr));
        for (size_t i = 0; i < out.size(); ++i)
            out[i] = (float) fn ((double) i / sr);
        return out;
    }

    /** Exponentially decaying sine: one resonance of a struck object (modal synthesis). */
    double mode (double t, double freq, double decay, double amp = 1.0)
    {
        return amp * std::exp (-t / decay) * std::sin (tau * freq * t);
    }

    /** Struck object made of several resonances: {frequency ratio, amplitude, decay}. */
    double modes (double t, double freq, std::initializer_list<std::array<double, 3>> partials)
    {
        double v = 0;
        for (const auto& [ratio, amp, decay] : partials)
            v += mode (t, freq * ratio, decay, amp);
        return v;
    }

    /** Smooth (raised-cosine) fade-in over `time` seconds, so onsets don't snap. */
    double fadeIn (double t, double time)
    {
        return t >= time ? 1.0 : 0.5 - 0.5 * std::cos (3.141592653589793 * t / time);
    }

    /** Sine whose pitch glides exponentially from `from` to `to` Hz (drum heads do this). */
    double glide (double t, double from, double to, double glideTime)
    {
        const auto phase = to * t + (from - to) * glideTime * (1.0 - std::exp (-t / glideTime));
        return std::sin (tau * phase);
    }

    /** Deterministic white noise so every render is identical. */
    Samples noise (double sr, double duration, uint32_t seed = 1)
    {
        auto s = seed;
        return synth (sr, duration, [&] (double) {
            s = s * 1664525u + 1013904223u;
            return (double) s / 2147483648.0 - 1.0;
        });
    }

    /** Two-pole low-pass (two cascaded one-pole filters). */
    Samples lowpass (Samples buf, double sr, double cutoff)
    {
        const auto a = std::exp (-tau * cutoff / sr);
        double y1 = 0, y2 = 0;
        for (auto& x : buf)
        {
            y1 = (1 - a) * x + a * y1;
            y2 = (1 - a) * y1 + a * y2;
            x = (float) y2;
        }
        return buf;
    }

    /** Biquad band-pass (RBJ cookbook, 0 dB peak). */
    Samples bandpass (Samples buf, double sr, double freq, double q)
    {
        const auto w0 = tau * freq / sr;
        const auto alpha = std::sin (w0) / (2 * q);
        const auto a0 = 1 + alpha;
        const auto b0 = alpha / a0, b2 = -alpha / a0;
        const auto a1 = -2 * std::cos (w0) / a0, a2 = (1 - alpha) / a0;
        double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        for (auto& x : buf)
        {
            const double x0 = x;
            const double y0 = b0 * x0 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x0; y2 = y1; y1 = y0;
            x = (float) y0;
        }
        return buf;
    }

    /** Multiplies a buffer by a decay envelope with a soft onset. */
    Samples envelope (Samples buf, double sr, double attack, double decay)
    {
        for (size_t i = 0; i < buf.size(); ++i)
        {
            const auto t = (double) i / sr;
            buf[i] *= (float) (fadeIn (t, attack) * std::exp (-t / decay));
        }
        return buf;
    }

    /** Filtered noise burst: the "tick" of a stick or a mechanism. */
    Samples tick (double sr, double duration, double freq, double q, double decay)
    {
        return envelope (bandpass (noise (sr, duration), sr, freq, q), sr, 0.0005, decay);
    }

    Samples mix (std::initializer_list<std::pair<Samples, double>> layers)
    {
        size_t length = 0;
        for (const auto& [buf, gain] : layers)
            length = std::max (length, buf.size());
        Samples out (length, 0.0f);
        for (const auto& [buf, gain] : layers)
            for (size_t i = 0; i < buf.size(); ++i)
                out[i] += (float) (buf[i] * gain);
        return out;
    }

    /** Picks the value for a voice: {accent, beat, sub}. */
    template <typename T>
    T pick (Voice v, T accent, T beat, T sub)
    {
        return v == Voice::accent ? accent : v == Voice::beat ? beat : sub;
    }

    // -----------------------------------------------------------------------

    std::vector<SoundDef> makeSounds()
    {
        return {
            { "woodblock", "Wood block", "Warm, hollow knock. Clear without being sharp.",
              [] (Voice v, double sr) {
                  const auto f = pick (v, 880.0, 700.0, 620.0);
                  return lowpass (synth (sr, 0.12, [&] (double t) {
                      return fadeIn (t, 0.001) * modes (t, f, { { 1, 1, 0.03 }, { 2.32, 0.3, 0.015 }, { 3.9, 0.08, 0.008 } });
                  }), sr, 4000);
              } },

            { "mechanical", "Mechanical", "Classic wind-up metronome \"tock\"; the downbeat is a stronger, higher tock.",
              [] (Voice v, double sr) {
                  // {tick band, body resonance, low resonance}: the accent is pitched up a little.
                  const auto tickFreq = pick (v, 2000.0, 1700.0, 1400.0);
                  const auto body = pick (v, 1480.0, 1250.0, 1100.0);
                  const auto low = pick (v, 440.0, 380.0, 340.0);
                  return lowpass (mix ({
                      { tick (sr, 0.08, tickFreq, 3, 0.004), 0.6 },
                      { synth (sr, 0.08, [&] (double t) { return fadeIn (t, 0.0008) * (mode (t, body, 0.01) + mode (t, low, 0.018, 0.6)); }), 1.0 },
                  }), sr, 5000);
              } },

            { "marimba", "Marimba", "Mellow pitched tone (A-E-A), pleasant for long sessions.",
              [] (Voice v, double sr) {
                  const auto f = pick (v, 880.0, 659.25, 440.0);
                  return synth (sr, 0.45, [&] (double t) {
                      return fadeIn (t, 0.002) * modes (t, f, { { 1, 1, 0.12 }, { 3.99, 0.18, 0.025 }, { 10, 0.03, 0.006 } });
                  });
              } },

            { "softbeep", "Soft beep", "Round electronic tone with a gentle onset.",
              [] (Voice v, double sr) {
                  const auto f = pick (v, 880.0, 660.0, 523.25);
                  return synth (sr, 0.18, [&] (double t) { return fadeIn (t, 0.004) * (mode (t, f, 0.035) + mode (t, 2 * f, 0.02, 0.12)); });
              } },

            { "sidestick", "Side stick", "Drummer's rim click, muted and woody.",
              [] (Voice v, double sr) {
                  const auto f = pick (v, 1900.0, 1600.0, 1300.0);
                  return lowpass (mix ({
                      { tick (sr, 0.1, f, 1.5, 0.008), 0.8 },
                      { synth (sr, 0.1, [] (double t) { return fadeIn (t, 0.0008) * (mode (t, 480, 0.02) + mode (t, 1100, 0.012, 0.5)); }), 1.0 },
                  }), sr, 5000);
              } },

            { "drums", "Drums", "Kick on the downbeat, low tom on beats, short tom ghost notes on subdivisions.",
              [] (Voice v, double sr) {
                  const auto from = pick (v, 140.0, 210.0, 250.0);
                  const auto to = pick (v, 50.0, 120.0, 150.0);
                  const auto decay = pick (v, 0.2, 0.12, 0.05);
                  auto head = synth (sr, decay * 4, [&] (double t) { return fadeIn (t, 0.001) * std::exp (-t / decay) * glide (t, from, to, 0.03); });
                  return lowpass (mix ({ { std::move (head), 1.0 }, { tick (sr, 0.02, 2000, 1, 0.002), 0.15 } }), sr, 4000);
              } },

            { "bassdrum", "Bass drum", "Deep, round kick drum with a felt beater on every beat.",
              [] (Voice v, double sr) {
                  const auto from = pick (v, 95.0, 110.0, 125.0);
                  const auto to = pick (v, 45.0, 52.0, 62.0);
                  const auto decay = pick (v, 0.28, 0.2, 0.09);
                  auto head = synth (sr, decay * 4, [&] (double t) {
                      const auto env = fadeIn (t, 0.0015) * std::exp (-t / decay);
                      // Second harmonic so the drum is still heard on speakers that can't play 50 Hz.
                      return env * (glide (t, from, to, 0.035) + 0.25 * std::exp (-t / 0.06) * glide (t, 2 * from, 2 * to, 0.035));
                  });
                  auto beater = lowpass (tick (sr, 0.03, 700, 0.8, 0.005), sr, 2000);
                  return lowpass (mix ({ { std::move (head), 1.0 }, { std::move (beater), v == Voice::sub ? 0.15 : 0.3 } }), sr, 2500);
              } },

            { "deeptom", "Deep tom", "Low floor tom: bass-heavy like a drum, but with a clearer pitch.",
              [] (Voice v, double sr) {
                  const auto from = pick (v, 150.0, 130.0, 115.0);
                  const auto to = pick (v, 98.0, 87.0, 82.0);
                  const auto decay = pick (v, 0.3, 0.22, 0.1);
                  auto head = synth (sr, decay * 4, [&] (double t) {
                      const auto env = fadeIn (t, 0.0015) * std::exp (-t / decay);
                      // Drum-head overtone (ratio ~1.59) decays faster than the fundamental.
                      return env * (glide (t, from, to, 0.05) + 0.3 * std::exp (-t / 0.05) * glide (t, 1.59 * from, 1.59 * to, 0.05));
                  });
                  auto stick = lowpass (tick (sr, 0.03, 1200, 1, 0.004), sr, 3000);
                  return lowpass (mix ({ { std::move (head), 1.0 }, { std::move (stick), 0.2 } }), sr, 3000);
              } },

            { "softclick", "Soft click", "A classic click with the sharp edge taken off.",
              [] (Voice v, double sr) {
                  const auto f = pick (v, 1800.0, 1500.0, 1200.0);
                  return lowpass (mix ({
                      { tick (sr, 0.05, f, 2, 0.003), 0.7 },
                      { synth (sr, 0.05, [&] (double t) { return fadeIn (t, 0.0006) * mode (t, f, 0.006); }), 0.6 },
                  }), sr, 4000);
              } },
        };
    }

    constexpr double fadeOutTime = 0.01;
    /** Window over which loudness is measured: roughly how long a click is perceived. */
    constexpr double loudnessWindow = 0.05;
    /** Target RMS over that window, for an accent (about -7 dBFS). */
    constexpr double targetRms = 0.45;
} // namespace

const std::vector<SoundDef>& sounds()
{
    static const auto all = makeSounds();
    return all;
}

int findSound (const std::string& id)
{
    const auto& all = sounds();
    const auto it = std::find_if (all.begin(), all.end(), [&] (const SoundDef& s) { return s.id == id; });
    return it == all.end() ? 0 : (int) std::distance (all.begin(), it);
}

Samples renderVoice (const SoundDef& sound, Voice voice, double sampleRate)
{
    auto samples = sound.render (voice, sampleRate);
    if (samples.empty())
        return samples;

    // Match sounds by short-term loudness, so sustained tones and bass sounds don't come
    // out much louder than clicks and overdrive small speakers; cap the peak at the level.
    double peak = 0, energy = 0;
    const auto window = std::min (samples.size(), (size_t) std::lround (loudnessWindow * sampleRate));
    for (size_t i = 0; i < samples.size(); ++i)
    {
        peak = std::max (peak, (double) std::abs (samples[i]));
        if (i < window)
            energy += (double) samples[i] * samples[i];
    }
    const auto rms = std::sqrt (energy / (double) window);
    const auto level = (double) voiceLevels[(int) voice];
    const auto gain = peak > 0 ? level * std::min (1.0 / peak, targetRms / rms) : 0.0;

    const auto fadeLength = std::min (samples.size(), (size_t) std::lround (fadeOutTime * sampleRate));
    const auto fadeStart = samples.size() - fadeLength;
    for (size_t i = 0; i < samples.size(); ++i)
    {
        const auto fade = i < fadeStart ? 1.0
                                        : 0.5 + 0.5 * std::cos (3.141592653589793 * (double) (i - fadeStart) / (double) fadeLength);
        samples[i] = (float) (samples[i] * gain * fade);
    }
    return samples;
}
} // namespace metro
