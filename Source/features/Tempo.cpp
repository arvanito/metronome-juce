#include "../core/Tempo.h"
#include "../ui/Theme.h"
#include "Features.h"

namespace metro
{
namespace
{
    /** BPM readout (click to type), -/+ buttons that repeat while held, and a slider. */
    class Tempo final : public Feature
    {
    public:
        explicit Tempo (AppContext& c) : Feature (c)
        {
            for (auto* b : { &minus, &plus })
            {
                noFocus (*b);
                b->getProperties().set ("round", true);
                b->setRepeatSpeed (400, 60);
                addAndMakeVisible (*b);
            }
            minus.onClick = [this] { nudge (-1); };
            plus.onClick = [this] { nudge (1); };

            bpm.setFont (juce::FontOptions (84.0f, juce::Font::bold));
            bpm.setJustificationType (juce::Justification::centred);
            bpm.setEditable (true);
            bpm.setTooltip ("Click to type a tempo");
            bpm.onTextChange = [this] {
                const auto value = bpm.getText().getIntValue();
                if (value > 0)
                    ctx.state.updateSettings ([value] (Settings& s) { s.bpm = value; });
                stateChanged(); // restore the display if the input was rejected
            };
            addAndMakeVisible (bpm);

            styleCaption (unit);
            unit.setText ("BPM", juce::dontSendNotification);
            unit.setJustificationType (juce::Justification::centred);
            addAndMakeVisible (unit);

            marking.setFont (juce::FontOptions (16.0f, juce::Font::italic));
            marking.setColour (juce::Label::textColourId, colours::muted);
            marking.setJustificationType (juce::Justification::centred);
            addAndMakeVisible (marking);

            noFocus (slider);
            slider.setSliderStyle (juce::Slider::LinearHorizontal);
            slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            slider.setRange (minBpm, maxBpm, 1);
            slider.onValueChange = [this] {
                const auto value = (int) slider.getValue();
                ctx.state.updateSettings ([value] (Settings& s) { s.bpm = value; });
            };
            addAndMakeVisible (slider);

            stateChanged();
        }

        int preferredHeight (int) const override { return 190; }

        void stateChanged() override
        {
            const auto value = ctx.state.settings().bpm;
            if (! bpm.isBeingEdited())
                bpm.setText (juce::String (value), juce::dontSendNotification);
            marking.setText (tempoMarking (value), juce::dontSendNotification);
            slider.setValue (value, juce::dontSendNotification);
        }

        void resized() override
        {
            auto r = getLocalBounds();
            slider.setBounds (r.removeFromBottom (36));
            r.removeFromBottom (10);
            constexpr int button = 56;
            minus.setBounds (r.removeFromLeft (button).withSizeKeepingCentre (button, button));
            plus.setBounds (r.removeFromRight (button).withSizeKeepingCentre (button, button));
            marking.setBounds (r.removeFromBottom (22));
            unit.setBounds (r.removeFromBottom (18));
            bpm.setBounds (r);
        }

        /** Up/right and down/left change the tempo by 1, or by 5 with Shift. */
        bool shortcut (const juce::KeyPress& key) override
        {
            const auto step = key.getModifiers().isShiftDown() ? 5 : 1;
            if (key.isKeyCode (juce::KeyPress::upKey) || key.isKeyCode (juce::KeyPress::rightKey))
                nudge (step);
            else if (key.isKeyCode (juce::KeyPress::downKey) || key.isKeyCode (juce::KeyPress::leftKey))
                nudge (-step);
            else
                return false;
            return true;
        }

        void nudge (int delta)
        {
            ctx.state.updateSettings ([delta] (Settings& s) { s.bpm += delta; });
        }

    private:
        juce::TextButton minus { juce::String::fromUTF8 ("\xe2\x88\x92") }, plus { "+" };
        juce::Label bpm, unit, marking;
        juce::Slider slider;
    };
} // namespace

std::unique_ptr<Feature> createTempo (AppContext& ctx) { return std::make_unique<Tempo> (ctx); }
} // namespace metro
