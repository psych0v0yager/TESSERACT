#pragma once
#include "SKSEMenuFramework.h"
#include <openai/openai.hpp>
#include <nlohmann/json.hpp>
#include <future>
#include <chrono>
#include <vector>
#include <string>

namespace UI {
    class ChatWindow {
    public:
        // Core window management
        static void Register();
        void Open(RE::Actor* targetNpc);  // Call this when dialogue starts
        void Close();

        // OpenAI configuration
        static void SetAPIKey(const std::string& key) { 
            if (!openaiInitialized) {
                openai::start(key);
                openaiInitialized = true;
            }
        }

    private:
        // Window rendering
        static void __stdcall RenderWindow();
        inline static MENU_WINDOW Window;
        bool autoScroll = true;

        // Chat message structure
        struct ChatMessage {
            enum class Sender { User, NPC };
            Sender sender;
            std::string content;
            std::string GetSenderName() const { return sender == Sender::User ? "Player" : "NPC"; }
        };
        
        // Chat state
        std::vector<ChatMessage> chatHistory;
        char inputBuffer[1024] = {0};  // Buffer for user input
        
        // Async state for AI responses
        bool isThinking = false;
        std::chrono::steady_clock::time_point thinkingAnimationTimer;
        std::future<std::string> aiResponseFuture;
        
        // NPC state tracking
        RE::Actor* currentNPC = nullptr;

        // OpenAI integration
        inline static bool openaiInitialized = false;
        std::string SendOpenAIRequest(const std::string& userInput);
        std::string GenerateSystemPrompt();  // Creates context for the NPC
        std::string GetNPCContext();         // Gets NPC's current state/info
        void ProcessResponse();              // Handle async response
        
        // Chat context with memory management
        struct ChatContext {
            std::vector<nlohmann::json> messages;  // OpenAI message history
            size_t maxMessages = 10;               // Memory window size (last N messages)
            void AddMessage(const std::string& role, const std::string& content);
            nlohmann::json GetMessageHistory() const;
        } context;
    };
}