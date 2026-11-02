#include "memory.h"

namespace Core {

uintptr_t Memory::GetModuleBase(const wchar_t* moduleName) {
    return reinterpret_cast<uintptr_t>(GetModuleHandleW(moduleName));
}

bool Memory::PatchMemory(void* dest, const void* src, size_t size) {
    if (!dest || !src || size == 0) return false;
    
    DWORD oldProtect = 0;
    if (!VirtualProtect(dest, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    __try {
        memcpy(dest, src, size);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        VirtualProtect(dest, size, oldProtect, &oldProtect);
        return false;
    }

    VirtualProtect(dest, size, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), dest, size);
    return true;
}

bool Memory::NopMemory(void* dest, size_t size) {
    if (!dest || size == 0) return false;

    DWORD oldProtect = 0;
    if (!VirtualProtect(dest, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    __try {
        memset(dest, 0x90, size);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        VirtualProtect(dest, size, oldProtect, &oldProtect);
        return false;
    }

    VirtualProtect(dest, size, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), dest, size);
    return true;
}

} // namespace Core
