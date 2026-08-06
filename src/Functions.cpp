#include "Functions.h"

#include "Utils.h"
#include "Settings.h"

#include <algorithm>
#include <ranges>

std::vector<RE::ActiveEffect*> Functions::currentActiveEffects{};

Functions::Function_Name Functions::GetFunctionHash(const std::string a_str)
{
	if (a_str == "HasItem") return Function_Name::HasItem;
	if (a_str == "AddItem") return Function_Name::AddItem;
	if (a_str == "RemoveItem") return Function_Name::RemoveItem;
	if (a_str == "GetSkill") return Function_Name::GetSkill;
	if (a_str == "RewardSkillPercent") return Function_Name::RewardSkillPercent;
	if (a_str == "RewardPlayerXP") return Function_Name::RewardPlayerXP;
	if (a_str == "IsGreaterThan") return Function_Name::IsGreaterThan;
	if (a_str == "IsEqual") return Function_Name::IsEqual;
	if (a_str == "IsGreaterOrEqual") return Function_Name::IsGreaterOrEqual;
	if (a_str == "GetRandom") return Function_Name::GetRandom;
	if (a_str == "GetDualRandom") return Function_Name::GetDualRandom;
	if (a_str == "HasActiveSpell") return Function_Name::HasActiveSpell;
	if (a_str == "CastSpellChance") return Function_Name::CastSpellChance;

	return Function_Name::UNKNOWN;
}

std::tuple<bool, int, int> Functions::DoFunction(std::string a_outcome)
{
	auto args = Utils::GetSplitStrings(a_outcome, ",");
	switch (Functions::GetFunctionHash(args.at(0))) {
		case Function_Name::HasItem: {
			return { Functions::HasItem(args), 0, 0 };
		}
		case Function_Name::AddItem: {
			Functions::AddItem(args);
			break;
		}
		case Function_Name::RemoveItem: {
			Functions::RemoveItem(args);
			break;
		}	
		case Function_Name::GetSkill: {
			return { Functions::GetSkill(args), 0, 0 };
		}
		case Function_Name::RewardSkillPercent: {
			Functions::RewardSkillPercent(args);
			break;
		}
		case Function_Name::RewardPlayerXP: {
			Functions::RewardPlayerXP(args);
			break;
		}
		case Function_Name::IsGreaterThan: {
			return { Functions::IsGreaterThan(args), 0, 0 };
		}
		case Function_Name::IsEqual: {
			return { Functions::IsEqual(args), 0, 0 };
		}
		case Function_Name::IsGreaterOrEqual: {
			return { Functions::IsGreaterOrEqual(args), 0, 0 };
		}
		case Function_Name::GetRandom: {
			return { true, Functions::GetRandom(args), 0 };
		}
		case Function_Name::GetDualRandom: {
			auto dualRandom = Functions::GetDualRandom(args);
			return { true, dualRandom.first, dualRandom.second };
		}
		case Function_Name::HasActiveSpell: {
			return { Functions::HasActiveSpell(args), 0, 0 };
		}
		case Function_Name::CastSpellChance: {
			Functions::CastSpellChance(args);
			break;
		}
		default: {
			logger::error("Function {} is not valid. Check the function name, it is most likely wrong.", a_outcome);
			break;
		}		
	}
	return { true, 0, 0 };
}

bool Functions::HasItem(std::vector<std::string> a_args)
{
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

	const auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler) {
		logger::error("HasItem error: TESDataHandler not found.");
		return false;
	}
	auto item = dataHandler->LookupForm(formPair.first, formPair.second);
	if (!item) {
		logger::error("HasItem error: item with FormID {} for mod {} does not exist.", formPair.first, formPair.second);
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

void Functions::AddItem(std::vector<std::string> a_args)
{
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

	const auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler) {
		logger::error("AddItem error: TESDataHandler not found.");
		return;
	}
	auto item = dataHandler->LookupForm(formPair.first, formPair.second);
	if (!item) {
		logger::error("AddItem error: item with FormID {} for mod {} does not exist.", formPair.first, formPair.second);
		return;
	}

	if (item->IsBoundObject() && item->formType.get() != RE::FormType::None) {
		auto itemObj = item->As<RE::TESBoundObject>();
		RE::PlayerCharacter::GetSingleton()->AddObjectToContainer(itemObj, nullptr, amount, nullptr);
		RE::SendHUDMessage::ShowInventoryChangeMessage(itemObj, amount, true, true, itemObj->GetName());
	}
}

void Functions::RemoveItem(std::vector<std::string> a_args)
{
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

	const auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler) {
		logger::error("RemoveItem error: TESDataHandler not found.");
		return;
	}
	auto item = dataHandler->LookupForm(formPair.first, formPair.second);
	if (!item) {
		logger::error("RemoveItem error: item with FormID {} for mod {} does not exist.", formPair.first, formPair.second);
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

RE::ActorValue Functions::GetPlayerSkillAV(std::string_view a_skillName)
{
	RE::ActorValue skill = RE::ActorValue::kNone;
	auto AVList = RE::ActorValueList::GetSingleton();
	for (const auto& skillAV : Functions::PLAYER_SKILL_AV) {
		if (a_skillName == AVList->GetActorValueInfo(skillAV)->GetFullName()) {
			skill = skillAV;
			break;
		}
	}
	return skill;
}

bool Functions::GetSkill(std::vector<std::string> a_args)
{
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

void Functions::RewardSkillPercent(std::vector<std::string> a_args)
{
	if (Settings::GetSingleton()->bIsExperienceModActive) {
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
	auto skillAVInfo = RE::ActorValueList::GetSingleton()->GetActorValueInfo(skillAV);
	if (!skillAVInfo->skill) {
		logger::error("RewardSkillPercent error: could not find information of the specified skill.");
		return;
	}

	auto skillData = RE::PlayerCharacter::GetSingleton()->GetPlayerRuntimeData().skills->data->skills;
	// PlayerSkills are numbered 6 less than the actual skill Actor Values
	auto playerSkillNum = std::to_underlying(skillAV) - 6;
	if (skillData[playerSkillNum].level > 0) {
		auto a_player = RE::PlayerCharacter::GetSingleton();
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

void Functions::RewardPlayerXP(std::vector<std::string> a_args)
{
	if (!Settings::GetSingleton()->bIsExperienceModActive) {
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

bool Functions::IsGreaterThan(std::vector<std::string> a_args)
{
	if (a_args.size() != 3) {
		logger::error("IsGreaterThan error: function was given the wrong amount of arguments.");
		return false;
	}
	if (!string::is_only_digit(a_args.at(1)) && !string::is_only_digit(a_args.at(2))) {
		logger::error("IsGreaterThan error: function was given an invalid number argument.");
		return false;
	}
	int a_value1 = string::to_num<int>(a_args.at(1));
	int a_value2 = string::to_num<int>(a_args.at(2));
	return a_value1 > a_value2;
}

bool Functions::IsEqual(std::vector<std::string> a_args)
{
	if (a_args.size() != 3) {
		logger::error("IsEqual error: function was given the wrong amount of arguments.");
		return false;
	}
	if (!string::is_only_digit(a_args.at(1)) && !string::is_only_digit(a_args.at(2))) {
		logger::error("IsEqual error: function was given an invalid number argument.");
		return false;
	}
	int a_value1 = string::to_num<int>(a_args.at(1));
	int a_value2 = string::to_num<int>(a_args.at(2));
	return a_value1 == a_value2;
}

bool Functions::IsGreaterOrEqual(std::vector<std::string> a_args)
{
	if (a_args.size() != 3) {
		logger::error("IsGreaterOrEqual error: function was given the wrong amount of arguments.");
		return false;
	}
	if (!string::is_only_digit(a_args.at(1)) && !string::is_only_digit(a_args.at(2))) {
		logger::error("IsGreaterOrEqual error: function was given an invalid number argument.");
		return false;
	}
	int a_value1 = string::to_num<int>(a_args.at(1));
	int a_value2 = string::to_num<int>(a_args.at(2));
	return a_value1 >= a_value2;
}

int Functions::GetRandom(std::vector<std::string> a_args)
{
	if (a_args.size() != 3) {
		logger::error("GetRandom error: function was given the wrong amount of arguments.");
		return 0;
	}
	if (!string::is_only_digit(a_args.at(1)) && !string::is_only_digit(a_args.at(2))) {
		logger::error("GetRandom error: function was given an invalid number argument.");
		return 0;
	}
	int a_min = string::to_num<int>(a_args.at(1));
	int a_max = string::to_num<int>(a_args.at(2));
	if (a_min > a_max) {
		logger::info("GetRandom error: minimum number must be less or equal to maximum.");
		return 0;
	}
	int a_random = clib_util::RNG().generate<int>(a_min, a_max);
	return a_random;
}

std::pair<int, int> Functions::GetDualRandom(std::vector<std::string> a_args)
{
	if (a_args.size() != 3) {
		logger::error("GetDualRandom error: function was given the wrong amount of arguments.");
		return { 0, 0 };
	}
	if (!string::is_only_digit(a_args.at(1)) && !string::is_only_digit(a_args.at(2))) {
		logger::error("GetDualRandom error: function was given an invalid number argument.");
		return { 0, 0 };
	}
	int a_min = string::to_num<int>(a_args.at(1));
	int a_max = string::to_num<int>(a_args.at(2));
	if (a_min > a_max) {
		logger::info("GetDualRandom error: minimum number must be less or equal to maximum.");
		return { 0, 0 };
	}
	int a_random1 = clib_util::RNG().generate<int>(a_min, a_max);
	int a_random2 = clib_util::RNG().generate<int>(a_min, a_max);
	return { a_random1, a_random2 };
}

bool Functions::HasActiveSpell(std::vector<std::string> a_args)
{
	if (a_args.size() != 2) {
		logger::error("HasActiveSpell error: function was given the wrong amount of arguments.");
		return false;
	}
	auto formPair = Utils::GetFormIDWithFile(a_args.at(1));
	if (formPair == std::pair<std::uint32_t, std::string>()) {
		logger::error("HasActiveSpell error: function was given an invalid Form argument.");
		return false;
	}
	const auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler) {
		logger::error("HasActiveSpell error: TESDataHandler not found.");
		return false;
	}
	auto a_spell = dataHandler->LookupForm<RE::SpellItem>(formPair.first, formPair.second);
	if (!a_spell) {
		logger::error("HasActiveSpell error: spell with FormID {} for mod {} does not exist.", formPair.first, formPair.second);
		return false;
	}
	using EffectFlag = RE::ActiveEffect::Flag;
	// Store the valid effects in case the encounter has more HasActiveSpell checks
	if (currentActiveEffects.size() == 0) {
		// SE/AE has GetActiveEffectList()
		if (!REL::Module::IsVR()) {
			if (const auto activeEffectList = RE::PlayerCharacter::GetSingleton()->AsMagicTarget()->GetActiveEffectList(); activeEffectList) {
				currentActiveEffects = *activeEffectList
									| std::ranges::views::filter([&](const auto &a_effect) {
										return a_effect && a_effect->flags.none(EffectFlag::kInactive) && a_effect->flags.none(EffectFlag::kDispelled);
									})
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
								| std::ranges::views::filter([&](const auto &a_effect) {
									return a_effect->flags.none(EffectFlag::kInactive) && a_effect->flags.none(EffectFlag::kDispelled);
								})
								| std::ranges::to<std::vector<RE::ActiveEffect*>>();
		}
	}
	return std::ranges::any_of(currentActiveEffects, [&](const auto &a_effect) {
		return a_effect && a_effect->spell == a_spell;
	});

	return false;
}

void Functions::ResetActiveEffectsList()
{
	currentActiveEffects = {};
}

void Functions::CastSpellChance(std::vector<std::string> a_args)
{
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
		const auto dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			logger::error("CastSpellChance error: TESDataHandler not found.");
			return;
		}
		auto spell = dataHandler->LookupForm<RE::SpellItem>(formPair.first, formPair.second);
		if (!spell) {
			logger::error("CastSpellChance error: spell with FormID {} for mod {} does not exist.", formPair.first, formPair.second);
			return;
		}
		auto a_caster = RE::PlayerCharacter::GetSingleton()->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		// Passing 0.0f to a_magnitutdeOverride keeps the original magnitudes of the effects
		a_caster->CastSpellImmediate(spell->As<RE::MagicItem>(), false, RE::PlayerCharacter::GetSingleton(), 1.0f, false, 0.0f, nullptr);
		auto spellName = spell->GetName();
		if (!string::is_empty(spellName)) {
			auto notification = std::format("You have gained {}", spellName);
			RE::SendHUDMessage::ShowHUDMessage(notification.c_str());
		}
	}
}
