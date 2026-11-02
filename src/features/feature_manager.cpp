#include "feature_manager.h"
#include "stats_feature.h"
#include "survival_feature.h"
#include "weapon_feature.h"
#include "teleport_feature.h"
#include "../game/game_manager.h"
#include <imgui.h>

namespace Features {

FeatureManager& FeatureManager::Get() {
    static FeatureManager instance;
    return instance;
}

void FeatureManager::Initialize() {
    if (m_initialized) return;

    RegisterFeature(std::make_shared<StatsFeature>());
    RegisterFeature(std::make_shared<SurvivalFeature>());
    RegisterFeature(std::make_shared<WeaponFeature>());
    RegisterFeature(std::make_shared<TeleportFeature>());

    m_initialized = true;
}

void FeatureManager::RegisterFeature(std::shared_ptr<IFeature> feature) {
    if (feature) {
        m_features.push_back(feature);
    }
}

void FeatureManager::OnTick() {
    for (auto& feat : m_features) {
        feat->OnTick();
    }
}

void FeatureManager::RenderUI() {
    if (ImGui::BeginTabBar("ModTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Player & Stats")) {
            auto feat = GetFeature<StatsFeature>();
            if (feat) feat->OnRenderUI();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("God Mode & Cheats")) {
            auto feat = GetFeature<SurvivalFeature>();
            if (feat) feat->OnRenderUI();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Weapon Arsenal")) {
            auto feat = GetFeature<WeaponFeature>();
            if (feat) feat->OnRenderUI();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Movement & Presets")) {
            auto feat = GetFeature<TeleportFeature>();
            if (feat) feat->OnRenderUI();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Diagnostics & Debug")) {
            const auto& diag = Game::GameManager::Get().GetDiagnostics();
            ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "Live Engine Pointer Chain:");
            ImGui::Separator();
            ImGui::Text("CI5.exe Base:      0x%08X", diag.moduleBase);
            ImGui::Text("Static Base:       0x%08X (CI5.exe + 0x5814F4)", diag.staticBaseAddr);
            ImGui::Text("Base Context:      0x%08X", diag.baseCtx);
            ImGui::Text("Network Context:     0x%08X (BaseContext + 0x28)", diag.gameUniverse);
            ImGui::Text("Role (THEROLE):    0x%08X (Network + 0x0C during update)", diag.role);
            ImGui::Text("Local Player ID:   %d (Role + 0x98)", diag.localPlayerId);
            ImGui::Text("MyRootObject:      0x%08X (Role + 0x2FC)", diag.rootObj);
            ImGui::Text("PlayerInfo:        0x%08X (client update)", diag.playerInfo);
            ImGui::Text("PlayerObject:      0x%08X (PlayerInfo + 0xB0)", diag.playerObj);
            ImGui::Separator();
            ImGui::Text("Current State:     %s", diag.statusMessage);
            ImGui::Spacing();
            ImGui::Text("Authoritative Info: 0x%08X", diag.authoritativeInfo);
            ImGui::TextWrapped("Last change: %s", Game::GameManager::Get().GetActionStatus());
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

} // namespace Features
