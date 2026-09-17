#include "Debug.h"
#include "LoadSaveObjectType.h"
#include "LoadSaveData.h"

#include "ContentManager.h"
#include "GameInstance.h"
#include "ItemModel.h"
#include <algorithm>


static void ReplaceInvalidItem(UINT16 & usItem)
{
	auto * item{ GCM->getItem(usItem, ItemSystem::nothrow) };
	if (!item)
	{
		SLOGW("Item (index {}) is not defined and will be ignored. Maybe the file was saved for a different game version", usItem);
		usItem = NONE;
	}
}

// Every branch below writes/skips exactly as many bytes as the OBJECTTYPE
// union is wide (currently 100, dictated by bStatus[]/ubShotsLeft[] at
// MAX_OBJECTS_PER_SLOT -- see the static_assert on OBJECTTYPE in
// Item_Types.h), so the fields after the switch always start at the same
// offset regardless of which branch ran. The EXTR_SKIP/INJ_SKIP amount in
// each branch is "union width minus that branch's own real fields" -- it
// must be recomputed by hand whenever MAX_OBJECTS_PER_SLOT changes (the
// two array branches read/write their own width via lengthof() and need no
// trailing skip; every other branch's skip below is spelled out in full so
// the arithmetic is checkable at a glance).
void ExtractObject(DataReader& d, OBJECTTYPE* const o)
{
	size_t start = d.getConsumed();
	EXTR_U16(d, o->usItem)
	EXTR_U8(d, o->ubNumberOfObjects)
	EXTR_SKIP(d, 1)

	ReplaceInvalidItem(o->usItem);
	const ItemModel* item = GCM->getItem(o->usItem);
	switch (item->getItemClass())
	{
		case IC_AMMO:
			// lengthof(o->ubShotsLeft) == MAX_OBJECTS_PER_SLOT == the full
			// union width -- nothing left to skip.
			EXTR_U8A(d, o->ubShotsLeft, lengthof(o->ubShotsLeft))
			break;

		case IC_GUN:
			EXTR_I8(d, o->bGunStatus)
			EXTR_U8(d, o->ubGunAmmoType)
			EXTR_U8(d, o->ubGunShotsLeft)
			EXTR_SKIP(d, 1)
			EXTR_U16(d, o->usGunAmmoItem)
			EXTR_I8(d, o->bGunAmmoStatus)
			EXTR_SKIP(d, 93) // 7 real bytes so far; 93 = 100 - 7
			break;

		case IC_KEY:
			EXTR_I8A(d, o->bKeyStatus, lengthof(o->bKeyStatus))
			EXTR_U8(d, o->ubKeyID)
			EXTR_SKIP(d, 93) // 7 real bytes so far; 93 = 100 - 7
			break;

		case IC_MONEY:
			EXTR_I8(d, o->bMoneyStatus)
			EXTR_SKIP(d, 3)
			EXTR_U32(d, o->uiMoneyAmount)
			EXTR_SKIP(d, 92) // 8 real bytes so far; 92 = 100 - 8
			break;

		case IC_MISC:
			switch (o->usItem)
			{
				case ACTION_ITEM:
					EXTR_I8(d, o->bBombStatus)
					EXTR_I8(d, o->bDetonatorType)
					EXTR_U16(d, o->usBombItem)
					EXTR_I8(d, o->bFrequency) // XXX unclear when to use bDelay
					EXTR_U8(d, o->ubBombOwner)
					EXTR_U8(d, o->bActionValue)
					EXTR_U8(d, o->ubTolerance)
					EXTR_SKIP(d, 92) // 8 real bytes so far; 92 = 100 - 8
					break;

				case OWNERSHIP:
					EXTR_U8(d, o->ubOwnerProfile)
					EXTR_U8(d, o->ubOwnerCivGroup)
					EXTR_SKIP(d, 98) // 2 real bytes so far; 98 = 100 - 2
					break;

				case SWITCH:
					EXTR_I8(d, o->bBombStatus)
					EXTR_I8(d, o->bDetonatorType)
					EXTR_U16(d, o->usBombItem)
					EXTR_I8(d, o->bFrequency)
					EXTR_U8(d, o->ubBombOwner)
					EXTR_U8(d, o->bActionValue)
					EXTR_U8(d, o->ubTolerance)
					EXTR_SKIP(d, 92) // 8 real bytes so far; 92 = 100 - 8
					break;

				default: goto extract_status;
			}
			break;

		default:
extract_status:
			// lengthof(o->bStatus) == MAX_OBJECTS_PER_SLOT == the full union
			// width -- nothing left to skip.
			EXTR_I8A(d, o->bStatus, lengthof(o->bStatus))
			break;
	}
	EXTR_U16A(d, o->usAttachItem, lengthof(o->usAttachItem))
	EXTR_I8A(d, o->bAttachStatus, lengthof(o->bAttachStatus))
	EXTR_I8(d, o->fFlags)
	EXTR_U8(d, o->ubMission)
	EXTR_I8(d, o->bTrap)
	EXTR_U8(d, o->ubImprintID)
	EXTR_U8(d, o->ubWeight)
	EXTR_U8(d, o->fUsed)
	EXTR_SKIP(d, 2)
	Assert(d.getConsumed() == start + 172);

	// Check and remove invalid items in attachment slots
	for (UINT16 & i : o->usAttachItem)
	{
		ReplaceInvalidItem(i);
	}
}


// Number of attachment slots in the original (vanilla) OBJECTTYPE layout.
// Deliberately NOT MAX_ATTACHMENTS - this must stay fixed at 4 forever, since
// it describes a byte layout baked into the sector map files shipped with the
// base game, which we can't change.
static constexpr INT8 LEGACY_MAX_ATTACHMENTS = 4;

// Number of bStatus[]/ubShotsLeft[] entries in the original (vanilla)
// OBJECTTYPE layout. Deliberately NOT MAX_OBJECTS_PER_SLOT (raised past 8) -
// same reasoning as LEGACY_MAX_ATTACHMENTS above: this describes a byte
// layout baked into the sector map files shipped with the base game. Used
// instead of lengthof(o->bStatus)/lengthof(o->ubShotsLeft) in the Legacy
// functions below, which would otherwise silently follow MAX_OBJECTS_PER_SLOT
// and misparse the fixed-width legacy stream.
static constexpr INT8 LEGACY_MAX_STACK = 8;


void ExtractLegacyObject(DataReader& d, OBJECTTYPE* const o)
{
	size_t start = d.getConsumed();
	EXTR_U16(d, o->usItem)
	EXTR_U8(d, o->ubNumberOfObjects)
	EXTR_SKIP(d, 1)

	ReplaceInvalidItem(o->usItem);
	const ItemModel* item = GCM->getItem(o->usItem);
	switch (item->getItemClass())
	{
		case IC_AMMO:
			// The legacy format only ever had LEGACY_MAX_STACK entries on disk.
			EXTR_U8A(d, o->ubShotsLeft, LEGACY_MAX_STACK)
			EXTR_SKIP(d, 4)
			// Any slots beyond the legacy ones don't exist in this format - default to empty.
			for (INT8 i = LEGACY_MAX_STACK; i < MAX_OBJECTS_PER_SLOT; ++i) o->ubShotsLeft[i] = 0;
			break;

		case IC_GUN:
			EXTR_I8(d, o->bGunStatus)
			EXTR_U8(d, o->ubGunAmmoType)
			EXTR_U8(d, o->ubGunShotsLeft)
			EXTR_SKIP(d, 1)
			EXTR_U16(d, o->usGunAmmoItem)
			EXTR_I8(d, o->bGunAmmoStatus)
			EXTR_SKIP(d, 5)
			break;

		case IC_KEY:
			EXTR_I8A(d, o->bKeyStatus, lengthof(o->bKeyStatus))
			EXTR_U8(d, o->ubKeyID)
			EXTR_SKIP(d, 5)
			break;

		case IC_MONEY:
			EXTR_I8(d, o->bMoneyStatus)
			EXTR_SKIP(d, 3)
			EXTR_U32(d, o->uiMoneyAmount)
			EXTR_SKIP(d, 4)
			break;

		case IC_MISC:
			switch (o->usItem)
			{
				case ACTION_ITEM:
					EXTR_I8(d, o->bBombStatus)
					EXTR_I8(d, o->bDetonatorType)
					EXTR_U16(d, o->usBombItem)
					EXTR_I8(d, o->bFrequency) // XXX unclear when to use bDelay
					EXTR_U8(d, o->ubBombOwner)
					EXTR_U8(d, o->bActionValue)
					EXTR_U8(d, o->ubTolerance)
					EXTR_SKIP(d, 4)
					break;

				case OWNERSHIP:
					EXTR_U8(d, o->ubOwnerProfile)
					EXTR_U8(d, o->ubOwnerCivGroup)
					EXTR_SKIP(d, 10)
					break;

				case SWITCH:
					EXTR_I8(d, o->bBombStatus)
					EXTR_I8(d, o->bDetonatorType)
					EXTR_U16(d, o->usBombItem)
					EXTR_I8(d, o->bFrequency)
					EXTR_U8(d, o->ubBombOwner)
					EXTR_U8(d, o->bActionValue)
					EXTR_U8(d, o->ubTolerance)
					EXTR_SKIP(d, 4)
					break;

				default: goto extract_legacy_status;
			}
			break;

		default:
extract_legacy_status:
			// The legacy format only ever had LEGACY_MAX_STACK entries on disk --
			// deliberately not lengthof(o->bStatus), which would silently follow
			// MAX_OBJECTS_PER_SLOT and misparse this fixed-width legacy stream.
			EXTR_I8A(d, o->bStatus, LEGACY_MAX_STACK)
			EXTR_SKIP(d, 4)
			// Any slots beyond the legacy ones don't exist in this format - default to empty.
			for (INT8 i = LEGACY_MAX_STACK; i < MAX_OBJECTS_PER_SLOT; ++i) o->bStatus[i] = 0;
			break;
	}

	// The legacy format only ever had LEGACY_MAX_ATTACHMENTS slots on disk.
	EXTR_U16A(d, o->usAttachItem, LEGACY_MAX_ATTACHMENTS)
	EXTR_I8A(d, o->bAttachStatus, LEGACY_MAX_ATTACHMENTS)
	// Any slots beyond the legacy ones don't exist in this format - default to empty.
	for (INT8 i = LEGACY_MAX_ATTACHMENTS; i < MAX_ATTACHMENTS; ++i)
	{
		o->usAttachItem[i]  = NOTHING;
		o->bAttachStatus[i] = 0;
	}
	EXTR_I8(d, o->fFlags)
	EXTR_U8(d, o->ubMission)
	EXTR_I8(d, o->bTrap)
	EXTR_U8(d, o->ubImprintID)
	EXTR_U8(d, o->ubWeight)
	EXTR_U8(d, o->fUsed)
	EXTR_SKIP(d, 2)
	Assert(d.getConsumed() == start + 36);

	// Check and remove invalid items in attachment slots
	for (UINT16 & i : o->usAttachItem)
	{
		ReplaceInvalidItem(i);
	}
}


// See the comment on ExtractObject() above -- same union-width bookkeeping,
// mirrored for writing.
void InjectObject(DataWriter& d, const OBJECTTYPE* o)
{
	size_t start = d.getConsumed();
	INJ_U16(d, o->usItem)
	INJ_U8(d, o->ubNumberOfObjects)
	INJ_SKIP(d, 1)
	switch (GCM->getItem(o->usItem)->getItemClass())
	{
		case IC_AMMO:
			// lengthof(o->ubShotsLeft) == MAX_OBJECTS_PER_SLOT == the full
			// union width -- nothing left to skip.
			INJ_U8A(d, o->ubShotsLeft, lengthof(o->ubShotsLeft))
			break;

		case IC_GUN:
			INJ_I8(d, o->bGunStatus)
			INJ_U8(d, o->ubGunAmmoType)
			INJ_U8(d, o->ubGunShotsLeft)
			INJ_SKIP(d, 1)
			INJ_U16(d, o->usGunAmmoItem)
			INJ_I8(d, o->bGunAmmoStatus)
			INJ_SKIP(d, 93) // 7 real bytes so far; 93 = 100 - 7
			break;

		case IC_KEY:
			INJ_I8A(d, o->bKeyStatus, lengthof(o->bKeyStatus))
			INJ_U8(d, o->ubKeyID)
			INJ_SKIP(d, 93) // 7 real bytes so far; 93 = 100 - 7
			break;

		case IC_MONEY:
			INJ_I8(d, o->bMoneyStatus)
			INJ_SKIP(d, 3)
			INJ_U32(d, o->uiMoneyAmount)
			INJ_SKIP(d, 92) // 8 real bytes so far; 92 = 100 - 8
			break;

		case IC_MISC:
			switch (o->usItem)
			{
				case ACTION_ITEM:
					INJ_I8(d, o->bBombStatus)
					INJ_I8(d, o->bDetonatorType)
					INJ_U16(d, o->usBombItem)
					INJ_I8(d, o->bFrequency) // XXX unclear when to use bDelay
					INJ_U8(d, o->ubBombOwner)
					INJ_U8(d, o->bActionValue)
					INJ_U8(d, o->ubTolerance)
					INJ_SKIP(d, 92) // 8 real bytes so far; 92 = 100 - 8
					break;

				case OWNERSHIP:
					INJ_U8(d, o->ubOwnerProfile)
					INJ_U8(d, o->ubOwnerCivGroup)
					INJ_SKIP(d, 98) // 2 real bytes so far; 98 = 100 - 2
					break;

				case SWITCH:
					INJ_I8(d, o->bBombStatus)
					INJ_I8(d, o->bDetonatorType)
					INJ_U16(d, o->usBombItem)
					INJ_I8(d, o->bFrequency)
					INJ_U8(d, o->ubBombOwner)
					INJ_U8(d, o->bActionValue)
					INJ_U8(d, o->ubTolerance)
					INJ_SKIP(d, 92) // 8 real bytes so far; 92 = 100 - 8
					break;

				default: goto inject_status;
			}
			break;

		default:
inject_status:
			// lengthof(o->bStatus) == MAX_OBJECTS_PER_SLOT == the full union
			// width -- nothing left to skip.
			INJ_I8A(d, o->bStatus, lengthof(o->bStatus))
			break;
	}
	INJ_U16A(d, o->usAttachItem, lengthof(o->usAttachItem))
	INJ_I8A(d, o->bAttachStatus, lengthof(o->bAttachStatus))
	INJ_I8(d, o->fFlags)
	INJ_U8(d, o->ubMission)
	INJ_I8(d, o->bTrap)
	INJ_U8(d, o->ubImprintID)
	d.writeU8(static_cast<UINT8>(std::clamp(Weight(*o), 1, 255)));
	INJ_U8(d, o->fUsed)
	INJ_SKIP(d, 2)
	Assert(d.getConsumed() == start + 172);
}


// Writes the original (vanilla) 36-byte, 4-attachment-slot OBJECTTYPE layout.
// Frozen forever - see ExtractLegacyObject(). Any attachment beyond the legacy
// 4 slots is silently dropped, since this format has nowhere to put it; this
// only matters for items placed on a map through the in-game editor.
void InjectLegacyObject(DataWriter& d, const OBJECTTYPE* o)
{
	size_t start = d.getConsumed();
	INJ_U16(d, o->usItem)
	INJ_U8(d, o->ubNumberOfObjects)
	INJ_SKIP(d, 1)
	switch (GCM->getItem(o->usItem)->getItemClass())
	{
		case IC_AMMO:
			// The legacy format only ever had room for LEGACY_MAX_STACK entries --
			// same silent truncation as the attachments below (see the function
			// comment) for any stack past that, which only matters for items
			// placed on a map through the in-game editor.
			INJ_U8A(d, o->ubShotsLeft, LEGACY_MAX_STACK)
			INJ_SKIP(d, 4)
			break;

		case IC_GUN:
			INJ_I8(d, o->bGunStatus)
			INJ_U8(d, o->ubGunAmmoType)
			INJ_U8(d, o->ubGunShotsLeft)
			INJ_SKIP(d, 1)
			INJ_U16(d, o->usGunAmmoItem)
			INJ_I8(d, o->bGunAmmoStatus)
			INJ_SKIP(d, 5)
			break;

		case IC_KEY:
			INJ_I8A(d, o->bKeyStatus, lengthof(o->bKeyStatus))
			INJ_U8(d, o->ubKeyID)
			INJ_SKIP(d, 5)
			break;

		case IC_MONEY:
			INJ_I8(d, o->bMoneyStatus)
			INJ_SKIP(d, 3)
			INJ_U32(d, o->uiMoneyAmount)
			INJ_SKIP(d, 4)
			break;

		case IC_MISC:
			switch (o->usItem)
			{
				case ACTION_ITEM:
					INJ_I8(d, o->bBombStatus)
					INJ_I8(d, o->bDetonatorType)
					INJ_U16(d, o->usBombItem)
					INJ_I8(d, o->bFrequency) // XXX unclear when to use bDelay
					INJ_U8(d, o->ubBombOwner)
					INJ_U8(d, o->bActionValue)
					INJ_U8(d, o->ubTolerance)
					INJ_SKIP(d, 4)
					break;

				case OWNERSHIP:
					INJ_U8(d, o->ubOwnerProfile)
					INJ_U8(d, o->ubOwnerCivGroup)
					INJ_SKIP(d, 10)
					break;

				case SWITCH:
					INJ_I8(d, o->bBombStatus)
					INJ_I8(d, o->bDetonatorType)
					INJ_U16(d, o->usBombItem)
					INJ_I8(d, o->bFrequency)
					INJ_U8(d, o->ubBombOwner)
					INJ_U8(d, o->bActionValue)
					INJ_U8(d, o->ubTolerance)
					INJ_SKIP(d, 4)
					break;

				default: goto inject_legacy_status;
			}
			break;

		default:
inject_legacy_status:
			// The legacy format only ever had room for LEGACY_MAX_STACK entries --
			// deliberately not lengthof(o->bStatus), which would silently follow
			// MAX_OBJECTS_PER_SLOT and write a stream this format can't hold.
			// Same silent truncation as the attachments below for any stack past
			// that, which only matters for items placed via the in-game editor.
			INJ_I8A(d, o->bStatus, LEGACY_MAX_STACK)
			INJ_SKIP(d, 4)
			break;
	}
	// The legacy format only ever had LEGACY_MAX_ATTACHMENTS slots on disk.
	INJ_U16A(d, o->usAttachItem, LEGACY_MAX_ATTACHMENTS)
	INJ_I8A(d, o->bAttachStatus, LEGACY_MAX_ATTACHMENTS)
	INJ_I8(d, o->fFlags)
	INJ_U8(d, o->ubMission)
	INJ_I8(d, o->bTrap)
	INJ_U8(d, o->ubImprintID)
	d.writeU8(static_cast<UINT8>(std::clamp(Weight(*o), 1, 255)));
	INJ_U8(d, o->fUsed)
	INJ_SKIP(d, 2)
	Assert(d.getConsumed() == start + 36);
}
