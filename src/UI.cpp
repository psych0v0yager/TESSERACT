#include "UI.h"
#include "Utils.h"
#include "HoldingQuestFunctions.h"
#include "Agent.h"


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
        
        // Initialize quests
        Dashboard::holdingQuest = RE::TESForm::LookupByEditorID<RE::TESQuest>(Dashboard::HOLDING_QUEST_EDITORID);
        Dashboard::placeholderQuest = RE::TESForm::LookupByEditorID<RE::TESQuest>(Dashboard::PLACEHOLDER_QUEST_EDITORID);

        if (!Dashboard::holdingQuest || !Dashboard::placeholderQuest) {
            logger::error("Failed to initialize quests in Register!");
            logger::error("HoldingQuest: {}", Dashboard::holdingQuest ? "Found" : "NULL");
            logger::error("PlaceholderQuest: {}", Dashboard::placeholderQuest ? "Found" : "NULL");
        } else {
            logger::info("Successfully initialized quests");
        }


        logger::info("Setting up TESSERACT menu section...");

        // Try to load config and connect to OpenAI on startup
        try {
            Config::LoadConfig();  // Now using UI::Config instead of ChatWindow::Config
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
        NPCDetails::Register();  // Make sure this is added
        
        logger::info("Menu section and components registered");
    }


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

        // void EnsureConfigDirectory() {
        //     try {
        //         // Get Skyrim's Data folder path
        //         auto dataPath = Skyrim::Get()->GetDataPath();
                
        //         // Construct the path for our config
        //         auto configDir = dataPath / "SKSE" / "Plugins" / "TESSERACT";
                
        //         // Create directory for new configs (this will go to Overwrite in MO2)
        //         if (!std::filesystem::exists(configDir)) {
        //             std::filesystem::create_directories(configDir);
        //             logger::info("Created config directory at: {}", configDir.string());
        //         }

        //         CONFIG_PATH = (configDir / "config.json").string();
        //         logger::info("Config path set to: {}", CONFIG_PATH);

        //     } catch (const std::exception& e) {
        //         lastError = std::format("Failed to create config directory: {}", e.what());
        //         loadSuccess = false;
        //         logger::error("{}", lastError);
        //     }
        // }

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
                
                // Load component-specific configurations
                Dashboard::LoadFromConfig(config);
                OpenAI::LoadFromConfig(config);
                Chat::LoadFromConfig(config);

                loadSuccess = true;
                logger::info("Config loaded successfully");

                // Try to establish OpenAI connection if credentials exist
                if (!OpenAI::baseUrl.empty() && !OpenAI::apiKey.empty()) {
                    OpenAI::StartConnection();
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
                
                // Save component-specific configurations
                Dashboard::SaveToConfig(config);
                OpenAI::SaveToConfig(config);
                Chat::SaveToConfig(config);

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

        // Dashboard config implementation
        namespace Dashboard {
            void SaveToConfig(nlohmann::json& config) {
                config["dashboard"] = {
                    {"npcCount", npcCount},
                    {"debugQuestEnabled", debugQuestEnabled}  // Added debug quest
                };
            }

            void LoadFromConfig(const nlohmann::json& config) {
                if (config.contains("dashboard")) {
                    const auto& dashboard = config["dashboard"];
                    if (dashboard.contains("npcCount")) {
                        npcCount = dashboard["npcCount"].get<int>();
                    }
                    if (dashboard.contains("debugQuestEnabled")) {
                        debugQuestEnabled = dashboard["debugQuestEnabled"].get<bool>();
                    }
                }
            }
        }

        // OpenAI config implementation
        namespace OpenAI {
            void SaveToConfig(nlohmann::json& config) {
                if (!baseUrl.empty() || !apiKey.empty()) {
                    config["openai"] = {
                        {"baseUrl", baseUrl},
                        {"apiKey", apiKey},
                        {"model", model}  // Add model here
                    };
                }
            }

            void LoadFromConfig(const nlohmann::json& config) {
                if (config.contains("openai")) {
                    const auto& openai = config["openai"];
                    if (openai.contains("baseUrl")) {
                        baseUrl = openai["baseUrl"].get<std::string>();
                    }
                    if (openai.contains("apiKey")) {
                        apiKey = openai["apiKey"].get<std::string>();
                    }
                    if (openai.contains("model")) {
                        model = openai["model"].get<std::string>();
                    }
                }
            }

            void StartConnection() {
                try {
                    openai::start(apiKey, "", true, baseUrl);
                    initialized.store(true);
                    logger::info("Successfully connected to OpenAI API");
                }
                catch (const std::exception& e) {
                    UI::Config::lastError = std::format("Failed to connect to OpenAI: {}", e.what());
                    UI::Config::loadSuccess = false;
                    logger::error("{}", UI::Config::lastError);
                }
            }
        }

        // Chat config implementation
        namespace Chat {
            void SaveToConfig(nlohmann::json& config) {
                config["chat"] = {
                    {"maxMessages", maxMessages}
                };
            }

            void LoadFromConfig(const nlohmann::json& config) {
                if (config.contains("chat")) {
                    const auto& chat = config["chat"];
                    if (chat.contains("maxMessages")) {
                        maxMessages = chat["maxMessages"].get<size_t>();
                    }
                }
            }
        }
    }

    namespace NPCDetails {
        // UI Drawing helpers
        namespace Drawing {
            void DrawNPCInfo() {
                // Create columns for portrait and info
                ImGui::Columns(2, "npc_info_columns", false);
                ImGui::SetColumnWidth(0, 160);  // Portrait column width

                // Left column: Portrait
                ImGui::BeginChild("Portrait", ImVec2(150, 150), true);
                ImGui::Text("[Portrait]");  // Placeholder for NPC portrait
                ImGui::EndChild();

                // Right column: Info list
                ImGui::NextColumn();
                ImGui::BeginGroup();
                ImGui::TextWrapped("Name: %s", "NPC Name");  // TODO: Get from JSON
                ImGui::TextWrapped("Race: %s", "Race");
                ImGui::TextWrapped("Level: %d", 1);
                ImGui::TextWrapped("Residence: %s", "Location");
                ImGui::TextWrapped("Relationship: %s", "None");
                ImGui::TextWrapped("Constitution: %s", "Normal");
                ImGui::EndGroup();

                ImGui::Columns(1);  // Reset columns
            }

            void DrawExperiencerContext() {
                ImGui::TextWrapped("Experiencer agent context and history will be displayed here.");
                // TODO: Add context history display
                // Example layout for context history:
                ImGui::BeginChild("ExperiencerHistory", ImVec2(0, 0), true);
                for (int i = 0; i < 10; i++) {
                    ImGui::TextWrapped("Sample context entry %d", i);
                    ImGui::Separator();
                }
                ImGui::EndChild();
            }

            void DrawNarratorContext() {
                ImGui::TextWrapped("Narrator agent context and history will be displayed here.");
                // TODO: Add context history display
                ImGui::BeginChild("NarratorHistory", ImVec2(0, 0), true);
                for (int i = 0; i < 10; i++) {
                    ImGui::TextWrapped("Sample narrative entry %d", i);
                    ImGui::Separator();
                }
                ImGui::EndChild();
            }

            void DrawPhysicalAgentContext() {
                ImGui::TextWrapped("Physical agent context and history will be displayed here.");
                // TODO: Add context history display
                ImGui::BeginChild("PhysicalAgentHistory", ImVec2(0, 0), true);
                for (int i = 0; i < 10; i++) {
                    ImGui::TextWrapped("Sample physical state entry %d", i);
                    ImGui::Separator();
                }
                ImGui::EndChild();
            }
        }

        void Open(int npcId) {
            selectedNpcId = npcId;
            detailsWindow->IsOpen = true;
            logger::info("Opening NPC Details window for NPC {}", npcId);
        }

        void Close() {
            detailsWindow->IsOpen = false;
            selectedNpcId = -1;
            logger::info("Closing NPC Details window");
        }

        void Register() {
            detailsWindow = SKSEMenuFramework::AddWindow(RenderWindow);
            logger::info("NPC Details window registered");
        }

        void __stdcall RenderWindow() {
            if (selectedNpcId == -1) {
                detailsWindow->IsOpen = false;
                return;
            }

            auto viewport = ImGui::GetMainViewport();
            
            // Size and position
            float windowWidth = viewport->Size.x * 0.3f;   // 30% of screen width
            float windowHeight = viewport->Size.y * 0.7f;  // 70% of screen height

            ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Appearing);
            ImGui::SetNextWindowPos(
                ImVec2(viewport->Size.x * 0.35f, viewport->Size.y * 0.15f), 
                ImGuiCond_Appearing,
                ImVec2(0.0f, 0.0f)
            );

            bool isOpen = detailsWindow->IsOpen;
            if (ImGui::Begin(std::format("NPC {} Details##NPCDetails", selectedNpcId).c_str(), 
                           &isOpen, 
                           ImGuiWindowFlags_NoCollapse)) {

                // Begin tab bar at the bottom
                if (ImGui::BeginTabBar("AgentTabs", TAB_FLAGS)) {
                    if (ImGui::BeginTabItem("Info")) {
                        Drawing::DrawNPCInfo();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Experiencer")) {
                        Drawing::DrawExperiencerContext();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Narrator")) {
                        Drawing::DrawNarratorContext();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Agent")) {
                        Drawing::DrawPhysicalAgentContext();
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
            }
            ImGui::End();

            if (detailsWindow->IsOpen != isOpen) {
                if (!isOpen) Close();
                detailsWindow->IsOpen = isOpen;
            }
        }
    }


    namespace Dashboard {
        void Register() {
            dashboardWindow = SKSEMenuFramework::AddWindow(RenderWindow);
        }

        void RefreshNPCs() {
            if (isRefreshing.exchange(true)) {
                logger::warn("Refresh already in progress");
                return;
            }

            try {
                logger::info("Starting manual NPC refresh");

                // Get player as scan center
                auto* player = RE::PlayerCharacter::GetSingleton();
                if (!player) {
                    logger::error("Cannot get player singleton");
                    return;
                }

                // Dashboard::holdingQuest = RE::TESForm::LookupByEditorID<RE::TESQuest>("TESSERACT_HoldingQuest_ActiveActors");
                // auto* placeholderQuest = RE::TESForm::LookupByEditorID<RE::TESQuest>("TESSERACT_PlaceholderQuest_PlaceholderStorage");

                if (!Dashboard::holdingQuest || !Dashboard::placeholderQuest) {
                    logger::error("Required quests not found");
                    return;
                }

                // Update placeholder set
                Dashboard::placeholderSet.clear();
                for (auto& [aliasID, handle] : Dashboard::placeholderQuest->refAliasMap) {
                    if (auto ref = handle.get().get()) {
                        Dashboard::placeholderSet.insert(ref);
                    }
                }

                // Use fast scanning function
                float scanRadius = 4000.0f;  // Adjust radius as needed
                auto scannedRefs = TESSERACT::Utils::FastScanningFunction(player, scanRadius);
                
                // Get unique NPCs
                auto uniqueNPCs = TESSERACT::HoldingQuest::GetUniqueNPCs(scannedRefs);
                
                // Update holding quest
                TESSERACT::HoldingQuest::FastQuestFill(Dashboard::holdingQuest, uniqueNPCs, Dashboard::placeholderQuest);

                // Update state
                State::activeNPCCount = std::count_if(uniqueNPCs.begin(), uniqueNPCs.end(),
                    [](RE::TESObjectREFR* ref) { return ref && ref->GetFormType() == RE::FormType::ActorCharacter; });

                logger::info("Manual refresh complete. Found {} active NPCs", State::activeNPCCount);

            } catch (const std::exception& e) {
                logger::error("Error during manual refresh: {}", e.what());
            }

            isRefreshing.store(false);
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

            // Calculate split point for the two columns
            int halfCount = (Config::Dashboard::npcCount + 1) / 2;  // Rounds up for odd numbers


            // This is the Top Row (The title bar)
            if (ImGui::Begin("NPC Dashboard", &isOpen, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse)) {

                FontAwesome::Pop();

                //|||| Reorganized menu bar with View on left, NPC count center, Status right
                    // Left - View Menu
                if (ImGui::BeginMenuBar()) {
                    // Left - View Menu
                    FontAwesome::PushSolid();
                    if (ImGui::BeginMenu("View")) {
                        // Single Refresh menu item with proper implementation
                        if (ImGui::MenuItem((Glyphs::RefreshIcon + " Refresh").c_str())) {
                            // Start refresh in a separate thread to avoid blocking UI
                            RefreshNPCs();
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Manually scan for and update NPCs");
                        }
                        
                        // Show refresh status if active
                        if (isRefreshing.load()) {
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Refreshing...");
                        }

                        if (ImGui::MenuItem((Glyphs::CloseIcon + " Close").c_str())) {
                            isOpen = false;
                        }
                        ImGui::EndMenu();
                    }

                    FontAwesome::Pop();
                    
                    ImGui::EndMenuBar();
                }


                // Main content area
                ImGui::Columns(2, "npc_columns", true);

                // Create ImVec2 to store region size
                auto availableRegion = ImGui::ImVec2Manager::Create();

                // Get available region size
                ImGui::GetContentRegionAvail(availableRegion);

                // Calculate available height for the lists
                float listHeight = availableRegion->y - ImGui::GetFrameHeightWithSpacing();

                // Clean up
                ImGui::ImVec2Manager::Destroy(availableRegion);


                // // First NPC List column
                // ImGui::BeginChild("NPCList1", ImVec2(0, listHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
                // FontAwesome::PushSolid();

                // for (int i = 0; i < halfCount; i++) {
                //     bool isSelected = NPCDetails::selectedNpcId == i + 1;
                //     if (ImGui::Selectable((Glyphs::NPCIcon + " NPC " + std::to_string(i + 1)).c_str(), isSelected)) {
                //         logger::debug("Selected NPC {} from first column", i + 1);
                //         NPCDetails::Open(i + 1);
                //     }

                // }


                // FontAwesome::Pop();
                // ImGui::EndChild();

                // // Second NPC List column
                // ImGui::NextColumn();
                // ImGui::BeginChild("NPCList2", ImVec2(0, listHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
                // FontAwesome::PushSolid();

                // for (int i = halfCount; i < Config::Dashboard::npcCount; i++) {
                //     bool isSelected = NPCDetails::selectedNpcId == i + 1;
                //     if (ImGui::Selectable((Glyphs::NPCIcon + " NPC " + std::to_string(i + 1)).c_str(), isSelected)) {
                //         logger::debug("Selected NPC {} from first column", i + 1);
                //         NPCDetails::Open(i + 1);
                //     }
                // }
                // FontAwesome::Pop();
                // ImGui::EndChild();
                
                // First NPC List column
                // Calculate split point for the two columns - moved up for clarity
                int totalNPCs = Config::Dashboard::npcCount;
                int halfCount = (totalNPCs + 1) / 2;  // Rounds up for odd numbers

                // First column (0 to halfCount-1)
                ImGui::BeginChild("NPCList1", ImVec2(0, listHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
                FontAwesome::PushSolid();

                // First half
                for (int i = 0; i < halfCount; i++) {
                    bool isSelected = NPCDetails::selectedNpcId == i;  // Changed from i+1
                    
                    std::string npcName = ": Empty";
                    if (Dashboard::holdingQuest) {
                        auto& aliasMap = Dashboard::holdingQuest->refAliasMap;
                        for (auto& [aliasID, handle] : aliasMap) {
                            if (aliasID == i) {  // Using i directly to match alias ID
                                if (auto ref = handle.get().get()) {
                                    if (Dashboard::placeholderSet.find(ref) == Dashboard::placeholderSet.end()) {
                                        npcName = std::format(": {}", ref->GetName());
                                    }
                                }
                                break;
                            }
                        }
                    }

                    if (ImGui::Selectable((Glyphs::NPCIcon + " NPC " + 
                                    std::to_string(i) +  // Changed from i+1
                                    npcName).c_str(), isSelected)) {
                        logger::debug("Selected NPC {} from first column", i);
                        NPCDetails::Open(i);  // Changed from i+1
                    }
                }

                FontAwesome::Pop();
                ImGui::EndChild();

                // Second column (halfCount to totalNPCs-1)
                ImGui::NextColumn();
                ImGui::BeginChild("NPCList2", ImVec2(0, listHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
                FontAwesome::PushSolid();

                for (int i = halfCount; i < totalNPCs; i++) {  // Changed from Config::Dashboard::npcCount
                    bool isSelected = NPCDetails::selectedNpcId == i;  // Changed from i+1
                    
                    std::string npcName = ": Empty";
                    if (Dashboard::holdingQuest) {
                        auto& aliasMap = Dashboard::holdingQuest->refAliasMap;
                        for (auto& [aliasID, handle] : aliasMap) {
                            if (aliasID == i) {  // Using i directly to match alias ID
                                if (auto ref = handle.get().get()) {
                                    if (Dashboard::placeholderSet.find(ref) == Dashboard::placeholderSet.end()) {
                                        npcName = std::format(": {}", ref->GetName());
                                    }
                                }
                                break;
                            }
                        }
                    }

                    if (ImGui::Selectable((Glyphs::NPCIcon + " NPC " + 
                                    std::to_string(i) +  // Changed from i+1
                                    npcName).c_str(), isSelected)) {
                        logger::debug("Selected NPC {} from second column", i);
                        NPCDetails::Open(i);  // Changed from i+1
                    }
                }
                FontAwesome::Pop();
                ImGui::EndChild();

                ImGui::Columns(1);

                // Add some padding before status bar
                ImGui::Spacing();
                ImGui::Spacing();

                // // Status bar with both elements
                // Simple status bar
                ImGui::Separator();
                // FontAwesome::PushSolid();
                // ImGui::Text("%s Active LLM NPCs: %d", Glyphs::NPCIcon.c_str(), State::activeNPCCount);
                // ImGui::SameLine();
                // ImGui::Text("%s Status: Active", Glyphs::ActiveIcon.c_str());
                // FontAwesome::Pop();

                FontAwesome::PushSolid();
                if (ImGui::BeginTable("status_bar", 2)) {  // 2 columns
                    ImGui::TableNextColumn();
                    ImGui::Text("%s Active LLM NPCs: %d", Glyphs::NPCIcon.c_str(), State::activeNPCCount);
                    ImGui::TableNextColumn();
                    ImGui::Text("%s Status: Active", Glyphs::ActiveIcon.c_str());
                    ImGui::EndTable();
                }
                FontAwesome::Pop();

            }
            ImGui::End();

            // Update the global IsOpen state
            if (dashboardWindow->IsOpen.load() != isOpen) {
                logger::info("Dashboard window state changed: {} -> {}", 
                    dashboardWindow->IsOpen.load(), isOpen);
                dashboardWindow->IsOpen.store(isOpen);
            }
        }
    }




    namespace ChatWindow {

        void Register() {
            chatWindow = SKSEMenuFramework::AddWindow(RenderWindow);
        }

        void Open(RE::Actor* targetNpc) {
            // OLD CODE
            // currentNPC = targetNpc;
            // OLD CODE
            // Now create an Agent object, and store the pointer
            currentNPC = std::make_unique<TESSERACT::Agent::SubAgent>(targetNpc, "test");
            // chatWindow->IsOpen = true;
            chatHistory.clear();
            // isThinking = false;
            isThinking.store(false);

            if (currentNPC) {
                // OLD CODE
                // Context::AddMessage("system", GenerateSystemPrompt());
                // OLD CODE
                chatHistory.push_back({
                    ChatMessage::Sender::NPC,
                    "Hello, how can I help you today?"
                });
            }
        }

        void Close() {
            chatWindow->IsOpen = false;
            // OLD CODE
            // currentNPC = nullptr;
            // OLD CODE
            currentNPC.reset();  // Properly destroy the agent
            chatHistory.clear();
            // OLD CODE
            // Context::messages.clear();
            // OLD CODE
        }

        // std::string GenerateSystemPrompt() {
        //     if (!currentNPC) return "";

        //     auto name = currentNPC->GetName();
        //     auto race = currentNPC->GetRace()->GetName();
        //     auto loc = currentNPC->GetCurrentLocation() ? 
        //                currentNPC->GetCurrentLocation()->GetName() : "Unknown Location";

        //     return std::format(
        //         "You are {}, a {} in {}. Maintain character and speak naturally. "
        //         "You have your own goals, personality, and daily routine. "
        //         "Keep responses concise and relevant to your character. "
        //         "Current time: {}, Weather: {}.",
        //         name, race, loc,
        //         "TODO: Add time", "TODO: Add weather"
        //     );
        // }

        // std::string GetNPCContext() {
        //     if (!currentNPC) return "";

        //     bool isSneaking = currentNPC->IsSneaking();
        //     bool isInCombat = currentNPC->IsInCombat();
        //     bool isAlarmed = currentNPC->IsAlarmed();
        //     float health = currentNPC->As<RE::ActorValueOwner>()->GetActorValue(RE::ActorValue::kHealth);
            
        //     return std::format(
        //         "Current state: {}"
        //         "Health: {:.0f}%\n"
        //         "Location: {}\n"
        //         "Time: {}\n",
        //         isInCombat ? "In Combat!" : 
        //         isAlarmed ? "Alarmed" : 
        //         isSneaking ? "Sneaking" : "Normal",
        //         health,
        //         currentNPC->GetCurrentLocation() ? 
        //             currentNPC->GetCurrentLocation()->GetName() : "Unknown",
        //         "TODO: Add time"
        //     );
        // }


        void __stdcall ChatWindow::RenderWindow() {
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
            // inline char formIdBuffer[9] = {0};  // 8 chars for hex + null terminator

            if (ImGui::Begin("Dialogue##ChatWindow", &isOpen, ImGuiWindowFlags_MenuBar)) {
                if (ImGui::BeginMenuBar()) {
                    // if (ImGui::BeginMenu("Options")) {
                    //     if (ImGui::MenuItem("Clear Chat")) {
                    //         chatHistory.clear();
                    //     }
                    //     if (ImGui::MenuItem("Close")) {
                    //         // Close();
                    //         isOpen = false;
                    //     }
                    //     ImGui::EndMenu();
                    // }

                    // if (ImGui::BeginMenu("Options")) {
                    //     if (ImGui::MenuItem("Clear Chat")) {
                    //         chatHistory.clear();
                    //     }

                    //     // Add a separator before debug options
                    //     ImGui::Separator();
                    //     ImGui::Text("Debug Options");

                    //     // Form ID input field
                    //     if (ImGui::InputText("Form ID", formIdBuffer, sizeof(formIdBuffer), 
                    //                         ImGuiInputTextFlags_CharsHexadecimal)) {
                    //         // Input is being modified
                    //     }
                        
                    //     if (ImGui::IsItemHovered()) {
                    //         ImGui::SetTooltip("Enter NPC Form ID in hex (e.g., 00000007 for player)\n"
                    //                         "You can find Form IDs using the console command 'help <name>'");
                    //     }

                    //     // Button to connect to NPC
                    //     if (ImGui::Button("Connect to NPC")) {
                    //         try {
                    //             // Convert hex string to integer
                    //             unsigned int formId;
                    //             std::stringstream ss;
                    //             ss << std::hex << formIdBuffer;
                    //             ss >> formId;

                    //             // Look up the NPC
                    //             if (auto form = RE::TESForm::LookupByID(formId)) {
                    //                 if (auto npc = form->As<RE::Actor>()) {
                    //                     // We found the NPC! Create a new agent
                    //                     logger::info("Found NPC: {}", npc->GetName());
                                        
                    //                     // Close existing agent if any
                    //                     if (currentNPC) {
                    //                         currentNPC.reset();
                    //                     }

                    //                     // Create new agent for this NPC
                    //                     currentNPC = std::make_unique<TESSERACT::Agent::SubAgent>(npc, "test");
                                        
                    //                     // Clear existing chat and add greeting
                    //                     chatHistory.clear();
                    //                     chatHistory.push_back({
                    //                         ChatMessage::Sender::NPC,
                    //                         "Hello, I am " + std::string(npc->GetName())
                    //                     });
                    //                 } else {
                    //                     logger::error("Form {} is not an Actor", formId);
                    //                 }
                    //             } else {
                    //                 logger::error("Could not find Form with ID {}", formId);
                    //             }
                    //         }
                    //         catch (const std::exception& e) {
                    //             logger::error("Error connecting to NPC: {}", e.what());
                    //         }
                    //     }

                    //     // Add status text showing current NPC
                    //     if (currentNPC) {
                    //         ImGui::TextColored(
                    //             ImVec4(0.0f, 1.0f, 0.0f, 1.0f),  // Green text
                    //             "Connected to: %s", 
                    //             currentNPC->GetNPC()->GetName()
                    //         );
                    //     } else {
                    //         ImGui::TextColored(
                    //             ImVec4(0.7f, 0.7f, 0.7f, 1.0f),  // Gray text
                    //             "Not connected to any NPC"
                    //         );
                    //     }

                    //     if (ImGui::MenuItem("Close")) {
                    //         isOpen = false;
                    //     }
                        
                    //     ImGui::EndMenu();
                    // }


                    if (ImGui::BeginMenu("Options")) {
                        // Basic chat management section
                        if (ImGui::MenuItem("Clear Chat")) {
                            chatHistory.clear();
                        }

                        // Debug section with clear visual separation
                        ImGui::Separator();
                        ImGui::Text("Debug Options");

                        // Form ID input section - This lets users specify which NPC to connect to
                        if (ImGui::InputText("Form ID", formIdBuffer, sizeof(formIdBuffer), 
                                            ImGuiInputTextFlags_CharsHexadecimal)) {
                            // We don't need to do anything here - the buffer updates automatically
                        }
                        
                        // Help tooltip - Explains how to use the Form ID input
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Enter NPC Form ID in hex (e.g., 00000007 for player)\n"
                                            "You can find Form IDs using the console command 'help <name>'");
                        }

                        // Connection status - Show this before the connect button so users know the current state
                        if (currentNPC && currentNPC->GetNPC()) {  // Double-check both the agent and NPC exist
                            ImGui::TextColored(
                                ImVec4(0.0f, 1.0f, 0.0f, 1.0f),  // Green for connected
                                "Connected to: %s", 
                                currentNPC->GetNPC()->GetName()
                            );
                        } else {
                            ImGui::TextColored(
                                ImVec4(0.7f, 0.7f, 0.7f, 1.0f),  // Gray for disconnected
                                "Not connected to any NPC"
                            );
                        }

                        // Connect button and NPC lookup logic
                        if (ImGui::Button("Connect to NPC")) {
                            try {
                                // First validate that we have input
                                if (strlen(formIdBuffer) == 0) {
                                    logger::warn("No Form ID entered");
                                    return;
                                }

                                // Convert hex string to integer
                                unsigned int formId;
                                std::stringstream ss;
                                ss << std::hex << formIdBuffer;
                                ss >> formId;

                                if (ss.fail()) {
                                    logger::error("Invalid Form ID format");
                                    return;
                                }

                                // Look up the NPC with careful null checking
                                auto form = RE::TESForm::LookupByID(formId);
                                if (!form) {
                                    logger::error("Could not find Form with ID {:X}", formId);
                                    return;
                                }

                                auto npc = form->As<RE::Actor>();
                                if (!npc) {
                                    logger::error("Form {:X} is not an Actor", formId);
                                    return;
                                }

                                // We found a valid NPC - now we can safely create the agent
                                logger::info("Found NPC: {}", npc->GetName());
                                
                                // Clean up existing agent before creating new one
                                if (currentNPC) {
                                    currentNPC.reset();
                                }

                                // Create new agent and initialize chat
                                currentNPC = std::make_unique<TESSERACT::Agent::SubAgent>(npc, "test");
                                chatHistory.clear();
                                chatHistory.push_back({
                                    ChatMessage::Sender::NPC,
                                    "Hello, I am " + std::string(npc->GetName())
                                });
                            }
                            catch (const std::exception& e) {
                                logger::error("Error connecting to NPC: {}", e.what());
                            }
                        }

                        // Add another separator before window management
                        ImGui::Separator();
                        
                        // Window management at the end
                        if (ImGui::MenuItem("Close")) {
                            isOpen = false;
                        }
                        
                        ImGui::EndMenu();
                    }

                    // ImGui::EndMenuBar();



                    ImGui::EndMenuBar();
                }



                // Add OpenAI connection status check
                if (!UI::Config::OpenAI::initialized.load()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.855f, 0.392f, 0.392f, 1.0f));
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
                // if (ImGui::Button("Send") || sendMessage) {
                //     if (strlen(inputBuffer) > 0) {
                //         std::string userMessage = inputBuffer;  // Create copy of message
                //         logger::info("Attempting to send message: '{}'", userMessage);  // Debug log
                //         chatHistory.push_back({
                //             ChatMessage::Sender::User,
                //             std::string(inputBuffer)
                //         });
                        
                //         // isThinking = true;
                //         isThinking.store(true);
                //         // autoScroll.store(true);  // Re-enable when sending message
                //         thinkingAnimationTimer = std::chrono::steady_clock::now();
                        
                //         // Properly pass a std::string to SendOpenAIRequest with namespace qualification
                //         aiResponseFuture = std::async(std::launch::async, 
                //             &UI::ChatWindow::SendOpenAIRequest, userMessage);  // Pass std::string
                        
                //         memset(inputBuffer, 0, sizeof(inputBuffer));
                //     }
                // }

                // ProcessResponse();
                ImGui::SameLine();
                // if (ImGui::Button("Send") || sendMessage) {
                //     if (strlen(inputBuffer) > 0 && currentNPC) {  // Check if we have an agent
                //         std::string userMessage = inputBuffer;
                        
                //         // Add user message to chat display
                //         chatHistory.push_back({
                //             ChatMessage::Sender::User,
                //             userMessage
                //         });
                        
                //         // Start the async processing through the agent
                //         isThinking.store(true);
                //         thinkingAnimationTimer = std::chrono::steady_clock::now();
                        
                //         // OLD CODE:
                //         // aiResponseFuture = std::async(std::launch::async, 
                //         //     &UI::ChatWindow::SendOpenAIRequest, userMessage);
                        
                //         // NEW CODE:
                //         // Start agent's processing
                //         aiResponseFuture = std::async(std::launch::async,
                //             [this, userMessage]() {
                //                 return currentNPC->ProcessInput(userMessage);
                //             });
                        
                //         memset(inputBuffer, 0, sizeof(inputBuffer));
                //     }
                // }
                // Inside RenderWindow()
                if (ImGui::Button("Send") || sendMessage) {
                    if (strlen(inputBuffer) > 0 && currentNPC) {
                        std::string userMessage = inputBuffer;
                        
                        // Add user message to chat display
                        chatHistory.push_back({
                            ChatMessage::Sender::User,
                            userMessage
                        });
                        
                        // Start the async processing
                        isThinking.store(true);
                        thinkingAnimationTimer = std::chrono::steady_clock::now();
                        
                        // Modify this part to avoid using 'this'
                        // aiResponseFuture = std::async(std::launch::async,
                        //     [capturedNPC = currentNPC.get(), userMessage]() {
                        //         return capturedNPC->ProcessInput(userMessage);
                        //     });

                         // Just call ProcessInput directly - no async wrapper
                        currentNPC->ProcessInput(userMessage);
                        
                        memset(inputBuffer, 0, sizeof(inputBuffer));
                    }
                }

                // Add agent update call
                if (currentNPC) {
                    currentNPC->Update();  // Let agent handle its background tasks
                    // Check for new responses
                    if (!currentNPC->latestResponse.empty()) {
                        // We have a new response to display
                        chatHistory.push_back({
                            ChatMessage::Sender::NPC,
                            currentNPC->latestResponse
                        });
                        currentNPC->latestResponse.clear();  // Clear it so we don't display it again
                        isThinking.store(false);  // Stop thinking animation
                        autoScroll.store(true);   // Scroll to show new message
                    }

                }

                // Handle UI response processing
                // ProcessResponse();

            }
            ImGui::End();

            // Step 4: Sync local state back to global at end of frame
            if (chatWindow->IsOpen.load() != isOpen) {
                logger::info("Chat window state changed: {} -> {}", 
                    chatWindow->IsOpen.load(), isOpen);
                chatWindow->IsOpen.store(isOpen);
            }
        }


        // std::string SendOpenAIRequest(const std::string& userInput) {

        //     //Update OpenAI check
        //     if (!UI::Config::OpenAI::initialized.load()) {
        //         return "I'm not connected to OpenAI yet. Please check your settings.";
        //     }

        //     try {
        //         Context::AddMessage("user", userInput);

        //         nlohmann::json chat_request = {
        //             {"model", "gpt-4o-mini"},
        //             {"messages", nlohmann::json::array()}
        //         };

        //         // Single system message combining identity and context
        //         chat_request["messages"].push_back({
        //             {"role", "system"},
        //             {"content", GenerateSystemPrompt() + "\n\n" + GetNPCContext()}
        //         });

        //         auto history = Context::GetMessageHistory();
        //         chat_request["messages"].insert(
        //             chat_request["messages"].end(),
        //             history.begin(), history.end()
        //         );

        //         // Log final request
        //         logger::info("Final OpenAI Request: {}", chat_request.dump(2));


        //         auto chat = openai::chat().create(chat_request);
        //         std::string response = chat["choices"][0]["message"]["content"];
                
        //         Context::AddMessage("assistant", response);
                
        //         return response;
        //     }
        //     catch (const std::exception& e) {
        //         logger::error("OpenAI request failed: {}", e.what());
        //         return "I'm sorry, I'm having trouble thinking clearly right now.";
        //     }
        // }

        std::string SendOpenAIRequest(const std::string& userInput) {
            // OLD: Handled all the OpenAI interaction here
            // NEW: Should delegate to the Agent

            // First check if we have an agent and OpenAI is initialized
            if (!currentNPC) {
                return "Error: No active NPC agent";
            }
            if (!UI::Config::OpenAI::initialized.load()) {
                return "I'm not connected to OpenAI yet. Please check your settings.";
            }

            // Use the Agent's ProcessInput method
            return currentNPC->ProcessInput(userInput);
        }


        void ProcessResponse() {
            if (!isThinking.load() || !aiResponseFuture.valid()) {  // Add .load()
                return;
            }

            auto status = aiResponseFuture.wait_for(std::chrono::milliseconds(0));
            if (status == std::future_status::ready) {
                std::string response = aiResponseFuture.get();
                chatHistory.push_back({
                    ChatMessage::Sender::NPC,
                    response
                });
                isThinking.store(false);  // Add .store()
                autoScroll.store(true);  // Re-enable when receiving response
            }
        }

        // namespace Context {
        //     void AddMessage(const std::string& role, const std::string& content) {
        //         nlohmann::json message = {
        //             {"role", role},
        //             {"content", content}
        //         };
                
        //         messages.push_back(message);
                
        //         if (messages.size() > UI::Config::Chat::maxMessages) {
        //             messages.erase(messages.begin());
        //         }

        //     }

        //     nlohmann::json GetMessageHistory() {
        //         return messages;
        //     }
        // }
    }

    // In UI.cpp, update the Settings::RenderMenu function
    namespace Settings {
        void __stdcall RenderMenu() {
            FontAwesome::PushSolid();
            ImGui::Text("%s TESSERACT Settings", Dashboard::Glyphs::SettingsIcon.c_str());

            // Dashboard Settings
            ImGui::Separator();
            ImGui::Text("Dashboard Settings");
            
            int npcCount = Config::Dashboard::npcCount;
            if (ImGui::InputInt("Number of NPCs", &npcCount)) {
                npcCount = std::clamp(npcCount, 2, 128);
                Config::Dashboard::npcCount = npcCount;
                Config::SaveConfig();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Maximum number of NPCs to manage (2-128).\n"
                                "Changes take effect on next scan.");
            }

            // Debug Settings
            ImGui::Separator();
            ImGui::Text("Debug Settings");

            bool debugEnabled = Config::Dashboard::debugQuestEnabled;
            if (ImGui::Checkbox("Enable Debug Quest", &debugEnabled)) {
                Config::Dashboard::debugQuestEnabled = debugEnabled;
                Config::SaveConfig();
                
                // Toggle debug quest
                if (auto debugQuest = RE::TESForm::LookupByEditorID<RE::TESQuest>("TESSERACT_DebugQuest")) {
                    if (debugEnabled) {
                        debugQuest->Start();
                        logger::info("Debug quest started");
                    } else {
                        debugQuest->Stop();
                        logger::info("Debug quest stopped");
                    }
                } else {
                    logger::error("Could not find TESSERACT_DebugQuest");
                    ImGui::OpenPopup("Debug Quest Error");
                }
            }

            // Debug status indicator
            ImGui::SameLine();
            ImGui::TextColored(
                Config::Dashboard::debugQuestEnabled ? 
                ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                Config::Dashboard::debugQuestEnabled ? "Active" : "Inactive"
            );

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Enables debug spells and functionality.\n"
                                "WARNING: For testing purposes only!");
            }

            // Error popup
            if (ImGui::BeginPopupModal("Debug Quest Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Could not find TESSERACT_DebugQuest!\n"
                        "Make sure the mod is properly installed.");
                if (ImGui::Button("OK")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            // OpenAI Settings
            ImGui::Separator();
            ImGui::Text("OpenAI Settings");

            char baseUrl[1024] = "";
            char apiKey[1024] = "";
            char model[1024] = "";  // Add model buffer
            
            strcpy_s(baseUrl, sizeof(baseUrl), Config::OpenAI::baseUrl.c_str());
            strcpy_s(apiKey, sizeof(apiKey), Config::OpenAI::apiKey.c_str());
            strcpy_s(model, sizeof(model), Config::OpenAI::model.c_str());  // Copy model

            bool urlChanged = ImGui::InputText("Base URL", baseUrl, sizeof(baseUrl));
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Model API endpoint URL.\n"
                                "Leave empty for default OpenAI endpoint.");
            }

            bool keyChanged = ImGui::InputText("API Key", apiKey, sizeof(apiKey), 
                                            ImGuiInputTextFlags_Password);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Your OpenAI API key.\n"
                                "Required for AI functionality.");
            }

            bool modelChanged = ImGui::InputText("Model", model, sizeof(model));  // Add model input
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Model that you are using (e.g., gpt-4o-mini, deepseek-chat).\n"
                                "Leave empty for default model (gpt-4o-mini).");
            }


            if (urlChanged) Config::OpenAI::baseUrl = baseUrl;
            if (keyChanged) Config::OpenAI::apiKey = apiKey;
            if (modelChanged) Config::OpenAI::model = model;  // Update model if changed

            if (ImGui::Button(Config::OpenAI::initialized.load() ? 
                            "Reconnect to OpenAI" : "Connect to OpenAI")) {
                Config::OpenAI::StartConnection();
                Config::SaveConfig();
            }
            
            // OpenAI connection status
            ImGui::SameLine();
            ImGui::TextColored(
                Config::OpenAI::initialized.load() ? 
                ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                Config::OpenAI::initialized.load() ? "Connected" : "Not Connected"
            );

            FontAwesome::Pop();
        }
    }

}
