#include "MessageBoxHandler.h"
#include "FastTravelHandler.h"
#include "Settings.h"
#include "Functions.h"

void InitializeLog()
{
	auto path = logger::log_directory();
	if (!path) {
		SKSE::stl::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= SKSE::GetPluginName();
	*path += ".log"sv;
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S:%e] %v"s);

	auto version = SKSE::GetPluginVersion();
	logger::info(FMT_STRING("{} v{}.{}.{}"), SKSE::GetPluginName(), version.major(), version.minor(), version.patch());
}

void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
		case SKSE::MessagingInterface::kDataLoaded :
			logger::info("{:*^50}", "DATA LOADED"sv);
			Settings::GetSingleton()->Initialize();
			Functions::CacheActorValueNames();
			FastTravelHandler::GetSingleton()->Initialize();			
			logger::info("{:*^50}", ""sv);
			break;
		case SKSE::MessagingInterface::kPreLoadGame :
			logger::info("in PreLoadGame");
			FastTravelHandler::GetSingleton()->ResetVars();
			// In case the game was reloaded before the current encounter was finished
			MessageBoxHandler::GetSingleton()->ResetCurrentEncounterData();
			break;
		default:
			break;
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);

	InitializeLog();

	logger::info("Game version {}", a_skse->RuntimeVersion().string("."));

	if (!SKSE::GetMessagingInterface()->RegisterListener(MessageHandler)) {
		return false;
	}
	return true;
}
