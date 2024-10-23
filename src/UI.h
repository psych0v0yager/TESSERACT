#pragma once

namespace UI {
    void Register();
    
    namespace MainMenu {
        void __stdcall Render();
        inline MENU_WINDOW MainWindow;
    }
}