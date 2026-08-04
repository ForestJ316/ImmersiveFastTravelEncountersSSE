#include "Functions.h"

#include "Utils.h"
#include "Settings.h"

using Function = Functions::Function;

bool Functions::DoFunction(std::string a_outcome)
{
	auto args = Utils::GetSplitStrings(a_outcome, ",");
	if (args.at(0) == "AddItem") {
		Function::AddItem(args);
	}
	else if (args.at(0) == "GetSkill") {
		return Function::GetSkill(args);
	}
	else if (args.at(0) == "RewardSkillPercent") {
		Function::RewardSkillPercent(args);
	}
	else if (args.at(0) == "RewardPlayerXP") {
		Function::RewardPlayerXP(args);
	}
	return true;
}

void Function::AddItem(std::vector<std::string> a_args)
{
	// Check if the arguments were given correctly
	if (a_args.size() != 3) {
		logger::error("AddItem error: function was given the wrong amount of arguments.");
		return;
	}
	auto formPair = Utils::GetFormIDWithFile(a_args.at(1));
	if (formPair == std::pair<std::uint32_t, std::string>()) {
		logger::error("AddItem error: function was given an invalid Form argument.");
		return;
	}
	if (string::is_empty(a_args.at(2).c_str()) || !string::is_only_digit(a_args.at(2))) {
		logger::error("AddItem error: function was given an invalid amount argument. It must be a number.");
		return;
	}
	auto amount = string::to_num<std::int32_t>(a_args.at(2));

	const auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler) {
		logger::error("TESDataHandler not found.");
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

RE::ActorValue Function::GetPlayerSkillAV(std::string_view a_skillName)
{
	RE::ActorValue skill = RE::ActorValue::kNone;
	auto AVList = RE::ActorValueList::GetSingleton();
	for (const auto& skillAV : Function::PLAYER_SKILL_AV) {
		if (a_skillName == AVList->GetActorValueInfo(skillAV)->GetFullName()) {
			skill = skillAV;
			break;
		}
	}
	return skill;
}

bool Function::GetSkill(std::vector<std::string> a_args)
{
	// Check if the arguments were given correctly
	if (a_args.size() != 3) {
		logger::error("GetSkill error: function was given the wrong amount of arguments.");
		return false;
	}
	if (string::is_empty(a_args.at(2).c_str()) || !string::is_only_digit(a_args.at(2))) {
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

void Function::RewardSkillPercent(std::vector<std::string> a_args)
{
	if (Settings::GetSingleton()->bIsExperienceModActive) {
		return;
	}
	// Check if the arguments were given correctly
	if (a_args.size() != 3) {
		logger::error("RewardSkillPercent error: function was given the wrong amount of arguments.");
		return;
	}
	if (string::is_empty(a_args.at(2).c_str()) || !string::is_only_digit(a_args.at(2))) {
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
	// PlayerSkills are numbered 6 less over actual skill Actor Values
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
	}
}

void Function::RewardPlayerXP(std::vector<std::string> a_args)
{
	if (!Settings::GetSingleton()->bIsExperienceModActive) {
		return;
	}
	// Check if the arguments were given correctly
	if (a_args.size() != 2) {
		logger::error("RewardPlayerXP error: function was given the wrong amount of arguments.");
		return;
	}
	if (string::is_empty(a_args.at(1).c_str()) || !string::is_only_digit(a_args.at(1))) {
		logger::error("RewardPlayerXP error: function was given an invalid number argument.");
		return;
	}
	auto rewardXP = string::to_num<std::uint16_t>(a_args.at(1));
	auto advLevelCmd = std::format("player.advlevel {:d}", rewardXP);
	RE::Console::ExecuteCommand(advLevelCmd.c_str());
}
