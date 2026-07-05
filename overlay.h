#define IMGUI_DEFINE_MATH_OPERATORS
#include <D3D11.h>
#include <dxgi.h>
#include <dwmapi.h>
#include <thread>
#include <shellapi.h>
#include <tlhelp32.h>
#include "slowdown.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "spoof.h"
#include "driver.h"
#include "settings.h"
#include "actor.h"
#include "objects.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "kernel32.lib")

void draw_menu();
void watermark(ImDrawList* draw_list);

static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

DWORD imgui_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;

HWND hwnd;
RECT rc;

static HWND g_hOSK = nullptr;
static bool g_bOSKInitialized = false;

static DWORD g_target_process_id = 0;
static DWORD g_main_process_id = 0;
static std::thread g_process_monitor_thread;
static std::thread g_main_monitor_thread;
static bool g_should_exit = false;

static HANDLE g_exit_event = nullptr;
static HANDLE g_cleanup_mutex = nullptr;

void Error()
{
    getchar();
    exit(-1);
}

ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

ID3D11Device* d3d_device;
ID3D11DeviceContext* d3d_device_ctx;
IDXGISwapChain* d3d_swap_chain;
ID3D11RenderTargetView* d3d_render_target;
// Removed D3DPRESENT_PARAMETERS as it's not used in D3D11

static ID3D11ShaderResourceView* users = nullptr;

typedef struct _Header
{
    UINT Magic;
    UINT FrameCount;
    UINT NoClue;
    UINT Width;
    UINT Height;
    BYTE Buffer[1];
} Header;

namespace overlay
{
    void CloseOSK();
    void PerformCleanup();
    BOOL WINAPI ConsoleHandler(DWORD dwType)
    {
        switch (dwType)
        {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            g_should_exit = true;
            if (g_exit_event) SetEvent(g_exit_event);
            CloseOSK();
            Sleep(1000);
            return TRUE;
        }
        return FALSE;
    }
    void PerformCleanup()
    {
        if (g_cleanup_mutex) {
            WaitForSingleObject(g_cleanup_mutex, INFINITE);
        }
        if (!g_should_exit) {
            g_should_exit = true;
        }
        CloseOSK();
        if (g_process_monitor_thread.joinable()) {
            g_process_monitor_thread.join();
        }
        if (g_main_monitor_thread.joinable()) {
            g_main_monitor_thread.join();
        }
        if (ImGui::GetCurrentContext()) {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }
        if (d3d_render_target) {
            d3d_render_target->Release();
            d3d_render_target = nullptr;
        }
        if (d3d_swap_chain) {
            d3d_swap_chain->Release();
            d3d_swap_chain = nullptr;
        }
        if (d3d_device_ctx) {
            d3d_device_ctx->Release();
            d3d_device_ctx = nullptr;
        }
        if (d3d_device) {
            d3d_device->Release();
            d3d_device = nullptr;
        }
        if (globals.window_handle) {
            DestroyWindow(globals.window_handle);
            globals.window_handle = nullptr;
        }

        if (g_cleanup_mutex) {
            ReleaseMutex(g_cleanup_mutex);
        }
        PostQuitMessage(0);
        exit(0);
    }
    void CloseOSK()
    {
        if (g_hOSK && IsWindow(g_hOSK))
        {
            PostMessage(g_hOSK, WM_CLOSE, 0, 0);
            Sleep(500);
            if (IsWindow(g_hOSK))
            {
                DWORD oskProcessId = 0;
                GetWindowThreadProcessId(g_hOSK, &oskProcessId);

                if (oskProcessId != 0)
                {
                    HANDLE hOSKProcess = OpenProcess(PROCESS_TERMINATE, FALSE, oskProcessId);
                    if (hOSKProcess)
                    {
                        TerminateProcess(hOSKProcess, 0);
                        CloseHandle(hOSKProcess);
                    }
                }
            }

            g_hOSK = nullptr;
        }
        HWND hOskWindow = nullptr;
        do {
            hOskWindow = FindWindow(OBF(L"OSKMainClass"), nullptr);
            if (hOskWindow)
            {
                PostMessage(hOskWindow, WM_CLOSE, 0, 0);
                Sleep(100);
            }
        } while (hOskWindow);
    }
    void MainProcessMonitor()
    {
        while (!g_should_exit)
        {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, g_main_process_id);

            if (hProcess == NULL)
            {
                g_should_exit = true;
                if (g_exit_event) SetEvent(g_exit_event);
                PerformCleanup();
                break;
            }

            DWORD exitCode = 0;
            if (GetExitCodeProcess(hProcess, &exitCode))
            {
                if (exitCode != STILL_ACTIVE)
                {
                    CloseHandle(hProcess);
                    g_should_exit = true;
                    if (g_exit_event) SetEvent(g_exit_event);
                    PerformCleanup();
                    break;
                }
            }
            CloseHandle(hProcess);
            Sleep(500);
        }
    }
    void ProcessMonitor()
    {
        while (!g_should_exit)
        {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, g_target_process_id);

            if (hProcess == NULL)
            {
                g_should_exit = true;
                if (g_exit_event) SetEvent(g_exit_event);
                PerformCleanup();
                break;
            }

            DWORD exitCode = 0;
            if (GetExitCodeProcess(hProcess, &exitCode))
            {
                if (exitCode != STILL_ACTIVE)
                {
                    CloseHandle(hProcess);
                    g_should_exit = true;
                    if (g_exit_event) SetEvent(g_exit_event);
                    PerformCleanup();
                    break;
                }
            }
            CloseHandle(hProcess);
            Sleep(1000);
        }
    }
    void DisableOSKContent()
    {
        if (g_hOSK && IsWindow(g_hOSK))
        {
            EnumChildWindows(g_hOSK, [](HWND hwnd, LPARAM) -> BOOL {
                ShowWindow(hwnd, SW_HIDE);
                SetWindowTextA(hwnd, OBF(""));
                SetWindowTextW(hwnd, OBF(L""));
                SetWindowPos(hwnd, HWND_BOTTOM, -2000, -2000, 1, 1, SWP_NOACTIVATE | SWP_NOZORDER);
                return TRUE;
                },
                0);
            SetWindowTextA(g_hOSK, OBF(""));
            SetWindowTextW(g_hOSK, OBF(L""));
        }
    }
    void EnsureOSKRunning()
    {
        if (!g_hOSK || !IsWindow(g_hOSK))
        {
            bool osk_launched = false;

            HINSTANCE result = ShellExecuteA(nullptr, OBF("open"), OBF("osk.exe"), nullptr, nullptr, SW_HIDE);
            if ((intptr_t)result > 32) {
                osk_launched = true;
            }
            if (!osk_launched) {
                STARTUPINFOA si = { 0 };
                PROCESS_INFORMATION pi = { 0 };
                si.cb = sizeof(si);
                si.dwFlags = STARTF_USESHOWWINDOW;
                si.wShowWindow = SW_HIDE;

                char oskPath[MAX_PATH];
                GetSystemDirectoryA(oskPath, MAX_PATH);
                strcat_s(oskPath, OBF("\\osk.exe"));

                if (CreateProcessA(oskPath, nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                    osk_launched = true;
                }
            }
            if (!osk_launched) {
                char systemPath[MAX_PATH];
                GetSystemDirectoryA(systemPath, MAX_PATH);
                strcat_s(systemPath, OBF("\\osk.exe"));

                HINSTANCE result2 = ShellExecuteA(nullptr, OBF("open"), systemPath, nullptr, nullptr, SW_HIDE);
                if ((intptr_t)result2 > 32) {
                    osk_launched = true;
                }
            }
            if (!osk_launched) {
                if (WinExec(OBF("osk.exe"), SW_HIDE) > 31) {
                    osk_launched = true;
                }
            }
            for (int i = 0; i < 50 && !g_hOSK; i++) {
                Sleep(100);

                g_hOSK = FindWindowA(OBF("OSKMainClass"), nullptr);
                if (!g_hOSK) {
                    g_hOSK = FindWindowW(OBF(L"OSKMainClass"), nullptr);
                }
                if (!g_hOSK) {
                    g_hOSK = FindWindowA(OBF("OSK"), nullptr);
                }
                if (!g_hOSK) {
                    g_hOSK = FindWindowA(OBF("On-Screen Keyboard"), nullptr);
                }
                if (!g_hOSK) {
                    g_hOSK = FindWindowExA(NULL, NULL, OBF("OSKMainClass"), NULL);
                }
            }
            if (!g_hOSK) {
                EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
                    char className[256];
                    char windowText[256];
                    GetClassNameA(hwnd, className, sizeof(className));
                    GetWindowTextA(hwnd, windowText, sizeof(windowText));

                    if (strstr(className, OBF("OSK")) != nullptr ||
                        strstr(windowText, OBF("On-Screen Keyboard")) != nullptr ||
                        strstr(windowText, OBF("Bildschirmtastatur")) != nullptr) {
                        g_hOSK = hwnd;
                        return FALSE;
                    }
                    return TRUE;
                    }, 0);
            }
        }
    }

    bool InitImgui()
    {
        DXGI_SWAP_CHAIN_DESC swap_chain_description;
        ZeroMemory(&swap_chain_description, sizeof(swap_chain_description));
        swap_chain_description.BufferCount = 2;
        swap_chain_description.BufferDesc.Width = 0;
        swap_chain_description.BufferDesc.Height = 0;
        swap_chain_description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_description.BufferDesc.RefreshRate.Numerator = 60;
        swap_chain_description.BufferDesc.RefreshRate.Denominator = 1;
        swap_chain_description.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        swap_chain_description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_description.OutputWindow = globals.window_handle;
        swap_chain_description.SampleDesc.Count = 1;
        swap_chain_description.SampleDesc.Quality = 0;
        swap_chain_description.Windowed = 1;
        swap_chain_description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL d3d_feature_lvl;
        const D3D_FEATURE_LEVEL d3d_feature_array[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

        if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, d3d_feature_array, 2,
            D3D11_SDK_VERSION, &swap_chain_description, &d3d_swap_chain, &d3d_device, &d3d_feature_lvl, &d3d_device_ctx)))
            return false;

        ID3D11Texture2D* pBackBuffer;
        d3d_swap_chain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        d3d_device->CreateRenderTargetView(pBackBuffer, NULL, &d3d_render_target);
        pBackBuffer->Release();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        io.IniFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        io.MouseDrawCursor = false;
        io.WantCaptureMouse = false;
        io.WantCaptureKeyboard = false;
        io.WantTextInput = false;
        io.WantSetMousePos = false;


        ImGuiStyle& style = ImGui::GetStyle();
        style.Colors[ImGuiCol_WindowBg] = ImColor(12, 12, 12);
        style.Colors[ImGuiCol_ChildBg] = ImColor(9, 9, 9);
        style.Colors[ImGuiCol_Border] = ImColor(12, 12, 12, 0);

        style.Colors[ImGuiCol_Text] = ImColor(255, 255, 255, 255);
        style.Colors[ImGuiCol_TextDisabled] = ImColor(255, 255, 255, 128);

        style.ChildRounding = 5.f;
        style.ItemSpacing = ImVec2(0, 0);
        style.WindowPadding = ImVec2(0, 0);
        style.WindowRounding = 5.f;

        ImGui_ImplWin32_Init(globals.window_handle);
        ImGui_ImplDX11_Init(d3d_device, d3d_device_ctx);

        return true;
    }

    auto hijack() -> bool {
        HWND hWnd = GetConsoleWindow();
        SPOOF;

        globals.window_handle = nullptr;

        EnsureOSKRunning();

        int attempts = 0;
        while (!globals.window_handle && attempts < 100) {
            globals.window_handle = FindWindowA(OBF("OSKMainClass"), nullptr);
            if (!globals.window_handle) {
                globals.window_handle = FindWindowW(OBF(L"OSKMainClass"), nullptr);
            }
            if (!globals.window_handle) {
                globals.window_handle = FindWindowA(OBF("OSK"), nullptr);
            }
            if (!globals.window_handle) {
                globals.window_handle = FindWindowA(OBF("On-Screen Keyboard"), nullptr);
            }
            if (!globals.window_handle) {
                globals.window_handle = FindWindowExA(NULL, NULL, OBF("OSKMainClass"), NULL);
            }
            if (!globals.window_handle && attempts % 20 == 19) {
                EnsureOSKRunning();
            }
            if (!globals.window_handle) {
                Sleep(100);
                attempts++;
            }
        }
        if (!globals.window_handle) {
            MessageBoxA(nullptr, OBF("Overlay cant start..."), OBF("ERROR"), MB_OK | MB_ICONERROR);
            return false;
        }
        if (hWnd) {
            ShowWindow(hWnd, SW_SHOW);
        }
        RECT desktop_rect;
        GetWindowRect(GetDesktopWindow(), &desktop_rect);
        globals.ScreenWidth = desktop_rect.right - desktop_rect.left;
        globals.ScreenHeight = desktop_rect.bottom - desktop_rect.top;

        SetWindowLong(globals.window_handle, GWL_STYLE, WS_POPUP);

        LONG exStyle = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
        SetWindowLong(globals.window_handle, GWL_EXSTYLE, exStyle);

        SetWindowPos(globals.window_handle, HWND_TOPMOST,
            0, 0, globals.ScreenWidth, globals.ScreenHeight,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOACTIVATE);

        SPOOF_CALL(SetLayeredWindowAttributes)(globals.window_handle, RGB(0, 0, 0), 240, LWA_ALPHA);

        MARGINS margin = { -1, -1, -1, -1 };
        SPOOF_CALL(DwmExtendFrameIntoClientArea)(globals.window_handle, &margin);

        DisableOSKContent();

        SPOOF_CALL(ShowWindow)(globals.window_handle, SW_SHOW);
        SPOOF_CALL(UpdateWindow)(globals.window_handle);

        return true;
    }

    void menu_loop() {
        if (g_should_exit || (g_exit_event && WaitForSingleObject(g_exit_event, 0) == WAIT_OBJECT_0)) {
            PerformCleanup();
            return;
        }

        if (GetAsyncKeyState(VK_INSERT) & 1)
            globals.show_menu = !globals.show_menu;

        if (globals.show_menu) {
            LONG exStyle = GetWindowLong(globals.window_handle, GWL_EXSTYLE);
            exStyle &= ~WS_EX_TRANSPARENT;
            SetWindowLong(globals.window_handle, GWL_EXSTYLE, exStyle);

            SetLayeredWindowAttributes(globals.window_handle, RGB(0, 0, 0), 255, LWA_ALPHA);

            draw_menu();
        }
        else {
            LONG exStyle = GetWindowLong(globals.window_handle, GWL_EXSTYLE);
            exStyle |= WS_EX_TRANSPARENT;
            SetWindowLong(globals.window_handle, GWL_EXSTYLE, exStyle);
            SetLayeredWindowAttributes(globals.window_handle, RGB(0, 0, 0), 240, LWA_ALPHA);
            DisableOSKContent();
        }
    }
    void draw()
    {
        if (g_should_exit) return;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        auto draw_list = ImGui::GetBackgroundDrawList();

        ActorSystem::ActorLoop(draw_list);
        ESP::UpdateObjects();
        ESP::DrawObjects(draw_list);
        menu_loop();

        ImGui::Render();

        D3D11_RASTERIZER_DESC rasterizer_desc = {};
        rasterizer_desc.MultisampleEnable = FALSE;
        rasterizer_desc.AntialiasedLineEnable = FALSE;
        ID3D11RasterizerState* rasterizer_state = nullptr;
        d3d_device->CreateRasterizerState(&rasterizer_desc, &rasterizer_state);
        d3d_device_ctx->RSSetState(rasterizer_state);

        const float clear_color_with_alpha[4] = {
            0.0f, 0.0f, 0.0f, 0.0f
        };

        d3d_device_ctx->OMSetRenderTargets(1, &d3d_render_target, nullptr);
        d3d_device_ctx->ClearRenderTargetView(d3d_render_target, clear_color_with_alpha);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if (!globals.vsync) {
            d3d_swap_chain->Present(0, 0);
        }
        else {
            d3d_swap_chain->Present(1, 0);
        }

        if (rasterizer_state) {
            rasterizer_state->Release();
        }
    }

    bool render()
    {
        MSG msg = { NULL };
        ZeroMemory(&msg, sizeof(MSG));

        while (msg.message != WM_QUIT && !g_should_exit && (!g_exit_event || WaitForSingleObject(g_exit_event, 0) != WAIT_OBJECT_0))
        {
            UpdateWindow(globals.window_handle);
            ShowWindow(globals.window_handle, SW_SHOW);

            if (PeekMessageA(&msg, globals.window_handle, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            ImGuiIO& io = ImGui::GetIO();
            io.DeltaTime = 1.0f / 60.0f;

            if (globals.show_menu) {
                POINT p_cursor;
                GetCursorPos(&p_cursor);
                io.MousePos.x = p_cursor.x;
                io.MousePos.y = p_cursor.y;

                io.WantCaptureMouse = true;
                io.WantCaptureKeyboard = true;

                if (GetAsyncKeyState(VK_LBUTTON)) {
                    io.MouseDown[0] = true;
                    io.MouseClicked[0] = true;
                    io.MouseClickedPos[0].x = io.MousePos.x;
                    io.MouseClickedPos[0].y = io.MousePos.y;
                }
                else {
                    io.MouseDown[0] = false;
                }
            }
            else {
                io.WantCaptureMouse = false;
                io.WantCaptureKeyboard = false;
                io.WantTextInput = false;
                io.WantSetMousePos = false;
                io.MouseDown[0] = false;
                io.MouseDown[1] = false;
                io.MouseDown[2] = false;
                io.MouseDown[3] = false;
                io.MouseDown[4] = false;
                io.MouseClicked[0] = false;
                io.MouseClicked[1] = false;
                io.MouseClicked[2] = false;
                io.MouseClicked[3] = false;
                io.MouseClicked[4] = false;
                io.MousePos.x = -FLT_MAX;
                io.MousePos.y = -FLT_MAX;
                io.MouseDelta.x = 0.0f;
                io.MouseDelta.y = 0.0f;
            }
            draw();
        }
        PerformCleanup();
        return true;
    }

    void start()
    {
        g_exit_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        g_cleanup_mutex = CreateMutex(nullptr, FALSE, nullptr);

        SetConsoleCtrlHandler(ConsoleHandler, TRUE);

        g_main_process_id = GetCurrentProcessId();
        g_target_process_id = Kernel->g_process_id;

        g_process_monitor_thread = std::thread(ProcessMonitor);
        g_main_monitor_thread = std::thread(MainProcessMonitor);

        if (!hijack()) {
            g_should_exit = true;
            PerformCleanup();
            return;
        }

        if (!InitImgui()) {
            g_should_exit = true;
            PerformCleanup();
            return;
        }

        std::thread([&]() {
            while (!g_should_exit && (!g_exit_event || WaitForSingleObject(g_exit_event, 0) != WAIT_OBJECT_0)) {
                //  camera_postion = game_helper.get_camera();
                Sleep(16);
            }
            }).detach();

        render();

        if (g_exit_event) {
            CloseHandle(g_exit_event);
        }
        if (g_cleanup_mutex) {
            CloseHandle(g_cleanup_mutex);
        }
    }
}