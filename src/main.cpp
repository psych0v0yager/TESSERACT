#include "pch.h"
#include "UI.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        UI::Register();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    
    // Setup logging
    auto path = SKSE::log::log_directory();
    if (!path) return false;
    
    auto plugin = SKSE::PluginDeclaration::GetSingleton();
    *path /= fmt::format(FMT_STRING("{}.log"), plugin->GetName());
    
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
    auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));
    
    log->set_level(spdlog::level::info);
    log->flush_on(spdlog::level::info);
    
    spdlog::set_default_logger(std::move(log));
    spdlog::set_pattern("[%^%l%$] %v");
    
    // Register messaging interface
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    
    logger::info("{} v{} loaded", plugin->GetName(), plugin->GetVersion());
    
    return true;
}