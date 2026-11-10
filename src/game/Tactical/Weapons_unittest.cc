// -*-coding: utf-8-unix;-*-

#include "gtest/gtest.h"

#include "Animation_Control.h"
#include "Animation_Data.h"
#include "DefaultContentManager.h"
#include "DefaultContentManagerUT.h"
#include "GameInstance.h"
#include "Isometric_Utils.h"
#include "Item_Types.h"
#include "ItemModel.h"
#include "Points.h"
#include "policy/GamePolicy.h"
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

	// A punch: brass knuckles and the crowbar their own cost, a bare fist 4 AP
	{
		auto o = OBJECTTYPE{};
		o.usItem = BRASS_KNUCKLES;
		EXPECT_EQ(BaseAPsToPunch(o), 5);
		o.usItem = CROWBAR;
		EXPECT_EQ(BaseAPsToPunch(o), 8);
		o.usItem = NOTHING;
		EXPECT_EQ(BaseAPsToPunch(o), 4);
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

TEST(WeaponsTest, DualBurstRulesAndCosts)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());
	GCM = cm;

	auto gun = [&](char const* const name, UINT8 const rounds)
	{
		auto o = OBJECTTYPE{};
		o.usItem          = cm->getWeaponByName(name)->getItemIndex();
		o.bGunStatus      = 100;
		o.ubGunShotsLeft  = rounds;
		return o;
	};

	auto s = SOLDIERTYPE{};
	s.inv[HANDPOS]       = gun("GLOCK_18", 15);
	s.inv[SECONDHANDPOS] = gun("MP5K", 30);
	s.bWeaponMode        = WM_BURST_SHORT;

	// Not in burst mode: no dual burst, though the guns could (AI, D9)
	s.bDoBurst = 0;
	EXPECT_FALSE(IsValidDualBurst(s));
	EXPECT_TRUE(CanDualBurst(s));

	// Two one-handed burst guns in burst mode
	s.bDoBurst = 1;
	EXPECT_TRUE(IsValidDualBurst(s));
	EXPECT_FALSE(IsValidSecondHandShot(&s));
	EXPECT_TRUE(IsValidSecondHandShotForReloadingPurposes(&s));

	// Each gun its own length (D3)
	s.bWeaponMode = WM_BURST_LONG;
	EXPECT_EQ(CalcDualBurstLength(s).first,  6);
	EXPECT_EQ(CalcDualBurstLength(s).second, 6);
	s.bWeaponMode = WM_BURST_FULL;
	EXPECT_EQ(CalcDualBurstLength(s).first,  15);
	EXPECT_EQ(CalcDualBurstLength(s).second, 30);
	s.inv[SECONDHANDPOS].ubGunShotsLeft = 7;
	EXPECT_EQ(CalcDualBurstLength(s).second, 7);
	s.inv[SECONDHANDPOS].ubGunShotsLeft = 30;

	// AP (D5): the higher of the two burst costs, Glock 18 10, MP5K 9
	s.bWeaponMode = WM_BURST_SHORT;
	EXPECT_EQ(BaseAPsToShootOrStab(s.inv[HANDPOS]) + CalcAPsToBurst(s.inv[HANDPOS], WM_BURST_SHORT), 10);
	EXPECT_EQ(BaseAPsToShootOrStab(s.inv[SECONDHANDPOS]) + CalcAPsToBurst(s.inv[SECONDHANDPOS], WM_BURST_SHORT), 9);
	EXPECT_EQ(CalcAPsToDualBurst(s, WM_BURST_SHORT), 10 - 6);
	std::swap(s.inv[HANDPOS], s.inv[SECONDHANDPOS]);
	EXPECT_EQ(CalcAPsToDualBurst(s, WM_BURST_SHORT), 10 - 6);
	std::swap(s.inv[HANDPOS], s.inv[SECONDHANDPOS]);

	// First Aim of two guns: the higher one (Glock 18 0, MP5K 1, MAC-10 2),
	// in a dual burst and in a dual shot
	EXPECT_EQ(GetAPsToReadyWeapon(&s, s.usAnimState), 1);
	s.inv[SECONDHANDPOS] = gun("MAC10", 30);
	EXPECT_EQ(GetAPsToReadyWeapon(&s, s.usAnimState), 2);
	s.bDoBurst = 0;
	EXPECT_TRUE(IsValidSecondHandShot(&s));
	EXPECT_EQ(GetAPsToReadyWeapon(&s, s.usAnimState), 2);
	s.bDoBurst = 1;
	s.inv[SECONDHANDPOS] = gun("MP5K", 30);

	// A gun without that burst mode, a two-handed gun, an empty or broken gun
	s.inv[SECONDHANDPOS] = gun("GLOCK_17", 15);
	EXPECT_FALSE(IsValidDualBurst(s));
	EXPECT_TRUE(IsValidSecondHandShotForReloadingPurposes(&s));
	s.inv[SECONDHANDPOS] = gun("AK74", 30);
	EXPECT_FALSE(IsValidDualBurst(s));
	s.inv[SECONDHANDPOS] = gun("MP5K", 0);
	EXPECT_FALSE(IsValidDualBurst(s));
	s.inv[SECONDHANDPOS] = gun("MP5K", 30);
	s.inv[SECONDHANDPOS].bGunStatus = 0;
	EXPECT_FALSE(IsValidDualBurst(s));
	s.inv[SECONDHANDPOS] = gun("MP5K", 30);
	EXPECT_TRUE(IsValidDualBurst(s));

	// Bullets of the burst; a jammed gun's shots are taken out (D4)
	s.ubDualBurstLength[0] = 3;
	s.ubDualBurstLength[1] = 3;
	s.ubDualBurstShots[0]  = 1;
	s.ubDualBurstShots[1]  = 0;
	s.ubBurstLength        = 6;
	EXPECT_EQ(DualBurstBulletsLeft(s), 5);
	s.bBulletsLeft = 6; // one bullet still in flight
	DropDualBurstHand(s, SECONDHANDPOS);
	EXPECT_EQ(s.ubDualBurstLength[1], 0);
	EXPECT_EQ(s.ubBurstLength, 3);
	EXPECT_EQ(s.bBulletsLeft, 3);
	EXPECT_EQ(DualBurstBulletsLeft(s), 2);
	s.inv[HANDPOS].ubGunAmmoType = AMMO_BUCKSHOT;
	EXPECT_EQ(DualBurstBulletsLeft(s), 2 * NUM_BUCKSHOT_PELLETS);

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


// docs/weapon-attachments-plan.md, stage 3: the original attachments, which give
// no effects in the data, behave exactly as the original formulas
TEST(WeaponsTest, OriginalAttachmentEffects)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());
	GCM = cm;
	GamePolicy const& p = *cm->getGamePolicy();
	auto const statusMod = [](int x) { return x >= 85 ? 100 : x * 100 / 85; };
	auto const scaled = [&](int bonus, int status)
	{
		int const s = statusMod(status);
		return s > 50 ? bonus * (s - 50) / 50 : -bonus * (50 - s) / 50;
	};

	WeaponModel const* const glock = cm->getWeaponByName("GLOCK_17");
	WeaponModel const* const ak    = cm->getWeaponByName("AK74");
	for (int status = 1; status <= 100; ++status)
	{
		INT8 const st = static_cast<INT8>(status);

		auto gun = OBJECTTYPE{};
		gun.usItem = glock->getItemIndex();
		gun.usAttachItem[5] = SILENCER;            gun.bAttachStatus[5] = st;
		gun.usAttachItem[9] = GUN_BARREL_EXTENDER; gun.bAttachStatus[9] = st;
		gun.usAttachItem[0] = LASERSCOPE;          gun.bAttachStatus[0] = st;
		EXPECT_EQ(GunRange(gun), glock->usRange + p.range_bonus_barrel_extender * statusMod(status) / 100 - p.range_penalty_silencer) << status;
		int const v = glock->ubAttackVolume;
		EXPECT_EQ(GunAttackVolume(gun), 1 + ((100 - statusMod(status)) / (100 / (v - 1)))) << status;
		EXPECT_TRUE(HasLaserScope(gun));
		EXPECT_EQ(GunLaserScopeBonus(gun), scaled(p.aim_bonus_laserscope, status)) << status;

		auto rifle = OBJECTTYPE{};
		rifle.usItem = ak->getItemIndex();
		rifle.usAttachItem[19] = BIPOD;       rifle.bAttachStatus[19] = st;
		rifle.usAttachItem[3]  = SNIPERSCOPE; rifle.bAttachStatus[3]  = st;
		EXPECT_TRUE(HasBipod(rifle));
		EXPECT_TRUE(HasSniperScope(rifle));
		EXPECT_EQ(GunBipodDisplayBonus(rifle), scaled(20, status)) << status;
		EXPECT_EQ(GunSniperScopeDisplayBonus(rifle), scaled(20, status)) << status;
		EXPECT_EQ(GunRange(rifle), ak->usRange);
		EXPECT_EQ(GunAttackVolume(rifle), ak->ubAttackVolume);
		EXPECT_FALSE(HasLaserScope(rifle));
		EXPECT_EQ(GunShotsPerBurst(rifle, WM_BURST_SHORT), ak->ubShotsPerShortBurst);
	}

	delete cm;
}


// stage 3: effects given in the data replace the family default
TEST(WeaponsTest, AttachmentEffectsFromData)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());
	GCM = cm;
	WeaponModel const* const ak = cm->getWeaponByName("AK74");

	// a "better silencer": half the noise, no range penalty, 1 shot more per burst, 2 AP less
	AttachmentEffects e;
	e.attackVolumePercent = 50;
	e.rangeBonus          = 0;
	e.burstShotsBonus     = 1;
	e.apPerShotChange     = -2;
	const_cast<ItemModel*>(cm->getItem(SILENCER))->setAttachmentEffects(e);

	auto rifle = OBJECTTYPE{};
	rifle.usItem = ak->getItemIndex();
	EXPECT_EQ(BaseAPsToShootOrStab(rifle), ak->ubAPCostPerSingleShot);
	rifle.usAttachItem[11] = SILENCER;
	rifle.bAttachStatus[11] = 100;

	EXPECT_EQ(GunRange(rifle), ak->usRange);
	EXPECT_EQ(GunAttackVolume(rifle), ak->ubAttackVolume / 2);
	EXPECT_EQ(GunShotsPerBurst(rifle, WM_BURST_SHORT), ak->ubShotsPerShortBurst + 1);
	EXPECT_EQ(BaseAPsToShootOrStab(rifle), ak->ubAPCostPerSingleShot - 2);

	// a worse condition lets more noise through
	rifle.bAttachStatus[11] = 40;
	EXPECT_GT(GunAttackVolume(rifle), ak->ubAttackVolume / 2);
	EXPECT_LE(GunAttackVolume(rifle), ak->ubAttackVolume);

	// the family default where the data gives nothing: the range penalty is back
	e.rangeBonus.reset();
	const_cast<ItemModel*>(cm->getItem(SILENCER))->setAttachmentEffects(e);
	EXPECT_EQ(GunRange(rifle), ak->usRange - cm->getGamePolicy()->range_penalty_silencer);

	// the original attachment found by its family
	EXPECT_EQ(FindAttachmentOfFamily(rifle, "SILENCER"), 11);
	EXPECT_EQ(FindAttachmentOfFamily(rifle, "BIPOD"), ITEM_NOT_FOUND);

	delete cm;
}
