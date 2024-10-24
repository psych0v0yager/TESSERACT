// Dashboard.h
#pragma once
#include "SKSEMenuFramework.h"
#include "UI/ChatWindow.h"

namespace UI {
    class Dashboard {
    public:
        static void Register();
        static void __stdcall RenderWindow();
        inline static MENU_WINDOW Window;

        // Icon definitions (keep as is)
        struct Glyphs {
            inline static std::string DashboardTitle = "TESSERACT " + FontAwesome::UnicodeToUtf8(0xf080);
            inline static std::string RefreshIcon = FontAwesome::UnicodeToUtf8(0xf021);
            inline static std::string CloseIcon = FontAwesome::UnicodeToUtf8(0xf00d);
            inline static std::string NPCIcon = FontAwesome::UnicodeToUtf8(0xf007);
            inline static std::string MapIcon = FontAwesome::UnicodeToUtf8(0xf279);
            inline static std::string ActiveIcon = FontAwesome::UnicodeToUtf8(0xf111);
            inline static std::string SettingsIcon = FontAwesome::UnicodeToUtf8(0xf013);
        };

        // Dashboard state/data
        struct State {
            inline static int activeNPCCount = 0;
        };
    };

    namespace Settings {
        void __stdcall RenderMenu();
    }
}