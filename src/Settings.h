#pragma once

class Settings
{
private:
	friend class MessageBoxHandler;

	struct CachedEncounterData;
	typedef std::unordered_map<std::string, std::unordered_map<std::string, std::vector<CachedEncounterData>>> CachedDataType;

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
	const bool IsValidHold(const std::string& a_hold) const;

	static inline bool bIsExperienceModActive = false;

	static inline std::int16_t iEncounterChance = 25;
	static inline float fMinimumDistance = 250.0f;

	static inline RE::BGSSoundCategory* sound_FXCategory = nullptr;
	static inline RE::BGSSoundOutput* sound_FXOutput = nullptr;
	// Survival Mode needs
	static inline RE::TESGlobal* survival_HungerCurrent = nullptr;
	static inline RE::TESGlobal* survival_HungerMax = nullptr;
	static inline RE::TESGlobal* survival_ExhaustionCurrent = nullptr;
	static inline RE::TESGlobal* survival_ExhaustionMax = nullptr;
	static inline RE::TESGlobal* survival_ColdCurrent = nullptr;
	static inline RE::TESGlobal* survival_ColdMax = nullptr;

private:
	void SetFastTravelEncounters(std::string a_type, std::vector<std::string> a_encounterHolds, const json::const_iterator& a_encounter);
	void InitializeSettings();
	void InitializeEncounterCache();
	void InitializeActivatorCache();

	void InitializeForms();
	void CheckIsExperienceModInstalled();
	
	std::int16_t iDebugEncounter = 0;
	std::vector<std::string> holds = {};

	struct CachedEncounterData
	{
		std::string title = "";
		std::string message = "";
		json choices;
		std::string soundFX = "";
		std::optional<bool> survival = std::nullopt;
	};
	// EncounterData structure: [Fast Travel Type][Encounter Conditions].Encounter Values()
	static CachedDataType EncounterCache;
	// Source Activators Cache structure: Key - Valid Base Object, Value - Fast Travel Type
	static std::unordered_map<RE::FormID, std::string> FastTravelActivatorCache;
};
