#include <windows.h>
#include "core/hooks.h"
#include "game/game_manager.h"

void SignalInitialization(bool ready) {
    wchar_t name[96];
    swprintf_s(name, L"Local\\CI5Mod.%s.%lu", ready ? L"Ready" : L"Failed", GetCurrentProcessId());
    HANDLE event = OpenEventW(EVENT_MODIFY_STATE, FALSE, name);
    if (event) { SetEvent(event); CloseHandle(event); }
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    HMODULE hModule = reinterpret_cast<HMODULE>(lpParam);

    // Initialize game manager
    Game::GameManager::Get().Initialize();

    // Initialize Direct3D 9 and WndProc hooks
    if (!Core::Hooks::Get().Initialize()) {
        SignalInitialization(false);
        Game::GameManager::Get().Shutdown();
        FreeLibraryAndExitThread(hModule, 1);
        return 1;
    }
    SignalInitialization(true);

    // Main loop: wait for user uninject trigger (END key)
    while (true) {
        if (GetAsyncKeyState(VK_END) & 0x8000) {
            // Wait until key is released
            while (GetAsyncKeyState(VK_END) & 0x8000) {
                Sleep(20);
            }
            break;
        }
        Sleep(100);
    }

    // Clean shutdown and unhook
    Core::Hooks::Get().Shutdown();
    Game::GameManager::Get().Shutdown();
    Sleep(200);

    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        {
            HANDLE thread = CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
            if (!thread) return FALSE;
            CloseHandle(thread);
        }
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
