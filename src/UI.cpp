#include "UI.h"

void UI::Register() {
    if (!SKSEMenuFramework::IsInstalled()) {
        logger::error("SKSE Menu Framework not installed!");
        return;
    }

    SKSEMenuFramework::SetSection("TESSERACT");
    UI::MainMenu::MainWindow = SKSEMenuFramework::AddWindow(MainMenu::Render);
    SKSEMenuFramework::AddSectionItem("Open Menu", []() {
        MainMenu::MainWindow->IsOpen = true;
    });
}

void __stdcall UI::MainMenu::Render() {
    auto viewport = ImGui::GetMainViewport();
    
    // Center the window
    auto center = ImGui::ImVec2Manager::Create();
    ImGui::ImGuiViewportManager::GetCenter(center, viewport);
    ImGui::SetNextWindowPos(*center, ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
    ImGui::ImVec2Manager::Destroy(center);
    
    // Set window size
    ImGui::SetNextWindowSize(ImVec2{viewport->Size.x * 0.4f, viewport->Size.y * 0.4f}, ImGuiCond_Appearing);
    
    // Begin window
    if (ImGui::Begin("TESSERACT##MainWindow", nullptr, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Close", "Ctrl+W")) {
                    MainWindow->IsOpen = false;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::Text("Welcome to TESSERACT");
        
        if (ImGui::Button("Close Window")) {
            MainWindow->IsOpen = false;
        }
    }
    ImGui::End();
}