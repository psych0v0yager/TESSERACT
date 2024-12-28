#include "Agent.h"
#include "UI.h"

namespace TESSERACT::Agent::Communication {
    // AKA this portion interfaces with the OpenAI API directly
    // OpenAI Request
    std::string SendOpenAIRequest(const std::vector<Message>& context, const std::string& userInput) {
        // First check OpenAI connection
        if (!UI::Config::OpenAI::initialized.load()) {
            return "I'm not connected to OpenAI yet. Please check your settings.";
        }

        try {
            // Create the request structure
            nlohmann::json chat_request = {
                {"model", UI::Config::OpenAI::model},
                {"messages", nlohmann::json::array()}
            };

            // Convert our Message objects to the format OpenAI expects
            for (const auto& msg : context) {
                chat_request["messages"].push_back({
                    {"role", msg.role},
                    {"content", msg.content}
                    // Note: timestamp is for our internal use, OpenAI doesn't need it
                });
            }

            // // Add the user's new input
            // chat_request["messages"].push_back({
            //     {"role", "user"},
            //     {"content", userInput}
            // });

            // Log the request for debugging
            logger::info("Final OpenAI Request: {}", chat_request.dump(2));

            // Send request and get response
            auto chat = openai::chat().create(chat_request);
            std::string response = chat["choices"][0]["message"]["content"];
            
            return response;
        }
        catch (const std::exception& e) {
            logger::error("OpenAI request failed: {}", e.what());
            return "I'm sorry, I'm having trouble thinking clearly right now.";
        }
    }

    // Generate System prompt
    std::string GenerateSystemPrompt(const RE::Actor* npc) {
        if (!npc) return "";

        auto name = npc->GetName();
        auto race = npc->GetRace()->GetName();
        auto loc = npc->GetCurrentLocation() ? 
                    npc->GetCurrentLocation()->GetName() : "Unknown Location";

        return std::format(
            "You are {}, a {} in {}. Maintain character and speak naturally. "
            "You have your own goals, personality, and daily routine. "
            "Keep responses concise and relevant to your character. "
            "Current time: {}, Weather: {}.",
            name, race, loc,
            "TODO: Add time", "TODO: Add weather"
        );
    };

    // Generate Context
    std::string GetNPCContext(const RE::Actor* npc) {
        if (!npc) return "";

        bool isSneaking = npc->IsSneaking();
        bool isInCombat = npc->IsInCombat();
        bool isAlarmed = npc->IsAlarmed();
// npc->GetActorBase()->GetActorValue(RE::ActorValue::kHealth);
        // float health = npc->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER a_modifier, RE::ActorValue::kHealth);
        
        return std::format(
            "Current state: {}"
            // "Health: {:.0f}%\n"
            "Location: {}\n"
            "Time: {}\n",
            isInCombat ? "In Combat!" : 
            isAlarmed ? "Alarmed" : 
            isSneaking ? "Sneaking" : "Normal",
            // health,
            npc->GetCurrentLocation() ? 
                npc->GetCurrentLocation()->GetName() : "Unknown",
            "TODO: Add time"
        );
    }


};

namespace TESSERACT::Agent::Memory {
    // This portion creates and stores the memory objects
    
    bool calculateImportance = false;
    int maxMemories = 50;

    // Function to create memory from raw strings
    MemoryEntry CreateFromString(const std::string& content, const std::string& role) {
        // Create and process the memory
        MemoryEntry memory(role, content);
        ProcessMemory(memory);
        return memory;
    }

    // Function to create memory from OpenAI message format
    MemoryEntry CreateFromOpenAI(const nlohmann::json& message) {
        // Extract role and content from the JSON
        std::string role = message["role"].get<std::string>();
        std::string content = message["content"].get<std::string>();
        
        // Create and process the memory
        MemoryEntry memory(role, content);
        ProcessMemory(memory);
        return memory;
    }
    
    void ProcessMemory(MemoryEntry& memory) {
        // First handle the required memory processing
        // Note: We already have role and content from the constructor
        memory.timestamp = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());

        // Only calculate importance if enabled
        if (calculateImportance) {
            try {
                memory.importance = CalculateImportance(memory.content);
            } catch (const std::exception& e) {
                logger::warn("Importance calculation failed: {}. Using default.", e.what());
                memory.importance = 1.0f;  // Default to high importance on failure
            }
        } else {
            memory.importance = 1.0f;  // Default importance when calculation is disabled
        }
    }

    float CalculateImportance(const std::string& content) {
        try {
            nlohmann::json chat_request = {
                {"model", "gpt-4o-mini"},
                {"messages", {
                    {
                        {"role", "system"},
                        {"content", "You are an importance evaluator. Rate the importance "
                                "of memories on a scale of 1-10. Respond with only the "
                                "number, no explanation."}
                    },
                    {
                        {"role", "user"},
                        {"content", content}
                    }
                }},
                {"max_tokens", 1},  // We only need one token for the number
                {"temperature", 0.3}  // Lower temperature for more consistent ratings
            };

            // This call is synchronous, but we're running in an async context
            auto chat = openai::chat().create(chat_request);
            std::string response = chat["choices"][0]["message"]["content"];
            
            // Convert response to float and normalize to 0-1
            float importance = std::stof(response) / 10.0f;
            return std::clamp(importance, 0.0f, 1.0f);
        }
        catch (const std::exception& e) {
            logger::error("Importance calculation failed: {}", e.what());
            return 1.0f;  // Default to high importance on error
        }
    }

}



namespace TESSERACT::Agent {
    // Constructor definition
    SubAgent::SubAgent(RE::Actor* npc, const std::string& role) 
        : npc(npc), agentRole(role), isProcessingUpdate(false) {
        // Any additional initialization
    }

    // ProcessInput definition
    std::string SubAgent::ProcessInput(const std::string& input) {
        // Store what was said to us as a memory
        AddMemory("user", input);
        
        // Start the async request - think of this like placing your order
        // and getting a number, instead of waiting at the counter
        responseFuture = std::async(std::launch::async,
            [this, input]() {
                // This part runs in a separate thread
                auto context = PrepareContext();
                try {
                    // Make the API call
                    std::string response = Communication::SendOpenAIRequest(context, input);
                    return response;
                }
                catch (const std::exception& e) {
                    logger::error("Failed to process input: {}", e.what());
                    return std::string("I'm having trouble thinking clearly right now.");
                }
            }
        );

        // Return immediately while the request processes in the background
        return "";  // Empty string indicates processing started
    }


    void SubAgent::Update() {
        // First, let's handle any pending response from the OpenAI API
        if (responseFuture.valid()) {
            // Check if response is ready without blocking
            auto status = responseFuture.wait_for(std::chrono::milliseconds(0));
            
            if (status == std::future_status::ready) {
                try {
                    // Get the completed response - think of this like a thought 
                    // finally crystallizing in the agent's mind
                    std::string response = responseFuture.get();
                    
                    // Store this response as a memory
                    AddMemory("assistant", response);

                    // Update the latest line
                    latestResponse = response;  // <--- crucial line
                    
                    // Mark that we're done processing this thought
                    isProcessingUpdate.store(false);
                }
                catch (const std::exception& e) {
                    logger::error("Error processing response in Update: {}", e.what());
                    isProcessingUpdate.store(false);
                }
            }
            // If status is not ready, we just continue waiting - the response
            // will be checked again next update
        }

        // Later, you might add other background processes here, such as:
        /* Future Features - Commented out for now
        
        // Periodically consolidate memories
        if (shouldConsolidateMemories()) {
            consolidateMemories();
        }

        // Check for any completed importance calculations
        processImportanceCalculations();

        // Update active conversation threads
        updateThreads();

        // Process any queued actions or behaviors
        processActionQueue();
        */
    }


    // Private method definitions
    void SubAgent::AddMemory(const std::string& role, const std::string& content) {
        // Create new memory using our Memory system
        auto memory = Memory::CreateFromString(content, role);
        
        // Add the new memory to our collection
        memories.push_back(memory);
        
        // Basic memory management - just remove oldest if we exceed capacity
        if (memories.size() > Memory::maxMemories) {
            // Remove oldest memory (first in vector)
            memories.erase(memories.begin());
        }

        /* Future Implementation - Commented out for now
        // Sophisticated memory management
        while (memories.size() > Memory::config.maxMemories) {
            auto leastImportant = std::min_element(
                memories.begin(), memories.end(),
                [](const Memory::MemoryEntry& a, const Memory::MemoryEntry& b) {
                    return a.importance < b.importance;
                }
            );
            memories.erase(leastImportant);
        }
        */
    }


    std::vector<Communication::Message> SubAgent::PrepareContext() {
        std::vector<Communication::Message> context;
        
        // Combine personality and current state into one system message
        std::string systemPrompt = 
            Communication::GenerateSystemPrompt(npc) + "\n\n" +
            Communication::GetNPCContext(npc);
        
        // Add the combined system message
        context.push_back({
            "system",
            systemPrompt,
            std::time(nullptr)
        });
        
        // Add conversation history
        for (const auto& memory : memories) {
            context.push_back({
                memory.role,
                memory.content,
                memory.timestamp
            });
        }
        
        return context;
    }
}
