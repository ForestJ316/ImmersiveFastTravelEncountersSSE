#pragma once

#include <SKSE/SKSE.h>
#include <RE/Skyrim.h>
#include <REL/Relocation.h>

#include <nlohmann/json.hpp>
#include <spdlog/sinks/basic_file_sink.h>

#include <ClibUtil/string.hpp>
#include <ClibUtil/rng.hpp>

using namespace std::literals;

using json = nlohmann::json;

namespace string = clib_util::string;
namespace logger = SKSE::log;
