#include "Settings.h"

#include "Utils.h"

#include <ClibUtil/simpleINI.hpp>

Settings::CachedDataType Settings::EncounterCache{};
std::unordered_map<RE::FormID, std::string> Settings::FastTravelActivatorCache{};

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
			for (auto& data : *settingsSection) {
				auto settingName = data.first.pItem;
				if (string::iequals(settingName, "iEncounterChance")) {
					iEncounterChance = string::to_num<int>(data.second);
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
			auto debugSection = ini.GetSection("Debug");
			auto debugData = debugSection->find("iDebugEncounter");
			if (debugData != debugSection->end() && string::iequals(debugData->first.pItem, "iDebugEncounter")) {
				int debug_temp = string::to_num<int>(debugData->second);
				if (debug_temp > 0) {
					iDebugEncounter = debug_temp;
					// If debug is on, then always show the relevant encounter
					iEncounterChance = 100;
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

void Settings::SetFastTravelEncounters(std::string a_type, std::vector<std::string> a_encounterHolds, const json::iterator a_encounter)
{
	CachedEncounterData cachedData;
	// Optional
	if (a_encounter.value().contains("Title") && a_encounter.value().at("Message").type() == json::value_t::string) {
		cachedData.title = a_encounter.value().at("Title");
	}
	// Mandatory, but fall-back to empty string
	if (a_encounter.value().contains("Message") && a_encounter.value().at("Message").type() == json::value_t::string) {
		cachedData.message = a_encounter.value().at("Message");
	}
	// Optional, provide means to specify custom text for the exit button. Fall-back to "Ok" button
	if (a_encounter.value().contains("Choices") && a_encounter.value().at("Choices").type() == json::value_t::array) {
		cachedData.choices = a_encounter.value().at("Choices");
	}
	else {
		json exitButton;
		exitButton[""] = { {"Choice", "Ok"} };
		if (a_encounter.value().contains("Choice") && a_encounter.value().at("Choice").type() == json::value_t::string) {
			exitButton[""] = { {"Choice", a_encounter.value().at("Choice")} };
		}
		cachedData.choices = exitButton;
	}
	// Optional
	if (a_encounter.value().contains("SoundFX") && a_encounter.value().at("SoundFX").type() == json::value_t::string) {
		cachedData.soundFX = a_encounter.value().at("SoundFX");
	}
	// Optional
	if (a_encounter.value().contains("Survival") && a_encounter.value().at("Survival").type() == json::value_t::boolean) {
		cachedData.survival = a_encounter.value().at("Survival");
	}
	// Check encounter being valid for multiple holds
	for (auto& hold : a_encounterHolds) {
		logger::info("inserting for hold: {}", hold);
		EncounterCache[a_type][hold].push_back(cachedData);
	}
	// TEST
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
			for (json::iterator encounter = encountersList.begin(); encounter != encountersList.end(); ++encounter) {
				// Keys must be numbered only
				if (string::is_only_digit(encounter.key())) {
					// iDebugEncounter setting, get only the specified encounter
					if (iDebugEncounter <= 0 || string::to_num<int>(encounter.key()) == iDebugEncounter) {
						// Type is a mandatory field
						if (encounter.value().contains("Type")) {
							// Check for conditions
							// TEST
							// Activator from json add later
							std::vector<std::string> encounterHolds;
							if (encounter.value().contains("Hold")) {
								encounterHolds = Utils::GetSplitStrings(encounter.value().at("Hold").get<std::string>(), ",");
							}
							// Check for fast travel type
							auto encounterType = encounter.value().at("Type").get<std::string>();
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
			auto mapSection = ini.GetSection("Map");
			for (auto& data : *mapSection) {
				auto formWithFile = Utils::GetFormIDWithFile(data.first.pItem);
				if (formWithFile.first) {
					auto formID = dataHandler->LookupFormID(formWithFile.first, formWithFile.second);
					if (formID) {
						FastTravelActivatorCache[formID] = "Map";
					}
				}
			}
			auto carriageSection = ini.GetSection("Carriage");
			for (auto& data : *carriageSection) {
				auto formWithFile = Utils::GetFormIDWithFile(data.first.pItem);
				if (formWithFile.first) {
					auto formID = dataHandler->LookupFormID(formWithFile.first, formWithFile.second);
					if (formID) {
						FastTravelActivatorCache[formID] = "Carriage";
					}
				}
			}
			auto ferrySection = ini.GetSection("Ferry");
			for (auto& data : *ferrySection) {
				auto formWithFile = Utils::GetFormIDWithFile(data.first.pItem);
				if (formWithFile.first) {
					auto formID = dataHandler->LookupFormID(formWithFile.first, formWithFile.second);
					if (formID) {
						FastTravelActivatorCache[formID] = "Ferry";
					}
				}
			}
			auto otherSection = ini.GetSection("Other");
			for (auto& data : *otherSection) {
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

void Settings::CheckIsExperienceModInstalled()
{
	auto experience = REX::W32::GetModuleHandleW(L"Experience.dll");
	if (experience != NULL) {
		bIsExperienceModActive = true;
	}
}

void Settings::InitializeSoundFXForms()
{
	const auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler) {
		logger::error("TESDataHandler not found.");
		return;
	}
	soundFXCategory = dataHandler->LookupForm<RE::BGSSoundCategory>(0x172A1, "Skyrim.esm");
	soundFXOutput = dataHandler->LookupForm<RE::BGSSoundOutput>(0x7EDCA, "Skyrim.esm");
}

void Settings::Initialize()
{
	InitializeSettings();
	InitializeEncounterCache();
	InitializeActivatorCache();
	CheckIsExperienceModInstalled();
	InitializeSoundFXForms();
}

Settings::CachedDataType Settings::GetEncounterCache()
{
	return Settings::GetSingleton()->EncounterCache;
}

std::unordered_map<RE::FormID, std::string> Settings::GetFastTravelActivatorCache()
{
	return Settings::GetSingleton()->FastTravelActivatorCache;
}

bool Settings::IsSurvivalEnabled()
{
	const auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler) {
		logger::error("TESDataHandler not found.");
		return false;
	}
	auto survivalModeForm = dataHandler->LookupForm<RE::TESGlobal>(0x826, "ccqdrsse001-survivalmode.esl");
	if (survivalModeForm) {
		return survivalModeForm->value;
	}
	return false;
}
