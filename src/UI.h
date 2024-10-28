#pragma once
#include "SKSEMenuFramework.h"
#include <openai/openai.hpp>
#include <nlohmann/json.hpp>
#include <future>
#include <chrono>
#include <vector>
#include <string>
#include <format>

namespace UI {
    // Global registration for all UI components
    void Register();

    namespace Dashboard {
        // Window handle
        inline MENU_WINDOW Window;

        // Glyphs definitions for icons
        namespace Glyphs {
            inline std::string DashboardTitle = "TESSERACT " + FontAwesome::UnicodeToUtf8(0xf080);
            inline std::string RefreshIcon = FontAwesome::UnicodeToUtf8(0xf021);
            inline std::string CloseIcon = FontAwesome::UnicodeToUtf8(0xf00d);
            inline std::string NPCIcon = FontAwesome::UnicodeToUtf8(0xf007);
            inline std::string MapIcon = FontAwesome::UnicodeToUtf8(0xf279);
            inline std::string ActiveIcon = FontAwesome::UnicodeToUtf8(0xf111);
            inline std::string SettingsIcon = FontAwesome::UnicodeToUtf8(0xf013);
        }

        // Dashboard state/data
        namespace State {
            inline int activeNPCCount = 0;
        }

        // Functions
        void __stdcall RenderWindow();
        void Register();
    }

    namespace ChatWindow {
        // Types and structures
        struct ChatMessage {
            enum class Sender { User, NPC };
            Sender sender;
            std::string content;
            std::string GetSenderName() const { return sender == Sender::User ? "Player" : "NPC"; }
        };

        // Window and UI state
        inline MENU_WINDOW Window;
        inline bool autoScroll = true;
        inline std::vector<ChatMessage> chatHistory;
        inline char inputBuffer[1024] = {0};
        
        // Async state
        inline bool isThinking = false;
        inline std::chrono::steady_clock::time_point thinkingAnimationTimer;
        inline std::future<std::string> aiResponseFuture;
        
        // NPC state
        inline RE::Actor* currentNPC = nullptr;

        // OpenAI integration
        inline bool openaiInitialized = false;
        
        // Chat context/memory management
        namespace Context {
            inline std::vector<nlohmann::json> messages;
            inline size_t maxMessages = 10;
            
            void AddMessage(const std::string& role, const std::string& content);
            nlohmann::json GetMessageHistory();
        }

        // Core functions
        void __stdcall RenderWindow();
        void Register();
        void Open(RE::Actor* targetNpc);
        void Close();
        
        // OpenAI related functions
        void SetAPIKey(const std::string& key);
        std::string SendOpenAIRequest(const std::string& userInput);
        std::string GenerateSystemPrompt();
        std::string GetNPCContext();
        void ProcessResponse();
    }

    namespace Settings {
        void __stdcall RenderMenu();
    }
}