#include "../ui/Theme.h"
#include "Features.h"

namespace metro
{
namespace
{
    constexpr auto presetsKey = "presets";

    struct Preset
    {
        juce::String name;
        Settings settings;
    };

    /** Save the current setup under a name, and load or delete saved setups. Collapsible. */
    class Presets final : public Feature
    {
    public:
        explicit Presets (AppContext& c) : Feature (c)
        {
            header.getProperties().set ("flat", true);
            noFocus (header);
            header.onClick = [this] {
                expanded = ! expanded;
                updateVisibility();
                requestLayout();
            };
            addAndMakeVisible (header);

            name.setTextToShowWhenEmpty ("e.g. Song A - verse", colours::muted);
            name.setInputRestrictions (40);
            name.onReturnKey = [this] { save(); };
            addChildComponent (name);

            for (auto* b : { &saveButton, &loadButton, &deleteButton })
            {
                noFocus (*b);
                addChildComponent (*b);
            }
            saveButton.setColour (juce::TextButton::buttonColourId, colours::accent);
            saveButton.setColour (juce::TextButton::textColourOffId, colours::accentText);
            saveButton.onClick = [this] { save(); };
            loadButton.onClick = [this] { load(); };
            deleteButton.onClick = [this] { remove(); };

            list.setTextWhenNoChoicesAvailable ("No presets yet");
            list.setTextWhenNothingSelected ("Choose a preset");
            noFocus (list);
            addChildComponent (list);

            readPresets();
            updateVisibility();
        }

        int preferredHeight (int) const override { return expanded ? 150 : 52; }

        void paint (juce::Graphics& g) override { paintCard (g, getLocalBounds().toFloat()); }

        void resized() override
        {
            auto r = getLocalBounds().reduced (6);
            header.setBounds (r.removeFromTop (40));
            r = r.reduced (8, 0);
            auto row = r.removeFromTop (36);
            saveButton.setBounds (row.removeFromRight (120));
            row.removeFromRight (8);
            name.setBounds (row);
            r.removeFromTop (10);
            row = r.removeFromTop (36);
            deleteButton.setBounds (row.removeFromRight (70));
            row.removeFromRight (6);
            loadButton.setBounds (row.removeFromRight (70));
            row.removeFromRight (8);
            list.setBounds (row);
        }

    private:
        void updateVisibility()
        {
            header.setButtonText (juce::String::fromUTF8 (expanded ? "\xe2\x96\xbe  Presets" : "\xe2\x96\xb8  Presets"));
            for (juce::Component* c : { (juce::Component*) &name, (juce::Component*) &saveButton, (juce::Component*) &list,
                                        (juce::Component*) &loadButton, (juce::Component*) &deleteButton })
                c->setVisible (expanded);
        }

        void readPresets()
        {
            presets.clear();
            const auto parsed = juce::JSON::parse (ctx.properties.getValue (presetsKey, "[]"));
            if (const auto* array = parsed.getArray())
                for (const auto& p : *array)
                    if (p["name"].isString())
                        presets.push_back ({ p["name"].toString(), settingsFromVar (p["settings"]) });
            refreshList();
        }

        void writePresets()
        {
            juce::Array<juce::var> array;
            for (const auto& p : presets)
            {
                auto* o = new juce::DynamicObject();
                o->setProperty ("name", p.name);
                o->setProperty ("settings", toVar (p.settings));
                array.add (juce::var (o));
            }
            ctx.properties.setValue (presetsKey, juce::JSON::toString (juce::var (array), true));
            refreshList();
        }

        void refreshList()
        {
            list.clear (juce::dontSendNotification);
            for (size_t i = 0; i < presets.size(); ++i)
            {
                const auto& s = presets[i].settings;
                list.addItem (presets[i].name + "   (" + juce::String (s.bpm) + " BPM, " + juce::String (s.beatsPerBar)
                                  + "/" + juce::String (s.beatUnit) + ")",
                              (int) i + 1);
            }
        }

        void save()
        {
            const auto label = name.getText().trim();
            if (label.isEmpty())
            {
                name.grabKeyboardFocus();
                return;
            }
            std::erase_if (presets, [&] (const Preset& p) { return p.name == label; });
            presets.push_back ({ label, ctx.state.settings() });
            writePresets();
            name.clear();
            list.setSelectedId ((int) presets.size(), juce::dontSendNotification);
        }

        void load()
        {
            const auto index = list.getSelectedId() - 1;
            if (index < 0)
                return;
            // Volume stays a device preference rather than part of a preset.
            ctx.state.updateSettings ([&] (Settings& s) {
                const auto volume = s.volume;
                s = presets[(size_t) index].settings;
                s.volume = volume;
            });
        }

        void remove()
        {
            const auto index = list.getSelectedId() - 1;
            if (index < 0)
                return;
            presets.erase (presets.begin() + index);
            writePresets();
        }

        bool expanded = false;
        std::vector<Preset> presets;
        juce::TextButton header, saveButton { "Save current" }, loadButton { "Load" }, deleteButton { "Delete" };
        juce::TextEditor name;
        juce::ComboBox list;
    };
} // namespace

std::unique_ptr<Feature> createPresets (AppContext& ctx) { return std::make_unique<Presets> (ctx); }
} // namespace metro
