#pragma once

#include <cstdint>
#include <string>
#include "encoded_int.h"
#include "../core/memory.h"

namespace Game {

inline bool ReadEncoded(const void* object, uintptr_t offset, int& value) {
    EncodedInt snapshot{};
    return object && Core::Memory::SafeRead(reinterpret_cast<uintptr_t>(object) + offset, &snapshot)
        && snapshot.Decode(value);
}

inline int EncodedValue(const void* object, uintptr_t offset) {
    int value = -1; // Invalid/unavailable, never silently treat a failed read as zero.
    ReadEncoded(object, offset, value);
    return value;
}


enum class EWeaponType : int {
    IonBlaster = 0,
    NeutronGun = 1,
    LaserCannon = 2,
    VulcanChaingun = 3,
    LightningFryer = 4,
    PlasmaRifle = 5,
    UtensilPoker = 6,
    MachineGun = 7,
    PositronStream = 8,
    TheRiddler = 9,
    Hypergun = 10,
    BoronRailgun = 11,
    CornFlaker = 12,
    Count = 13
};

struct WeaponInfo {
    int id;
    const char* name;
    const char* internalClass;
    const char* description;
};

inline const WeaponInfo G_WEAPON_CATALOG[] = {
    { 0,  "Ion Blaster",     "WeaponIon",         "Default twin energy bolts; balanced heat & fire rate" },
    { 1,  "Neutron Gun",     "WeaponNeutron",     "Heavy green slow-moving explosive spheres" },
    { 2,  "Laser Cannon",    "WeaponLaser",       "Continuous yellow concentrated beam; rapid heat build-up" },
    { 3,  "Vulcan Chaingun", "WeaponGatling",     "High-velocity kinetic gatling spread; low per-shot heat" },
    { 4,  "Lightning Fryer", "WeaponLightning",   "Blue electrical arcs branching to nearby enemies" },
    { 5,  "Plasma Rifle",    "WeaponPlasma",      "Heavy teal plasma balls with splash radius" },
    { 6,  "Utensil Poker",   "WeaponFork",        "Secret cutlery barrage (forks, knives, spoons)" },
    { 7,  "Machine Gun",     "WeaponMachineGun",  "Rapid-fire scattering kinetic barrage" },
    { 8,  "Positron Stream", "WeaponSwarm",       "Swarming micro-homing projectiles" },
    { 9,  "The Riddler",     "WeaponCrazy",       "Erratic bouncing and curving projectiles" },
    { 10, "Hypergun",        "WeaponHyperGun",    "Piercing high-energy spherical blasts" },
    { 11, "Boron Railgun",   "WeaponUzi",         "Micro-bullet ultra high-fire-rate spray" },
    { 12, "Corn Flak-er",    "WeaponCorn",        "Popcorn / unpopped kernel shotgun spread" }
};

inline const char* GetWeaponName(int typeId) {
    if (typeId >= 0 && typeId < static_cast<int>(EWeaponType::Count)) {
        return G_WEAPON_CATALOG[typeId].name;
    }
    return "Unknown Weapon";
}

// Memory Layout: Weapon (Base Class)
#pragma pack(push, 1)
class Weapon {
public:
    uintptr_t GetVTable() const { return *reinterpret_cast<const uintptr_t*>(this); }
    int GetWeaponType() const { return *reinterpret_cast<const int*>(reinterpret_cast<const char*>(this) + 0x2C8); }
    
    double GetFireInterval() const { return *reinterpret_cast<const double*>(reinterpret_cast<const char*>(this) + 0x2D0); }
    void SetFireInterval(double val) { *reinterpret_cast<double*>(reinterpret_cast<char*>(this) + 0x2D0) = val; }

    float GetHeatPerShot() const { return Core::Memory::Read<float>(reinterpret_cast<uintptr_t>(this) + 0x2F0); }
    bool IsFiring() const { return Core::Memory::Read<char>(reinterpret_cast<uintptr_t>(this) + 0x2F4) != 0; }
    int GetPowerLevel() const { return EncodedValue(this, Offset::WeaponPower); }

};

class PlayerInfo;

// Memory Layout: PlayerObject (Spaceship Entity)
class PlayerObject {
public:
    uintptr_t GetVTable() const { return *reinterpret_cast<const uintptr_t*>(this); }

    int GetLifeState() const { return *reinterpret_cast<const int*>(reinterpret_cast<const char*>(this) + 0x28); }

    float GetPosX() const { return *reinterpret_cast<const float*>(reinterpret_cast<const char*>(this) + 0x1B0); }
    void SetPosX(float val) { *reinterpret_cast<float*>(reinterpret_cast<char*>(this) + 0x1B0) = val; }

    float GetPosY() const { return *reinterpret_cast<const float*>(reinterpret_cast<const char*>(this) + 0x1B4); }
    void SetPosY(float val) { *reinterpret_cast<float*>(reinterpret_cast<char*>(this) + 0x1B4) = val; }

    float GetPosZ() const { return *reinterpret_cast<const float*>(reinterpret_cast<const char*>(this) + 0x1B8); }
    void SetPosZ(float val) { *reinterpret_cast<float*>(reinterpret_cast<char*>(this) + 0x1B8) = val; }

    float GetFacingAngle() const { return *reinterpret_cast<const float*>(reinterpret_cast<const char*>(this) + 0x1C4); }
    void SetFacingAngle(float val) { *reinterpret_cast<float*>(reinterpret_cast<char*>(this) + 0x1C4) = val; }

    PlayerInfo* GetPlayerInfo() const { return *reinterpret_cast<PlayerInfo* const*>(reinterpret_cast<const char*>(this) + 0x2AC); }
    Weapon* GetActiveWeapon() const { return *reinterpret_cast<Weapon* const*>(reinterpret_cast<const char*>(this) + 0x2C8); }

    // The stored value is remaining energy, not accumulated heat.
    float GetHeatPercentage() const {
        int energy = EncodedValue(this, Offset::WeaponEnergy);
        return energy < 0 ? -1.0f : (1000000 - energy) / 10000.0f;
    }

};

// Memory Layout: PlayerInfo (Session profile, lives, score, inventory)
class PlayerInfo {
public:
    uintptr_t GetVTable() const { return *reinterpret_cast<const uintptr_t*>(this); }
    PlayerInfo* GetPrev() const { return *reinterpret_cast<PlayerInfo* const*>(reinterpret_cast<const char*>(this) + 0x04); }
    PlayerInfo* GetNext() const { return *reinterpret_cast<PlayerInfo* const*>(reinterpret_cast<const char*>(this) + 0x08); }

    int GetPlayerId() const { return *reinterpret_cast<const int*>(reinterpret_cast<const char*>(this) + 0x90); }
    PlayerObject* GetPlayerObject() const { return *reinterpret_cast<PlayerObject* const*>(reinterpret_cast<const char*>(this) + 0xB0); }

    const wchar_t* GetPlayerName() const {
        // MSVC std::wstring: pointer/buffer at +0xB8, capacity at +0xB8 + 0x14
        size_t capacity = *reinterpret_cast<const size_t*>(reinterpret_cast<const char*>(this) + 0xB8 + 0x14);
        if (capacity >= 8) {
            const wchar_t* const* pHeapStr = reinterpret_cast<const wchar_t* const*>(reinterpret_cast<const char*>(this) + 0xB8);
            return pHeapStr ? *pHeapStr : L"";
        } else {
            return reinterpret_cast<const wchar_t*>(reinterpret_cast<const char*>(this) + 0xB8);
        }
    }

    int GetNextExtraLifeScore() const { return EncodedValue(this, Offset::NextExtraLife); }
    int GetFoodCollected() const { return EncodedValue(this, Offset::Food); }
    int GetScore() const { return EncodedValue(this, Offset::Score); }
    int GetLivesRemaining() const { return EncodedValue(this, Offset::Lives); }
    int GetMissilesAvailable() const { return EncodedValue(this, Offset::Missiles); }

};
#pragma pack(pop)

} // namespace Game
