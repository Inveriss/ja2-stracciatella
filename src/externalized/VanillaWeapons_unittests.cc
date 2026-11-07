#ifdef WITH_UNITTESTS
#include "gtest/gtest.h"


#include "AmmoTypeModel.h"
#include "CalibreModel.h"
#include "DefaultContentManager.h"
#include "DefaultContentManagerUT.h"
#include "GameInstance.h"
#include "GamePolicy.h"
#include "Items.h"
#include "MagazineModel.h"
#include "Weapons.h"
#include "WeaponModels.h"
#include <memory>
#include <utility>
#include <vector>

TEST(Items, weaponsLoading)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm != NULL);
	ASSERT_TRUE(cm->loadGameData());
	EXPECT_TRUE(cm->getWeaponByName("MP5K") != NULL);
	EXPECT_TRUE(cm->getWeapon(9 /* MP5K */) != NULL);
	EXPECT_EQ(cm->getWeaponByName("MP5K"), cm->getWeapon(9 /* MP5K */));
	delete cm;
}

TEST(Items, bug120_cawsAmmo)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());

	// test SAP clip parameters
	const MagazineModel* sapClip = cm->getMagazineByName("CLIPCAWS_10_SAP");
	EXPECT_EQ(sapClip->calibre->internalName, ST::string("AMMOCAWS"));
	EXPECT_EQ(sapClip->ammoType->internalName, ST::string("AMMO_SUPER_AP"));

	// test FLECH clip parameters
	const MagazineModel* flechClip = cm->getMagazineByName("CLIPCAWS_10_FLECH");
	EXPECT_EQ(flechClip->calibre->internalName, ST::string("AMMOCAWS"));
	EXPECT_EQ(flechClip->ammoType->internalName, ST::string("AMMO_BUCKSHOT"));

	delete cm;
}

TEST(Items, bug120_12gAmmo)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());

	// test SAP clip parameters
	const MagazineModel* clip = cm->getMagazineByName("CLIP12G_7");
	EXPECT_EQ(clip->calibre->internalName,           ST::string("AMMO12G"));
	EXPECT_EQ(clip->ammoType->internalName,          ST::string("AMMO_REGULAR"));

	// test FLECH clip parameters
	const MagazineModel* clipBuckshot = cm->getMagazineByName("CLIP12G_7_BUCKSHOT");
	EXPECT_EQ(clipBuckshot->calibre->internalName,           ST::string("AMMO12G"));
	EXPECT_EQ(clipBuckshot->ammoType->internalName,          ST::string("AMMO_BUCKSHOT"));

	delete cm;
}

TEST(Items, bug120_cawsDefaultMag)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());

	GCM = cm;

	const WeaponModel *caws = cm->getWeaponByName("CAWS");
	ASSERT_TRUE(caws != NULL);

	const MagazineModel* flechClip = cm->getMagazineByName("CLIPCAWS_10_FLECH");
	const MagazineModel* sapClip = cm->getMagazineByName("CLIPCAWS_10_SAP");
	ASSERT_TRUE(flechClip != NULL);
	ASSERT_TRUE(sapClip != NULL);

	EXPECT_EQ(DefaultMagazine(caws->getItemIndex()), flechClip->getItemIndex());

	EXPECT_EQ(FindReplacementMagazine(sapClip->calibre, 10, AMMO_BUCKSHOT), flechClip->getItemIndex());
	EXPECT_EQ(FindReplacementMagazine(sapClip->calibre, 10, AMMO_SUPER_AP), sapClip->getItemIndex());

	delete cm;
}

TEST(Items, bug120_spas15DefaultMag)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());

	GCM = cm;

	const WeaponModel *spas15 = cm->getWeaponByName("SPAS15");
	ASSERT_TRUE(spas15 != NULL);

	const MagazineModel* clipBuckshot = cm->getMagazineByName("CLIP12G_7_BUCKSHOT");
	const MagazineModel* clip = cm->getMagazineByName("CLIP12G_7");
	ASSERT_TRUE(clipBuckshot != NULL);
	ASSERT_TRUE(clip != NULL);

	EXPECT_EQ(DefaultMagazine(spas15->getItemIndex()), clipBuckshot->getItemIndex());

	EXPECT_EQ(FindReplacementMagazine(clip->calibre, 7, AMMO_BUCKSHOT), clipBuckshot->getItemIndex());
	EXPECT_EQ(FindReplacementMagazine(clip->calibre, 7, AMMO_REGULAR),  clip->getItemIndex());

	delete cm;
}

TEST(Items, ValidLaunchable)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());

	GCM = cm;

	EXPECT_TRUE(ValidLaunchable(MORTAR_SHELL, MORTAR));
	EXPECT_FALSE(ValidLaunchable(MORTAR_SHELL, MORTAR_SHELL));
	EXPECT_FALSE(ValidLaunchable(MORTAR_SHELL, TANK_CANNON));
	EXPECT_FALSE(ValidLaunchable(MORTAR, MORTAR_SHELL));
	EXPECT_TRUE(ValidLaunchable(GL_HE_GRENADE, GLAUNCHER));
	EXPECT_TRUE(ValidLaunchable(GL_HE_GRENADE, UNDER_GLAUNCHER));

	// Check if the function handles some random garbage input
	EXPECT_FALSE(ValidLaunchable(BATTERIES, WINE));
	EXPECT_FALSE(ValidLaunchable(0xf123, 0x97b2));

	delete cm;
}

TEST(Items, GetLauncherFromLaunchable)
{
	DefaultContentManager * cm = DefaultContentManagerUT::createDefaultCMForTesting();
	ASSERT_TRUE(cm->loadGameData());

	GCM = cm;

	EXPECT_EQ(GetLauncherFromLaunchable(GL_TEARGAS_GRENADE), GLAUNCHER);
	EXPECT_EQ(GetLauncherFromLaunchable(MORTAR_SHELL), MORTAR);
	EXPECT_EQ(GetLauncherFromLaunchable(TANK_SHELL), TANK_CANNON);
	EXPECT_EQ(GetLauncherFromLaunchable(TANK_CANNON), NOTHING);

	// Check if the function handles some random garbage input
	EXPECT_EQ(GetLauncherFromLaunchable(G11), NOTHING);
	EXPECT_EQ(GetLauncherFromLaunchable(0xe941), NOTHING);

	delete cm;
}

TEST(Items, ValidAttachment)
{
	std::unique_ptr<DefaultContentManager> cm(DefaultContentManagerUT::createDefaultCMForTesting());
	ASSERT_TRUE(cm->loadGameData());
	auto const oldGCM = std::exchange(GCM, cm.release());

	bool& extra_attachments = const_cast<GamePolicy *>(GCM->getGamePolicy())->extra_attachments;

	extra_attachments = false;
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::DETONATOR, ITEMDEFINE::HMX));
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::DETONATOR, ITEMDEFINE::TNT));
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::CHEWING_GUM, ITEMDEFINE::FUMBLE_PAK));
	EXPECT_FALSE(ValidAttachment(ITEMDEFINE::UVGOGGLES, ITEMDEFINE::SPECTRA_HELMET));;
	EXPECT_FALSE(ValidAttachment(ITEMDEFINE::SUNGOGGLES, ITEMDEFINE::SPECTRA_HELMET));;
	EXPECT_FALSE(ValidAttachment(ITEMDEFINE::ADRENALINE_BOOSTER, ITEMDEFINE::KEVLAR_LEGGINGS_Y));
	EXPECT_FALSE(ValidAttachment(ITEMDEFINE::AUTO_ROCKET_RIFLE, ITEMDEFINE::BRASS_KNUCKLES));
	EXPECT_FALSE(ValidAttachment(0xf083, 0x8c12)); // Random junk crashes the old version of ValidAttachment

	// Next test relies on a certain order of the vests
	static_assert(ITEMDEFINE::SPECTRA_VEST_Y - ITEMDEFINE::FLAK_JACKET == 8);
	int count = 0;
	for (int i = ITEMDEFINE::FLAK_JACKET; i <= ITEMDEFINE::SPECTRA_VEST_Y; ++i)
	{
		if (ValidAttachment(ITEMDEFINE::CERAMIC_PLATES, i)) ++count;
	}
	EXPECT_EQ(count, 9);

	extra_attachments = true;
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::DETONATOR, ITEMDEFINE::HMX));
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::DETONATOR, ITEMDEFINE::TNT));
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::CHEWING_GUM, ITEMDEFINE::FUMBLE_PAK));
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::UVGOGGLES, ITEMDEFINE::SPECTRA_HELMET));;
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::UVGOGGLES, ITEMDEFINE::KEVLAR_HELMET_18));;
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::SUNGOGGLES, ITEMDEFINE::SPECTRA_HELMET));;
	EXPECT_TRUE(ValidAttachment(ITEMDEFINE::ADRENALINE_BOOSTER, ITEMDEFINE::KEVLAR_LEGGINGS_Y));
	EXPECT_FALSE(ValidAttachment(ITEMDEFINE::SUNGOGGLES, ITEMDEFINE::SPECTRA_VEST));;
	EXPECT_FALSE(ValidAttachment(ITEMDEFINE::AUTO_ROCKET_RIFLE, ITEMDEFINE::BRASS_KNUCKLES));
	EXPECT_FALSE(ValidAttachment(0xf083, 0x8c12));

	delete GCM;
	GCM = oldGCM;
}

TEST(Items, CompatibleFaceItem)
{
	EXPECT_TRUE(CompatibleFaceItem(ITEMDEFINE::NIGHTGOGGLES, ITEMDEFINE::EXTENDEDEAR));
	EXPECT_TRUE(CompatibleFaceItem(ITEMDEFINE::EXTENDEDEAR, ITEMDEFINE::NIGHTGOGGLES));
	EXPECT_FALSE(CompatibleFaceItem(ITEMDEFINE::EXTENDEDEAR, ITEMDEFINE::EXTENDEDEAR));
	EXPECT_TRUE(CompatibleFaceItem(ITEMDEFINE::WALKMAN, ITEMDEFINE::GASMASK));
	EXPECT_FALSE(CompatibleFaceItem(ITEMDEFINE::UVGOGGLES, ITEMDEFINE::RDX));
	EXPECT_TRUE(CompatibleFaceItem(0xda83, NOTHING)); // item2 == NOTHING is a special case
	EXPECT_FALSE(CompatibleFaceItem(0x75e4, 0xcafe));
	for (int i = 0; i <= 0xffff; ++i)
	{
		EXPECT_FALSE(CompatibleFaceItem(i, ITEMDEFINE::STEEL_HELMET));
	}
}

// docs/magazine-size-variants-plan.md: a gun loaded with a magazine of another
// size remembers the magazine of its own size with the same ammo
TEST(Items, magazineVariantOriginalPairs)
{
	std::unique_ptr<DefaultContentManager> cm(DefaultContentManagerUT::createDefaultCMForTesting());
	ASSERT_TRUE(cm->loadGameData());
	auto const oldGCM = std::exchange(GCM, cm.get());

	auto const* const ap5  = cm->getMagazineByName("CLIP762N_5_AP");
	auto const* const ap20 = cm->getMagazineByName("CLIP762N_20_AP");
	auto const* const hp5  = cm->getMagazineByName("CLIP762N_5_HP");
	auto const* const hp20 = cm->getMagazineByName("CLIP762N_20_HP");
	auto const* const r9   = cm->getMagazineByName("CLIP9_15");

	EXPECT_EQ(FindMagazineVariant(*ap20, 5),  ap5->getItemIndex());  // 20 into a 5-round gun
	EXPECT_EQ(FindMagazineVariant(*ap5, 20),  ap20->getItemIndex()); // 5 into a 20-round gun
	EXPECT_EQ(FindMagazineVariant(*hp20, 5),  hp5->getItemIndex());  // HP stays HP
	EXPECT_EQ(FindMagazineVariant(*hp5, 20),  hp20->getItemIndex());
	EXPECT_EQ(FindMagazineVariant(*ap20, 20), ap20->getItemIndex()); // same size: itself
	EXPECT_EQ(FindMagazineVariant(*r9, 99),   r9->getItemIndex());   // no variant: itself

	GCM = oldGCM;
}

TEST(Items, magazineVariantIndicesDoNotMatter)
{
	std::unique_ptr<DefaultContentManager> cm(DefaultContentManagerUT::createDefaultCMForTesting());
	ASSERT_TRUE(cm->loadGameData());
	auto const* const regular = cm->getMagazineByName("CLIP9_15");
	auto const* const ap      = cm->getMagazineByName("CLIP9_15_AP");

	auto mag = [&](uint16_t index, uint16_t capacity, MagazineModel const* type)
	{
		return std::make_unique<MagazineModel>(index, ST::string("TEST_MAG"), ST::string(), ST::string(), ST::string(),
			IC_AMMO, type->calibre, capacity, type->ammoType, false);
	};
	// three sizes far apart and out of order, plus a second 20 and a 20 of another ammo type
	auto const m30    = mag(3900, 30, regular);
	auto const m10    = mag(3500, 10, regular);
	auto const m20    = mag(3100, 20, regular);
	auto const m20b   = mag(3200, 20, regular);
	auto const m20ap  = mag(3050, 20, ap);
	std::vector<const MagazineModel*> const mags{ m30.get(), m20b.get(), m10.get(), m20ap.get(), m20.get() };

	EXPECT_EQ(FindMagazineVariant(mags, *m30, 10), 3500);  // biggest into the smallest gun
	EXPECT_EQ(FindMagazineVariant(mags, *m10, 30), 3900);  // and back
	EXPECT_EQ(FindMagazineVariant(mags, *m10, 20), 3100);  // same ammo type, lowest index
	EXPECT_EQ(FindMagazineVariant(mags, *m30, 20), 3100);
	EXPECT_EQ(FindMagazineVariant(mags, *m20ap, 10), 3050); // no AP 10: the loaded one itself
}

TEST(Items, magazineDuplicates)
{
	std::unique_ptr<DefaultContentManager> cm(DefaultContentManagerUT::createDefaultCMForTesting());
	ASSERT_TRUE(cm->loadGameData());

	// the original data has none
	EXPECT_TRUE(MagazineModel::describeDuplicates(cm->getMagazines()).empty());

	auto const* const regular = cm->getMagazineByName("CLIP9_15");
	auto const* const ap      = cm->getMagazineByName("CLIP9_15_AP");
	auto mag = [&](uint16_t index, const char* name, uint16_t capacity, MagazineModel const* type)
	{
		return std::make_unique<MagazineModel>(index, ST::string(name), ST::string(), ST::string(), ST::string(),
			IC_AMMO, type->calibre, capacity, type->ammoType, false);
	};
	auto const high = mag(3200, "MAG_HIGH", 20, regular);
	auto const low  = mag(3100, "MAG_LOW",  20, regular);
	auto const apm  = mag(3150, "MAG_AP",   20, ap);      // other ammo type: no duplicate
	auto const big  = mag(3300, "MAG_BIG",  30, regular); // other capacity: no duplicate
	std::vector<const MagazineModel*> const mags{ high.get(), apm.get(), low.get(), big.get() };

	auto const messages = MagazineModel::describeDuplicates(mags);
	ASSERT_EQ(messages.size(), 1u);
	EXPECT_NE(messages[0].find("'MAG_HIGH' (3200)"), -1);
	EXPECT_NE(messages[0].find("'MAG_LOW' (3100)"), -1);
}

TEST(Items, Invalid_ItemIndex_Exception)
{
	std::unique_ptr<DefaultContentManager> cm(DefaultContentManagerUT::createDefaultCMForTesting());
	ASSERT_TRUE(cm->loadGameData());
	auto const oldGCM = std::exchange(GCM, cm.release());

	EXPECT_NO_THROW(GCM->getItem(MAXITEMS - 1));
	for (uint16_t i = UINT16_MAX; i >= MAXITEMS; --i)
	{
		EXPECT_THROW(GCM->getItem(i), std::out_of_range);
	}

	EXPECT_NO_THROW(GCM->getItem(57000, ItemSystem::nothrow));
	EXPECT_EQ(GCM->getItem(38000, ItemSystem::nothrow), nullptr);

	delete GCM;
	GCM = oldGCM;
}

#endif
