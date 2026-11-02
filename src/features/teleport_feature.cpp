#include "teleport_feature.h"
#include "../game/game_manager.h"
#include "../game/game_structures.h"
#include <imgui.h>

namespace Features {

void TeleportFeature::OnTick() {
    // If not actively teleporting, sync sliders with current coordinates
    auto& gm = Game::GameManager::Get();
    auto pObj = gm.GetLocalPlayerObject();
    if (pObj) {
        // Can be used for noclip or freeze if needed
    }
}

void TeleportFeature::OnRenderUI() {
    auto& gm = Game::GameManager::Get();
    auto pObj = gm.GetLocalPlayerObject();

    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Coordinates & Spatial Positioning");
    ImGui::Spacing();

    if (!pObj) {
        ImGui::TextDisabled("Spaceship not present in world.");
        return;
    }

    float currentX = pObj->GetPosX();
    float currentY = pObj->GetPosY();

    ImGui::Text("Current Position: X = %.3f | Y = %.3f", currentX, currentY);
    ImGui::Separator();

    ImGui::Text("Teleport Coordinates:");
    ImGui::SliderFloat("Target X", &m_targetX, -1.5f, 1.5f, "%.3f");
    ImGui::SliderFloat("Target Y", &m_targetY, -1.5f, 1.5f, "%.3f");

    if (ImGui::Button("Teleport to Coordinates", ImVec2(200, 30))) {
        gm.Teleport(m_targetX, m_targetY);
    }
    ImGui::SameLine();
    if (ImGui::Button("Copy Current Position", ImVec2(180, 30))) {
        m_targetX = currentX;
        m_targetY = currentY;
    }

    ImGui::Separator();
    ImGui::Text("Quick Position Presets:");

    if (ImGui::Button("Bottom Center (Spawn Area)", ImVec2(180, 26))) {
        gm.Teleport(0.0f, -0.7f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Screen Center", ImVec2(140, 26))) {
        gm.Teleport(0.0f, 0.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Top Edge", ImVec2(120, 26))) {
        gm.Teleport(0.0f, 0.7f);
    }

    if (ImGui::Button("Left Wing (-0.8)", ImVec2(180, 26))) {
        gm.Teleport(-0.8f, currentY);
    }
    ImGui::SameLine();
    if (ImGui::Button("Right Wing (+0.8)", ImVec2(180, 26))) {
        gm.Teleport(0.8f, currentY);
    }
}

} // namespace Features
