#include "MessageBoxHandler.h"

#include "FastTravelHandler.h"
#include "Settings.h"
#include "Functions.h"
#include "Utils.h"

MessageBoxHandler::CurrentEncounterData MessageBoxHandler::currentEncounterData{};

void MessageBoxHandler::Run(std::uint8_t a_button)
{
	if (currentEncounterData.exit) {
		// Do all outcomes upon exiting the MessageBox menu
		for (auto& outcome : currentEncounterData.outcomes) {
			Functions::DoFunction(outcome);
		}
		ResetEncounterData();
	}
	else {
		callback(static_cast<std::uint8_t>(a_button));
		if (!currentEncounterData.choices.empty()) {
			DisplayMessageBox();
		}
	}
}

void MessageBoxHandler::Show(const std::string& a_bodyText, std::vector<std::string> a_buttonText, std::function<void(std::uint8_t)> a_callback)
{
	SKSE::GetTaskInterface()->AddTask([a_bodyText, a_buttonText, a_callback]() {
		auto* factoryManager = RE::MessageDataFactoryManager::GetSingleton();
		auto* uiStringHolder = RE::InterfaceStrings::GetSingleton();
		auto* factory = factoryManager->GetCreator<RE::MessageBoxData>(uiStringHolder->messageBoxData);
		auto* messageBox = factory->Create();
		messageBox->callback = RE::make_smart<MessageBoxHandler>(a_callback);
		messageBox->bodyText = a_bodyText;
		for (auto& text : a_buttonText) {
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
		std::string bodyText = currentEncounterData.title + "\n\n" + currentEncounterData.message;
		if (string::is_empty(currentEncounterData.title.c_str())) {
			bodyText = currentEncounterData.message;
		}
		std::vector<std::string> buttonText;
		for (json::iterator choice = currentEncounterData.choices.begin(); choice != currentEncounterData.choices.end(); ++choice) {
			if (choice.value().contains("Choice")) {
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
	if (!currentEncounterData.choices.empty()) {
		if (currentEncounterData.choices.type() == json::value_t::array) {
			pickedChoice = currentEncounterData.choices.at(a_button);
		}
		else if (currentEncounterData.choices.type() == json::value_t::object) {
			pickedChoice = currentEncounterData.choices;
		}
	}
	int iRandom = 0;
	std::pair<int, int> iDualRandom = { 0, 0 };
	// Check for a "Check" condition, as well as any nested ones
	// Success by default without a check
	bool bSuccess = true;
	std::string successStr = "Success";
	while (pickedChoice.contains("Check")) {
		auto check = pickedChoice["Check"].get<std::string>();
		SetRandomizedValues(pickedChoice, check, iRandom, iDualRandom);
		bSuccess = std::get<0>(Functions::DoFunction(check));
		successStr = bSuccess ? "Success" : "Failure";
		if (pickedChoice.contains(successStr) && pickedChoice[successStr].contains("Check")) {
			pickedChoice = pickedChoice[successStr];
		}
		else {
			break;
		}
	}
	if (pickedChoice.contains(successStr)) {
		// Title is optional
		currentEncounterData.title = "";
		if (pickedChoice[successStr].contains("Title")) {
			currentEncounterData.title = pickedChoice[successStr]["Title"];
		}
		// Message is mandatory, but still check in case of user error
		currentEncounterData.message = "";
		bool bRandomizerDone = false;
		if (pickedChoice[successStr].contains("Message")) {
			currentEncounterData.message = pickedChoice[successStr]["Message"];
			SetRandomizedValues(pickedChoice[successStr], currentEncounterData.message, iRandom, iDualRandom);
			bRandomizerDone = true;
		}
		// Check for nested choices
		if (pickedChoice[successStr].contains("Choices")) {
			currentEncounterData.choices = pickedChoice[successStr]["Choices"];
		}
		else {
			json jsonExitButton;
			jsonExitButton[""] = { {"Choice", "Ok"} };
			// Custom text for the exit button set by the user
			if (pickedChoice[successStr].contains("Choice")) {
				jsonExitButton[""] = { {"Choice", pickedChoice[successStr]["Choice"]} };
			}
			currentEncounterData.choices = jsonExitButton;
			currentEncounterData.exit = true;
		}
		if (pickedChoice[successStr].contains("Outcomes")) {
			for (json::iterator it = pickedChoice[successStr]["Outcomes"].begin(); it != pickedChoice[successStr]["Outcomes"].end(); ++it) {
				auto outcome = it.value().get<std::string>();
				SetRandomizedValues(pickedChoice[successStr], outcome, iRandom, iDualRandom, bRandomizerDone);
				bRandomizerDone = true;
				// Store outcomes and perform them after exiting the MessageBox menu
				currentEncounterData.outcomes.push_back(outcome);
			}
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
	// We don't want to re-run the GetRandom and/or GetDualRandom functions for Outcomes etc.
	if (a_alreadyDone) {
		a_jsonObj = {};
	}
	// DualRandomized first so we don't replace %random1 and %random2 strings in case both are included
	if (a_jsonObj.contains("DualRandomized")) {
		auto dualRandomReturn = Functions::DoFunction(a_jsonObj["DualRandomized"].get<std::string>());
		a_iDualRandom = { std::get<1>(dualRandomReturn), std::get<2>(dualRandomReturn) };
	}
	// replace_all has a check whether the a_search argument is in the string
	string::replace_all(a_textStr, "%random1", std::to_string(a_iDualRandom.first));
	string::replace_all(a_textStr, "%random2", std::to_string(a_iDualRandom.second));
	if (a_jsonObj.contains("Randomized")) {
		a_iRandom = std::get<1>(Functions::DoFunction(a_jsonObj["Randomized"].get<std::string>()));
	}
	string::replace_all(a_textStr, "%random", std::to_string(a_iRandom));
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
		if (Settings::soundFXOutput) {
			soundHandle.SetOutputModel(Settings::soundFXOutput);
		}
		if (Settings::soundFXCategory) {
			soundHandle.SetVolume(Settings::soundFXCategory->GetCategoryVolume());
		}		
	}
	else {
		// If the file path was invalid when setting up no sound will play
		soundHandle.Play();
	}
}

void MessageBoxHandler::SetupCurrentEncounterData(std::string a_fastTravelType)
{
	if (string::is_empty(a_fastTravelType.c_str())) {
		return;
	}
	// Roll the chance to show an encounter based on the setting
	auto randomPercent = clib_util::RNG().generate<std::uint16_t>(1, 100);
	logger::info("encounter chance: {}, rand perc: {}", Settings::iEncounterChance, randomPercent);
	if (Settings::iEncounterChance == 0 || randomPercent > Settings::iEncounterChance) {
		return;
	}
	auto encounterCache = Settings::GetSingleton()->GetEncounterCache();
	if (auto encounters = encounterCache.find(a_fastTravelType); encounters != encounterCache.end()) {
		std::vector<Settings::CachedEncounterData> validEncounters;
		auto& nearestCellWithLocation = FastTravelHandler::GetSingleton()->GetNearestCellWithLocation();
		for (auto& encounter : encounters->second) {
			// First - Empty string, since it's valid for everything of this fast travel type
			if (encounter.first == "") {
				validEncounters.insert_range(validEncounters.end(), encounter.second);
			}
			else {
				// Second - Check if player is in a valid hold
				if (nearestCellWithLocation && Utils::GetCellIsInLocation(encounter.first, nearestCellWithLocation)) {
					validEncounters.insert_range(validEncounters.end(), encounter.second);
					// We don't care about the check after this, therefore reset the cell object
					nearestCellWithLocation = nullptr;
				}
				// TEST
				// Third - Check activator
			}
		}
		auto survivalEnabled = Settings::GetSingleton()->IsSurvivalEnabled();
		for (auto it = validEncounters.begin(); it != validEncounters.end(); ++it) {
			// If Survival Mode is off, remove all Survival Mode encounters
			if (!survivalEnabled && it->survival == true) {
				validEncounters.erase(it);
			}
			// If Survival Mode is on, remove all non-Survival Mode encounters
			else if (survivalEnabled && it->survival == false) {
				validEncounters.erase(it);
			}
		}
		if (validEncounters.size() > 0) {
			auto& randomEncounter = validEncounters.at(0);
			if (validEncounters.size() > 1) {
				// Return a random valid encounter
				auto randomEncounterPos = clib_util::RNG().generate<std::uint16_t>(0, static_cast<std::uint16_t>(validEncounters.size() - 1));
				randomEncounter = validEncounters.at(randomEncounterPos);
			}
			// Setup the sound fx now
			if (!string::is_empty(randomEncounter.soundFX.c_str())) {
				PlayEncounterSoundFX(randomEncounter.soundFX, true);
			}
			currentEncounterData = { randomEncounter.title, randomEncounter.message, randomEncounter.choices, randomEncounter.soundFX };
		}
	}
}

void MessageBoxHandler::ResetEncounterData()
{
	currentEncounterData = {};
	soundHandle = {};
	Functions::ResetActiveEffectsList();
}
