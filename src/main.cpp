
namespace
{
	void InitializeLog()
	{
		auto path = logger::log_directory();
		if (!path) {
			util::report_and_fail("Failed to find standard logging directory"sv);
		}

		*path /= Version::NAME;
		*path += ".log"sv;
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%H:%M:%S:%e] %v"s);

		logger::info(FMT_STRING("{} v{}"), Version::NAME, Version::VERSION);
	}

	void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type)
		{
			case SKSE::MessagingInterface::kDataLoaded :
				logger::info("{:*^50}", "DATA LOADED"sv);
				// Do something
				logger::info("{:*^50}", ""sv);
				break;
			default:
				break;
		}
	}
	/*
	extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
	{
		a_info->infoVersion = SKSE::PluginInfo::kVersion;
		a_info->name = Plugin::NAME.data();
		a_info->version = Plugin::VERSION[0];

		if (a_skse->IsEditor()) {
			logger::critical("Loaded in editor, marking as incompatible"sv);
			return false;
		}

		const auto ver = a_skse->RuntimeVersion();
		if (ver < SKSE::RUNTIME_SSE_1_5_39 || ver < SKSE::RUNTIME_LATEST_VR) {
			logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
			return false;
		}

		return true;
	}
	*/

	extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
	{
		SKSE::Init(a_skse);

		InitializeLog();

		logger::info("Game version : {}", a_skse->RuntimeVersion().string());

		//EnemyHandler::Hooks::Install();
		//Papyrus::RegisterPapyrus();

		if (!SKSE::GetMessagingInterface()->RegisterListener("SKSE", MessageHandler)) {
			return false;
		}
		return true;
	}

	/*
	extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
		SKSE::PluginVersionData data;

		data.PluginName(Plugin::NAME);
		data.PluginVersion(Plugin::VERSION);
		data.AuthorName("ForestJ316");
		data.UsesAddressLibrary();
		data.CompatibleVersions({ SKSE::RUNTIME_SSE_LATEST_SE, SKSE::RUNTIME_SSE_LATEST_AE, SKSE::RUNTIME_LATEST_VR });
		data.UsesNoStructs();

		return data;
	}();
	*/
}
