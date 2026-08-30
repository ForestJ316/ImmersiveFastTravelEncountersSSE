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

void MessageBoxHandler::SetupNextMessageBox(std::uint8_t a_button, json a_encounter)
{
	auto& nestedRandomsRef = currentEncounterData.nestedRandoms;
	auto& randomRef = currentEncounterData.random;
	auto& dualRandomRef = currentEncounterData.dualRandom;

	json pickedChoice = a_encounter;
	std::string successStr = "Success";
	bool randomizedChecksDone = false;

	if (a_encounter.empty()) {
		// Check for any nested randoms first and set the values appropriately
		try {
			if (!nestedRandomsRef.empty()) {
				const auto& [choiceRandom, choiceDualRandom] = nestedRandomsRef.at(a_button);
				randomRef = choiceRandom;
				dualRandomRef = choiceDualRandom;
			}
		}
		catch (...) {}
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
		// Skip the first SetRandomizedNumbers if it's a nested random
		// Rest will be checked against further nested Randomized/DualRandomized
		if (!nestedRandomsRef.empty()) {
			ReplaceRandomizedStrings(check, randomRef, dualRandomRef);
			nestedRandomsRef.clear();
		}
		else {
			SetRandomizedNumbers(pickedChoice, randomRef, dualRandomRef);
			ReplaceRandomizedStrings(check, randomRef, dualRandomRef);
		}
		bool bSuccess = Functions::DoFunction<bool>(check, "Check");
		successStr = bSuccess ? "Success" : "Failure";
		if (pickedChoice.contains(successStr)) {
			pickedChoice = pickedChoice[successStr];
		}
		else {
			randomizedChecksDone = true;
			break;
		}
	}
	// Case when there was no "Check", but has a "Randomized" or "DualRandomized" in the parent, set the random values now
	// Make sure to not re-set the random values set by the previous iteration in choice
	if (!randomizedChecksDone && nestedRandomsRef.empty()) {
		SetRandomizedNumbers(pickedChoice, randomRef, dualRandomRef);
	}
	if (pickedChoice.contains(successStr)) {
		// If there is a "Randomized" or "DualRandomized" in the successStr then set it now
		SetRandomizedNumbers(pickedChoice[successStr], randomRef, dualRandomRef);
		pickedChoice = pickedChoice[successStr];
	}
	if (!pickedChoice.empty()) {
		// Title, optional
		currentEncounterData.title = "";
		SetupNextTitle(pickedChoice, currentEncounterData);
		// Outcomes, optional
		// Do before message for cases with %item strings
		SetupNextOutcomes(pickedChoice, currentEncounterData);
		// Message, mandatory to advance the encounter. Still check in case of user error
		currentEncounterData.message = "";
		SetupNextMessage(pickedChoice, currentEncounterData);
		// Choice/Choices, optional. Will just be an "Ok" button if no key
		SetupNextChoices(pickedChoice, currentEncounterData);
		// If there is no message then exit the encounter...
		if (string::is_empty(currentEncounterData.message.c_str())) {
			currentEncounterData.choices.clear();
			currentEncounterData.exit = true;
			Run(0);
		}
	}
	// Fail-safe in case the "Check" fails or the fields are wrong names
	// Also case for if there is a custom exit button in "Choices" without any further MessageBoxes
	else {
		ResetCurrentEncounterData();
	}
}

void MessageBoxHandler::SetupNextTitle(const json& a_json, CurrentEncounterData& a_encounterData)
{
	if (a_json.contains("Title") && a_json["Title"].is_string()) {
		a_encounterData.title = a_json["Title"];
		ReplaceRandomizedStrings(a_encounterData.title, a_encounterData.random, a_encounterData.dualRandom);
	}
}

void MessageBoxHandler::SetupNextOutcomes(const json& a_json, CurrentEncounterData& a_encounterData)
{
	if (a_json.contains("Outcomes") && a_json["Outcomes"].is_array()) {
		auto& outcomesRef = a_encounterData.outcomes;
		auto& storedItemsRef = a_encounterData.storedItems;
		// Store outcomes and perform them after exiting the MessageBox menu
		for (json::const_iterator it = a_json["Outcomes"].begin(); it != a_json["Outcomes"].end(); ++it) {
			if (it.value().is_string()) {
				auto outcome = it.value().get<std::string>();
				ReplaceRandomizedStrings(outcome, a_encounterData.random, a_encounterData.dualRandom);
				outcomesRef.emplace_back(outcome);
				// Special case for AddItem, RemoveItem, AddRandomItem
				// Store the selected items now, adding to inventory will be done upon exit
				if (std::ranges::any_of(Functions::ITEM_FUNCTIONS, [&outcome](const auto& a_func) { return outcome.contains(a_func); })) {
					storedItemsRef.insert_range(storedItemsRef.end(), Functions::DoFunction<StoredItemType>(outcome, "Outcome"));
					if (storedItemsRef.size() == 0) {
						storedItemsRef.emplace_back(nullptr, 1);
					}
				}
			}
		}
	}
}

void MessageBoxHandler::SetupNextMessage(json& a_json, CurrentEncounterData& a_encounterData)
{
	if (a_json.contains("Message") && a_json["Message"].is_string()) {
		a_encounterData.message = a_json["Message"];
		ReplaceRandomizedStrings(a_encounterData.message, a_encounterData.random, a_encounterData.dualRandom);
		// Check if strings in message need replacing
		if (a_encounterData.storedItems.size() > 0 && a_encounterData.message.contains("%item")) {
			ReplaceItemStrings(a_encounterData.message, a_encounterData.storedItems);
		}
	}
}

void MessageBoxHandler::SetupNextChoices(json& a_json, CurrentEncounterData& a_encounterData)
{
	if (a_json.contains("Choices") && a_json["Choices"].is_array()) {
		a_encounterData.nestedRandoms.clear();
		a_encounterData.choices = a_json["Choices"];
		// Check for any randomized values
		for (json::iterator choice = a_encounterData.choices.begin(); choice != a_encounterData.choices.end(); ++choice) {
			if (choice.value().contains("Choice") && choice.value()["Choice"].is_string()) {
				auto& choiceStr = choice.value()["Choice"].get_ref<std::string&>();
				SetRandomizedNumbers(*choice, a_encounterData.random, a_encounterData.dualRandom);
				ReplaceRandomizedStrings(choiceStr, a_encounterData.random, a_encounterData.dualRandom);
				// Store the randoms for later
				a_encounterData.nestedRandoms.emplace_back(a_encounterData.random, a_encounterData.dualRandom);
			}
		}
	}
	else {
		json jsonExitButton;
		jsonExitButton[""] = { {"Choice", "Ok"} };
		// Custom text for the exit button set by the user
		if (a_json.contains("Choice") && a_json["Choice"].is_string()) {
			auto& choice = a_json["Choice"].get_ref<std::string&>();
			ReplaceRandomizedStrings(choice, a_encounterData.random, a_encounterData.dualRandom);
			jsonExitButton[""] = { {"Choice", choice} };
		}
		a_encounterData.choices = jsonExitButton;
		a_encounterData.exit = true;
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

void MessageBoxHandler::SetRandomizedNumbers(const json& a_json, int& a_random, std::pair<int, int>& a_dualRandom)
{
	if (a_json.contains("Randomized") && a_json["Randomized"].is_string()) {
		a_random = Functions::DoFunction<int>(a_json["Randomized"].get<std::string>(), "Randomized");
	}
	if (a_json.contains("DualRandomized") && a_json["DualRandomized"].is_string()) {
		a_dualRandom = Functions::DoFunction<std::pair<int, int>>(a_json["DualRandomized"].get<std::string>(), "DualRandomized");
	}
}

void MessageBoxHandler::ReplaceRandomizedStrings(std::string& a_text, const int& a_random, const std::pair<int, int>& a_dualRandom)
{
	const auto& [dualRandom1, dualRandom2] = a_dualRandom;
	// replace_all has a check whether the a_search argument is in the string
	string::replace_all(a_text, "%random", std::to_string(a_random));	
	string::replace_all(a_text, "%dualRandom1", std::to_string(dualRandom1));
	string::replace_all(a_text, "%dualRandom2", std::to_string(dualRandom2));
}

void MessageBoxHandler::ReplaceItemStrings(std::string& a_message, const StoredItemType& a_itemList)
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
		std::vector<json> validEncounters;
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
		std::erase_if(validEncounters, [&survivalEnabled](json& encounter) {
			if (encounter.contains("Survival") && encounter["Survival"].is_boolean()) {
				const auto& encounterSurvival = encounter["Survival"].get_ref<bool&>();
				if ((!survivalEnabled && encounterSurvival) || (survivalEnabled && !encounterSurvival)) {
					return true;
				}
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
			if (randomEncounter.contains("SoundFX") && randomEncounter["SoundFX"].is_string()) {
				const auto& soundFX = randomEncounter["SoundFX"].get_ref<std::string&>();
				if (!string::is_empty(soundFX.c_str())) {
					SetupEncounterSoundFX(soundFX);
				}
			}
			currentEncounterData.isSetup = true;
			SetupNextMessageBox(0, randomEncounter);
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
		Functions::ResetVars();
	}
}
