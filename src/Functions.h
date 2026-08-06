#pragma once

class Functions
{
public:
	static std::tuple<bool, int, int> DoFunction(std::string a_outcome);
	static void ResetActiveEffectsList();

private:
	enum class Function_Name {
		HasItem,
		AddItem,
		RemoveItem,
		GetSkill,
		RewardSkillPercent,
		RewardPlayerXP,
		IsGreaterThan,
		IsEqual,
		IsGreaterOrEqual,
		GetRandom,
		GetDualRandom,
		HasActiveSpell,
		CastSpellChance,
		UNKNOWN
	};
	static Function_Name GetFunctionHash(const std::string a_str);

	// HasItem, FormID|Mod, a_amount
	static bool HasItem(std::vector<std::string> a_args);
	// AddItem, FormID|Mod, a_amount
	static void AddItem(std::vector<std::string> a_args);
	// RemoveItem, FormID|Mod, a_amount
	static void RemoveItem(std::vector<std::string> a_args);
	// GetSkill, a_skillName, a_amount
	static bool GetSkill(std::vector<std::string> a_args);
	// RewardSkillPercent, a_skillName, a_percAmount
	// (Only if Experience mod is not active)
	static void RewardSkillPercent(std::vector<std::string> a_args);
	// RewardPlayerXP, a_amount
	// (Only if Experience mod is active)
	static void RewardPlayerXP(std::vector<std::string> a_args);
	// IsGreaterThan, a_value1, a_value2
	static bool IsGreaterThan(std::vector<std::string> a_args);
	// IsEqual, a_value1, a_value2
	static bool IsEqual(std::vector<std::string> a_args);
	// IsGreaterOrEqual, a_value1, a_value2
	static bool IsGreaterOrEqual(std::vector<std::string> a_args);
	// GetRandom, a_min, a_max
	static int GetRandom(std::vector<std::string> a_args);
	// GetDualRandom, a_min, a_max
	static std::pair<int, int> GetDualRandom(std::vector<std::string> a_args);
	// HasActiveSpell, FormID|Mod
	static bool HasActiveSpell(std::vector<std::string> a_args);
	// CastSpell, FormID|Mod, a_percentage
	static void CastSpellChance(std::vector<std::string> a_args);

	// Store the current valid active effects for the encounter
	// in order to avoid iterating active effects multiple times
	static std::vector<RE::ActiveEffect*> currentActiveEffects;

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
