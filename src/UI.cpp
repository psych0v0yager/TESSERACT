#include "UI.h"

namespace UI {
    //**** New menu render functions - these get called to draw items in the dropdown
    void __stdcall Dashboard::RenderMenuItem() {
        FontAwesome::PushSolid();
        if (ImGui::MenuItem((Glyphs::DashboardTitle + " Open Dashboard").c_str())) {
            dashboardWindow->IsOpen.store(true);
            logger::debug("Dashboard opened from menu");
        }
        FontAwesome::Pop();
    }

    void __stdcall ChatWindow::RenderMenuItem() {
        FontAwesome::PushSolid();
        if (ImGui::MenuItem("👤 Open Debug Chat")) {
            chatWindow->IsOpen.store(true);
            Open(nullptr);
            logger::debug("Debug chat opened from menu");
        }
        FontAwesome::Pop();
    }

    //**** Modified Register function to use the render functions
    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) {
            logger::error("SKSE Menu Framework not found!");
            return;
        }
        
        logger::info("Setting up TESSERACT menu section...");

        // Try to load config and connect to OpenAI on startup
        try {
            ChatWindow::Config::LoadConfig();  // This already attempts connection if config exists
            logger::info("Initial config load complete");
        }
        catch (const std::exception& e) {
            logger::warn("Initial OpenAI connection attempt failed: {}. Chat will be unavailable until configured.", e.what());
            // Don't return - let the rest of the UI load
        }
            
        SKSEMenuFramework::SetSection("TESSERACT");

        // Register menu items with their render functions
        SKSEMenuFramework::AddSectionItem("Dashboard", Dashboard::RenderMenuItem);
        SKSEMenuFramework::AddSectionItem("Settings", Settings::RenderMenu);
        SKSEMenuFramework::AddSectionItem("Debug Chat", ChatWindow::RenderMenuItem);

        // Register windows
        Dashboard::Register();
        ChatWindow::Register();
        
        logger::info("Menu section and components registered");
    }


    namespace Dashboard {
        void Register() {
            dashboardWindow = SKSEMenuFramework::AddWindow(RenderWindow);
        }

        void __stdcall RenderWindow() {
            auto viewport = ImGui::GetMainViewport();

            auto center = ImGui::ImVec2Manager::Create();
            ImGui::ImGuiViewportManager::GetCenter(center, viewport);
            ImGui::SetNextWindowPos(*center, ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
            ImGui::ImVec2Manager::Destroy(center);
            
            ImGui::SetNextWindowSize(ImVec2{viewport->Size.x * 0.6f, viewport->Size.y * 0.7f}, ImGuiCond_Appearing);
            
            FontAwesome::PushSolid();

            // Load the atomic IsOpen value into a local bool
            bool isOpen = dashboardWindow->IsOpen.load();

            // Pass &isOpen to ImGui::Begin()
            // if (ImGui::Begin("Dialogue##ChatWindow", &isOpen, ImGuiWindowFlags_MenuBar)) {
            
            if (ImGui::Begin(Glyphs::DashboardTitle.c_str(), &isOpen, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse)) {

            // if (ImGui::Begin(Glyphs::DashboardTitle.c_str(), nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse)) {
                FontAwesome::Pop();

                // Menu Bar
                if (ImGui::BeginMenuBar()) {

                    // View Menu
                    FontAwesome::PushSolid();
                    if (ImGui::BeginMenu("View")) {
                        if (ImGui::MenuItem((Glyphs::RefreshIcon + " Refresh").c_str())) {
                            // Refresh dashboard data
                        }
                        if (ImGui::MenuItem((Glyphs::CloseIcon + " Close").c_str())) {
                            // Window->IsOpen = false;
                            isOpen = false;
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

            // Update the global IsOpen state
            // Window->IsOpen.store(isOpen);
            // Update global state if changed
            // if (dashboardWindow->IsOpen.load() != isOpen) {
            //     dashboardWindow->IsOpen.store(isOpen);
            // }

            if (dashboardWindow->IsOpen.load() != isOpen) {
                logger::info("Dashboard window state changed: {} -> {}", 
                    dashboardWindow->IsOpen.load(), isOpen);
                dashboardWindow->IsOpen.store(isOpen);
            }
        }
    }

    namespace ChatWindow {

        // Config boilerplate
        namespace Config {
            void EnsureConfigDirectory() {
                try {
                    std::filesystem::path configPath(CONFIG_PATH);
                    auto parent = configPath.parent_path();
                    if (!std::filesystem::exists(parent)) {
                        std::filesystem::create_directories(parent);
                    }
                } catch (const std::exception& e) {
                    lastError = std::format("Failed to create config directory: {}", e.what());
                    loadSuccess = false;
                    logger::error("{}", lastError);
                }
            }

            void LoadConfig() {
                try {
                    // Reset status
                    loadSuccess = false;
                    lastError = "";

                    // Ensure directory exists
                    EnsureConfigDirectory();

                    // Check if file exists
                    if (!std::filesystem::exists(CONFIG_PATH)) {
                        logger::info("No config file found. Will create on first save.");
                        return;
                    }

                    // Read and parse file
                    std::ifstream file(CONFIG_PATH);
                    if (!file.is_open()) {
                        throw std::runtime_error("Cannot open config file");
                    }

                    auto config = nlohmann::json::parse(file);
                    
                    // Load values if they exist and aren't empty
                    if (config.contains("openai")) {
                        const auto& openai = config["openai"];
                        
                        // Only override if values are empty or we're loading for first time
                        if (openaiURL.empty() && openai.contains("baseUrl")) {
                            openaiURL = openai["baseUrl"].get<std::string>();
                        }
                        if (openaiAPIKey.empty() && openai.contains("apiKey")) {
                            openaiAPIKey = openai["apiKey"].get<std::string>();
                        }
                    }

                    loadSuccess = true;
                    logger::info("Config loaded successfully");

                    // Try to connect if we have both values
                    // if (!openaiURL.empty() && !openaiAPIKey.empty() && !openaiInitialized) {
                    if (!openaiURL.empty() && !openaiAPIKey.empty() && !openaiInitialized.load()) {
                        StartOpenAI(openaiURL, openaiAPIKey);
                    }
                }
                catch (const std::exception& e) {
                    lastError = std::format("Failed to load config: {}", e.what());
                    loadSuccess = false;
                    logger::error("{}", lastError);
                }
            }

            void SaveConfig() {
                try {
                    // Ensure directory exists
                    EnsureConfigDirectory();

                    // Create config object
                    nlohmann::json config;
                    
                    // Only save non-empty values
                    if (!openaiURL.empty() || !openaiAPIKey.empty()) {
                        config["openai"] = {
                            {"baseUrl", openaiURL},
                            {"apiKey", openaiAPIKey}
                        };
                    }

                    // Write to file
                    std::ofstream file(CONFIG_PATH);
                    if (!file.is_open()) {
                        throw std::runtime_error("Cannot open config file for writing");
                    }
                    
                    file << config.dump(2);  // Pretty print with 2-space indent
                    loadSuccess = true;
                    lastError = "";
                    logger::info("Config saved successfully");
                }
                catch (const std::exception& e) {
                    lastError = std::format("Failed to save config: {}", e.what());
                    loadSuccess = false;
                    logger::error("{}", lastError);
                }
            }

            std::string GetErrorMessage() {
                return lastError.empty() ? "No errors" : lastError;
            }
        }

        // Update StartOpenAI to handle configuration
        void StartOpenAI(const std::string& url, const std::string& key) {
            try {
                openai::start(key, "", true, url);
                // openaiInitialized = true;
                openaiInitialized.store(true);
                
                // Save successful configuration
                openaiURL = url;
                openaiAPIKey = key;
                Config::SaveConfig();
                
                logger::info("Successfully connected to OpenAI API");
            } 
            catch (const std::exception& e) {
                Config::lastError = std::format("Failed to connect to OpenAI: {}", e.what());
                Config::loadSuccess = false;
                logger::error("{}", Config::lastError);
            }
        }


        void Register() {
            chatWindow = SKSEMenuFramework::AddWindow(RenderWindow);
        }

        void Open(RE::Actor* targetNpc) {
            currentNPC = targetNpc;
            // chatWindow->IsOpen = true;
            chatHistory.clear();
            // isThinking = false;
            isThinking.store(false);
            
            if (currentNPC) {
                Context::AddMessage("system", GenerateSystemPrompt());
                chatHistory.push_back({
                    ChatMessage::Sender::NPC,
                    "Hello, how can I help you today?"
                });
            }
        }

        void Close() {
            chatWindow->IsOpen = false;
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


        void __stdcall ChatWindow::RenderWindow() {
            // auto viewport = ImGui::GetMainViewport();

            // auto center = ImGui::ImVec2Manager::Create();
            // ImGui::ImGuiViewportManager::GetCenter(center, viewport);
            // ImGui::SetNextWindowPos(*center, ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
            // ImGui::ImVec2Manager::Destroy(center);
            
            // ImGui::SetNextWindowSize(ImVec2{viewport->Size.x * 0.5f, viewport->Size.y * 0.6f}, ImGuiCond_Appearing);

            auto viewport = ImGui::GetMainViewport();
            
            // First, adjust the window size to be larger vertically
            float windowWidth = viewport->Size.x * 0.4f;   // 40% of screen width
            float windowHeight = viewport->Size.y * 0.6f;  // Increased to 60% of screen height (from 0.4f)

            float posX = viewport->Size.x - windowWidth - (viewport->Size.x * 0.02f);
            float posY = viewport->Size.y - windowHeight - (viewport->Size.y * 0.1f);

            ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Appearing, ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Appearing);


            // Load the atomic IsOpen value into a local bool
            bool isOpen = chatWindow->IsOpen.load();

            if (ImGui::Begin("Dialogue##ChatWindow", &isOpen, ImGuiWindowFlags_MenuBar)) {
                if (ImGui::BeginMenuBar()) {
                    if (ImGui::BeginMenu("Options")) {
                        if (ImGui::MenuItem("Clear Chat")) {
                            chatHistory.clear();
                        }
                        if (ImGui::MenuItem("Close")) {
                            // Close();
                            isOpen = false;
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }

                // Add OpenAI connection status check
                if (!openaiInitialized.load()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.855f, 0.392f, 0.392f, 1.0f)); // Light red
                    ImGui::TextWrapped("⚠️ Not connected to OpenAI. Please check your settings.");
                    if (ImGui::Button("Open Settings")) {
                        SKSEMenuFramework::SetSection("TESSERACT");
                        Settings::RenderMenu();
                    }
                    ImGui::Separator();
                    ImGui::PopStyleColor();
                }

                // Get the content region available size
                ImVec2 contentRegionAvail;
                ImGui::GetContentRegionAvail(&contentRegionAvail);

                // Calculate chat history size based on available content region
                ImVec2 chatHistorySize(contentRegionAvail.x, contentRegionAvail.y - ImGui::GetFrameHeightWithSpacing() * 2);


                ImGui::BeginChild("ScrollingRegion", chatHistorySize, false, ImGuiWindowFlags_HorizontalScrollbar);

                for (const auto& message : chatHistory) {
                    const bool isUser = message.sender == ChatMessage::Sender::User;
                    ImGui::PushStyleColor(ImGuiCol_Text, 
                        isUser ? ImVec4(0.729f, 0.855f, 0.984f, 1.0f) : ImVec4(0.824f, 0.706f, 0.549f, 1.0f));
                    
                    // // Add sender name on its own line
                    // ImGui::Text("%s:", message.GetSenderName().c_str());
                    
                    // // Add message content with text wrapping
                    // ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - 20.0f);  // Leave small margin
                    // ImGui::TextUnformatted(message.content.c_str());  // TextUnformatted handles large texts better
                    
                    // Put sender and message on same line but wrap the text
                    ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - 20.0f);
                    ImGui::Text("%s: %s", message.GetSenderName().c_str(), message.content.c_str());
                    ImGui::PopTextWrapPos();
                    
                    ImGui::PopStyleColor();
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                }

                // In RenderWindow:
                auto io = ImGui::GetIO(); // Get IO struct
                float mouseWheel = io->MouseWheel;  // Get wheel movement

                // Disable autoscroll on any scrolling interaction
                if (ImGui::IsMouseDragging(0) || mouseWheel != 0.0f) {
                    autoScroll.store(false);
                }

                // Use autoscroll if enabled
                if (autoScroll.load()) {
                    ImGui::SetScrollHereY(1.0f);
                }

                ImGui::EndChild();

                // Thinking animation gets its own line of reserved space
                ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight())); // Reserve space

                if (isThinking.load()) {
                    // Position thinking text in the reserved space
                    float thinkPos = ImGui::GetCursorPosY() - ImGui::GetFrameHeight();
                    ImGui::SetCursorPosY(thinkPos);

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
                        std::string userMessage = inputBuffer;  // Create copy of message
                        logger::info("Attempting to send message: '{}'", userMessage);  // Debug log
                        chatHistory.push_back({
                            ChatMessage::Sender::User,
                            std::string(inputBuffer)
                        });
                        
                        // isThinking = true;
                        isThinking.store(true);
                        // autoScroll.store(true);  // Re-enable when sending message
                        thinkingAnimationTimer = std::chrono::steady_clock::now();
                        
                        // Properly pass a std::string to SendOpenAIRequest with namespace qualification
                        aiResponseFuture = std::async(std::launch::async, 
                            &UI::ChatWindow::SendOpenAIRequest, userMessage);  // Pass std::string
                        
                        memset(inputBuffer, 0, sizeof(inputBuffer));
                    }
                }

                ProcessResponse();
            }
            ImGui::End();

            // Step 4: Sync local state back to global at end of frame
            if (chatWindow->IsOpen.load() != isOpen) {
                logger::info("Chat window state changed: {} -> {}", 
                    chatWindow->IsOpen.load(), isOpen);
                chatWindow->IsOpen.store(isOpen);
            }
        }


        std::string SendOpenAIRequest(const std::string& userInput) {
            if (!openaiInitialized.load()) {
                return "I'm not connected to OpenAI yet. Please check your settings.";
            }
            
            try {
                Context::AddMessage("user", userInput);

                nlohmann::json chat_request = {
                    {"model", "gpt-4o-mini"},
                    {"messages", nlohmann::json::array()}
                };

                // Single system message combining identity and context
                chat_request["messages"].push_back({
                    {"role", "system"},
                    {"content", GenerateSystemPrompt() + "\n\n" + GetNPCContext()}
                });

                auto history = Context::GetMessageHistory();
                chat_request["messages"].insert(
                    chat_request["messages"].end(),
                    history.begin(), history.end()
                );

                // Log final request
                logger::info("Final OpenAI Request: {}", chat_request.dump(2));


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
            if (!isThinking.load() || !aiResponseFuture.valid()) {  // Add .load()
            // if (!isThinking || !aiResponseFuture.valid()) {
                return;
            }

            auto status = aiResponseFuture.wait_for(std::chrono::milliseconds(0));
            if (status == std::future_status::ready) {
                std::string response = aiResponseFuture.get();
                chatHistory.push_back({
                    ChatMessage::Sender::NPC,
                    response
                });
                // isThinking = false;
                isThinking.store(false);  // Add .store()
                autoScroll.store(true);  // Re-enable when receiving response
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
    }

    namespace Settings {
        void __stdcall RenderMenu() {
            FontAwesome::PushSolid();
            ImGui::Text("%s TESSERACT Settings", Dashboard::Glyphs::SettingsIcon.c_str());

            // Load config on first open
            static bool configLoaded = false;
            if (!configLoaded) {
                ChatWindow::Config::LoadConfig();
                configLoaded = true;
            }

            // Cache current values for detecting changes
            static char baseURL[1024] = "";
            static char apiKey[1024] = "";
            
            // Update buffers if empty and we have saved values
            if (strlen(baseURL) == 0 && !ChatWindow::openaiURL.empty()) {
                strcpy_s(baseURL, sizeof(baseURL), ChatWindow::openaiURL.c_str());
            }
            if (strlen(apiKey) == 0 && !ChatWindow::openaiAPIKey.empty()) {
                strcpy_s(apiKey, sizeof(apiKey), ChatWindow::openaiAPIKey.c_str());
            }

            // Input fields
            bool urlChanged = ImGui::InputText("OpenAI Base URL", baseURL, sizeof(baseURL));
            bool keyChanged = ImGui::InputText("OpenAI API Key", apiKey, sizeof(apiKey), 
                                            ImGuiInputTextFlags_Password);

            // Update stored values if changed
            if (urlChanged) {
                ChatWindow::openaiURL = baseURL;
            }
            if (keyChanged) {
                ChatWindow::openaiAPIKey = apiKey;
            }

            // Connect button
            // if (ImGui::Button(ChatWindow::openaiInitialized ? 
            if (ImGui::Button(ChatWindow::openaiInitialized.load() ? 
                            "Reconnect to OpenAI" : "Connect to OpenAI")) {
                ChatWindow::StartOpenAI(ChatWindow::openaiURL, ChatWindow::openaiAPIKey);
            }

            // Status display
            // if (ChatWindow::openaiInitialized) {
            if (ChatWindow::openaiInitialized.load()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                ImGui::Text("✓ Connected to OpenAI");
                ImGui::PopStyleColor();
            }
            else if (!ChatWindow::Config::lastError.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("✗ Error: %s", ChatWindow::Config::lastError.c_str());
                ImGui::PopStyleColor();
            }

            // Save button (optional, since we auto-save on successful connection)
            if (urlChanged || keyChanged) {
                if (ImGui::Button("Save Settings")) {
                    ChatWindow::Config::SaveConfig();
                }
            }

            FontAwesome::Pop();
        }
    }
}