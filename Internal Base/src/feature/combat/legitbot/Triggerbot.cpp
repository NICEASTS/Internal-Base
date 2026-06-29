#include "Triggerbot.h"
#include "../../../sdk/utils/Globals.h"
#include "../../../sdk/memory/Offsets.h"
#include "../../../sdk/utils/Utils.h"
#include "../../../sdk/utils/Raycasting.h"
#include "../../../sdk/memory/PatternScan.h"
#include "../../../sdk/entity/EntityManager.h"
#include "../../../menu/Menu.h"
#include <Windows.h>
#include <thread>
#include <chrono>
#include <random>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace Triggerbot {

static bool IsScopedReliable(uintptr_t pawnAddr)
{
    bool scopedFlag = Utils::SafeRead<bool>(pawnAddr + Offsets::m_bIsScoped);
    if (scopedFlag)
        return true;

    int zoomLevel = Utils::SafeRead<int>(pawnAddr + Offsets::m_zoomLevel);
    if (zoomLevel > 0)
        return true;

    uintptr_t cameraServices = Utils::SafeRead<uintptr_t>(pawnAddr + Offsets::m_pCameraServices);
    if (Utils::IsValidPtr(cameraServices))
    {
        int fov = Utils::SafeRead<int>(cameraServices + Offsets::m_iFOV);
        if (fov > 0 && fov < 89)
            return true;
    }

    return false;
}

static Vector CalculateAngle(const Vector& source, const Vector& destination)
{
    Vector angle;
    Vector delta = destination - source;
    float hyp = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    angle.x = (float)(std::atan2(-delta.z, hyp) * (180.0f / M_PI));
    angle.y = (float)(std::atan2(delta.y, delta.x) * (180.0f / M_PI));
    angle.z = 0.0f;
    return angle;
}

static float GetFov(const Vector& viewAngle, const Vector& aimAngle)
{
    Vector delta = aimAngle - viewAngle;
    while (delta.y > 180) delta.y -= 360;
    while (delta.y < -180) delta.y += 360;
    return (float)std::sqrt(std::pow(delta.x, 2) + std::pow(delta.y, 2));
}

static bool RayHitsSphere(const Vector& rayOrigin, const Vector& rayDir, const Vector& sphereCenter, float radius)
{
    Vector L = sphereCenter - rayOrigin;
    float tca = L.x * rayDir.x + L.y * rayDir.y + L.z * rayDir.z;
    if (tca < 0) return false; 

    
    float l2 = L.x * L.x + L.y * L.y + L.z * L.z;
    float d2 = l2 - tca * tca;
    
    return d2 <= (radius * radius);
}



void Run()
{
    static bool toggleState = false;
    static bool prevDown = false;

    const bool keyDown = (Globals::trigger_key != 0) && ((GetAsyncKeyState(Globals::trigger_key) & 0x8000) != 0);
    bool keyActive = true;
    if (Globals::trigger_key_mode == 1) {
        if (Globals::trigger_key == 0) {
            toggleState = true;
        } else {
            if (keyDown && !prevDown) toggleState = !toggleState;
            prevDown = keyDown;
        }
        keyActive = toggleState;
    } else {
        if (Globals::trigger_key == 0) keyActive = true;
        else keyActive = keyDown;
        toggleState = false;
        prevDown = keyDown;
    }
    Globals::trigger_toggle_state = toggleState;

    if (!Globals::trigger_enabled || !keyActive) return;

    
    if (Menu::IsOpen) return;

    auto localPawn = EntityManager::Get().GetLocalPawn();
    if (!localPawn || !Utils::SafeAlive(localPawn)) return;

    if (Globals::trigger_ignore_flash)
    {
        float flashDuration = Utils::SafeRead<float>(reinterpret_cast<uintptr_t>(localPawn) + Offsets::m_flFlashDuration);
        if (flashDuration > 0.05f)
            return;
    }

    // Scope Only: if enabled and using a sniper, require scope to be active
    if (Globals::trigger_scope_only)
    {
        bool isScoped = IsScopedReliable(reinterpret_cast<uintptr_t>(localPawn));
        if (!isScoped)
            return;
    }

    uintptr_t scene = Utils::SafeRead<uintptr_t>(reinterpret_cast<uintptr_t>(localPawn) + Offsets::m_pGameSceneNode);
    Vector localOrigin = Utils::SafeRead<Vector>(scene + Offsets::m_vecAbsOrigin);
    Vector viewOffset = Utils::SafeRead<Vector>(reinterpret_cast<uintptr_t>(localPawn) + Offsets::m_vecViewOffset);
    Vector localPos = localOrigin + viewOffset;

    bool shouldShoot = false;

    
    if (Globals::trigger_fov <= 0.01f)
    {
        int id = Utils::SafeRead<int>(reinterpret_cast<uintptr_t>(localPawn) + Offsets::m_iIDEntIndex);
        if (id > 0)
        {
            auto entities = EntityManager::Get().GetEntities();
            for (const auto& entity : entities)
            {
                if (entity.index == id && entity.pawn && (entity.isEnemy || Globals::target_teammates) && Utils::SafeAlive(entity.pawn) && entity.pawn != localPawn)
                {
                    shouldShoot = true;
                    break;
                }
            }
        }
    }
    
    else
    {
        uintptr_t client = Memory::GetModuleBase("client.dll");
        if (!client) return;

        uintptr_t viewAnglesAddr = client + Offsets::dwViewAngles;
        Vector currentAngles = Utils::SafeRead<Vector>(viewAnglesAddr);

        auto entities = EntityManager::Get().GetEntities();
        
        std::vector<std::pair<BoneID, float>> activeBones;
        if (Globals::trigger_hitbox_head) activeBones.push_back({BoneID::Head, 5.5f});
        if (Globals::trigger_hitbox_neck) activeBones.push_back({BoneID::Neck, 4.5f});
        if (Globals::trigger_hitbox_chest) activeBones.push_back({BoneID::Spine, 8.5f});
        if (Globals::trigger_hitbox_stomach) activeBones.push_back({BoneID::Spine, 8.5f});
        if (Globals::trigger_hitbox_pelvis) activeBones.push_back({BoneID::Pelvis, 8.5f});

        Vector rayDirection;
        Utils::AngleVectors(currentAngles, rayDirection);

        for (const auto& entity : entities)
        {
            if (!entity.pawn || !(entity.isEnemy || Globals::target_teammates) || !Utils::SafeAlive(entity.pawn)) continue;
            if (entity.pawn == localPawn) continue; 

            for (auto& pair : activeBones) {
                BoneID bone = pair.first;
                float radius = pair.second;
                
                Vector targetBonePos = Utils::GetBonePos(entity.pawn, bone);
                if (!targetBonePos.IsZero())
                {
                    
                    
                    if (RayHitsSphere(localPos, rayDirection, targetBonePos, radius))
                    {
                        
                        Vector angle = CalculateAngle(localPos, targetBonePos);
                        if (GetFov(currentAngles, angle) <= Globals::trigger_fov)
                        {
                            shouldShoot = true;
                            break;
                        }
                    }
                }
            }
            if (shouldShoot) break;
        }
    }

    static auto firstSeenTime = std::chrono::high_resolution_clock::now();
    static bool wasSeen = false;
    static auto lastShotTime = std::chrono::high_resolution_clock::now();
    static bool isHolding = false;

    auto now = std::chrono::high_resolution_clock::now();

    if (shouldShoot)
    {
        if (!wasSeen) {
            firstSeenTime = now;
            wasSeen = true;
        }

        auto elapsedSeen = std::chrono::duration_cast<std::chrono::milliseconds>(now - firstSeenTime).count();
        auto elapsedShot = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastShotTime).count();

        // Calculate effective delay (base + optional random)
        float effectiveDelay = Globals::trigger_delay;
        if (Globals::trigger_random_delay && Globals::trigger_max_delay > Globals::trigger_min_delay)
        {
            static std::mt19937 rng(static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
            std::uniform_int_distribution<int> dist(Globals::trigger_min_delay, Globals::trigger_max_delay);
            effectiveDelay += static_cast<float>(dist(rng));
        }
        
        if (elapsedSeen >= effectiveDelay && elapsedShot >= 175 && !isHolding)
        {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            isHolding = true;
            lastShotTime = now; 
        }
    }
    else
    {
        wasSeen = false;
    }

    
    if (isHolding)
    {
        auto elapsedHold = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastShotTime).count();
        if (elapsedHold >= 25)
        {
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            isHolding = false;
        }
    }
}

}
