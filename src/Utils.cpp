#include "Utils.h"

#include <algorithm>

std::vector<std::string> Utils::SplitString(const std::string& a_str, std::string_view a_delimiter)
{
	if (string::is_empty(a_str.c_str())) {
		return {};
	}
	auto splitStr = string::split(a_str, a_delimiter);
	// Remove leading and trailing spaces
	std::ranges::for_each(splitStr, [](std::string& str) { string::trim(str); });
	return splitStr;
}

std::string Utils::GetSeparateNotationRandom(const std::string& a_str)
{
	const auto randomMinMax = string::split(a_str, "-");
	if (!string::is_only_digit(randomMinMax.at(0))) {
		logger::error("Function was given an invalid min amount argument. It must be a number.");
		return "";
	}
	if (!string::is_only_digit(randomMinMax.at(1))) {
		logger::error("Function was given an invalid max amount argument. It must be a number.");
		return "";
	}
	auto a_min = !string::is_empty(randomMinMax.at(0).c_str()) ? string::to_num<std::int32_t>(randomMinMax.at(0)) : 1;
	auto a_max = !string::is_empty(randomMinMax.at(1).c_str()) ? string::to_num<std::int32_t>(randomMinMax.at(1)) : 1;
	auto randomAmount = clib_util::RNG().generate<std::int32_t>(a_min, a_max);
	return std::to_string(randomAmount);
}

std::pair<std::string, std::string> Utils::JoinItemListString(std::string a_itemForm, std::string a_amount)
{
	if (string::is_empty(a_itemForm.c_str()) || string::is_empty(a_amount.c_str())) {
		return {};
	}
	if (!a_itemForm.contains("{") || a_itemForm.contains("}") || !a_amount.contains("}") || a_amount.contains("{")) {
		logger::error("Form {} with amount {} input is invalid.", a_itemForm, a_amount);
		return {};
	}
	string::replace_first_instance(a_itemForm, "{", "");
	string::replace_first_instance(a_amount, "}", "");
	if (a_amount.contains("-")) {
		a_amount = GetSeparateNotationRandom(a_amount);
	}
	return std::pair(a_itemForm, a_amount);
}

std::pair<std::uint32_t, std::string> Utils::GetFormIDWithFile(const std::string& a_formWithFile)
{
	if (string::is_empty(a_formWithFile.c_str())) {
		logger::error("Form for a specified function is empty.");
		return {};
	}
	if (!a_formWithFile.contains("|")) {
		logger::error("Form {} is invalid. It must be 0xFormID|Mod.esp.", a_formWithFile);
		return {};
	}
	auto formStr = a_formWithFile.substr(0, a_formWithFile.find("|"));
	if (!string::is_only_hex(formStr)) {
		logger::error("Form {} is invalid. It must be 0xFormID", formStr);
		return {};
	}
	auto formID = string::to_num<std::uint32_t>(formStr, true);
	auto fileName = a_formWithFile.substr(a_formWithFile.find("|") + 1);
	return std::make_pair(formID, fileName);
}

bool Utils::GetCellIsInLocation(RE::TESObjectCELL* a_cell, const std::string_view& a_locName)
{
	if (!a_cell || !a_cell->GetLocation()) {
		return false;
	}
	// Exhaust all locations until Tamriel, fail-safe 4 iterations (most cases 2 is enough, 3 is possible though)
	int i = 4;
	auto currentLoc = a_cell->GetLocation();
	RE::BSFixedString currentLocName = currentLoc->GetFullName();
	while (currentLocName != "Tamriel" && i != 0) {
		if (string::iequals(currentLocName, a_locName)) {
			return true;
		}
		if (currentLoc->parentLoc) {
			currentLoc = currentLoc->parentLoc;
			currentLocName = currentLoc->GetFullName();
		}
		i -= 1;
	}
	return false;
}

RE::TESObjectCELL* Utils::GetCellNearPlayerWithLocation(RE::TESObjectCELL* a_parentCell)
{
	if (!a_parentCell) {
		return nullptr;
	}
	if (a_parentCell->GetLocation()) {
		return a_parentCell;
	}
	// Fall-back for when the parent cell has no location
	// Find the location from checking the surrounding grid
	// Only exterior cells for this
	if (a_parentCell->IsExteriorCell()) {
		if (const auto gridCellArray = RE::TES::GetSingleton()->gridCells; gridCellArray) {
			std::unordered_map<RE::BGSLocation*, std::pair<RE::TESObjectCELL*, std::uint8_t>> locationCounter = {};
			for (std::uint32_t gridX = 0; gridX < gridCellArray->length; ++gridX) {
				for (std::uint32_t gridY = 0; gridY < gridCellArray->length; ++gridY) {
					const auto gridCell = gridCellArray->GetCell(gridX, gridY);
					// Store the locations and check which appears the most, that will be the relevant one
					if (gridCell && gridCell->IsExteriorCell() && gridCell->GetLocation()) {
						// Parent location of location should be enough for most cases
						auto gridCellLocParentLoc = gridCell->GetLocation()->parentLoc;
						if (gridCellLocParentLoc) {
							if (!locationCounter.contains(gridCellLocParentLoc)) {
								locationCounter[gridCellLocParentLoc] = {};
								// Just input first valid cell for location, we only care about the location
								locationCounter[gridCellLocParentLoc].first = gridCell;
							}
							auto& counter = locationCounter[gridCellLocParentLoc].second;
							if (counter > 0) {
								counter += 1;
							}
							else {
								counter = 1;
							}
						}
					}
				}
			}
			using locMap_value_t = std::pair<RE::BGSLocation*, std::pair<RE::TESObjectCELL*, std::uint8_t>>;
			const auto bestLocation = std::ranges::max_element(locationCounter, [](locMap_value_t prev, locMap_value_t next) {
				return prev.second.second < next.second.second;
			});
			return bestLocation != locationCounter.end() ? bestLocation->second.first : nullptr;
		}
	}
	return nullptr;
}

float Utils::GetDistanceInMeters(float a_distance)
{
	if (a_distance >= 0.0f) {
		return a_distance * 1.428f / 100.0f;
	}
	return 0.0f;
}
