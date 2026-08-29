#pragma once

#include <SKSE/SKSE.h>
#include <RE/Skyrim.h>
#include <REL/Relocation.h>

#include <nlohmann/json.hpp>
#include <spdlog/sinks/basic_file_sink.h>

using json = nlohmann::json;

#include <ClibUtil/string.hpp>
#include <ClibUtil/rng.hpp>

#include "Utils.h"

using namespace std::literals;

namespace utils = Utils;
namespace string = clib_util::string;
namespace logger = SKSE::log;
