#include "../core/Tempo.h"
#include "../ui/Theme.h"
#include "Features.h"

namespace metro
{
namespace
{
    /** Gradually changes the tempo: +/-N BPM every M bars until a target tempo. Collapsible. */
    class SpeedTrainer final : public Feature
    {
    public:
        explicit SpeedTrainer (AppContext& c) : Feature (c)
        {
            header.getProperties().set ("flat", true);
            noFocus (header);
            header.onClick = [this] {
                expanded = ! expanded;
                updateVisibility();
                requestLayout();
            };
            addAndMakeVisible (header);

            noFocus (enabled);
            enabled.onClick = [this] {
                const auto on = enabled.getToggleState();
                ctx.state.updateTrainer ([on] (TrainerSettings& t) { t.enabled = on; });
            };
            addChildComponent (enabled);

            setUp (step, stepCaption, "Change by (BPM)", 1, 50, [] (TrainerSettings& t, int v) { t.stepBpm = v; });
            setUp (every, everyCaption, "Every (bars)", 1, 64, [] (TrainerSettings& t, int v) { t.everyBars = v; });
            setUp (target, targetCaption, "Up to (BPM)", minBpm, maxBpm, [] (TrainerSettings& t, int v) { t.targetBpm = v; });

            styleCaption (status);
            addChildComponent (status);

            updateVisibility();
            stateChanged();
        }

        int preferredHeight (int) const override { return expanded ? 184 : 52; }

        void stateChanged() override
        {
            const auto& t = ctx.state.trainer();
            enabled.setToggleState (t.enabled, juce::dontSendNotification);
            step.setValue (t.stepBpm, juce::dontSendNotification);
            every.setValue (t.everyBars, juce::dontSendNotification);
            target.setValue (t.targetBpm, juce::dontSendNotification);
            updateStatus();
        }

        void frame (double) override
        {
            // Bars are counted on the audio thread; refresh the countdown when one completes.
            const auto bars = ctx.engine.barsCompleted();
            if (bars != lastBars)
            {
                lastBars = bars;
                updateStatus();
            }
        }

        void paint (juce::Graphics& g) override { paintCard (g, getLocalBounds().toFloat()); }

        void resized() override
        {
            auto r = getLocalBounds().reduced (6);
            header.setBounds (r.removeFromTop (40));
            r = r.reduced (8, 0);
            enabled.setBounds (r.removeFromTop (30));
            r.removeFromTop (6);
            auto captions = r.removeFromTop (20);
            auto controls = r.removeFromTop (34);
            const auto w = r.getWidth() / 3;
            for (auto [caption, slider] : { std::pair { &stepCaption, &step }, { &everyCaption, &every }, { &targetCaption, &target } })
            {
                caption->setBounds (captions.removeFromLeft (w));
                slider->setBounds (controls.removeFromLeft (w).reduced (2, 0));
            }
            r.removeFromTop (8);
            status.setBounds (r.removeFromTop (22));
        }

    private:
        template <typename Apply>
        void setUp (juce::Slider& slider, juce::Label& caption, const char* text, int min, int max, Apply apply)
        {
            styleCaption (caption);
            caption.setText (text, juce::dontSendNotification);
            addChildComponent (caption);

            slider.setSliderStyle (juce::Slider::IncDecButtons);
            slider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 44, 30);
            slider.setRange (min, max, 1);
            slider.onValueChange = [this, &slider, apply] {
                const auto v = (int) slider.getValue();
                ctx.state.updateTrainer ([v, apply] (TrainerSettings& t) { apply (t, v); });
            };
            addChildComponent (slider);
        }

        void updateVisibility()
        {
            header.setButtonText (juce::String::fromUTF8 (expanded ? "\xe2\x96\xbe  Speed trainer" : "\xe2\x96\xb8  Speed trainer"));
            for (juce::Component* c : { (juce::Component*) &enabled, (juce::Component*) &step, (juce::Component*) &every,
                                        (juce::Component*) &target, (juce::Component*) &stepCaption, (juce::Component*) &everyCaption,
                                        (juce::Component*) &targetCaption, (juce::Component*) &status })
                c->setVisible (expanded);
        }

        void updateStatus()
        {
            const auto& t = ctx.state.trainer();
            juce::String text;
            if (! t.enabled)
                text = "";
            else if (ctx.state.settings().bpm == t.targetBpm)
                text = "At target tempo (" + juce::String (t.targetBpm) + " BPM).";
            else if (! ctx.state.isPlaying())
                text = "Starts changing the tempo once you press Start.";
            else
            {
                const auto left = t.everyBars - (int) (lastBars % t.everyBars);
                text = "Next change in " + juce::String (left) + (left == 1 ? " bar." : " bars.");
            }
            status.setText (text, juce::dontSendNotification);
        }

        bool expanded = false;
        int64_t lastBars = 0;
        juce::TextButton header;
        juce::ToggleButton enabled { "Enabled" };
        juce::Slider step, every, target;
        juce::Label stepCaption, everyCaption, targetCaption, status;
    };
} // namespace

std::unique_ptr<Feature> createSpeedTrainer (AppContext& ctx) { return std::make_unique<SpeedTrainer> (ctx); }
} // namespace metro
