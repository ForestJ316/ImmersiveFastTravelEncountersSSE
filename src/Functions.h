#pragma once

namespace Functions
{
	bool DoFunction(std::string a_outcome);

	class Function
	{
	public:
		// AddItem, FormID|Mod, a_amount
		static void AddItem(std::vector<std::string> a_args);
		// GetSkill, a_skillName, a_amount
		static bool GetSkill(std::vector<std::string> a_args);
		// RewardSkillPercent, a_skillName, a_percAmount
		// (Only if Experience mod is not active)
		static void RewardSkillPercent(std::vector<std::string> a_args);
		// RewardPlayerXP, a_amount
		// (Only if Experience mod is active)
		static void RewardPlayerXP(std::vector<std::string> a_args);

	private:
		static RE::ActorValue GetPlayerSkillAV(std::string_view a_skillName);

		static constexpr std::array PLAYER_SKILL_AV = {
			RE::ActorValue::kOneHanded,
			RE::ActorValue::kTwoHanded,
			RE::ActorValue::kArchery,
			RE::ActorValue::kBlock,
			RE::ActorValue::kSmithing,
			RE::ActorValue::kHeavyArmor,
			RE::ActorValue::kLightArmor,
			RE::ActorValue::kPickpocket,
			RE::ActorValue::kLockpicking,
			RE::ActorValue::kSneak,
			RE::ActorValue::kAlchemy,
			RE::ActorValue::kSpeech,
			RE::ActorValue::kAlchemy,
			RE::ActorValue::kConjuration,
			RE::ActorValue::kDestruction,
			RE::ActorValue::kIllusion,
			RE::ActorValue::kRestoration,
			RE::ActorValue::kEnchanting,
		};
	};
}
