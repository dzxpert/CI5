#pragma once

#include <windows.h>
#include <cstdint>
#include <type_traits>

namespace Core {

class Memory {
public:
    static uintptr_t GetModuleBase(const wchar_t* moduleName = nullptr);

    template<typename T>
    static bool SafeRead(uintptr_t address, T* outVal) {
        if (!address || !outVal) return false;
        __try {
            *outVal = *reinterpret_cast<const T*>(address);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    template<typename T>
    static bool SafeWrite(uintptr_t address, const T& val) {
        if (!address) return false;
        __try {
            *reinterpret_cast<T*>(address) = val;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    template<typename T>
    static T Read(uintptr_t address, T defaultVal = T{}) {
        T result = defaultVal;
        SafeRead(address, &result);
        return result;
    }

    template<typename T>
    static bool Write(uintptr_t address, const T& val) {
        return SafeWrite(address, val);
    }

    static bool PatchMemory(void* dest, const void* src, size_t size);
    static bool NopMemory(void* dest, size_t size);
};

} // namespace Core
