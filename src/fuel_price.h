#ifndef __FUEL_PRICE_H__
#define __FUEL_PRICE_H__

#include "common.h"
#include <unordered_map>

class FuelPrice
{
private:
	// Unordered map to hold FuelGrade -> float mappings
	unordered_map<FuelGrade, float> fuelData;
	// Function to get the float value associated with a FuelGrade
	optional<float> getFuelPrice(FuelGrade grade);

public:
	FuelPrice()
	{
		// Initialize all FuelGrade values with some default float value
		fuelData[FuelGrade::Oct87] = 4.1f;
		fuelData[FuelGrade::Oct89] = 4.6f;
		fuelData[FuelGrade::Oct91] = 4.9f;
		fuelData[FuelGrade::Oct94] = 5.2f;
		fuelData[FuelGrade::Invalid] = 0.0f;
	}

	// Function to set the float value associated with a FuelGrade
	void setFuelPrice(FuelGrade grade, float price);

	float getCost(float volume, FuelGrade grade);

	float getUnitCost(FuelGrade grade);
};

#endif // __FUEL_PRICE_H__