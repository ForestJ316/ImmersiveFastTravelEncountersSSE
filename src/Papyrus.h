#pragma once

namespace Papyrus
{
	void IFTE_SettingChangedInt(RE::StaticFunctionTag*, const std::string a_settingName, const int a_settingValue);
	void IFTE_SettingChangedBool(RE::StaticFunctionTag*, const std::string a_settingName, const bool a_settingValue);
	bool PapyrusNativeFunctions(RE::BSScript::IVirtualMachine* a_vm);
	void RegisterFunctions();
}
