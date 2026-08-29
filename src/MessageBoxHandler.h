#pragma once

// Credit to https://github.com/SkyrimScripting/MessageBox
// for the IMessageBoxCallback functionality
class MessageBoxHandler : public RE::IMessageBoxCallback
{
private:
	struct CurrentEncounterData;
	using StoredItemType = std::vector<std::pair<RE::TESForm*, std::int32_t>>;
	using NestedChoiceType = std::vector<std::pair<int, std::pair<int, int>>>;
	
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
	void SetupCurrentEncounterData(const std::string& a_fastTravelType);

	const CurrentEncounterData& GetCurrentEncounterData() const;
	void ResetCurrentEncounterData();

private:
	std::function<void(std::uint8_t)> callback;

	void SetupNextMessageBox(std::uint8_t a_button);
	void SetupNextTitle(const json& a_json, std::string& a_currentTitle);
	void SetupNextOutcomes(const json& a_json, std::vector<std::string>& a_currentOutcomes, const int& a_iRandom, const std::pair<int, int>& a_iDualRandom, StoredItemType& a_storedItems);
	void SetupNextMessage(json& a_json, std::string& a_currentMessage, const int& a_iRandom, const std::pair<int, int>& a_iDualRandom, StoredItemType& a_storedItems);
	void SetupNextChoices(json& a_json, CurrentEncounterData& a_currentEncData, int& a_iRandom, std::pair<int, int>& a_iDualRandom, NestedChoiceType& a_nestedChoices);
	// Play (or setup) a Sound FX if it's specified on start of the encounter
	void SetupEncounterSoundFX(std::string a_soundPath);

	RE::BSSoundHandle soundHandle = {};
	// Store the randomized values to not overwrite the values for nested cases
	int iRandom = 0;
	std::pair<int, int> iDualRandom = { 0, 0 };
	StoredItemType storedItems = {};
	NestedChoiceType nestedChoices = {};

	struct CurrentEncounterData
	{
		bool isSetup = false;
		std::string title = "";
		std::string message = "";
		json choices;
		std::vector<std::string> outcomes = {};
		bool exit = false;
	};
	static CurrentEncounterData currentEncounterData;
};
