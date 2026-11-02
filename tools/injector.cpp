#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <vector>

struct Handle {
    HANDLE value = nullptr;
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};
std::vector<DWORD> FindGameProcesses() {
    std::vector<DWORD> result;
    Handle snapshot{CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)};
    PROCESSENTRY32W entry{sizeof(entry)};
    if (Process32FirstW(snapshot.value, &entry)) {
        do { if (_wcsicmp(entry.szExeFile, L"CI5.exe") == 0) result.push_back(entry.th32ProcessID); }
        while (Process32NextW(snapshot.value, &entry));
    }
    return result;
}
std::wstring LoadedMod(DWORD pid) {
    Handle snapshot{CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid)};
    MODULEENTRY32W entry{sizeof(entry)};
    if (Module32FirstW(snapshot.value, &entry)) {
        do { if (_wcsicmp(entry.szModule, L"CI5Mod.dll") == 0) return entry.szExePath; }
        while (Module32NextW(snapshot.value, &entry));
    }
    return {};
}
int wmain(int argc, wchar_t* argv[]) {
    std::wcout << L"CI5 Galactic Commander injector\n";
    wchar_t path[MAX_PATH]{};
    if (argc > 1) {
        DWORD length = GetFullPathNameW(argv[1], MAX_PATH, path, nullptr);
        if (!length || length >= MAX_PATH) { std::wcerr << L"Invalid DLL path.\n"; return 1; }
    } else {
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        wchar_t* slash = wcsrchr(path, L'\\');
        if (!slash) return 1;
        wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"CI5Mod.dll");
    }
    const auto attributes = GetFileAttributesW(path);
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        std::wcerr << L"DLL not found: " << path << L"\nRun build.bat first.\n"; return 1;
    }
    auto games = FindGameProcesses();
    if (games.size() != 1) {
        std::wcerr << L"Expected one running CI5.exe, found " << games.size()
                   << L". Start the game, then run inject.bat again.\n"; return 1;
    }
    const DWORD pid = games.front();
    auto loaded = LoadedMod(pid);
    if (!loaded.empty()) {
        std::wcerr << L"A mod is already loaded: " << loaded
                   << L"\nPress END in CI5 to unload it, then run inject.bat again.\n"; return 2;
    }
    std::wcout << L"DLL: " << path << L"\nPID: " << pid << std::endl;
    Handle process{OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid)};
    if (!process.value) { std::wcerr << L"OpenProcess failed: " << GetLastError() << L"\n"; return 1; }

    const auto suffix = std::to_wstring(pid);
    Handle ready{CreateEventW(nullptr, TRUE, FALSE, (L"Local\\CI5Mod.Ready." + suffix).c_str())};
    Handle failed{CreateEventW(nullptr, TRUE, FALSE, (L"Local\\CI5Mod.Failed." + suffix).c_str())};
    if (!ready.value || !failed.value) { std::wcerr << L"Could not create initialization events.\n"; return 1; }
    ResetEvent(ready.value); ResetEvent(failed.value);
    const size_t bytes = (wcslen(path) + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(process.value, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) { std::wcerr << L"VirtualAllocEx failed: " << GetLastError() << L"\n"; return 1; }
    SIZE_T written = 0;
    if (!WriteProcessMemory(process.value, remote, path, bytes, &written) || written != bytes) {
        std::wcerr << L"WriteProcessMemory failed: " << GetLastError() << L"\n";
        VirtualFreeEx(process.value, remote, 0, MEM_RELEASE); return 1;
    }
    auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
    Handle thread{loadLibrary ? CreateRemoteThread(process.value, nullptr, 0, loadLibrary, remote, 0, nullptr) : nullptr};
    if (!thread.value) {
        std::wcerr << L"CreateRemoteThread failed: " << GetLastError() << L"\n";
        VirtualFreeEx(process.value, remote, 0, MEM_RELEASE); return 1;
    }
    const DWORD wait = WaitForSingleObject(thread.value, 10000);
    if (wait != WAIT_OBJECT_0) {
        // The remote thread may still be reading its argument; do not free it.
        std::wcerr << L"DLL load did not finish within 10 seconds (wait=" << wait << L").\n"; return 1;
    }
    DWORD module = 0;
    const bool gotResult = GetExitCodeThread(thread.value, &module) != FALSE;
    VirtualFreeEx(process.value, remote, 0, MEM_RELEASE);
    if (!gotResult || !module) { std::wcerr << L"LoadLibraryW failed; the DLL was not loaded.\n"; return 1; }
    HANDLE signals[] = {ready.value, failed.value};
    const DWORD initialized = WaitForMultipleObjects(2, signals, FALSE, 10000);
    if (initialized != WAIT_OBJECT_0) {
        std::wcerr << (initialized == WAIT_OBJECT_0 + 1 ? L"Mod initialization failed.\n" : L"Mod initialization did not finish within 10 seconds.\n")
                   << L"Check CI5Mod.log beside the DLL for the actual error.\n";
        return 1;
    }
    std::wcout << L"SUCCESS: DLL loaded and game/overlay hooks initialized.\n"
                  L"INSERT: show menu. END: unload.\n";
    return 0;
}
