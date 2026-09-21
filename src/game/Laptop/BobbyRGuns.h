#ifndef __BOBBYRGUNS_H
#define __BOBBYRGUNS_H

#include "Types.h"

#define BOBBYRDESCFILE BINARYDATADIR "/braydesc.edt"

#define BOBBYR_ITEM_DESC_NAME_SIZE  80
#define BOBBYR_ITEM_DESC_INFO_SIZE 320
#define BOBBYR_ITEM_DESC_FILE_SIZE 400

#define BOBBYR_USED_ITEMS		0xFFFFFFFF

// Categories of the pages that are not a plain item class (see BobbyRItemMatchesClass()); they are
// used in the place of the class mask of SetFirstLastPagesForNew() and DisplayItemInfo().
// Attachments: the attachments of weapons (not armour, face items, weapons and explosives).
#define BOBBYR_ATTACHMENT_ITEMS		0xFFFFFFFE
// Miscellaneous: what the old Misc page had (IC_BOBBY_MISC) without the attachments and explosives.
#define BOBBYR_MISC_ITEMS		0xFFFFFFFD
// The classes of the guns page (the buttons at the bottom of it): pistols, machine pistols and
// submachine guns plus the melee weapons; the assault rifles; the sniper rifles and the plain rifles;
// the shotguns; the machine guns, the launchers and the rocket rifle.
#define BOBBYR_GUNS_PISTOL_SMG_ITEMS	0xFFFFFFFC
#define BOBBYR_GUNS_ASSAULT_ITEMS	0xFFFFFFFB
#define BOBBYR_GUNS_SNIPER_ITEMS	0xFFFFFFFA
#define BOBBYR_GUNS_SHOTGUN_ITEMS	0xFFFFFFF9
#define BOBBYR_GUNS_HEAVY_ITEMS		0xFFFFFFF8
// All the items of the guns page: the guns, the launchers and the melee weapons
#define BOBBYR_ALL_GUN_ITEMS		(IC_GUN | IC_LAUNCHER | IC_BLADE | IC_THROWING_KNIFE | IC_PUNCH)
// The classes of the attachments page (the buttons at the bottom of it): the front of the barrel
// (silencer, gun barrel extender, duckbill), the top (laser scope, sniper scope), the rear (rod and
// spring) and down (bipod).
#define BOBBYR_ATTACH_FRONT_ITEMS	0xFFFFFFF7
#define BOBBYR_ATTACH_TOP_ITEMS		0xFFFFFFF6
#define BOBBYR_ATTACH_REAR_ITEMS	0xFFFFFFF5
#define BOBBYR_ATTACH_DOWN_ITEMS	0xFFFFFFF4
// The classes of the ammo page (the buttons at the bottom of it): the magazines by the number of
// rounds: up to 15, 16-30, 31-50, 51-100 and 101-250.
#define BOBBYR_AMMO_UP_TO_15_ITEMS	0xFFFFFFF3
#define BOBBYR_AMMO_UP_TO_30_ITEMS	0xFFFFFFF2
#define BOBBYR_AMMO_UP_TO_50_ITEMS	0xFFFFFFF1
#define BOBBYR_AMMO_UP_TO_100_ITEMS	0xFFFFFFF0
#define BOBBYR_AMMO_UP_TO_250_ITEMS	0xFFFFFFEF
// Armor and headgear: the armour and everything that is worn on the head (the face items).
#define BOBBYR_ARMOUR_ITEMS		(IC_ARMOUR | IC_FACE)


#define BOBBYR_GUNS_BUTTON_FONT		FONT10ARIAL
#define BOBBYR_GUNS_TEXT_COLOR_ON	FONT_NEARBLACK
#define BOBBYR_GUNS_TEXT_COLOR_OFF	FONT_NEARBLACK
//#define BOBBYR_GUNS_TEXT_COLOR_ON	FONT_MCOLOR_DKWHITE
//#define BOBBYR_GUNS_TEXT_COLOR_OFF	FONT_MCOLOR_WHITE

#define BOBBYR_GUNS_SHADOW_COLOR	169

#define BOBBYR_NO_ITEMS			65535





extern UINT16 gusCurWeaponIndex;
extern UINT8  gubLastGunIndex;



void GameInitBobbyRGuns(void);
void EnterBobbyRGuns(void);
void ExitBobbyRGuns(void);
void RenderBobbyRGuns(void);


void DisplayBobbyRBrTitle(void);
void DeleteBobbyBrTitle(void);
void InitBobbyBrTitle(void);
void InitBobbyMenuBar();
void DeleteBobbyMenuBar();

//BOOLEAN DisplayWeaponInfo();
void DisplayItemInfo(UINT32 uiItemClass);
void DeleteMouseRegionForBigImage(void);
void UpdateButtonText(UINT32	uiCurPage);
UINT16 CalcBobbyRayCost( UINT16 usIndex, UINT16 usBobbyIndex, BOOLEAN fUsed);
void SetFirstLastPagesForUsed(void);
void SetFirstLastPagesForNew( UINT32 uiClass );

// Does the item belong to the page of the given class mask (an item class or one of the categories above)?
struct ItemModel;
bool BobbyRItemMatchesClass(const ItemModel* item, UINT32 uiClassMask);

// The class mask of the attachments page with its class button pressed (all attachments when none is)
UINT32 BobbyRAttachmentsPageMask(void);
// The class mask of the ammo page with its class button pressed (all the ammo when none is)
UINT32 BobbyRAmmoPageMask(void);

#endif
