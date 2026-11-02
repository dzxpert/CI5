#pragma once

#include <windows.h>
#include <d3d9.h>
#include <atomic>
#include "../game/game_structures.h"

namespace Core {

class Hooks {
public:
    static Hooks& Get();

    bool Initialize();
    void Shutdown();

    bool IsInitialized() const { return m_initialized; }

private:
    Hooks() = default;
    ~Hooks() = default;

    static HRESULT STDMETHODCALLTYPE HookedEndScene(IDirect3DDevice9* pDevice);
    static HRESULT STDMETHODCALLTYPE HookedReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
    static LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static char __fastcall HookedPlayerUpdate(Game::PlayerInfo* info, void*, double time, double delta);

    typedef HRESULT(STDMETHODCALLTYPE* tEndScene)(IDirect3DDevice9* pDevice);
    typedef HRESULT(STDMETHODCALLTYPE* tReset)(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
    typedef char(__thiscall* tPlayerUpdate)(Game::PlayerInfo*, double, double);

    tEndScene m_oEndScene = nullptr;
    tReset m_oReset = nullptr;
    tPlayerUpdate m_oPlayerUpdate = nullptr;
    WNDPROC m_oWndProc = nullptr;
    HWND m_hGameWnd = nullptr;

    bool m_initialized = false;
    std::atomic<bool> m_stopping{false};
    std::atomic<unsigned> m_activeCallbacks{0};
};

} // namespace Core
