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
	CachedDataType GetEncounterCache();
	std::unordered_map<RE::FormID, std::string> GetFastTravelActivatorCache();
	bool IsSurvivalEnabled();

	bool bIsExperienceModActive = false;

	static inline int iEncounterChance = 30;
	static inline float fMinimumDistance = 300.0f;
	static inline RE::BGSSoundCategory* soundFXCategory = nullptr;
	static inline RE::BGSSoundOutput* soundFXOutput = nullptr;

private:
	void SetFastTravelEncounters(std::string a_type, std::vector<std::string> a_encounterHolds, const json::iterator a_encounter);
	void InitializeSettings();
	void InitializeEncounterCache();
	void InitializeActivatorCache();
	void CheckIsExperienceModInstalled();
	void InitializeSoundFXForms();

	int iDebugEncounter = 0;

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
