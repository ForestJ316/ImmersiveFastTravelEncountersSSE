#pragma once

// Credit to https://github.com/SkyrimScripting/MessageBox
// for the IMessageBoxCallback functionality
class MessageBoxHandler : public RE::IMessageBoxCallback
{
private:
	struct CurrentEncounterData;
	using StoredItemType = std::vector<std::pair<RE::TESForm*, std::int32_t>>;
	using NestedRandomType = std::vector<std::pair<int, std::pair<int, int>>>;
	
public:
	static MessageBoxHandler* GetSingleton()
	{
		static MessageBoxHandler singleton;
		return std::addressof(singleton);
	}
	MessageBoxHandler(std::function<void(std::uint8_t)> a_callback = {}) : callback(a_callback) {};
	~MessageBoxHandler() override {};
	void Run(std::uint8_t a_button) override;
	void Show(const std::string& a_bodyText, const std::vector<std::string>& a_buttonText, std::function<void(std::uint8_t)> a_callback);

	void DisplayMessageBox(bool a_init = false);
	void SetupCurrentEncounterData(const std::string& a_travelType);

	const CurrentEncounterData& GetCurrentEncounter() const;
	void ResetCurrentEncounterData();

private:
	std::function<void(std::uint8_t)> callback;

	void SetupNextMessageBox(std::uint8_t a_button, json a_encounter = {});
	// Optional
	void SetupNextTitle(const json& a_json, CurrentEncounterData& a_encounterData);
	// Optional
	void SetupNextOutcomes(const json& a_json, CurrentEncounterData& a_encounterData);
	// Mandatory, but fall-back to empty string
	void SetupNextMessage(json& a_json, CurrentEncounterData& a_encounterData);
	// Optional, provide means to specify custom text for the exit button. Fall-back to "Ok" button
	void SetupNextChoices(json& a_json, CurrentEncounterData& a_encounterData);
	// Setup a Sound FX if it's specified on start of the encounter
	void SetupEncounterSoundFX(std::string a_soundPath);

	// Check for "Randomized" and "DualRandomized" conditions to replace the %random, %dualRandom1, %dualRandom2 strings
	void SetRandomizedNumbers(const json& a_json, int& a_random, std::pair<int, int>& a_dualRandom);
	void ReplaceRandomizedStrings(std::string& a_text, const int& a_random, const std::pair<int, int>& a_dualRandom);
	// Case for "AddRandomItem" function where there might be %item1 etc. strings in the message
	void ReplaceItemStrings(std::string& a_message, const StoredItemType& a_itemList);

	RE::BSSoundHandle soundHandle = {};

	struct CurrentEncounterData
	{
		bool isSetup = false;
		std::string title = "";
		std::string message = "";
		json choices = {};
		std::vector<std::string> outcomes = {};
		// Store the randomized values to not overwrite the values for nested cases
		int random = 0;
		std::pair<int, int> dualRandom = { 0, 0 };
		StoredItemType storedItems = {};
		NestedRandomType nestedRandoms = {};
		bool exit = false;
	};
	static CurrentEncounterData CurrentEncounter;
};
