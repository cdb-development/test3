#pragma once
#include <imgui.h>

class GUI {
    ImVec2 size;
public:
    static GUI& get() {
        static GUI instance;
        return instance;
    }
};

// DEKLARATION der globalen draw_menu() Funktion
extern void draw_menu();