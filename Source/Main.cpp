#include "ui/MainComponent.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace metro
{
class MetronomeApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise (const juce::String& commandLine) override
    {
        // Developer aid: `Metronome --screenshot out.png` renders the UI to an image and quits.
        if (const auto args = juce::StringArray::fromTokens (commandLine, true); args[0] == "--screenshot")
        {
            MainComponent ui;
            if (args.contains ("--expanded"))
            {
                expandPanels (ui);
                ui.setSize (ui.getWidth(), 1200); // tall enough to show everything
            }
            const auto image = ui.createComponentSnapshot (ui.getLocalBounds(), true, 1.0f);
            juce::File file (juce::File::getCurrentWorkingDirectory().getChildFile (args[1].unquoted()));
            file.deleteFile();
            juce::FileOutputStream out (file);
            juce::PNGImageFormat().writeImageToStream (image, out);
            quit();
            return;
        }
        mainWindow = std::make_unique<MainWindow> (getApplicationName());
    }
    void shutdown() override { mainWindow = nullptr; }
    void systemRequestedQuit() override { quit(); }

private:
    /** Opens every collapsible panel (their headers are "flat" buttons), for screenshots. */
    static void expandPanels (juce::Component& c)
    {
        for (auto* child : c.getChildren())
        {
            if (auto* b = dynamic_cast<juce::TextButton*> (child); b != nullptr && b->getProperties()["flat"] && b->onClick)
                b->onClick();
            expandPanels (*child);
        }
    }

    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (const juce::String& name)
            : DocumentWindow (name, colours::background, DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);
           #if JUCE_ANDROID || JUCE_IOS
            setFullScreen (true);
           #else
            setResizable (true, true);
            setResizeLimits (340, 480, 1200, 2000);
            centreWithSize (getWidth(), getHeight());
           #endif
            setVisible (true);
            getContentComponent()->grabKeyboardFocus();
        }

        void closeButtonPressed() override { JUCEApplication::getInstance()->systemRequestedQuit(); }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
};
} // namespace metro

START_JUCE_APPLICATION (metro::MetronomeApplication)
