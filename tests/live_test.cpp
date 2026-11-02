#include "game/game_manager.h"

// Test-only export. The remote thread queues work; game calls run exclusively
// in the authoritative update. Every changed stat is restored before returning
// to the original game update, with no intervening gameplay.
extern "C" __declspec(dllexport) DWORD WINAPI CI5Mod_TestStats(LPVOID) {
    return Game::GameManager::Get().QueueStatRoundTripTest() ? 0 : 1;
}
