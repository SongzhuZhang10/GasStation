#include "fuel_price.h"

using namespace std;

optional<float>
FuelPrice::getFuelPrice(FuelGrade grade)
{
	if (fuelData.find(grade) != fuelData.end()) {
		return fuelData[grade]; // Error occurs here if `const` is used for this function
	}
	else {
		// Return nullopt when no value is found for the grade
		return nullopt;
	}
}

void
FuelPrice::setFuelPrice(FuelGrade grade, float price)
{
	if (fuelData.find(grade) != fuelData.end()) {
		fuelData[grade] = price;
	}
	else {
		// throw an exception of type invalid_argument with the message "Invalid FuelGrade".
		throw invalid_argument("Invalid FuelGrade");
	}
}

float
FuelPrice::getCost(float volume, FuelGrade grade)
{
	float cost = 0;
	auto fuel_price = getFuelPrice(grade);
	if (fuel_price.has_value()) {
		if (*fuel_price > 0 && volume > 0) {
			// Access the value inside the optional using the * operator
			cost = *fuel_price * volume;
		}
	}
	else {
		cerr << "Error: Cannot find the price info on the fuel grade " << fuelGradeToString(grade) << endl;
	}
	return cost;
}

float
FuelPrice::getUnitCost(FuelGrade grade)
{
	optional<float> unit_cost = getFuelPrice(grade);
	if (unit_cost) {
		return  *unit_cost;
	}
	else {
		cout << "No valid unit price found for the given fuel grade." << endl;
	}
	return 0.0f;
}