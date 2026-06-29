#pragma once
#include <cstdint>
#include "../classes/SceneSystem.h"


class ILegacyGameUI {
public:
    void show_message_box(const char* title, const char* message,
        bool show_ok = true, bool show_cancel = false,
        const char* ok_command = nullptr, const char* cancel_command = nullptr,
        const char* closed_command = nullptr, const char* legend = nullptr,
        const char* unknown = nullptr)
    {
        
        using fn = void(__fastcall*)(void*, const char*, const char*,
            bool, bool, const char*, const char*, const char*, const char*, const char*);
        void** vtable = *reinterpret_cast<void***>(this);
        reinterpret_cast<fn>(vtable[28])(this, title, message,
            show_ok, show_cancel, ok_command, cancel_command,
            closed_command, legend, unknown);
    }
};

namespace Interfaces
{
    inline void* m_pTraceManager = nullptr;
    inline void* fnTraceShape = nullptr;
    inline void* fnInitEntitiesOnly = nullptr;
    inline void* fnInitStandard = nullptr;
    inline ISceneSystem* m_pSceneSystem = nullptr;
    inline void* m_pIClient = nullptr; 
    inline ILegacyGameUI* m_pLegacyGameUI = nullptr;

    
    class IVEngine {
    public:
        void ExecuteClientCMD(const char* cmd) {
            using fn = void(__fastcall*)(void*, int, const char*, int);
            void** vtable = *reinterpret_cast<void***>(this);
            reinterpret_cast<fn>(vtable[50])(this, 0, cmd, 0x7FFEF001);
        }
    };

    inline IVEngine* m_pEngine = nullptr;

    void Setup();
}
