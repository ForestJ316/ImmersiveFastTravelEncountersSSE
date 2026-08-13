#pragma once

class Functions
{
private:
	typedef std::vector<std::pair<RE::TESForm*, std::int32_t>> FormAndAmountType;

public:
	// Re-check the names of player skills and stats on kDataLoaded
	static void CacheActorValueNames();
	static void ResetVars();

	static std::tuple<bool, int, int, FormAndAmountType> DoFunction(const std::string& a_outcome, const std::string a_type);

private:
	enum class Function_Name {
		HasItem,
		AddItem,
		RemoveItem,
		AddRandomItem,
		GetSkill,
		RewardSkillPercent,
		RewardPlayerXP,
		IsGreater,
		IsGreaterOrEqual,
		IsEqual,
		IsLessOrEqual,
		IsLess,
		RollRandom,
		RollDualRandom,
		HasSpell,
		HasActiveSpell,
		CastSpellChance,
		DamageAV,
		RestoreAV,
		ModHungerPercent,
		ModFatiguePercent,
		ModColdPercent,
		UNKNOWN
	};
	static constexpr Function_Name GetFunctionHash(const std::string& a_str);

	// Helpers
	static void AddItemAndNotify(RE::TESBoundObject* a_item, std::int32_t a_amount);
	static RE::ActorValue GetPlayerSkillAV(const std::string& a_skillName);
	static RE::ActorValue GetPlayerStatAV(const std::string& a_statName);

	// HasItem, 0xFormID|Mod, a_amount
	static bool HasItem(const std::vector<std::string>& a_args, const std::string& a_type);
	// AddItem, 0xFormID|Mod, a_amount
	static void AddItem(const std::vector<std::string>& a_args, const std::string& a_type);
	// RemoveItem, 0xFormID|Mod, a_amount
	static void RemoveItem(const std::vector<std::string>& a_args, const std::string& a_type);
	// AddRandomItem, a_count, list of {a_item, a_amount}
	// First call: store the items because of replacement strings in the message
	// Second call: actually add the selected items
	static FormAndAmountType AddRandomItem(const std::vector<std::string>& a_args, const std::string& a_type);
	// GetSkill, a_skillName, a_amount
	static bool GetSkill(const std::vector<std::string>& a_args, const std::string& a_type);
	// RewardSkillPercent, a_skillName, a_percAmount
	// (Only if Experience mod is not active)
	static void RewardSkillPercent(const std::vector<std::string>& a_args, const std::string& a_type);
	// RewardPlayerXP, a_amount
	// (Only if Experience mod is active)
	static void RewardPlayerXP(const std::vector<std::string>& a_args, const std::string& a_type);
	// IsGreater, a_value1, a_value2
	static bool IsGreater(const std::vector<std::string>& a_args, const std::string& a_type);
	// IsGreaterOrEqual, a_value1, a_value2
	static bool IsGreaterOrEqual(const std::vector<std::string>& a_args, const std::string& a_type);
	// IsEqual, a_value1, a_value2
	static bool IsEqual(const std::vector<std::string>& a_args, const std::string& a_type);
	// IsLessOrEqual, a_value1, a_value2
	static bool IsLessOrEqual(const std::vector<std::string>& a_args, const std::string& a_type);
	// IsLess, a_value1, a_value2
	static bool IsLess(const std::vector<std::string>& a_args, const std::string& a_type);
	// RollRandom, a_min, a_max
	static int RollRandom(const std::vector<std::string>& a_args, const std::string& a_type);
	// RollDualRandom, a_min, a_max
	static std::pair<int, int> RollDualRandom(const std::vector<std::string>& a_args, const std::string& a_type);
	// HasSpell, 0xFormID|Mod
	static bool HasSpell(const std::vector<std::string>& a_args, const std::string& a_type);
	// HasActiveSpell, 0xFormID|Mod
	static bool HasActiveSpell(const std::vector<std::string>& a_args, const std::string& a_type);
	// CastSpell, 0xFormID|Mod, a_percentage
	static void CastSpellChance(const std::vector<std::string>& a_args, const std::string& a_type);
	// DamageAV, a_actorValue, a_amount
	static void DamageAV(const std::vector<std::string>& a_args, const std::string& a_type);
	// RestoreAV, a_actorValue, a_amount
	static void RestoreAV(const std::vector<std::string>& a_args, const std::string& a_type);
	// ModHungerPercent, a_percentage
	static void ModHungerPercent(const std::vector<std::string>& a_args, const std::string& a_type);
	// ModFatiguePercent, a_percentage
	static void ModFatiguePercent(const std::vector<std::string>& a_args, const std::string& a_type);
	// ModColdPercent, a_percentage
	static void ModColdPercent(const std::vector<std::string>& a_args, const std::string& a_type);
	
	// Store the selected items for AddRandomItem
	static FormAndAmountType selectedRandomItems;
	// Store the current valid active effects for the encounter
	// in order to avoid iterating active effects multiple times
	static std::vector<RE::ActiveEffect*> currentActiveEffects;
	
	static inline std::array PLAYER_SKILL_AV = {
		std::make_pair("One", RE::ActorValue::kOneHanded),
		std::make_pair("Two-handed",RE::ActorValue::kTwoHanded),
		std::make_pair("Archery", RE::ActorValue::kArchery),
		std::make_pair("Block", RE::ActorValue::kBlock),
		std::make_pair("Smithing", RE::ActorValue::kSmithing),
		std::make_pair("Heavy Armor", RE::ActorValue::kHeavyArmor),
		std::make_pair("Light Armor", RE::ActorValue::kLightArmor),
		std::make_pair("Pickpocket", RE::ActorValue::kPickpocket),
		std::make_pair("Lockpicking", RE::ActorValue::kLockpicking),
		std::make_pair("Sneak", RE::ActorValue::kSneak),
		std::make_pair("Alchemy", RE::ActorValue::kAlchemy),
		std::make_pair("Speech", RE::ActorValue::kSpeech),
		std::make_pair("Alteration", RE::ActorValue::kAlteration),
		std::make_pair("Conjuration", RE::ActorValue::kConjuration),
		std::make_pair("Destruction", RE::ActorValue::kDestruction),
		std::make_pair("Illusion", RE::ActorValue::kIllusion),
		std::make_pair("Restoration", RE::ActorValue::kRestoration),
		std::make_pair("Enchanting", RE::ActorValue::kEnchanting),
	};
	static inline std::array PLAYER_STAT_AV = {
		std::make_pair("Health", RE::ActorValue::kHealth),
		std::make_pair("Stamina", RE::ActorValue::kStamina),
		std::make_pair("Magicka", RE::ActorValue::kMagicka),
	};
};
