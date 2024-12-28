// UI.h
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
#include <atomic>     // For std::atomic operations
#include <unordered_set>
#include "Agent.h"

namespace UI {
    // Global registration for all UI components
    void Register();

    // Centralized Config System
    namespace Config {
        // Path to config file
        inline const std::string CONFIG_PATH = "Data\\SKSE\\Plugins\\TESSERACT\\config.json";
        
        // Status tracking for UI feedback
        inline bool loadSuccess = false;
        inline std::string lastError = "";

        // Core configuration functions
        void LoadConfig();
        void SaveConfig();
        void EnsureConfigDirectory();
        std::string GetErrorMessage();  // Helper to get formatted error messages

        // Dashboard-specific configuration
        namespace Dashboard {
            inline int npcCount = 20;  // Default to 20 NPCs
            inline bool debugQuestEnabled = false;  // Admin level spells
            
            // Save/Load dashboard specific settings
            void SaveToConfig(nlohmann::json& config);
            void LoadFromConfig(const nlohmann::json& config);
        }

        // OpenAI-specific configuration
        namespace OpenAI {
            inline std::string baseUrl = "";
            inline std::string apiKey = "";
            inline std::string model = "gpt-4o-mini";  // Add default model
            inline std::atomic<bool> initialized{false};
            
            // Save/Load OpenAI specific settings
            void SaveToConfig(nlohmann::json& config);
            void LoadFromConfig(const nlohmann::json& config);
            
            // OpenAI connection management
            void StartConnection();
        }

        // Chat-specific configuration
        namespace Chat {
            inline size_t maxMessages = 10;  // Maximum number of messages to keep in context
            
            void SaveToConfig(nlohmann::json& config);
            void LoadFromConfig(const nlohmann::json& config);
        }
    }

    namespace NPCDetails {
        // Window handle and state
        inline MENU_WINDOW detailsWindow;
        inline int selectedNpcId = -1;

        // Tab flags
        inline const ImGuiTabBarFlags TAB_FLAGS = 
            ImGuiTabBarFlags_NoCloseWithMiddleMouseButton |
            ImGuiTabBarFlags_FittingPolicyResizeDown |
            (1 << 20);  // ImGuiTabBarFlags_Bottom equivalent


        // Core functions
        void Register();
        void Open(int npcId);
        void Close();
        void __stdcall RenderWindow();

        // Drawing helpers namespace declaration
        namespace Drawing {
            void DrawNPCInfo();
            void DrawExperiencerContext();
            void DrawNarratorContext();
            void DrawPhysicalAgentContext();
        }
    }

    namespace Dashboard {
        // Dashboard Register function
        void __stdcall RenderMenuItem();  // New render function for menu item

        // Window handle
        inline MENU_WINDOW dashboardWindow;

        // Quest and placeholder tracking
        // Quest Editor IDs
        inline const std::string HOLDING_QUEST_EDITORID = "TESSERACT_HoldingQuest_ActiveActors";
        inline const std::string PLACEHOLDER_QUEST_EDITORID = "TESSERACT_HoldingQuest_PlaceholderStorage";

        // Quest handles
        inline RE::TESQuest* holdingQuest = nullptr;
        inline RE::TESQuest* placeholderQuest = nullptr;

        inline std::unordered_set<RE::TESObjectREFR*> placeholderSet;

        // Refresh variable
        inline std::atomic<bool> isRefreshing{false};

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
        void __stdcall RenderMenuItem();
        void __stdcall RenderWindow();
        void RefreshNPCs();
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
        inline std::atomic<bool> autoScroll{true};
        inline std::vector<ChatMessage> chatHistory;
        inline char inputBuffer[1024] = {0};
        inline char formIdBuffer[9] = {0};  // Move it here, at namespace scope

        
        // Async state
        inline std::atomic<bool> isThinking{false};
        inline std::chrono::steady_clock::time_point thinkingAnimationTimer;
        inline std::future<std::string> aiResponseFuture;
        
        // NPC state
        // inline RE::Actor* currentNPC = nullptr;
        inline std::unique_ptr<TESSERACT::Agent::SubAgent> currentNPC = nullptr;

        // Chat context/memory management
        // namespace Context {
        //     inline std::vector<nlohmann::json> messages;
            
        //     void AddMessage(const std::string& role, const std::string& content);
        //     nlohmann::json GetMessageHistory();
        // }

        // Core functions
        void __stdcall RenderWindow();
        void Register();
        void Open(RE::Actor* targetNpc);
        void Close();
        
        // OpenAI related functions
        std::string SendOpenAIRequest(const std::string& userInput);
        // std::string GenerateSystemPrompt();
        // std::string GetNPCContext();
        void ProcessResponse();
    }

    namespace Settings {
        void __stdcall RenderMenu();
    }
}