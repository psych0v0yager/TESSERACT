#include "UI/ChatWindow.h"

namespace UI {
    void ChatWindow::Register() {
        Window = SKSEMenuFramework::AddWindow(RenderWindow);
    }

    void ChatWindow::Open(RE::Actor* targetNpc) {
        currentNPC = targetNpc;
        Window->IsOpen = true;
        chatHistory.clear();
        isThinking = false;
        
        // Initialize chat with NPC greeting
        if (currentNPC) {
            // Add system context for OpenAI
            context.AddMessage("system", GenerateSystemPrompt());
            
            // Add initial greeting to visual chat history
            chatHistory.push_back({
                ChatMessage::Sender::NPC,
                "Hello, how can I help you today?"  // Will be AI generated once OpenAI is integrated
            });
        }
    }

    void ChatWindow::Close() {
        Window->IsOpen = false;
        currentNPC = nullptr;
        chatHistory.clear();
        // Clear OpenAI context when closing
        context.messages.clear();
    }

    // Generates comprehensive system prompt for OpenAI based on NPC state
    std::string ChatWindow::GenerateSystemPrompt() {
        if (!currentNPC) return "";

        // Get NPC info
        auto name = currentNPC->GetName();
        auto race = currentNPC->GetRace()->GetName();
        auto loc = currentNPC->GetCurrentLocation() ? 
                   currentNPC->GetCurrentLocation()->GetName() : "Unknown Location";

        // Create rich context for the AI
        return fmt::format(
            "You are {}, a {} in {}. Maintain character and speak naturally. "
            "You have your own goals, personality, and daily routine. "
            "Keep responses concise and relevant to your character. "
            "Current time: {}, Weather: {}.",
            name, race, loc,
            "TODO: Add time", "TODO: Add weather"  // We can add these later
        );
    }

    // Gets current NPC state information for context
    std::string ChatWindow::GetNPCContext() {
        if (!currentNPC) return "";

        // Get current NPC state
        bool isSneaking = currentNPC->IsSneaking();
        bool isInCombat = currentNPC->IsInCombat();
        bool isAlarmed = currentNPC->IsAlarmed();
        float health = currentNPC->GetActorValue(RE::ActorValue::kHealth);
        
        return fmt::format(
            "Current state: {}"
            "Health: {:.0f}%\n"
            "Location: {}\n"
            "Time: {}\n",
            isInCombat ? "In Combat!" : 
            isAlarmed ? "Alarmed" : 
            isSneaking ? "Sneaking" : "Normal",
            health,
            currentNPC->GetCurrentLocation() ? 
                currentNPC->GetCurrentLocation()->GetName() : "Unknown",
            "TODO: Add time"  // We can add this later
        );
    }

    // Add message to context with memory management
    void ChatWindow::ChatContext::AddMessage(const std::string& role, 
                                           const std::string& content) {
        nlohmann::json message = {
            {"role", role},
            {"content", content}
        };
        
        messages.push_back(message);
        
        // Keep memory window limited
        if (messages.size() > maxMessages) {
            messages.erase(messages.begin());
        }
    }

    nlohmann::json ChatWindow::ChatContext::GetMessageHistory() const {
        return messages;
    }

    void __stdcall ChatWindow::RenderWindow() {
        auto viewport = ImGui::GetMainViewport();

        // Center and size the window
        auto center = ImGui::ImVec2Manager::Create();
        ImGui::ImGuiViewportManager::GetCenter(center, viewport);
        ImGui::SetNextWindowPos(*center, ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
        ImGui::ImVec2Manager::Destroy(center);
        
        ImGui::SetNextWindowSize(ImVec2{viewport->Size.x * 0.5f, viewport->Size.y * 0.6f}, ImGuiCond_Appearing);

        // Main window
        if (ImGui::Begin("Dialogue##ChatWindow", nullptr, ImGuiWindowFlags_MenuBar)) {
            // Menu bar
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("Options")) {
                    if (ImGui::MenuItem("Clear Chat")) {
                        chatHistory.clear();
                    }
                    if (ImGui::MenuItem("Close")) {
                        Close();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            // Chat history display area
            ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
            ImVec2 chatHistorySize(contentRegionAvail.x, contentRegionAvail.y - ImGui::GetFrameHeightWithSpacing() * 2);
            
            ImGui::BeginChild("ScrollingRegion", chatHistorySize, false, ImGuiWindowFlags_HorizontalScrollbar);
            
            // Render chat messages with color coding
            for (const auto& message : chatHistory) {
                const bool isUser = message.sender == ChatMessage::Sender::User;
                ImGui::PushStyleColor(ImGuiCol_Text, 
                    isUser ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f)  // User text in green
                          : ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // NPC text in yellow
                
                ImGui::Text("%s: %s", message.GetSenderName().c_str(), message.content.c_str());
                ImGui::PopStyleColor();
                ImGui::Spacing();
            }

            // Handle auto-scrolling
            if (autoScroll) {
                ImGui::SetScrollHereY(1.0f);
            }

            // Update auto-scroll based on user interaction
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                autoScroll = true;
            else if (ImGui::IsMouseDragging(0))
                autoScroll = false;

            ImGui::EndChild();

            // Thinking animation
            if (isThinking) {
                auto now = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - thinkingAnimationTimer);
                int dots = (duration.count() / 500) % 4;
                std::string thinkingText = "Thinking" + std::string(dots, '.');
                ImGui::Text("%s", thinkingText.c_str());
            }

            // Input area with send button
            bool sendMessage = false;
            if (ImGui::InputText("##ChatInput", inputBuffer, sizeof(inputBuffer), 
                               ImGuiInputTextFlags_EnterReturnsTrue)) {
                sendMessage = true;
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Send") || sendMessage) {
                if (strlen(inputBuffer) > 0) {
                    // Add user message to chat history
                    chatHistory.push_back({
                        ChatMessage::Sender::User,
                        std::string(inputBuffer)
                    });
                    
                    // Start async AI response
                    isThinking = true;
                    thinkingAnimationTimer = std::chrono::steady_clock::now();
                    aiResponseFuture = std::async(std::launch::async, 
                        &ChatWindow::SendOpenAIRequest, this, inputBuffer);
                    
                    // Clear input after sending
                    memset(inputBuffer, 0, sizeof(inputBuffer));
                }
            }

            // Process AI response if ready
            ProcessResponse();
        }
        ImGui::End();
    }

    // Send message to OpenAI and get response
    std::string ChatWindow::SendOpenAIRequest(const std::string& userInput) {
        try {
            // Update context with user input
            context.AddMessage("user", userInput);

            // Create chat completion request
            nlohmann::json chat_request = {
                {"model", "gpt-4"},  // or whatever model you prefer
                {"messages", nlohmann::json::array()}
            };

            // Add system prompt
            chat_request["messages"].push_back({
                {"role", "system"},
                {"content", GenerateSystemPrompt()}
            });

            // Add NPC state as context
            chat_request["messages"].push_back({
                {"role", "system"},
                {"content", GetNPCContext()}
            });

            // Add message history
            auto history = context.GetMessageHistory();
            chat_request["messages"].insert(
                chat_request["messages"].end(),
                history.begin(), history.end()
            );

            // Send request to OpenAI
            auto chat = openai::chat().create(chat_request);
            std::string response = chat["choices"][0]["message"]["content"];
            
            // Update context with AI response
            context.AddMessage("assistant", response);
            
            return response;
        }
        catch (const std::exception& e) {
            logger::error("OpenAI request failed: {}", e.what());
            return "I'm sorry, I'm having trouble thinking clearly right now.";
        }
    }

    // Process async AI response
    void ChatWindow::ProcessResponse() {
        if (!isThinking || !aiResponseFuture.valid()) {
            return;
        }

        auto status = aiResponseFuture.wait_for(std::chrono::milliseconds(0));
        if (status == std::future_status::ready) {
            std::string response = aiResponseFuture.get();
            chatHistory.push_back({
                ChatMessage::Sender::NPC,
                response
            });
            isThinking = false;
        }
    }
}