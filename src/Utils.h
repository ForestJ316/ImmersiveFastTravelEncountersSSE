#pragma once

namespace Utils
{
	std::vector<std::string> SplitString(const std::string& a_str, std::string_view a_delimiter);
	std::string GetSeparateNotationRandom(const std::string& a_str);
	std::pair<std::string, std::string> JoinItemListString(std::string a_itemForm, std::string a_amount);
	std::pair<std::uint32_t, std::string> GetFormIDWithFile(const std::string& a_formWithFile);
	// Iterate "bottom-up" through a cell and its parents until the specified location is either found or exhausted
	bool GetCellIsInLocation(RE::TESObjectCELL* a_cell, const std::string_view& a_locName);
	// Return a cell from the surrounding grid that has a Location specified
	RE::TESObjectCELL* GetCellNearPlayerWithLocation(RE::TESObjectCELL* a_parentCell);
	// Calculate distance in meters based on the formula provided in
	// https://ck.uesp.net/wiki/Unit
	float GetDistanceInMeters(float a_distance);
	// Check for "Randomized" and "DualRandomized" conditions to replace the %random, %dualRandom1, %dualRandom2 strings
	void SetRandomizedNumbers(const json& a_json, int& a_iRandom, std::pair<int, int>& a_iDualRandom);
	void ReplaceRandomizedStrings(std::string& a_text, const int& a_iRandom, const std::pair<int, int>& a_iDualRandom);
	// Case for "AddRandomItem" function where there might be %item1 etc. strings in the message
	void ReplaceItemStrings(const std::vector<std::pair<RE::TESForm*, std::int32_t>>& a_itemList, std::string& a_message);
}
