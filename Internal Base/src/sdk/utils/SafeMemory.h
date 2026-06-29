#pragma once
#include <Windows.h>
#include <cstdint>

namespace SafeMemory
{
    inline bool IsValidPtr(uintptr_t addr)
    {
        
        return (addr >= 0x10000 && addr < 0x00007FFFFFFFFFFF) && (addr % 1 == 0);
    }

    template <typename T>
    inline T Read(uintptr_t addr, T defaultVal = T())
    {
        if (!IsValidPtr(addr))
            return defaultVal;

        __try
        {
            return *reinterpret_cast<T*>(addr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return defaultVal;
        }
    }

    template <typename T>
    inline bool Write(uintptr_t addr, T val)
    {
        if (!IsValidPtr(addr))
            return false;

        __try
        {
            *reinterpret_cast<T*>(addr) = val;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    template <typename T>
    inline bool WriteProtected(uintptr_t addr, T val)
    {
        if (!IsValidPtr(addr))
            return false;

        DWORD oldProtect;
        if (VirtualProtect(reinterpret_cast<void*>(addr), sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            __try
            {
                *reinterpret_cast<T*>(addr) = val;
                VirtualProtect(reinterpret_cast<void*>(addr), sizeof(T), oldProtect, &oldProtect);
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                VirtualProtect(reinterpret_cast<void*>(addr), sizeof(T), oldProtect, &oldProtect);
                return false;
            }
        }
        return false;
    }
}
