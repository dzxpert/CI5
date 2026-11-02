#include "game/game_structures.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>

void Require(bool condition) {
    if (!condition) { std::fputs("Encoded stat regression failed\n", stderr); std::exit(1); }
}
Game::EncodedInt Encode(int value, uint32_t i, uint32_t j, uint32_t k) {
    Game::EncodedInt n{};
    for (unsigned slot = 0; slot < 64; ++slot) n.words[slot] = 0x9e3779b9u * (slot + 1);
    n.first = i; n.second = j; n.check = k;
    n.words[i] = n.words[j] ^ static_cast<uint32_t>(value);
    n.words[k] = k + n.words[i] * n.words[j];
    return n;
}
int main() {
    for (int value : {0, 1, -1, INT_MIN, INT_MAX, 1675788}) {
        for (unsigned i = 0; i < 64; ++i) {
            auto n = Encode(value, i, (i + 1) % 64, (i + 2) % 64);
            int decoded = 0;
            Require(n.Decode(decoded) && decoded == value);
            n.words[n.check] ^= 1;
            Require(!n.Decode(decoded));
        }
    }
    auto n = Encode(99, 1, 2, 3);
    int decoded;
    n.first = 64; Require(!n.Decode(decoded));
    n.first = 2; Require(!n.Decode(decoded));
    n.first = 1; n.errors = 1; Require(!n.Decode(decoded));

    // Distinct field fixtures catch wrong offsets, scalar reads, and coordinate
    // fields accidentally being interpreted as stats.
    std::array<unsigned char, 0x900> player{};
    const uintptr_t offsets[] = {Game::Offset::Score, Game::Offset::Lives,
        Game::Offset::Food, Game::Offset::Missiles, Game::Offset::NextExtraLife};
    const int values[] = {1675788, 3, 17, 8, 3000000};
    for (int i = 0; i < 5; ++i) {
        n = Encode(values[i], i, i + 10, i + 20);
        std::memcpy(player.data() + offsets[i], &n, sizeof(n));
    }
    std::memset(player.data() + 0x848, 0x5a, 12);
    auto info = reinterpret_cast<Game::PlayerInfo*>(player.data());
    Require(info->GetScore() == values[0]);
    Require(info->GetLivesRemaining() == values[1]);
    Require(info->GetFoodCollected() == values[2]);
    Require(info->GetMissilesAvailable() == values[3]);
    Require(info->GetNextExtraLifeScore() == values[4]);
    Require(!Game::ReadEncoded(nullptr, 0, decoded));

    std::array<unsigned char, 0x800> ship{};
    for (int energy : {0, 350000, 1000000}) {
        n = Encode(energy, 0, 63, 30);
        std::memcpy(ship.data() + Game::Offset::WeaponEnergy, &n, sizeof(n));
        Require(reinterpret_cast<Game::PlayerObject*>(ship.data())->GetHeatPercentage()
            == (1000000 - energy) / 10000.0f);
    }
    std::puts("Encoded reads, integrity validation, stat offsets and heat conversion passed");
}
