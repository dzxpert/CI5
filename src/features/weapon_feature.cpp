#include "weapon_feature.h"
#include "../game/game_manager.h"
#include <imgui.h>

namespace Features {

void WeaponFeature::OnTick() {
    Game::GameManager::Get().ConfigureRapidFire(m_rapidFireEnabled, m_customFireInterval);
}

void WeaponFeature::OnRenderUI() {
    auto& gm = Game::GameManager::Get();
    Game::Weapon* pWeapon = gm.GetActiveWeapon();

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Weapon Selector & Hot-Swapper");
    ImGui::Spacing();

    // Active weapon info
    if (pWeapon) {
        int activeType = pWeapon->GetWeaponType();
        int activePower = pWeapon->GetPowerLevel();
        ImGui::Text("Active Weapon: %s (Type ID: %d)", Game::GetWeaponName(activeType), activeType);
        ImGui::Text("Active Power Level: %d (0-indexed: %d)", activePower + 1, activePower);
        ImGui::Text("Fire Interval: %.3fs | Heat/Shot: %.3f",
            pWeapon->GetFireInterval(), pWeapon->GetHeatPerShot());
    } else {
        ImGui::TextDisabled("No active weapon found. (Spawn into a wave first)");
    }

    ImGui::Separator();

    // Weapon Selection UI
    ImGui::Text("Choose Weapon to Equip:");
    
    // Weapon Combo Box
    const char* currentWeaponName = Game::GetWeaponName(m_selectedWeaponId);
    if (ImGui::BeginCombo("Weapon", currentWeaponName)) {
        for (int i = 0; i < static_cast<int>(Game::EWeaponType::Count); ++i) {
            const bool isSelected = (m_selectedWeaponId == i);
            const auto& wInfo = Game::G_WEAPON_CATALOG[i];
            
            char label[128];
            snprintf(label, sizeof(label), "[%d] %s - %s", wInfo.id, wInfo.name, wInfo.description);

            if (ImGui::Selectable(label, isSelected)) {
                m_selectedWeaponId = i;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Power Level Slider & Direct Input (Supports 1 to 200+)
    int displayPower = m_selectedPowerLevel + 1;
    if (ImGui::SliderInt("Power Level", &displayPower, 1, 200, "Level %d")) {
        m_selectedPowerLevel = displayPower - 1;
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(80.0f);
    if (ImGui::InputInt("##DirectPowerInput", &displayPower)) {
        if (displayPower < 1) displayPower = 1;
        if (displayPower > 1000) displayPower = 1000;
        m_selectedPowerLevel = displayPower - 1;
    }
    ImGui::PopItemWidth();

    // Quick Power presets
    ImGui::Text("Quick Power Presets:");
    ImGui::SameLine();
    if (ImGui::SmallButton("Lvl 1")) { m_selectedPowerLevel = 0; }
    ImGui::SameLine();
    if (ImGui::SmallButton("Lvl 10")) { m_selectedPowerLevel = 9; }
    ImGui::SameLine();
    if (ImGui::SmallButton("Lvl 20")) { m_selectedPowerLevel = 19; }
    ImGui::SameLine();
    if (ImGui::SmallButton("Lvl 50")) { m_selectedPowerLevel = 49; }
    ImGui::SameLine();
    if (ImGui::SmallButton("Lvl 100")) { m_selectedPowerLevel = 99; }
    ImGui::SameLine();
    if (ImGui::SmallButton("Lvl 200")) { m_selectedPowerLevel = 199; }

    ImGui::Spacing();
    if (ImGui::Button("⚡ Equip Selected Weapon Now ⚡", ImVec2(-1, 32))) {
        gm.InstallWeapon(m_selectedWeaponId, m_selectedPowerLevel);
    }
    if (pWeapon) {
        if (ImGui::Button("⚡ Set Active Weapon to Selected Power ⚡", ImVec2(-1, 26))) {
            gm.SetWeaponPower(m_selectedPowerLevel);
        }
    }

    ImGui::Separator();
    ImGui::Text("Quick Arsenal Grid:");

    // Quick one-click buttons for popular / fun weapons
    if (ImGui::BeginTable("WeaponGrid", 4)) {
        for (int i = 0; i < static_cast<int>(Game::EWeaponType::Count); ++i) {
            ImGui::TableNextColumn();
            const auto& wInfo = Game::G_WEAPON_CATALOG[i];
            char btnLabel[64];
            snprintf(btnLabel, sizeof(btnLabel), "%s##%d", wInfo.name, i);
            if (ImGui::Button(btnLabel, ImVec2(-1, 26))) {
                m_selectedWeaponId = i;
                gm.InstallWeapon(i, m_selectedPowerLevel);
            }
        }
        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::Text("Live Weapon Tweaker:");

    ImGui::Checkbox("Rapid Fire Override", &m_rapidFireEnabled);
    if (m_rapidFireEnabled) {
        ImGui::SameLine();
        float intervalFloat = static_cast<float>(m_customFireInterval);
        ImGui::PushItemWidth(120.0f);
        if (ImGui::SliderFloat("Cooldown##FireInterval", &intervalFloat, 0.01f, 0.5f, "%.3fs")) {
            m_customFireInterval = static_cast<double>(intervalFloat);
        }
        ImGui::PopItemWidth();
    }

    ImGui::TextDisabled("Projectile speed control is unavailable for this build.");
}

} // namespace Features
