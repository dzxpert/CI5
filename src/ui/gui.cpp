#include "gui.h"
#include "../features/feature_manager.h"
#include "../game/game_manager.h"
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

namespace UI {

GUI& GUI::Get() {
    static GUI instance;
    return instance;
}

void GUI::ApplyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Window & Padding styling
    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 5.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding      = 5.0f;
    style.TabRounding       = 6.0f;

    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;

    style.WindowPadding     = ImVec2(12.0f, 12.0f);
    style.FramePadding      = ImVec2(8.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 8.0f);
    style.ItemInnerSpacing  = ImVec2(6.0f, 6.0f);

    // Deep space palette (slate black, glowing cyan, warm gold)
    colors[ImGuiCol_Text]                  = ImVec4(0.92f, 0.95f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.55f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.08f, 0.10f, 0.14f, 0.96f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.11f, 0.14f, 0.19f, 0.70f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.09f, 0.11f, 0.16f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.18f, 0.28f, 0.40f, 0.80f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.14f, 0.18f, 0.25f, 0.70f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.20f, 0.28f, 0.38f, 0.85f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.24f, 0.34f, 0.46f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.07f, 0.09f, 0.13f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.10f, 0.15f, 0.24f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.07f, 0.09f, 0.13f, 0.75f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.10f, 0.12f, 0.17f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.05f, 0.06f, 0.08f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.20f, 0.28f, 0.40f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.28f, 0.38f, 0.52f, 0.90f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.35f, 0.48f, 0.65f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.00f, 0.95f, 0.85f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.00f, 0.85f, 0.95f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.10f, 1.00f, 0.90f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.15f, 0.24f, 0.35f, 0.85f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.22f, 0.35f, 0.50f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.18f, 0.42f, 0.65f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.16f, 0.25f, 0.38f, 0.75f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.22f, 0.35f, 0.52f, 0.90f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.26f, 0.42f, 0.62f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.20f, 0.30f, 0.45f, 0.60f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.30f, 0.45f, 0.65f, 0.80f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.00f, 0.90f, 1.00f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.20f, 0.28f, 0.40f, 0.50f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.00f, 0.85f, 0.95f, 0.80f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.00f, 1.00f, 0.90f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.12f, 0.16f, 0.24f, 0.85f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.22f, 0.32f, 0.48f, 1.00f);
    colors[ImGuiCol_TabSelected]           = ImVec4(0.18f, 0.28f, 0.42f, 1.00f);
    colors[ImGuiCol_TabDimmed]             = ImVec4(0.09f, 0.12f, 0.18f, 0.80f);
    colors[ImGuiCol_TabDimmedSelected]     = ImVec4(0.14f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.00f, 0.85f, 0.95f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.40f, 0.30f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.00f, 0.85f, 0.95f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
}

void GUI::Initialize(HWND hwnd, IDirect3DDevice9* pDevice) {
    if (m_initialized) return;

    m_hwnd = hwnd;
    m_pDevice = pDevice;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // Don't litter user directory with imgui.ini

    ApplyTheme();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(pDevice);

    Features::FeatureManager::Get().Initialize();

    m_initialized = true;
}

void GUI::Shutdown() {
    if (!m_initialized) return;

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_initialized = false;
}

void GUI::OnDeviceLost() {
    if (m_initialized) {
        ImGui_ImplDX9_InvalidateDeviceObjects();
    }
}

void GUI::OnDeviceReset() {
    if (m_initialized) {
        ImGui_ImplDX9_CreateDeviceObjects();
    }
}

void GUI::SetOpen(bool open) {
    if (m_open != open) {
        m_open = open;
        if (m_open) {
            ClipCursor(nullptr);
        }
    }
}

void GUI::ToggleOpen() {
    SetOpen(!m_open);
}

void GUI::Render() {
    if (!m_initialized) return;

    // Process background logic and queued main thread commands
    Features::FeatureManager::Get().OnTick();

    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = m_open;

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Small persistent HUD indicator when menu is closed
    if (!m_open) {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.6f);
        if (ImGui::Begin("CI5_Watermark", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav)) {
            
            bool inGame = Game::GameManager::Get().IsInGame();
            ImGui::TextColored(inGame ? ImVec4(0.2f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                "CI5 Mod [Press INSERT]");
            
            if (ImGui::IsItemClicked()) {
                m_open = true;
            }
        }
        ImGui::End();
    }

    // Main Mod Window
    if (m_open) {
        ImGui::SetNextWindowSize(ImVec2(560, 480), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Chicken Invaders 5 - Galactic Commander v1.0", &m_open)) {
            // Header bar
            const auto& diag = Game::GameManager::Get().GetDiagnostics();
            bool inGame = Game::GameManager::Get().IsInGame();
            bool shipActive = (Game::GameManager::Get().GetLocalPlayerObject() != nullptr);

            ImGui::TextColored(ImVec4(0.00f, 0.90f, 1.00f, 1.00f), "CI5 INTERNAL MOD");
            ImGui::SameLine();
            ImGui::TextDisabled("| Status:");
            ImGui::SameLine();
            if (shipActive) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "IN MISSION (Ship Active)");
            } else if (inGame) {
                ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "IN MISSION (Player Linked)");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), diag.statusMessage ? diag.statusMessage : "WAITING FOR WAVE");
            }
            ImGui::SameLine();
            ImGui::TextDisabled("| Toggle: [INSERT]");

            ImGui::Separator();

            // Render modular features
            Features::FeatureManager::Get().RenderUI();
        }
        ImGui::End();
    }

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

} // namespace UI
