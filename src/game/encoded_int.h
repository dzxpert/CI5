#pragma once

#include <cstdint>

namespace Game {

// CI5's 0x110-byte integer container, verified in sub_4EEE80/4EEFC0/4EF000
// (IDA snapshot image base 0x360000). Reading must not rotate the game RNG.
struct EncodedInt {
    uint32_t words[64];
    uint32_t first;
    uint32_t second;
    uint32_t check;
    uint32_t errors;

    bool Decode(int& value) const {
        if (first >= 64 || second >= 64 || check >= 64 ||
            first == second || first == check || second == check || errors != 0)
            return false;
        if (words[check] != check + words[first] * words[second])
            return false;
        value = static_cast<int32_t>(words[first] ^ words[second]);
        return true;
    }
};
static_assert(sizeof(EncodedInt) == 0x110, "CI5 encoded integer layout");

namespace Offset {
    constexpr uintptr_t Score = 0xD8;
    constexpr uintptr_t Lives = 0x1E8;
    constexpr uintptr_t Food = 0x2F8;
    constexpr uintptr_t Missiles = 0x408;
    constexpr uintptr_t NextExtraLife = 0x518;
    constexpr uintptr_t WeaponEnergy = 0x2D8;
    constexpr uintptr_t Overheated = 0x4F8;
    constexpr uintptr_t WeaponPower = 0x2F8;
}
} // namespace Game
