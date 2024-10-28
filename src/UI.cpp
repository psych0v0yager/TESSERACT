#include "UI.h"

namespace UI {
    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) {
            return;
        }
        SKSEMenuFramework::SetSection("TESSERACT");
        Dashboard::Register();
        ChatWindow::Register();
    }

    namespace Dashboard {
        void Register() {
            Window = SKSEMenuFramework::AddWindow(RenderWindow);
        }

        void __stdcall RenderWindow() {
            auto viewport = ImGui::GetMainViewport();

            auto center = ImGui::ImVec2Manager::Create();
            ImGui::ImGuiViewportManager::GetCenter(center, viewport);
            ImGui::SetNextWindowPos(*center, ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
            ImGui::ImVec2Manager::Destroy(center);
            
            ImGui::SetNextWindowSize(ImVec2{viewport->Size.x * 0.6f, viewport->Size.y * 0.7f}, ImGuiCond_Appearing);
            
            FontAwesome::PushSolid();
            if (ImGui::Begin(Glyphs::DashboardTitle.c_str(), nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse)) {
                FontAwesome::Pop();

                // Menu Bar
                if (ImGui::BeginMenuBar()) {
                    // View Menu
                    if (ImGui::BeginMenu("View")) {
                        FontAwesome::PushSolid();
                        if (ImGui::MenuItem((Glyphs::RefreshIcon + " Refresh").c_str())) {
                            // Refresh dashboard data
                        }
                        if (ImGui::MenuItem((Glyphs::CloseIcon + " Close").c_str())) {
                            Window->IsOpen = false;
                        }
                        FontAwesome::Pop();
                        ImGui::EndMenu();
                    }
                    
                    // Settings Menu
                    FontAwesome::PushSolid();
                    if (ImGui::BeginMenu((Glyphs::SettingsIcon + " Settings").c_str())) {
                        static char apiKey[1024] = "";
                        if (ImGui::InputText("OpenAI API Key", apiKey, sizeof(apiKey), 
                                           ImGuiInputTextFlags_Password)) {
                            ChatWindow::SetAPIKey(apiKey);
                        }
                        ImGui::EndMenu();
                    }
                    FontAwesome::Pop();
                    
                    ImGui::EndMenuBar();
                }

                // Main content area
                ImGui::Columns(2, "dashboard_columns", true);
                
                // Left column: NPC List
                FontAwesome::PushSolid();
                ImGui::Text("%s Active LLM NPCs: %d", Glyphs::NPCIcon.c_str(), State::activeNPCCount);
                FontAwesome::Pop();

                ImGui::BeginChild("NPCList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
                FontAwesome::PushSolid();
                for (int i = 0; i < 5; i++) {
                    if (ImGui::Selectable((Glyphs::NPCIcon + " NPC " + std::to_string(i)).c_str(), false, 0, ImVec2(0,0))) {
                        // Handle NPC selection
                    }
                }
                FontAwesome::Pop();
                ImGui::EndChild();
                
                // Right column: Map
                ImGui::NextColumn();
                
                FontAwesome::PushSolid();
                ImGui::Text("%s Local Map", Glyphs::MapIcon.c_str());
                FontAwesome::Pop();

                ImGui::BeginChild("Map", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
                // Map visualization will go here
                ImGui::EndChild();

                ImGui::Columns(1);

                // Status bar
                ImGui::Separator();
                FontAwesome::PushSolid();
                ImGui::Text("%s Status: Active", Glyphs::ActiveIcon.c_str());
                FontAwesome::Pop();
            }
            ImGui::End();
        }
    }

    namespace ChatWindow {
        void Register() {
            Window = SKSEMenuFramework::AddWindow(RenderWindow);
        }

        void Open(RE::Actor* targetNpc) {
            currentNPC = targetNpc;
            Window->IsOpen = true;
            chatHistory.clear();
            isThinking = false;
            
            if (currentNPC) {
                Context::AddMessage("system", GenerateSystemPrompt());
                chatHistory.push_back({
                    ChatMessage::Sender::NPC,
                    "Hello, how can I help you today?"
                });
            }
        }

        void Close() {
            Window->IsOpen = false;
            currentNPC = nullptr;
            chatHistory.clear();
            Context::messages.clear();
        }

        std::string GenerateSystemPrompt() {
            if (!currentNPC) return "";

            auto name = currentNPC->GetName();
            auto race = currentNPC->GetRace()->GetName();
            auto loc = currentNPC->GetCurrentLocation() ? 
                       currentNPC->GetCurrentLocation()->GetName() : "Unknown Location";

            return std::format(
                "You are {}, a {} in {}. Maintain character and speak naturally. "
                "You have your own goals, personality, and daily routine. "
                "Keep responses concise and relevant to your character. "
                "Current time: {}, Weather: {}.",
                name, race, loc,
                "TODO: Add time", "TODO: Add weather"
            );
        }

        std::string GetNPCContext() {
            if (!currentNPC) return "";

            bool isSneaking = currentNPC->IsSneaking();
            bool isInCombat = currentNPC->IsInCombat();
            bool isAlarmed = currentNPC->IsAlarmed();
            float health = currentNPC->As<RE::ActorValueOwner>()->GetActorValue(RE::ActorValue::kHealth);
            
            return std::format(
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
                "TODO: Add time"
            );
        }

        void __stdcall RenderWindow() {
            auto viewport = ImGui::GetMainViewport();

            auto center = ImGui::ImVec2Manager::Create();
            ImGui::ImGuiViewportManager::GetCenter(center, viewport);
            ImGui::SetNextWindowPos(*center, ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
            ImGui::ImVec2Manager::Destroy(center);
            
            ImGui::SetNextWindowSize(ImVec2{viewport->Size.x * 0.5f, viewport->Size.y * 0.6f}, ImGuiCond_Appearing);

            if (ImGui::Begin("Dialogue##ChatWindow", nullptr, ImGuiWindowFlags_MenuBar)) {
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

                // ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
                // ImVec2 chatHistorySize(contentRegionAvail.x, contentRegionAvail.y - ImGui::GetFrameHeightWithSpacing() * 2);
                
                // Get the content region available size
                ImVec2 contentRegionAvail;
                ImGui::GetContentRegionAvail(&contentRegionAvail);

                // Calculate chat history size based on available content region
                ImVec2 chatHistorySize(contentRegionAvail.x, contentRegionAvail.y - ImGui::GetFrameHeightWithSpacing() * 2);



                ImGui::BeginChild("ScrollingRegion", chatHistorySize, false, ImGuiWindowFlags_HorizontalScrollbar);
                
                for (const auto& message : chatHistory) {
                    const bool isUser = message.sender == ChatMessage::Sender::User;
                    ImGui::PushStyleColor(ImGuiCol_Text, 
                        isUser ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                    
                    ImGui::Text("%s: %s", message.GetSenderName().c_str(), message.content.c_str());
                    ImGui::PopStyleColor();
                    ImGui::Spacing();
                }

                if (autoScroll) {
                    ImGui::SetScrollHereY(1.0f);
                }

                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                    autoScroll = true;
                else if (ImGui::IsMouseDragging(0))
                    autoScroll = false;

                ImGui::EndChild();

                if (isThinking) {
                    auto now = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - thinkingAnimationTimer);
                    int dots = (duration.count() / 500) % 4;
                    std::string thinkingText = "Thinking" + std::string(dots, '.');
                    ImGui::Text("%s", thinkingText.c_str());
                }

                bool sendMessage = false;
                if (ImGui::InputText("##ChatInput", inputBuffer, sizeof(inputBuffer), 
                                   ImGuiInputTextFlags_EnterReturnsTrue)) {
                    sendMessage = true;
                }
                
                ImGui::SameLine();
                if (ImGui::Button("Send") || sendMessage) {
                    if (strlen(inputBuffer) > 0) {
                        chatHistory.push_back({
                            ChatMessage::Sender::User,
                            std::string(inputBuffer)
                        });
                        
                        isThinking = true;
                        thinkingAnimationTimer = std::chrono::steady_clock::now();
                        aiResponseFuture = std::async(std::launch::async, 
                            SendOpenAIRequest, inputBuffer);  // Note: Changed to namespace function
                        
                        memset(inputBuffer, 0, sizeof(inputBuffer));
                    }
                }

                ProcessResponse();
            }
            ImGui::End();
        }

        std::string SendOpenAIRequest(const std::string& userInput) {
            try {
                Context::AddMessage("user", userInput);

                nlohmann::json chat_request = {
                    {"model", "gpt-4"},
                    {"messages", nlohmann::json::array()}
                };

                chat_request["messages"].push_back({
                    {"role", "system"},
                    {"content", GenerateSystemPrompt()}
                });

                chat_request["messages"].push_back({
                    {"role", "system"},
                    {"content", GetNPCContext()}
                });

                auto history = Context::GetMessageHistory();
                chat_request["messages"].insert(
                    chat_request["messages"].end(),
                    history.begin(), history.end()
                );

                auto chat = openai::chat().create(chat_request);
                std::string response = chat["choices"][0]["message"]["content"];
                
                Context::AddMessage("assistant", response);
                
                return response;
            }
            catch (const std::exception& e) {
                logger::error("OpenAI request failed: {}", e.what());
                return "I'm sorry, I'm having trouble thinking clearly right now.";
            }
        }

        void ProcessResponse() {
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

        namespace Context {
            void AddMessage(const std::string& role, const std::string& content) {
                nlohmann::json message = {
                    {"role", role},
                    {"content", content}
                };
                
                messages.push_back(message);
                
                if (messages.size() > maxMessages) {
                    messages.erase(messages.begin());
                }
            }

            nlohmann::json GetMessageHistory() {
                return messages;
            }
        }

        void SetAPIKey(const std::string& key) {
            if (!openaiInitialized) {
                openai::start(key);
                openaiInitialized = true;
            }
        }
    }

    namespace Settings {
        void __stdcall RenderMenu() {
            FontAwesome::PushSolid();
            ImGui::Text("%s TESSERACT Settings", Dashboard::Glyphs::SettingsIcon.c_str());
            
            if (ImGui::Button((Dashboard::Glyphs::DashboardTitle + " Open Dashboard").c_str())) {
                Dashboard::Window->IsOpen = true;
            }
            FontAwesome::Pop();
        }
    }
}