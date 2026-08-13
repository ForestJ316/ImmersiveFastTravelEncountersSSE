#include "Settings.h"

#include "Utils.h"

#include <ClibUtil/SimpleIni.hpp>

Settings::CachedDataType Settings::EncounterCache = {};
std::unordered_map<RE::FormID, std::string> Settings::FastTravelActivatorCache = {};

void Settings::InitializeSettings()
{
	logger::info("Initializing Settings...");
	const auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler) {
		logger::error("TESDataHandler not found.");
		return;
	}
	constexpr auto ini_path = L"Data/SKSE/Plugins/ImmersiveFastTravelEncountersSSE/ImmersiveFastTravelEncountersSSE.ini";
	const auto InitSettings = [&](std::filesystem::path path) {
		CSimpleIniA ini;
		ini.SetUnicode();
		ini.SetAllowKeyOnly(true);
		ini.SetMultiKey(false);
		if (ini.LoadFile(path.string().c_str()) == SI_OK) {
			// Settings
			auto settingsSection = ini.GetSection("Settings");
			for (const auto& data : *settingsSection) {
				const auto settingName = data.first.pItem;
				if (string::iequals(settingName, "iEncounterChance")) {
					iEncounterChance = string::to_num<std::int16_t>(data.second);
					// Check user error in input
					if (iEncounterChance < 0) {
						iEncounterChance = 0;
					}
					if (iEncounterChance > 100) {
						iEncounterChance = 100;
					}
				}
				if (string::iequals(settingName, "fMinimumDistance")) {
					fMinimumDistance = string::to_num<float>(data.second);
					// Check user error in input
					if (fMinimumDistance < 0.0f) {
						fMinimumDistance = 0.0f;
					}
				}
			}
			// Debug
			const auto debugSection = ini.GetSection("Debug");
			const auto debugData = debugSection->find("iDebugEncounter");
			if (debugData != debugSection->end() && string::iequals(debugData->first.pItem, "iDebugEncounter")) {
				std::int16_t debug_temp = string::to_num<std::int16_t>(debugData->second);
				if (debug_temp > 0) {
					iDebugEncounter = debug_temp;
					// If debug is on, then always show the relevant encounter
					iEncounterChance = 100;
				}
				else {
					// Just put something that will probably never appear
					// in case the user decided to make an encounter with the key "0"
					iDebugEncounter = INT16_MIN + 1;
				}
			}
			logger::info("...Settings initialized.");
		}
		else {
			logger::error("...File Path: {} for ImmersiveFastTravelEncountersSSE.ini doesn't exist.", path.string());
		}
		ini.Reset(); // Deallocate memory
	};
	InitSettings(ini_path);
}

void Settings::SetFastTravelEncounters(std::string a_type, std::vector<std::string> a_encounterHolds, const json::const_iterator& a_encounter)
{
	CachedEncounterData cachedData;
	// Optional
	if (a_encounter.value().contains("Title") && a_encounter.value()["Title"].is_string()) {
		cachedData.title = a_encounter.value()["Title"];
	}
	// Mandatory, but fall-back to empty string
	if (a_encounter.value().contains("Message") && a_encounter.value()["Message"].is_string()) {
		cachedData.message = a_encounter.value()["Message"];
	}
	// Optional, provide means to specify custom text for the exit button. Fall-back to "Ok" button
	if (a_encounter.value().contains("Choices") && a_encounter.value()["Choices"].is_array()) {
		cachedData.choices = a_encounter.value()["Choices"];
	}
	else {
		json exitButton;
		exitButton[""] = { {"Choice", "Ok"} };
		if (a_encounter.value().contains("Choice") && a_encounter.value()["Choice"].is_string()) {
			exitButton[""] = { {"Choice", a_encounter.value()["Choice"]} };
		}
		cachedData.choices = exitButton;
	}
	// Optional
	if (a_encounter.value().contains("SoundFX") && a_encounter.value()["SoundFX"].is_string()) {
		cachedData.soundFX = a_encounter.value()["SoundFX"];
	}
	// Optional
	if (a_encounter.value().contains("Survival") && a_encounter.value()["Survival"].is_boolean()) {
		cachedData.survival = a_encounter.value()["Survival"];
	}
	// Check encounter being valid for multiple holds
	for (const auto& hold : a_encounterHolds) {
		logger::info("inserting for hold: {}", hold);
		holds.push_back(hold);
		EncounterCache[a_type][hold].push_back(cachedData);
	}
	// TODO
	// Check activator specific, add later

	// Check empty: no holds, no activator
	if (a_encounterHolds.size() == 0 /* && no activator */) {
		EncounterCache[a_type][""].push_back(cachedData);
	}
}

void Settings::InitializeEncounterCache()
{
	logger::info("Initializing Encounter cache...");
	// Initialize EncounterData
	EncounterCache["Map"] = {};
	EncounterCache["Carriage"] = {};
	EncounterCache["Ferry"] = {};
	EncounterCache["Other"] = {};
	// Populate EncounterData
	constexpr auto encounters_path = L"Data/SKSE/Plugins/ImmersiveFastTravelEncountersSSE/Encounters.json";
	const auto InitEncounterCache = [&](std::filesystem::path path) {
		std::ifstream file(path.string().c_str());
		if (file.is_open()) {
			json encountersList = json::parse(file);
			for (json::const_iterator encounter = encountersList.begin(); encounter != encountersList.end(); ++encounter) {
				// Keys must be numbered only
				if (string::is_only_digit(encounter.key())) {
					// iDebugEncounter setting, get only the specified encounter
					if (iDebugEncounter <= 0 || string::to_num<int>(encounter.key()) == iDebugEncounter) {
						// Type is a mandatory field
						if (encounter.value().contains("Type") && encounter.value()["Type"].is_string()) {
							// Check for conditions
							// TODO
							// Activator from json add later
							std::vector<std::string> encounterHolds;
							if (encounter.value().contains("Hold") && encounter.value()["Hold"].is_string()) {
								encounterHolds = Utils::GetSplitStrings(encounter.value()["Hold"].get<std::string>(), ",");
							}
							// Check for fast travel type
							auto encounterType = encounter.value()["Type"].get<std::string>();
							if (encounterType.contains("Map")) {
								SetFastTravelEncounters("Map", encounterHolds, encounter);
							}
							if (encounterType.contains("Carriage")) {
								SetFastTravelEncounters("Carriage", encounterHolds, encounter);
							}
							if (encounterType.contains("Ferry")) {
								SetFastTravelEncounters("Ferry", encounterHolds, encounter);
							}
							if (encounterType.contains("Other")) {
								SetFastTravelEncounters("Other", encounterHolds, encounter);
							}
						}
						// Debug forced encounter added, don't iterate the rest of the json
						if (iDebugEncounter > 0) {
							break;
						}
					}
				}
			}
			logger::info("...Encounter cache initialized.");
		}
		else {
			logger::error("...File Path: {} for Encounters.json doesn't exist.", path.string());
		}
	};
	InitEncounterCache(encounters_path);
}

void Settings::InitializeActivatorCache()
{
	logger::info("Initializing Fast Travel Activator cache...");
	const auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler) {
		logger::error("TESDataHandler not found.");
		return;
	}
	constexpr auto ini_path = L"Data/SKSE/Plugins/ImmersiveFastTravelEncountersSSE/ImmersiveFastTravelEncountersSSE.ini";
	const auto InitFastTravelActivatorCache = [&](std::filesystem::path path) {
		CSimpleIniA ini;
		ini.SetUnicode();
		ini.SetAllowKeyOnly(true);
		// Get the last key entry only
		// No same activator form for 2 different types of fast travel
		ini.SetMultiKey(false);
		if (ini.LoadFile(path.string().c_str()) == SI_OK) {
			// GetSection returns a multimap. The keys are already sorted by default
			const auto mapSection = ini.GetSection("Map");
			for (const auto& data : *mapSection) {
				auto formWithFile = Utils::GetFormIDWithFile(data.first.pItem);
				if (formWithFile.first) {
					auto formID = dataHandler->LookupFormID(formWithFile.first, formWithFile.second);
					if (formID) {
						FastTravelActivatorCache[formID] = "Map";
					}
				}
			}
			const auto carriageSection = ini.GetSection("Carriage");
			for (const auto& data : *carriageSection) {
				auto formWithFile = Utils::GetFormIDWithFile(data.first.pItem);
				if (formWithFile.first) {
					auto formID = dataHandler->LookupFormID(formWithFile.first, formWithFile.second);
					if (formID) {
						FastTravelActivatorCache[formID] = "Carriage";
					}
				}
			}
			const auto ferrySection = ini.GetSection("Ferry");
			for (const auto& data : *ferrySection) {
				auto formWithFile = Utils::GetFormIDWithFile(data.first.pItem);
				if (formWithFile.first) {
					auto formID = dataHandler->LookupFormID(formWithFile.first, formWithFile.second);
					if (formID) {
						FastTravelActivatorCache[formID] = "Ferry";
					}
				}
			}
			const auto otherSection = ini.GetSection("Other");
			for (const auto& data : *otherSection) {
				auto formWithFile = Utils::GetFormIDWithFile(data.first.pItem);
				if (formWithFile.first) {
					auto formID = dataHandler->LookupFormID(formWithFile.first, formWithFile.second);
					if (formID) {
						FastTravelActivatorCache[formID] = "Other";
					}
				}
			}
			logger::info("...Fast Travel Activator cache initialized.");
		}
		else {
			logger::error("...File Path: {} for ImmersiveFastTravelEncountersSSE.ini doesn't exist.", path.string());
		}
		ini.Reset(); // Deallocate memory
	};
	InitFastTravelActivatorCache(ini_path);
}

void Settings::InitializeForms()
{
	const auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler) {
		logger::error("Settings::InitializeSoundFXForms: TESDataHandler not found.");
		return;
	}
	sound_FXCategory = dataHandler->LookupForm<RE::BGSSoundCategory>(0x172A1, "Skyrim.esm");
	sound_FXOutput = dataHandler->LookupForm<RE::BGSSoundOutput>(0x7EDCA, "Skyrim.esm");

	survival_HungerCurrent = dataHandler->LookupForm<RE::TESGlobal>(0x81A, "ccqdrsse001-survivalmode.esl");
	survival_HungerMax = dataHandler->LookupForm<RE::TESGlobal>(0x80C, "ccqdrsse001-survivalmode.esl");
	survival_ExhaustionCurrent = dataHandler->LookupForm<RE::TESGlobal>(0x816, "ccqdrsse001-survivalmode.esl");
	survival_ExhaustionMax = dataHandler->LookupForm<RE::TESGlobal>(0x84A, "ccqdrsse001-survivalmode.esl");
	survival_ColdCurrent = dataHandler->LookupForm<RE::TESGlobal>(0x81B, "ccqdrsse001-survivalmode.esl");
	survival_ColdMax = dataHandler->LookupForm<RE::TESGlobal>(0x84B, "ccqdrsse001-survivalmode.esl");
}

void Settings::CheckIsExperienceModInstalled()
{
	auto experience = REX::W32::GetModuleHandleW(L"Experience.dll");
	if (experience != NULL) {
		bIsExperienceModActive = true;
	}
}

void Settings::Initialize()
{
	InitializeSettings();
	InitializeEncounterCache();
	InitializeActivatorCache();

	InitializeForms();
	CheckIsExperienceModInstalled();
}

const Settings::CachedDataType& Settings::GetEncounterCache() const
{
	return EncounterCache;
}

const std::unordered_map<RE::FormID, std::string>& Settings::GetFastTravelActivatorCache() const
{
	return FastTravelActivatorCache;
}

const bool Settings::IsSurvivalEnabled() const
{
	const auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler) {
		logger::error("Settings::IsSurvivalEnabled: TESDataHandler not found.");
		return false;
	}
	auto survivalModeForm = dataHandler->LookupForm<RE::TESGlobal>(0x826, "ccqdrsse001-survivalmode.esl");
	if (survivalModeForm) {
		return survivalModeForm->value;
	}
	return false;
}

const bool Settings::IsValidHold(const std::string& a_hold) const
{
	const auto found = std::ranges::find_if(holds, [a_hold](const std::string& hold) { return hold == a_hold; });
	return !found->empty();
}
