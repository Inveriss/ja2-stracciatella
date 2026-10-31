#ifndef __INTERFACE_PANELS
#define __INTERFACE_PANELS

#include "Button_System.h"
#include "Interface.h"
#include "JA2Types.h"
#include "MouseSystem.h"


enum
{
	STANCEUP_BUTTON = 0,
	UPDOWN_BUTTON,
	CLIMB_BUTTON,
	STANCEDOWN_BUTTON,
	HANDCURSOR_BUTTON,
	PREVMERC_BUTTON,
	NEXTMERC_BUTTON,
	OPTIONS_BUTTON,
	BURSTMODE_BUTTON,
	LOOK_BUTTON,
	TALK_BUTTON,
	MUTE_BUTTON,
	SM_DONE_BUTTON,
	SM_MAP_SCREEN_BUTTON,
	SM_EMAIL_BUTTON,
	SM_AIM_MEMBERS_BUTTON,
	SM_MERC_BUTTON,
	SM_BOBBYR_BUTTON,
	SM_HISTORY_BUTTON,
	SM_PERSONNEL_BUTTON,
	SM_STRATSCREEN_BUTTON,
	SM_SHORTCUTS_BUTTON,
	SM_STATS_BUTTON,
	SM_SKILLS_BUTTON,
	SM_VEHICLE_CARGO_BUTTON,
	NUM_SM_BUTTONS
};


enum
{
	TEAM_DONE_BUTTON = 0,
	TEAM_MAP_SCREEN_BUTTON,
	CHANGE_SQUAD_BUTTON,
	NUM_TEAM_BUTTONS
};


#define NEW_ITEM_CYCLE_COUNT			19
#define NEW_ITEM_CYCLES			4
#define NUM_TEAM_SLOTS				(g_ui.getTeamPanelNumSlots())


#define PASSING_ITEM_DISTANCE_OKLIFE		3
#define PASSING_ITEM_DISTANCE_NOTOKLIFE	2

#define SHOW_LOCATOR_NORMAL			1
#define SHOW_LOCATOR_FAST			2

void CreateSMPanelButtons(void);
void RemoveSMPanelButtons(void);
void InitializeSMPanel();
void ShutdownSMPanel();
void RenderSMPanel(DirtyLevel*);
void EnableSMPanelButtons(BOOLEAN fEnable, BOOLEAN fFromItemPickup);

// GUI_BUTTONs render through their own, independent pass (RenderButtons())
// that isn't gated by InItemDescriptionBox() the way the rest of the SM
// panel is -- so any bookmark button whose rectangle overlaps Infobox.sti/
// Infobox_money.sti would otherwise draw on top of it. Called from
// InternalInitItemDescriptionBox()/DeleteItemDescriptionBox() in
// Interface_Items.cc.
void HideSMBookmarkButtons(void);
void ShowSMBookmarkButtons(void);

// The 1366x768 SM panel under the full width weapon box
// (infobox_weapons_big_1366x768.png): all its buttons hidden, its clock,
// sector name, action points and bars not drawn, until shown again.
void HideSMPanelUnderItemDesc(void);
void ShowSMPanelUnderItemDesc(void);
bool IsSMPanelUnderItemDesc(void);

// Narrower version of the pair above, for the tactical-screen Stats/Skills/
// KeyRing popups (Infobox_stats.sti, Infobox_skills.sti, the key ring),
// which -- unlike Infobox.sti -- are narrow enough to only ever cover the
// right end of the bookmark row (Strategic Map/Keyboard shortcuts/
// Statistics/Skills) and the "hide empty attachment slots" checkbox, never
// the left end (Mail/AIM/MERC/BR/History/Personnel). Called from
// InitStatsPopup()/DeleteStatsPopup(), InitSkillsPopup()/DeleteSkillsPopup()
// (this file) and InitKeyRingPopup()/DeleteKeyRingPopup() (Interface_Items.cc).
void HideSMBookmarkButtonsUnderInfoPopups(void);

// The helicopter inventory panels (1366x768, tactical and strategic): their
// texts (tactical panel coordinates, moved by block on the map panel) and
// their two check boxes -- one state for both panels, unchecked in a new
// game, saved with the game (g_gameStates; missing = unchecked).
enum
{
	HELI_INV_CHECK_ENEMY_LANDING = 0, // shows the landing places and the damage risk
	HELI_INV_CHECK_WALDO,             // shows Waldo's three repair lines
	NUM_HELI_INV_CHECKS
};
void DrawHeliInventoryTexts(SGPVSurface* dst, INT16 x, INT16 y, bool map);
void GetHeliInventoryCheckboxPos(INT32 check, bool map, INT16* x, INT16* y);
bool IsHeliInventoryChecked(INT32 check);
void SetHeliInventoryChecked(INT32 check, bool checked);
void InitHeliSMCheckboxesForNewGame();
void SaveHeliSMCheckboxesToSaveGameFile();
void LoadHeliSMCheckboxesFromSaveGameFile();
void ShowSMBookmarkButtonsUnderInfoPopups(void);

// The key ring popup only ever covers the checkbox, none of the bookmark
// row buttons -- called from InitKeyRingPopup()/DeleteKeyRingPopup()
// (Interface_Items.cc).
void HideSMHideEmptySlotsCheckbox(void);
void ShowSMHideEmptySlotsCheckbox(void);


void CreateTEAMPanelButtons(void);
void RemoveTEAMPanelButtons(void);
void InitializeTEAMPanel();
void ShutdownTEAMPanel();
void RenderTEAMPanel(DirtyLevel);


void SetSMPanelCurrentMerc(SOLDIERTYPE* s);
void SetTEAMPanelCurrentMerc(void);

void         InitTEAMSlots();
SOLDIERTYPE* GetPlayerFromInterfaceTeamSlot(UINT8 ubPanelSlot);
void    RemoveAllPlayersFromSlot();
BOOLEAN      RemovePlayerFromTeamSlot(const SOLDIERTYPE* s);
void    CheckForAndAddMercToTeamPanel(SOLDIERTYPE* s);

// The 1366x768 team panel showing those aboard the helicopter (a left click
// on its window) instead of the current squad; a squad change ends it.
bool IsHelicopterPassengersView();
void EndHelicopterPassengersView();
// every tactical frame: back to the squad when the helicopter left or is
// empty, the slots again when someone got in or out
void UpdateHelicopterPassengersView();
// entering a sector with nobody of ours on the ground, everyone aboard a
// vehicle: the first one with someone aboard -- the helicopter (its
// passengers view), the Hummer, the ice cream truck (its squad) -- at once;
// false: not on the tactical screen yet
bool ShowVehicleOnSectorEntry();

void DisableTacticalTeamPanelButtons(BOOLEAN fDisable);
void RenderTownIDString(void);

void KeyRingItemPanelButtonCallback(MOUSE_REGION* pRegion, UINT32 iReason);
extern MOUSE_CALLBACK KeyRingSlotInvClickCallback;


// Tactical-screen merc-statistics popup (Infobox_stats.sti) -- opened from
// the SM panel's Statistics bookmark button (SM_STATS_BUTTON).
void InitStatsPopup(SOLDIERTYPE* pSoldier, INT16 sInvX, INT16 sInvY, INT16 sInvWidth, INT16 sInvHeight);
void RenderStatsPopup(BOOLEAN fFullRender);
void DeleteStatsPopup(void);
// Opens the stats popup for the SM panel's merc (over the team panel: the
// selected merc), or closes it -- the Statistics button.
void ToggleTacticalStatsPopup();
BOOLEAN InStatsPopup(void);

// Tactical-screen merc-skills popup (Infobox_skills.sti) -- opened from the
// SM panel's Skills bookmark button (SM_SKILLS_BUTTON). Split out of the
// merc-statistics popup above.
void InitSkillsPopup(SOLDIERTYPE* pSoldier, INT16 sInvX, INT16 sInvY, INT16 sInvWidth, INT16 sInvHeight);
void RenderSkillsPopup(BOOLEAN fFullRender);
void DeleteSkillsPopup(void);
BOOLEAN InSkillsPopup(void);


void ShowRadioLocator(SOLDIERTYPE* s, UINT8 ubLocatorSpeed);
void EndRadioLocator(SOLDIERTYPE* s);


extern MOUSE_REGION gSMPanelRegion;


extern BOOLEAN gfDisableTacticalPanelButtons;


//Used when the shop keeper interface is active
void ShopKeeperInterface_SetSMpanelButtonsState(bool enabled);

void ReEvaluateDisabledINVPanelButtons(void);

void CheckForDisabledForGiveItem(void);
void ReevaluateItemHatches(SOLDIERTYPE* s, BOOLEAN fEnable);

void HandlePanelFaceAnimations(SOLDIERTYPE* s);

void GoToMapScreenFromTactical(void);

void HandleTacticalEffectsOfEquipmentChange(SOLDIERTYPE* s, UINT32 uiInvPos, UINT16 usOldItem, UINT16 usNewItem);

void FinishAnySkullPanelAnimations(void);

SOLDIERTYPE* FindNextMercInTeamPanel(SOLDIERTYPE* prev);

void BeginKeyPanelFromKeyShortcut(void);

void UpdateForContOverPortrait(SOLDIERTYPE* s, BOOLEAN fOn);

void HandleLocateSelectMerc(SOLDIERTYPE*, bool force_select);

BOOLEAN HandleNailsVestFetish(const SOLDIERTYPE* pSoldier, UINT32 uiHandPos, UINT16 usReplaceItem);

extern SOLDIERTYPE* gpSMCurrentMerc;
// The SM panel's merc; on a vehicle's cargo panel the merc who opened it
SOLDIERTYPE* GetSMPanelOwnerMerc();
extern GUIButtonRef iSMPanelButtons[NUM_SM_BUTTONS];
extern GUIButtonRef iTEAMPanelButtons[NUM_TEAM_BUTTONS];
extern GUIButtonRef giSMStealthButton;
extern SOLDIERTYPE* gSelectSMPanelToMerc;
extern MOUSE_REGION gSM_SELMERCMoneyRegion;
extern BOOLEAN      fSMKeyringIconPressed;
extern UINT8        gubHandPos;
extern UINT16       gusOldItemIndex;
extern UINT16       gusNewItemIndex;
extern BOOLEAN      gfDeductPoints;
extern BOOLEAN      gfSMDisableForItems;

void ProgressBarBackgroundRect(const INT16 sLeft, const INT16 sTop, const INT16 sWidth, const INT16 sHeight, const UINT32 rgb, const UINT8 scale_rgb);

#endif
