#pragma once
#include <cstdint>

namespace SkinChanger
{
    void Setup();
    void InstallHooks();
    void Run();
    
    
    
    
    
    void RunVisuals(int frameStage);

    inline bool g_Initialized = false;
    
    
    inline uintptr_t g_PendingKnifeWeapon = 0;
    inline int g_PendingKnifeIndex = 0;

    
    inline uintptr_t g_PendingGlovePawn = 0;
    inline uintptr_t g_PendingGloveWeapons[64] = {};
    inline int g_PendingGloveWeaponCount = 0;
    inline bool g_PendingGloveApply = false;

    
    inline volatile int g_MeshUpdateFrames = 0;

    
    inline uintptr_t g_PendingAgentPawn = 0;
    inline char g_PendingAgentModel[256] = {};
    inline bool g_PendingAgentApply = false;

    
    
    using fnSetModel_t = void(__fastcall*)(void* pModelEntity, const char* szModelPath);
    inline fnSetModel_t fnSetModel = nullptr;
}
