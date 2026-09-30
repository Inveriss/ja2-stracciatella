#ifndef __MAPSCREEN_H
#define __MAPSCREEN_H

#include "Button_System.h"
#include "MessageBoxScreen.h"
#include "ScreenIDs.h"
#include "JA2Types.h"
#include <string_theory/string>


// Sector name identifiers
enum Towns
{
	BLANK_SECTOR=0,
	OMERTA,
	DRASSEN,
	ALMA,
	GRUMM,
	TIXA,
	CAMBRIA,
	SAN_MONA,
	ESTONI,
	ORTA,
	BALIME,
	MEDUNA,
	CHITZENA,
	NUM_TOWNS
};

#define FIRST_TOWN	OMERTA


extern BOOLEAN fCharacterInfoPanelDirty;
extern BOOLEAN fTeamPanelDirty;
extern BOOLEAN fMapPanelDirty;

extern BOOLEAN fMapInventoryItem;
extern BOOLEAN gfInConfirmMapMoveMode;
extern BOOLEAN gfInChangeArrivalSectorMode;

extern BOOLEAN gfSkyriderEmptyHelpGiven;


void SetInfoChar(SOLDIERTYPE const*);
void EndMapScreen( BOOLEAN fDuringFade );
void ReBuildCharactersList( void );


void HandlePreloadOfMapGraphics(void);
void HandleRemovalOfPreLoadedMapGraphics( void );

void ChangeSelectedMapSector(const SGPSector& sector);

BOOLEAN CanExtendContractForSoldier(const SOLDIERTYPE* s);

void TellPlayerWhyHeCantCompressTime( void );

// the info character
extern INT8 bSelectedInfoChar;

SOLDIERTYPE* GetSelectedInfoChar(void);
void ChangeSelectedInfoChar( INT8 bCharNumber, BOOLEAN fResetSelectedList );

void MAPEndItemPointer(void);

// "Show Large Icons" toggle of the merc inventory panel (mapinv.sti) --
// independent of the sector inventory's own toggle. TRUE only on the wide
// strategic screen (1280+), the only one the button exists on.
BOOLEAN IsMapInvBigImages(void);

// Size of mapinv_big_1280_720/768.sti, the "Show Large Icons" merc inventory panel.
// Drawn at MAP_SCREEN_X, MAP_SCREEN_Y + 107 on every 1280+ resolution,
// whatever the screen height -- so it reaches down over the left part of
// map_screen_bottom's strip (see MapInvBigPanelCoversBottomStrip()).
#define MAP_INV_BIG_PANEL_WIDTH   518
// mapinv_big_1280_720.sti is 518x613 (screen height 720-767),
// mapinv_big_1280_768.sti 518x661 (768+).
#define MAP_INV_BIG_PANEL_HEIGHT_720  613
#define MAP_INV_BIG_PANEL_HEIGHT_768  661
#define MAP_INV_BIG_PANEL_HEIGHT  (g_ui.isCompactStrategicScreen() ? MAP_INV_BIG_PANEL_HEIGHT_720 : MAP_INV_BIG_PANEL_HEIGHT_768)

// TRUE while that panel is showing and reaches into the bottom strip: the
// strip then leaves its covered part (message list, scroll bar/buttons,
// laptop shortcuts under the panel) alone, and the panel is drawn on top.
BOOLEAN MapInvBigPanelCoversBottomStrip(void);
void InitMapInvBigImagesForNewGame(void);
void SaveMapInvBigImagesToSaveGameFile(void);
void LoadMapInvBigImagesFromSaveGameFile(void);

void CopyPathToAllSelectedCharacters(PathSt* pPath);
void CancelPathsOfAllSelectedCharacters(void);

INT32 GetPathTravelTimeDuringPlotting(PathSt* pPath);

void AbortMovementPlottingMode( void );

BOOLEAN CanChangeSleepStatusForSoldier(const SOLDIERTYPE* s);

bool MapCharacterHasAccessibleInventory(SOLDIERTYPE const&);

ST::string GetMapscreenMercAssignmentString(SOLDIERTYPE const& s);
ST::string GetMapscreenMercLocationString(SOLDIERTYPE const& s);
ST::string GetMapscreenMercDestinationString(SOLDIERTYPE const& s);
ST::string GetMapscreenMercDepartureString(SOLDIERTYPE const& s, UINT8* text_colour);

// mapscreen wrapper to init the item description box
void MAPInternalInitItemDescriptionBox(OBJECTTYPE* pObject, UINT8 ubStatusIndex, SOLDIERTYPE* pSoldier);

// rebuild contract box this character
void RebuildContractBoxForMerc(const SOLDIERTYPE* s);

void    InternalMAPBeginItemPointer(SOLDIERTYPE* pSoldier);
BOOLEAN ContinueDialogue(SOLDIERTYPE* pSoldier, BOOLEAN fDone);
void    EndConfirmMapMoveMode(void);
BOOLEAN CanDrawSectorCursor(void);
void    RememberPreviousPathForAllSelectedChars(void);
void    MapScreenDefaultOkBoxCallback(MessageBoxReturnValue);
void    SetUpCursorForStrategicMap(void);
void    DrawFace(void);
void DrawStringRight(const ST::string& str, UINT16 x, UINT16 y, UINT16 w, UINT16 h, SGPFont font);

extern GUIButtonRef giMapInvDoneButton;
extern BOOLEAN      fInMapMode;
extern BOOLEAN      fReDrawFace;
extern BOOLEAN      fShowInventoryFlag;
extern BOOLEAN      fShowDescriptionFlag;
extern GUIButtonRef giMapContractButton;
extern GUIButtonRef giCharInfoButton[2];
extern BOOLEAN      fDrawCharacterList;
extern SGPSector    gsHighlightSector;

// create/destroy inventory button as needed
void CreateDestroyMapInvButton(void);

// On the wide strategic screen (UILayout::isWideStrategicScreen()) returns
// `wide` if that file exists, otherwise -- and always on the legacy 1024
// canvas -- `legacy`, so the game keeps working before the _wide assets are
// delivered. Both must be string literals (cache_key_t).
char const* GetWideStrategicAsset(char const* wide, char const* legacy);

void     MapScreenInit(void);
ScreenID MapScreenHandle(void);
void     MapScreenShutdown(void);

void LockMapScreenInterface(bool lock);
void MakeDialogueEventEnterMapScreen();

void SetMapCursorItem();

#define NAME_X                (MAP_SCREEN_X + 11)
#define NAME_WIDTH            (MAP_SCREEN_X + 62 - NAME_X)
#define ASSIGN_X              (MAP_SCREEN_X + 67)
#define ASSIGN_WIDTH          (MAP_SCREEN_X + 118 - ASSIGN_X)
#define SLEEP_X               (MAP_SCREEN_X + 123)
#define SLEEP_WIDTH           (MAP_SCREEN_X + 142 - SLEEP_X)
#define LOC_X                 (MAP_SCREEN_X + 147)
#define LOC_WIDTH             (MAP_SCREEN_X + 179 - LOC_X)
#define DEST_ETA_X            (MAP_SCREEN_X + 184)
#define DEST_ETA_WIDTH        (MAP_SCREEN_X + 217 - DEST_ETA_X)
#define TIME_REMAINING_X      (MAP_SCREEN_X + 222)
#define TIME_REMAINING_WIDTH  (MAP_SCREEN_X + 250 - TIME_REMAINING_X)
// Bottom-anchored per user request: 480-298=182, same distance from the old
// 640x480 canvas' bottom edge as before. Used only by DisplayGroundEta().
#define CLOCK_Y_START         (MAP_SCREEN_BOTTOM - 182)
// X shifted +136 per user request. CLOCK_HOUR_X_START/CLOCK_MIN_X_START
// (the value columns) additionally shifted +15 to add a margin between the
// "ETA:" label and the values -- the new, wider FONTMAP made the label wide
// enough to overlap them.
#define CLOCK_ETA_X           (MAP_SCREEN_RIGHT_BLOCK_X + 463 - 15 + 6 + 30 + 136)
#define CLOCK_HOUR_X_START    (MAP_SCREEN_RIGHT_BLOCK_X + 463 + 25 + 30 + 136 + 15)
#define CLOCK_MIN_X_START     (MAP_SCREEN_RIGHT_BLOCK_X + 463 + 45 + 30 + 136 + 15)

// contract
#define CONTRACT_X            (MAP_SCREEN_X + 185)
#define CONTRACT_Y            (MAP_SCREEN_Y + 50)

// Merc inventory panel (mapinv.sti, drawn at MAP_SCREEN_X, MAP_SCREEN_Y +
// 107 -- PLAYER_INFO_X/Y in MapScreen.cc): Done button and the money
// (deposit/withdraw), keyring and trash-can icons next to it.
//
// Every position has a normal-mode value (..._NORMAL: mapinv.sti /
// mapinv_1280.sti) and an independent "Show Large Icons"-mode value
// (..._BIG: mapinv_big_1280_720/768.sti, see IsMapInvBigImages() in MapScreen.cc);
// the plain name (bottom block) picks the one for the current mode.

// Normal mode. Icons are MAP_INV_ICON_SIZE square, drawn from
// inventory_bottom_panel_bookmarks.sti with the same sub-images as the
// tactical panel, in one row: left to right money, keyring, trash can (the
// tactical order), the trash can ending MAP_INV_ICON_GAP px left of the
// Done button and each icon MAP_INV_ICON_GAP px from the next; top edges
// aligned with the Done button.
#define MAP_INV_ICON_SIZE             32
#define MAP_INV_ICON_GAP              3
#define MAP_INV_DONE_BTN_X_NORMAL     (MAP_SCREEN_X + 221)
#define MAP_INV_DONE_BTN_Y_NORMAL     (MAP_SCREEN_Y + 107 + 453)
#define TRASH_CAN_X_NORMAL            (MAP_INV_DONE_BTN_X_NORMAL - MAP_INV_ICON_GAP - MAP_INV_ICON_SIZE)
#define TRASH_CAN_Y_NORMAL            (MAP_INV_DONE_BTN_Y_NORMAL)
#define MAP_INV_KEYRING_X_NORMAL      (TRASH_CAN_X_NORMAL - MAP_INV_ICON_GAP - MAP_INV_ICON_SIZE)
#define MAP_INV_KEYRING_Y_NORMAL      (MAP_INV_DONE_BTN_Y_NORMAL)
#define MAP_INV_MONEY_X_NORMAL        (MAP_INV_KEYRING_X_NORMAL - MAP_INV_ICON_GAP - MAP_INV_ICON_SIZE)
#define MAP_INV_MONEY_Y_NORMAL        (MAP_INV_DONE_BTN_Y_NORMAL)

// "Show Large Icons" mode -- independent of the normal mode; starts equal
// to it (X/Y relative to MAP_SCREEN_X/MAP_SCREEN_Y) -- tune to
// mapinv_big_1280_720/768.sti.
#define TRASH_CAN_X_BIG               (MAP_SCREEN_X + 245)
#define TRASH_CAN_Y_BIG               (MAP_SCREEN_Y + 107 + 26)
#define MAP_INV_KEYRING_X_BIG         (MAP_SCREEN_X + 209)
#define MAP_INV_KEYRING_Y_BIG         (MAP_SCREEN_Y + 107 + 26)
#define MAP_INV_MONEY_X_BIG           (MAP_SCREEN_X + 173)
#define MAP_INV_MONEY_Y_BIG           (MAP_SCREEN_Y + 107 + 26)

// Done button on both "Show Large Icons" panels (mapinv_big_1280_720.sti and
// mapinv_big_1280_768.sti -- one set): its own graphic (mapinv_done_buttons.sti
// sub-images 2 = ready, 3 = pressed, MAP_INV_DONE_BTN_W/H_BIG) and position.
#define MAP_INV_DONE_BTN_X_BIG        (MAP_SCREEN_X + 438)
#define MAP_INV_DONE_BTN_Y_BIG        (MAP_SCREEN_Y + 107 + 90)
#define MAP_INV_DONE_BTN_W_BIG        50
#define MAP_INV_DONE_BTN_H_BIG        50

// Current mode -- what the code uses.
#define MAP_INV_DONE_BTN_X    (IsMapInvBigImages() ? MAP_INV_DONE_BTN_X_BIG : MAP_INV_DONE_BTN_X_NORMAL)
#define MAP_INV_DONE_BTN_Y    (IsMapInvBigImages() ? MAP_INV_DONE_BTN_Y_BIG : MAP_INV_DONE_BTN_Y_NORMAL)

// trash can
#define TRASH_CAN_X           (IsMapInvBigImages() ? TRASH_CAN_X_BIG : TRASH_CAN_X_NORMAL)
#define TRASH_CAN_Y           (IsMapInvBigImages() ? TRASH_CAN_Y_BIG : TRASH_CAN_Y_NORMAL)
#define TRASH_CAN_WIDTH       (MAP_INV_ICON_SIZE)
#define TRASH_CAN_HEIGHT      (MAP_INV_ICON_SIZE)

// keyring
#define MAP_INV_KEYRING_X     (IsMapInvBigImages() ? MAP_INV_KEYRING_X_BIG : MAP_INV_KEYRING_X_NORMAL)
#define MAP_INV_KEYRING_Y     (IsMapInvBigImages() ? MAP_INV_KEYRING_Y_BIG : MAP_INV_KEYRING_Y_NORMAL)

// money (deposit to / withdraw from the player's account)
#define MAP_INV_MONEY_X       (IsMapInvBigImages() ? MAP_INV_MONEY_X_BIG : MAP_INV_MONEY_X_NORMAL)
#define MAP_INV_MONEY_Y       (IsMapInvBigImages() ? MAP_INV_MONEY_Y_BIG : MAP_INV_MONEY_Y_NORMAL)
#define MAP_INV_MONEY_WIDTH   (MAP_INV_ICON_SIZE)
#define MAP_INV_MONEY_HEIGHT  (MAP_INV_ICON_SIZE)

// Keyring popup on the map -- independent of the tactical one. Its origin
// is the same place the map's item description box (ItemInfoC.sti) opens
// at: the left column's own origin, MAP_SCREEN_X, MAP_SCREEN_Y + 107
// (MAP_ITEMDESC_START_X/Y in MapScreen.cc) -- not the legacy 640x480
// STD_SCREEN_X/Y window. WIDTH/HEIGHT is the area the popup takes over
// (shaded, mouse restricted to it, a click outside the key boxes closes
// it). BOX_OFFSET_X/Y is where the first extra_inventory.sti key box sits
// relative to the popup's origin; the boxes then run MAP_KEY_RING_ROW_WIDTH
// per row (Interface_Items.cc).
#define MAP_KEYRING_POPUP_X             (MAP_SCREEN_X + 0)
#define MAP_KEYRING_POPUP_Y             (MAP_SCREEN_Y + 107)
#define MAP_KEYRING_POPUP_WIDTH         261
#define MAP_KEYRING_POPUP_HEIGHT        (359 - 107)
#define MAP_KEYRING_POPUP_BOX_OFFSET_X  40
#define MAP_KEYRING_POPUP_BOX_OFFSET_Y  15

#endif
