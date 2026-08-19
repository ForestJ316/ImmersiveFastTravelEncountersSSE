#include "Functions.h"

#include "Settings.h"
#include "Utils.h"
#include "MessageBoxHandler.h"

#include <algorithm>
#include <ranges>

std::vector<std::pair<RE::TESForm*, std::int32_t>> Functions::selectedRandomItems = {};
std::vector<RE::ActiveEffect*> Functions::currentActiveEffects = {};

void Functions::Initialize()
{
	logger::info("Initializing Functions...");
	const auto a_AVList = RE::ActorValueList::GetSingleton();
	for (auto& skillAV : Functions::PLAYER_SKILL_AV) {
		if (skillAV.first != a_AVList->GetActorValueName(skillAV.second)) {
			skillAV.first = a_AVList->GetActorValueName(skillAV.second);
		}
	}
	for (auto& statAV : Functions::PLAYER_STAT_AV) {
		if (statAV.first != a_AVList->GetActorValueName(statAV.second)) {
			statAV.first = a_AVList->GetActorValueName(statAV.second);
		}
	}
	const auto a_dataHandler = RE::TESDataHandler::GetSingleton();
	if (!a_dataHandler) {
		logger::error("Functions::Initialize: TESDataHandler not found.");
		return;
	}
	spell_DummyHitEvent = a_dataHandler->LookupForm<RE::SpellItem>(0x813, "ImmersiveFastTravelEncountersSSE.esp");
	spell_DummyRestoreEffect = a_dataHandler->LookupForm<RE::SpellItem>(0x815, "ImmersiveFastTravelEncountersSSE.esp");
	logger::info("...Functions done initializing.");
}

void Functions::ResetVars()
{
	selectedRandomItems.clear();
	currentActiveEffects.clear();
}

constexpr Functions::Function_Name Functions::GetFunctionHash(const std::string& a_str)
{
	if (a_str == "HasItem") return Function_Name::HasItem;
	if (a_str == "AddItem") return Function_Name::AddItem;
	if (a_str == "RemoveItem") return Function_Name::RemoveItem;
	if (a_str == "AddRandomItem") return Function_Name::AddRandomItem;
	if (a_str == "GetSkill") return Function_Name::GetSkill;
	if (a_str == "RewardSkillPercent") return Function_Name::RewardSkillPercent;
	if (a_str == "RewardPlayerXP") return Function_Name::RewardPlayerXP;
	if (a_str == "IsGreater") return Function_Name::IsGreater;
	if (a_str == "IsGreaterOrEqual") return Function_Name::IsGreaterOrEqual;
	if (a_str == "IsEqual") return Function_Name::IsEqual;
	if (a_str == "IsLessOrEqual") return Function_Name::IsLessOrEqual;
	if (a_str == "IsLess") return Function_Name::IsLess;
	if (a_str == "RollRandom") return Function_Name::RollRandom;
	if (a_str == "RollDualRandom") return Function_Name::RollDualRandom;
	if (a_str == "HasSpell") return Function_Name::HasSpell;
	if (a_str == "HasActiveSpell") return Function_Name::HasActiveSpell;
	if (a_str == "CastSpellChance") return Function_Name::CastSpellChance;
	if (a_str == "RemoveActiveSpell") return Function_Name::RemoveActiveSpell;
	if (a_str == "DamageAV") return Function_Name::DamageAV;
	if (a_str == "RestoreAV") return Function_Name::RestoreAV;
	if (a_str == "ModHungerPercent") return Function_Name::ModHungerPercent;
	if (a_str == "ModFatiguePercent") return Function_Name::ModFatiguePercent;
	if (a_str == "ModColdPercent") return Function_Name::ModColdPercent;

	return Function_Name::UNKNOWN;
}

void Functions::AddItemAndNotify(RE::TESBoundObject* a_item, std::int32_t a_amount)
{
	RE::PlayerCharacter::GetSingleton()->AddObjectToContainer(a_item, nullptr, a_amount, nullptr);
	RE::SendHUDMessage::ShowInventoryChangeMessage(a_item, a_amount, true, true, a_item->GetName());
}

RE::ActorValue Functions::GetPlayerSkillAV(const std::string& a_skillName)
{
	for (const auto& skillAV : Functions::PLAYER_SKILL_AV) {
		if (a_skillName == skillAV.first) {
			return skillAV.second;
		}
	}
	return RE::ActorValue::kNone;
}

RE::ActorValue Functions::GetPlayerStatAV(const std::string& a_statName)
{
	for (const auto& skillAV : Functions::PLAYER_STAT_AV) {
		if (a_statName == skillAV.first) {
			return skillAV.second;
		}
	}
	return RE::ActorValue::kNone;
}

std::tuple<bool, int, int, Functions::FormAndAmountType> Functions::DoFunction(const std::string& a_outcome, const std::string a_type)
{
	const auto args = Utils::GetSplitStrings(a_outcome, ",");
	switch (Functions::GetFunctionHash(args.at(0))) {
		case Function_Name::HasItem: {
			return { Functions::HasItem(args, a_type), 0, 0, {} };
		}
		case Function_Name::AddItem: {
			Functions::AddItem(args, a_type);
			break;
		}
		case Function_Name::RemoveItem: {
			Functions::RemoveItem(args, a_type);
			break;
		}
		case Function_Name::AddRandomItem: {
			return { true, 0, 0, Functions::AddRandomItem(args, a_type) };
		}
		case Function_Name::GetSkill: {
			return { Functions::GetSkill(args, a_type), 0, 0, {} };
		}
		case Function_Name::RewardSkillPercent: {
			Functions::RewardSkillPercent(args, a_type);
			break;
		}
		case Function_Name::RewardPlayerXP: {
			Functions::RewardPlayerXP(args, a_type);
			break;
		}
		case Function_Name::IsGreater: {
			return { Functions::IsGreater(args, a_type), 0, 0, {} };
		}
		case Function_Name::IsGreaterOrEqual: {
			return { Functions::IsGreaterOrEqual(args, a_type), 0, 0, {} };
		}
		case Function_Name::IsEqual: {
			return { Functions::IsEqual(args, a_type), 0, 0, {} };
		}
		case Function_Name::IsLessOrEqual: {
			return { Functions::IsLessOrEqual(args, a_type), 0, 0, {} };
		}
		case Function_Name::IsLess: {
			return { Functions::IsLess(args, a_type), 0, 0, {} };
		}
		case Function_Name::RollRandom: {
			return { true, Functions::RollRandom(args, a_type), 0, {} };
		}
		case Function_Name::RollDualRandom: {
			auto dualRandom = Functions::RollDualRandom(args, a_type);
			return { true, dualRandom.first, dualRandom.second, {} };
		}
		case Function_Name::HasSpell: {
			return { Functions::HasSpell(args, a_type), 0, 0, {} };
		}
		case Function_Name::HasActiveSpell: {
			return { Functions::HasActiveSpell(args, a_type), 0, 0, {} };
		}
		case Function_Name::CastSpellChance: {
			Functions::CastSpellChance(args, a_type);
			break;
		}
		case Function_Name::RemoveActiveSpell: {
			Functions::RemoveActiveSpell(args, a_type);
			break;
		}
		case Function_Name::DamageAV: {
			Functions::DamageAV(args, a_type);
			break;
		}
		case Function_Name::RestoreAV: {
			Functions::RestoreAV(args, a_type);
			break;
		}
		case Function_Name::ModHungerPercent: {
			Functions::ModHungerPercent(args, a_type);
			break;
		}
		case Function_Name::ModFatiguePercent: {
			Functions::ModFatiguePercent(args, a_type);
			break;
		}
		case Function_Name::ModColdPercent: {
			Functions::ModColdPercent(args, a_type);
			break;
		}
		default: {
			logger::error("Function {} is not valid. Check the function name, it is most likely wrong.", a_outcome);
			break;
		}		
	}
	return { true, 0, 0, {} };
}

bool Functions::HasItem(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Check") {
		logger::error("HasItem error: function is a \"Check\" function only.");
		return false;
	}
	if (a_args.size() != 3) {
		logger::error("HasItem error: function was given the wrong amount of arguments.");
		return false;
	}
	auto formPair = Utils::GetFormIDWithFile(a_args.at(1));
	if (formPair == std::pair<std::uint32_t, std::string>()) {
		logger::error("HasItem error: function was given an invalid Form argument.");
		return false;
	}
	if (!string::is_only_digit(a_args.at(2))) {
		logger::error("HasItem error: function was given an invalid amount argument. It must be a number.");
		return false;
	}
	auto amount = string::to_num<std::int32_t>(a_args.at(2));

	const auto a_dataHandler = RE::TESDataHandler::GetSingleton();
	if (!a_dataHandler) {
		logger::error("HasItem error: TESDataHandler not found.");
		return false;
	}
	auto item = a_dataHandler->LookupForm(formPair.first, formPair.second);
	if (!item) {
		logger::error("HasItem error: item with FormID {} for Mod {} does not exist.", formPair.first, formPair.second);
		return false;
	}

	if (item->IsBoundObject() && item->formType.get() != RE::FormType::None) {
		auto itemObj = item->As<RE::TESBoundObject>();
		auto ITEM_FILTER = [itemObj](RE::TESBoundObject& a_obj) { return itemObj->formID == a_obj.formID; };
		auto itemCountMap = RE::PlayerCharacter::GetSingleton()->GetInventoryCounts(ITEM_FILTER);
		auto itemCount = itemCountMap.find(itemObj);
		if (itemCount != itemCountMap.end()) {
			return itemCount->second >= amount;
		}
	}
	return false;
}

void Functions::AddItem(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Outcome") {
		logger::error("AddItem error: function is an \"Outcome\" function only.");
		return;
	}
	if (a_args.size() != 3) {
		logger::error("AddItem error: function was given the wrong amount of arguments.");
		return;
	}
	auto formPair = Utils::GetFormIDWithFile(a_args.at(1));
	if (formPair == std::pair<std::uint32_t, std::string>()) {
		logger::error("AddItem error: function was given an invalid Form argument.");
		return;
	}
	if (!string::is_only_digit(a_args.at(2))) {
		logger::error("AddItem error: function was given an invalid amount argument. It must be a number.");
		return;
	}
	auto amount = string::to_num<std::int32_t>(a_args.at(2));

	const auto a_dataHandler = RE::TESDataHandler::GetSingleton();
	if (!a_dataHandler) {
		logger::error("AddItem error: TESDataHandler not found.");
		return;
	}
	auto item = a_dataHandler->LookupForm(formPair.first, formPair.second);
	if (!item) {
		logger::error("AddItem error: item with FormID {} for Mod {} does not exist.", formPair.first, formPair.second);
		return;
	}

	if (item->IsBoundObject() && item->formType.get() != RE::FormType::None) {
		AddItemAndNotify(item->As<RE::TESBoundObject>(), amount);
	}
}

void Functions::RemoveItem(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Outcome") {
		logger::error("RemoveItem error: function is an \"Outcome\" function only.");
		return;
	}
	if (a_args.size() != 3) {
		logger::error("RemoveItem error: function was given the wrong amount of arguments.");
		return;
	}
	auto formPair = Utils::GetFormIDWithFile(a_args.at(1));
	if (formPair == std::pair<std::uint32_t, std::string>()) {
		logger::error("RemoveItem error: function was given an invalid Form argument.");
		return;
	}
	if (!string::is_only_digit(a_args.at(2))) {
		logger::error("RemoveItem error: function was given an invalid amount argument. It must be a number.");
		return;
	}
	auto amount = string::to_num<std::int32_t>(a_args.at(2));

	const auto a_dataHandler = RE::TESDataHandler::GetSingleton();
	if (!a_dataHandler) {
		logger::error("RemoveItem error: TESDataHandler not found.");
		return;
	}
	auto item = a_dataHandler->LookupForm(formPair.first, formPair.second);
	if (!item) {
		logger::error("RemoveItem error: item with FormID {} for Mod {} does not exist.", formPair.first, formPair.second);
		return;
	}

	if (item->IsBoundObject() && item->formType.get() != RE::FormType::None) {
		auto itemObj = item->As<RE::TESBoundObject>();
		// If the amount is bigger than the actual count, use only the actual count
		auto ITEM_FILTER = [itemObj](RE::TESBoundObject& a_obj) { return itemObj->formID == a_obj.formID; };
		auto itemCountMap = RE::PlayerCharacter::GetSingleton()->GetInventoryCounts(ITEM_FILTER);
		auto itemCount = itemCountMap.find(itemObj);
		if (itemCount != itemCountMap.end()) {
			amount = amount > itemCount->second ? itemCount->second : amount;
		}
		RE::PlayerCharacter::GetSingleton()->AddObjectToContainer(itemObj, nullptr, -amount, nullptr);
		RE::SendHUDMessage::ShowInventoryChangeMessage(itemObj, -amount, false, true, itemObj->GetName());
	}
}

Functions::FormAndAmountType Functions::AddRandomItem(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Outcome") {
		logger::error("AddRandomItem error: function is an \"Outcome\" function only.");
		return {};
	}
	if (a_args.size() < 3) {
		logger::error("AddRandomItem error: function was given the wrong amount of arguments.");
		return {};
	}
	if (!string::is_only_digit(a_args.at(1))) {
		logger::error("AddRandomItem error: function was given an invalid count argument. It must be a number.");
		return {};
	}
	auto itemCount = string::to_num<std::uint16_t>(a_args.at(1));

	auto& currentEncounterData = MessageBoxHandler::GetSingleton()->GetCurrentEncounterData();
	// Keep adding to the item list until it's time to exit the encounter
	// In case there are nested outcomes with AddRandomItem...
	if (!currentEncounterData.exit) {
		const auto a_dataHandler = RE::TESDataHandler::GetSingleton();
		if (!a_dataHandler) {
			logger::error("AddRandomItem error: TESDataHandler not found.");
			return {};
		}
		// Items and amounts to add
		std::vector<std::pair<std::string, std::string>> itemList;
		// Skip the function name and count
		for (auto it = a_args.begin() + 2; it != a_args.end(); it++) {
			std::string formPair = *it;
			// Join back the item and the amount
			if (formPair.contains("{")) {
				it += 1;
				if (it != a_args.end() && it->contains("}")) {
					std::string amount = *it;
					string::replace_first_instance(formPair, "{", "");
					string::replace_first_instance(amount, "}", "");
					if (formPair.contains("}") || amount.contains("{")) {
						logger::error("AddRandomItem error: Form {} with amount {} input is invalid.", formPair, amount);
						return {};
					}
					if (amount.contains("-")) {
						const auto randomMinMax = string::split(amount, "-");
						auto a_min = !string::is_empty(randomMinMax.at(0).c_str()) ? string::to_num<std::int32_t>(randomMinMax.at(0)) : 1;
						auto a_max = !string::is_empty(randomMinMax.at(1).c_str()) ? string::to_num<std::int32_t>(randomMinMax.at(1)) : 1;
						auto randomAmount = clib_util::RNG().generate<std::int32_t>(a_min, a_max);
						amount = std::to_string(randomAmount);
					}
					itemList.push_back({ formPair, amount });
				}
			}
		}
		if (itemCount > itemList.size()) {
			logger::warn("AddRandomItem warning: the count argument must be less or equal to the number of items.");
			itemCount = static_cast<std::uint16_t>(itemList.size());
		}

		// Select %itemCount% random items and store them for later
		while (itemCount > 0) {
			auto randomPos = clib_util::RNG().generate<std::uint16_t>(0, static_cast<std::uint16_t>(itemList.size() - 1));
			std::pair<std::string, std::string> formAndAmountPair = itemList.at(randomPos);
			// Select 1 item/amount pair only 1 time
			itemList.erase(itemList.begin() + randomPos);
			auto formPair = Utils::GetFormIDWithFile(formAndAmountPair.first);
			if (formPair == std::pair<std::uint32_t, std::string>()) {
				logger::error("AddRandomItem error: {{item, amount}} was given an invalid Form argument.");
				return {};
			}
			auto item = a_dataHandler->LookupForm(formPair.first, formPair.second);
			if (!item) {
				logger::error("AddRandomItem error: item with FormID {} for Mod {} does not exist.", formPair.first, formPair.second);
				return {};
			}
			if (!string::is_only_digit(formAndAmountPair.second)) {
				logger::error("AddRandomItem error: the specified amount for item {} is invalid. It must be a number.", item->GetName());
				return {};
			}
			auto itemAmount = string::to_num<std::int32_t>(formAndAmountPair.second);

			selectedRandomItems.push_back({ item, itemAmount });

			itemCount -= 1;
		}
		return selectedRandomItems;
	}
	else {
		std::ranges::for_each(selectedRandomItems, [](std::pair<RE::TESForm*, std::int32_t>& a_itemAndAmount) {
			if (a_itemAndAmount.first && a_itemAndAmount.first->IsBoundObject() && a_itemAndAmount.first->formType.get() != RE::FormType::None) {
				AddItemAndNotify(a_itemAndAmount.first->As<RE::TESBoundObject>(), a_itemAndAmount.second);
			}
		});
	}
	return {};
}

bool Functions::GetSkill(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Check") {
		logger::error("GetSkill error: function is a \"Check\" function only.");
		return false;
	}
	if (a_args.size() != 3) {
		logger::error("GetSkill error: function was given the wrong amount of arguments.");
		return false;
	}
	if (!string::is_only_digit(a_args.at(2))) {
		logger::error("GetSkill error: function was given an invalid number argument.");
		return false;
	}
	auto skillCheckLevel = string::to_num<std::uint8_t>(a_args.at(2));
	auto skillAV = GetPlayerSkillAV(a_args.at(1));
	if (skillAV == RE::ActorValue::kNone) {
		logger::error("GetSkill error: function was given the wrong skill name.");
		return false;
	}
	// GetActorValue() gets the skill level with modifiers
	auto currentSkillLevel = RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetActorValue(skillAV);
	return skillCheckLevel <= currentSkillLevel;
}

void Functions::RewardSkillPercent(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (Settings::bIsExperienceModActive) {
		return;
	}
	if (a_type != "Outcome") {
		logger::error("RewardSkillPercent error: function is an \"Outcome\" function only.");
		return;
	}
	if (a_args.size() != 3) {
		logger::error("RewardSkillPercent error: function was given the wrong amount of arguments.");
		return;
	}
	if (!string::is_only_digit(a_args.at(2))) {
		logger::error("RewardSkillPercent error: function was given an invalid number argument.");
		return;
	}
	auto rewardPercentage = string::to_num<float>(a_args.at(2));
	if (rewardPercentage < 0.0f) {
		logger::error("RewardSkillPercent error: percentage number must be above 0.");
	}
	if (rewardPercentage > 100.0f) {
		rewardPercentage = 100.0f;
	}
	rewardPercentage /= 100.0f;
	auto skillAV = GetPlayerSkillAV(a_args.at(1));
	if (skillAV == RE::ActorValue::kNone) {
		logger::error("RewardSkillPercent error: function was given the wrong skill name.");
		return;
	}
	const auto skillAVInfo = RE::ActorValueList::GetSingleton()->GetActorValueInfo(skillAV);
	if (!skillAVInfo->skill) {
		logger::error("RewardSkillPercent error: could not find information of the specified skill.");
		return;
	}

	const auto a_player = RE::PlayerCharacter::GetSingleton();
	// Different struct for VR in RuntimeData()
	auto skillData = !REL::Module::IsVR() ? a_player->GetPlayerRuntimeData().skills->data->skills : a_player->GetVRPlayerRuntimeData()->skills->data->skills;
	// PlayerSkills are numbered 6 less than the actual skill Actor Values
	auto playerSkillNum = std::to_underlying(skillAV) - 6;
	// In certain cases levelThreshold can be 0, then just don't execute anything
	if (skillData[playerSkillNum].level > 0 && skillData[playerSkillNum].levelThreshold > 0) {
		auto threshold = skillData[playerSkillNum].levelThreshold;
		auto xp = skillData[playerSkillNum].xp;
		// Don't go over the threshold, start next level from 0 xp
		auto xpToAdd = rewardPercentage * threshold;
		if (xp + xpToAdd > threshold) {
			xpToAdd = xpToAdd - (xp + xpToAdd - threshold);
		}
		// Formula is: (XP * Skill Use Mult) + Skill Offset Mult
		// Have to eat the extra xp gained from Skill Offset Mult in this case, to avoid negatives
		if (xpToAdd <= skillAVInfo->skill->offsetMult) {
			xpToAdd = xpToAdd / skillAVInfo->skill->useMult;
		}
		else {
			xpToAdd = (xpToAdd - skillAVInfo->skill->offsetMult) / skillAVInfo->skill->useMult;
		}
		a_player->AddSkillExperience(skillAV, xpToAdd);
		auto notification = std::format("+{} XP", a_args.at(1));
		RE::SendHUDMessage::ShowHUDMessage(notification.c_str());
	}
}

void Functions::RewardPlayerXP(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (!Settings::bIsExperienceModActive) {
		return;
	}
	if (a_type != "Outcome") {
		logger::error("RewardPlayerXP error: function is an \"Outcome\" function only.");
		return;
	}
	if (a_args.size() != 2) {
		logger::error("RewardPlayerXP error: function was given the wrong amount of arguments.");
		return;
	}
	if (!string::is_only_digit(a_args.at(1))) {
		logger::error("RewardPlayerXP error: function was given an invalid number argument.");
		return;
	}
	auto rewardXP = string::to_num<std::uint16_t>(a_args.at(1));
	auto advLevelCmd = std::format("player.advlevel {:d}", rewardXP);
	RE::Console::ExecuteCommand(advLevelCmd.c_str());
}

bool Functions::IsGreater(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Check") {
		logger::error("IsGreater error: function is a \"Check\" function only.");
		return false;
	}
	if (a_args.size() != 3) {
		logger::error("IsGreater error: function was given the wrong amount of arguments.");
		return false;
	}
	if (string::is_only_digit(a_args.at(1)) && string::is_only_digit(a_args.at(2))) {
		int a_value1 = string::to_num<int>(a_args.at(1));
		int a_value2 = string::to_num<int>(a_args.at(2));
		return a_value1 > a_value2;
	}
	auto skillAV1 = GetPlayerSkillAV(a_args.at(1));
	auto skillAV2 = GetPlayerSkillAV(a_args.at(2));
	if (skillAV1 != RE::ActorValue::kNone && skillAV2 != RE::ActorValue::kNone) {
		// GetBaseActorValue() gets the skill level without modifiers
		auto currentBaseSkillLevel1 = RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetBaseActorValue(skillAV1);
		auto currentBaseSkillLevel2 = RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetBaseActorValue(skillAV2);
		return currentBaseSkillLevel1 > currentBaseSkillLevel2;
	}
	logger::error("IsGreater error: function was given an invalid number/skill name argument.");
	return false;
}

bool Functions::IsGreaterOrEqual(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Check") {
		logger::error("IsGreaterOrEqual error: function is a \"Check\" function only.");
		return false;
	}
	if (a_args.size() != 3) {
		logger::error("IsGreaterOrEqual error: function was given the wrong amount of arguments.");
		return false;
	}
	if (string::is_only_digit(a_args.at(1)) && string::is_only_digit(a_args.at(2))) {
		int a_value1 = string::to_num<int>(a_args.at(1));
		int a_value2 = string::to_num<int>(a_args.at(2));
		return a_value1 >= a_value2;
	}
	auto skillAV1 = GetPlayerSkillAV(a_args.at(1));
	auto skillAV2 = GetPlayerSkillAV(a_args.at(2));
	if (skillAV1 != RE::ActorValue::kNone && skillAV2 != RE::ActorValue::kNone) {
		// GetBaseActorValue() gets the skill level without modifiers
		auto currentBaseSkillLevel1 = RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetBaseActorValue(skillAV1);
		auto currentBaseSkillLevel2 = RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetBaseActorValue(skillAV2);
		return currentBaseSkillLevel1 >= currentBaseSkillLevel2;
	}
	logger::error("IsGreaterOrEqual error: function was given an invalid number/skill name argument.");
	return false;
}

bool Functions::IsEqual(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Check") {
		logger::error("IsEqual error: function is a \"Check\" function only.");
		return false;
	}
	if (a_args.size() != 3) {
		logger::error("IsEqual error: function was given the wrong amount of arguments.");
		return false;
	}
	if (string::is_only_digit(a_args.at(1)) && string::is_only_digit(a_args.at(2))) {
		int a_value1 = string::to_num<int>(a_args.at(1));
		int a_value2 = string::to_num<int>(a_args.at(2));
		return a_value1 == a_value2;
	}
	auto skillAV1 = GetPlayerSkillAV(a_args.at(1));
	auto skillAV2 = GetPlayerSkillAV(a_args.at(2));
	if (skillAV1 != RE::ActorValue::kNone && skillAV2 != RE::ActorValue::kNone) {
		// GetBaseActorValue() gets the skill level without modifiers
		auto currentBaseSkillLevel1 = RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetBaseActorValue(skillAV1);
		auto currentBaseSkillLevel2 = RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetBaseActorValue(skillAV2);
		return currentBaseSkillLevel1 == currentBaseSkillLevel2;
	}
	logger::error("IsEqual error: function was given an invalid number/skill name argument.");
	return false;
}

bool Functions::IsLessOrEqual(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Check") {
		logger::error("IsLessOrEqual error: function is a \"Check\" function only.");
		return false;
	}
	if (a_args.size() != 3) {
		logger::error("IsLessOrEqual error: function was given the wrong amount of arguments.");
		return false;
	}
	if (string::is_only_digit(a_args.at(1)) && string::is_only_digit(a_args.at(2))) {
		int a_value1 = string::to_num<int>(a_args.at(1));
		int a_value2 = string::to_num<int>(a_args.at(2));
		return a_value1 <= a_value2;
	}
	auto skillAV1 = GetPlayerSkillAV(a_args.at(1));
	auto skillAV2 = GetPlayerSkillAV(a_args.at(2));
	if (skillAV1 != RE::ActorValue::kNone && skillAV2 != RE::ActorValue::kNone) {
		// GetBaseActorValue() gets the skill level without modifiers
		auto currentBaseSkillLevel1 = RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetBaseActorValue(skillAV1);
		auto currentBaseSkillLevel2 = RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetBaseActorValue(skillAV2);
		return currentBaseSkillLevel1 <= currentBaseSkillLevel2;
	}
	logger::error("IsLessOrEqual error: function was given an invalid number/skill name argument.");
	return false;
}

bool Functions::IsLess(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Check") {
		logger::error("IsLess error: function is a \"Check\" function only.");
		return false;
	}
	if (a_args.size() != 3) {
		logger::error("IsLess error: function was given the wrong amount of arguments.");
		return false;
	}
	if (string::is_only_digit(a_args.at(1)) && string::is_only_digit(a_args.at(2))) {
		int a_value1 = string::to_num<int>(a_args.at(1));
		int a_value2 = string::to_num<int>(a_args.at(2));
		return a_value1 < a_value2;
	}
	auto skillAV1 = GetPlayerSkillAV(a_args.at(1));
	auto skillAV2 = GetPlayerSkillAV(a_args.at(2));
	if (skillAV1 != RE::ActorValue::kNone && skillAV2 != RE::ActorValue::kNone) {
		// GetBaseActorValue() gets the skill level without modifiers
		auto currentBaseSkillLevel1 = RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetBaseActorValue(skillAV1);
		auto currentBaseSkillLevel2 = RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetBaseActorValue(skillAV2);
		return currentBaseSkillLevel1 < currentBaseSkillLevel2;
	}
	logger::error("IsLess error: function was given an invalid number/skill name argument.");
	return false;
}

int Functions::RollRandom(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Randomized") {
		logger::error("RollRandom error: function is an \"Randomized\" function only.");
		return 0;
	}
	if (a_args.size() != 3) {
		logger::error("RollRandom error: function was given the wrong amount of arguments.");
		return 0;
	}
	if (!string::is_only_digit(a_args.at(1)) && !string::is_only_digit(a_args.at(2))) {
		logger::error("RollRandom error: function was given an invalid number argument.");
		return 0;
	}
	int a_min = string::to_num<int>(a_args.at(1));
	int a_max = string::to_num<int>(a_args.at(2));
	if (a_min > a_max) {
		logger::error("RollRandom error: minimum number must be less or equal to maximum.");
		return 0;
	}
	int a_random = clib_util::RNG().generate<int>(a_min, a_max);
	return a_random;
}

std::pair<int, int> Functions::RollDualRandom(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "DualRandomized") {
		logger::error("RollDualRandom error: function is an \"DualRandomized\" function only.");
		return { 0, 0 };
	}
	if (a_args.size() != 3) {
		logger::error("RollDualRandom error: function was given the wrong amount of arguments.");
		return { 0, 0 };
	}
	if (!string::is_only_digit(a_args.at(1)) && !string::is_only_digit(a_args.at(2))) {
		logger::error("RollDualRandom error: function was given an invalid number argument.");
		return { 0, 0 };
	}
	int a_min = string::to_num<int>(a_args.at(1));
	int a_max = string::to_num<int>(a_args.at(2));
	if (a_min > a_max) {
		logger::error("RollDualRandom error: minimum number must be less or equal to maximum.");
		return { 0, 0 };
	}
	int a_dualRandom1 = clib_util::RNG().generate<int>(a_min, a_max);
	int a_dualRandom2 = clib_util::RNG().generate<int>(a_min, a_max);
	return { a_dualRandom1, a_dualRandom2 };
}

bool Functions::HasSpell(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Check") {
		logger::error("HasSpell error: function is a \"Check\" function only.");
		return false;
	}
	if (a_args.size() != 2) {
		logger::error("HasSpell error: function was given the wrong amount of arguments.");
		return false;
	}
	auto formPair = Utils::GetFormIDWithFile(a_args.at(1));
	if (formPair == std::pair<std::uint32_t, std::string>()) {
		logger::error("HasSpell error: function was given an invalid Form argument.");
		return false;
	}
	const auto a_dataHandler = RE::TESDataHandler::GetSingleton();
	if (!a_dataHandler) {
		logger::error("HasSpell error: TESDataHandler not found.");
		return false;
	}
	auto a_spell = a_dataHandler->LookupForm<RE::SpellItem>(formPair.first, formPair.second);
	if (!a_spell) {
		logger::error("HasSpell error: spell with FormID {} for Mod {} does not exist.", formPair.first, formPair.second);
		return false;
	}
	return RE::PlayerCharacter::GetSingleton()->HasSpell(a_spell);
}

bool Functions::HasActiveSpell(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Check") {
		logger::error("HasActiveSpell error: function is a \"Check\" function only.");
		return false;
	}
	if (a_args.size() != 2) {
		logger::error("HasActiveSpell error: function was given the wrong amount of arguments.");
		return false;
	}
	auto formPair = Utils::GetFormIDWithFile(a_args.at(1));
	if (formPair == std::pair<std::uint32_t, std::string>()) {
		logger::error("HasActiveSpell error: function was given an invalid Form argument.");
		return false;
	}
	const auto a_dataHandler = RE::TESDataHandler::GetSingleton();
	if (!a_dataHandler) {
		logger::error("HasActiveSpell error: TESDataHandler not found.");
		return false;
	}
	auto a_spell = a_dataHandler->LookupForm<RE::SpellItem>(formPair.first, formPair.second);
	if (!a_spell) {
		logger::error("HasActiveSpell error: spell with FormID {} for Mod {} does not exist.", formPair.first, formPair.second);
		return false;
	}
	using EffectFlag = RE::ActiveEffect::Flag;
	// Store the valid effects in case the encounter has more HasActiveSpell checks
	if (currentActiveEffects.size() == 0) {
		// SE/AE has GetActiveEffectList()
		if (!REL::Module::IsVR()) {
			if (const auto activeEffectList = RE::PlayerCharacter::GetSingleton()->AsMagicTarget()->GetActiveEffectList(); activeEffectList) {
				currentActiveEffects = *activeEffectList
									| std::ranges::views::filter([](const auto& a_effect) {
										return a_effect && a_effect->flags.none(EffectFlag::kInactive) && a_effect->flags.none(EffectFlag::kDispelled); })
									| std::ranges::to<std::vector<RE::ActiveEffect*>>();
			}
		}
		// Have to use a different approach for VR
		else {
			RE::PlayerCharacter::GetSingleton()->AsMagicTarget()->VisitActiveEffects([&](RE::ActiveEffect* a_effect) -> RE::BSContainer::ForEachResult {
				if (a_effect) {
					currentActiveEffects.push_back(a_effect);
				}
				return RE::BSContainer::ForEachResult::kContinue;
			});
			currentActiveEffects = currentActiveEffects
								| std::ranges::views::filter([](const auto& a_effect) {
									return a_effect->flags.none(EffectFlag::kInactive) && a_effect->flags.none(EffectFlag::kDispelled); })
								| std::ranges::to<std::vector<RE::ActiveEffect*>>();
		}
	}
	return std::ranges::any_of(currentActiveEffects, [a_spell](const auto& a_effect) {
		return a_effect && a_effect->spell == a_spell;
	});

	return false;
}

void Functions::CastSpellChance(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Outcome") {
		logger::error("CastSpellChance error: function is an \"Outcome\" function only.");
		return;
	}
	if (a_args.size() != 3) {
		logger::error("CastSpellChance error: function was given the wrong amount of arguments.");
		return;
	}
	auto formPair = Utils::GetFormIDWithFile(a_args.at(1));
	if (formPair == std::pair<std::uint32_t, std::string>()) {
		logger::error("CastSpellChance error: function was given an invalid Form argument.");
		return;
	}
	if (!string::is_only_digit(a_args.at(2))) {
		logger::error("CastSpellChance error: function was given an invalid percentage argument. It must be a number.");
		return;
	}
	auto percentage = string::to_num<std::int16_t>(a_args.at(2));
	if (percentage < 0) {
		percentage = 0;
	}
	auto randomPercentage = percentage >= 100 ? 100 : clib_util::RNG().generate<std::uint16_t>(1, 100);
	if (percentage >= randomPercentage) {
		const auto a_dataHandler = RE::TESDataHandler::GetSingleton();
		if (!a_dataHandler) {
			logger::error("CastSpellChance error: TESDataHandler not found.");
			return;
		}
		auto spell = a_dataHandler->LookupForm<RE::SpellItem>(formPair.first, formPair.second);
		if (!spell) {
			logger::error("CastSpellChance error: spell with FormID {} for Mod {} does not exist.", formPair.first, formPair.second);
			return;
		}
		
		const auto a_player = RE::PlayerCharacter::GetSingleton();
		// Don't stack the effects, reset the duration instead (dispel method)
		// Don't need to do this with Peak Value Modifier effects as they don't stack
		std::vector<RE::EffectSetting*> hostileEffects = {};
		bool bIsDispelled = false;
		auto magicTarget = a_player->AsMagicTarget();
		for (const auto& a_effect : spell->effects) {
			// Use a bool flag for dispelled since I'm not sure how intensive HasMagicEffect() check is (haven't RE'd it)
			if (!bIsDispelled && !a_effect->baseEffect->HasArchetype(RE::EffectSetting::Archetype::kPeakValueModifier) && magicTarget->HasMagicEffect(a_effect->baseEffect)) {
				auto playerHandle = a_player->GetHandle();
				if (playerHandle) {
					magicTarget->DispelEffect(spell, playerHandle);
					bIsDispelled = true;
					// If player is in tgm then check for Hostile flag instead
					if (!a_player->IsGodMode()) {
						break;
					}
				}
			}
			// Remove Hostile flag since the effect doesn't apply if player is in tgm
			if (a_player->IsGodMode() && a_effect->baseEffect->IsHostile()) {
				hostileEffects.push_back(a_effect->baseEffect);
				a_effect->baseEffect->data.flags.set(false, RE::EffectSetting::EffectSettingData::Flag::kHostile);
			}
		}
		auto a_caster = a_player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		// Passing 0.0f to a_magnitudeOverride keeps the original magnitudes of the effects
		a_caster->CastSpellImmediate(spell, false, a_player, 1.0f, false, 0.0f, nullptr);
		auto spellName = spell->GetFullName();
		if (!string::is_empty(spellName)) {
			auto notification = std::format("You have gained {}", spellName);
			RE::SendHUDMessage::ShowHUDMessage(notification.c_str());
		}
		// Reset back the Hostile flags for effects that had it removed
		for (const auto& a_effect : hostileEffects) {
			a_effect->data.flags.set(true, RE::EffectSetting::EffectSettingData::Flag::kHostile);
		}
	}
}

void Functions::RemoveActiveSpell(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Outcome") {
		logger::error("RemoveActiveSpell error: function is an \"Outcome\" function only.");
		return;
	}
	if (a_args.size() != 2) {
		logger::error("RemoveActiveSpell error: function was given the wrong amount of arguments.");
		return;
	}
	auto formPair = Utils::GetFormIDWithFile(a_args.at(1));
	if (formPair == std::pair<std::uint32_t, std::string>()) {
		logger::error("RemoveActiveSpell error: function was given an invalid Form argument.");
		return;
	}
	const auto a_dataHandler = RE::TESDataHandler::GetSingleton();
	if (!a_dataHandler) {
		logger::error("RemoveActiveSpell error: TESDataHandler not found.");
		return;
	}
	auto a_spell = a_dataHandler->LookupForm<RE::SpellItem>(formPair.first, formPair.second);
	if (!a_spell) {
		logger::error("RemoveActiveSpell error: spell with FormID {} for Mod {} does not exist.", formPair.first, formPair.second);
		return;
	}
	const auto a_player = RE::PlayerCharacter::GetSingleton();
	auto magicTarget = a_player->AsMagicTarget();
	for (const auto& a_effect : a_spell->effects) {
		if (magicTarget->HasMagicEffect(a_effect->baseEffect)) {
			auto playerHandle = a_player->GetHandle();
			if (playerHandle) {
				magicTarget->DispelEffect(a_spell, playerHandle);
				auto spellName = a_spell->GetFullName();
				if (!string::is_empty(spellName)) {
					auto notification = std::format("{} has been removed", spellName);
					RE::SendHUDMessage::ShowHUDMessage(notification.c_str());
				}
				break;
			}
		}
	}
}

void Functions::DamageAV(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Outcome") {
		logger::error("DamageAV error: function is an \"Outcome\" function only.");
		return;
	}
	if (a_args.size() != 3) {
		logger::error("DamageAV error: function was given the wrong amount of arguments.");
		return;
	}
	if (!string::is_only_letter(a_args.at(1))) {
		logger::error("DamageAV error: function was given an invalid actor value argument.");
		return;
	}
	auto& avName = a_args.at(1);

	auto amount = 0.0f;
	try {
		amount = string::to_num<float>(a_args.at(2));
	}
	catch (...) {
		logger::error("DamageAV error: function was given an invalid damage amount \"{}\" argument. It must be a number.", a_args.at(2));
		return;
	}

	auto statAV = GetPlayerStatAV(avName);
	if (statAV == RE::ActorValue::kNone) {
		logger::error("DamageAV error: function was given the wrong actor value name. {} hasn't been found.", avName);
		return;
	}
	const auto a_player = RE::PlayerCharacter::GetSingleton();
	// Make sure to not go below 1
	if (auto a_avOwner = a_player->AsActorValueOwner()) {
		auto currentAVAmount = a_avOwner->GetActorValue(statAV);
		if (currentAVAmount + 1.0f > amount) {

			a_avOwner->DamageActorValue(statAV, amount);
		}
		else {
			a_avOwner->DamageActorValue(statAV, currentAVAmount - 1.0f);
		}
		// Do a hit visual/sound
		if (spell_DummyHitEvent) {
			auto a_caster = a_player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
			a_caster->CastSpellImmediate(spell_DummyHitEvent, false, a_player, 1.0f, true, 0.0f, nullptr);
		}
	}
}

void Functions::RestoreAV(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Outcome") {
		logger::error("RestoreAV error: function is an \"Outcome\" function only.");
		return;
	}
	if (a_args.size() != 3) {
		logger::error("RestoreAV error: function was given the wrong amount of arguments.");
		return;
	}
	if (!string::is_only_letter(a_args.at(1))) {
		logger::error("RestoreAV error: function was given an invalid actor value argument.");
		return;
	}
	auto& avName = a_args.at(1);

	auto amount = 0.0f;
	try {
		amount = string::to_num<float>(a_args.at(2));
	}
	catch (...) {
		logger::error("RestoreAV error: function was given an invalid damage amount \"{}\" argument. It must be a number.", a_args.at(2));
		return;
	}

	auto statAV = GetPlayerStatAV(avName);
	if (statAV == RE::ActorValue::kNone) {
		logger::error("RestoreAV error: function was given the wrong actor value name. {} hasn't been found.", avName);
		return;
	}
	const auto a_player = RE::PlayerCharacter::GetSingleton();
	// RestoreActorValue will not go above max actor value
	if (auto a_avOwner = a_player->AsActorValueOwner()) {
		a_avOwner->RestoreActorValue(statAV, amount);
		// Show a restore visual
		if (spell_DummyRestoreEffect) {
			auto a_caster = a_player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
			a_caster->CastSpellImmediate(spell_DummyRestoreEffect, false, a_player, 1.0f, false, 0.0f, nullptr);
		}
	}
}

void Functions::ModHungerPercent(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Outcome") {
		logger::error("ModHungerPercent error: function is an \"Outcome\" function only.");
		return;
	}
	if (a_args.size() != 2) {
		logger::error("ModHungerPercent error: function was given the wrong amount of arguments.");
		return;
	}
	float modPercentage = 0.0f;
	try {
		modPercentage = string::to_num<float>(a_args.at(1));
	}
	catch (...) {
		logger::error("ModHungerPercent error: function was given an invalid percentage \"{}\" argument. It must be a number.", a_args.at(1));
		return;
	}
	if (!Settings::survival_HungerCurrent || !Settings::survival_HungerMax) {
		logger::error("ModHungerPercent error: could not find hunger values.");
		return;
	}
	auto& survival_hungerCurrent = Settings::survival_HungerCurrent->value;
	auto& survival_hungerMax = Settings::survival_HungerMax->value;
	auto hungerValueMod = survival_hungerMax * (modPercentage / 100.0f);
	// Don't go below 0
	if (survival_hungerCurrent + hungerValueMod < 0.0f) {
		hungerValueMod = survival_hungerCurrent * -1;
	}
	// Don't go above max
	if (survival_hungerCurrent + hungerValueMod > survival_hungerMax) {
		hungerValueMod = survival_hungerMax - survival_hungerCurrent;
	}
	survival_hungerCurrent = survival_hungerCurrent + hungerValueMod;
}

void Functions::ModFatiguePercent(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Outcome") {
		logger::error("ModFatiguePercent error: function is an \"Outcome\" function only.");
		return;
	}
	if (a_args.size() != 2) {
		logger::error("ModFatiguePercent error: function was given the wrong amount of arguments.");
		return;
	}
	float modPercentage = 0.0f;
	try {
		modPercentage = string::to_num<float>(a_args.at(1));
	}
	catch (...) {
		logger::error("ModFatiguePercent error: function was given an invalid percentage \"{}\" argument. It must be a number.", a_args.at(1));
		return;
	}
	if (!Settings::survival_ExhaustionCurrent || !Settings::survival_ExhaustionMax) {
		logger::error("ModFatiguePercent error: could not find exhaustion values.");
		return;
	}
	auto& survival_exhaustionCurrent = Settings::survival_ExhaustionCurrent->value;
	auto& survival_exhaustionMax = Settings::survival_ExhaustionMax->value;
	auto exhaustionValueMod = survival_exhaustionMax * (modPercentage / 100.0f);
	// Don't go below 0
	if (survival_exhaustionCurrent + exhaustionValueMod < 0.0f) {
		exhaustionValueMod = survival_exhaustionCurrent * -1;
	}
	// Don't go above max
	if (survival_exhaustionCurrent + exhaustionValueMod > survival_exhaustionMax) {
		exhaustionValueMod = survival_exhaustionMax - survival_exhaustionCurrent;
	}
	survival_exhaustionCurrent = survival_exhaustionCurrent + exhaustionValueMod;
}

void Functions::ModColdPercent(const std::vector<std::string>& a_args, const std::string& a_type)
{
	if (a_type != "Outcome") {
		logger::error("ModColdPercent error: function is an \"Outcome\" function only.");
		return;
	}
	if (a_args.size() != 2) {
		logger::error("ModColdPercent error: function was given the wrong amount of arguments.");
		return;
	}
	float modPercentage = 0.0f;
	try {
		modPercentage = string::to_num<float>(a_args.at(1));
	}
	catch (...) {
		logger::error("ModColdPercent error: function was given an invalid percentage \"{}\" argument. It must be a number.", a_args.at(1));
		return;
	}
	if (!Settings::survival_ColdCurrent || !Settings::survival_ColdMax) {
		logger::error("ModColdPercent error: could not find cold values.");
		return;
	}
	auto& survival_coldCurrent = Settings::survival_ColdCurrent->value;
	auto& survival_coldMax = Settings::survival_ColdMax->value;
	auto coldValueMod = survival_coldMax * (modPercentage / 100.0f);
	// Don't go below 0
	if (survival_coldCurrent + coldValueMod < 0.0f) {
		coldValueMod = survival_coldCurrent * -1;
	}
	// Don't go above max
	if (survival_coldCurrent + coldValueMod > survival_coldMax) {
		coldValueMod = survival_coldMax - survival_coldCurrent;
	}
	survival_coldCurrent = survival_coldCurrent + coldValueMod;
}
