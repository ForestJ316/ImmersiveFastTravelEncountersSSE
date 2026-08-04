#pragma once

namespace Utils
{
	std::vector<std::string> GetSplitStrings(std::string a_str, std::string_view a_delimiter);
	std::pair<std::uint32_t, std::string> GetFormIDWithFile(std::string a_formWithFile);
	// Iterate "bottom-up" through a cell and its parents until the specified location is either found or exhausted
	bool GetCellIsInLocation(std::string_view a_locName, RE::TESObjectCELL* a_cell);
	// Return a cell from the surrounding grid that has a Location specified
	RE::TESObjectCELL* GetCellNearPlayerWithLocation(RE::TESObjectCELL* a_parentCell);
	// Calculate distance in meters based on the formula provided in
	// https://ck.uesp.net/wiki/Unit
	float GetDistanceInMeters(float a_distance);
}
