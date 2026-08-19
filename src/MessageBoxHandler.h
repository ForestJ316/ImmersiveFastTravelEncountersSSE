#pragma once

// Credit to https://github.com/SkyrimScripting/MessageBox
// for the IMessageBoxCallback functionality
class MessageBoxHandler : public RE::IMessageBoxCallback
{
private:
	struct CurrentEncounterData;

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
	// Check for "Randomized" and "DualRandomized" conditions to replace the %random, %dualRandom1, %dualRandom2 strings
	void SetRandomizedValues(json a_jsonObj, std::string& a_textStr, int& a_iRandom, std::pair<int, int>& a_iDualRandom, bool a_alreadyDone = false);
	// Case for "AddRandomItem" function where there might be %item1 etc. strings in the message
	void SetRandomItemStrings(const std::vector<std::pair<RE::TESForm*, std::int32_t>>& a_itemList, std::string& a_message);
	// Play (or setup) a Sound FX if it's specified on start of the encounter
	void PlayEncounterSoundFX(std::string a_soundPath, bool a_setup = false);

	RE::BSSoundHandle soundHandle = {};
	// Store the randomized values to not overwrite the values for nested cases
	int iRandom = 0;
	std::pair<int, int> iDualRandom = {0, 0};
	std::vector<std::pair<RE::TESForm*, std::int32_t>> randomItemList = {};

	struct CurrentEncounterData
	{
		bool isSetup = false;
		std::string title = "";
		std::string message = "";
		json choices;
		std::string soundFX = "";
		std::vector<std::string> outcomes = {};
		bool exit = false;
	};
	static CurrentEncounterData currentEncounterData;
};
