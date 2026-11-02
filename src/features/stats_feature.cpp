#include "stats_feature.h"
#include "../game/game_manager.h"
#include "../game/game_structures.h"
#include <imgui.h>

namespace Features {

void StatsFeature::OnTick() {
}

void StatsFeature::OnRenderUI() {
    auto& gm = Game::GameManager::Get();
    Game::PlayerInfo* pInfo = gm.GetLocalPlayerInfo();
    Game::PlayerObject* pObj = gm.GetLocalPlayerObject();
    const auto diag = gm.GetDiagnostics();
    ImGui::TextWrapped("Last change: %s", gm.GetActionStatus());

    if (!pInfo) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "[!] Not In-Mission");
        ImGui::TextDisabled("Status: %s", diag.statusMessage);
        ImGui::Spacing();

        // Allow setting keys even while in menus / store
        int menuKeys = gm.GetProfileKeys();
        if (menuKeys >= 0) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Player Keys (Profile / Store Bank): %d", menuKeys);
            ImGui::SameLine();
            if (ImGui::SmallButton("+10##MenuKeys")) { gm.AddKeys(10); }
            ImGui::SameLine();
            if (ImGui::SmallButton("+100##MenuKeys")) { gm.AddKeys(100); }
            ImGui::SameLine();
            if (ImGui::SmallButton("+1000##MenuKeys")) { gm.AddKeys(1000); }

            ImGui::PushItemWidth(120.0f);
            ImGui::InputInt("##CustomKeysMenu", &m_customKeys);
            ImGui::SameLine();
            if (ImGui::Button("Set Keys##MenuBtn")) {
                gm.SetKeys(m_customKeys);
            }
            ImGui::PopItemWidth();
            ImGui::Spacing();
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "Pointer Chain Diagnostics:");
        ImGui::Text("CI5.exe Base:    0x%08X", diag.moduleBase);
        ImGui::Text("Static Base:     0x%08X (CI5.exe + 0x5814F4)", diag.staticBaseAddr);
        ImGui::Text("Engine Context:  0x%08X", diag.baseCtx);
        ImGui::Text("Network Context:   0x%08X", diag.gameUniverse);
        ImGui::Text("Role (THEROLE):  0x%08X (Player ID: %d)", diag.role, diag.localPlayerId);
        ImGui::Text("MyRootObject:    0x%08X", diag.rootObj);
        ImGui::Text("PlayerInfo:      0x%08X", diag.playerInfo);
        ImGui::Spacing();
        ImGui::TextDisabled("Start or continue a wave to link to the player spaceship.");
        return;
    }

    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[*] Player Profile Linked (Player ID: %d)", pInfo->GetPlayerId());
    ImGui::SameLine();
    ImGui::TextDisabled("| %s", diag.statusMessage ? diag.statusMessage : "Active");
    
    // Display callsign if available
    const wchar_t* pName = pInfo->GetPlayerName();
    if (pName && pName[0] != L'\0') {
        char nameUtf8[64] = {0};
        WideCharToMultiByte(CP_UTF8, 0, pName, -1, nameUtf8, sizeof(nameUtf8), nullptr, nullptr);
        ImGui::Text("Callsign: %s", nameUtf8);
    }

    ImGui::Separator();

    // Stats Grid
    if (ImGui::BeginTable("StatsGrid", 2, ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Remaining Lives: %d", pInfo->GetLivesRemaining());
        ImGui::SameLine();
        if (ImGui::SmallButton("+1 Life")) {
            gm.AddLives(1);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("+5 Lives")) {
            gm.AddLives(5);
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("Missiles: %d", pInfo->GetMissilesAvailable());
        ImGui::SameLine();
        if (ImGui::SmallButton("+1 Missile")) {
            gm.AddMissiles(1);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("+5 Missiles")) {
            gm.AddMissiles(5);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Current Score: %d", pInfo->GetScore());
        ImGui::SameLine();
        if (ImGui::SmallButton("+10k")) {
            gm.AddScore(10000);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("+100k")) {
            gm.AddScore(100000);
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("Food Collected: %d", pInfo->GetFoodCollected());
        ImGui::SameLine();
        if (ImGui::SmallButton("+50 Food")) {
            gm.AddFood(50);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        int profileKeys = gm.GetProfileKeys();
        int missionKeys = gm.GetMissionKeys();
        if (missionKeys >= 0) {
            ImGui::Text("Keys: %d (Wave: %d)", profileKeys >= 0 ? profileKeys : 0, missionKeys);
        } else {
            ImGui::Text("Keys: %d", profileKeys >= 0 ? profileKeys : 0);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("+10 Keys")) {
            gm.AddKeys(10);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("+100 Keys")) {
            gm.AddKeys(100);
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("Next Extra Life Score: %d", pInfo->GetNextExtraLifeScore());

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        if (pObj) {
            ImGui::Text("Ship State: %d", pObj->GetLifeState());
        } else {
            ImGui::TextDisabled("Ship: Spawning...");
        }

        ImGui::TableSetColumnIndex(1);
        // empty cell for alignment

        ImGui::EndTable();
    }

    ImGui::Separator();

    // Heat Gauge & Spaceship Position (if spaceship entity is currently alive in world)
    if (pObj) {
        float heatRatio = pObj->GetHeatPercentage();
        ImVec4 heatColor = ImVec4(0.2f, 0.8f, 1.0f, 1.0f);
        if (heatRatio > 75.0f) heatColor = ImVec4(1.0f, 0.3f, 0.2f, 1.0f);
        else if (heatRatio > 50.0f) heatColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);

        ImGui::Text("Weapon Heat: %.1f%%", heatRatio);
        ImGui::SameLine();
        if (ImGui::Button("Cool Down to 0%")) {
            gm.ResetHeat();
        }
        ImGui::ProgressBar(heatRatio / 100.0f, ImVec2(-1.0f, 16.0f));

        ImGui::Separator();
        ImGui::Text("Ship Coordinates: X: %.3f | Y: %.3f | Z: %.3f | Tilt: %.2f rad",
            pObj->GetPosX(), pObj->GetPosY(), pObj->GetPosZ(), pObj->GetFacingAngle());
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "[*] Spaceship entity not in flight (Waiting for wave / respawn)");
    }

    ImGui::Separator();
    ImGui::Text("Direct Value Modifiers:");

    ImGui::PushItemWidth(120.0f);
    ImGui::InputInt("##CustomLives", &m_customLives);
    ImGui::SameLine();
    if (ImGui::Button("Set Lives")) {
        gm.SetLives(m_customLives);
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    ImGui::InputInt("##CustomMissiles", &m_customMissiles);
    ImGui::SameLine();
    if (ImGui::Button("Set Missiles")) {
        gm.SetMissiles(m_customMissiles);
    }

    ImGui::InputInt("##CustomScore", &m_customScore);
    ImGui::SameLine();
    if (ImGui::Button("Set Score")) {
        gm.SetScore(m_customScore);
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    ImGui::InputInt("##CustomFood", &m_customFood);
    ImGui::SameLine();
    if (ImGui::Button("Set Food")) {
        gm.SetFood(m_customFood);
    }

    ImGui::InputInt("##CustomKeysInMission", &m_customKeys);
    ImGui::SameLine();
    if (ImGui::Button("Set Keys")) {
        gm.SetKeys(m_customKeys);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("+1000 Keys")) {
        gm.AddKeys(1000);
    }

    ImGui::PopItemWidth();
}

} // namespace Features
