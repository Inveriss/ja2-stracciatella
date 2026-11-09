#ifndef ITEMS_H
#define ITEMS_H

#include "Item_Types.h"
#include "JA2Types.h"

#include <vector>

struct CalibreModel;
struct MagazineModel;
struct WeaponModel;

void DamageObj(OBJECTTYPE* pObj, INT8 bAmount);

extern UINT8 SlotToPocket[7];

BOOLEAN WeaponInHand(const SOLDIERTYPE* pSoldier);

INT8 FindObj(const SOLDIERTYPE* pSoldier, UINT16 usItem);
INT8 FindAmmo(const SOLDIERTYPE*, const CalibreModel *, UINT8 ubMagSize, INT8 bExcludeSlot);

INT8 FindAttachment(const OBJECTTYPE* pObj, UINT16 usItem);
INT8 FindObjClass(const SOLDIERTYPE* s, UINT32 usItemClass);
extern INT8 FindAIUsableObjClass( const SOLDIERTYPE * pSoldier, 	UINT32 usItemClass );
extern INT8 FindAIUsableObjClassWithin( const SOLDIERTYPE * pSoldier, 	UINT32 usItemClass, INT8 bLower, INT8 bUpper );
extern INT8 FindEmptySlotWithin( const SOLDIERTYPE * pSoldier, INT8 bLower, INT8 bUpper );
extern INT8 FindExactObj( const SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj );
INT8 FindObjInObjRange(const SOLDIERTYPE* s, UINT16 usItem1, UINT16 usItem2);
extern INT8 FindLaunchable( const SOLDIERTYPE * pSoldier, UINT16 usWeapon );
extern INT8 FindThrowableGrenade( const SOLDIERTYPE * pSoldier );
extern INT8 FindUsableObj( const SOLDIERTYPE * pSoldier, UINT16 usItem );

extern void DeleteObj(OBJECTTYPE * pObj );
extern void SwapObjs( OBJECTTYPE * pObj1, OBJECTTYPE * pObj2 );

extern void RemoveObjFrom( OBJECTTYPE * pObj, UINT8 ubRemoveIndex );
//Returns true if swapped, false if added to end of stack
extern BOOLEAN PlaceObjectAtObjectIndex( OBJECTTYPE * pSourceObj, OBJECTTYPE * pTargetObj, UINT8 ubIndex );
extern void GetObjFrom( OBJECTTYPE * pObj, UINT8 ubGetIndex, OBJECTTYPE * pDest );

// bRequestedAttachPos: specific attachment slot to use (e.g. the one the player
// dropped the item on), or -1 (NO_SLOT) to auto-pick a slot as before.
bool AttachObject(SOLDIERTYPE* const s, OBJECTTYPE* const pTargetObj, OBJECTTYPE* const pAttachment, UINT8 const ubIndexInStack = 0, INT8 const bRequestedAttachPos = -1);
extern BOOLEAN RemoveAttachment( OBJECTTYPE * pObj, INT8 bAttachPos, OBJECTTYPE * pNewObj );

// Returns (in percent) how much of his carrying capacity this soldier uses.
UINT32 CalculateCarriedWeight(const SOLDIERTYPE* pSoldier);

extern UINT16 TotalPoints(const OBJECTTYPE*);
UINT16 UseKitPoints(OBJECTTYPE&, UINT16 points, SOLDIERTYPE const&);

extern BOOLEAN EmptyWeaponMagazine( OBJECTTYPE * pWeapon, OBJECTTYPE *pAmmo );
void CreateItem(UINT16 usItem, INT8 bStatus, OBJECTTYPE*);
void CreateItems(UINT16 usItem, INT8 bStatus, UINT8 ubNumber, OBJECTTYPE*);
void CreateMoney(UINT32 uiMoney, OBJECTTYPE*);
UINT16 DefaultMagazine(UINT16 gun);
UINT16 RandomMagazine( UINT16 usItem, UINT8 ubPercentStandard );
extern BOOLEAN ReloadGun( SOLDIERTYPE * pSoldier, OBJECTTYPE * pGun, OBJECTTYPE * pAmmo );

UINT8 ItemSlotLimit( UINT16 usItem, INT8 bSlot );

// Applies an item's optional ubBigPerPocket override (if set) as a further
// cap on an already-computed ItemSlotLimit() result, but ONLY for a
// soldier's own real BIGPOCK1-10POS slot. Deliberately a separate step
// rather than folded into ItemSlotLimit() itself -- the sector-inventory
// stash (Map_Screen_Interface_Map_Inventory.cc) calls ItemSlotLimit() with
// BIGPOCK1POS as a dummy slot to get the plain (unhalved) big-pocket
// formula, and must keep ignoring this override per user request; only
// call sites that place into/inspect a soldier's actual inventory slot
// (PlaceObject(), InitItemStackPopup()) should use this.
UINT8 ApplyBigPerPocketOverride( UINT16 usItem, INT8 bSlot, UINT8 ubSlotLimit );

// How many of this item fit this soldier's own slot: ItemSlotLimit() with the
// big pocket override, capped at MAX_OBJECTS_PER_SLOT; a vehicle's cargo
// pockets take its vehicles.json slotLimitMultiplier times that -- not for
// weapons or brass knuckles, nor for items that go one per slot. Guns in a
// vehicle's cargo pockets: its gunStackBigPocket / gunStackSmallPocket, never
// more than GunStackSize() (docs/gun-stacking-plan.md, G9); a merc keeps one
// gun per slot (G7). Money has its own MoneySlotLimit().
UINT8 SoldierSlotLimit(SOLDIERTYPE const&, UINT16 usItem, INT8 bSlot);

// docs/gun-stacking-plan.md: how many of this gun share one stack outside a
// merc's own inventory -- weapons.json stackSize, else game.json
// gun_stack_size, at most MAX_GUNS_PER_STACK (G3/G5). 1 for other items.
UINT8 GunStackSize(UINT16 usItem);

// How many of this item fit one sector inventory slot: GunStackSize() for
// guns (G8), else the item's big pocket limit (ItemSlotLimit() with
// BIGPOCK1POS); at most MAX_OBJECTS_PER_SLOT. 0 = one, it's a big item.
UINT8 SectorSlotLimit(UINT16 usItem);

// docs/enemy-drop-progress-plan.md: may soldier s leave object o behind?
// For ENEMY_TEAM with game.json enemy_drop_progress_limits on, an item whose
// minDropProgress is above the highest game progress must not drop (false);
// attachments above it are removed from o and lost (D4). A "blocked" item
// (D13) never drops from ENEMY_TEAM, also with the limits off. True for every
// other team and for an object the soldier picked up (OBJECT_PICKED_UP, D9).
bool ApplyEnemyDropProgressLimit(SOLDIERTYPE const& s, OBJECTTYPE& o);
// the item part of the above with a given progress
bool ApplyDropProgressLimit(OBJECTTYPE& o, UINT8 progress);
// true when the item is "blocked" (D13), or game.json enemy_drop_progress_limits
// is on and the item's minDropProgress is above the game progress (an enemy can't drop it)
bool EnemyDropBlockedByProgress(UINT16 usItem);
// ApplyEnemyDropProgressLimit() for an item whose owner may be unknown (null:
// no limit), e.g. the thrower of a knife
bool ApplyEnemyDropProgressLimit(SOLDIERTYPE const* s, OBJECTTYPE& o);
// D9: soldier s picks o up from the world -- a non-player soldier marks it
// OBJECT_PICKED_UP (it may be the player's own), a merc clears the mark
void MarkObjectPickedUpBy(SOLDIERTYPE const& s, OBJECTTYPE& o);

// Function to put an item in a soldier profile
// It's very primitive, just finds an empty place!
BOOLEAN PlaceObjectInSoldierProfile( UINT8 ubProfile, OBJECTTYPE *pObject );
BOOLEAN RemoveObjectFromSoldierProfile( UINT8 ubProfile, UINT16 usItem );
INT8 FindObjectInSoldierProfile(MERCPROFILESTRUCT const&, UINT16 item_id);

void SetMoneyInSoldierProfile( UINT8 ubProfile, UINT32 uiMoney );

void CheckEquipmentForDamage( SOLDIERTYPE *pSoldier, INT32 iDamage );
BOOLEAN ArmBomb( OBJECTTYPE * pObj, INT8 bSetting );

// NOTE TO ANDREW:
//
// The following functions expect that pObj points to the object
// "in the cursor", which should have memory allocated for it already
BOOLEAN PlaceObject( SOLDIERTYPE * pSoldier, INT8 bPos, OBJECTTYPE * pObj );

// Send fNewItem to true to set off new item glow in inv panel
BOOLEAN AutoPlaceObject( SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj, BOOLEAN fNewItem );
BOOLEAN RemoveObjectFromSlot( SOLDIERTYPE * pSoldier, INT8 bPos, OBJECTTYPE * pObj );

// Swap keys in keyring slot and keys in pocket
void SwapKeysToSlot(SOLDIERTYPE&, INT8 key_ring_pos, OBJECTTYPE&);

// create a keyobject
void CreateKeyObject(OBJECTTYPE*, UINT8 ubNumberOfKeys, UINT8 ubKeyIdValue);
BOOLEAN DeleteKeyObject( OBJECTTYPE * pObj );
void    AllocateObject(OBJECTTYPE** pObj);

// remove one or more keys from a *KEYRING* slot
BOOLEAN RemoveKeysFromSlot( SOLDIERTYPE * pSoldier, INT8 bKeyRingPosition, UINT8 ubNumberOfKeys ,OBJECTTYPE * pObj );

// add the keys to an inventory slot
UINT8 AddKeysToSlot(SOLDIERTYPE&, INT8 key_ring_pos, OBJECTTYPE const& key);



//Kris:  December 9, 1997
//I need a bunch of validation functions for ammo, attachments, etc., so I'll be adding them here.
//Chris, maybe you might find these useful, or add your own.  I don't really know what I'm doing yet,
//so feel free to correct me...

//Simple check to see if the item has any attachments
bool ItemHasAttachments(OBJECTTYPE const&);

// Whether two OBJECTTYPEs of the same usItem may share one multi-unit
// "stack" (ubNumberOfObjects > 1). Always true for non-guns. For guns, the
// union's ammo fields and usAttachItem[]/bAttachStatus[] are a single shared
// value for the whole OBJECTTYPE, not one per unit, so two guns may only
// merge when equal in all of it: same ammo type, shots left, magazine item,
// jam status and attachments. Only the condition is kept per unit
// (docs/gun-stacking-plan.md, G2), see UnitStatus().
bool CanGunsStack(OBJECTTYPE const& a, OBJECTTYPE const& b);

// At most this many guns share one OBJECTTYPE: unit 0's condition is
// bGunStatus, units 1.. use the bGunUnitStatus[] bytes after the gun fields
// (docs/gun-stacking-plan.md, G1/G5).
constexpr UINT8 MAX_GUNS_PER_STACK = MAX_OBJECTS_PER_SLOT - 6;

// The status (condition) of unit i of a stack: bStatus[i] for most items,
// bGunStatus / bGunUnitStatus[i - 1] for guns. Not for ammo (ubShotsLeft[]),
// money or keys, which keep their own fields.
INT8 UnitStatus(OBJECTTYPE const& o, UINT8 i);
void SetUnitStatus(OBJECTTYPE& o, UINT8 i, INT8 status);

//Determine if this item can receive this attachment.  This is different, in that it may
//be possible to have this attachment on this item, but may already have an attachment on
//it which doesn't work simultaneously with the new attachment (like a silencer and duckbill).
BOOLEAN ValidItemAttachment(const OBJECTTYPE* pObj, UINT16 usAttachment, BOOLEAN fAttemptingAttachment);
// docs/weapon-attachments-plan.md (W4): the attachment already on the object
// that keeps `attachment` off it -- one of the same family (*sameFamily) or of
// a family that excludes it -- or NOTHING
UINT16 FindConflictingAttachment(OBJECTTYPE const& o, UINT16 attachment, bool* sameFamily);

//Determines if it is possible to equip this weapon with this ammo.
bool ValidAmmoType( UINT16 usItem, UINT16 usAmmoType );

// Determine if it is possible to add this attachment to the item
bool ValidAttachment(UINT16 attachment, UINT16 item);

BOOLEAN ValidLaunchable( UINT16 usLaunchable, UINT16 usItem );
UINT16 GetLauncherFromLaunchable( UINT16 usLaunchable );

BOOLEAN ValidMerge( UINT16 usMerge, UINT16 usItem );


// Is the item passed a medical kit?
BOOLEAN IsMedicalKitItem(const OBJECTTYPE* pObject);

BOOLEAN AutoReload( SOLDIERTYPE * pSoldier );
INT8 FindAmmoToReload( const SOLDIERTYPE * pSoldier, INT8 bWeaponIn, INT8 bExcludeSlot );
// The ammo to reload a gun with, as FindAmmoToReload(); a passenger who
// shoots from inside a vehicle and has none of his own takes it from the
// vehicle's cargo (passenger shooting, stage 5). NULL: none.
OBJECTTYPE* FindAmmoObjToReload(SOLDIERTYPE* pSoldier, INT8 bWeaponIn, INT8 bExcludeSlot);

void SwapHandItems( SOLDIERTYPE * pSoldier );

INT8 FindAttachmentByClass(OBJECTTYPE const*, UINT32 uiItemClass);
INT8 FindPlatesAttachment(OBJECTTYPE const*);
void RemoveObjs( OBJECTTYPE * pObj, UINT8 ubNumberToRemove );

void WaterDamage(SOLDIERTYPE&);

INT8 FindObjWithin( SOLDIERTYPE * pSoldier, UINT16 usItem, INT8 bLower, INT8 bUpper );

BOOLEAN ApplyCamo(SOLDIERTYPE* pSoldier, OBJECTTYPE* pObj, BOOLEAN* pfGoodAPs);

BOOLEAN ItemIsLegal( UINT16 usItemIndex );
UINT16 FindReplacementMagazine(const CalibreModel * calibre, UINT8 const mag_size, UINT8 const ammo_type);
// The magazine item a gun with the given magazine size holds after it was
// loaded with `loaded` (docs/magazine-size-variants-plan.md): the magazine of
// the same calibre and ammo type with that capacity, the lowest index among
// several; `loaded` itself when there is none. Item indices and their order
// do not matter.
UINT16 FindMagazineVariant(std::vector<const MagazineModel*> const& magazines, MagazineModel const& loaded, UINT16 gun_mag_size);
UINT16 FindMagazineVariant(MagazineModel const& loaded, UINT16 gun_mag_size);
UINT16 FindReplacementMagazineIfNecessary(const WeaponModel *old_gun, UINT16 const old_ammo_id, const WeaponModel *new_gun);

BOOLEAN DamageItemOnGround(OBJECTTYPE* pObject, INT16 sGridNo, INT8 bLevel, INT32 iDamage, SOLDIERTYPE* owner);

BOOLEAN ApplyCanteen( SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj, BOOLEAN *pfGoodAPs );
BOOLEAN ApplyElixir( SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj, BOOLEAN *pfGoodAPs );

BOOLEAN CompatibleFaceItem( UINT16 usItem1, UINT16 usItem2 );
UINT16  FirstIncompatibleHeadItem( const SOLDIERTYPE* pSoldier, INT8 bTargetSlot, UINT16 usNewItem );

UINT32 MoneySlotLimit( INT8 bSlot );

void CheckEquipmentForFragileItemDamage( SOLDIERTYPE *pSoldier, INT32 iDamage );

// Range of Xray device
#define XRAY_RANGE	40
// Seconds that Xray lasts
#define XRAY_TIME	5

extern void ActivateXRayDevice( SOLDIERTYPE * pSoldier );
extern void TurnOffXRayEffects( SOLDIERTYPE * pSoldier );
INT8 FindLaunchableAttachment(const OBJECTTYPE* pObj, UINT16 usWeapon);

// A magazine for the gun of the player's tank, which reloads it where it sits
bool IsTankGunReload(SOLDIERTYPE const&, OBJECTTYPE const& ammo, INT8 slot);
BOOLEAN CanItemFitInPosition(SOLDIERTYPE* s, OBJECTTYPE* pObj, INT8 bPos, BOOLEAN fDoingPlacement);

void SetNewItem(SOLDIERTYPE* pSoldier, UINT8 ubInvPos, BOOLEAN fNewItem);
void CleanUpStack(OBJECTTYPE* pObj, OBJECTTYPE* pCursorObj);
void StackObjs(OBJECTTYPE* pSourceObj, OBJECTTYPE* pTargetObj, UINT8 ubNumberToCopy);
void SortItemStackByStatus(OBJECTTYPE* pObj);
bool ItemIsCool(OBJECTTYPE const&);

bool HasObjectImprint(OBJECTTYPE const&);

#endif
