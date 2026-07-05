#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <vector>
#include <string>
#include <d3d11.h>
#include <iostream>
#include <thread>
#include <chrono>
//
#include <gui.hpp>
#include "../driver.h"
#include "../overlay.h"

extern void draw_menu();
globals_t globals;

int main() {
    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow) {
        ShowWindow(consoleWindow, SW_HIDE);
    }

    std::cout << "Initializing driver connection..." << std::endl;

    if (!Kernel->driverconnect()) {
        std::cout << "Failed to connect to driver!" << std::endl;
        std::cin.get();
        return -1;
    }

    std::cout << "Driver connected successfully!" << std::endl;

    INT32 processId = 0;
    std::cout << "Searching for Dead By Daylight..." << std::endl;

    const wchar_t* processNames[] = {
        L"DeadByDaylight-Win64-Shipping.exe",
        L"DeadByDaylight.exe",
        L"DBD.exe"
    };

    for (const auto& processName : processNames) {
        processId = Kernel->get_process_id(processName);
        if (processId != 0) {
            std::wcout << L"Found Dead By Daylight process: " << processName << L" (PID: " << processId << L")" << std::endl;
            break;
        }
    }

    if (processId == 0) {
        std::cout << "Dead By Daylight process not found! Make sure the game is running." << std::endl;
        std::cin.get();
        return -1;
    }

    Kernel->g_process_id = processId;

    std::cout << "Getting process base address..." << std::endl;
    uintptr_t baseAddress = Kernel->get_base_address();
    if (baseAddress == 0) {
        std::cout << "Failed to get base address!" << std::endl;
        std::cin.get();
        return -1;
    }

    Kernel->g_process_base = baseAddress;
    std::cout << "Base address: 0x" << std::hex << baseAddress << std::dec << std::endl;

    std::cout << "Caching CR3..." << std::endl;
    uintptr_t cr3 = Kernel->get_cr3();
    if (cr3 != 0) {
        Kernel->g_process_cr3 = cr3;
        std::cout << "CR3 cached successfully!" << std::endl;
    }

    std::cout << "Starting overlay..." << std::endl;

    try {
        overlay::start();
    }
    catch (const std::exception& e) {
        std::cout << "Overlay error: " << e.what() << std::endl;
        std::cin.get();
        return -1;
    }

    return 0;
}



void watermark(ImDrawList* draw_list) {

}