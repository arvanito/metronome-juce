#include "Settings.h"

#include "Tempo.h"
#include "Volume.h"

namespace metro
{
namespace
{
    int oneOf (int value, std::initializer_list<int> options, int fallback)
    {
        return std::find (options.begin(), options.end(), value) != options.end() ? value : fallback;
    }

    // Accents are stored compactly as a string, e.g. "SNNM" (strong, normal, normal, mute).
    juce::String accentsToString (const AccentPattern& accents)
    {
        juce::String s;
        for (auto a : accents)
            s << (a == Accent::strong ? 'S' : a == Accent::mute ? 'M' : 'N');
        return s;
    }

    AccentPattern accentsFromString (const juce::String& s)
    {
        auto accents = defaultAccents();
        for (int i = 0; i < std::min (s.length(), maxBeats); ++i)
            accents[(size_t) i] = s[i] == 'S' ? Accent::strong : s[i] == 'M' ? Accent::mute : Accent::normal;
        return accents;
    }

    int intProp (const juce::var& v, const char* key, int fallback)
    {
        const auto& p = v[key];
        return p.isInt() || p.isInt64() || p.isDouble() ? (int) std::lround ((double) p) : fallback;
    }

    double doubleProp (const juce::var& v, const char* key, double fallback)
    {
        const auto& p = v[key];
        return p.isInt() || p.isInt64() || p.isDouble() ? (double) p : fallback;
    }
} // namespace

Settings sanitize (Settings s)
{
    const Settings d;
    s.bpm = std::clamp (s.bpm, minBpm, maxBpm);
    s.beatsPerBar = std::clamp (s.beatsPerBar, 1, maxBeats);
    s.beatUnit = oneOf (s.beatUnit, { 2, 4, 8, 16 }, d.beatUnit);
    s.subdivision = oneOf (s.subdivision, { 1, 2, 3, 4 }, d.subdivision);
    s.volume = std::isfinite (s.volume) ? std::clamp (s.volume, 0.0, maxVolume) : d.volume;
    if (s.sound.empty())
        s.sound = d.sound;
    return s;
}

TrainerSettings sanitize (TrainerSettings t)
{
    t.stepBpm = std::clamp (t.stepBpm, 1, 50);
    t.everyBars = std::clamp (t.everyBars, 1, 64);
    t.targetBpm = std::clamp (t.targetBpm, minBpm, maxBpm);
    return t;
}

juce::var toVar (const Settings& s)
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("bpm", s.bpm);
    o->setProperty ("beatsPerBar", s.beatsPerBar);
    o->setProperty ("beatUnit", s.beatUnit);
    o->setProperty ("accents", accentsToString (s.accents));
    o->setProperty ("subdivision", s.subdivision);
    o->setProperty ("sound", juce::String (s.sound));
    o->setProperty ("volume", s.volume);
    return juce::var (o);
}

juce::var toVar (const TrainerSettings& t)
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("enabled", t.enabled);
    o->setProperty ("stepBpm", t.stepBpm);
    o->setProperty ("everyBars", t.everyBars);
    o->setProperty ("targetBpm", t.targetBpm);
    return juce::var (o);
}

Settings settingsFromVar (const juce::var& v)
{
    const Settings d;
    Settings s;
    s.bpm = intProp (v, "bpm", d.bpm);
    s.beatsPerBar = intProp (v, "beatsPerBar", d.beatsPerBar);
    s.beatUnit = intProp (v, "beatUnit", d.beatUnit);
    s.accents = v["accents"].isString() ? accentsFromString (v["accents"].toString()) : d.accents;
    s.subdivision = intProp (v, "subdivision", d.subdivision);
    s.sound = v["sound"].isString() ? v["sound"].toString().toStdString() : d.sound;
    s.volume = doubleProp (v, "volume", d.volume);
    return sanitize (s);
}

TrainerSettings trainerFromVar (const juce::var& v)
{
    const TrainerSettings d;
    TrainerSettings t;
    t.enabled = v["enabled"].isBool() ? (bool) v["enabled"] : d.enabled;
    t.stepBpm = intProp (v, "stepBpm", d.stepBpm);
    t.everyBars = intProp (v, "everyBars", d.everyBars);
    t.targetBpm = intProp (v, "targetBpm", d.targetBpm);
    return sanitize (t);
}
} // namespace metro
