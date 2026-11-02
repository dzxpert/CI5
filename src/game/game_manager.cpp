#include "game_manager.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <cstdio>

namespace Game {
namespace {
// RVAs, not addresses from the snapshot's relocated image base (0x360000).
constexpr uintptr_t SetEncodedRva = 0x18F000;
constexpr uintptr_t InstallWeaponRva = 0x0EE510; // command wrapper, not local_installWeapon
constexpr uintptr_t GiveShieldRva = 0x0EE910;

void Log(const char* text, uintptr_t player = 0) {
    char path[MAX_PATH]{};
    HMODULE self = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                      reinterpret_cast<LPCSTR>(&Log), &self);
    if (!GetModuleFileNameA(self, path, MAX_PATH)) return;
    char* slash = strrchr(path, '\\');
    if (!slash) return;
    strcpy_s(slash + 1, MAX_PATH - (slash + 1 - path), "CI5Mod.log");
    FILE* file = nullptr;
    if (fopen_s(&file, path, "a") == 0) {
        fprintf(file, "%llu %s player=0x%08X\n", GetTickCount64(), text, static_cast<unsigned>(player));
        fclose(file);
    }
}

bool NativeSet(uintptr_t function, void* object, int value) {
    __try {
        reinterpret_cast<int(__thiscall*)(void*, int)>(function)(object, value);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool NativeInstall(uintptr_t function, void* object, int type, int power) {
    __try {
        reinterpret_cast<int(__thiscall*)(void*, int, int)>(function)(object, type, power);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool NativeShield(uintptr_t function, void* object) {
    __try {
        reinterpret_cast<int(__thiscall*)(void*, double)>(function)(object, 2.0);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
}

GameManager& GameManager::Get() { static GameManager instance; return instance; }
void GameManager::ReportHookStatus(const char* status) { Log(status); }
uintptr_t GameManager::GetModuleBase() {
    if (!m_moduleBase) m_moduleBase = Core::Memory::GetModuleBase(nullptr);
    return m_moduleBase;
}
void GameManager::Initialize() {
    static_assert(sizeof(void*) == 4, "CI5 requires an x86 DLL");
    const auto base = GetModuleBase();
    // Relocation-independent instruction bytes at the verified setter and update entry.
    m_supported = Core::Memory::Read<uint32_t>(base + SetEncodedRva) == 0xA1EC8B55 &&
        Core::Memory::Read<uint32_t>(base + SetEncodedRva + 8) == 0x43FDC069 &&
        Core::Memory::Read<uint32_t>(base + 0x6610) == 0x83EC8B55 &&
        Core::Memory::Read<uint16_t>(base + 0x6614) == 0xC0E4;
    m_diagnostics.moduleBase = base;
    m_diagnostics.staticBaseAddr = base + 0x5814F4;
    if (!m_supported) m_actionStatus = "Unsupported CI5 build: writes disabled";
    Log(m_supported ? "Initialized verified CI5 build" : "Unsupported build");
}
void GameManager::Shutdown() {
    std::lock_guard<std::mutex> lock(m_actionMutex);
    m_actions.clear();
    m_mirrorWrites.clear();
    m_clientInfo = nullptr;
}
bool GameManager::ValidatePlayerInfo(PlayerInfo* info) {
    const auto p = reinterpret_cast<uintptr_t>(info);
    return p && Core::Memory::Read<uintptr_t>(p) == m_moduleBase + 0x44E230 &&
        Core::Memory::Read<uintptr_t>(p + 0xB4) == p;
}
PlayerObject* GameManager::PlayerShip(PlayerInfo* info) {
    if (!ValidatePlayerInfo(info)) return nullptr;
    auto ship = Core::Memory::Read<PlayerObject*>(reinterpret_cast<uintptr_t>(info) + 0xB0);
    const auto p = reinterpret_cast<uintptr_t>(ship);
    if (!p || Core::Memory::Read<uintptr_t>(p) != m_moduleBase + 0x4696D8 ||
        Core::Memory::Read<PlayerInfo*>(p + 0x2AC) != info) return nullptr;
    return ship;
}
PlayerInfo* GameManager::GetLocalPlayerInfo() {
    auto info = m_clientInfo.load();
    // Never rediscover a player by scanning old heap allocations.
    if (GetTickCount64() - m_lastClientUpdate.load() > 1500 || !ValidatePlayerInfo(info)) return nullptr;
    return info;
}
PlayerObject* GameManager::GetLocalPlayerObject() { return PlayerShip(GetLocalPlayerInfo()); }
bool GameManager::IsInGame() { return GetLocalPlayerInfo() != nullptr; }
Weapon* GameManager::GetActiveWeapon() {
    auto ship = GetLocalPlayerObject();
    if (!ship) return nullptr;
    auto weapon = Core::Memory::Read<Weapon*>(reinterpret_cast<uintptr_t>(ship) + 0x2C8);
    int power;
    return ReadEncoded(weapon, Offset::WeaponPower, power) ? weapon : nullptr;
}
DiagnosticsInfo GameManager::GetDiagnostics() {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    auto result = m_diagnostics;
    if (!GetLocalPlayerInfo()) {
        result.statusMessage = "No active local player update (menu or paused)";
        result.playerInfo = result.playerObj = result.authoritativeInfo = 0;
    }
    return result;
}

void GameManager::OnPlayerUpdate(PlayerInfo* info) {
    if (!m_supported || !ValidatePlayerInfo(info)) return;
    const auto ctx = Core::Memory::Read<uintptr_t>(m_moduleBase + 0x5814F4);
    const auto network = Core::Memory::Read<uintptr_t>(ctx + 0x28);
    const auto role = Core::Memory::Read<uintptr_t>(network + 0xC);
    if (!role) return;
    const int id = Core::Memory::Read<int>(reinterpret_cast<uintptr_t>(info) + 0x90);
    // isServer is +0x1D0. The client role owns the local input/player ID.
    const bool server = Core::Memory::Read<uint8_t>(role + 0x1D0) != 0;
    if (!server) {
        if (id != Core::Memory::Read<int>(role + 0x98)) return;
        if (info != m_clientInfo.load()) Log("Captured local client", reinterpret_cast<uintptr_t>(info));
        m_playerId = id;
        m_clientInfo = info;
        m_lastClientUpdate = GetTickCount64();
        std::vector<MirrorWrite> mirrors;
        Options options;
        {
            std::lock_guard<std::mutex> lock(m_actionMutex);
            mirrors.swap(m_mirrorWrites);
            options = m_options;
        }
        // Direct stat edits must reach both local simulation copies. Apply each
        // copy in its own role callback; never invoke game code from EndScene.
        for (const auto& write : mirrors) {
            if (write.clientIdentity != info) continue;
            void* target = write.ship ? static_cast<void*>(PlayerShip(info)) : info;
            if (!WriteEncoded(target, write.offset, write.value))
                m_actionStatus = "Client readback failed after authoritative write";
        }
        ApplyOptions(info, options, false);
        std::lock_guard<std::mutex> lock(m_stateMutex);
        m_diagnostics.baseCtx = ctx;
        m_diagnostics.gameUniverse = network;
        m_diagnostics.role = role;
        m_diagnostics.localPlayerId = id;
        m_diagnostics.rootObj = Core::Memory::Read<uintptr_t>(role + 0x2FC);
        m_diagnostics.playerInfo = reinterpret_cast<uintptr_t>(info);
        m_diagnostics.playerObj = reinterpret_cast<uintptr_t>(PlayerShip(info));
        m_diagnostics.statusMessage = "Local player captured from game update";
        return;
    }
    if (id != m_playerId.load() || !GetLocalPlayerInfo()) return;
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (m_diagnostics.authoritativeInfo != reinterpret_cast<uintptr_t>(info))
            Log("Captured authoritative player", reinterpret_cast<uintptr_t>(info));
        m_diagnostics.authoritativeInfo = reinterpret_cast<uintptr_t>(info);
    }
    std::vector<Action> actions;
    Options options;
    {
        std::lock_guard<std::mutex> lock(m_actionMutex);
        actions.swap(m_actions);
        options = m_options;
    }
    for (auto& action : actions) {
        if (action.clientIdentity != m_clientInfo.load() || action.playerId != id ||
            GetTickCount64() - action.queuedAt > 10000) {
            m_actionStatus = "Change expired or player session changed";
            continue;
        }
        m_actionStatus = action.apply(info) ? "Applied on authoritative player" : "Change failed: invalid state or value";
        Log(m_actionStatus.load(), reinterpret_cast<uintptr_t>(info));
    }
    ApplyOptions(info, options, true);
}

bool GameManager::QueueAction(std::function<bool(PlayerInfo*)> action) {
    auto info = GetLocalPlayerInfo();
    if (!m_supported || !info) {
        m_actionStatus = "No active local player; start/resume a mission";
        return false;
    }
    std::lock_guard<std::mutex> lock(m_actionMutex);
    if (m_actions.size() >= 64) {
        m_actionStatus = "Waiting for simulation; action queue is full";
        return false;
    }
    m_actions.push_back({info, m_playerId.load(), GetTickCount64(), std::move(action)});
    m_actionStatus = "Queued: waiting for authoritative player update";
    return true;
}
bool GameManager::WriteEncoded(void* object, uintptr_t offset, int value) {
    int current;
    if (!ReadEncoded(object, offset, current)) return false;
    if (!NativeSet(m_moduleBase + SetEncodedRva,
                   reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(object) + offset), value)) return false;
    return ReadEncoded(object, offset, current) && current == value;
}
bool GameManager::WriteStat(PlayerInfo* info, uintptr_t offset, int value) {
    if (!WriteEncoded(info, offset, value)) return false;
    std::lock_guard<std::mutex> lock(m_actionMutex);
    m_mirrorWrites.push_back({m_clientInfo.load(), offset, value, false});
    return true;
}
bool GameManager::QueueStat(uintptr_t offset, int value, bool add) {
    if (!add && value < 0) { m_actionStatus = "Stat values must be nonnegative"; return false; }
    return QueueAction([this, offset, value, add](PlayerInfo* info) {
        int current;
        if (!ReadEncoded(info, offset, current) || current < 0) return false;
        const int64_t next = add ? static_cast<int64_t>(current) + value : value;
        const int clamped = static_cast<int>((std::clamp)(next, int64_t{0}, int64_t{INT_MAX}));
        return WriteStat(info, offset, clamped);
    });
}
bool GameManager::SetLives(int n) { return QueueStat(Offset::Lives, n, false); }
bool GameManager::AddLives(int n) { return QueueStat(Offset::Lives, n, true); }
bool GameManager::SetMissiles(int n) { return QueueStat(Offset::Missiles, n, false); }
bool GameManager::AddMissiles(int n) { return QueueStat(Offset::Missiles, n, true); }
bool GameManager::SetScore(int n) { return QueueStat(Offset::Score, n, false); }
bool GameManager::AddScore(int n) { return QueueStat(Offset::Score, n, true); }
bool GameManager::SetFood(int n) { return QueueStat(Offset::Food, n, false); }
bool GameManager::AddFood(int n) { return QueueStat(Offset::Food, n, true); }
bool GameManager::InstallWeapon(int type, int power) {
    if (type < 0 || type >= static_cast<int>(EWeaponType::Count) || power < 0 || power > 999) {
        m_actionStatus = "Invalid weapon type or power"; return false;
    }
    return QueueAction([this, type, power](PlayerInfo* info) {
        auto ship = PlayerShip(info);
        return ship && NativeInstall(m_moduleBase + InstallWeaponRva, ship, type, power);
    });
}
bool GameManager::SetWeaponPower(int power) {
    if (power < 0 || power > 999) {
        m_actionStatus = "Invalid weapon power level"; return false;
    }
    return QueueAction([this, power](PlayerInfo* info) {
        auto ship = PlayerShip(info);
        if (!ship) return false;
        auto weapon = Core::Memory::Read<Weapon*>(reinterpret_cast<uintptr_t>(ship) + 0x2C8);
        return weapon && WriteEncoded(weapon, Offset::WeaponPower, power);
    });
}
uintptr_t GameManager::GetWaveController() {
    const auto base = GetModuleBase();
    if (!base) return 0;
    const auto ctx = Core::Memory::Read<uintptr_t>(base + 0x5814F4);
    if (!ctx) return 0;
    const auto network = Core::Memory::Read<uintptr_t>(ctx + 0x28);
    if (!network) return 0;
    const auto role = Core::Memory::Read<uintptr_t>(network + 0xC);
    if (!role) return 0;
    const auto rootObj = Core::Memory::Read<uintptr_t>(role + 0x2FC);
    if (!rootObj) return 0;
    return Core::Memory::Read<uintptr_t>(rootObj + 148);
}
int GameManager::GetProfileKeys() {
    const auto base = GetModuleBase();
    if (!base) return -1;
    const auto pSystem = Core::Memory::Read<uintptr_t>(base + 0x5814E8);
    if (!pSystem) return -1;
    int slot = Core::Memory::Read<int>(pSystem + 3352);
    if (slot < 0 || slot > 32) slot = 0;
    const uintptr_t profileBase = pSystem + 110064 * slot;
    const uint32_t w1 = Core::Memory::Read<uint32_t>(profileBase + 102760);
    const uint32_t w2 = Core::Memory::Read<uint32_t>(profileBase + 102892);
    return static_cast<int>(w1 ^ w2);
}
bool GameManager::SetProfileKeys(int keys) {
    if (keys < 0) { m_actionStatus = "Keys must be nonnegative"; return false; }
    const auto base = GetModuleBase();
    if (!base) return false;
    const auto pSystem = Core::Memory::Read<uintptr_t>(base + 0x5814E8);
    if (!pSystem) { m_actionStatus = "Profile system not ready"; return false; }
    int slot = Core::Memory::Read<int>(pSystem + 3352);
    if (slot < 0 || slot > 32) slot = 0;
    const uintptr_t profileBase = pSystem + 110064 * slot;
    const uint32_t mask = static_cast<uint32_t>(GetTickCount64() ^ 0x4B455953); // "KEYS"
    const bool ok1 = Core::Memory::SafeWrite<uint32_t>(profileBase + 102760, mask);
    const bool ok2 = Core::Memory::SafeWrite<uint32_t>(profileBase + 102892, static_cast<uint32_t>(keys) ^ mask);
    if (ok1 && ok2) {
        m_actionStatus = "Profile keys updated";
        Log("Updated profile keys", keys);
        return true;
    }
    m_actionStatus = "Failed to write profile keys";
    return false;
}
bool GameManager::AddProfileKeys(int delta) {
    int cur = GetProfileKeys();
    if (cur < 0) cur = 0;
    int64_t next = static_cast<int64_t>(cur) + delta;
    if (next < 0) next = 0;
    if (next > INT_MAX) next = INT_MAX;
    return SetProfileKeys(static_cast<int>(next));
}
int GameManager::GetMissionKeys() {
    uintptr_t waveCtrl = GetWaveController();
    if (!waveCtrl) return -1;
    uint32_t w1 = Core::Memory::Read<uint32_t>(waveCtrl + 920);
    uint32_t w2 = Core::Memory::Read<uint32_t>(waveCtrl + 924);
    return static_cast<int>(w1 ^ w2);
}
bool GameManager::SetMissionKeys(int keys) {
    if (keys < 0) return false;
    uintptr_t waveCtrl = GetWaveController();
    if (!waveCtrl) return false;
    uint32_t mask = static_cast<uint32_t>(GetTickCount64() ^ 0x57415645);
    bool ok1 = Core::Memory::SafeWrite<uint32_t>(waveCtrl + 920, mask);
    bool ok2 = Core::Memory::SafeWrite<uint32_t>(waveCtrl + 924, static_cast<uint32_t>(keys) ^ mask);
    return ok1 && ok2;
}
bool GameManager::AddMissionKeys(int delta) {
    int cur = GetMissionKeys();
    if (cur < 0) cur = 0;
    int64_t next = static_cast<int64_t>(cur) + delta;
    if (next < 0) next = 0;
    if (next > INT_MAX) next = INT_MAX;
    return SetMissionKeys(static_cast<int>(next));
}
bool GameManager::SetKeys(int keys) {
    bool ok = SetProfileKeys(keys);
    SetMissionKeys(keys);
    return ok;
}
bool GameManager::AddKeys(int delta) {
    int cur = GetProfileKeys();
    if (cur < 0) cur = GetMissionKeys();
    if (cur < 0) cur = 0;
    int64_t next = static_cast<int64_t>(cur) + delta;
    if (next < 0) next = 0;
    if (next > INT_MAX) next = INT_MAX;
    return SetKeys(static_cast<int>(next));
}
bool GameManager::CoolShip(PlayerInfo* info) {
    auto ship = PlayerShip(info);
    return ship && WriteEncoded(ship, Offset::WeaponEnergy, 1000000) &&
        WriteEncoded(ship, Offset::Overheated, 0);
}
bool GameManager::SetHeat(int heat) {
    if (heat < 0 || heat > 1000000) { m_actionStatus = "Heat must be between 0 and 1000000"; return false; }
    return QueueAction([this, heat](PlayerInfo* info) {
        auto ship = PlayerShip(info);
        if (!ship || !WriteEncoded(ship, Offset::WeaponEnergy, 1000000 - heat)) return false;
        if (heat == 0 && !WriteEncoded(ship, Offset::Overheated, 0)) return false;
        std::lock_guard<std::mutex> lock(m_actionMutex);
        m_mirrorWrites.push_back({m_clientInfo.load(), Offset::WeaponEnergy, 1000000 - heat, true});
        if (heat == 0) m_mirrorWrites.push_back({m_clientInfo.load(), Offset::Overheated, 0, true});
        return true;
    });
}
bool GameManager::ResetHeat() { return SetHeat(0); }
bool GameManager::GiveShield(PlayerInfo* info) {
    auto ship = PlayerShip(info);
    return ship && ship->GetLifeState() == 3 && NativeShield(m_moduleBase + GiveShieldRva, ship);
}
bool GameManager::SetInvulnerability(bool enabled) {
    // Shields expire naturally; never overwrite the unrelated +0x2518 flag.
    return !enabled || QueueAction([this](PlayerInfo* info) { return GiveShield(info); });
}
bool GameManager::Teleport(float x, float y) {
    if (!std::isfinite(x) || !std::isfinite(y)) return false;
    return QueueAction([this, x, y](PlayerInfo* info) {
        auto ship = PlayerShip(info);
        return ship && Core::Memory::SafeWrite(reinterpret_cast<uintptr_t>(ship) + 0x1B0, x) &&
            Core::Memory::SafeWrite(reinterpret_cast<uintptr_t>(ship) + 0x1B4, y);
    });
}
void GameManager::ConfigureSurvival(bool shield, bool cool, bool lives, int targetLives,
                                    bool missiles, int targetMissiles) {
    std::lock_guard<std::mutex> lock(m_actionMutex);
    m_options.shield = shield; m_options.cool = cool;
    m_options.lives = lives; m_options.targetLives = (std::clamp)(targetLives, 1, 999);
    m_options.missiles = missiles; m_options.targetMissiles = (std::clamp)(targetMissiles, 1, 999);
}
void GameManager::ConfigureRapidFire(bool enabled, double interval) {
    std::lock_guard<std::mutex> lock(m_actionMutex);
    m_options.rapidFire = enabled;
    m_options.interval = std::isfinite(interval) ? (std::clamp)(interval, 0.01, 0.5) : 0.05;
}
void GameManager::ApplyOptions(PlayerInfo* info, const Options& options, bool authoritative) {
    int value;
    if (options.lives && ReadEncoded(info, Offset::Lives, value) && value < options.targetLives)
        WriteEncoded(info, Offset::Lives, options.targetLives);
    if (options.missiles && ReadEncoded(info, Offset::Missiles, value) && value < options.targetMissiles)
        WriteEncoded(info, Offset::Missiles, options.targetMissiles);
    if (options.cool) CoolShip(info);
    auto ship = PlayerShip(info);
    if (!ship) return;
    if (options.rapidFire) {
        auto weapon = Core::Memory::Read<Weapon*>(reinterpret_cast<uintptr_t>(ship) + 0x2C8);
        if (ReadEncoded(weapon, Offset::WeaponPower, value))
            Core::Memory::SafeWrite(reinterpret_cast<uintptr_t>(weapon) + 0x2D0, options.interval);
    }
    if (!authoritative) return;
    const double time = Core::Memory::Read<double>(reinterpret_cast<uintptr_t>(ship) + 0xB0);
    if (!options.shield) { m_shieldShip = nullptr; return; }
    // Use simulation time: a paused game must not accumulate shield entities.
    if (ship != m_shieldShip || time >= m_nextShieldTime || time < m_nextShieldTime - 2.0) {
        if (GiveShield(info)) { m_shieldShip = ship; m_nextShieldTime = time + 1.0; }
    }
}
#ifdef CI5MOD_LIVE_TEST
bool GameManager::QueueStatRoundTripTest() {
    return QueueAction([this](PlayerInfo* info) {
        for (auto offset : {Offset::Lives, Offset::Missiles, Offset::Score, Offset::Food}) {
            int original;
            if (!ReadEncoded(info, offset, original) || original < 0 || original == INT_MAX) return false;
            const bool changed = WriteEncoded(info, offset, original + 1);
            const bool restored = WriteEncoded(info, offset, original);
            if (!changed || !restored) { Log("LIVE TEST FAILED", offset); return false; }
        }
        Log("LIVE TEST PASSED: lives/missiles/score/food increment, readback, restore", reinterpret_cast<uintptr_t>(info));
        return true;
    });
}
#endif
} // namespace Game
