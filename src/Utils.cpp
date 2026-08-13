#include "Utils.h"

#include <algorithm>

std::vector<std::string> Utils::GetSplitStrings(const std::string& a_str, std::string_view a_delimiter)
{
	if (string::is_empty(a_str.c_str())) {
		return {};
	}
	auto splitString = string::split(a_str, a_delimiter);
	// Remove leading and trailing spaces
	std::ranges::for_each(splitString, [](std::string& str) { string::trim(str); });
	return splitString;
}

std::pair<std::uint32_t, std::string> Utils::GetFormIDWithFile(const std::string& a_formWithFile)
{
	if (string::is_empty(a_formWithFile.c_str())) {
		logger::error("Form for a specified function is empty.");
		return std::pair<std::uint32_t, std::string>();
	}
	if (!a_formWithFile.contains("|")) {
		logger::error("Form {} is invalid. It must be 0xFormID|Mod.esp.", a_formWithFile);
		return std::pair<std::uint32_t, std::string>();
	}
	auto formStr = a_formWithFile.substr(0, a_formWithFile.find("|"));
	if (!string::is_only_hex(formStr)) {
		logger::error("Form {} is invalid. It must be 0xFormID", formStr);
		return std::pair<std::uint32_t, std::string>();
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
	// Exhaust all locations until Tamriel, fail-safe 4 iterations (most cases 2 is enough)
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
		auto gridCellArray = RE::TES::GetSingleton()->gridCells;
		if (gridCellArray) {
			std::unordered_map<RE::BGSLocation*, std::pair<RE::TESObjectCELL*, std::uint8_t>> locationCounter = {};
			for (std::uint32_t gridX = 0; gridX < gridCellArray->length; ++gridX) {
				for (std::uint32_t gridY = 0; gridY < gridCellArray->length; ++gridY) {
					auto gridCell = gridCellArray->GetCell(gridX, gridY);
					// Store the locations and check which appears the most, that will be the relevant one
					if (gridCell && gridCell->GetLocation() && gridCell->IsExteriorCell()) {
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
			std::pair<RE::TESObjectCELL*, std::uint8_t> bestLocation = {};
			for (auto it = locationCounter.begin(); it != locationCounter.end(); ++it) {
				// TEST
				logger::info("loc name: {}, counter: {}", it->first->GetFullName(), it->second.second);
				if (it->second.second > bestLocation.second) {
					bestLocation = it->second;
				}
			}
			return bestLocation.first;
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
