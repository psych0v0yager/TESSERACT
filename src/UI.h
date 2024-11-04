#pragma once
#include "SKSEMenuFramework.h"
#include <openai/openai.hpp>
#include <nlohmann/json.hpp>
#include <future>
#include <chrono>
#include <vector>
#include <string>
#include <format>
#include <filesystem>  // For filesystem operations
#include <fstream>    // For file I/O
#include <atomic>  // For std::atomic operations

namespace UI {
    // Global registration for all UI components
    void Register();

    namespace Dashboard {
        // Dashboard Register function
        void __stdcall RenderMenuItem();  // New render function for menu item

        // Window handle
        inline MENU_WINDOW dashboardWindow;

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
        // Chat Window Register function
        void __stdcall RenderMenuItem();  // New render function for menu item

        // Types and structures
        struct ChatMessage {
            enum class Sender { User, NPC };
            Sender sender;
            std::string content;
            std::string GetSenderName() const { return sender == Sender::User ? "Player" : "NPC"; }
        };

        // Window and UI state
        inline MENU_WINDOW chatWindow;
        // inline bool autoScroll = true;
        inline std::atomic<bool> autoScroll{true};
        inline std::vector<ChatMessage> chatHistory;
        inline char inputBuffer[1024] = {0};
        
        // Async state
        // inline bool isThinking = false;
        inline std::atomic<bool> isThinking{false};        // Was: inline bool isThinking = false;
        inline std::chrono::steady_clock::time_point thinkingAnimationTimer;
        inline std::future<std::string> aiResponseFuture;
        
        // NPC state
        inline RE::Actor* currentNPC = nullptr;

        // OpenAI integration
        inline std::string openaiURL = "";
        inline std::string openaiAPIKey = "";
        // inline bool openaiInitialized = false;
        inline std::atomic<bool> openaiInitialized{false}; // Was: inline bool openaiInitialized = false;

        // Config data structure
        namespace Config {
            // Path to config file
            inline const std::string CONFIG_PATH = "Data\\SKSE\\Plugins\\TESSERACT\\config.json";
            
            // Status tracking for UI feedback
            inline bool loadSuccess = false;
            inline std::string lastError = "";
            
            // Configuration functions
            void LoadConfig();
            void SaveConfig();
            void EnsureConfigDirectory();
            
            // Helper to get formatted error messages
            std::string GetErrorMessage();
        }
 
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
        // void SetAPIKey(const std::string& key);
        void StartOpenAI(const std::string& url, const std::string& key);
        std::string SendOpenAIRequest(const std::string& userInput);
        std::string GenerateSystemPrompt();
        std::string GetNPCContext();
        void ProcessResponse();
    }

    namespace Settings {
        void __stdcall RenderMenu();
    }
}