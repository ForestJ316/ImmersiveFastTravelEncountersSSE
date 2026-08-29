#pragma once

class Functions
{
private:
	using StoredItemFuncType = std::vector<std::tuple<std::string, RE::TESForm*, std::int32_t>>;
	using StoredItemType = std::vector<std::pair<RE::TESForm*, std::int32_t>>;

public:
	// Re-check the names of player skills and stats on kDataLoaded
	static void Initialize();
	static void ResetVars();

	static constexpr std::array ITEM_FUNCTIONS = {
		"AddItem",
		"RemoveItem",
		"AddRandomItem",
	};
	
	template <typename T>
	static T DoFunction(const std::string& a_outcome, const std::string a_type)
	{
		const auto args = utils::SplitString(a_outcome, ",");
		switch (Functions::GetFunctionHash(args.at(0))) {
			// Randomized & DualRandomized
			case Function_Name::RollRandom: {
				if constexpr (std::is_same_v<T, int>) {
					return Functions::RollRandom(args, a_type);
				}
				break;
			}
			case Function_Name::RollDualRandom: {
				if constexpr (std::is_same_v<T, std::pair<int, int>>) {
					return Functions::RollDualRandom(args, a_type);
				}
				break;
			}
			// Check
			case Function_Name::HasItem: {
				if constexpr (std::is_same_v<T, bool>) {
					return Functions::HasItem(args, a_type);
				}
				break;
			}
			case Function_Name::HasSkill: {
				if constexpr (std::is_same_v<T, bool>) {
					return Functions::HasSkill(args, a_type);
				}
				break;
			}
			case Function_Name::HasSpell: {
				if constexpr (std::is_same_v<T, bool>) {
					return Functions::HasSpell(args, a_type);
				}
				break;
			}
			case Function_Name::HasActiveSpell: {
				if constexpr (std::is_same_v<T, bool>) {
					return Functions::HasActiveSpell(args, a_type);
				}
				break;
			}
			case Function_Name::IsRace: {
				if constexpr (std::is_same_v<T, bool>) {
					return Functions::IsRace(args, a_type);
				}
				break;
			}
			case Function_Name::IsGreater: {
				if constexpr (std::is_same_v<T, bool>) {
					return Functions::IsGreater(args, a_type);
				}
				break;
			}
			case Function_Name::IsGreaterOrEqual: {
				if constexpr (std::is_same_v<T, bool>) {
					return Functions::IsGreaterOrEqual(args, a_type);
				}
				break;
			}
			case Function_Name::IsEqual: {
				if constexpr (std::is_same_v<T, bool>) {
					return Functions::IsEqual(args, a_type);
				}
				break;
			}
			case Function_Name::IsLessOrEqual: {
				if constexpr (std::is_same_v<T, bool>) {
					return Functions::IsLessOrEqual(args, a_type);
				}
				break;
			}
			case Function_Name::IsLess: {
				if constexpr (std::is_same_v<T, bool>) {
					return Functions::IsLess(args, a_type);
				}
				break;
			}
			// Outcomes
			case Function_Name::AddItem: {
				if constexpr (std::is_same_v<T, StoredItemType>) {
					return Functions::AddItem(args, a_type);
				}
				Functions::AddItem(args, a_type);
				break;
			}
			case Function_Name::RemoveItem: {
				if constexpr (std::is_same_v<T, StoredItemType>) {
					return Functions::RemoveItem(args, a_type);
				}
				Functions::RemoveItem(args, a_type);
				break;
			}
			case Function_Name::AddRandomItem: {
				if constexpr (std::is_same_v<T, StoredItemType>) {
					return Functions::AddRandomItem(args, a_type);
				}
				Functions::AddRandomItem(args, a_type);
				break;
			}
			case Function_Name::RewardSkillPercent: {
				Functions::RewardSkillPercent(args, a_type);
				break;
			}
			case Function_Name::RewardPlayerXP: {
				Functions::RewardPlayerXP(args, a_type);
				break;
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
		return T();
	}

private:
	enum class Function_Name {
		// Randomized & DualRandomized
		RollRandom,
		RollDualRandom,
		// Check
		HasItem,
		HasSkill,
		HasSpell,
		HasActiveSpell,
		IsRace,
		IsGreater,
		IsGreaterOrEqual,
		IsEqual,
		IsLessOrEqual,
		IsLess,
		// Outcomes
		AddItem,
		RemoveItem,
		AddRandomItem,
		RewardSkillPercent,
		RewardPlayerXP,
		CastSpellChance,
		RemoveActiveSpell,
		DamageAV,
		RestoreAV,
		ModHungerPercent,
		ModFatiguePercent,
		ModColdPercent,

		UNKNOWN
	};
	static constexpr Function_Name GetFunctionHash(const std::string& a_str)
	{
		// Randomized & DualRandomized
		if (a_str == "RollRandom") return Function_Name::RollRandom;
		if (a_str == "RollDualRandom") return Function_Name::RollDualRandom;
		// Check
		if (a_str == "HasItem") return Function_Name::HasItem;
		if (a_str == "HasSkill") return Function_Name::HasSkill;
		if (a_str == "HasSpell") return Function_Name::HasSpell;
		if (a_str == "HasActiveSpell") return Function_Name::HasActiveSpell;
		if (a_str == "IsRace") return Function_Name::IsRace;
		if (a_str == "IsGreater") return Function_Name::IsGreater;
		if (a_str == "IsGreaterOrEqual") return Function_Name::IsGreaterOrEqual;
		if (a_str == "IsEqual") return Function_Name::IsEqual;
		if (a_str == "IsLessOrEqual") return Function_Name::IsLessOrEqual;
		if (a_str == "IsLess") return Function_Name::IsLess;
		// Outcomes
		if (a_str == "AddItem") return Function_Name::AddItem;
		if (a_str == "RemoveItem") return Function_Name::RemoveItem;
		if (a_str == "AddRandomItem") return Function_Name::AddRandomItem;
		if (a_str == "RewardSkillPercent") return Function_Name::RewardSkillPercent;
		if (a_str == "RewardPlayerXP") return Function_Name::RewardPlayerXP;		
		if (a_str == "CastSpellChance") return Function_Name::CastSpellChance;
		if (a_str == "RemoveActiveSpell") return Function_Name::RemoveActiveSpell;
		if (a_str == "DamageAV") return Function_Name::DamageAV;
		if (a_str == "RestoreAV") return Function_Name::RestoreAV;
		if (a_str == "ModHungerPercent") return Function_Name::ModHungerPercent;
		if (a_str == "ModFatiguePercent") return Function_Name::ModFatiguePercent;
		if (a_str == "ModColdPercent") return Function_Name::ModColdPercent;

		return Function_Name::UNKNOWN;
	}

	// Helpers
	static void AddItemAndNotify(RE::TESBoundObject* a_item, std::int32_t a_amount);
	static void RemoveItemAndNotify(RE::TESBoundObject* a_item, std::int32_t a_amount);
	static RE::ActorValue GetPlayerSkillAV(const std::string& a_skillName);
	static RE::ActorValue GetPlayerStatAV(const std::string& a_statName);

	// ----------------------------------- Randomized & DualRandomized -----------------------------------
	// RollRandom, a_min, a_max
	static int RollRandom(const std::vector<std::string>& a_args, const std::string& a_type);
	// RollDualRandom, a_min, a_max
	static std::pair<int, int> RollDualRandom(const std::vector<std::string>& a_args, const std::string& a_type);
	// ----------------------------------- Check -----------------------------------
	// HasItem, 0xFormID|Mod, a_amount
	static bool HasItem(const std::vector<std::string>& a_args, const std::string& a_type);
	// HasSkill, a_skillName, a_amount
	static bool HasSkill(const std::vector<std::string>& a_args, const std::string& a_type);
	// HasSpell, 0xFormID|Mod
	static bool HasSpell(const std::vector<std::string>& a_args, const std::string& a_type);
	// HasActiveSpell, 0xFormID|Mod
	static bool HasActiveSpell(const std::vector<std::string>& a_args, const std::string& a_type);
	// IsRace, list of {races}
	static bool IsRace(const std::vector<std::string>& a_args, const std::string& a_type);
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
	// ----------------------------------- Outcomes -----------------------------------
	// AddItem, 0xFormID|Mod, a_amount
	static StoredItemType AddItem(const std::vector<std::string>& a_args, const std::string& a_type);
	// RemoveItem, 0xFormID|Mod, a_amount
	static StoredItemType RemoveItem(const std::vector<std::string>& a_args, const std::string& a_type);
	// AddRandomItem, a_count, list of {a_item, a_amount}
	// First call: store the items because of replacement strings in the message
	// Second call: actually add the selected items
	static StoredItemType AddRandomItem(const std::vector<std::string>& a_args, const std::string& a_type);
	// RewardSkillPercent, a_skillName, a_percentage
	// (Only if Experience mod is not active)
	static void RewardSkillPercent(const std::vector<std::string>& a_args, const std::string& a_type);
	// RewardPlayerXP, a_amount
	// (Only if Experience mod is active)
	static void RewardPlayerXP(const std::vector<std::string>& a_args, const std::string& a_type);
	// CastSpellChance, 0xFormID|Mod, a_percentage
	static void CastSpellChance(const std::vector<std::string>& a_args, const std::string& a_type);
	// RemoveActiveSpell, 0xFormID|Mod, a_silent
	static void RemoveActiveSpell(const std::vector<std::string>& a_args, const std::string& a_type);
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
	static StoredItemFuncType storedItemsFunc;
	// Store the current valid active effects for the encounter
	// in order to avoid iterating active effects multiple times
	static std::vector<RE::ActiveEffect*> currentActiveEffects;
	
	// Dummy effects for DamageAV and RestoreAV functions
	static inline RE::SpellItem* spell_DummyHitEvent = nullptr;
	static inline RE::SpellItem* spell_DummyRestoreEffect = nullptr;

	static inline std::array PLAYER_SKILL_AV = {
		std::make_pair("One-handed", RE::ActorValue::kOneHanded),
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
