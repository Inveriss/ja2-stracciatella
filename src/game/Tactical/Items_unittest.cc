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


TEST_F(DropProgressTest, itemsWithoutThresholdAlwaysDrop)
{
	EXPECT_EQ(GCM->getItem(GLOCK_17)->getMinDropProgress(), 0);
	OBJECTTYPE o = ScopedGun();
	EXPECT_TRUE(ApplyDropProgressLimit(o, 0));
	EXPECT_EQ(o.usAttachItem[0], LASERSCOPE);
}


TEST_F(DropProgressTest, itemBelowItsThresholdDoesNotDrop)
{
	SetMinDrop(GLOCK_17, 30);
	OBJECTTYPE o = ScopedGun();
	EXPECT_FALSE(ApplyDropProgressLimit(o, 29));
	EXPECT_TRUE(ApplyDropProgressLimit(o, 30));
	EXPECT_TRUE(ApplyDropProgressLimit(o, 100));
	OBJECTTYPE nothing{};
	EXPECT_TRUE(ApplyDropProgressLimit(nothing, 0));
}


TEST_F(DropProgressTest, blockedAttachmentIsRemoved)
{
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
	UINT8 const saved = gStrategicStatus.ubHighestProgress;
	gStrategicStatus.ubHighestProgress = 20;

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
	gStrategicStatus.ubHighestProgress = 50;
	EXPECT_TRUE(ApplyEnemyDropProgressLimit(s, o));
	gStrategicStatus.ubHighestProgress = saved;
}


TEST_F(DropProgressTest, pickedUpObjectsAndSwitchOffAreNotLimited)
{
	SetMinDrop(GLOCK_17, 50);
	UINT8 const saved = gStrategicStatus.ubHighestProgress;
	gStrategicStatus.ubHighestProgress = 0;
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
	gStrategicStatus.ubHighestProgress = saved;
}


TEST_F(DropProgressTest, blockedItemForTheWeaponDropRecord)
{
	SetMinDrop(GLOCK_17, 50);
	UINT8 const saved = gStrategicStatus.ubHighestProgress;
	gStrategicStatus.ubHighestProgress = 49;
	EXPECT_TRUE(EnemyDropBlockedByProgress(GLOCK_17));
	EXPECT_FALSE(EnemyDropBlockedByProgress(G41));
	EXPECT_FALSE(EnemyDropBlockedByProgress(NOTHING));
	gStrategicStatus.ubHighestProgress = 50;
	EXPECT_FALSE(EnemyDropBlockedByProgress(GLOCK_17));

	gStrategicStatus.ubHighestProgress = 0;
	const_cast<GamePolicy*>(GCM->getGamePolicy())->enemy_drop_progress_limits = false;
	EXPECT_FALSE(EnemyDropBlockedByProgress(GLOCK_17));
	const_cast<GamePolicy*>(GCM->getGamePolicy())->enemy_drop_progress_limits = true;
	gStrategicStatus.ubHighestProgress = saved;
}

#endif
