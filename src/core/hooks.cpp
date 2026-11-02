#include "hooks.h"
#include "../ui/gui.h"
#include "../game/game_manager.h"
#include <MinHook.h>
#include <imgui.h>
#include <iostream>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Core {
namespace {
struct CallbackGuard {
    std::atomic<unsigned>& count;
    explicit CallbackGuard(std::atomic<unsigned>& value) : count(value) { ++count; }
    ~CallbackGuard() { --count; }
};
}

Hooks& Hooks::Get() {
    static Hooks instance;
    return instance;
}

bool Hooks::Initialize() {
    if (m_initialized) return true;
    if (!Game::GameManager::Get().IsSupported()) return false;

    if (MH_Initialize() != MH_OK) {
        Game::GameManager::Get().ReportHookStatus("MH_Initialize failed");
        return false;
    }

    // Create a temporary dummy window to obtain the IDirect3DDevice9 vtable
    WNDCLASSEXA wc = { sizeof(WNDCLASSEXA), CS_CLASSDC, DefWindowProcA, 0L, 0L, GetModuleHandleA(nullptr), nullptr, nullptr, nullptr, nullptr, "CI5_Dummy", nullptr };
    RegisterClassExA(&wc);
    HWND dummyWnd = CreateWindowA("CI5_Dummy", "Dummy", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D) {
        Game::GameManager::Get().ReportHookStatus("Direct3DCreate9 failed");
        DestroyWindow(dummyWnd);
        UnregisterClassA("CI5_Dummy", wc.hInstance);
        MH_Uninitialize();
        return false;
    }

    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = dummyWnd;

    IDirect3DDevice9* pDummyDevice = nullptr;
    HRESULT hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dummyWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);
    if (FAILED(hr) || !pDummyDevice) {
        hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, dummyWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);
    }
    if (FAILED(hr) || !pDummyDevice) {
        // A fullscreen exclusive game can prevent another HAL/REF device.
        // NULLREF needs no display surface; only its COM method table is used.
        hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, dummyWnd,
                              D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);
    }

    if (FAILED(hr) || !pDummyDevice) {
        Game::GameManager::Get().ReportHookStatus("Dummy D3D9 device creation failed");
        char error[96];
        sprintf_s(error, "CreateDevice HRESULT=0x%08X, window=0x%08X", static_cast<unsigned>(hr),
                  reinterpret_cast<unsigned>(dummyWnd));
        Game::GameManager::Get().ReportHookStatus(error);
        pD3D->Release();
        DestroyWindow(dummyWnd);
        UnregisterClassA("CI5_Dummy", wc.hInstance);
        MH_Uninitialize();
        return false;
    }

    void** vTable = *reinterpret_cast<void***>(pDummyDevice);
    void* pResetAddr = vTable[16];
    void* pEndSceneAddr = vTable[42];

    pDummyDevice->Release();
    pD3D->Release();
    DestroyWindow(dummyWnd);
    UnregisterClassA("CI5_Dummy", wc.hInstance);

    // Hook EndScene and Reset
    if (auto status = MH_CreateHook(pEndSceneAddr, &HookedEndScene, reinterpret_cast<LPVOID*>(&m_oEndScene)); status != MH_OK) {
        Game::GameManager::Get().ReportHookStatus("EndScene failed");
        Game::GameManager::Get().ReportHookStatus(MH_StatusToString(status));
        MH_Uninitialize();
        return false;
    }

    if (auto status = MH_CreateHook(pResetAddr, &HookedReset, reinterpret_cast<LPVOID*>(&m_oReset)); status != MH_OK) {
        Game::GameManager::Get().ReportHookStatus("Reset failed");
        Game::GameManager::Get().ReportHookStatus(MH_StatusToString(status));
        MH_Uninitialize();
        return false;
    }

    auto update = reinterpret_cast<void*>(Game::GameManager::Get().GetModuleBase() + 0x6610);
    if (auto status = MH_CreateHook(update, &HookedPlayerUpdate, reinterpret_cast<LPVOID*>(&m_oPlayerUpdate)); status != MH_OK) {
        Game::GameManager::Get().ReportHookStatus("PlayerInfo update failed");
        Game::GameManager::Get().ReportHookStatus(MH_StatusToString(status));
        MH_Uninitialize();
        return false;
    }
    if (auto status = MH_EnableHook(MH_ALL_HOOKS); status != MH_OK) {
        Game::GameManager::Get().ReportHookStatus("Enable hooks failed");
        Game::GameManager::Get().ReportHookStatus(MH_StatusToString(status));
        MH_Uninitialize();
        return false;
    }

    Game::GameManager::Get().ReportHookStatus("D3D9 and PlayerInfo hooks installed");
    m_initialized = true;
    return true;
}

void Hooks::Shutdown() {
    if (!m_initialized) return;
    m_stopping = true;

    if (m_hGameWnd && m_oWndProc) {
        SetWindowLongPtrA(m_hGameWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_oWndProc));
    }

    MH_DisableHook(MH_ALL_HOOKS);
    // A callback can still be using the original function trampoline or ImGui.
    while (m_activeCallbacks.load() != 0) Sleep(1);
    MH_Uninitialize();

    UI::GUI::Get().Shutdown();

    m_initialized = false;
}

char __fastcall Hooks::HookedPlayerUpdate(Game::PlayerInfo* info, void*, double time, double delta) {
    auto& hooks = Hooks::Get();
    CallbackGuard guard(hooks.m_activeCallbacks);
    // THEROLE is scoped to simulation callbacks and is null during EndScene.
    if (!hooks.m_stopping) Game::GameManager::Get().OnPlayerUpdate(info);
    return hooks.m_oPlayerUpdate(info, time, delta);
}

HRESULT STDMETHODCALLTYPE Hooks::HookedEndScene(IDirect3DDevice9* pDevice) {
    auto& hooks = Hooks::Get();
    CallbackGuard guard(hooks.m_activeCallbacks);
    if (hooks.m_stopping) return hooks.m_oEndScene(pDevice);

    if (!UI::GUI::Get().IsInitialized()) {
        D3DDEVICE_CREATION_PARAMETERS params;
        HWND hwnd = nullptr;
        if (SUCCEEDED(pDevice->GetCreationParameters(&params)) && params.hFocusWindow) {
            hwnd = params.hFocusWindow;
        } else {
            hwnd = FindWindowA(nullptr, "Chicken Invaders 5");
            if (!hwnd) {
                hwnd = GetActiveWindow();
            }
        }

        if (hwnd) {
            hooks.m_hGameWnd = hwnd;
            hooks.m_oWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&HookedWndProc)));
            UI::GUI::Get().Initialize(hwnd, pDevice);
        }
    }

    if (UI::GUI::Get().IsInitialized()) {
        UI::GUI::Get().Render();
    }

    return hooks.m_oEndScene(pDevice);
}

HRESULT STDMETHODCALLTYPE Hooks::HookedReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    auto& hooks = Hooks::Get();
    CallbackGuard guard(hooks.m_activeCallbacks);
    if (hooks.m_stopping) return hooks.m_oReset(pDevice, pPresentationParameters);

    UI::GUI::Get().OnDeviceLost();
    HRESULT hr = hooks.m_oReset(pDevice, pPresentationParameters);
    if (SUCCEEDED(hr)) {
        UI::GUI::Get().OnDeviceReset();
    }

    return hr;
}

LRESULT CALLBACK Hooks::HookedWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto& hooks = Hooks::Get();
    CallbackGuard guard(hooks.m_activeCallbacks);
    if (hooks.m_stopping)
        return hooks.m_oWndProc ? CallWindowProcA(hooks.m_oWndProc, hWnd, uMsg, wParam, lParam)
                               : DefWindowProcA(hWnd, uMsg, wParam, lParam);

    // Toggle menu with INSERT key
    if (uMsg == WM_KEYDOWN && wParam == VK_INSERT) {
        UI::GUI::Get().ToggleOpen();
        return 0;
    }

    if (UI::GUI::Get().IsOpen()) {
        if (uMsg == WM_SETCURSOR) {
            SetCursor(LoadCursorA(nullptr, IDC_ARROW));
            return TRUE;
        }

        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
            // Block game from receiving mouse clicks / key strokes when interacting with ImGui
            return 1;
        }
    }

    if (hooks.m_oWndProc) {
        return CallWindowProcA(hooks.m_oWndProc, hWnd, uMsg, wParam, lParam);
    }
    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

} // namespace Core
