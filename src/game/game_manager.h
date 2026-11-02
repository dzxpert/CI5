#pragma once
#include "game_structures.h"
#include <windows.h>
#include <atomic>
#include <functional>
#include <mutex>
#include <vector>

namespace Game {
struct DiagnosticsInfo {
    uintptr_t moduleBase = 0, staticBaseAddr = 0, baseCtx = 0;
    uintptr_t gameUniverse = 0, role = 0; // UveNetwork; +0xC is the temporarily active role.
    int localPlayerId = -1;
    uintptr_t rootObj = 0, playerInfo = 0, playerObj = 0, authoritativeInfo = 0;
    const char* statusMessage = "Waiting for player update";
};

class GameManager {
public:
    static GameManager& Get();
    void Initialize();
    void Shutdown();
    bool IsSupported() const { return m_supported; }
    void ReportHookStatus(const char* status);
    bool IsInGame();
    PlayerInfo* GetLocalPlayerInfo();
    PlayerObject* GetLocalPlayerObject();
    Weapon* GetActiveWeapon();
    DiagnosticsInfo GetDiagnostics();
    const char* GetActionStatus() const { return m_actionStatus.load(); }
    uintptr_t GetModuleBase();
    // Called by the PlayerInfo update detour while THEROLE is valid.
    void OnPlayerUpdate(PlayerInfo* info);
    bool InstallWeapon(int weaponType, int powerLevel);
    bool SetWeaponPower(int powerLevel);
    bool SetLives(int lives);
    bool AddLives(int delta);
    bool SetMissiles(int missiles);
    bool AddMissiles(int delta);
    bool SetScore(int score);
    bool AddScore(int delta);
    bool SetFood(int food);
    bool AddFood(int delta);
    int GetProfileKeys();
    bool SetProfileKeys(int keys);
    bool AddProfileKeys(int delta);
    int GetMissionKeys();
    bool SetMissionKeys(int keys);
    bool AddMissionKeys(int delta);
    bool SetKeys(int keys);
    bool AddKeys(int delta);
    uintptr_t GetWaveController();
    bool SetHeat(int heatValue);
    bool ResetHeat();
    bool SetInvulnerability(bool enabled);
    bool Teleport(float x, float y);
    void ConfigureSurvival(bool shield, bool cool, bool lives, int targetLives,
                           bool missiles, int targetMissiles);
    void ConfigureRapidFire(bool enabled, double interval);
#ifdef CI5MOD_LIVE_TEST
    bool QueueStatRoundTripTest();
#endif
private:
    struct Options {
        bool shield = false, cool = false, lives = false, missiles = false;
        int targetLives = 99, targetMissiles = 99;
        bool rapidFire = false;
        double interval = 0.05;
    };
    struct Action {
        PlayerInfo* clientIdentity; // Compared only; never dereferenced at execution.
        int playerId;
        ULONGLONG queuedAt;
        std::function<bool(PlayerInfo*)> apply;
    };
    struct MirrorWrite {
        PlayerInfo* clientIdentity;
        uintptr_t offset;
        int value;
        bool ship;
    };
    GameManager() = default;
    bool ValidatePlayerInfo(PlayerInfo* info);
    PlayerObject* PlayerShip(PlayerInfo* info);
    bool QueueAction(std::function<bool(PlayerInfo*)> action);
    bool QueueStat(uintptr_t offset, int value, bool add);
    bool WriteEncoded(void* object, uintptr_t offset, int value);
    bool WriteStat(PlayerInfo* info, uintptr_t offset, int value);
    bool CoolShip(PlayerInfo* info);
    bool GiveShield(PlayerInfo* info);
    void ApplyOptions(PlayerInfo* info, const Options& options, bool authoritative);
    uintptr_t m_moduleBase = 0;
    bool m_supported = false;
    std::atomic<PlayerInfo*> m_clientInfo{nullptr};
    std::atomic<int> m_playerId{-1};
    std::atomic<ULONGLONG> m_lastClientUpdate{0};
    std::atomic<const char*> m_actionStatus{"No changes requested"};
    std::mutex m_stateMutex;
    DiagnosticsInfo m_diagnostics;
    std::mutex m_actionMutex;
    std::vector<Action> m_actions;
    std::vector<MirrorWrite> m_mirrorWrites;
    Options m_options;
    PlayerObject* m_shieldShip = nullptr;
    double m_nextShieldTime = 0;
};
} // namespace Game
