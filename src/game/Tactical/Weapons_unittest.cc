// -*-coding: utf-8-unix;-*-

#include "gtest/gtest.h"

#include "Animation_Control.h"
#include "Animation_Data.h"
#include "DefaultContentManager.h"
#include "DefaultContentManagerUT.h"
#include "GameInstance.h"
#include "Isometric_Utils.h"
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

	// Ready time: a grenade launcher's own, and the underslung one's own, not
	// the gun's
	{
		auto s = SOLDIERTYPE{};
		s.inv[HANDPOS].usItem = GLAUNCHER;
		EXPECT_EQ(GetAPsToReadyWeapon(&s, s.usAnimState), 3);

		s.inv[HANDPOS].usItem = cm->getWeaponByName("AK74")->getItemIndex();
		s.inv[HANDPOS].usAttachItem[0] = UNDER_GLAUNCHER;
		EXPECT_EQ(GetAPsToReadyWeapon(&s, s.usAnimState), 2);
		s.bWeaponMode = WM_ATTACHED;
		EXPECT_EQ(GetAPsToReadyWeapon(&s, s.usAnimState), 3);
	}

	auto cannon = OBJECTTYPE{};
	cannon.usItem = TANK_CANNON;
	EXPECT_EQ(BaseAPsToShootOrStab(cannon), 13);

	auto minimi = OBJECTTYPE{};
	minimi.usItem = MINIMI;
	EXPECT_EQ(BaseAPsToShootOrStab(minimi), 6);
	EXPECT_EQ(BaseAPsToShootOrStab(minimi) + CalcAPsToBurst(minimi, WM_BURST_SHORT), 10);

	delete cm;
}

TEST(WeaponsTest, NewTargetAPCharge)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());
	GCM = cm;

	GridNo const target = 100;
	auto s = SOLDIERTYPE{};
	s.inv[HANDPOS].usItem = MINIMI;
	s.sLastTarget = NOWHERE;

	// Weapon lowered: raising it is part of aiming at the target
	s.usAnimState = STANDING;
	EXPECT_TRUE(IsWeaponLoweredForAttack(s));
	EXPECT_FALSE(ChargesAPsForNewTarget(s, target));

	// Weapon raised, another target
	s.usAnimState = AIM_RIFLE_STAND;
	EXPECT_FALSE(IsWeaponLoweredForAttack(s));
	EXPECT_TRUE(ChargesAPsForNewTarget(s, target));

	// Weapon raised, the same target
	s.sLastTarget = target;
	EXPECT_FALSE(ChargesAPsForNewTarget(s, target));

	// The gun fires in an attack that began with the weapon lowered
	s.sLastTarget = NOWHERE;
	s.fRaisingWeaponForAttack = true;
	EXPECT_FALSE(ChargesAPsForNewTarget(s, target));

	// Raised with the Q key: the first target is free
	s.fRaisingWeaponForAttack = false;
	s.fRaisedWithoutTarget = true;
	EXPECT_FALSE(ChargesAPsForNewTarget(s, target));
	s.fRaisedWithoutTarget = false;

	// Nor do the grenade launchers, with the weapon raised
	s.fRaisingWeaponForAttack = false;
	s.inv[HANDPOS].usItem = GLAUNCHER;
	EXPECT_FALSE(ChargesAPsForNewTarget(s, target));
	s.inv[HANDPOS].usItem = MINIMI;
	s.inv[HANDPOS].usAttachItem[0] = UNDER_GLAUNCHER;
	s.bWeaponMode = WM_ATTACHED;
	EXPECT_FALSE(ChargesAPsForNewTarget(s, target));
	s.bWeaponMode = WM_NORMAL;
	EXPECT_TRUE(ChargesAPsForNewTarget(s, target));
	s.inv[HANDPOS].usAttachItem[0] = NOTHING;

	// A tank never pays for a new target, only for turning its turret
	s.fRaisingWeaponForAttack = false;
	s.ubBodyType = TANK_NE;
	EXPECT_FALSE(ChargesAPsForNewTarget(s, target));

	delete cm;
}


TEST(WeaponsTest, FixedThrowCost)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());
	GCM = cm;

	// every throwable item has its own entry
	EXPECT_EQ(cm->getItem(HAND_GRENADE)->getAPCostPerThrow(), 11);
	EXPECT_EQ(cm->getWeaponByName("ROCK")->getAPCostPerThrow(), 11);
	EXPECT_EQ(cm->getItem(MORTAR_SHELL)->getAPCostPerThrow(), 0);

	// the same cost whatever the thrower's dexterity and AP
	auto s = SOLDIERTYPE{};
	s.inv[HANDPOS].usItem = HAND_GRENADE;
	s.usAnimState = STANDING;
	s.sLastTarget = NOWHERE;
	for (INT8 const dexterity : { INT8{0}, INT8{50}, INT8{100} })
	{
		s.bDexterity = dexterity;
		EXPECT_EQ(MinAPsToThrow(s, NOWHERE, false), 11);
	}

	// no AP for a new target
	s.sLastTarget = 100;
	EXPECT_EQ(MinAPsToThrow(s, NOWHERE, false), 11);

	delete cm;
}
