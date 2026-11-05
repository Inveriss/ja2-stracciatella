// -*-coding: utf-8-unix;-*-

#include "gtest/gtest.h"

#include "DefaultContentManager.h"
#include "DefaultContentManagerUT.h"
#include "GameInstance.h"
#include "Item_Types.h"
#include "Points.h"
#include "Soldier_Control.h"
#include "WeaponModels.h"
#include "Weapons.h"


TEST(WeaponsTest, EffectiveArmour)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());
	GCM = cm;

	// Immaculate spectra vest
	auto obj = OBJECTTYPE{};
	obj.usItem =  SPECTRA_VEST_18;
	obj.bStatus[0] = 100;
	EXPECT_EQ(EffectiveArmour(&obj), 36);

	// Status should have influence on effective armour
	obj.bStatus[0] = 50;
	EXPECT_EQ(EffectiveArmour(&obj), 18);

	// With ceramic plates
	obj.usAttachItem[0] = CERAMIC_PLATES;
	obj.bAttachStatus[0] = 100;
	EXPECT_EQ(EffectiveArmour(&obj), 33);

	// Status of ceramic plates should have influence on effective armour
	obj.bAttachStatus[0] = 80;
	EXPECT_EQ(EffectiveArmour(&obj), 30);

	delete cm;
}

TEST(WeaponsTest, ExplosiveEffectiveArmour)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());
	GCM = cm;

	// Immaculate spectra vest
	auto obj = OBJECTTYPE{};
	obj.usItem = SPECTRA_VEST_18;
	obj.bStatus[0] = 100;
	EXPECT_EQ(ExplosiveEffectiveArmour(&obj), 36);

	// Status should have influence on effective armour
	obj.bStatus[0] = 50;
	EXPECT_EQ(ExplosiveEffectiveArmour(&obj), 18);

	// With ceramic plates
	obj.usAttachItem[0] = CERAMIC_PLATES;
	obj.bAttachStatus[0] = 100;
	EXPECT_EQ(ExplosiveEffectiveArmour(&obj), 33);

	// Status of ceramic plates should have influence on effective armour
	obj.bAttachStatus[0] = 80;
	EXPECT_EQ(ExplosiveEffectiveArmour(&obj), 30);

	// Immaculate flak jacket
	obj = OBJECTTYPE{};
	obj.usItem = FLAK_JACKET;
	obj.bStatus[0] = 100;
	EXPECT_EQ(ExplosiveEffectiveArmour(&obj), 30);

	// Status should have influence on effective armour
	obj.bStatus[0] = 50;
	EXPECT_EQ(ExplosiveEffectiveArmour(&obj), 15);

	// With ceramic plates
	obj.usAttachItem[0] = CERAMIC_PLATES;
	obj.bAttachStatus[0] = 100;
	EXPECT_EQ(ExplosiveEffectiveArmour(&obj), 30);

	// Status of ceramic plates should have influence on effective armour
	obj.bAttachStatus[0] = 80;
	EXPECT_EQ(ExplosiveEffectiveArmour(&obj), 27);

	delete cm;
}

TEST(WeaponsTest, ArmourPercent)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());
	GCM = cm;

	// Fully kitted out soldier without ceramic plates
	auto soldier = SOLDIERTYPE{};
	soldier.inv[0].usItem = SPECTRA_HELMET_18;
	soldier.inv[0].bStatus[0] = 100;
	soldier.inv[1].usItem = SPECTRA_VEST_18;
	soldier.inv[1].bStatus[0] = 100;
	soldier.inv[2].usItem = SPECTRA_LEGGINGS_18;
	soldier.inv[2].bStatus[0] = 100;
	EXPECT_EQ(ArmourPercent(&soldier), 85);

	// Fully kitted out soldier with ceramic plates
	soldier.inv[1].usAttachItem[0] = CERAMIC_PLATES;
	soldier.inv[1].bAttachStatus[0] = 100;
	EXPECT_EQ(ArmourPercent(&soldier), 105);

	soldier.inv[1].usItem = SPECTRA_VEST_Y;
	EXPECT_EQ(ArmourPercent(&soldier), 112);

	soldier.inv[1].usItem = FLAK_JACKET;
	EXPECT_EQ(ArmourPercent(&soldier), 71);

	soldier.inv[1].bStatus[0] = 80;
	soldier.inv[1].bAttachStatus[0] = 80;
	EXPECT_EQ(ArmourPercent(&soldier), 65);

	delete cm;
}

TEST(WeaponsTest, ArmourVersusExplosivesPercent)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());
	GCM = cm;

	// Fully kitted out soldier without ceramic plates
	auto soldier = SOLDIERTYPE{};
	soldier.inv[0].usItem = SPECTRA_HELMET_18;
	soldier.inv[0].bStatus[0] = 100;
	soldier.inv[1].usItem = SPECTRA_VEST_18;
	soldier.inv[1].bStatus[0] = 100;
	soldier.inv[2].usItem = SPECTRA_LEGGINGS_18;
	soldier.inv[2].bStatus[0] = 100;
	EXPECT_EQ(ArmourVersusExplosivesPercent(&soldier), 85);

	// Fully kitted out soldier with ceramic plates
	soldier.inv[1].usAttachItem[0] = CERAMIC_PLATES;
	soldier.inv[1].bAttachStatus[0] = 100;
	EXPECT_EQ(ArmourVersusExplosivesPercent(&soldier), 105);

	soldier.inv[1].usItem = SPECTRA_VEST_Y;
	EXPECT_EQ(ArmourVersusExplosivesPercent(&soldier), 105);

	soldier.inv[1].usItem = FLAK_JACKET;
	EXPECT_EQ(ArmourVersusExplosivesPercent(&soldier), 97);

	soldier.inv[1].bStatus[0] = 80;
	soldier.inv[1].bAttachStatus[0] = 80;
	EXPECT_EQ(ArmourVersusExplosivesPercent(&soldier), 85);

	delete cm;
}


TEST(WeaponsTest, FixedAPCosts)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());
	GCM = cm;

	auto rpk = OBJECTTYPE{};
	rpk.usItem = cm->getWeaponByName("RPK74")->getItemIndex();
	EXPECT_EQ(BaseAPsToShootOrStab(rpk), 6);
	EXPECT_EQ(CalcAPsToBurst(rpk, WM_BURST_SHORT), 4);
	EXPECT_EQ(CalcAPsToBurst(rpk, WM_BURST_LONG), 4);
	EXPECT_EQ(CalcAPsToBurst(rpk, WM_BURST_FULL), 4);
	EXPECT_EQ(CalcAPsToBurst(rpk, WM_NORMAL), 0);

	// Spring and Bolt: a fixed -1 AP for any status, a burst drops only once
	rpk.usAttachItem[0] = SPRING_AND_BOLT_UPGRADE;
	for (int const status : { 1, 100 })
	{
		rpk.bAttachStatus[0] = static_cast<INT8>(status);
		EXPECT_EQ(BaseAPsToShootOrStab(rpk), 5);
		EXPECT_EQ(BaseAPsToShootOrStab(rpk) + CalcAPsToBurst(rpk, WM_BURST_SHORT), 9);
	}

	// A weapon without a burst value: its cost + 4
	auto m24 = OBJECTTYPE{};
	m24.usItem = cm->getWeaponByName("M24")->getItemIndex();
	EXPECT_EQ(BaseAPsToShootOrStab(m24), 10);
	EXPECT_EQ(CalcAPsToBurst(m24, WM_BURST_SHORT), 4);

	auto cannon = OBJECTTYPE{};
	cannon.usItem = TANK_CANNON;
	EXPECT_EQ(BaseAPsToShootOrStab(cannon), 13);

	auto minimi = OBJECTTYPE{};
	minimi.usItem = MINIMI;
	EXPECT_EQ(BaseAPsToShootOrStab(minimi), 6);
	EXPECT_EQ(BaseAPsToShootOrStab(minimi) + CalcAPsToBurst(minimi, WM_BURST_SHORT), 10);

	delete cm;
}
