#include "Papyrus.h"

#include "Settings.h"

namespace Papyrus
{
	void IFTE_SettingChangedInt(RE::StaticFunctionTag*, const std::string a_settingName, const int a_settingValue)
	{
		Settings::GetSingleton()->OnSettingChanged(a_settingName, a_settingValue);
	}

	void IFTE_SettingChangedBool(RE::StaticFunctionTag*, const std::string a_settingName, const bool a_settingValue)
	{
		Settings::GetSingleton()->OnSettingChanged(a_settingName, a_settingValue);
	}

	bool PapyrusNativeFunctions(RE::BSScript::IVirtualMachine* a_vm)
	{
		a_vm->RegisterFunction("IFTE_SettingChangedInt", "ImmersiveFastTravelEncounters_MCM", IFTE_SettingChangedInt);
		a_vm->RegisterFunction("IFTE_SettingChangedBool", "ImmersiveFastTravelEncounters_MCM", IFTE_SettingChangedBool);
		logger::info("Registered Papyrus native functions.");
		return true;
	}

	void RegisterFunctions()
	{
		const auto papyrus = SKSE::GetPapyrusInterface();
		if (!papyrus || !papyrus->Register(Papyrus::PapyrusNativeFunctions)) {
			logger::critical("Cannot register Papyrus native functions!");
			return;
		}
	}
}
