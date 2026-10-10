#include "../core/Volume.h"
#include "../engine/Sounds.h"
#include "../ui/Theme.h"
#include "Features.h"

#include <juce_audio_utils/juce_audio_utils.h>

namespace metro
{
namespace
{
    /** Sound pack picker, volume (with boost above 100%) and audio device settings. */
    class Sound final : public Feature
    {
    public:
        explicit Sound (AppContext& c) : Feature (c)
        {
            styleCaption (soundCaption);
            soundCaption.setText ("Sound", juce::dontSendNotification);
            addAndMakeVisible (soundCaption);
            styleCaption (volumeCaption);
            addAndMakeVisible (volumeCaption);

            const auto& all = sounds();
            for (size_t i = 0; i < all.size(); ++i)
                sound.addItem (all[i].label, (int) i + 1);
            sound.onChange = [this] {
                const auto index = sound.getSelectedId() - 1;
                if (index >= 0)
                    ctx.state.updateSettings ([index] (Settings& s) { s.sound = sounds()[(size_t) index].id; });
            };
            noFocus (sound);
            addAndMakeVisible (sound);

            volume.setSliderStyle (juce::Slider::LinearHorizontal);
            volume.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            volume.setRange (0.0, maxVolume, 0.01);
            volume.setDoubleClickReturnValue (true, 1.0); // double-click resets to 100%
            volume.onValueChange = [this] {
                const auto v = volume.getValue();
                ctx.state.updateSettings ([v] (Settings& s) { s.volume = v; });
            };
            noFocus (volume);
            addAndMakeVisible (volume);

            noFocus (audioSettings);
            audioSettings.onClick = [this] { showAudioSettings(); };
            addAndMakeVisible (audioSettings);

            stateChanged();
        }

        int preferredHeight (int) const override { return 140; }

        void stateChanged() override
        {
            const auto& s = ctx.state.settings();
            sound.setSelectedId (findSound (s.sound) + 1, juce::dontSendNotification);
            volume.setValue (s.volume, juce::dontSendNotification);

            const auto percent = (int) std::lround (s.volume * 100);
            const auto boosted = percent > 100;
            volumeCaption.setText ("Volume " + juce::String (percent) + "%" + (boosted ? " - boost" : ""),
                                   juce::dontSendNotification);
            const auto colour = boosted ? colours::boost : colours::accent;
            volume.setColour (juce::Slider::thumbColourId, colour);
            volume.setColour (juce::Slider::trackColourId, colour);
        }

        void paint (juce::Graphics& g) override { paintCard (g, getLocalBounds().toFloat()); }

        void resized() override
        {
            auto r = getLocalBounds().reduced (14);
            audioSettings.setBounds (r.removeFromBottom (30).removeFromLeft (150));
            r.removeFromBottom (8);
            auto left = r.removeFromLeft (r.getWidth() / 2).withTrimmedRight (8);
            soundCaption.setBounds (left.removeFromTop (20));
            sound.setBounds (left.removeFromTop (38));
            volumeCaption.setBounds (r.removeFromTop (20));
            volume.setBounds (r.removeFromTop (38));
        }

    private:
        void showAudioSettings()
        {
            auto selector = std::make_unique<juce::AudioDeviceSelectorComponent> (ctx.devices, 0, 0, 1, 2, false, false, true, false);
            selector->setSize (480, 360);
            juce::DialogWindow::LaunchOptions options;
            options.content.setOwned (selector.release());
            options.dialogTitle = "Audio settings";
            options.dialogBackgroundColour = colours::surface;
            options.useNativeTitleBar = true;
            options.resizable = false;
            options.launchAsync();
        }

        juce::Label soundCaption, volumeCaption;
        juce::ComboBox sound;
        juce::Slider volume;
        juce::TextButton audioSettings { "Audio settings..." };
    };
} // namespace

std::unique_ptr<Feature> createSound (AppContext& ctx) { return std::make_unique<Sound> (ctx); }
} // namespace metro
