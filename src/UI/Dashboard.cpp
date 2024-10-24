// Dashboard.cpp
#include "UI/Dashboard.h"

void UI::Dashboard::Register() {
    Window = SKSEMenuFramework::AddWindow(RenderWindow);
}

void __stdcall UI::Dashboard::RenderWindow() {
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
                // API Key input with password masking
                static char apiKey[1024] = "";
                if (ImGui::InputText("OpenAI API Key", apiKey, IM_ARRAYSIZE(apiKey), 
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
            if (ImGui::Selectable((Glyphs::NPCIcon + " NPC " + std::to_string(i)).c_str())) {
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

void __stdcall UI::Settings::RenderMenu() {
    FontAwesome::PushSolid();
    ImGui::Text("%s TESSERACT Settings", Dashboard::Glyphs::SettingsIcon.c_str());
    
    if (ImGui::Button((Dashboard::Glyphs::DashboardTitle + " Open Dashboard").c_str())) {
        Dashboard::Window->IsOpen = true;
    }
    FontAwesome::Pop();
}