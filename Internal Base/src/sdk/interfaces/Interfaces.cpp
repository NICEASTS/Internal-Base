#include "Interfaces.h"
#include "../memory/PatternScan.h"
#include "../utils/Utils.h"
#include <Windows.h>

void Interfaces::Setup()
{
    __try {
        uintptr_t client = Memory::GetModuleBase("client.dll");
        uintptr_t scenesystem = Memory::GetModuleBase("scenesystem.dll");

        if (client) {
            uintptr_t traceManagerPtr = Memory::PatternScan("client.dll", "48 8B 0D ? ? ? ? 48 8B 01 FF 90 ? ? ? ?");
            if (traceManagerPtr)
                m_pTraceManager = *reinterpret_cast<void**>(Utils::ResolveRelativeAddress(traceManagerPtr, 3, 7));
            
            fnTraceShape = reinterpret_cast<void*>(Memory::PatternScan("client.dll", "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 30 41 8B D9 49 8B F8"));
            fnInitEntitiesOnly = reinterpret_cast<void*>(Memory::PatternScan("client.dll", "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 30 48 8B F9 48 8D 4C 24 ? E8 ? ? ? ? 48 8B 5C 24 ? 48 8B B4 24 ? ? ? ?"));
            fnInitStandard = reinterpret_cast<void*>(Memory::PatternScan("client.dll", "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 30 48 8B F1 48 8D 4D 20"));
            
            
            HMODULE hClient = GetModuleHandleA("client.dll");
            if (hClient) {
                typedef void* (*CreateInterfaceFn)(const char*, int*);
                auto pCreateInterface = (CreateInterfaceFn)GetProcAddress(hClient, "CreateInterface");
                if (pCreateInterface) {
                    
                    m_pIClient = pCreateInterface("Source2Client002", nullptr);
                    if (!m_pIClient) {
                        
                        m_pIClient = pCreateInterface("Source2Client001", nullptr);
                    }

                    
                    m_pLegacyGameUI = reinterpret_cast<ILegacyGameUI*>(
                        pCreateInterface("LegacyGameUI001", nullptr));
                }
            }
        }

        if (scenesystem) {
            uintptr_t sceneSystemPtr = Memory::PatternScan("scenesystem.dll", "48 8B 05 ? ? ? ? 48 8B 5C 24 ? 48 83 C4 20 5F C3");
            if (sceneSystemPtr)
                m_pSceneSystem = reinterpret_cast<ISceneSystem*>(*reinterpret_cast<void**>(Utils::ResolveRelativeAddress(sceneSystemPtr, 3, 7)));
        }

        
        {
            HMODULE hEngine = GetModuleHandleA("engine2.dll");
            if (hEngine) {
                typedef void* (*CreateInterfaceFn)(const char*, int*);
                auto pCreateInterface = (CreateInterfaceFn)GetProcAddress(hEngine, "CreateInterface");
                if (pCreateInterface) {
                    m_pEngine = reinterpret_cast<IVEngine*>(
                        pCreateInterface("Source2EngineToClient001", nullptr));
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}
