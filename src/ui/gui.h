#pragma once

#include <windows.h>
#include <d3d9.h>

namespace UI {

class GUI {
public:
    static GUI& Get();

    void Initialize(HWND hwnd, IDirect3DDevice9* pDevice);
    void Shutdown();

    void OnDeviceLost();
    void OnDeviceReset();

    void Render();

    bool IsOpen() const { return m_open; }
    void ToggleOpen();
    void SetOpen(bool open);

    bool IsInitialized() const { return m_initialized; }
    HWND GetHwnd() const { return m_hwnd; }

private:
    GUI() = default;
    ~GUI() = default;

    void ApplyTheme();

    bool m_initialized = false;
    bool m_open = true; // Open by default upon injection so user sees it right away
    HWND m_hwnd = nullptr;
    IDirect3DDevice9* m_pDevice = nullptr;
};

} // namespace UI
