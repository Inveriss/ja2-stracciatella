#ifdef WITH_UNITTESTS

// docs/weapon-attachments-plan.md, stage 1: families, rules and exceptions

#include "CalibreModel.h"
#include "DefaultContentManagerUT.h"
#include "Exceptions.h"
#include "ItemModel.h"
#include "Items.h"
#include "MagazineModel.h"
#include "SkillCheck.h"
#include "WeaponAttachmentFamilyModel.h"
#include "WeaponModels.h"

#include "gtest/gtest.h"

#include <map>
#include <memory>

namespace {

using AttachmentFamilies = DefaultContentManagerUT::BaseTest;

std::unique_ptr<ItemModel> MakeAttachment(WeaponAttachmentFamilyModel const* family)
{
	auto item = std::make_unique<ItemModel>(
		7000, ST::string("TEST_ATTACHMENT"), ST::string(), ST::string(), ST::string(),
		IC_MISC, 0, INVALIDCURS,
		InventoryGraphicsModel(GraphicModel("interface/mdguns.sti", 0), GraphicModel("bigitems/gun00.sti", 0)),
		TilesetTileIndexModel(TileTypeDefines::GUNS, 1),
		0, 0, 0, 0, 0, 0, ITEM_ATTACHMENT);
	item->setAttachmentFamily(family);
	return item;
}

std::unique_ptr<WeaponAttachmentFamilyModel> ParseFamily(char const* json, std::map<ST::string, const CalibreModel*> const& calibres)
{
	return std::unique_ptr<WeaponAttachmentFamilyModel>(
		WeaponAttachmentFamilyModel::deserialize(JsonValue::deserialize(json), calibres));
}

}

TEST_F(AttachmentFamilies, originalAttachmentsHaveTheirFamily)
{
	for (char const* const name : { "SILENCER", "SNIPERSCOPE", "LASERSCOPE", "BIPOD", "DUCKBILL",
		"UNDER_GLAUNCHER", "SPRING_AND_BOLT_UPGRADE", "GUN_BARREL_EXTENDER" })
	{
		ItemModel const* const item = cm->getItemByName(name);
		ASSERT_NE(item->getAttachmentFamily(), nullptr) << name;
		EXPECT_EQ(item->getAttachmentFamily()->name, ST::string(name));
		EXPECT_EQ(item->getAttachmentLevel(), 1) << name;
	}
	EXPECT_EQ(cm->getItemByName("SILENCER")->getAttachmentFamily()->skillCheck, NO_CHECK);
	EXPECT_EQ(cm->getItemByName("GUN_BARREL_EXTENDER")->getAttachmentFamily()->skillCheck, ATTACHING_SPECIAL_ITEM_CHECK);
	EXPECT_EQ(cm->getItemByName("FIRSTAIDKIT")->getAttachmentFamily(), nullptr);
}

// The original weapons take exactly the attachments their attachment_Xxx flags name
TEST_F(AttachmentFamilies, originalWeaponsKeepTheirAttachments)
{
	struct Legacy { char const* attachment; bool WeaponModel::* flag; };
	Legacy const legacy[] = {
		{ "SILENCER",                &WeaponModel::attachSilencer },
		{ "SNIPERSCOPE",             &WeaponModel::attachSniperScope },
		{ "LASERSCOPE",              &WeaponModel::attachLaserScope },
		{ "BIPOD",                   &WeaponModel::attachBipod },
		{ "DUCKBILL",                &WeaponModel::attachDuckbill },
		{ "UNDER_GLAUNCHER",         &WeaponModel::attachUnderGLauncher },
		{ "SPRING_AND_BOLT_UPGRADE", &WeaponModel::attachSpringAndBoltUpgrade },
		{ "GUN_BARREL_EXTENDER",     &WeaponModel::attachGunBarrelExtender },
	};
	int checked = 0;
	for (ItemModel const* const item : cm->getItems())
	{
		WeaponModel const* const weapon = item->asWeapon();
		if (!weapon) continue;
		for (Legacy const& l : legacy)
		{
			ItemModel const* const attachment = cm->getItemByName(l.attachment);
			EXPECT_EQ(weapon->canBeAttached(cm->getGamePolicy(), attachment), weapon->*l.flag)
				<< weapon->getInternalName().c_str() << " / " << l.attachment;
			++checked;
		}
	}
	EXPECT_GT(checked, 0);
}

TEST_F(AttachmentFamilies, ruleByWeaponTypeAndCalibre)
{
	std::map<ST::string, const CalibreModel*> const calibres{
		{ "AMMO9", cm->getMagazineByName("CLIP9_15")->calibre } };
	auto const family = ParseFamily(R"({ "family": "TEST", "weaponTypes": ["PISTOL", "M_PISTOL"], "calibres": ["AMMO9"] })", calibres);
	auto const attachment = MakeAttachment(family.get());

	WeaponModel const* const glock = cm->getWeaponByName("GLOCK_17"); // PISTOL, 9mm
	WeaponModel const* const sw38  = cm->getWeaponByName("SW38");     // PISTOL, .38
	WeaponModel const* const ak74  = cm->getWeaponByName("AK74");     // ASRIFLE
	EXPECT_TRUE(glock->canBeAttached(cm->getGamePolicy(), attachment.get()));
	EXPECT_FALSE(sw38->canBeAttached(cm->getGamePolicy(), attachment.get()));
	EXPECT_FALSE(ak74->canBeAttached(cm->getGamePolicy(), attachment.get()));

	// per-weapon exceptions
	WeaponModel denied(*glock);
	denied.attachmentsDeny.insert("TEST");
	EXPECT_FALSE(denied.canBeAttached(cm->getGamePolicy(), attachment.get()));
	WeaponModel allowed(*ak74);
	allowed.attachmentsAllow.insert("TEST");
	EXPECT_TRUE(allowed.canBeAttached(cm->getGamePolicy(), attachment.get()));
}

TEST_F(AttachmentFamilies, excludesWorkBothWays)
{
	std::map<ST::string, const CalibreModel*> const none;
	auto const a = ParseFamily(R"({ "family": "A", "weaponTypes": [], "excludes": ["B"] })", none);
	auto const b = ParseFamily(R"({ "family": "B", "weaponTypes": [] })", none);
	auto const c = ParseFamily(R"({ "family": "C", "weaponTypes": [] })", none);
	EXPECT_TRUE(a->excludesFamily(*b));
	EXPECT_TRUE(b->excludesFamily(*a));
	EXPECT_FALSE(a->excludesFamily(*c));
}

TEST_F(AttachmentFamilies, badFamilyDataIsRejected)
{
	std::map<ST::string, const CalibreModel*> const none;
	EXPECT_THROW(ParseFamily(R"({ "family": "X", "weaponTypes": ["PISTOLS"] })", none), DataError);
	EXPECT_THROW(ParseFamily(R"({ "family": "X", "weaponTypes": [], "calibres": ["AMMO9"] })", none), DataError);
	EXPECT_THROW(ParseFamily(R"({ "family": "X", "weaponTypes": [], "skillCheck": "HARD" })", none), DataError);
}

#endif
