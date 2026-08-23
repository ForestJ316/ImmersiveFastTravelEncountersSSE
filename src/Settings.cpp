#include "Settings.h"

#include <ClibUtil/SimpleIni.hpp>

std::unordered_map<std::uint16_t, Settings::EncounterCacheData> Settings::EncounterCache = {};
std::unordered_map<RE::FormID, std::string> Settings::FastTravelActivatorCache = {};

void Settings::InitializeSettings()
{
	logger::info("Initializing Settings...");
	const auto a_dataHandler = RE::TESDataHandler::GetSingleton();
	if (!a_dataHandler) {
		logger::error("TESDataHandler not found.");
		return;
	}
	// For Main Settings
	constexpr auto mcm_default_path = L"Data/MCM/Config/ImmersiveFastTravelEncountersSSE/settings.ini";
	constexpr auto mcm_current_path = L"Data/MCM/Settings/ImmersiveFastTravelEncountersSSE.ini";
	const auto InitMCMSettings = [&](std::filesystem::path path) {
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
				if (string::iequals(settingName, "iMinimumDistance")) {
					fMinimumDistance = string::to_num<float>(data.second);
					// Check user error in input
					if (fMinimumDistance < 0.0f) {
						fMinimumDistance = 0.0f;
					}
				}
				if (string::iequals(settingName, "bMapEncounters")) {
					bMapEncounters = data.second == "1"sv ? true : false;
				}
				if (string::iequals(settingName, "bCarriageEncounters")) {
					bCarriageEncounters = data.second == "1"sv ? true : false;
				}
				if (string::iequals(settingName, "bFerryEncounters")) {
					bFerryEncounters = data.second == "1"sv ? true : false;
				}
				if (string::iequals(settingName, "bOtherEncounters")) {
					bOtherEncounters = data.second == "1"sv ? true : false;
				}
			}
		}
		else {
			logger::error("...File Path: {} for MCM settings doesn't exist.", path.string());
		}
		ini.Reset(); // Deallocate memory
	};
	// For Debug Setting
	constexpr auto base_ini_path = L"Data/SKSE/Plugins/ImmersiveFastTravelEncountersSSE.ini";
	const auto InitDebugSetting = [&](std::filesystem::path path) {
		CSimpleIniA ini;
		ini.SetUnicode();
		ini.SetAllowKeyOnly(true);
		ini.SetMultiKey(false);
		if (ini.LoadFile(path.string().c_str()) == SI_OK) {
			// Debug
			const auto debugSection = ini.GetSection("Debug");
			const auto debugData = debugSection->find("iDebugEncounter");
			if (debugData != debugSection->end() && string::iequals(debugData->first.pItem, "iDebugEncounter")) {
				if (string::icontains(debugData->second, "|")) {
					auto debug_split = utils::SplitString(debugData->second, "|");
					try {
						std::int16_t debugNum_temp = string::to_num<std::int16_t>(debug_split.at(1));
						if (debugNum_temp > 0) {
							iDebugEncounter = { debug_split.at(0), debugNum_temp };
							// If debug is on, then always show the relevant encounter
							iEncounterChance = 100;
						}
						else {
							// Just put something that will probably never appear
							// in case the user decided to make an encounter with the key "0"
							iDebugEncounter = { debug_split.at(0), INT16_MIN + 1 };
						}
					}
					catch (...) {
						logger::warn("Error parsing File: {} for iDebugEncounter. Check if the value is in the correct format.", path.string());
					}
				}
			}
		}
		else {
			logger::error("...File Path: {} for ImmersiveFastTravelEncountersSSE.ini doesn't exist.", path.string());
		}
		ini.Reset(); // Deallocate memory
	};
	if (std::filesystem::exists(mcm_current_path)) {
		InitMCMSettings(mcm_current_path);
	}
	else {
		InitMCMSettings(mcm_default_path);
	}
	InitDebugSetting(base_ini_path);
	logger::info("...Settings done initializing.");
}

void Settings::SetFastTravelEncounters(const json::const_iterator& a_encounter)
{
	EncounterCacheData encounterData = {};
	encounterData.encounter = *a_encounter;
	// Check for conditions
	// TODO (if there is use-case for it)
	// Activator from json add later
	encounterData.travelTypes = utils::SplitString(a_encounter.value()["Type"].get<std::string>(), ",");
	if (a_encounter.value().contains("Hold") && a_encounter.value()["Hold"].is_string()) {
		encounterData.holds = a_encounter.value()["Hold"].get<std::string>();
	}
	if (a_encounter.value().contains("Survival") && a_encounter.value()["Survival"].is_boolean()) {
		encounterData.survival = a_encounter.value()["Survival"];
	}
	const auto encounterNum = static_cast<std::uint16_t>(EncounterCache.size() + 1);
	EncounterCache[encounterNum] = encounterData;
}

void Settings::InitializeEncounterCache()
{
	logger::info("Initializing Encounter cache...");
	// Populate EncounterData
	constexpr auto encounters_dir = L"Data/SKSE/Plugins/ImmersiveFastTravelEncountersSSE";
	if (!std::filesystem::exists(encounters_dir)) {
		char dir[256];
		std::wcstombs(dir, encounters_dir, sizeof(dir));
		logger::error("...File Directory: {} for encounters doesn't exist.", dir);
		return;
	}
	bool is_initialized = false;
	const auto InitEncounterFileCache = [&](std::filesystem::path path) {
		std::ifstream file(path.string().c_str());
		if (file.is_open()) {
			try {
				json encountersList = json::parse(file);
				if (encountersList.is_discarded()) {
					return;
				}
				for (json::const_iterator encounter = encountersList.begin(); encounter != encountersList.end(); ++encounter) {
					// Keys must be numbered only
					if (string::is_only_digit(encounter.key())) {
						const auto& [debug_file, debug_num] = iDebugEncounter;
						// iDebugEncounter setting, get only the specified encounter
						if (debug_num <= 0 || (debug_file == path.filename() && string::to_num<int>(encounter.key()) == debug_num)) {
							// Type is a mandatory field
							if (encounter.value().contains("Type") && encounter.value()["Type"].is_string()) {
								SetFastTravelEncounters(encounter);
								is_initialized = true;
							}
							else {
								logger::warn("Skipped encounter {} in file {}. It doesn't have a \"Type\" field which is mandatory.", encounter.key(), path.filename().string().c_str());
							}
							// Debug forced encounter added, don't iterate the rest of the json
							if (debug_num > 0) {
								break;
							}
						}
					}
				}
			}
			catch (...) {
				logger::warn("Couldn't parse JSON File: \"{}\".", path.filename().string().c_str());
			}
		}
		else {
			logger::error("Couldn't open JSON File: \"{}\".", path.filename().string().c_str());
		}
	};
	for (const auto& file : std::filesystem::directory_iterator(encounters_dir)) {
		if (file.path().extension() == ".json") {
			InitEncounterFileCache(file);
		}
	}
	if (is_initialized) {
		logger::info("...Encounter cache initialized.");
	}
	else {
		logger::info("...Encounter cache failed to initialize.");
	}
}

void Settings::InitializeActivatorCache()
{
	logger::info("Initializing Fast Travel Activator cache...");
	const auto a_dataHandler = RE::TESDataHandler::GetSingleton();
	if (!a_dataHandler) {
		logger::error("Settings::InitializeActivatorCache: TESDataHandler not found.");
		return;
	}
	constexpr auto ini_path = L"Data/SKSE/Plugins/ImmersiveFastTravelEncountersSSE.ini";
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
				const auto formWithFile = utils::GetFormIDWithFile(data.first.pItem);
				if (formWithFile.first) {
					auto formID = a_dataHandler->LookupFormID(formWithFile.first, formWithFile.second);
					if (formID) {
						FastTravelActivatorCache[formID] = "Map";
					}
				}
			}
			const auto carriageSection = ini.GetSection("Carriage");
			for (const auto& data : *carriageSection) {
				const auto formWithFile = utils::GetFormIDWithFile(data.first.pItem);
				if (formWithFile.first) {
					auto formID = a_dataHandler->LookupFormID(formWithFile.first, formWithFile.second);
					if (formID) {
						FastTravelActivatorCache[formID] = "Carriage";
					}
				}
			}
			const auto ferrySection = ini.GetSection("Ferry");
			for (const auto& data : *ferrySection) {
				const auto formWithFile = utils::GetFormIDWithFile(data.first.pItem);
				if (formWithFile.first) {
					auto formID = a_dataHandler->LookupFormID(formWithFile.first, formWithFile.second);
					if (formID) {
						FastTravelActivatorCache[formID] = "Ferry";
					}
				}
			}
			const auto otherSection = ini.GetSection("Other");
			for (const auto& data : *otherSection) {
				const auto formWithFile = utils::GetFormIDWithFile(data.first.pItem);
				if (formWithFile.first) {
					auto formID = a_dataHandler->LookupFormID(formWithFile.first, formWithFile.second);
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

void Settings::InitializeGlobals()
{
	const auto a_dataHandler = RE::TESDataHandler::GetSingleton();
	if (!a_dataHandler) {
		logger::error("Settings::InitializeGlobals: TESDataHandler not found.");
		return;
	}
	sound_FXCategory = a_dataHandler->LookupForm<RE::BGSSoundCategory>(0x172A1, "Skyrim.esm");
	sound_FXOutput = a_dataHandler->LookupForm<RE::BGSSoundOutput>(0x7EDCA, "Skyrim.esm");

	survival_HungerCurrent = a_dataHandler->LookupForm<RE::TESGlobal>(0x81A, "ccqdrsse001-survivalmode.esl");
	survival_HungerMax = a_dataHandler->LookupForm<RE::TESGlobal>(0x80C, "ccqdrsse001-survivalmode.esl");
	survival_ExhaustionCurrent = a_dataHandler->LookupForm<RE::TESGlobal>(0x816, "ccqdrsse001-survivalmode.esl");
	survival_ExhaustionMax = a_dataHandler->LookupForm<RE::TESGlobal>(0x84A, "ccqdrsse001-survivalmode.esl");
	survival_ColdCurrent = a_dataHandler->LookupForm<RE::TESGlobal>(0x81B, "ccqdrsse001-survivalmode.esl");
	survival_ColdMax = a_dataHandler->LookupForm<RE::TESGlobal>(0x84B, "ccqdrsse001-survivalmode.esl");
	
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

	InitializeGlobals();
}

const std::unordered_map<std::uint16_t, Settings::EncounterCacheData>& Settings::GetEncounterCache() const
{
	return EncounterCache;
}

const std::unordered_map<RE::FormID, std::string>& Settings::GetFastTravelActivatorCache() const
{
	return FastTravelActivatorCache;
}

const bool Settings::IsSurvivalEnabled() const
{
	const auto a_dataHandler = RE::TESDataHandler::GetSingleton();
	if (!a_dataHandler) {
		logger::error("Settings::IsSurvivalEnabled: TESDataHandler not found.");
		return false;
	}
	auto survivalModeForm = a_dataHandler->LookupForm<RE::TESGlobal>(0x826, "ccqdrsse001-survivalmode.esl");
	if (survivalModeForm) {
		return survivalModeForm->value;
	}
	return false;
}

const bool Settings::IsFastTravelTypeEnabled(const std::string& a_travelType) const
{
	if (a_travelType == "Map" && bMapEncounters) {
		return true;
	}
	else if (a_travelType == "Carriage" && bCarriageEncounters) {
		return true;
	}
	else if (a_travelType == "Ferry" && bFerryEncounters) {
		return true;
	}
	else if (a_travelType == "Other" && bOtherEncounters) {
		return true;
	}
	return false;
}
