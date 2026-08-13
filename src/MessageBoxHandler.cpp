#include "MessageBoxHandler.h"

#include "FastTravelHandler.h"
#include "Settings.h"
#include "Functions.h"
#include "Utils.h"

MessageBoxHandler::CurrentEncounterData MessageBoxHandler::currentEncounterData = {};

void MessageBoxHandler::Run(std::uint8_t a_button)
{
	if (currentEncounterData.exit) {
		// Do all outcomes upon exiting the MessageBox menu
		for (const auto& outcome : currentEncounterData.outcomes) {
			Functions::DoFunction(outcome, "Outcome");
		}
		ResetCurrentEncounterData();
	}
	else {
		callback(static_cast<std::uint8_t>(a_button));
		DisplayMessageBox();
	}
}

void MessageBoxHandler::Show(const std::string& a_bodyText, const std::vector<std::string>& a_buttonText, std::function<void(std::uint8_t)> a_callback)
{
	SKSE::GetTaskInterface()->AddTask([a_bodyText, a_buttonText, a_callback]() {
		auto* factoryManager = RE::MessageDataFactoryManager::GetSingleton();
		auto* uiStringHolder = RE::InterfaceStrings::GetSingleton();
		auto* factory = factoryManager->GetCreator<RE::MessageBoxData>(uiStringHolder->messageBoxData);
		auto* messageBox = factory->Create();
		messageBox->callback = RE::make_smart<MessageBoxHandler>(a_callback);
		messageBox->bodyText = a_bodyText;
		for (const auto& text : a_buttonText) {
			messageBox->buttonText.push_back(text.c_str());
		}
		RE::MessageBoxMenu::QueueMessage(messageBox);
	});
}

void MessageBoxHandler::DisplayMessageBox(bool a_init)
{
	// Don't show anything if there are no buttons
	if (!currentEncounterData.choices.empty()) {
		// Create bodyText
		std::string bodyText = currentEncounterData.title + "\n" + currentEncounterData.message;
		if (string::is_empty(currentEncounterData.title.c_str())) {
			bodyText = currentEncounterData.message;
		}
		std::vector<std::string> buttonText;
		for (json::iterator choice = currentEncounterData.choices.begin(); choice != currentEncounterData.choices.end(); ++choice) {
			if (choice.value().contains("Choice") && choice.value()["Choice"].is_string()) {
				buttonText.push_back(choice.value()["Choice"].get<std::string>());
			}
		}
		Show(bodyText, buttonText, [&](std::uint8_t a_button) {
			SetupNextMessageBox(a_button);
		});
		// Check for a SoundFX field and play the sound if found, only at the beginning
		if (a_init && !string::is_empty(currentEncounterData.soundFX.c_str())) {
			PlayEncounterSoundFX(currentEncounterData.soundFX);
			currentEncounterData.soundFX = "";
		}
	}
}

void MessageBoxHandler::SetupNextMessageBox(std::uint8_t a_button)
{
	json pickedChoice;
	std::string successStr = "Success";
	bool bRandomizedDone = false;

	if (!currentEncounterData.choices.empty()) {
		if (currentEncounterData.choices.is_array()) {
			pickedChoice = currentEncounterData.choices.at(a_button);
		}
		else if (currentEncounterData.choices.is_object()) {
			pickedChoice = currentEncounterData.choices;
		}
	}
	// Check for a "Check" condition, as well as any nested ones
	// Success by default without a check
	while (pickedChoice.contains("Check") && pickedChoice["Check"].is_string()) {
		auto check = pickedChoice["Check"].get<std::string>();
		SetRandomizedValues(pickedChoice, check, iRandom, iDualRandom);
		bool bSuccess = std::get<0>(Functions::DoFunction(check, "Check"));
		successStr = bSuccess ? "Success" : "Failure";
		if (pickedChoice.contains(successStr) && pickedChoice[successStr].contains("Check") && pickedChoice[successStr]["Check"].is_string()) {
			pickedChoice = pickedChoice[successStr];
		}
		else {
			bRandomizedDone = true;
			break;
		}
	}
	if (pickedChoice.contains(successStr)) {
		currentEncounterData.title = "";
		currentEncounterData.message = "";
		// Case when there was no "Check", but has a "Randomized" or "DualRandomized" in the parent, set the random values now
		if (!bRandomizedDone) {
			SetRandomizedValues(pickedChoice, currentEncounterData.message, iRandom, iDualRandom);
		}
		// Title, optional
		if (pickedChoice[successStr].contains("Title") && pickedChoice[successStr]["Title"].is_string()) {
			currentEncounterData.title = pickedChoice[successStr]["Title"];
		}
		// Outcomes, optional
		if (pickedChoice[successStr].contains("Outcomes") && pickedChoice[successStr]["Outcomes"].is_array()) {
			bool bRandomItems_flag = false;
			// Store outcomes and perform them after exiting the MessageBox menu
			for (json::const_iterator it = pickedChoice[successStr]["Outcomes"].begin(); it != pickedChoice[successStr]["Outcomes"].end(); ++it) {
				auto outcome = it.value().get<std::string>();
				SetRandomizedValues(pickedChoice[successStr], outcome, iRandom, iDualRandom, bRandomizedDone);
				bRandomizedDone = true;
				currentEncounterData.outcomes.push_back(outcome);
				// Special case for AddRandomItem
				// Store the selected items now, adding to inventory will be done upon exit
				if (outcome.contains("AddRandomItem") && !bRandomItems_flag) {
					randomItemList = std::get<3>(Functions::DoFunction(outcome, "Outcome"));
					if (randomItemList.size() == 0) {
						randomItemList.push_back({ nullptr, 1 });
					}
					// Don't check again, even if there is something wrong with the AddRandomItem input or there is a duplicate function
					bRandomItems_flag = true;
				}
			}
		}
		// Message, mandatory. Still check in case of user error
		if (pickedChoice[successStr].contains("Message") && pickedChoice[successStr]["Message"].is_string()) {
			currentEncounterData.message = pickedChoice[successStr]["Message"];
			SetRandomizedValues(pickedChoice[successStr], currentEncounterData.message, iRandom, iDualRandom, bRandomizedDone);
			bRandomizedDone = true;
			// Check if strings in message need replacing
			if (randomItemList.size() > 0 && currentEncounterData.message.contains("%item")) {
				SetRandomItemStrings(randomItemList, currentEncounterData.message);
			}
		}
		// Choice/Choices, optional. Will just be an "Ok" button if no key
		if (pickedChoice[successStr].contains("Choices") && pickedChoice[successStr]["Choices"].is_array()) {
			currentEncounterData.choices = pickedChoice[successStr]["Choices"];
		}
		else {
			json jsonExitButton;
			jsonExitButton[""] = { {"Choice", "Ok"} };
			// Custom text for the exit button set by the user
			if (pickedChoice[successStr].contains("Choice") && pickedChoice[successStr]["Choice"].is_string()) {
				jsonExitButton[""] = { {"Choice", pickedChoice[successStr]["Choice"]} };
			}
			currentEncounterData.choices = jsonExitButton;
			currentEncounterData.exit = true;
		}
	}
	// Fail-safe in case the "Check" fails or the fields are wrong names
	// Also case for if there is a custom exit button in "Choices" without any further MessageBoxes
	else {
		currentEncounterData = {};
	}
}

void MessageBoxHandler::SetRandomizedValues(json a_jsonObj, std::string& a_textStr, int& a_iRandom, std::pair<int, int>& a_iDualRandom, bool a_alreadyDone)
{
	// We don't want to re-run the RollRandom and/or RollDualRandom functions for Outcomes etc.
	if (!a_alreadyDone) {
		if (a_jsonObj.contains("DualRandomized") && a_jsonObj["DualRandomized"].is_string()) {
			auto dualRandomReturn = Functions::DoFunction(a_jsonObj["DualRandomized"].get<std::string>(), "DualRandomized");
			a_iDualRandom = { std::get<1>(dualRandomReturn), std::get<2>(dualRandomReturn) };
		}
		if (a_jsonObj.contains("Randomized") && a_jsonObj["Randomized"].is_string()) {
			a_iRandom = std::get<1>(Functions::DoFunction(a_jsonObj["Randomized"].get<std::string>(), "Randomized"));
		}
	}
	// DualRandomized first so we don't risk replacing %dualRandom1 and %dualRandom2 strings in case both keys are included
	// replace_all has a check whether the a_search argument is in the string
	string::replace_all(a_textStr, "%dualRandom1", std::to_string(a_iDualRandom.first));
	string::replace_all(a_textStr, "%dualRandom2", std::to_string(a_iDualRandom.second));
	string::replace_all(a_textStr, "%random", std::to_string(a_iRandom));
}

void MessageBoxHandler::SetRandomItemStrings(const std::vector<std::pair<RE::TESForm*, std::int32_t>>& a_itemList, std::string& a_message)
{
	for (auto i = 0; i < a_itemList.size(); ++i) {
		auto itemStr = std::format("%item{:d}", i + 1);
		if (a_message.contains(itemStr)) {
			auto itemName = a_itemList.at(i).first ? a_itemList.at(i).first->GetName() : "";
			if (!string::is_empty(itemName)) {
				// Amount, Name
				string::replace_all(a_message, itemStr, std::format("{} {}", a_itemList.at(i).second, itemName));
			}
			// If there is something wrong with the AddRandomItem function or input then just remove the %item{:d} string
			else {
				string::replace_all(a_message, itemStr, "");
			}
		}
	}
}


void MessageBoxHandler::PlayEncounterSoundFX(std::string a_soundPath, bool a_setup)
{
	if (a_setup) {
		// Erase Data
		if (a_soundPath.find("Data/", 0, 5) != std::string::npos || a_soundPath.find("Data\\", 0, 5) != std::string::npos) {
			a_soundPath.erase(0, 5);
		}
		RE::BSResource::ID file;
		file.GenerateFromPath(a_soundPath.c_str());
		RE::BSAudioManager::GetSingleton()->GetSoundHandleByFile(soundHandle, file, 128 | 0x20, 128);
		if (Settings::sound_FXOutput) {
			soundHandle.SetOutputModel(Settings::sound_FXOutput);
		}
		if (Settings::sound_FXCategory) {
			soundHandle.SetVolume(Settings::sound_FXCategory->GetCategoryVolume());
		}		
	}
	else {
		// If the file path was invalid when setting up no sound will play
		soundHandle.Play();
	}
}

void MessageBoxHandler::SetupCurrentEncounterData(const std::string& a_fastTravelType)
{
	if (string::is_empty(a_fastTravelType.c_str())) {
		return;
	}
	// Roll the chance to show an encounter based on the setting
	auto randomPercent = clib_util::RNG().generate<std::uint16_t>(1, 100);
	// TEST
	logger::info("encounter chance: {}, rand perc: {}", Settings::iEncounterChance, randomPercent);
	if (Settings::iEncounterChance == 0 || randomPercent > Settings::iEncounterChance) {
		return;
	}
	const auto& EncounterCache = Settings::GetSingleton()->GetEncounterCache();
	if (auto encounters = EncounterCache.find(a_fastTravelType); encounters != EncounterCache.end()) {
		std::vector<Settings::CachedEncounterData> validEncounters;
		auto& nearestCellWithLocation = FastTravelHandler::GetSingleton()->GetNearestCellWithLocation();
		for (const auto& encounter : encounters->second) {
			// First - Empty string, since it's valid for everything of this fast travel type
			if (encounter.first == "") {
				validEncounters.insert_range(validEncounters.end(), encounter.second);
			}
			// Second - Check if player is in a valid hold
			else if (Settings::GetSingleton()->IsValidHold(encounter.first) && Utils::GetCellIsInLocation(nearestCellWithLocation, encounter.first)) {
				validEncounters.insert_range(validEncounters.end(), encounter.second);
				// We don't care about the check after this, therefore reset the cell object
				nearestCellWithLocation = nullptr;
			}
			// TODO
			// Third - Check activator
		}
		auto survivalEnabled = Settings::GetSingleton()->IsSurvivalEnabled();
		// If Survival Mode is on, remove all non-Survival Mode encounters
		// If Survival Mode is off, remove all Survival Mode encounters
		std::erase_if(validEncounters, [survivalEnabled](Settings::CachedEncounterData& encounter) {
			if ((!survivalEnabled && encounter.survival == true) || (survivalEnabled && encounter.survival == false)) {
				return true;
			}
			return false;
		});
		if (validEncounters.size() > 0) {
			std::uint16_t randomEncounterPos = 0;
			if (validEncounters.size() > 1) {
				// Return a random valid encounter
				randomEncounterPos = clib_util::RNG().generate<std::uint16_t>(0, static_cast<std::uint16_t>(validEncounters.size() - 1));
			}
			auto& randomEncounter = validEncounters.at(randomEncounterPos);
			// Setup the sound fx now, but don't play it yet
			if (!string::is_empty(randomEncounter.soundFX.c_str())) {
				PlayEncounterSoundFX(randomEncounter.soundFX, true);
			}
			currentEncounterData = { randomEncounter.title, randomEncounter.message, randomEncounter.choices, randomEncounter.soundFX };
		}
	}
}

const MessageBoxHandler::CurrentEncounterData& MessageBoxHandler::GetCurrentEncounterData() const
{
	return currentEncounterData;
}

void MessageBoxHandler::ResetCurrentEncounterData()
{
	auto messageBoxHandler = MessageBoxHandler::GetSingleton();
	messageBoxHandler->currentEncounterData = {};
	messageBoxHandler->soundHandle = {};
	messageBoxHandler->iRandom = 0;
	messageBoxHandler->iDualRandom = { 0, 0 };
	messageBoxHandler->randomItemList.clear();
	Functions::ResetVars();
}
