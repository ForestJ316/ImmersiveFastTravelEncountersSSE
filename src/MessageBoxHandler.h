#pragma once

// Credit to https://github.com/SkyrimScripting/MessageBox
// for the IMessageBoxCallback functionality
class MessageBoxHandler : public RE::IMessageBoxCallback
{
public:
	static MessageBoxHandler* GetSingleton()
	{
		static MessageBoxHandler singleton;
		return std::addressof(singleton);
	}
	MessageBoxHandler(std::function<void(std::uint8_t)> a_callback = {}) : callback(a_callback) {};
	~MessageBoxHandler() override {};
	void Run(std::uint8_t a_button) override;
	void Show(const std::string& a_bodyText, std::vector<std::string> a_buttonText, std::function<void(std::uint8_t)> a_callback);

	void DisplayMessageBox(bool a_init = false);
	void SetupCurrentEncounterData(std::string a_fastTravelType);

private:
	std::function<void(std::uint8_t)> callback;

	void SetupNextMessageBox(std::uint8_t a_button);
	// Check for a "DualRandomized" and "Randomized" conditions to replace the %random strings
	void SetRandomizedValues(json a_jsonObj, std::string& a_textStr, int& a_iRandom, std::pair<int, int>& a_iDualRandom, bool a_alreadyDone = false);
	// Play a Sound FX if it's specified on start of the encounter
	void PlayEncounterSoundFX(std::string a_soundPath, bool a_setup = false);
	RE::BSSoundHandle soundHandle{};

	struct CurrentEncounterData
	{
		std::string title = "";
		std::string message = "";
		json choices;
		std::string soundFX = "";
		std::vector<std::string> outcomes = {};
		bool exit = false;
	};
	static CurrentEncounterData currentEncounterData;
	
	void ResetEncounterData();
};
