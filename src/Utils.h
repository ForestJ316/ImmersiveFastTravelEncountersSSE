#pragma once

namespace Utils
{
	std::vector<std::string> SplitString(const std::string& a_str, std::string_view a_delimiter);
	std::string GetSeparateNotationRandom(const std::string& a_str, std::string_view a_delimiter);
	void CleanItemListString(std::string& a_str);
	std::pair<std::string, std::string> JoinItemListString(std::string a_itemForm, std::string a_amount);
	const std::pair<std::uint32_t, std::string> GetFormIDWithFile(const std::string& a_formWithFile);
	// Iterate "bottom-up" through a cell and its parents until the specified location is either found or exhausted
	bool GetCellIsInLocation(RE::TESObjectCELL* a_cell, const std::string_view& a_locName);
	// Return a cell from the surrounding grid that has a Location specified
	RE::TESObjectCELL* GetCellNearPlayerWithLocation(RE::TESObjectCELL* a_parentCell);
	// Calculate distance in meters based on the formula provided in
	// https://ck.uesp.net/wiki/Unit
	float GetDistanceInMeters(float a_distance);
}
