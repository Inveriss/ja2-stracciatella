#include "DefaultGamePolicy.h"
#include "Exceptions.h"
#include "Types.h"

#include <string_theory/format>

#include <algorithm>
#include <iterator>

namespace {

// docs/enemy-equipment-progress-plan.md, E7-E9: defaults are the decided values
GamePolicy::OffsetRange ReadOffsetRange(JsonObject const& obj, char const* name, int8_t min, int8_t max)
{
	if (obj.has(name))
	{
		auto const v = obj[name].toVec();
		if (v.size() != 2) throw DataError(ST::format("game.json: enemy_equipment {} needs [min, max]", name));
		min = static_cast<int8_t>(v[0].toInt());
		max = static_cast<int8_t>(v[1].toInt());
		if (min > max) throw DataError(ST::format("game.json: enemy_equipment {}: min {} is above max {}", name, min, max));
	}
	return { min, max };
}

void ReadEnemyEquipment(GamePolicy& p, JsonObject const& gp)
{
	JsonObject const ee = gp.has("enemy_equipment") ? gp["enemy_equipment"].toObject() : JsonObject();
	p.enemy_equipment_by_progress = ee.getOptionalBool("by_progress", true);

	JsonObject const lvl = ee.has("level_offsets") ? ee["level_offsets"].toObject() : JsonObject();
	p.enemy_equipment_level_offsets[0] = ReadOffsetRange(lvl, "bad",     -20, -10);
	p.enemy_equipment_level_offsets[1] = ReadOffsetRange(lvl, "poor",    -10,   0);
	p.enemy_equipment_level_offsets[2] = ReadOffsetRange(lvl, "average", -10,  10);
	p.enemy_equipment_level_offsets[3] = ReadOffsetRange(lvl, "good",      0,  10);
	p.enemy_equipment_level_offsets[4] = ReadOffsetRange(lvl, "great",    10,  20);

	JsonObject const typ = ee.has("type_offsets") ? ee["type_offsets"].toObject() : JsonObject();
	p.enemy_equipment_type_offsets[0] = ReadOffsetRange(typ, "admin", -20, -10);
	p.enemy_equipment_type_offsets[1] = ReadOffsetRange(typ, "army",  -10,  10);
	p.enemy_equipment_type_offsets[2] = ReadOffsetRange(typ, "elite",   0,  20);

	JsonObject const dif = ee.has("difficulty_offsets") ? ee["difficulty_offsets"].toObject() : JsonObject();
	p.enemy_equipment_difficulty_offsets[0] = static_cast<int8_t>(dif.getOptionalInt("easy", -10));
	p.enemy_equipment_difficulty_offsets[1] = static_cast<int8_t>(dif.getOptionalInt("experienced", 0));
	p.enemy_equipment_difficulty_offsets[2] = static_cast<int8_t>(dif.getOptionalInt("expert", 10));
}

// docs/enemy-elite-heavy-plan.md, section 3; left out = off
void ReadEnemyEliteHeavy(GamePolicy& p, JsonObject const& gp)
{
	JsonObject const eh = gp.has("enemy_elite_heavy") ? gp["enemy_elite_heavy"].toObject() : JsonObject();
	p.enemy_elite_heavy_enabled = eh.getOptionalBool("enabled", false);
	int const per = eh.getOptionalInt("elites_per_heavy", 5);
	if (per < 1 || per > 100) throw DataError(ST::format("game.json: enemy_elite_heavy elites_per_heavy must be 1-100, not {}", per));
	p.enemy_elite_heavy_elites_per_heavy = static_cast<uint8_t>(per);

	JsonObject const mod = eh.has("difficulty_modifiers") ? eh["difficulty_modifiers"].toObject() : JsonObject();
	p.enemy_elite_heavy_difficulty_modifiers[0] = static_cast<int8_t>(mod.getOptionalInt("easy", -1));
	p.enemy_elite_heavy_difficulty_modifiers[1] = static_cast<int8_t>(mod.getOptionalInt("experienced", 0));
	p.enemy_elite_heavy_difficulty_modifiers[2] = static_cast<int8_t>(mod.getOptionalInt("expert", 1));
}

// docs/enemy-ammo-chances-plan.md, section 3; defaults = the decided table
void ReadEnemyAmmo(GamePolicy& p, JsonObject const& gp)
{
	static uint8_t const defaults[10][4] = {
		{ 80, 20,  0,  0 }, { 40, 60,  0,  0 }, { 20, 80,  0,  0 }, { 10, 40, 50,  0 },
		{  0, 30, 70,  0 }, {  0, 20, 80,  0 }, {  0, 10, 80, 10 }, {  0,  0, 70, 30 },
		{  0,  0, 50, 50 }, {  0,  0, 30, 70 }
	};
	std::copy(&defaults[0][0], &defaults[0][0] + 40, &p.enemy_ammo_type_chances[0][0]);

	JsonObject const ea = gp.has("enemy_ammo") ? gp["enemy_ammo"].toObject() : JsonObject();
	if (ea.has("type_chances"))
	{
		auto const rows = ea["type_chances"].toVec();
		if (rows.size() != 10) throw DataError(ST::format("game.json: enemy_ammo type_chances needs 10 rows, not {}", rows.size()));
		for (size_t r = 0; r < 10; ++r)
		{
			auto const row = rows[r].toVec();
			if (row.size() != 4) throw DataError(ST::format("game.json: enemy_ammo type_chances row {} needs 4 values (REGULAR, HP, AP, SUPER_AP)", r + 1));
			int sum = 0;
			for (size_t c = 0; c < 4; ++c)
			{
				int const v = row[c].toInt();
				if (v < 0 || v > 100) throw DataError(ST::format("game.json: enemy_ammo type_chances row {}: {} is not 0-100", r + 1, v));
				p.enemy_ammo_type_chances[r][c] = static_cast<uint8_t>(v);
				sum += v;
			}
			if (sum != 100) throw DataError(ST::format("game.json: enemy_ammo type_chances row {} adds up to {}, not 100", r + 1, sum));
		}
	}

	auto const share = [&](char const* name, int def)
	{
		int const v = ea.getOptionalInt(name, def);
		if (v < 0 || v > 100) throw DataError(ST::format("game.json: enemy_ammo {} must be 0-100, not {}", name, v));
		return static_cast<uint8_t>(v);
	};
	p.enemy_ammo_buckshot_chance = share("buckshot_chance", 50);
	p.enemy_ammo_he_chance       = share("he_chance", 70);
}

}

DefaultGamePolicy::DefaultGamePolicy(const JsonValue& json)
{
	auto gp = json.toObject();
	extra_hotkeys = gp.getOptionalBool("extra_hotkeys", true);
	can_enter_turnbased = gp.getOptionalBool("can_enter_turnbased");
	turnbased_starts_when_spotted = gp.getOptionalBool("turnbased_starts_when_spotted", false);
	middle_mouse_look = gp.getOptionalBool("middle_mouse_look", true);

	f_draw_item_shadow = gp.getOptionalBool("draw_item_shadow", true);
	target_fps = gp.getOptionalInt("target_fps", 40);
	game_durations_multiplier = gp.getOptionalDouble("game_durations_multiplier", 1.0);

	starting_cash_easy = gp.getOptionalInt("starting_cash_easy", 45000);
	starting_cash_medium = gp.getOptionalInt("starting_cash_medium", 35000);
	starting_cash_hard = gp.getOptionalInt("starting_cash_hard", 30000);

	f_drop_everything = gp.getOptionalBool("drop_everything");
	enemy_drop_progress_limits = gp.getOptionalBool("enemy_drop_progress_limits", true);
	dealer_stock_progress_limits = gp.getOptionalBool("dealer_stock_progress_limits", true);
	dealer_stock_progress_lead = gp.getOptionalUInt("dealer_stock_progress_lead", 10);
	dealer_guaranteed_deliveries = gp.getOptionalBool("dealer_guaranteed_deliveries", false);
	f_all_dropped_visible = gp.getOptionalBool("all_drops_visible");

	multiple_interrupts = gp.getOptionalBool("multiple_interrupts");

	enemy_weapon_minimal_status = gp.getOptionalInt("enemy_weapon_minimal_status", 0);

	squad_size = gp.getOptionalUInt("squad_size", 6);

	{
		unsigned int const n = gp.getOptionalUInt("gun_stack_size", 5);
		if (n < 1 || n > 94)
		{
			throw DataError(ST::format("gun_stack_size must be 1-94, not {}", n));
		}
		gun_stack_size = static_cast<uint8_t>(n);
	}

	auto ai = gp["ai"].toObject();
	ai_better_aiming_choice = ai.getOptionalBool("better_aiming_choice");
	ai_go_prone_more_often = ai.getOptionalBool("go_prone_more_often");
	threshold_cth_head = ai.getOptionalInt("threshold_cth_head", 67);
	threshold_cth_legs = ai.getOptionalInt("threshold_cth_legs", 67);

	avoid_ambushes = ai.getOptionalBool("avoid_ambushes");
	stay_on_rooftop = ai.getOptionalBool("stay_on_rooftop");

	enemy_elite_minimum_level = gp.getOptionalInt("enemy_elite_minimum_level", 6);
	enemy_elite_maximum_level = gp.getOptionalInt("enemy_elite_maximum_level", 10);

	gui_extras = gp.getOptionalBool("gui_extras", true);
	extra_attachments = gp.getOptionalBool("extra_attachments");
	skip_sleep_explanation = gp.getOptionalBool("skip_sleep_explanation");

	pablo_wont_steal = gp.getOptionalBool("pablo_wont_steal");

	critical_damage_head_multiplier = gp.getOptionalDouble("tactical_head_damage_multiplier", 1.5);
	critical_damage_legs_multiplier = gp.getOptionalDouble("tactical_legs_damage_multiplier", 0.5);
	chance_to_hit_maximum = gp.getOptionalInt("chance_to_hit_maximum", 99);
	chance_to_hit_minimum = gp.getOptionalInt("chance_to_hit_minimum", 1);

	aim_bonus_per_std_ap = gp.getOptionalInt("aim_bonus_per_std_ap", 10);
	aim_bonus_sniperscope = gp.getOptionalInt("aim_bonus_sniperscope", 20);
	aim_bonus_laserscope = gp.getOptionalInt("aim_bonus_laserscope", 20);
	range_penalty_silencer = gp.getOptionalInt("range_penalty_silencer", 0);
	range_bonus_barrel_extender = gp.getOptionalInt("range_bonus_barrel_extender", 100);

	always_show_cursor_in_tactical = gp.getOptionalBool("always_show_cursor_in_tactical", false);
	show_hit_chance = gp.getOptionalBool("show_hit_chance", false);
	website_loading_time_scale = gp.getOptionalDouble("website_loading_time_scale", 1.0);
	diagonally_interactable_doors = gp.getOptionalBool("diagonally_interactable_doors", false);

	auto imp = gp["imp"].toObject();
	imp_load_saved_merc_by_nickname = imp.getOptionalBool("load_saved_merc_by_nickname");
	imp_load_keep_inventory = imp.getOptionalBool("load_keep_inventory");
	imp_attribute_max = imp.getOptionalInt("max_attribute_points", 85);
	imp_attribute_min = imp.getOptionalInt("min_attribute_points", 35);
	imp_attribute_zero_bonus = imp.getOptionalInt("zero_attribute_points_bonus", 15);
	imp_attribute_bonus = imp.getOptionalInt("bonus_attribute_points", 40);
	imp_pick_skills_directly = imp.getOptionalBool("pick_skills_directly");

	merc_online_min_days = gp.getOptionalUInt("merc_online_min_days", 1);
	merc_online_max_days = gp.getOptionalUInt("merc_online_max_days", 2);

	auto progress = gp["progress"].toObject();
	progress_event_madlab_min = progress.getOptionalInt("event_madlab_min", 35);
	progress_event_mike_min = progress.getOptionalInt("event_mike_min", 50);
	progress_event_iggy_min = progress.getOptionalInt("event_iggy_min", 70);

	game_progress_sector_percent = progress.getOptionalUInt("game_progress_sector_percent", 2);
	game_progress_quest_percent = progress.getOptionalUInt("game_progress_quest_percent", 2);

	ReadEnemyEquipment(*this, gp);
	ReadEnemyEliteHeavy(*this, gp);
	ReadEnemyAmmo(*this, gp);

	kills_per_point_easy = progress.getOptionalInt("kills_per_point_easy", 7);
	kills_per_point_medium = progress.getOptionalInt("kills_per_point_medium", 10);
	kills_per_point_hard = progress.getOptionalInt("kills_per_point_hard", 15);

	progress_weight_kills = progress.getOptionalDouble("weight_kills", 25.0);
	progress_weight_control = progress.getOptionalDouble("weight_control", 25.0);
	progress_weight_income = progress.getOptionalDouble("weight_income", 50.0);

	unhired_merc_deaths_easy = gp.getOptionalInt("unhired_merc_deaths_easy", 1);
	unhired_merc_deaths_medium = gp.getOptionalInt("unhired_merc_deaths_medium", 2);
	unhired_merc_deaths_hard = gp.getOptionalInt("unhired_merc_deaths_hard", 3);

	auto campaign = gp["campaign"].toObject();
	ST::string sector_string = campaign.getOptionalString("start_sector");
	start_sector = SGPSector::FromShortString(!sector_string.empty() ? sector_string : "A9").AsByte();
	reveal_start_sector = campaign.getOptionalBool("start_sector_revealed", false);

	{
		static int32_t const default_prices[4]  = { 100, 200, 500, 1000 };
		static uint8_t const default_max[4]     = { 25, 50, 75, 100 };
		static int32_t const default_percent[4] = { 130, 115, 85, 70 };
		std::copy(std::begin(default_prices),  std::end(default_prices),  militia_recruit_price);
		std::copy(std::begin(default_max),     std::end(default_max),     militia_recruit_leadership_max);
		std::copy(std::begin(default_percent), std::end(default_percent), militia_recruit_price_percent);
		militia_recruitment = true;
		if (gp.has("militia_recruitment"))
		{
			auto mr = gp["militia_recruitment"].toObject();
			militia_recruitment = mr.getOptionalBool("enabled", true);
			char const* const price_keys[4] = { "price_green", "price_regular", "price_elite", "price_elite_heavy" };
			for (int i = 0; i != 4; ++i)
			{
				militia_recruit_price[i] = mr.getOptionalInt(price_keys[i], default_prices[i]);
			}
			if (mr.has("leadership_max"))
			{
				auto const v = mr["leadership_max"].toVec();
				for (size_t i = 0; i != 4 && i < v.size(); ++i) militia_recruit_leadership_max[i] = v[i].toInt();
			}
			if (mr.has("price_percent"))
			{
				auto const v = mr["price_percent"].toVec();
				for (size_t i = 0; i != 4 && i < v.size(); ++i) militia_recruit_price_percent[i] = v[i].toInt();
			}
		}
	}

	militia_watch_realtime    = true;
	militia_watch_ai_delay_ms = 100;
	if (gp.has("militia_watch"))
	{
		auto mw = gp["militia_watch"].toObject();
		militia_watch_realtime    = mw.getOptionalBool("realtime", true);
		militia_watch_ai_delay_ms = mw.getOptionalUInt("ai_delay_ms", 100);
	}

	militia_explosives_spare_friends = true;
	if (gp.has("militia_ai"))
	{
		auto ma = gp["militia_ai"].toObject();
		militia_explosives_spare_friends = ma.getOptionalBool("explosives_spare_friends", true);
	}
}

/** Check if a hotkey is enabled. */
bool DefaultGamePolicy::isHotkeyEnabled(UIMode mode, HotkeyModifier modifier, uint32_t key) const
{
	if(mode == UI_Tactical)
	{
		if(modifier == HKMOD_None)
		{
			switch(key)
			{
				case 'j':         return extra_hotkeys;
			}
		}
		else if(modifier == HKMOD_CTRL)
		{
			switch(key)
			{
				case 'n':
				case 'q':         return extra_hotkeys;
			}
		}
		else if(modifier == HKMOD_SHIFT)
		{
			switch(key)
			{
				case 'j':         return extra_hotkeys;
			}
		}
		else if(modifier == HKMOD_ALT)
		{
			switch(key)
			{
				case 'r':         return extra_hotkeys;
			}
		}
		else if(modifier == HKMOD_CTRL_SHIFT)
		{
			switch(key)
			{
				case 'r':         return extra_hotkeys;
			}
		}
	}

	if(mode == UI_Map)
	{
		if(modifier == HKMOD_CTRL)
		{
			switch(key)
			{
				case 'i':         return extra_hotkeys;
			}
		}
	}

	return false;
}
