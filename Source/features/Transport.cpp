#include "../core/TapTempo.h"
#include "../ui/Theme.h"
#include "Features.h"

namespace metro
{
namespace
{
    /** Start/stop button and tap tempo. Keyboard shortcuts live in MainComponent. */
    class Transport final : public Feature
    {
    public:
        explicit Transport (AppContext& c) : Feature (c)
        {
            noFocus (play);
            play.setColour (juce::TextButton::buttonColourId, colours::accent);
            play.setColour (juce::TextButton::textColourOffId, colours::accentText);
            play.onClick = [this] { ctx.state.setPlaying (! ctx.state.isPlaying()); };
            addAndMakeVisible (play);

            noFocus (tap);
            tap.setTriggeredOnMouseDown (true); // a tap counts when the finger lands
            tap.setTooltip ("Tap tempo (T)");
            tap.onClick = [this] { tapNow(); };
            addAndMakeVisible (tap);

            stateChanged();
        }

        int preferredHeight (int) const override { return 64; }

        void stateChanged() override
        {
            const auto playing = ctx.state.isPlaying();
            play.setButtonText (playing ? "Stop" : "Start");
            play.setColour (juce::TextButton::buttonColourId, playing ? colours::text : colours::accent);
        }

        void resized() override
        {
            auto r = getLocalBounds();
            tap.setBounds (r.removeFromRight (96));
            r.removeFromRight (12);
            play.setBounds (r);
        }

        /** Space starts and stops; T taps the tempo. */
        bool shortcut (const juce::KeyPress& key) override
        {
            if (key == juce::KeyPress::spaceKey)
                ctx.state.setPlaying (! ctx.state.isPlaying());
            else if (juce::CharacterFunctions::toLowerCase (key.getTextCharacter()) == 't')
                tapNow();
            else
                return false;
            return true;
        }

        void tapNow()
        {
            if (const auto bpm = tapper.tap (juce::Time::getMillisecondCounterHiRes()))
                ctx.state.updateSettings ([v = *bpm] (Settings& s) { s.bpm = (int) std::lround (v); });
        }

    private:
        juce::TextButton play, tap { "Tap" };
        TapTempo tapper;
    };
} // namespace

std::unique_ptr<Feature> createTransport (AppContext& ctx) { return std::make_unique<Transport> (ctx); }
} // namespace metro
