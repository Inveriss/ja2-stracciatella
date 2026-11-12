#include <stdexcept>

#include "Types.h"
#include "ArmsDealerInvInit.h"
#include "Item_Types.h"
#include "Arms_Dealer_Init.h"
#include "Debug.h"
#include "Weapons.h"
#include "Campaign.h"
#include "BobbyR.h"
#include "LaptopSave.h"
#include "ShopKeeper_Interface.h"
#include "Random.h"
#include "Items.h"

#include "CalibreModel.h"
#include "ContentManager.h"
#include "DealerInventory.h"
#include "GameInstance.h"
#include "policy/GamePolicy.h"
#include "DealerModel.h"
#include "Quests.h"
#include "MagazineModel.h"
#include "WeaponModels.h"

// Item sort categories, in the order they are listed: the same order at
// Bobby Ray's, at the dealers and in the sector inventory.
// - Guns are split by their exact type (internalType in weapons.json).
// - Heavy weapons (IsHeavyWeapon: launchers, rocket rifles, LAW, tank
//   cannon) are listed together.
// - Weapon attachments: items with an attachment family that are no weapons
//   themselves (scopes, silencer, bipod, duckbill, barrel extender, ...);
//   the under-barrel grenade launcher stays with the launchers.
// - Detonators and the remote bomb trigger are listed with the bombs.
enum ItemSortCategory : UINT8
{
	ISC_PUNCH,
	ISC_BLADE,
	ISC_THROWING_KNIFE,
	ISC_PISTOL,
	ISC_MACHINE_PISTOL,
	ISC_SMG,
	ISC_SHOTGUN,
	ISC_ASSAULT_RIFLE,
	ISC_RIFLE,
	ISC_SNIPER_RIFLE,
	ISC_MG,
	ISC_LAUNCHER,
	ISC_WEAPON_ATTACHMENT,
	ISC_AMMO,
	ISC_ARMOUR,
	ISC_FACE,
	ISC_GRENADE,
	ISC_BOMB,
	ISC_MEDKIT,
	ISC_KIT,
	ISC_MISC,
	ISC_KEY,
	ISC_MONEY,
	ISC_THROWN,
	NUM_ITEM_SORT_CATEGORIES
};

// Which categories of items are sold used (by category, as the original
// table did by item class)
static BOOLEAN const DealerItemSortAllowUsed[NUM_ITEM_SORT_CATEGORIES] =
{
	FALSE, // punch
	FALSE, // blade
	FALSE, // throwing knife
	TRUE,  // pistol
	TRUE,  // machine pistol
	TRUE,  // SMG
	TRUE,  // shotgun
	TRUE,  // assault rifle
	TRUE,  // rifle
	TRUE,  // sniper rifle
	FALSE, // machine gun
	FALSE, // launcher (heavy guns: see CanDealerItemBeSoldUsed)
	TRUE,  // weapon attachment (misc items)
	FALSE, // ammo
	TRUE,  // armour
	TRUE,  // face
	FALSE, // grenade
	FALSE, // bomb
	FALSE, // medkit
	FALSE, // kit
	TRUE,  // misc
	FALSE, // key
	FALSE, // money
	FALSE  // thrown
};


// Detonators and the remote bomb trigger: listed with the bombs, though they
// are misc items (and sold used like them)
bool IsDetonatorItem(UINT16 const usItemIndex)
{
	return usItemIndex == DETONATOR || usItemIndex == REMDETONATOR || usItemIndex == REMOTEBOMBTRIGGER;
}


static UINT8 GetDealerItemCategoryNumber(UINT16 const usItemIndex)
{
	if (IsDetonatorItem(usItemIndex)) return ISC_BOMB;

	ItemModel const* const item = GCM->getItem(usItemIndex);
	UINT32 const item_class = item->getItemClass();

	if (item_class == IC_GUN || item_class == IC_LAUNCHER)
	{
		if (IsHeavyWeapon(usItemIndex)) return ISC_LAUNCHER;
		switch (item->asWeapon()->ubWeaponType)
		{
			case GUN_PISTOL:    return ISC_PISTOL;
			case GUN_M_PISTOL:  return ISC_MACHINE_PISTOL;
			case GUN_SMG:       return ISC_SMG;
			case GUN_SHOTGUN:   return ISC_SHOTGUN;
			case GUN_AS_RIFLE:  return ISC_ASSAULT_RIFLE;
			case GUN_RIFLE:     return ISC_RIFLE;
			case GUN_SN_RIFLE:  return ISC_SNIPER_RIFLE;
			case GUN_LMG:       return ISC_MG;
			default:            return ISC_LAUNCHER;
		}
	}
	if (!item->isWeapon() && item->getAttachmentFamily()) return ISC_WEAPON_ATTACHMENT;

	switch (item_class)
	{
		case IC_PUNCH:          return ISC_PUNCH;
		case IC_BLADE:          return ISC_BLADE;
		case IC_THROWING_KNIFE: return ISC_THROWING_KNIFE;
		case IC_AMMO:           return ISC_AMMO;
		case IC_ARMOUR:         return ISC_ARMOUR;
		case IC_FACE:           return ISC_FACE;
		case IC_GRENADE:        return ISC_GRENADE;
		case IC_BOMB:           return ISC_BOMB;
		case IC_MEDKIT:         return ISC_MEDKIT;
		case IC_KIT:            return ISC_KIT;
		case IC_KEY:            return ISC_KEY;
		case IC_MONEY:          return ISC_MONEY;
		case IC_THROWN:         return ISC_THROWN;
		default:                return ISC_MISC; // IC_MISC and anything else
	}
}


INT8 GetDealersMaxItemAmount(ArmsDealerID const ubDealerID, UINT16 const usItemIndex)
{
	return GCM->getDealerInventory(ubDealerID)->getMaxItemAmount(GCM->getItem(usItemIndex));
}


// docs/game-progress-plan.md, stage 3: every dealer stocks an item only from its
// minDropProgress on (new game progress); for Tony, Devin and Bobby Ray instead of the
// coolness cut-off
static bool DealerStockLimitedByProgress(ArmsDealerID /*bArmsDealer*/)
{
	return gamepolicy(dealer_stock_progress_limits);
}


// Jake's gas cans without the guaranteed deliveries: normal stock, but only after the
// Shank quest; their coolness 0 does not keep them off the shelf
static bool IsFuelForFuelDealer(ArmsDealerID const bArmsDealer, UINT16 const usItemIndex)
{
	return !gamepolicy(dealer_guaranteed_deliveries) &&
		usItemIndex == GAS_CAN && bArmsDealer != ARMS_DEALER_BOBBYR &&
		GCM->getDealer(bArmsDealer)->hasFlag(ArmsDealerFlag::SELLS_FUEL);
}


bool DealerStockBlockedByProgress(ArmsDealerID const bArmsDealer, UINT16 const usItemIndex)
{
	if (!DealerStockLimitedByProgress(bArmsDealer)) return false;

	// Tony and Devin are ahead of the progress: 50% + 10 = items up to 60%
	UINT32 limit = GameProgressPercentage();
	if (bArmsDealer == ARMS_DEALER_TONY || bArmsDealer == ARMS_DEALER_DEVIN)
	{
		limit += gamepolicy(dealer_stock_progress_lead);
	}
	return GCM->getItem(usItemIndex)->getMinDropProgress() > limit;
}


static UINT8 GetCurrentSuitabilityForItem(ArmsDealerID const bArmsDealer, UINT16 const usItemIndex)
{
	UINT8 ubItemCoolness;
	UINT8 ubMinCoolness, ubMaxCoolness;


	// item suitability varies with the player's maximum progress through the game.  The farther he gets, the better items
	// we make available.  Weak items become more and more infrequent later in the game, although they never quite vanish.

	// items illegal in this game are unsuitable [this checks guns vs. current GunSet!]
	if (!ItemIsLegal( usItemIndex ))
	{
		return(ITEM_SUITABILITY_NONE);
	}

	// items normally not sold at shops are unsuitable
	if ( GCM->getItem(usItemIndex)->getFlags() & ITEM_NOT_BUYABLE )
	{
		return(ITEM_SUITABILITY_NONE);
	}


	ubItemCoolness = GCM->getItem(usItemIndex)->getCoolness();

	if (IsFuelForFuelDealer(bArmsDealer, usItemIndex))
	{
		if (!CheckFact(FACT_ESTONI_REFUELLING_POSSIBLE, 0)) return(ITEM_SUITABILITY_NONE);
		if (DealerStockBlockedByProgress(bArmsDealer, usItemIndex)) return(ITEM_SUITABILITY_NONE);
		return(ITEM_SUITABILITY_HIGH); // like his other items
	}

	if (ubItemCoolness == 0)
	{
		// items without a coolness rating can't be sold to the player by shopkeepers
		return(ITEM_SUITABILITY_NONE);
	}

	// not before the game progress reaches the item's minDropProgress (staple items too)
	if (DealerStockBlockedByProgress(bArmsDealer, usItemIndex))
	{
		return(ITEM_SUITABILITY_NONE);
	}

	// the following staple items are always deemed highly suitable regardless of player's progress:
	switch (usItemIndex)
	{
		case CLIP38_6:
		case CLIP9_15:
		case CLIP9_30:
		case CLIP357_6:
		case CLIP357_9:
		case CLIP45_7:
		case CLIP45_30:
		case CLIP12G_7:
		case CLIP12G_7_BUCKSHOT:
		case CLIP545_30_HP:
		case CLIP556_30_HP:
		case CLIP762W_10_HP:
		case CLIP762W_30_HP:
		case CLIP762N_5_HP:
		case CLIP762N_20_HP:

		case FIRSTAIDKIT:
		case MEDICKIT:
		case TOOLKIT:
		case LOCKSMITHKIT:

		case CANTEEN:
		case CROWBAR:
		case JAR:
		case JAR_ELIXIR:
		case JAR_CREATURE_BLOOD:

			return(ITEM_SUITABILITY_ALWAYS);
	}

	switch (bArmsDealer)
	{
		case ARMS_DEALER_BOBBYR:
		case ARMS_DEALER_TONY:
		case ARMS_DEALER_DEVIN:
			break;

		default:
			// All the other dealers have very limited inventories, so their
			// suitability remains constant at all times in game
			return ITEM_SUITABILITY_HIGH;
	}

	// figure out the appropriate range of coolness based on player's maximum progress so far

	ubMinCoolness = HighestPlayerProgressPercentage() / 10;
	ubMaxCoolness = ( HighestPlayerProgressPercentage() / 10 ) + 1;

	// Tony has the better stuff sooner (than Bobby R's)
	if (bArmsDealer == ARMS_DEALER_TONY)
	{
		ubMinCoolness += 1;
		ubMaxCoolness += 1;
	}
	else if (bArmsDealer == ARMS_DEALER_DEVIN)
	{
		// almost everything Devin sells is pretty cool (4+), so gotta apply a minimum or he'd have nothing early on
		if ( ubMinCoolness < 3 )
		{
			ubMinCoolness = 3;
			ubMaxCoolness = 4;
		}
	}

	ubMinCoolness = std::clamp(int(ubMinCoolness), 1, 9);
	ubMaxCoolness = std::clamp(int(ubMaxCoolness), 2, 10);

	// if item is too cool for current level of progress
	if (ubItemCoolness > ubMaxCoolness)
	{
		// minDropProgress has already allowed it: as often as an item inside the window
		if (DealerStockLimitedByProgress(bArmsDealer)) return(ITEM_SUITABILITY_HIGH);
		return(ITEM_SUITABILITY_NONE);
	}

	// if item is exactly within the current coolness window
	if ((ubItemCoolness >= ubMinCoolness) && (ubItemCoolness <= ubMaxCoolness))
	{
		return(ITEM_SUITABILITY_HIGH);
	}

	// if item is still relatively close to low end of the window
	if ((ubItemCoolness + 2) >= ubMinCoolness)
	{
		return(ITEM_SUITABILITY_MEDIUM);
	}

	// item is way uncool for player's current progress, but it's still possible for it to make an appearance
	return(ITEM_SUITABILITY_LOW);
}


UINT8 ChanceOfItemTransaction(ArmsDealerID const bArmsDealer, UINT16 const usItemIndex, BOOLEAN const fDealerIsSelling, BOOLEAN const fUsed)
{
	UINT8 ubItemCoolness;
	UINT8 ubChance = 0;

	// make sure dealers don't carry used items that they shouldn't
	if ( fUsed && !fDealerIsSelling && !CanDealerItemBeSoldUsed( usItemIndex ) )
		return( 0 );

	// Bobby Ray has an easier time getting resupplied than the local dealers do
	BOOLEAN const fBobbyRay = bArmsDealer == ARMS_DEALER_BOBBYR;

	ubItemCoolness = GCM->getItem(usItemIndex)->getCoolness();

	switch (GetCurrentSuitabilityForItem( bArmsDealer, usItemIndex ) )
	{
		case ITEM_SUITABILITY_NONE:
			if (fDealerIsSelling)
			{
				// dealer always gets rid of stuff that is too advanced or inappropriate ASAP
				ubChance = 100;
			}
			else // dealer is buying
			{
				// can't get these at all
				ubChance = 0;
			}
			break;

		case ITEM_SUITABILITY_LOW:
			ubChance = (fBobbyRay) ? 25 : 15;
			break;

		case ITEM_SUITABILITY_MEDIUM:
			ubChance = (fBobbyRay) ? 50 : 30;
			break;

		case ITEM_SUITABILITY_HIGH:
			ubChance = (fBobbyRay) ? 75 : 50;
			break;

		case ITEM_SUITABILITY_ALWAYS:
			if (fDealerIsSelling)
			{
				// sells just like suitability high
				ubChance = 75;
			}
			else // dealer is buying
			{
				// dealer can always get a (re)supply of these
				ubChance = 100;
			}
			break;

		default:
			SLOGA("ChanceOfItemTransaction: invalid item suitability");
			break;
	}


	// if there's any uncertainty
	if ((ubChance > 0) && (ubChance < 100))
	{
		// cooler items sell faster
		if (fDealerIsSelling)
		{
			ubChance += (5 * ubItemCoolness);

			// ARM: New - keep stuff on the shelves longer
			ubChance /= 2;
		}

		// used items are traded more rarely
		if (fUsed)
		{
			ubChance /= 2;
		}
	}


	return(ubChance);
}


BOOLEAN ItemTransactionOccurs(ArmsDealerID const bArmsDealer, UINT16 const usItemIndex, BOOLEAN const fDealerIsSelling, BOOLEAN const fUsed)
{
	UINT8 ubChance;
	INT16 sInventorySlot;


	ubChance = ChanceOfItemTransaction( bArmsDealer, usItemIndex, fDealerIsSelling, fUsed );

	// if the dealer is buying, and a chance exists (i.e. the item is "eligible")
	if (!fDealerIsSelling && (ubChance > 0))
	{
		// mark it as such
		if (bArmsDealer == ARMS_DEALER_BOBBYR)
		{
			STORE_INVENTORY* pInventory = fUsed ? LaptopSaveInfo.BobbyRayUsedInventory : LaptopSaveInfo.BobbyRayInventory;
			sInventorySlot = GetInventorySlotForItem(pInventory, usItemIndex, fUsed);
			if (sInventorySlot == -1)
			{
				return(FALSE);
			}
			pInventory[ sInventorySlot ].fPreviouslyEligible = TRUE;
		}
		else
		{
			gArmsDealersInventory[ bArmsDealer ][ usItemIndex ].fPreviouslyEligible = TRUE;
		}
	}

	// roll to see if a transaction occurs
	if (Random(100) < ubChance)
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}


UINT8 DetermineInitialInvItems(ArmsDealerID const bArmsDealerID, UINT16 const usItemIndex, UINT8 const ubChances, BOOLEAN const fUsed)
{
	UINT8 ubNumBought;
	UINT8 ubCnt;

	// initial inventory is now rolled for one item at a time, instead of one type at a time, to improve variety
	ubNumBought = 0;
	for (ubCnt = 0; ubCnt < ubChances; ubCnt++)
	{
		if (ItemTransactionOccurs( bArmsDealerID, usItemIndex, DEALER_BUYING, fUsed))
		{
			ubNumBought++;
		}
	}

	return( ubNumBought );
}


UINT8 HowManyItemsAreSold(ArmsDealerID const bArmsDealerID, UINT16 const usItemIndex, UINT8 const ubNumInStock, BOOLEAN const fUsed)
{
	UINT8 ubNumSold;
	UINT8 ubCnt;

	// items are now virtually "sold" one at a time
	ubNumSold = 0;
	for (ubCnt = 0; ubCnt < ubNumInStock; ubCnt++)
	{
		if (ItemTransactionOccurs( bArmsDealerID, usItemIndex, DEALER_SELLING, fUsed))
		{
			ubNumSold++;
		}
	}

	return( ubNumSold );
}



UINT8 HowManyItemsToReorder(UINT8 ubWanted, UINT8 ubStillHave)
{
	UINT8 ubNumReordered;

	Assert(ubStillHave <= ubWanted);

	ubNumReordered = ubWanted - ubStillHave;

	//randomize the amount. 33% of the time we add to it, 33% we subtract from it, rest leave it alone
	switch (Random(3))
	{
		case 0:
			ubNumReordered += ubNumReordered / 2;
			break;
		case 1:
			ubNumReordered -= ubNumReordered / 2;
			break;
	}

	return(ubNumReordered);
}



int BobbyRayItemQsortCompare(const void *pArg1, const void *pArg2)
{
	UINT16 usItem1Index;
	UINT16 usItem2Index;
	UINT8  ubItem1Quality;
	UINT8  ubItem2Quality;

	usItem1Index = ( ( STORE_INVENTORY * ) pArg1 ) -> usItemIndex;
	usItem2Index = ( ( STORE_INVENTORY * ) pArg2 ) -> usItemIndex;

	ubItem1Quality = ( ( STORE_INVENTORY * ) pArg1 ) -> ubItemQuality;
	ubItem2Quality = ( ( STORE_INVENTORY * ) pArg2 ) -> ubItemQuality;

	return( CompareItemsForSorting( usItem1Index, usItem2Index, ubItem1Quality, ubItem2Quality ) );
}



int ArmsDealerItemQsortCompare(const void *pArg1, const void *pArg2)
{
	UINT16 usItem1Index;
	UINT16 usItem2Index;
	UINT8  ubItem1Quality;
	UINT8  ubItem2Quality;

	usItem1Index = ( ( INVENTORY_IN_SLOT * ) pArg1 ) -> sItemIndex;
	usItem2Index = ( ( INVENTORY_IN_SLOT * ) pArg2 ) -> sItemIndex;

	ubItem1Quality = ( ( INVENTORY_IN_SLOT * ) pArg1 ) -> ItemObject.bStatus[ 0 ];
	ubItem2Quality = ( ( INVENTORY_IN_SLOT * ) pArg2 ) -> ItemObject.bStatus[ 0 ];

	return( CompareItemsForSorting( usItem1Index, usItem2Index, ubItem1Quality, ubItem2Quality ) );
}


int CompareItemsForSorting(UINT16 const item_index1, UINT16 const item_index2, UINT8 const item_quality1, UINT8 const item_quality2)
{
	// lower category first
	UINT8 const category1 = GetDealerItemCategoryNumber(item_index1);
	UINT8 const category2 = GetDealerItemCategoryNumber(item_index2);
	if (category1 < category2) return -1;
	if (category1 > category2) return  1;

	const ItemModel * item1 = GCM->getItem(item_index1);
	const ItemModel * item2 = GCM->getItem(item_index2);

	// the same category
	if (item1->getItemClass() == IC_AMMO && item2->getItemClass() == IC_AMMO)
	{
		// AMMO is sorted by caliber first
		uint16_t calibre1 = item1->asAmmo()->calibre->index;
		uint16_t calibre2 = item2->asAmmo()->calibre->index;
		if (calibre1 > calibre2) return -1;
		if (calibre1 < calibre2) return  1;

		// the same caliber - compare size of magazine
		UINT8 const mag_size1 = item1->asAmmo()->capacity;
		UINT8 const mag_size2 = item2->asAmmo()->capacity;
		if (mag_size1 > mag_size2) return -1;
		if (mag_size1 < mag_size2) return  1;
	}
	else
	{
		// items other than ammo are compared on coolness first
		// higher coolness first
		UINT8 const coolness1 = item1->getCoolness();
		UINT8 const coolness2 = item2->getCoolness();
		if (coolness1 > coolness2) return -1;
		if (coolness1 < coolness2) return  1;
	}

	// the same coolness/caliber - compare base prices then
	// higher price first
	UINT16 const price1 = item1->getPrice();
	UINT16 const price2 = item2->getPrice();
	if (price1 > price2) return -1;
	if (price1 < price2) return  1;

	// the same price - compare item #s, then

	// lower index first
	if (item_index1 < item_index2) return -1;
	if (item_index1 > item_index2) return  1;

	// same item type = compare item quality, then

	// higher quality first
	if (item_quality1 > item_quality2) return -1;
	if (item_quality1 < item_quality2) return  1;

	// identical items!
	return 0;
}





BOOLEAN CanDealerItemBeSoldUsed( UINT16 usItemIndex )
{
	if ( !( GCM->getItem(usItemIndex)->getFlags() & ITEM_DAMAGEABLE ) )
		return(FALSE);

	// certain items, although they're damagable, shouldn't be sold in a used condition
	// heavy guns (rocket rifles, LAW, tank cannon) were rifles here: sold used
	if (GCM->getItem(usItemIndex)->isGun() && IsHeavyWeapon(usItemIndex)) return TRUE;
	// detonators were misc items here: sold used as before
	if (IsDetonatorItem(usItemIndex)) return DealerItemSortAllowUsed[ISC_MISC];
	return( DealerItemSortAllowUsed[ GetDealerItemCategoryNumber( usItemIndex ) ] );
}
