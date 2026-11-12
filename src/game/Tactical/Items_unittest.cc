#ifdef WITH_UNITTESTS

// Gun stacks with a condition per unit, docs/gun-stacking-plan.md stage 1,
// and enemy drops limited by progress, docs/enemy-drop-progress-plan.md.

#include "DefaultContentManager.h"
#include "DefaultContentManagerUT.h"
#include "GameInstance.h"
#include "Item_Types.h"
#include "Items.h"
#include "LoadSaveData.h"
#include "LoadSaveObjectType.h"
#include "GamePolicy.h"
#include "ItemModel.h"
#include "Soldier_Control.h"
#include "Strategic_Status.h"
#include "Campaign.h"
#include "Campaign_Types.h"
#include "Quests.h"
#include "ArmsDealerInvInit.h"
#include "Weapons.h"
#include "SamSiteModel.h"
#include "strategic/TownModel.h"

#include "gtest/gtest.h"

#include <memory>

namespace {

struct GunStackTest : public ::testing::Test
{
	std::unique_ptr<DefaultContentManager> cm;

	void SetUp() override
	{
		cm.reset(DefaultContentManagerUT::createDefaultCMForTesting());
		ASSERT_TRUE(cm->loadGameData());
		GCM = cm.get();
	}

	void TearDown() override
	{
		GCM = nullptr;
	}

	static OBJECTTYPE Gun(INT8 const status)
	{
		OBJECTTYPE o{};
		CreateItem(GLOCK_17, status, &o);
		return o;
	}

	static void ExpectSameAmmo(OBJECTTYPE const& a, OBJECTTYPE const& b)
	{
		EXPECT_EQ(a.ubGunAmmoType,  b.ubGunAmmoType);
		EXPECT_EQ(a.ubGunShotsLeft, b.ubGunShotsLeft);
		EXPECT_EQ(a.usGunAmmoItem,  b.usGunAmmoItem);
		EXPECT_EQ(a.bGunAmmoStatus, b.bGunAmmoStatus);
	}
};

}


TEST_F(GunStackTest, unitStatusOfGunsAndOtherItems)
{
	OBJECTTYPE gun = Gun(90);
	EXPECT_EQ(UnitStatus(gun, 0), 90);
	SetUnitStatus(gun, 1, 40);
	SetUnitStatus(gun, MAX_GUNS_PER_STACK - 1, 30);
	EXPECT_EQ(gun.bGunUnitStatus[0], 40);
	EXPECT_EQ(UnitStatus(gun, 1), 40);
	EXPECT_EQ(UnitStatus(gun, MAX_GUNS_PER_STACK - 1), 30);
	// the ammo bytes are untouched
	ExpectSameAmmo(gun, Gun(90));
	// beyond the last unit: nothing is written
	SetUnitStatus(gun, MAX_GUNS_PER_STACK, 10);
	EXPECT_EQ(UnitStatus(gun, MAX_GUNS_PER_STACK), 0);

	OBJECTTYPE kit{};
	CreateItem(FIRSTAIDKIT, 70, &kit);
	SetUnitStatus(kit, 3, 55);
	EXPECT_EQ(kit.bStatus[3], 55);
	EXPECT_EQ(UnitStatus(kit, 3), 55);
}


TEST_F(GunStackTest, gunsOfDifferentConditionStackAndSplit)
{
	OBJECTTYPE const reference = Gun(90);
	OBJECTTYPE a = Gun(90);
	OBJECTTYPE b = Gun(40);
	OBJECTTYPE c = Gun(70);
	ASSERT_TRUE(CanGunsStack(a, b));

	StackObjs(&b, &a, 1);
	StackObjs(&c, &a, 1);
	EXPECT_EQ(a.ubNumberOfObjects, 3);
	EXPECT_EQ(b.usItem, NOTHING);
	EXPECT_EQ(UnitStatus(a, 0), 90);
	EXPECT_EQ(UnitStatus(a, 1), 40);
	EXPECT_EQ(UnitStatus(a, 2), 70);
	ExpectSameAmmo(a, reference);

	// take the middle one out
	OBJECTTYPE out{};
	GetObjFrom(&a, 1, &out);
	EXPECT_EQ(out.usItem, GLOCK_17);
	EXPECT_EQ(out.ubNumberOfObjects, 1);
	EXPECT_EQ(out.bGunStatus, 40);
	EXPECT_EQ(UnitStatus(out, 1), 0);
	ExpectSameAmmo(out, reference);
	EXPECT_EQ(a.ubNumberOfObjects, 2);
	EXPECT_EQ(UnitStatus(a, 0), 90);
	EXPECT_EQ(UnitStatus(a, 1), 70);
	ExpectSameAmmo(a, reference);

	// remove the first one: the next one becomes unit 0
	RemoveObjFrom(&a, 0);
	EXPECT_EQ(a.ubNumberOfObjects, 1);
	EXPECT_EQ(a.bGunStatus, 70);
	ExpectSameAmmo(a, reference);
}


TEST_F(GunStackTest, sortAndSwapUseTheUnitCondition)
{
	OBJECTTYPE const reference = Gun(90);
	OBJECTTYPE a = Gun(20);
	OBJECTTYPE b = Gun(90);
	OBJECTTYPE c = Gun(50);
	StackObjs(&b, &a, 1);
	StackObjs(&c, &a, 1);

	SortItemStackByStatus(&a);
	EXPECT_EQ(UnitStatus(a, 0), 90);
	EXPECT_EQ(UnitStatus(a, 1), 50);
	EXPECT_EQ(UnitStatus(a, 2), 20);
	ExpectSameAmmo(a, reference);

	OBJECTTYPE d = Gun(60);
	EXPECT_TRUE(PlaceObjectAtObjectIndex(&d, &a, 2));
	EXPECT_EQ(d.bGunStatus, 20);
	EXPECT_EQ(UnitStatus(a, 2), 60);
	ExpectSameAmmo(a, reference);
	ExpectSameAmmo(d, reference);
}


TEST_F(GunStackTest, differentAmmoOrAttachmentsDontStack)
{
	OBJECTTYPE a = Gun(90);
	OBJECTTYPE b = Gun(90);
	b.ubGunShotsLeft = a.ubGunShotsLeft - 1;
	EXPECT_FALSE(CanGunsStack(a, b));

	OBJECTTYPE c = Gun(90);
	c.usAttachItem[0]  = LASERSCOPE;
	c.bAttachStatus[0] = 100;
	EXPECT_FALSE(CanGunsStack(a, c));

	OBJECTTYPE d = Gun(90);
	d.usAttachItem[0]  = LASERSCOPE;
	d.bAttachStatus[0] = 80;
	EXPECT_FALSE(CanGunsStack(c, d));
	d.bAttachStatus[0] = 100;
	EXPECT_TRUE(CanGunsStack(c, d));
}


TEST_F(GunStackTest, createItemsKeepsTheAmmo)
{
	OBJECTTYPE const reference = Gun(80);
	OBJECTTYPE o{};
	CreateItems(GLOCK_17, 80, 3, &o);
	EXPECT_EQ(o.ubNumberOfObjects, 3);
	for (UINT8 i = 0; i != 3; ++i) EXPECT_EQ(UnitStatus(o, i), 80);
	ExpectSameAmmo(o, reference);

	CreateItems(GLOCK_17, 80, MAX_OBJECTS_PER_SLOT, &o);
	EXPECT_EQ(o.ubNumberOfObjects, MAX_GUNS_PER_STACK);
	ExpectSameAmmo(o, reference);
}


TEST_F(GunStackTest, stackSizes)
{
	// game.json gun_stack_size (94) for guns without their own stackSize
	EXPECT_EQ(GunStackSize(GLOCK_17), 94);
	EXPECT_EQ(SectorSlotLimit(GLOCK_17), 94);
	EXPECT_EQ(SectorSlotLimit(G41), 94);
	// a merc's own slots keep one gun (G7)
	EXPECT_LE(ItemSlotLimit(GLOCK_17, BIGPOCK1POS), 1);
	EXPECT_EQ(ItemSlotLimit(G41, BIGPOCK1POS), 0);
	// other items keep their big pocket limit
	EXPECT_EQ(GunStackSize(CLIP9_15), 1);
	EXPECT_EQ(SectorSlotLimit(CLIP9_15), ItemSlotLimit(CLIP9_15, BIGPOCK1POS));
}


TEST_F(GunStackTest, removingAnAttachmentGivesOnePerGun)
{
	OBJECTTYPE a = Gun(90);
	OBJECTTYPE b = Gun(40);
	for (OBJECTTYPE* o : { &a, &b })
	{
		o->usAttachItem[0]  = LASERSCOPE;
		o->bAttachStatus[0] = 80;
	}
	StackObjs(&b, &a, 1);
	ASSERT_EQ(a.ubNumberOfObjects, 2);

	OBJECTTYPE scopes{};
	ASSERT_TRUE(RemoveAttachment(&a, 0, &scopes));
	EXPECT_EQ(scopes.usItem, LASERSCOPE);
	EXPECT_EQ(scopes.ubNumberOfObjects, 2);
	EXPECT_EQ(scopes.bStatus[0], 80);
	EXPECT_EQ(scopes.bStatus[1], 80);
	EXPECT_EQ(a.usAttachItem[0], NOTHING);
	EXPECT_EQ(a.ubNumberOfObjects, 2);
	EXPECT_EQ(UnitStatus(a, 1), 40);
}


TEST_F(GunStackTest, saveKeepsEveryUnitCondition)
{
	OBJECTTYPE a = Gun(90);
	OBJECTTYPE b = Gun(40);
	OBJECTTYPE c = Gun(70);
	StackObjs(&b, &a, 1);
	StackObjs(&c, &a, 1);

	uint8_t data[172]{};
	{
		DataWriter d{data};
		InjectObject(d, &a);
	}
	OBJECTTYPE loaded{};
	{
		DataReader d{data};
		ExtractObject(d, &loaded);
	}
	EXPECT_EQ(loaded.ubNumberOfObjects, 3);
	EXPECT_EQ(UnitStatus(loaded, 0), 90);
	EXPECT_EQ(UnitStatus(loaded, 1), 40);
	EXPECT_EQ(UnitStatus(loaded, 2), 70);
	ExpectSameAmmo(loaded, a);
}


// Enemy drops limited by game progress, docs/enemy-drop-progress-plan.md stage 1.

namespace {

struct DropProgressTest : public GunStackTest
{
	// docs/game-progress-plan.md: progress p (even) from first visits of the counted sectors
	static void ClearGameProgress()
	{
		for (auto& si : SectorInfo) si.uiFlags &= ~SF_PROGRESS_VISITED;
		for (auto& q : gubQuest) q = QUESTNOTSTARTED;
		for (auto& b : gStrategicStatus.ubGameProgressQuestsDone) b = 0;
		gStrategicStatus.ubGameProgressAnnounced = 0;
	}

	static void SetGameProgress(UINT8 const p)
	{
		ClearGameProgress();
		for (auto const& town : GCM->getTowns())
		{
			for (UINT8 const id : town.second->sectorIDs)
			{
				if (GameProgressPercentage() >= p) break;
				SectorInfo[id].uiFlags |= SF_PROGRESS_VISITED;
			}
		}
		ASSERT_EQ(GameProgressPercentage(), p);
	}
	static void SetMinDrop(UINT16 const item, UINT8 const progress)
	{
		const_cast<ItemModel*>(GCM->getItem(item))->setMinDropProgress(progress);
	}

	static OBJECTTYPE ScopedGun()
	{
		OBJECTTYPE o = Gun(90);
		o.usAttachItem[0]  = LASERSCOPE;
		o.bAttachStatus[0] = 80;
		return o;
	}
};

}


TEST_F(DropProgressTest, baseDataStartsFromCoolness)
{
	// placeholder values until they are tuned: ubCoolness x 10
	for (UINT16 const item : { GLOCK_17, G41, LASERSCOPE, CLIP9_15, FIRSTAIDKIT })
	{
		ItemModel const* const m = GCM->getItem(item);
		EXPECT_EQ(m->getMinDropProgress(), m->getCoolness() * 10) << m->getInternalName().c_str();
	}
}


TEST_F(DropProgressTest, itemsWithoutThresholdAlwaysDrop)
{
	SetMinDrop(GLOCK_17, 0);
	SetMinDrop(LASERSCOPE, 0);
	OBJECTTYPE o = ScopedGun();
	EXPECT_TRUE(ApplyDropProgressLimit(o, 0));
	EXPECT_EQ(o.usAttachItem[0], LASERSCOPE);
}


TEST_F(DropProgressTest, itemBelowItsThresholdDoesNotDrop)
{
	SetMinDrop(GLOCK_17, 30);
	SetMinDrop(LASERSCOPE, 0);
	OBJECTTYPE o = ScopedGun();
	EXPECT_FALSE(ApplyDropProgressLimit(o, 29));
	EXPECT_TRUE(ApplyDropProgressLimit(o, 30));
	EXPECT_TRUE(ApplyDropProgressLimit(o, 100));
	OBJECTTYPE nothing{};
	EXPECT_TRUE(ApplyDropProgressLimit(nothing, 0));
}


TEST_F(DropProgressTest, blockedAttachmentIsRemoved)
{
	SetMinDrop(GLOCK_17, 0);
	SetMinDrop(LASERSCOPE, 80);
	OBJECTTYPE o = ScopedGun();
	EXPECT_TRUE(ApplyDropProgressLimit(o, 80));
	EXPECT_EQ(o.usAttachItem[0], LASERSCOPE);
	EXPECT_EQ(o.bAttachStatus[0], 80);

	EXPECT_TRUE(ApplyDropProgressLimit(o, 79));
	EXPECT_EQ(o.usAttachItem[0], NOTHING);
	EXPECT_EQ(o.bAttachStatus[0], 0);
	EXPECT_EQ(o.usItem, GLOCK_17);
	EXPECT_EQ(o.bGunStatus, 90);
}


TEST_F(DropProgressTest, onlyTheEnemyTeamIsLimited)
{
	SetMinDrop(GLOCK_17, 50);
	SetGameProgress(20);

	auto s = SOLDIERTYPE{};
	OBJECTTYPE o = Gun(90);
	s.bTeam = ENEMY_TEAM;
	EXPECT_FALSE(ApplyEnemyDropProgressLimit(s, o));
	for (INT8 const team : { OUR_TEAM, MILITIA_TEAM, CIV_TEAM, CREATURE_TEAM })
	{
		s.bTeam = team;
		EXPECT_TRUE(ApplyEnemyDropProgressLimit(s, o));
	}

	// the record reaches the threshold
	s.bTeam = ENEMY_TEAM;
	SetGameProgress(50);
	EXPECT_TRUE(ApplyEnemyDropProgressLimit(s, o));
	SetGameProgress(0);
}


TEST_F(DropProgressTest, pickedUpObjectsAndSwitchOffAreNotLimited)
{
	SetMinDrop(GLOCK_17, 50);
	SetGameProgress(0);
	auto s = SOLDIERTYPE{};
	s.bTeam = ENEMY_TEAM;

	OBJECTTYPE o = Gun(90);
	o.fFlags |= OBJECT_PICKED_UP;
	EXPECT_TRUE(ApplyEnemyDropProgressLimit(s, o));

	OBJECTTYPE p = Gun(90);
	ASSERT_TRUE(gamepolicy(enemy_drop_progress_limits));
	const_cast<GamePolicy*>(GCM->getGamePolicy())->enemy_drop_progress_limits = false;
	EXPECT_TRUE(ApplyEnemyDropProgressLimit(s, p));
	const_cast<GamePolicy*>(GCM->getGamePolicy())->enemy_drop_progress_limits = true;
	SetGameProgress(0);
}


TEST_F(DropProgressTest, blockedItemForTheWeaponDropRecord)
{
	SetMinDrop(GLOCK_17, 50);
	SetMinDrop(G41, 0);
	SetGameProgress(48);
	EXPECT_TRUE(EnemyDropBlockedByProgress(GLOCK_17));
	EXPECT_FALSE(EnemyDropBlockedByProgress(G41));
	EXPECT_FALSE(EnemyDropBlockedByProgress(NOTHING));
	SetGameProgress(50);
	EXPECT_FALSE(EnemyDropBlockedByProgress(GLOCK_17));

	SetGameProgress(0);
	const_cast<GamePolicy*>(GCM->getGamePolicy())->enemy_drop_progress_limits = false;
	EXPECT_FALSE(EnemyDropBlockedByProgress(GLOCK_17));
	const_cast<GamePolicy*>(GCM->getGamePolicy())->enemy_drop_progress_limits = true;
	SetGameProgress(0);
}


TEST_F(DropProgressTest, blockedItemNeverDropsFromEnemies)
{
	// D13: "minDropProgress": "blocked"
	const_cast<ItemModel*>(GCM->getItem(GLOCK_17))->setEnemyDropBlocked(true);
	SetMinDrop(GLOCK_17, 0);
	SetGameProgress(70); // any progress: the word, not a number, blocks it

	auto s = SOLDIERTYPE{};
	OBJECTTYPE o = Gun(90);
	s.bTeam = ENEMY_TEAM;
	EXPECT_FALSE(ApplyEnemyDropProgressLimit(s, o));
	EXPECT_TRUE(EnemyDropBlockedByProgress(GLOCK_17));
	for (INT8 const team : { OUR_TEAM, MILITIA_TEAM, CIV_TEAM, CREATURE_TEAM })
	{
		s.bTeam = team;
		EXPECT_TRUE(ApplyEnemyDropProgressLimit(s, o));
	}

	// also with the progress limits off
	s.bTeam = ENEMY_TEAM;
	const_cast<GamePolicy*>(GCM->getGamePolicy())->enemy_drop_progress_limits = false;
	EXPECT_FALSE(ApplyEnemyDropProgressLimit(s, o));
	EXPECT_TRUE(EnemyDropBlockedByProgress(GLOCK_17));
	const_cast<GamePolicy*>(GCM->getGamePolicy())->enemy_drop_progress_limits = true;

	// picked up by the enemy (maybe the player's own): still drops (D9)
	OBJECTTYPE p = Gun(90);
	p.fFlags |= OBJECT_PICKED_UP;
	EXPECT_TRUE(ApplyEnemyDropProgressLimit(s, p));

	// dealers treat it like 0
	EXPECT_FALSE(DealerStockBlockedByProgress(ARMS_DEALER_TONY, GLOCK_17));
	EXPECT_FALSE(DealerStockBlockedByProgress(ARMS_DEALER_BOBBYR, GLOCK_17));
	SetGameProgress(0);
}


TEST_F(DropProgressTest, blockedAttachmentIsAlwaysRemoved)
{
	SetMinDrop(GLOCK_17, 0);
	SetMinDrop(LASERSCOPE, 0);
	const_cast<ItemModel*>(GCM->getItem(LASERSCOPE))->setEnemyDropBlocked(true);
	OBJECTTYPE o = ScopedGun();
	EXPECT_TRUE(ApplyDropProgressLimit(o, 100));
	EXPECT_EQ(o.usAttachItem[0], NOTHING);
	EXPECT_EQ(o.usItem, GLOCK_17);
}


TEST_F(DropProgressTest, pickingUpMarksTheObject)
{
	SetMinDrop(GLOCK_17, 50);
	SetGameProgress(0);
	auto enemy = SOLDIERTYPE{};
	enemy.bTeam = ENEMY_TEAM;
	auto merc = SOLDIERTYPE{};
	merc.bTeam = OUR_TEAM;

	// an enemy picks up the player's Glock: it may drop again
	OBJECTTYPE o = Gun(90);
	MarkObjectPickedUpBy(enemy, o);
	EXPECT_TRUE(o.fFlags & OBJECT_PICKED_UP);
	EXPECT_TRUE(ApplyEnemyDropProgressLimit(enemy, o));

	// a merc takes it: the mark goes away
	MarkObjectPickedUpBy(merc, o);
	EXPECT_FALSE(o.fFlags & OBJECT_PICKED_UP);
	EXPECT_FALSE(ApplyEnemyDropProgressLimit(enemy, o));

	// an unknown owner is never limited
	EXPECT_TRUE(ApplyEnemyDropProgressLimit(static_cast<SOLDIERTYPE const*>(nullptr), o));
	EXPECT_FALSE(ApplyEnemyDropProgressLimit(&enemy, o));
	SetGameProgress(0);
}




// docs/game-progress-plan.md
TEST_F(DropProgressTest, gameProgressFromVisitsAndQuests)
{
	ClearGameProgress();
	EXPECT_EQ(GameProgressPercentage(), 0);

	// SF_ALREADY_VISITED alone does not count
	UINT32 const a9Flags = SectorInfo[SGPSector(9, 1).AsByte()].uiFlags;
	SectorInfo[SGPSector(9, 1).AsByte()].uiFlags |= SF_ALREADY_VISITED;
	EXPECT_EQ(GameProgressPercentage(), 0);
	SectorInfo[SGPSector(9, 1).AsByte()].uiFlags = a9Flags;

	// a town sector (A9) and a wilderness sector (A1, not counted)
	SectorInfo[SGPSector(9, 1).AsByte()].uiFlags |= SF_PROGRESS_VISITED;
	SectorInfo[SGPSector(1, 1).AsByte()].uiFlags |= SF_PROGRESS_VISITED;
	EXPECT_EQ(GameProgressVisitedEntries(), 1);
	EXPECT_EQ(GameProgressPercentage(), 2);

	// N4: Meduna town sector and SAM site, counts twice (P1)
	SectorInfo[SGPSector(4, 14).AsByte()].uiFlags |= SF_PROGRESS_VISITED;
	EXPECT_EQ(GameProgressVisitedEntries(), 3);
	EXPECT_EQ(GameProgressPercentage(), 6);

	// quests: done counts, in progress does not, 8 and 9 never (P2)
	gubQuest[QUEST_DELIVER_LETTER] = QUESTDONE;
	gubQuest[QUEST_FOOD_ROUTE]     = QUESTINPROGRESS;
	gubQuest[QUEST_HELD_IN_ALMA]   = QUESTDONE;
	gubQuest[QUEST_INTERROGATION]  = QUESTDONE;
	EXPECT_EQ(GameProgressDoneQuests(), 1);
	EXPECT_EQ(GameProgressPercentage(), 8);

	// a done quest set back to "in progress": its progress stays
	gubQuest[QUEST_DELIVER_LETTER] = QUESTINPROGRESS;
	EXPECT_EQ(GameProgressPercentage(), 8);
	// ... a new visit meanwhile still adds
	SectorInfo[SGPSector(10, 1).AsByte()].uiFlags |= SF_PROGRESS_VISITED;
	EXPECT_EQ(GameProgressPercentage(), 10);
	// ... and completing the same quest again gives nothing more
	gubQuest[QUEST_DELIVER_LETTER] = QUESTDONE;
	EXPECT_EQ(GameProgressDoneQuests(), 1);
	EXPECT_EQ(GameProgressPercentage(), 10);

	// EndQuest path notes the quest even if it is never read as done
	GameProgressNoteQuestDone(QUEST_FOOD_ROUTE);
	GameProgressNoteQuestDone(QUEST_INTERROGATION); // not counted
	EXPECT_EQ(GameProgressDoneQuests(), 2);
	EXPECT_EQ(GameProgressPercentage(), 12);

	// everything: 40 entries + 21 quests = 122%, capped at 100
	int quests = 0;
	for (UINT8 q = 0; q < MAX_QUESTS; ++q)
	{
		if (IsGameProgressQuest(q)) { gubQuest[q] = QUESTDONE; ++quests; }
	}
	EXPECT_EQ(quests, 21);
	EXPECT_EQ(GameProgressDoneQuests(), 21);
	for (auto const& town : GCM->getTowns())
		for (UINT8 const id : town.second->sectorIDs) SectorInfo[id].uiFlags |= SF_PROGRESS_VISITED;
	for (auto const* const sam : GCM->getSamSites()) SectorInfo[sam->sectorId].uiFlags |= SF_PROGRESS_VISITED;
	EXPECT_EQ(GameProgressVisitedEntries(), 40);
	EXPECT_EQ(GameProgressPercentage(), 100);

	ClearGameProgress();
}


// docs/game-progress-plan.md, stage 3: dealer stock follows minDropProgress
TEST_F(DropProgressTest, dealerStockFollowsMinDropProgress)
{
	UINT8 const savedHighest = gStrategicStatus.ubHighestProgress;
	gStrategicStatus.ubHighestProgress = 0; // old coolness window: 0-1 (Bobby Ray)
	SetMinDrop(LASERSCOPE, 30);
	SetMinDrop(FIRSTAIDKIT, 10);

	// Bobby Ray: up to the progress; Tony and Devin: up to progress + 10
	SetGameProgress(28);
	EXPECT_TRUE(DealerStockBlockedByProgress(ARMS_DEALER_BOBBYR, LASERSCOPE));
	EXPECT_FALSE(DealerStockBlockedByProgress(ARMS_DEALER_TONY, LASERSCOPE));
	EXPECT_FALSE(DealerStockBlockedByProgress(ARMS_DEALER_DEVIN, LASERSCOPE));
	EXPECT_EQ(ChanceOfItemTransaction(ARMS_DEALER_BOBBYR, LASERSCOPE, DEALER_BUYING, FALSE), 0);
	for (ArmsDealerID const d : { ARMS_DEALER_BOBBYR, ARMS_DEALER_TONY, ARMS_DEALER_DEVIN })
	{
		EXPECT_FALSE(DealerStockBlockedByProgress(d, FIRSTAIDKIT)) << int(d);
	}
	SetMinDrop(LASERSCOPE, 40);
	EXPECT_TRUE(DealerStockBlockedByProgress(ARMS_DEALER_TONY, LASERSCOPE));  // 40 > 28 + 10
	EXPECT_TRUE(DealerStockBlockedByProgress(ARMS_DEALER_DEVIN, LASERSCOPE));
	SetGameProgress(30);
	EXPECT_FALSE(DealerStockBlockedByProgress(ARMS_DEALER_TONY, LASERSCOPE)); // 40 <= 30 + 10
	EXPECT_FALSE(DealerStockBlockedByProgress(ARMS_DEALER_DEVIN, LASERSCOPE));
	EXPECT_TRUE(DealerStockBlockedByProgress(ARMS_DEALER_BOBBYR, LASERSCOPE));
	// every other dealer is limited too, without a lead
	EXPECT_TRUE(DealerStockBlockedByProgress(ARMS_DEALER_KEITH, LASERSCOPE));  // 40 > 30
	EXPECT_TRUE(DealerStockBlockedByProgress(ARMS_DEALER_FRANZ, LASERSCOPE));
	SetMinDrop(FIRSTAIDKIT, 30);
	EXPECT_FALSE(DealerStockBlockedByProgress(ARMS_DEALER_KEITH, FIRSTAIDKIT)); // 30 <= 30
	EXPECT_GT(ChanceOfItemTransaction(ARMS_DEALER_KEITH, FIRSTAIDKIT, DEALER_BUYING, FALSE), 0);
	SetMinDrop(FIRSTAIDKIT, 32);
	EXPECT_EQ(ChanceOfItemTransaction(ARMS_DEALER_KEITH, FIRSTAIDKIT, DEALER_BUYING, FALSE), 0);

	// reached: allowed although its coolness is above the old window
	SetMinDrop(LASERSCOPE, 30);
	EXPECT_FALSE(DealerStockBlockedByProgress(ARMS_DEALER_BOBBYR, LASERSCOPE));
	EXPECT_GT(GCM->getItem(LASERSCOPE)->getCoolness(), 1);
	EXPECT_GT(ChanceOfItemTransaction(ARMS_DEALER_BOBBYR, LASERSCOPE, DEALER_BUYING, FALSE), 0);

	// staple items are limited too
	SetMinDrop(FIRSTAIDKIT, 50);
	EXPECT_TRUE(DealerStockBlockedByProgress(ARMS_DEALER_TONY, FIRSTAIDKIT));
	EXPECT_EQ(ChanceOfItemTransaction(ARMS_DEALER_TONY, FIRSTAIDKIT, DEALER_BUYING, FALSE), 0);

	// switch off: the original coolness cut-off again
	const_cast<GamePolicy*>(GCM->getGamePolicy())->dealer_stock_progress_limits = false;
	SetGameProgress(0);
	EXPECT_FALSE(DealerStockBlockedByProgress(ARMS_DEALER_TONY, FIRSTAIDKIT));
	EXPECT_EQ(ChanceOfItemTransaction(ARMS_DEALER_BOBBYR, LASERSCOPE, DEALER_BUYING, FALSE), 0);
	const_cast<GamePolicy*>(GCM->getGamePolicy())->dealer_stock_progress_limits = true;

	SetMinDrop(LASERSCOPE, GCM->getItem(LASERSCOPE)->getCoolness() * 10);
	SetMinDrop(FIRSTAIDKIT, GCM->getItem(FIRSTAIDKIT)->getCoolness() * 10);
	ClearGameProgress();
	gStrategicStatus.ubHighestProgress = savedHighest;
}


// docs/game-progress-plan.md: each full 10% step is announced once
TEST_F(DropProgressTest, gameProgressMilestonesOnce)
{
	using Steps = std::vector<UINT8>;
	SetGameProgress(8);
	EXPECT_EQ(GameProgressNewMilestones(), Steps{});

	SetGameProgress(10); // clears the announced step too
	EXPECT_EQ(GameProgressNewMilestones(), Steps{ 10 });
	EXPECT_EQ(GameProgressNewMilestones(), Steps{});

	// a jump over several steps: all of them, in order
	SetGameProgress(32);
	EXPECT_EQ(GameProgressNewMilestones(), (Steps{ 10, 20, 30 }));

	// more progress below the next step: nothing; the next step: once
	gubQuest[QUEST_DELIVER_LETTER] = QUESTDONE; // 34
	EXPECT_EQ(GameProgressNewMilestones(), Steps{});
	gubQuest[QUEST_FOOD_ROUTE]       = QUESTDONE;
	gubQuest[QUEST_KILL_TERRORISTS]  = QUESTDONE;
	gubQuest[QUEST_KINGPIN_IDOL]     = QUESTDONE; // 40
	EXPECT_EQ(GameProgressPercentage(), 40);
	EXPECT_EQ(GameProgressNewMilestones(), Steps{ 40 });
	EXPECT_EQ(gStrategicStatus.ubGameProgressAnnounced, 40);

	// everything: up to 100, never above
	for (UINT8 q = 0; q < MAX_QUESTS; ++q)
	{
		if (IsGameProgressQuest(q)) gubQuest[q] = QUESTDONE;
	}
	for (auto const& town : GCM->getTowns())
		for (UINT8 const id : town.second->sectorIDs) SectorInfo[id].uiFlags |= SF_PROGRESS_VISITED;
	EXPECT_EQ(GameProgressNewMilestones(), (Steps{ 50, 60, 70, 80, 90, 100 }));
	EXPECT_EQ(GameProgressNewMilestones(), Steps{});

	ClearGameProgress();
}


// docs/game-progress-plan.md: no guaranteed deliveries; Jake's gas cans are normal
// stock, only after the Shank quest
TEST_F(DropProgressTest, jakeGasCansAfterTheShankQuest)
{
	UINT8 const savedFact = gubFact[FACT_ESTONI_REFUELLING_POSSIBLE];
	EXPECT_FALSE(GCM->getGamePolicy()->dealer_guaranteed_deliveries);
	SetGameProgress(0);

	gubFact[FACT_ESTONI_REFUELLING_POSSIBLE] = FALSE;
	EXPECT_EQ(ChanceOfItemTransaction(ARMS_DEALER_JAKE, GAS_CAN, DEALER_BUYING, FALSE), 0);
	gubFact[FACT_ESTONI_REFUELLING_POSSIBLE] = TRUE;
	EXPECT_GT(ChanceOfItemTransaction(ARMS_DEALER_JAKE, GAS_CAN, DEALER_BUYING, FALSE), 0);
	// only the fuel dealer: coolness 0 still keeps it away from the others
	EXPECT_EQ(ChanceOfItemTransaction(ARMS_DEALER_KEITH, GAS_CAN, DEALER_BUYING, FALSE), 0);
	// with its own minDropProgress
	SetMinDrop(GAS_CAN, 10);
	EXPECT_EQ(ChanceOfItemTransaction(ARMS_DEALER_JAKE, GAS_CAN, DEALER_BUYING, FALSE), 0);
	SetMinDrop(GAS_CAN, 0);

	// guaranteed deliveries back on: the original rule (coolness 0 = no normal stock)
	const_cast<GamePolicy*>(GCM->getGamePolicy())->dealer_guaranteed_deliveries = true;
	EXPECT_EQ(ChanceOfItemTransaction(ARMS_DEALER_JAKE, GAS_CAN, DEALER_BUYING, FALSE), 0);
	const_cast<GamePolicy*>(GCM->getGamePolicy())->dealer_guaranteed_deliveries = false;

	gubFact[FACT_ESTONI_REFUELLING_POSSIBLE] = savedFact;
	ClearGameProgress();
}


// Sort categories: assault rifles, rifles and sniper rifles apart (by the gun
// type), rocket rifles, LAW and the tank cannon with the launchers
TEST_F(GunStackTest, rifleSortCategories)
{
	UINT16 const sks = 17, m24 = 19, aug = 20;
	auto before = [](UINT16 a, UINT16 b) { return CompareItemsForSorting(a, b, 100, 100) < 0; };

	UINT16 const mp5k = 9, m870 = 31;

	// the whole order: melee, knives, guns by type, launchers, attachments,
	// ammo, armour, face, grenades, bombs, medkits, kits, misc, keys, money,
	// thrown
	UINT16 const order[] = { BRASS_KNUCKLES, COMBAT_KNIFE, THROWING_KNIFE,
		GLOCK_17, GLOCK_18, mp5k, m870, aug, sks, m24, MINIMI, GLAUNCHER,
		SILENCER, CLIP9_15, KEVLAR_VEST, GASMASK, STUN_GRENADE, TNT,
		FIRSTAIDKIT, TOOLKIT, DETONATOR, KEY_1, MONEY, ROCK };
	for (size_t i = 0; i + 1 < std::size(order); ++i)
	{
		EXPECT_TRUE(before(order[i], order[i + 1])) << order[i] << " before " << order[i + 1];
	}

	EXPECT_TRUE(before(GLOCK_17, aug));
	EXPECT_TRUE(before(aug, sks));          // assault rifles first
	EXPECT_TRUE(before(sks, m24));          // then rifles, then sniper rifles
	EXPECT_TRUE(before(m24, MINIMI));       // then machine guns
	EXPECT_TRUE(before(MINIMI, ROCKET_RIFLE));
	EXPECT_TRUE(before(MINIMI, AUTO_ROCKET_RIFLE));
	EXPECT_TRUE(before(MINIMI, TANK_CANNON));
	EXPECT_TRUE(before(ROCKET_RIFLE, CLIP9_15));    // launchers before ammo
	EXPECT_TRUE(before(GLAUNCHER, CLIP9_15));

	// heavy weapons: "Heavy Weapon" in the description box
	EXPECT_TRUE(IsHeavyWeapon(GLAUNCHER));
	EXPECT_TRUE(IsHeavyWeapon(ROCKET_RIFLE));
	EXPECT_TRUE(IsHeavyWeapon(AUTO_ROCKET_RIFLE));
	EXPECT_TRUE(IsHeavyWeapon(TANK_CANNON));
	EXPECT_FALSE(IsHeavyWeapon(aug));
	EXPECT_FALSE(IsHeavyWeapon(MINIMI));
	EXPECT_FALSE(IsHeavyWeapon(CLIP9_15));

	// rocket rifles are still sold used, like the other rifles
	EXPECT_EQ(CanDealerItemBeSoldUsed(ROCKET_RIFLE), CanDealerItemBeSoldUsed(aug));
}

#endif
