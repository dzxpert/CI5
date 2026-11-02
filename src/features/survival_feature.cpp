#include "survival_feature.h"
#include "../game/game_manager.h"
#include "../game/game_structures.h"
#include <imgui.h>

namespace Features {

void SurvivalFeature::OnTick() {
    Game::GameManager::Get().ConfigureSurvival(m_godMode, m_neverOverheat,
        m_infiniteLives, m_targetLives, m_infiniteMissiles, m_targetMissiles);
}

void SurvivalFeature::OnRenderUI() {
    auto& gm = Game::GameManager::Get();
    bool inGame = gm.IsInGame();

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Survival & Invulnerability Controls");
    ImGui::Spacing();

    // God Mode Toggle
    ImGui::Checkbox("God Mode (Refresh Native Spawn Shield)", &m_godMode);
    ImGui::SameLine();
    ImGui::TextDisabled("(Shield expires within 2 seconds after disabling)");

    // Never Overheat Toggle
    ImGui::Checkbox("Never Overheat (Infinite Continuous Firing)", &m_neverOverheat);
    ImGui::SameLine();
    ImGui::TextDisabled("(Heat locked at 0%)");

    // Infinite Lives Toggle
    ImGui::Checkbox("Infinite Lives (Auto-Replenish)", &m_infiniteLives);
    if (m_infiniteLives) {
        ImGui::SameLine();
        ImGui::PushItemWidth(80.0f);
        ImGui::SliderInt("Target Lives##LivesTarget", &m_targetLives, 1, 999);
        ImGui::PopItemWidth();
    }

    // Infinite Missiles Toggle
    ImGui::Checkbox("Infinite Missiles (Auto-Replenish)", &m_infiniteMissiles);
    if (m_infiniteMissiles) {
        ImGui::SameLine();
        ImGui::PushItemWidth(80.0f);
        ImGui::SliderInt("Target Missiles##MissilesTarget", &m_targetMissiles, 1, 999);
        ImGui::PopItemWidth();
    }

    ImGui::Separator();
    ImGui::Text("Quick Emergency Cheats:");

    if (ImGui::Button("Heal & Restore All (99 Lives + 99 Missiles + Cool)", ImVec2(-1, 30))) {
        gm.SetLives(99);
        gm.SetMissiles(99);
        gm.ResetHeat();
        gm.SetInvulnerability(true);
    }
}

} // namespace Features
