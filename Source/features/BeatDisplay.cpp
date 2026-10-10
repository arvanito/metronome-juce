#include "../core/Rhythm.h"
#include "../ui/Theme.h"
#include "Features.h"

namespace metro
{
namespace
{
    /** Row of beat lights. Clicking a beat cycles accented -> normal -> muted. */
    class BeatDisplay final : public Feature
    {
    public:
        using Feature::Feature;

        int preferredHeight (int width) const override
        {
            const auto [perRow, rows, size] = layout (width);
            juce::ignoreUnused (perRow);
            return rows * (int) (size + gap) + 34;
        }

        void stateChanged() override
        {
            if (! ctx.state.isPlaying())
                activeBeat = -1;
            requestLayout();
            repaint();
        }

        void tickHeard (const Tick& tick) override
        {
            activeBeat = tick.pos.beat;
            activeIsSub = tick.pos.sub > 0;
            activeSince = juce::Time::getMillisecondCounterHiRes();
            repaint();
        }

        void frame (double nowMs) override
        {
            if (activeBeat >= 0 && nowMs - activeSince < 250)
                repaint();
        }

        void paint (juce::Graphics& g) override
        {
            const auto& s = ctx.state.settings();
            const auto now = juce::Time::getMillisecondCounterHiRes();
            const auto flash = juce::jlimit (0.0, 1.0, 1.0 - (now - activeSince) / 180.0);

            for (int i = 0; i < s.beatsPerBar; ++i)
            {
                auto r = circleBounds (i);
                const auto accent = s.accents[(size_t) i];
                const auto colour = accent == Accent::strong ? colours::accent : colours::beatNormal;
                const bool active = i == activeBeat;

                if (active)
                {
                    const auto scale = activeIsSub ? 1.0f + 0.12f * (float) flash : 1.25f;
                    r = r.withSizeKeepingCentre (r.getWidth() * scale, r.getHeight() * scale);
                    if (! activeIsSub)
                    {
                        g.setColour (colours::accent.withAlpha (0.3f));
                        g.fillEllipse (r.expanded (6));
                    }
                }

                if (accent == Accent::mute)
                {
                    g.setColour (colour);
                    juce::Path circle, dashed;
                    circle.addEllipse (r.reduced (1.5f));
                    const float dashes[] { 4.0f, 4.0f };
                    juce::PathStrokeType (3.0f).createDashedStroke (dashed, circle, dashes, 2);
                    g.fillPath (dashed);
                }
                else
                {
                    g.setColour (active && ! activeIsSub ? colour.brighter ((float) flash * 0.8f) : colour);
                    g.fillEllipse (r);
                }
            }

            g.setColour (colours::muted);
            g.setFont (juce::FontOptions (13.0f));
            g.drawText ("Click a beat to cycle: accented / normal / muted",
                        getLocalBounds().removeFromBottom (24), juce::Justification::centred);
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            for (int i = 0; i < ctx.state.settings().beatsPerBar; ++i)
                if (circleBounds (i).expanded (4).contains (e.position))
                {
                    ctx.state.updateSettings ([i] (Settings& s) { s.accents[(size_t) i] = cycleAccent (s.accents[(size_t) i]); });
                    return;
                }
        }

    private:
        static constexpr float gap = 12.0f;

        struct Layout { int perRow, rows; float size; };

        Layout layout (int width) const
        {
            const auto n = ctx.state.settings().beatsPerBar;
            const auto perRow = std::min (n, 8);
            const auto rows = (n + perRow - 1) / perRow;
            const auto size = std::min (40.0f, ((float) width - gap * (float) (perRow + 1)) / (float) perRow);
            return { perRow, rows, size };
        }

        juce::Rectangle<float> circleBounds (int index) const
        {
            const auto [perRow, rows, size] = layout (getWidth());
            juce::ignoreUnused (rows);
            const auto row = index / perRow, col = index % perRow;
            const auto inRow = std::min (perRow, ctx.state.settings().beatsPerBar - row * perRow);
            const auto rowWidth = (float) inRow * size + (float) (inRow - 1) * gap;
            const auto x = ((float) getWidth() - rowWidth) / 2 + (float) col * (size + gap);
            const auto y = gap / 2 + (float) row * (size + gap);
            return { x, y, size, size };
        }

        int activeBeat = -1;
        bool activeIsSub = false;
        double activeSince = 0;
    };
} // namespace

std::unique_ptr<Feature> createBeatDisplay (AppContext& ctx) { return std::make_unique<BeatDisplay> (ctx); }
} // namespace metro
