#include "../ui/Theme.h"
#include "Features.h"

namespace metro
{
namespace
{
    /** Time signature and subdivision controls. */
    class Meter final : public Feature
    {
    public:
        explicit Meter (AppContext& c) : Feature (c)
        {
            styleCaption (timeSigCaption);
            timeSigCaption.setText ("Time signature", juce::dontSendNotification);
            addAndMakeVisible (timeSigCaption);
            styleCaption (subCaption);
            subCaption.setText ("Subdivision", juce::dontSendNotification);
            addAndMakeVisible (subCaption);

            for (int i = 1; i <= maxBeats; ++i)
                beats.addItem (juce::String (i), i);
            beats.onChange = [this] {
                const auto n = beats.getSelectedId();
                ctx.state.updateSettings ([n] (Settings& s) { s.beatsPerBar = n; });
            };
            for (auto u : { 2, 4, 8, 16 })
                unit.addItem (juce::String (u), u);
            unit.onChange = [this] {
                const auto u = unit.getSelectedId();
                ctx.state.updateSettings ([u] (Settings& s) { s.beatUnit = u; });
            };
            for (auto* box : { &beats, &unit })
            {
                noFocus (*box);
                addAndMakeVisible (*box);
            }
            slash.setText ("/", juce::dontSendNotification);
            slash.setJustificationType (juce::Justification::centred);
            addAndMakeVisible (slash);

            const char* labels[] { "Off", "8ths", "Triplets", "16ths" };
            for (int i = 0; i < 4; ++i)
            {
                auto& b = subButtons[(size_t) i];
                b.setButtonText (labels[i]);
                b.setRadioGroupId (1);
                b.setColour (juce::TextButton::buttonColourId, colours::surface2);
                noFocus (b);
                b.onClick = [this, i] { ctx.state.updateSettings ([i] (Settings& s) { s.subdivision = i + 1; }); };
                addAndMakeVisible (b);
            }

            stateChanged();
        }

        int preferredHeight (int) const override { return 150; }

        void stateChanged() override
        {
            const auto& s = ctx.state.settings();
            beats.setSelectedId (s.beatsPerBar, juce::dontSendNotification);
            unit.setSelectedId (s.beatUnit, juce::dontSendNotification);
            for (int i = 0; i < 4; ++i)
                subButtons[(size_t) i].setToggleState (s.subdivision == i + 1, juce::dontSendNotification);
        }

        void paint (juce::Graphics& g) override { paintCard (g, getLocalBounds().toFloat()); }

        void resized() override
        {
            auto r = getLocalBounds().reduced (14);
            timeSigCaption.setBounds (r.removeFromTop (20));
            auto row = r.removeFromTop (38);
            beats.setBounds (row.removeFromLeft (80));
            slash.setBounds (row.removeFromLeft (24));
            unit.setBounds (row.removeFromLeft (80));
            r.removeFromTop (10);
            subCaption.setBounds (r.removeFromTop (20));
            auto buttons = r.removeFromTop (38);
            const auto w = buttons.getWidth() / 4;
            for (auto& b : subButtons)
                b.setBounds (buttons.removeFromLeft (w).reduced (2, 0));
        }

    private:
        juce::Label timeSigCaption, subCaption, slash;
        juce::ComboBox beats, unit;
        std::array<juce::TextButton, 4> subButtons;
    };
} // namespace

std::unique_ptr<Feature> createMeter (AppContext& ctx) { return std::make_unique<Meter> (ctx); }
} // namespace metro
