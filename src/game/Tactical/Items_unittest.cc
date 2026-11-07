#ifdef WITH_UNITTESTS

// Gun stacks with a condition per unit, docs/gun-stacking-plan.md stage 1.

#include "DefaultContentManager.h"
#include "DefaultContentManagerUT.h"
#include "GameInstance.h"
#include "Item_Types.h"
#include "Items.h"
#include "LoadSaveData.h"
#include "LoadSaveObjectType.h"

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

#endif
