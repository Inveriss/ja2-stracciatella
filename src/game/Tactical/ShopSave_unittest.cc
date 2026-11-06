#ifdef WITH_UNITTESTS

// Sparse save of the shop tables (dealers and Bobby Ray), docs/item-limits-plan.md D9.

#include "Arms_Dealer_Init.h"
#include "BobbyR.h"
#include "BobbyRGuns.h"
#include "DefaultContentManagerUT.h"
#include "FileMan.h"
#include "GameVersion.h"
#include "Item_Types.h"
#include "Laptop.h"
#include "LaptopSave.h"
#include "LoadSaveData.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace {

void ClearDealers()
{
	for (DEALER_ITEM_HEADER (&dealer)[MAXITEMS] : gArmsDealersInventory)
	{
		std::fill(std::begin(dealer), std::end(dealer), DEALER_ITEM_HEADER{});
	}
}

}

using ShopSaveTest = DefaultContentManagerUT::BaseTest;

TEST_F(ShopSaveTest, dealersRoundTrip)
{
	ClearDealers();
	DEALER_ITEM_HEADER& glock = gArmsDealersInventory[ARMS_DEALER_TONY][GLOCK_17];
	glock.ubTotalItems       = 3;
	glock.ubPerfectItems     = 2;
	glock.uiOrderArrivalTime = 12;
	glock.ubQtyOnOrder       = 4;
	glock.SpecialItem.assign(1, DEALER_SPECIAL_ITEM{});
	glock.SpecialItem[0].fActive              = TRUE;
	glock.SpecialItem[0].Info.bItemCondition  = 55;
	glock.SpecialItem[0].Info.usAttachment[0] = SILENCER;
	gArmsDealersInventory[ARMS_DEALER_MANNY][CLIP9_15].fPreviouslyEligible = TRUE;
	gArmsDealerStatus[ARMS_DEALER_TONY].uiArmsDealersCash = 1234;

	{
		AutoSGPFile f(cm->tempFiles()->openForWriting("dealers.sav", true));
		SaveArmsDealerInventoryToSaveGameFile(f);
	}

	ClearDealers();
	gArmsDealerStatus[ARMS_DEALER_TONY].uiArmsDealersCash = 0;
	gArmsDealersInventory[ARMS_DEALER_KEITH][GLOCK_18].ubTotalItems = 9; // must be cleared by the load

	{
		AutoSGPFile f(cm->tempFiles()->openForReading("dealers.sav"));
		// status + one count per dealer + 2 entries (2 + 16 bytes) + 1 special item
		EXPECT_EQ(f->size(), sizeof(gArmsDealerStatus) + 2u * NUM_ARMS_DEALERS + 2u * 18 + sizeof(DEALER_SPECIAL_ITEM));
		LoadArmsDealerInventoryFromSavedGameFile(f, SAVE_GAME_VERSION);
	}

	DEALER_ITEM_HEADER const& g = gArmsDealersInventory[ARMS_DEALER_TONY][GLOCK_17];
	EXPECT_EQ(g.ubTotalItems, 3);
	EXPECT_EQ(g.ubPerfectItems, 2);
	EXPECT_EQ(g.uiOrderArrivalTime, 12u);
	EXPECT_EQ(g.ubQtyOnOrder, 4);
	ASSERT_EQ(g.SpecialItem.size(), 1u);
	EXPECT_TRUE(g.SpecialItem[0].fActive);
	EXPECT_EQ(g.SpecialItem[0].Info.bItemCondition, 55);
	EXPECT_EQ(g.SpecialItem[0].Info.usAttachment[0], SILENCER);
	EXPECT_TRUE(gArmsDealersInventory[ARMS_DEALER_MANNY][CLIP9_15].fPreviouslyEligible);
	EXPECT_EQ(gArmsDealersInventory[ARMS_DEALER_KEITH][GLOCK_18].ubTotalItems, 0);
	EXPECT_EQ(gArmsDealerStatus[ARMS_DEALER_TONY].uiArmsDealersCash, 1234u);
	ClearDealers();
}

TEST_F(ShopSaveTest, dealerItemNotInDataIsRejected)
{
	ClearDealers();
	{
		AutoSGPFile f(cm->tempFiles()->openForWriting("dealers.sav", true));
		f->write(gArmsDealerStatus, sizeof(gArmsDealerStatus));
		for (int dealer = 0; dealer != NUM_ARMS_DEALERS; ++dealer)
		{
			BYTE data[2 + 2 + 16]{};
			DataWriter d{data};
			INJ_U16(d, dealer == 0 ? 1 : 0)
			INJ_U16(d, FIRST_NEW_KEY) // a placeholder: no data defines it
			INJ_U8( d, 1)
			f->write(data, dealer == 0 ? sizeof(data) : 2);
		}
	}
	AutoSGPFile f(cm->tempFiles()->openForReading("dealers.sav"));
	EXPECT_THROW(LoadArmsDealerInventoryFromSavedGameFile(f, SAVE_GAME_VERSION), std::runtime_error);
	ClearDealers();
}

TEST(BobbyRRestock, usedFlagDoesNotCollideWithItemIndices)
{
	for (UINT16 const item : { UINT16{GLOCK_17}, UINT16{999}, FIRST_NEW_WEAPON, UINT16{1500}, LAST_NEW_KEY })
	{
		EXPECT_EQ(BobbyRRestockItem(BobbyRRestockParam(item, false)), item);
		EXPECT_FALSE(BobbyRRestockIsUsed(BobbyRRestockParam(item, false)));
		EXPECT_EQ(BobbyRRestockItem(BobbyRRestockParam(item, true)), item);
		EXPECT_TRUE(BobbyRRestockIsUsed(BobbyRRestockParam(item, true)));
	}
	// a new weapon at 1000 was a used GLOCK_17 + 1000 before
	EXPECT_FALSE(BobbyRRestockIsUsed(FIRST_NEW_WEAPON));
}

TEST_F(ShopSaveTest, bobbyRayRoundTrip)
{
	LaptopSaveInfoStruct& l = LaptopSaveInfo;
	std::fill(std::begin(l.BobbyRayInventory), std::end(l.BobbyRayInventory), STORE_INVENTORY{});
	std::fill(std::begin(l.BobbyRayUsedInventory), std::end(l.BobbyRayUsedInventory), STORE_INVENTORY{});
	l.BobbyRayOrdersOnDeliveryArray.clear();
	l.usNumberOfBobbyRayOrderUsed = 0;
	l.pLifeInsurancePayouts.clear();
	l.ubNumberLifeInsurancePayoutUsed = 0;

	l.BobbyRayInventory[0].usItemIndex = GLOCK_17;
	l.BobbyRayInventory[0].ubQtyOnHand = 5;
	l.BobbyRayInventory[1].usItemIndex = CLIP9_15;
	l.BobbyRayInventory[1].fNotifyOnRestock = TRUE;
	l.BobbyRayInventory[2].usItemIndex = BOBBYR_NO_ITEMS;
	l.BobbyRayUsedInventory[0].usItemIndex = GLOCK_17; // not saved: no used stock
	l.BobbyRayUsedInventory[0].ubQtyOnHand = 2;
	l.iCurrentBalance = 4321;

	{
		AutoSGPFile f(cm->tempFiles()->openForWriting("laptop.sav", true));
		SaveLaptopInfoToSavedGame(f);
	}

	std::fill(std::begin(l.BobbyRayInventory), std::end(l.BobbyRayInventory), STORE_INVENTORY{});
	l.BobbyRayUsedInventory[7].ubQtyOnHand = 3; // must be cleared by the load
	l.iCurrentBalance = 0;

	{
		AutoSGPFile f(cm->tempFiles()->openForReading("laptop.sav"));
		// fixed block + count + 3 slots of 10 bytes; the used stock is not saved
		EXPECT_EQ(f->size(), 1829u + 2 + 3 * 10);
		LoadLaptopInfoFromSavedGame(f);
	}

	EXPECT_EQ(l.BobbyRayInventory[0].usItemIndex, GLOCK_17);
	EXPECT_EQ(l.BobbyRayInventory[0].ubQtyOnHand, 5);
	EXPECT_EQ(l.BobbyRayInventory[1].usItemIndex, CLIP9_15);
	EXPECT_TRUE(l.BobbyRayInventory[1].fNotifyOnRestock);
	EXPECT_EQ(l.BobbyRayInventory[2].usItemIndex, BOBBYR_NO_ITEMS);
	EXPECT_EQ(l.BobbyRayInventory[3].usItemIndex, 0);
	// an empty used list after loading
	EXPECT_EQ(l.BobbyRayUsedInventory[0].usItemIndex, BOBBYR_NO_ITEMS);
	EXPECT_EQ(l.BobbyRayUsedInventory[0].ubQtyOnHand, 0);
	EXPECT_EQ(l.BobbyRayUsedInventory[7].ubQtyOnHand, 0);
	EXPECT_EQ(l.usInventoryListLength[BOBBY_RAY_USED], 0);
	EXPECT_EQ(l.iCurrentBalance, 4321);
}

#endif
