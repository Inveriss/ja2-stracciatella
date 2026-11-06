#ifndef __BOBBYR_H
#define __BOBBYR_H

#include "Laptop.h"
#include "Store_Inventory.h"


void EnterBobbyR(void);
void ExitBobbyR(void);
void HandleBobbyR(void);
void RenderBobbyR(void);


#define BOBBYR_BACKGROUND_WIDTH		125
#define BOBBYR_BACKGROUND_HEIGHT	100
#define BOBBYR_NUM_HORIZONTAL_TILES	4
#define BOBBYR_NUM_VERTICAL_TILES	4

#define BOBBYR_GRIDLOC_X		LAPTOP_SCREEN_UL_X + 4
#define BOBBYR_GRIDLOC_Y		LAPTOP_SCREEN_WEB_UL_Y + 45


extern LaptopMode guiLastBobbyRayPage;


void DrawBobbyRWoodBackground(void);
void DeleteBobbyRWoodBackground(void);
void InitBobbyRWoodBackground(void);
void DailyUpdateOfBobbyRaysNewInventory(void);
void DailyUpdateOfBobbyRaysUsedInventory(void);
// The parameter of an EVENT_UPDATE_BOBBY_RAY_INVENTORY event: the item index in
// the low 16 bits, plus this flag for the used stock (formerly item + 1000,
// which would collide with the item indices from 1000 on, docs/item-limits-plan.md).
constexpr UINT32 BOBBY_R_USED_ORDER_FLAG = 0x10000;
constexpr UINT32 BobbyRRestockParam(UINT16 const usItemIndex, bool const fUsed)
{
	return usItemIndex | (fUsed ? BOBBY_R_USED_ORDER_FLAG : 0);
}
constexpr UINT16 BobbyRRestockItem(UINT32 const uiParam)  { return static_cast<UINT16>(uiParam & 0xFFFF); }
constexpr bool   BobbyRRestockIsUsed(UINT32 const uiParam) { return (uiParam & BOBBY_R_USED_ORDER_FLAG) != 0; }

void AddFreshBobbyRayInventory(UINT16 usItemIndex, bool fUsed);
void InitBobbyRayInventory(void);
void CancelAllPendingBRPurchaseOrders(void);
INT16 GetInventorySlotForItem(STORE_INVENTORY *pInventoryArray, UINT16 usItemIndex, BOOLEAN fUsed);


#endif
