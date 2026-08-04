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

	void DisplayMessageBox();
	void SetupCurrentEncounterData(std::string a_fastTravelType);

private:
	std::function<void(std::uint8_t)> callback;

	struct CurrentEncounterData
	{
		std::string title = "";
		std::string message = "";
		json choices;
		std::vector<std::string> outcomes = {};
		bool exit = false;
	};
	static CurrentEncounterData currentEncounterData;
	
	void SetupNextMessageBox(std::uint8_t a_button);
};
