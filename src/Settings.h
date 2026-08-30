#pragma once

class Settings
{
private:
	using CachedDataType = std::unordered_map<std::string, std::unordered_map<std::string, std::vector<json>>>;

public:
	static Settings* GetSingleton()
	{
		static Settings singleton;
		return std::addressof(singleton);
	}
	void Initialize();
	const CachedDataType& GetEncounterCache() const;
	const std::unordered_map<RE::FormID, std::string>& GetFastTravelActivatorCache() const;
	const bool IsSurvivalEnabled() const;
	const bool IsFastTravelTypeEnabled(const std::string& a_fastTravelType) const;

	static inline bool bIsExperienceModActive = false;

	static inline std::int16_t iEncounterChance = 25;
	// Called iMinimumDistance in the ini for MCM
	static inline float fMinimumDistance = 250.0f;
	bool bMapEncounters = true;
	bool bCarriageEncounters = true;
	bool bFerryEncounters = true;
	bool bOtherEncounters = true;

	static inline RE::BGSSoundCategory* sound_FXCategory = nullptr;
	static inline RE::BGSSoundOutput* sound_FXOutput = nullptr;
	// Survival Mode needs
	static inline RE::TESGlobal* survival_HungerCurrent = nullptr;
	static inline RE::TESGlobal* survival_HungerMax = nullptr;
	static inline RE::TESGlobal* survival_ExhaustionCurrent = nullptr;
	static inline RE::TESGlobal* survival_ExhaustionMax = nullptr;
	static inline RE::TESGlobal* survival_ColdCurrent = nullptr;
	static inline RE::TESGlobal* survival_ColdMax = nullptr;

	template <typename T>
	void OnSettingChanged(const std::string& a_settingName, const T& a_settingValue)
	{
		if constexpr (std::is_same_v<T, int>) {
			// Make sure to not overwrite iEncounterChance in case there is a forced encounter
			if (a_settingName == "iEncounterChance" && iDebugEncounter.second <= 0) {
				iEncounterChance = static_cast<std::int16_t>(a_settingValue);
			}
			else if (a_settingName == "iMinimumDistance") {
				fMinimumDistance = static_cast<float>(a_settingValue);
			}
		}
		else if constexpr (std::is_same_v<T, bool>) {
			if (a_settingName == "bMapEncounters") {
				bMapEncounters = static_cast<bool>(a_settingValue);
			}
			else if (a_settingName == "bCarriageEncounters") {
				bCarriageEncounters = static_cast<bool>(a_settingValue);
			}
			else if (a_settingName == "bFerryEncounters") {
				bFerryEncounters = static_cast<bool>(a_settingValue);
			}
			else if (a_settingName == "bOtherEncounters") {
				bOtherEncounters = static_cast<bool>(a_settingValue);
			}
		}
	}

private:
	void SetFastTravelEncounters(std::string a_type, std::vector<std::string> a_encounterHolds, const json::const_iterator& a_encounter);
	void InitializeSettings();
	void InitializeEncounterCache();
	void InitializeActivatorCache();

	void InitializeGlobals();
	
	std::pair<std::string, std::int16_t> iDebugEncounter = { "Encounters.json", 0 };
	
	// EncounterData structure: [Fast Travel Type][Encounter Conditions].Encounter Values()
	static CachedDataType EncounterCache;
	// Source Activators Cache structure: Key - Valid Base Object, Value - Fast Travel Type
	static std::unordered_map<RE::FormID, std::string> FastTravelActivatorCache;
};
