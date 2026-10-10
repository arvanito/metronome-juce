#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace metro
{
/** Colours shared with the web app's dark theme. */
namespace colours
{
    const juce::Colour background { 0xff14171f };
    const juce::Colour surface { 0xff1d212b };
    const juce::Colour surface2 { 0xff272c38 };
    const juce::Colour text { 0xffeef0f4 };
    const juce::Colour muted { 0xff979cab };
    const juce::Colour border { 0xff323846 };
    const juce::Colour accent { 0xffff7a45 };
    const juce::Colour accentText { 0xff14171f };
    const juce::Colour beatNormal { 0xff6c7282 };
    const juce::Colour boost { 0xffff5a4f };
} // namespace colours

constexpr float cornerRadius = 12.0f;

class Theme : public juce::LookAndFeel_V4
{
public:
    Theme()
    {
        using namespace juce;
        setColourScheme ({ colours::surface, colours::background, colours::surface, colours::border, colours::text,
                           colours::accent, colours::accentText, colours::surface2, colours::text });

        setColour (ResizableWindow::backgroundColourId, colours::background);
        setColour (TextButton::buttonColourId, colours::surface);
        setColour (TextButton::buttonOnColourId, colours::accent);
        setColour (TextButton::textColourOffId, colours::text);
        setColour (TextButton::textColourOnId, colours::accentText);
        setColour (ComboBox::backgroundColourId, colours::background);
        setColour (ComboBox::outlineColourId, colours::border);
        setColour (ComboBox::arrowColourId, colours::muted);
        setColour (PopupMenu::backgroundColourId, colours::surface);
        setColour (PopupMenu::highlightedBackgroundColourId, colours::surface2);
        setColour (Label::textColourId, colours::text);
        setColour (Label::textWhenEditingColourId, colours::text);
        setColour (Slider::thumbColourId, colours::accent);
        setColour (Slider::trackColourId, colours::accent);
        setColour (Slider::backgroundColourId, colours::surface2);
        setColour (Slider::textBoxTextColourId, colours::text);
        setColour (Slider::textBoxBackgroundColourId, colours::background);
        setColour (Slider::textBoxOutlineColourId, colours::border);
        setColour (TextEditor::backgroundColourId, colours::background);
        setColour (TextEditor::outlineColourId, colours::border);
        setColour (TextEditor::focusedOutlineColourId, colours::accent);
        setColour (TextEditor::textColourId, colours::text);
        setColour (ToggleButton::textColourId, colours::text);
        setColour (ToggleButton::tickColourId, colours::accent);
        setColour (ToggleButton::tickDisabledColourId, colours::muted);
        setColour (ScrollBar::thumbColourId, colours::surface2);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                               bool isHighlighted, bool isDown) override
    {
        // Buttons can opt into a style: "flat" (no background, for panel headers) or "round" (circle).
        if (button.getProperties()["flat"])
        {
            if (isHighlighted || isDown)
            {
                g.setColour (colours::surface2.withAlpha (isDown ? 0.8f : 0.5f));
                g.fillRoundedRectangle (button.getLocalBounds().toFloat(), cornerRadius);
            }
            return;
        }

        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        const auto radius = button.getProperties()["round"] ? bounds.getHeight() / 2
                                                            : std::min (cornerRadius, bounds.getHeight() / 2);
        auto fill = button.getToggleState() ? button.findColour (juce::TextButton::buttonOnColourId) : backgroundColour;
        if (isDown)
            fill = fill.darker (0.15f);
        else if (isHighlighted)
            fill = fill.brighter (0.06f);
        g.setColour (fill);
        g.fillRoundedRectangle (bounds, radius);
        if (! button.getToggleState())
        {
            g.setColour (colours::border);
            g.drawRoundedRectangle (bounds, radius, 1.0f);
        }
    }

    juce::Font getTextButtonFont (juce::TextButton& button, int buttonHeight) override
    {
        const auto size = button.getProperties()["round"] ? (float) buttonHeight * 0.5f
                                                          : std::min (20.0f, (float) buttonHeight * 0.45f);
        return juce::Font (juce::FontOptions (size, juce::Font::bold));
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool isHighlighted, bool isDown) override
    {
        if (! button.getProperties()["flat"])
            return LookAndFeel_V4::drawButtonText (g, button, isHighlighted, isDown);

        // Panel headers: left-aligned, like a disclosure triangle.
        g.setFont (juce::FontOptions (17.0f, juce::Font::bold));
        g.setColour (colours::text);
        g.drawText (button.getButtonText(), button.getLocalBounds().reduced (10, 0), juce::Justification::centredLeft);
    }
};

/** Background card used to group controls. */
inline void paintCard (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour (colours::surface);
    g.fillRoundedRectangle (bounds, cornerRadius);
    g.setColour (colours::border);
    g.drawRoundedRectangle (bounds.reduced (0.5f), cornerRadius, 1.0f);
}

/** Keeps keyboard focus on the main window, so shortcuts keep working after clicks. */
inline void noFocus (juce::Component& c)
{
    c.setWantsKeyboardFocus (false);
    c.setMouseClickGrabsKeyboardFocus (false);
}

inline void styleCaption (juce::Label& label)
{
    label.setFont (juce::FontOptions (13.0f));
    label.setColour (juce::Label::textColourId, colours::muted);
}
} // namespace metro
