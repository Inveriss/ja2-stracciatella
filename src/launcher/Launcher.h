#ifndef JA2_LAUNCHER_H_H
#define JA2_LAUNCHER_H_H

#include "StracciatellaLauncher.h"
#include "RustInterface.h"

#include <string_theory/string>

#include <cstdint>
#include <iostream>
#include <iterator>
#include <set>
#include <optional>
#include <string>

struct sortMods {
    bool operator() (ST::string a, ST::string b) const {
        return a.compare(b) < 0;
    }
};

#define SIMPLIFIED_CHINESE_MOD_NAME "simplified-chinese-localization"

class Launcher : public StracciatellaLauncher {
public:
	Launcher(int argc, char* argv[]);
	~Launcher();

	void loadJa2Json();
	void show();
	void initializeInputsFromDefaults();
	int writeJsonFile();
private:
	int argc;
	char** argv;
	RustPointer<EngineOptions> engineOptions;
	RustPointer<ModManager> modManager;
	std::optional<RustPointer<SubProcess>> subProcess;
	Fl_Text_Buffer logsBuffer;

	void populateChoices();
	void startExecutable(bool asEditor);
	// Desktop resolution of the primary monitor, read once at startup (see
	// detectDesktopResolution()); 0x0 if it couldn't be read.
	uint16_t desktopWidth = 0;
	uint16_t desktopHeight = 0;
	bool desktopKnown() const { return desktopWidth != 0 && desktopHeight != 0; }
	bool baseResolutionFits(size_t index) const;
	Fl_Round_Button* baseResolutionRadio(size_t index) const;
	Fl_Box* baseResolutionHint(size_t index) const;
	// Index of the selected base resolution radio, or -1 for none (the
	// MANUAL mode with a "res" from ja2.json that is no base resolution).
	int selectedBaseResolution() const;
	// The MANUAL mode's own selection, kept while AUTO shows its pick instead.
	int manualBaseResolution = -1;
	void selectBaseResolution(int index);
	void updateResolutionWidgets();
	// Texts the widgets' tooltip() and label() point to.
	std::string invalidResolutionTooltip;
	std::string baseResolutionHintTooltip[2];
	std::string resolutionLabelText;
	std::string baseResolutionLabelTooltip;
	// "Stretch In-Game Laptop"'s own value while its checkbox is inactive
	// (shown empty) -- see update().
	bool stretchLaptopValue = true;
	bool gameIsRunning();
	void update(bool changed);
	void updateLogs();
	void showModDetails(const ST::string& modName);
	void hideModDetails();
	static bool checkGameDirectoryForCommonMistakes(const ST::string& gameDir);
	static void openGameDirectorySelector(Fl_Widget *btn, void *userdata);
	static void openSaveGameDirectorySelector(Fl_Widget *btn, void *userdata);
	static void startGame(Fl_Widget* btn, void* userdata);
	static void startEditor(Fl_Widget* btn, void* userdata);
	static void guessVersion(Fl_Widget* btn, void* userdata);
	static void widgetChanged(Fl_Widget* widget, void* userdata);
	static void resolutionModeChanged(Fl_Widget* widget, void* userdata);
	static void reloadJa2Json(Fl_Widget* widget, void* userdata);
	static void saveJa2Json(Fl_Widget* widget, void* userdata);
	static void selectEnabledMods(Fl_Widget* widget, void* userdata);
	static void selectAvailableMods(Fl_Widget* widget, void* userdata);
	static void enableMods(Fl_Widget* widget, void* userdata);
	static void disableMods(Fl_Widget* widget, void* userdata);
	static void moveUpMods(Fl_Widget* widget, void* userdata);
	static void moveDownMods(Fl_Widget* widget, void* userdata);
	static void selectGameVersion(Fl_Widget* widget, void* userdata);
	static void maintainSubProcessState(void*);
};

#endif //JA2_LAUNCHER_H_H
