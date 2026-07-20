#pragma once

#pragma warning(push)
#include <SKSE/SKSE.h>
#include <RE/Skyrim.h>
#include <REL/Relocation.h>

#include <nlohmann/json.hpp>
#include <SimpleIni.h>

#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/msvc_sink.h>
#endif
#pragma warning(pop)

using namespace std::literals;

using json = nlohmann::json;
using Callback = RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>;

namespace logger = SKSE::log;
namespace util
{
	using SKSE::stl::report_and_fail;
}

#define DLLEXPORT __declspec(dllexport)

#include "Version.h"
