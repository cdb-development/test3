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
//#include <d3dx11tex.h>
#include <functional>
#include <animations.hpp>
#include "managers/FontManager.hpp"
#include "managers/ImageManager.hpp"
#include "compbuilder/CompBuilder.hpp"
#include "managers/WidgetsManager.hpp"
#include "managers/StyleManager.hpp"
#include "fonts.h"
#include "unicodes.hpp"
#include "managers/TabsManager.hpp"
#include "managers/ChildManager.hpp"
#include "managers/PopupManager.hpp"
#include "managers/LangManager.hpp"

#include <gui.hpp>

#pragma comment(lib, "freetype64.lib")

using namespace ImGui;

#include "managers/NotifyManager.hpp"
#include <imstb_truetype.h>
#include "../settings.h"

ImFont* main_font = nullptr;
ImFont* lithium_font = nullptr;

void draw_menu() {
	static bool initialized = false;
	static ImVec2 size = ImVec2{ 750, 500 };

	if (!initialized) {
		GImGui->IO.IniFilename = "";

		StyleManager::get().Styles();
		StyleManager::get().Colors();
		fonts.resize(fonts_size);
		OutputDebugStringA("Loading fonts...");

		if (main_font && main_font->IsLoaded()) {
			fonts.at(font).fonts.clear();
			fonts.at(font).fonts.push_back(main_font);
			fonts.at(font).fonts.push_back(main_font);
		}
		else {
			fonts.at(font).setup(b_font, sizeof(b_font),
				{ 14, 12 },
				GetIO().Fonts->GetGlyphRangesCyrillic());
		}
		const static ImWchar icons_ranges[] = { 0x3E80, 0x4311, 0 };
		fonts.at(icons).setup(glyphter, sizeof(glyphter),
			{ 14, 12 },
			icons_ranges);

		ImGuiIO& io = ImGui::GetIO();
		lithium_font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Poppins-Bold.ttf", 22.0f);

		if (!lithium_font) {
			lithium_font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 22.0f);
		}
		if (!lithium_font) {
			ImFontConfig config;
			config.SizePixels = 22.0f;
			lithium_font = io.Fonts->AddFontDefault(&config);
		}

		ImGui::GetIO().Fonts->Build();
		ImGui_ImplDX11_InvalidateDeviceObjects();
		ImGui_ImplDX11_CreateDeviceObjects();

		OutputDebugStringA("Font atlas built!");

		if (fonts[icons].fonts.size() > 0 && fonts[icons].fonts[0]) {
			auto icon_font = fonts[icons].fonts[0];
			ImFontGlyph* testGlyph = icon_font->FindGlyph(0x3EA8);
			if (testGlyph && testGlyph->Codepoint != 0) {
			}
			else {
			}
		}

		if (main_font && main_font->IsLoaded()) {
			ImGui::GetIO().FontDefault = main_font;
		}
		else if (fonts[font].fonts.size() > 0) {
			ImGui::GetIO().FontDefault = fonts[font].fonts[0];
		}

		LangManager::get().initialize();

		ChildManager& cm = ChildManager::get();
		auto& widgets = WidgetsManager::get();

		static bool bools[100];
		static int ints[100];
		static float floats[100];
		static bool multicombo_values[5];
		static float col[100][4];
		static char buf[16];

		TabsManager::get().add_page(0, [&]() {
			BeginGroup();
			{
				cm.beginchild("General");
				{
					widgets.Checkbox("Enable Aim Assist", &bools[0], &ints[20]);
					widgets.Checkbox("Draw FOV", &bools[1], 0, col[0]);
					widgets.SliderInt("Field of view", &ints[0], 0, 180, "%d°");
					widgets.SliderFloat("Smoothness", &floats[0], 1, 10);
					widgets.MultiCombo("HitBox", multicombo_values, { "Head", "Neck", "Body", "Legs", "Arms" });
				}
				cm.endchild();
			}
			EndGroup();

			SameLine();

			BeginGroup();
			{


				cm.beginchild("Settings");
				{
					widgets.Checkbox("Aim lock", &bools[11]);
					widgets.Checkbox("Predict", &bools[12]);
					widgets.Checkbox("Draw target", &bools[13]);
					widgets.Checkbox("Save target", &bools[14]);
					widgets.Checkbox("Triggerbot", &bools[15]);
					widgets.SliderFloat("Delay", &floats[22], 0, 3, "%.1fs");
					widgets.Checkbox("Draw crosshair", &bools[16], 0, col[40], [&]() {
				    widgets.Checkbox("Rainbow", &bools[30]);
					widgets.SliderInt("Thickness", &ints[30], 1, 4, "%dpx");
					widgets.SliderInt("Space", &ints[31], 0, 5, "%dpx");
					});
					widgets.Checkbox("Hide shots", &bools[17]);
				}
				cm.endchild();
			}
			EndGroup();
			});

		TabsManager::get().add_page(1, [&]() {
			BeginGroup();
			{
				cm.beginchild("Visuals");
				{
					widgets.Checkbox("Enable Survivor ESP", &globals.show_survivors);
					widgets.Checkbox("Enable Killer ESP", &globals.show_killers);
					widgets.Checkbox("Enable Username", &globals.show_names);
					widgets.Checkbox("Enable Distance", &globals.show_distance);
					widgets.SliderFloat("Render Distance", &globals.esp_distance, 10, 80000, "%.0fm");
					widgets.Combo("Box Type", &globals.esp_box_type, { "Normal", "Filled", "Corner" });
				}
				cm.endchild();
			}
			EndGroup();

			SameLine();

			BeginGroup();
			{
				cm.beginchild("Extra");
				{
					widgets.Checkbox("Enable Generator", &globals.show_generators);
					widgets.Checkbox("Enable Totem", &globals.show_totems);
					widgets.Checkbox("Enable Window", &globals.show_windows);
					widgets.Checkbox("Enable Pallets", &globals.show_pallets);
					widgets.Checkbox("Enable Hatch", &globals.show_hatch);
					widgets.Checkbox("Enable Exit Gate", &globals.show_exit_gates);
					widgets.Checkbox("Enable Chest", &globals.show_chests);
					widgets.Checkbox("Enable Hook", &globals.show_hooks);
					widgets.SliderFloat("Object Distance", &globals.object_esp_distance, 10, 80000, "%.0fm");
				}
				cm.endchild();
			}
			EndGroup();
			});

		TabsManager::get().add_page(1, [&]() {
			BeginGroup();
			{
				cm.beginchild("Aura Settings");
				{
					widgets.Checkbox("Enable Survivor Aura", &globals.aura, 0, globals.aura_survivor_color);
					widgets.Checkbox("Enable Killer Aura", &globals.aurakiller, 0, globals.aura_killer_color);
					widgets.Checkbox("Enable Pallets Aura", &globals.PalletsAura, 0, globals.aura_pallets_color);
					widgets.Checkbox("Enable Windows Aura", &globals.WindowsAura, 0, globals.aura_windows_color);
					widgets.Checkbox("Enable Hooks Aura", &globals.MeatHookAura, 0, globals.aura_hooks_color);
					widgets.Checkbox("Enable Generator Aura", &globals.GeneratorAura, 0, globals.aura_generator_color);
				}
				cm.endchild();
			}
			EndGroup();

			SameLine();

			BeginGroup();
			{
				cm.beginchild("Aura Colors");
				{

				}
				cm.endchild();
			}
			EndGroup();
			});

		TabsManager::get().add_page(2, [&]() {
			BeginGroup();
			{
				cm.beginchild("Players");
				{
					widgets.Checkbox("Enable Jump", &globals.jump);
					widgets.Checkbox("FOV Changer", &globals.fov_enabled);
					widgets.Checkbox("Auto SkillCheck", &globals.skillcheckauto);
				}
				cm.endchild();
			}
			EndGroup();

			SameLine();

			BeginGroup();
			{
				cm.beginchild("Settings");
				{
					widgets.SliderFloat("FOV ", &globals.custom_fov, globals.min_fov, globals.max_fov, "%.1fx");
				}
				cm.endchild();
			}
			EndGroup();
			});

		TabsManager::get().add_page(4, [&]() {
			static int cfg = 0;

			BeginGroup();
			{
				PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
				BeginChild("configs", { GetWindowWidth() / 2 - 14 - GImGui->Style.ItemSpacing.x / 2, 0 }, ImGuiChildFlags_Border);
				{
					SetCursorPos({ 14, 14 });
					static char buf[16];
					PushItemFlag(ImGuiItemFlags_NoNav, true);
					widgets.TextField("##name", buf, sizeof(buf), { GetWindowWidth() - 28 - GetFrameHeight() - 10, GetFrameHeight() }, "Enter the name...", i_tag_01);
					PopItemFlag();
					SameLine(0, 10);
					if (CompBuilder::get().Button("add", { GetFrameHeight(), GetFrameHeight() }, [&](CompBuilder::ButtonEnv env) {
						auto col = col_anim(col_anim(GetColorU32(ImGuiCol_Button), GetColorU32(ImGuiCol_ButtonHovered), env.anim.hover), GetColorU32(ImGuiCol_ButtonActive), env.anim.held);
						GetWindowDrawList()->AddRectFilled(env.bb.Min, env.bb.Max, col, 2);

						GetWindowDrawList()->AddText(fonts[icons].get(14), 14, env.bb.GetCenter() - ImVec2{ 7, 7 }, GetColorU32(ImGuiCol_TextButton), i_plus);
						})) {

						if (strlen(buf) == 0) {
							NotifyManager::get().add("Enter the name!", NotifyManager::notify_error);
						}
						else {
							NotifyManager::get().add("Successfully created a config", NotifyManager::notify_success);
						}
					}

					Separator();
					SetCursorPosY(GetCursorPosY() - GImGui->Style.ItemSpacing.y);

					PushStyleVar(ImGuiStyleVar_WindowPadding, { 14, 14 });
					BeginChild("list", { 0, 0 }, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
					{
						cm.smoothscroll();

						for (int i = 0; i < 10; ++i) {
							char label[64];
							ImFormatString(label, sizeof(label), "Config #%d", i);

							ImRect bb{ GetCurrentWindow()->DC.CursorPos, GetCurrentWindow()->DC.CursorPos + ImVec2{ GetWindowWidth() - GImGui->Style.WindowPadding.x * 2, GetFrameHeight() } };
							if (CompBuilder::get().Selectable(label, cfg == i, bb, [&](const CompBuilder::SelectableEnv& env) {
								ImColor col = col_anim(col_anim(GetColorU32(ImGuiCol_TextDisabled), GetColorU32(ImGuiCol_TextHovered), env.anim.hover), GetColorU32(ImGuiCol_Text), env.anim.selected);
								ImColor icon_col = col_anim(col_anim(GetColorU32(ImGuiCol_TextDisabled), GetColorU32(ImGuiCol_TextHovered), env.anim.hover), GetColorU32(ImGuiCol_Scheme), env.anim.selected);

								GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg, env.anim.selected), GImGui->Style.FrameRounding);

								GetWindowDrawList()->AddText(fonts[icons].get(14), 14, bb.Min + GImGui->Style.FramePadding, icon_col, i_file_04);
								GetWindowDrawList()->AddText(bb.Min + GImGui->Style.FramePadding + ImVec2{ 24, 0 }, col, label);
								})) {
								cfg = i;
							}
						}
					}
					EndChild();
					PopStyleVar();
				}
				EndChild();
				PopStyleVar();
			}
			EndGroup();

			SameLine();

			BeginGroup();
			{
				cm.beginchild("CONFIG");
				{
					char label[64];
					ImFormatString(label, sizeof(label), "Config #%d", cfg);

					TextDisabled(LangManager::get().translate("Name:"));
					SameLine(GetWindowWidth() - CalcTextSize(label).x - GImGui->Style.WindowPadding.x);
					Text(label);

					TextDisabled(LangManager::get().translate("Created:"));
					SameLine(GetWindowWidth() - CalcTextSize("22.03.2025").x - GImGui->Style.WindowPadding.x);
					Text("22.03.2025");

					TextDisabled(LangManager::get().translate("Created by:"));
					SameLine(GetWindowWidth() - CalcTextSize("Michael Conors").x - GImGui->Style.WindowPadding.x);
					Text("Michael Conors");

					widgets.Separator();

					if (widgets.Button("Load")) {
						NotifyManager::get().add("Loaded the config", NotifyManager::notify_info);
					}

					ImColor red;
					float h, s, v;
					ColorConvertRGBtoHSV(GetStyleColorVec4(ImGuiCol_Scheme).x, GetStyleColorVec4(ImGuiCol_Scheme).y, GetStyleColorVec4(ImGuiCol_Scheme).z, h, s, v);
					ColorConvertHSVtoRGB(0.f, s, v, red.Value.x, red.Value.y, red.Value.z);
					red.Value.w = GImGui->Style.Alpha;

					PushStyleColor(ImGuiCol_Button, red.Value);
					PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ red.Value.x / 1.15f, red.Value.y / 1.15f, red.Value.z / 1.15f, GImGui->Style.Alpha });
					PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ red.Value.x / 1.35f, red.Value.y / 1.35f, red.Value.z / 1.35f, GImGui->Style.Alpha });
					if (widgets.Button("Delete")) {
						NotifyManager::get().add("Removed the config", NotifyManager::notify_info);
					}
					PopStyleColor(3);
				}
				cm.endchild();
			}
			EndGroup();
			});

		initialized = true;
		OutputDebugStringA("GUI initialized with proper fonts!");
	}

	static bool bools[100];
	static int ints[100];
	static bool multicombo_values[4];
	static float col[100][4];
	static char buf[16];

	SetNextWindowSize(size);
	Begin("GUI", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
	{
		BeginChild("navbar", { 180, 0 }, 0, ImGuiWindowFlags_NoBackground);
		{
			SetCursorPos({ 14, 14 });

			if (lithium_font) {
				PushFont(lithium_font);
				Text("LITHIUM.RIP");
				PopFont();
			}
			else {
				SetWindowFontScale(1.8f);
				Text("LITHIUM.RIP");
				SetWindowFontScale(1.0f);
			}

			WidgetsManager::get().Separator();
			SetCursorPosX(14);
			TabsManager::get().render_tabs(8);

			GetWindowDrawList()->AddRectFilled(GetWindowPos() + ImVec2{ GetWindowWidth() - 1, 0 }, GetWindowPos() + GetWindowSize(), GetColorU32(ImGuiCol_Border));

			SetCursorPos({ 14, GetWindowHeight() - 38 });
			CompBuilder::get().OpenButton("lang selector", { 32 + fonts[font].get(12)->CalcTextSizeA(12, FLT_MAX, -1, LangManager::get().get_lang_name()).x, 24 }, [&](CompBuilder::OpenButtonEnv env) {
				GetWindowDrawList()->AddRectFilled(env.bb.Min, env.bb.Max, GetColorU32(ImGuiCol_ChildBg), GImGui->Style.FrameRounding);
				GetWindowDrawList()->AddText(fonts[icons].get(12), 12, { env.bb.Min.x + 6, env.bb.GetCenter().y - 6 }, GetColorU32(ImGuiCol_Scheme), i_translate_02);
				GetWindowDrawList()->AddText(fonts[font].get(12), 12, { env.bb.Min.x + 26, env.bb.GetCenter().y - 6 }, GetColorU32(ImGuiCol_Text), LangManager::get().get_lang_name());

				if (env.pressed) {
					env.open = !env.open;
				}

				if (env.hovered) env.open = true;

				if (env.anim.open > 0.05f) {
					PushStyleVar(ImGuiStyleVar_Alpha, env.anim.open);
					PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
					PushStyleVar(ImGuiStyleVar_WindowRounding, GImGui->Style.FrameRounding);
					PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 8 });
					PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 8 });
					PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
					PushStyleColor(ImGuiCol_WindowBg, GetColorU32(ImGuiCol_FrameBg));
					Begin("lang selector", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
					{
						SetWindowPos({ env.bb.Min.x, env.bb.GetCenter().y - GetWindowHeight() / 2 });
						SetWindowSize({ 100, (GetCurrentWindow()->ContentSize.y + GImGui->Style.WindowPadding.y * 2) * env.anim.open });

						BringWindowToDisplayFront(GetCurrentWindow());
						BringWindowToFocusFront(GetCurrentWindow());

						if (!IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && !env.hovered) {
							env.open = false;
						}

						PushStyleColor(ImGuiCol_FrameBg, GetColorU32(ImGuiCol_FrameBgHovered));

						for (int i = 0; i < LangManager::get().get_langs().size(); ++i) {
							if (WidgetsManager::get().Selectable(LangManager::get().get_langs()[i].label, LangManager::get().get_lang() == i)) {
								LangManager::get().set_lang(i);
							}
						}

						PopStyleColor();
					}
					End();
					PopStyleColor();
					PopStyleVar(6);
				}
				});
		}
		EndChild();

		SameLine(0, 0);

		PushStyleVar(ImGuiStyleVar_WindowPadding, { 14, 14 });
		BeginChild("main", { 0, 0 }, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		{
			PopStyleVar();

			ChildManager::get().smoothscroll();

			TabsManager::get().render_subtabs(20);

			auto window = GetCurrentWindow();

			PushStyleVar(ImGuiStyleVar_Alpha, GImGui->Style.Alpha * TabsManager::get().get_anim() * GImGui->Style.Alpha);
			PushStyleVar(ImGuiStyleVar_WindowPadding, { 14, 14 });
			BeginChild("content", { 0, 0 }, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			{
				TabsManager::get().draw_page(window);
			}
			EndChild();
			PopStyleVar(2);
		}
		EndChild();

		PopupManager::get().handle();
	}
	End();

	NotifyManager::get().draw();
}