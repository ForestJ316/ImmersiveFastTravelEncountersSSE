#include "MessageBoxHandler.h"

#include "Settings.h"
#include "Functions.h"
#include "FastTravelHandler.h"

#include <algorithm>

MessageBoxHandler::CurrentEncounterData MessageBoxHandler::currentEncounterData = {};

void MessageBoxHandler::Run(std::uint8_t a_button)
{
	if (currentEncounterData.exit) {
		// Do all outcomes upon exiting the MessageBox menu
		for (const auto& outcome : currentEncounterData.outcomes) {
			Functions::DoFunction<void>(outcome, "Outcome");
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
		const auto a_factoryManager = RE::MessageDataFactoryManager::GetSingleton();
		const auto a_uiStringHolder = RE::InterfaceStrings::GetSingleton();
		auto factory = a_factoryManager->GetCreator<RE::MessageBoxData>(a_uiStringHolder->messageBoxData);
		auto messageBox = factory->Create();
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
				buttonText.emplace_back(choice.value()["Choice"].get<std::string>());
			}
		}
		Show(bodyText, buttonText, [&](std::uint8_t a_button) {
			SetupNextMessageBox(a_button);
		});
		// Check for a Sound Handle and play the sound if found, only at the beginning
		if (a_init && soundHandle.IsValid()) {
			// If the file path was invalid when setting up no sound will play
			soundHandle.Play();
			soundHandle = {};
		}
	}
}

void MessageBoxHandler::SetupNextMessageBox(std::uint8_t a_button)
{
	const auto a_messageBoxHandler = MessageBoxHandler::GetSingleton();
	auto& iRandomRef = a_messageBoxHandler->iRandom;
	auto& iDualRandomRef = a_messageBoxHandler->iDualRandom;
	auto& storedItemsRef = a_messageBoxHandler->storedItems;
	auto& nestedChoicesRef = a_messageBoxHandler->nestedChoices;

	json pickedChoice;
	std::string successStr = "Success";
	bool randomizedChecksDone = false;

	// Check for nested choices first and set the random values appropriately
	if (nestedChoicesRef.size() >= static_cast<std::uint16_t>(a_button + 1)) {
		const auto& [choiceRandom, choiceDualRandom] = nestedChoicesRef.at(a_button);
		iRandomRef = choiceRandom;
		iDualRandomRef = choiceDualRandom;
	}

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
		// Skip the first one if it's a nested choice, rest will be checked against further nested Randomized/DualRandomized
		if (!nestedChoicesRef.empty()) {
			ReplaceRandomizedStrings(check, iRandomRef, iDualRandomRef);
			nestedChoicesRef.clear();
		}
		else {
			SetRandomizedNumbers(pickedChoice, iRandomRef, iDualRandomRef);
			ReplaceRandomizedStrings(check, iRandomRef, iDualRandomRef);
		}
		bool bSuccess = Functions::DoFunction<bool>(check, "Check");
		successStr = bSuccess ? "Success" : "Failure";
		if (pickedChoice.contains(successStr) && pickedChoice[successStr].contains("Check") && pickedChoice[successStr]["Check"].is_string()) {
			pickedChoice = pickedChoice[successStr];
		}
		else {
			randomizedChecksDone = true;
			break;
		}
	}
	// Case when there was no "Check", but has a "Randomized" or "DualRandomized" in the parent, set the random values now
	if (!randomizedChecksDone) {
		SetRandomizedNumbers(pickedChoice, iRandomRef, iDualRandomRef);
	}
	if (pickedChoice.contains(successStr)) {
		// If there is a "Randomized" or "DualRandomized" in the successStr then set it now
		SetRandomizedNumbers(pickedChoice[successStr], iRandomRef, iDualRandomRef);
		// Title, optional
		currentEncounterData.title = "";
		SetupNextTitle(pickedChoice[successStr], currentEncounterData.title);
		// Outcomes, optional
		// Do before message for cases with %item strings
		SetupNextOutcomes(pickedChoice[successStr], currentEncounterData.outcomes, iRandomRef, iDualRandomRef, storedItemsRef);
		// Message, mandatory. Still check in case of user error
		currentEncounterData.message = "";
		SetupNextMessage(pickedChoice[successStr], currentEncounterData.message, iRandomRef, iDualRandomRef, storedItemsRef);
		// Choice/Choices, optional. Will just be an "Ok" button if no key
		SetupNextChoices(pickedChoice[successStr], currentEncounterData, iRandomRef, iDualRandomRef, nestedChoicesRef);
	}
	// Fail-safe in case the "Check" fails or the fields are wrong names
	// Also case for if there is a custom exit button in "Choices" without any further MessageBoxes
	else {
		ResetCurrentEncounterData();
	}
}

void MessageBoxHandler::SetupNextTitle(const json& a_json, std::string& a_currentTitle)
{
	if (a_json.contains("Title") && a_json["Title"].is_string()) {
		a_currentTitle = a_json["Title"];
	}
}

void MessageBoxHandler::SetupNextOutcomes(const json& a_json, std::vector<std::string>& a_currentOutcomes, const int& a_iRandom, const std::pair<int, int>& a_iDualRandom, StoredItemType& a_storedItems)
{
	if (a_json.contains("Outcomes") && a_json["Outcomes"].is_array()) {
		// Store outcomes and perform them after exiting the MessageBox menu
		for (json::const_iterator it = a_json["Outcomes"].begin(); it != a_json["Outcomes"].end(); ++it) {
			if (it.value().is_string()) {
				auto outcome = it.value().get<std::string>();
				ReplaceRandomizedStrings(outcome, a_iRandom, a_iDualRandom);
				a_currentOutcomes.emplace_back(outcome);
				// Special case for AddItem, RemoveItem, AddRandomItem
				// Store the selected items now, adding to inventory will be done upon exit
				if (std::ranges::any_of(Functions::ITEM_FUNCTIONS, [&outcome](const auto& a_func) { return outcome.contains(a_func); })) {
					a_storedItems.insert_range(a_storedItems.end(), Functions::DoFunction<StoredItemType>(outcome, "Outcome"));
					if (a_storedItems.size() == 0) {
						a_storedItems.emplace_back(nullptr, 1);
					}
				}
			}
		}
	}
}

void MessageBoxHandler::SetupNextMessage(json& a_json, std::string& a_currentMessage, const int& a_iRandom, const std::pair<int, int>& a_iDualRandom, StoredItemType& a_storedItems)
{
	if (a_json.contains("Message") && a_json["Message"].is_string()) {
		a_currentMessage = a_json["Message"];
		ReplaceRandomizedStrings(a_currentMessage, a_iRandom, a_iDualRandom);
		// Check if strings in message need replacing
		if (a_storedItems.size() > 0 && a_currentMessage.contains("%item")) {
			ReplaceItemStrings(a_storedItems, a_currentMessage);
		}
	}
}

void MessageBoxHandler::SetupNextChoices(json& a_json, CurrentEncounterData& a_currentEncData, int& a_iRandom, std::pair<int, int>& a_iDualRandom, NestedChoiceType& a_nestedChoices)
{
	if (a_json.contains("Choices") && a_json["Choices"].is_array()) {
		a_nestedChoices.clear();
		a_currentEncData.choices = a_json["Choices"];
		// Check for any randomized values
		for (json::iterator choice = a_currentEncData.choices.begin(); choice != a_currentEncData.choices.end(); ++choice) {
			if (choice.value().contains("Choice") && choice.value()["Choice"].is_string()) {
				auto& choiceStr = choice.value()["Choice"].get_ref<std::string&>();
				SetRandomizedNumbers(*choice, a_iRandom, a_iDualRandom);
				ReplaceRandomizedStrings(choiceStr, a_iRandom, a_iDualRandom);
				// Store the nested choices for later
				a_nestedChoices.emplace_back(a_iRandom, a_iDualRandom);
			}
		}
	}
	else {
		json jsonExitButton;
		jsonExitButton[""] = { {"Choice", "Ok"} };
		// Custom text for the exit button set by the user
		if (a_json.contains("Choice") && a_json["Choice"].is_string()) {
			auto& choice = a_json["Choice"].get_ref<std::string&>();
			ReplaceRandomizedStrings(choice, a_iRandom, a_iDualRandom);
			jsonExitButton[""] = { {"Choice", choice} };
		}
		a_currentEncData.choices = jsonExitButton;
		a_currentEncData.exit = true;
	}
}

void MessageBoxHandler::SetupEncounterSoundFX(std::string a_soundPath)
{
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

void MessageBoxHandler::SetRandomizedNumbers(const json& a_json, int& a_iRandom, std::pair<int, int>& a_iDualRandom)
{
	if (a_json.contains("DualRandomized") && a_json["DualRandomized"].is_string()) {
		a_iDualRandom = Functions::DoFunction<std::pair<int, int>>(a_json["DualRandomized"].get<std::string>(), "DualRandomized");
	}
	if (a_json.contains("Randomized") && a_json["Randomized"].is_string()) {
		a_iRandom = Functions::DoFunction<int>(a_json["Randomized"].get<std::string>(), "Randomized");
	}
}

void MessageBoxHandler::ReplaceRandomizedStrings(std::string& a_text, const int& a_iRandom, const std::pair<int, int>& a_iDualRandom)
{
	// dualRandom first so we don't risk replacing %dualRandom1 and %dualRandom2 strings in case both keys are included
	// replace_all has a check whether the a_search argument is in the string
	const auto& [dualRandom1, dualRandom2] = a_iDualRandom;
	string::replace_all(a_text, "%dualRandom1", std::to_string(dualRandom1));
	string::replace_all(a_text, "%dualRandom2", std::to_string(dualRandom2));
	string::replace_all(a_text, "%random", std::to_string(a_iRandom));
}

void MessageBoxHandler::ReplaceItemStrings(const std::vector<std::pair<RE::TESForm*, std::int32_t>>& a_itemList, std::string& a_message)
{
	for (auto i = 0; i < a_itemList.size(); ++i) {
		auto itemStr = std::format("%item{:d}", i + 1);
		if (a_message.contains(itemStr)) {
			const auto& [a_item, a_amount] = a_itemList.at(i);
			auto itemName = a_item ? a_item->GetName() : "";
			if (!string::is_empty(itemName)) {
				// Amount, Name
				string::replace_all(a_message, itemStr, std::format("{} {}", a_amount, itemName));
			}
			// If there is something wrong with the function or input then just remove the %item{:d} string
			else {
				string::replace_all(a_message, itemStr, "");
			}
		}
	}
}

void MessageBoxHandler::SetupCurrentEncounterData(const std::string& a_fastTravelType)
{
	ResetCurrentEncounterData(); // In case there was a non-regular exit from the previous encounter

	if (string::is_empty(a_fastTravelType.c_str())) {
		return;
	}
	// Roll the chance to show an encounter based on the setting
	auto randomPercent = clib_util::RNG().generate<std::uint16_t>(1, 100);
	if (Settings::iEncounterChance == 0 || randomPercent > Settings::iEncounterChance) {
		return;
	}
	const auto a_settings = Settings::GetSingleton();
	const auto& EncounterCache = a_settings->GetEncounterCache();
	if (const auto& encounters = EncounterCache.find(a_fastTravelType); encounters != EncounterCache.end()) {
		std::vector<Settings::CachedEncounterData> validEncounters;
		auto& nearestCellWithLocation = FastTravelHandler::GetSingleton()->GetNearestCellWithLocation();
		for (const auto& [encCondition, encData] : encounters->second) {
			// First - Empty string, since it's valid for everything of this fast travel type
			if (encCondition == "") {
				validEncounters.insert_range(validEncounters.end(), encData);
			}
			// Second - Check if player is in a valid hold
			else if (utils::GetCellIsInLocation(nearestCellWithLocation, encCondition)) {
				validEncounters.insert_range(validEncounters.end(), encData);
				// We don't care about the check after this, therefore reset the cell object
				nearestCellWithLocation = nullptr;
			}
			// TODO (maybe if there is a use-case)
			// Third - Check activator
		}
		const auto survivalEnabled = a_settings->IsSurvivalEnabled();
		// If Survival Mode is on, remove all non-Survival Mode encounters
		// If Survival Mode is off, remove all Survival Mode encounters
		std::erase_if(validEncounters, [&survivalEnabled](Settings::CachedEncounterData& encounter) {
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
				SetupEncounterSoundFX(randomEncounter.soundFX);
			}
			currentEncounterData = { true, randomEncounter.title, randomEncounter.message, randomEncounter.choices };
		}
	}
}

const MessageBoxHandler::CurrentEncounterData& MessageBoxHandler::GetCurrentEncounterData() const
{
	return currentEncounterData;
}

void MessageBoxHandler::ResetCurrentEncounterData()
{
	const auto a_messageBoxHandler = MessageBoxHandler::GetSingleton();
	if (a_messageBoxHandler->currentEncounterData.isSetup) {
		a_messageBoxHandler->currentEncounterData = {};
		a_messageBoxHandler->soundHandle = {};
		a_messageBoxHandler->iRandom = 0;
		a_messageBoxHandler->iDualRandom = { 0, 0 };
		a_messageBoxHandler->storedItems.clear();
		a_messageBoxHandler->nestedChoices.clear();
		Functions::ResetVars();
	}
}
