// main.cpp
#include "pch.h"
#include "UI.h"
#include "PapyrusRegistration.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        UI::Register();
        logger::info("TESSERACT UI components registered");
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    SKSE::Init(a_skse);

    // Setup logging
    auto path = SKSE::log::log_directory();
    if (!path) {
        return false;
    }

    auto plugin = SKSE::PluginDeclaration::GetSingleton();
    *path /= std::format("{}.log", plugin->GetName());

    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
    auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));

    log->set_level(spdlog::level::info);
    log->flush_on(spdlog::level::info);

    spdlog::set_default_logger(std::move(log));
    spdlog::set_pattern("[%^%l%$] %v");

    logger::info("{} v{} loaded", plugin->GetName(), plugin->GetVersion());

    // Register for SKSE messages
    auto messaging = SKSE::GetMessagingInterface();
    if (!messaging->RegisterListener(OnMessage)) {
        return false;
    }

    // Register Papyrus functions
    auto papyrus = SKSE::GetPapyrusInterface();
    if (!papyrus->Register(TESSERACT::RegisterPapyrusFunctions)) {
        return false;
    }

    return true;
}