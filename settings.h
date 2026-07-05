#pragma once
#include <wtypes.h>
#include <cstdint>

class globals_t {
public:
    HWND window_handle;
    bool show_menu = true;
    bool vsync = false;
    int ScreenWidth;
    int ScreenHeight;

    bool esp_enabled = true;
    bool show_survivors = true;
    bool show_survivor_names = true;
    bool show_killers = true;
    bool show_names = true;
	bool jump = false;
	bool noclip = false;

    bool aura = false;
    bool PalletsAura = false;
    bool aurakiller = false;
	bool WindowsAura = false;
    bool MeatHookAura = false;
    bool GeneratorAura = false;

	bool speed = false;
    bool skillcheckauto = false;
    bool perfectskillauto = false;
	bool redstain_enabled = false;
	bool NoAttackCooldown = false;
    bool show_distance = true;
    bool show_health = false;
    float esp_distance = 70000.0f;
    int esp_box_type = 2;
    bool show_debug_info = false;
    float speed_multiplier = 1.1f;

    bool show_generators = false;
    bool show_totems = false;
    bool show_windows = false;
    bool show_pallets = false;
    bool show_hatch = false;
    bool show_hooks = false;
    bool show_chests = false;
    bool show_exit_gates = false;
    float object_esp_distance = 70000.0f;

    bool show_survivor_tracers = false;
    bool show_killer_tracers = false;
    int tracer_type = 1;
    float survivor_tracer_color[4] = { 0.0f, 1.0f, 0.0f, 0.8f };
    float killer_tracer_color[4] = { 1.0f, 0.0f, 0.0f, 0.8f };

    bool fov_enabled = false;
    float custom_fov = 87.0f;
    float original_fov = 87.0f;
    float min_fov = 60.0f;
    float max_fov = 150.0f;
    bool save_original_fov = true;
    uintptr_t camera_manager_ptr = 0;
    bool fov_first_read = true;


    float survivor_color[4] = { 0.0f, 1.0f, 0.0f, 1.0f }; // Green
    float killer_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };   // Red
    float generator_color[4] = { 0.0f, 0.5f, 1.0f, 1.0f }; // Blue
    float hook_color[4] = { 1.0f, 0.5f, 0.0f, 1.0f };     // Orange
    float totem_color[4] = { 0.5f, 0.0f, 1.0f, 1.0f };    // Purple
    float chest_color[4] = { 1.0f, 0.8f, 0.0f, 1.0f };    // Gold

    float aura_survivor_color[4] = { 0.0f, 1.0f, 1.0f, 1.0f };   // (0.0-1.0)
    float aura_killer_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };     // (0.0-1.0)
    float aura_pallets_color[4] = { 1.0f, 1.0f, 0.0f, 1.0f };    // Yellow
    float aura_windows_color[4] = { 0.0f, 1.0f, 1.0f, 1.0f };    // Cyan
    float aura_hooks_color[4] = { 1.0f, 0.5f, 0.0f, 1.0f };      // Orange
    float aura_generator_color[4] = { 0.0f, 0.5f, 1.0f, 1.0f };  // Blue

    bool driver_connected = false;
    bool game_found = false;
};

extern globals_t globals;