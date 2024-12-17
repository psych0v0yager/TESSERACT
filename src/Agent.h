#pragma once

// CommonLib includes
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

// Third-party libraries
#include <openai/openai.hpp>
#include <nlohmann/json.hpp>

// Standard library
#include <vector>
#include <string>
#include <chrono>
#include <future>
#include <atomic>
#include <memory>
#include <format>

// For logging
namespace logger = SKSE::log;

namespace TESSERACT::Agent {
    // The "low-level" OpenAI interaction system
    namespace Communication {
        struct Message {
            std::string role;
            std::string content;
            std::time_t timestamp;
        };

        // Functions for handling OpenAI API calls
        std::string SendOpenAIRequest(const std::vector<Message>& context, const std::string& userInput);
        std::string GenerateSystemPrompt(const RE::Actor* npc);
        std::string GetNPCContext(const RE::Actor* npc);
    }

    // Memory system that any agent type can use
    namespace Memory {
        // Configuration settings
        extern bool calculateImportance;  // Notice the 'extern' keyword
        extern int maxMemories;

        // Memory object
        struct MemoryEntry {
            std::string role;
            std::string content;
            float importance;
            std::time_t timestamp;
            
            // Constructor for easy creation
            MemoryEntry(std::string r, std::string c, float imp = 1.0f) 
                : role(r), content(c), importance(imp), 
                  timestamp(std::chrono::system_clock::to_time_t(
                      std::chrono::system_clock::now())) {}
        };


        // Convert strings to Memories
        MemoryEntry CreateFromString(const std::string& content, const std::string& role);
        MemoryEntry CreateFromOpenAI(const nlohmann::json& message);

        // Memory management functions
        void ProcessMemory(MemoryEntry& memory);
        float CalculateImportance(const std::string& content);
    }

    // The base SubAgent class
    // This organization reflects how a mind works 
    // - public methods for interacting with the world, 
    // - protected members for core mental state
    // - private methods for internal thought processes. 
    // The virtual functions allow us to create specialized types of agents later.
    class SubAgent {
    public:
        SubAgent(RE::Actor* npc, const std::string& role) 
            : npc(npc), agentRole(role) {}

        // Core functionality
        virtual std::string ProcessInput(const std::string& input);
        virtual void Update();  // Called regularly to update agent state

        // Helper functions
        RE::Actor* GetNPC() const { return npc; } 
        
    protected:
        RE::Actor* npc;
        std::string agentRole;  // e.g., "id", "ego", "superego", "basal-ganglia"
        std::vector<Memory::MemoryEntry> memories;
        
        // Async state (moved from ChatWindow)
        std::atomic<bool> isProcessingUpdate{false};
        std::future<std::string> responseFuture;

    private:
        // Internal helper functions
        void AddMemory(const std::string& role, const std::string& content);
        std::vector<Communication::Message> PrepareContext();
    };
}