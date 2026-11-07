#include "VehicleModel.h"
#include "Exceptions.h"
#include "ItemModel.h"
#include "ItemSystem.h"
#include "Vehicles.h"
#include <utility>


VehicleModel::VehicleModel(ST::string enterSound, ST::string movementSound,
                           ProfileID profile, VehicleMovementType movementType,
                           uint16_t armourType, uint8_t seats_, uint8_t repairPointsDivisor,
                           uint8_t slotLimitMultiplier, uint16_t tacticalFuelPer100Tiles,
                           uint8_t travelTimePerSector, uint8_t fuelPerTravelMinute,
                           uint8_t gunStackBigPocket, uint8_t gunStackSmallPocket)
    : enter_sound(std::move(enterSound)), move_sound(std::move(movementSound)), profile(profile),
      movement_type(movementType), armour_type(armourType), seats(seats_),
      repair_points_divisor(repairPointsDivisor), slot_limit_multiplier(slotLimitMultiplier),
      tactical_fuel_per_100_tiles(tacticalFuelPer100Tiles), travel_time_per_sector(travelTimePerSector),
      fuel_per_travel_minute(fuelPerTravelMinute), gun_stack_big_pocket(gunStackBigPocket),
      gun_stack_small_pocket(gunStackSmallPocket) {}

static VehicleMovementType toMovementType(const ST::string& typeName)
{
	if (typeName == "FOOT") return FOOT;
	if (typeName == "CAR") return CAR;
	if (typeName == "TRUCK") return TRUCK;
	if (typeName == "TRACKED") return TRACKED;
	if (typeName == "AIR") return AIR;

	throw DataError(ST::format("'{}' is not a valid vehicle movement type", typeName));
}

const VehicleModel* VehicleModel::deserialize(const JsonValue &json, const ItemSystem* itemSystem, const MercSystem* mercSystem)
{
	auto obj = json.toObject();
	ST::string profile = obj.GetString("profile");
	auto mercProfile = mercSystem->getMercProfileInfoByName(profile);
	if (mercProfile == NULL) {
		throw DataError(ST::format("'{}' does not refer to a valid profile", profile));
	}
	if (mercProfile->mercType != MercType::VEHICLE && mercProfile->mercType != MercType::NOT_USED) {
		throw DataError(ST::format("Profile '{}' does not refer to a vehicle or unused profile", profile));
	}

	ST::string armourName = obj.GetString("armourType");
	const ItemModel* armour = itemSystem->getItemByName(armourName);
	if (!armour || !(armour->isArmour()))
	{
		throw DataError(ST::format("'{}' does not refer to an armour item", armourName));
	}

	unsigned int numSeats = obj.GetUInt("seats");
	if (numSeats > MAX_PASSENGERS_IN_VEHICLE)
	{
		// limited by struct VEHICLETYPE
		throw DataError(ST::format("Vehicles cannot have more than {} seats", MAX_PASSENGERS_IN_VEHICLE));
	}

	unsigned int const repairPointsDivisor = obj.getOptionalUInt("repairPointsDivisor", 10);
	if (repairPointsDivisor < 1 || repairPointsDivisor > 100)
	{
		throw DataError(ST::format("repairPointsDivisor must be 1-100, not {}", repairPointsDivisor));
	}

	unsigned int const slotLimitMultiplier = obj.getOptionalUInt("slotLimitMultiplier", 1);
	if (slotLimitMultiplier < 1 || slotLimitMultiplier > 100)
	{
		throw DataError(ST::format("slotLimitMultiplier must be 1-100, not {}", slotLimitMultiplier));
	}

	unsigned int const tacticalFuelPer100Tiles = obj.getOptionalUInt("tacticalFuelPer100Tiles", 200);
	if (tacticalFuelPer100Tiles > 10000)
	{
		throw DataError(ST::format("tacticalFuelPer100Tiles must be 0-10000, not {}", tacticalFuelPer100Tiles));
	}

	unsigned int const travelTimePerSector = obj.getOptionalUInt("travelTimePerSector", 30);
	if (travelTimePerSector < 1 || travelTimePerSector > 200)
	{
		throw DataError(ST::format("travelTimePerSector must be 1-200, not {}", travelTimePerSector));
	}

	unsigned int const fuelPerTravelMinute = obj.getOptionalUInt("fuelPerTravelMinute", 6);
	if (fuelPerTravelMinute > 100)
	{
		throw DataError(ST::format("fuelPerTravelMinute must be 0-100, not {}", fuelPerTravelMinute));
	}

	// docs/gun-stacking-plan.md, G9 (0 = the gun's own stack size)
	unsigned int const gunStackBigPocket = obj.getOptionalUInt("gunStackBigPocket", 0);
	if (gunStackBigPocket > 94)
	{
		throw DataError(ST::format("gunStackBigPocket must be 0-94, not {}", gunStackBigPocket));
	}
	unsigned int const gunStackSmallPocket = obj.getOptionalUInt("gunStackSmallPocket", 1);
	if (gunStackSmallPocket < 1 || gunStackSmallPocket > 94)
	{
		throw DataError(ST::format("gunStackSmallPocket must be 1-94, not {}", gunStackSmallPocket));
	}

	return new VehicleModel(
		obj.GetString("enterSound"),
		obj.GetString("moveSound"),
		mercProfile->profileID,
		toMovementType(obj.GetString("movementType")),
		armour->getItemIndex(),
		numSeats,
		repairPointsDivisor,
		slotLimitMultiplier,
		tacticalFuelPer100Tiles,
		travelTimePerSector,
		fuelPerTravelMinute,
		gunStackBigPocket,
		gunStackSmallPocket
		);
}

void VehicleModel::validateData(const std::vector<const VehicleModel*>& vehicles)
{
	if (vehicles.size() < NUMBER_OF_TYPES_OF_VEHICLES)
	{
		// the code has a lot of references to HELICOPTER(5)
		throw DataError(ST::format("There must be at least {} vehicle types", NUMBER_OF_TYPES_OF_VEHICLES));
	}
}
