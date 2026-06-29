#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "Aimbot.h"
#include "RCS.h"
#include "../Combat.h"
#include "../../../sdk/utils/Globals.h"
#include "../../../sdk/memory/Offsets.h"
#include "../../../sdk/memory/PatternScan.h"
#include "../../../sdk/utils/Utils.h"
#include "../../../sdk/entity/EntityManager.h"
#include "../../../sdk/utils/CCSGOInput.h"
#include "../../../sdk/interfaces/Interfaces.h"
#include "../../../sdk/classes/SceneSystem.h"
#include "../../../sdk/utils/Raycasting.h"
#include "../../../menu/Menu.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <chrono>
#include <unordered_map>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace Aimbot {

constexpr float kPi = 3.14159265358979323846f;

struct RuntimeSettings
{
    bool enabled = true;
    float fov = 7.0f;
    float smoothing = 0.4f;
    bool dynamicSmoothing = true;
    float smoothingNear = 0.16f;
    float smoothingFar = 0.30f;
    bool scopeOnly = false;
    bool hitboxHead = true;
    bool hitboxNeck = false;
    bool hitboxChest = false;
    bool hitboxStomach = false;
    bool hitboxPelvis = false;
};

static RuntimeSettings BuildRuntimeSettings(Globals::AimWeaponGroup group)
{
    RuntimeSettings settings;
    settings.enabled = false;

    if (group >= 0 && group < Globals::AIM_GROUP_COUNT)
    {
        const auto& groupSettings = Globals::aim_weapon_groups[group];
        settings.enabled = groupSettings.enabled;
        settings.fov = groupSettings.fov;
        settings.smoothing = groupSettings.smoothing;
        settings.dynamicSmoothing = groupSettings.dynamic_smoothing;
        settings.smoothingNear = groupSettings.smoothing_near;
        settings.smoothingFar = groupSettings.smoothing_far;
        settings.scopeOnly = groupSettings.scope_only;
        settings.hitboxHead = groupSettings.hitbox_head;
        settings.hitboxNeck = groupSettings.hitbox_neck;
        settings.hitboxChest = groupSettings.hitbox_chest;
        settings.hitboxStomach = groupSettings.hitbox_stomach;
        settings.hitboxPelvis = groupSettings.hitbox_pelvis;
    }

    return settings;
}

static bool IsScopedReliable(uintptr_t pawnAddr, uintptr_t activeWeapon)
{
    bool scopedFlag = Utils::SafeRead<bool>(pawnAddr + Offsets::m_bIsScoped);
    if (scopedFlag)
        return true;

    if (Utils::IsValidPtr(activeWeapon))
    {
        int zoomLevel = Utils::SafeRead<int>(activeWeapon + Offsets::m_zoomLevel);
        if (zoomLevel > 0)
            return true;
    }

    uintptr_t cameraServices = Utils::SafeRead<uintptr_t>(pawnAddr + Offsets::m_pCameraServices);
    if (Utils::IsValidPtr(cameraServices))
    {
        int fov = Utils::SafeRead<int>(cameraServices + Offsets::m_iFOV);
        if (fov > 0 && fov < 89)
            return true;
    }

    return false;
}




static float RandFloat(float min, float max)
{
    static thread_local std::mt19937 rng(
        (unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}




struct HumanState
{
    int   targetIndex = -1;
    float offsetPitch = 0.f;          
    float offsetYaw   = 0.f;
    float offsetDecay = 1.f;          
    float accumulatedTime = 0.f;      
    LARGE_INTEGER lastTime = {};      
};
static HumanState g_human;

static void ResetHumanState(int newTarget)
{
    g_human.targetIndex = newTarget;
    g_human.accumulatedTime = 0.f;
    g_human.offsetDecay = 1.f;
    QueryPerformanceCounter(&g_human.lastTime);

    
    float angle = RandFloat(0.f, 2.f * kPi);
    float radius = RandFloat(0.3f, 1.0f); 
    g_human.offsetPitch = std::sin(angle) * radius;
    g_human.offsetYaw   = std::cos(angle) * radius;
}

static float GetDeltaTime()
{
    LARGE_INTEGER now, freq;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&freq);
    float dt = (float)(now.QuadPart - g_human.lastTime.QuadPart) / (float)freq.QuadPart;
    g_human.lastTime = now;
    return std::clamp(dt, 0.0001f, 0.1f); 
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




static void NormalizeDelta(Vector& delta)
{
    while (delta.y > 180.f)  delta.y -= 360.f;
    while (delta.y < -180.f) delta.y += 360.f;
    delta.x = std::clamp(delta.x, -89.f, 89.f);
    delta.z = 0.f;
}

static void NormalizeAngle(Vector& angle)
{
    while (angle.y > 180.f)  angle.y -= 360.f;
    while (angle.y < -180.f) angle.y += 360.f;
    angle.x = std::clamp(angle.x, -89.f, 89.f);
    angle.z = 0.f;
}




static Vector ApplySmoothing(const Vector& currentAngles, const Vector& targetAngle, float dt, const RuntimeSettings& settings)
{
    Vector delta = targetAngle - currentAngles;
    NormalizeDelta(delta);

    float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (distance < 0.01f)
        return targetAngle; 

    
    
    
    float effectiveSmoothing = settings.smoothing;
    if (settings.dynamicSmoothing && settings.fov > 0.01f) {
        float distNorm = std::clamp(distance / settings.fov, 0.f, 1.f);
        effectiveSmoothing = settings.smoothingNear * (1.f - distNorm) + settings.smoothingFar * distNorm;
    }
    
    float speed = 1.01f - effectiveSmoothing; 

    
    
    
    float normalizedDist = std::clamp(distance / std::max(settings.fov, 0.01f), 0.f, 1.f);

    float curveFactor = 1.0f;
    if (Globals::aim_humanize)
    {
        float curve = Globals::aim_humanize_curve;
        
        float easeOut = 1.f - std::pow(1.f - normalizedDist, 2.f); 
        float easeIn  = std::pow(normalizedDist, 2.f);              
        curveFactor = easeOut * (1.f - curve) + easeIn * curve;
        curveFactor = std::clamp(curveFactor, 0.15f, 1.5f);
    }

    
    
    
    float multiplier = 8.f + speed * 52.f; 
    multiplier *= curveFactor;
    float factor = 1.f - std::exp(-multiplier * dt);
    factor = std::clamp(factor, 0.001f, 1.0f);

    Vector result;
    result.x = currentAngles.x + delta.x * factor;
    result.y = currentAngles.y + delta.y * factor;
    result.z = 0.f;
    NormalizeAngle(result);

    return result;
}

static Vector ApplyHumanization(const Vector& targetAngle, float dt, const RuntimeSettings& settings)
{
    if (!Globals::aim_humanize)
        return targetAngle;

    Vector result = targetAngle;

    
    
    g_human.accumulatedTime += dt;
    float decaySpeed = 1.5f + settings.smoothing * 2.f;
    g_human.offsetDecay = std::exp(-g_human.accumulatedTime * decaySpeed);
    g_human.offsetDecay = std::clamp(g_human.offsetDecay, 0.f, 1.f);

    float strength = Globals::aim_humanize_strength;
    result.x += g_human.offsetPitch * strength * g_human.offsetDecay;
    result.y += g_human.offsetYaw   * strength * g_human.offsetDecay;

    
    float jitter = Globals::aim_humanize_jitter;
    if (jitter > 0.01f)
    {
        
        float jitterScale = g_human.offsetDecay * 0.7f + 0.3f;
        result.x += RandFloat(-jitter, jitter) * jitterScale * 0.15f;
        result.y += RandFloat(-jitter, jitter) * jitterScale * 0.15f;
    }

    return result;
}




static void RunInternal()
{
    Globals::aim_has_target = false;
    Globals::aim_target_index = -1;

    static int lastTargetIndex = -1;
    static std::unordered_map<int, uint64_t> lastVisibleMs;
    static uint64_t lastTargetChangeMs = 0;
    static bool toggleState = false;
    static bool prevDown = false;

    const bool keyDown = (Globals::aim_key != 0) && ((GetAsyncKeyState(Globals::aim_key) & 0x8000) != 0);
    bool keyActive = true;
    if (Globals::aim_key_mode == 1) {
        if (Globals::aim_key == 0) {
            toggleState = true;
        } else {
            if (keyDown && !prevDown) toggleState = !toggleState;
            prevDown = keyDown;
        }
        keyActive = toggleState;
    } else {
        if (Globals::aim_key == 0) keyActive = true;
        else keyActive = keyDown;
        toggleState = false;
        prevDown = keyDown;
    }
    Globals::aim_toggle_state = toggleState;

    if (!Globals::aim_enabled || !keyActive)
    {
        lastTargetIndex = -1;
        g_human.targetIndex = -1;
        return;
    }

    
    if (Menu::IsOpen) return;

    uintptr_t client = Memory::GetModuleBase("client.dll");
    if (!client) return;

    auto localPawn = EntityManager::Get().GetLocalPawn();
    if (!localPawn || !Utils::SafeAlive(localPawn))
    {
        lastTargetIndex = -1;
        g_human.targetIndex = -1;
        return;
    }

    uintptr_t activeWeapon = Combat::GetActiveWeapon(reinterpret_cast<uintptr_t>(localPawn));
    uint16_t activeWeaponDef = Combat::GetWeaponDefinition(activeWeapon);
    Globals::AimWeaponGroup activeGroup = Combat::GetWeaponGroup(activeWeaponDef);
    RuntimeSettings settings = BuildRuntimeSettings(activeGroup);
    if (!settings.enabled)
    {
        lastTargetIndex = -1;
        g_human.targetIndex = -1;
        return;
    }

    if (!Globals::aim_ignore_flash)
    {
        float flashDuration = Utils::SafeRead<float>(reinterpret_cast<uintptr_t>(localPawn) + Offsets::m_flFlashDuration);
        if (flashDuration > 0.05f)
        {
            lastTargetIndex = -1;
            g_human.targetIndex = -1;
            return;
        }
    }

    if (settings.scopeOnly)
    {
        bool isScoped = IsScopedReliable(reinterpret_cast<uintptr_t>(localPawn), activeWeapon);
        if (!isScoped)
        {
            lastTargetIndex = -1;
            g_human.targetIndex = -1;
            return;
        }
    }

    uintptr_t scene = Utils::SafeRead<uintptr_t>(reinterpret_cast<uintptr_t>(localPawn) + Offsets::m_pGameSceneNode);
    if (!Utils::IsValidPtr(scene)) return;

    Vector localOrigin = Utils::SafeRead<Vector>(scene + Offsets::m_vecAbsOrigin);
    Vector viewOffset = Utils::SafeRead<Vector>(reinterpret_cast<uintptr_t>(localPawn) + Offsets::m_vecViewOffset);
    Vector localPos = localOrigin + viewOffset;

    uintptr_t viewAnglesAddr = client + Offsets::dwViewAngles;
    if (!Utils::IsValidPtr(viewAnglesAddr)) return;

    Vector currentAngles = Utils::SafeRead<Vector>(viewAnglesAddr);

    const Entity_t* bestTarget = nullptr;
    Vector bestTargetPos;

    auto entities = EntityManager::Get().GetEntities();

    float currentFov = Globals::visuals_fov_enabled ? Globals::visuals_fov : 90.0f;
    float fovScale = currentFov / 90.0f;
    float scaledAimFov = settings.fov * fovScale;

    
    std::vector<BoneID> activeBones;
    if (settings.hitboxHead) activeBones.push_back(BoneID::Head);
    if (settings.hitboxNeck) activeBones.push_back(BoneID::Neck);
    if (settings.hitboxChest) activeBones.push_back(BoneID::Chest);
    if (settings.hitboxStomach) activeBones.push_back(BoneID::Spine);
    if (settings.hitboxPelvis) activeBones.push_back(BoneID::Pelvis);

    if (activeBones.empty()) return; 

    auto nowMs = GetTickCount64();
    
    // Visibility check - uses raycasting if available, otherwise engine fallback
    auto visCheckPass = [&](const Entity_t& entity) -> bool {
        if (!Globals::aim_vis_check) return true;
        if (!entity.pawn) return false;

        for (BoneID bone : activeBones) {
            Vector bp = Utils::GetBonePos(entity.pawn, bone);
            if (bp.IsZero()) continue;

            // Primary: raycasting (most accurate, requires .tri map files)
            if (Raycasting::Get().IsLoaded()) {
                if (Raycasting::Get().IsVisible(localPos, bp))
                    return true;
            }
            // Fallback: engine visibility (less accurate, may miss some walls)
            else if (entity.pawn->IsVisible()) {
                return true;
            }
        }
        return false;
    };
    
    auto softVisiblePass = [&](const Entity_t& entity) -> bool {
        if (!Globals::aim_soft_visible) return true;
        if (!entity.pawn) return false;

        int visibleBones = 0;
        for (BoneID bone : activeBones) {
            Vector bp = Utils::GetBonePos(entity.pawn, bone);
            if (bp.IsZero()) continue;
            if (Raycasting::Get().IsLoaded()) {
                if (Raycasting::Get().IsVisible(localPos, bp)) visibleBones++;
            } else {
                if (entity.pawn->IsVisible()) visibleBones++;
            }
        }

        if (visibleBones > 0) lastVisibleMs[entity.index] = nowMs;

        int minBones = Globals::aim_soft_visible_min_bones;
        if (minBones < 1) minBones = 1;
        if (visibleBones >= minBones) return true;

        auto it = lastVisibleMs.find(entity.index);
        if (it == lastVisibleMs.end()) return false;
        uint64_t grace = (Globals::aim_soft_visible_grace_ms < 0) ? 0u : (uint64_t)Globals::aim_soft_visible_grace_ms;
        return (nowMs - it->second) <= grace;
    };

    
    // Sticky target: keep current target if still valid (within FOV and visible)
    if (Globals::aim_sticky_target && lastTargetIndex != -1 && !bestTarget)
    {
        for (const auto& entity : entities)
        {
            if (entity.index == lastTargetIndex && entity.pawn && Utils::SafeAlive(entity.pawn) && (entity.isEnemy || Globals::target_teammates))
            {
                if (entity.pawn == localPawn) continue;
                if (!visCheckPass(entity)) continue;
                if (!softVisiblePass(entity)) continue;
                
                float bestFovForEntity = scaledAimFov;
                for (BoneID bone : activeBones) {
                    Vector bonePos = Utils::GetBonePos(entity.pawn, bone);
                    if (!bonePos.IsZero())
                    {
                        Vector angle = CalculateAngle(localPos, bonePos);
                        float fov = GetFov(currentAngles, angle);
                        if (fov < bestFovForEntity)
                        {
                            bestFovForEntity = fov;
                            bestTarget = &entity;
                            bestTargetPos = bonePos;
                        }
                    }
                }
                break;
            }
        }
    }
    
    if (Globals::aim_persistent && lastTargetIndex != -1 && !bestTarget)
    {
        for (const auto& entity : entities)
        {
            if (entity.index == lastTargetIndex && entity.pawn && Utils::SafeAlive(entity.pawn) && (entity.isEnemy || Globals::target_teammates))
            {
            if (entity.pawn == localPawn) continue; 
                if (!visCheckPass(entity)) continue;
                if (!softVisiblePass(entity)) continue;
                float bestFovForEntity = scaledAimFov * 2.0f; 
                bool foundBone = false;

                for (BoneID bone : activeBones) {
                    Vector bonePos = Utils::GetBonePos(entity.pawn, bone);
                    if (!bonePos.IsZero())
                    {
                        Vector angle = CalculateAngle(localPos, bonePos);
                        float fov = GetFov(currentAngles, angle);

                        if (fov < bestFovForEntity) 
                        {
                            bestFovForEntity = fov;
                            bestTarget = &entity;
                            bestTargetPos = bonePos;
                            foundBone = true;
                        }
                    }
                }
                if (foundBone) break; 
            }
        }
    }

    
    if (!bestTarget)
    {
        float bestFov = scaledAimFov;
        for (const auto& entity : entities)
        {
            if (!entity.pawn || !(entity.isEnemy || Globals::target_teammates) || !Utils::SafeAlive(entity.pawn)) continue;
            if (entity.pawn == localPawn) continue;
            if (!visCheckPass(entity)) continue;
            if (!softVisiblePass(entity)) continue;

            for (BoneID bone : activeBones) {
                Vector bonePos = Utils::GetBonePos(entity.pawn, bone);
                if (bonePos.IsZero()) continue;

                
                Vector angle = CalculateAngle(localPos, bonePos);
                float fov = GetFov(currentAngles, angle);

                if (fov < bestFov)
                {
                    bestFov = fov;
                    bestTarget = &entity;
                    bestTargetPos = bonePos;
                }
            }
        }
    }

    if (bestTarget)
    {
        if (lastTargetIndex != -1 && bestTarget->index != lastTargetIndex)
        {
            const uint64_t switchDelay = (Globals::aim_switch_delay_ms < 0) ? 0u : static_cast<uint64_t>(Globals::aim_switch_delay_ms);
            if (Globals::aim_sticky_target && switchDelay > 0 && lastTargetChangeMs != 0 && (nowMs - lastTargetChangeMs) < switchDelay)
                return;
        }

        if (bestTarget->index != lastTargetIndex)
            lastTargetChangeMs = nowMs;

        lastTargetIndex = bestTarget->index;
        Globals::aim_has_target = true;
        Globals::aim_target_index = bestTarget->index;

        
        if (g_human.targetIndex != bestTarget->index)
        {
            ResetHumanState(bestTarget->index);
        }

        float dt = GetDeltaTime();

        
        Vector rawAngle = CalculateAngle(localPos, bestTargetPos);

        
        Vector humanizedAngle = ApplyHumanization(rawAngle, dt, settings);

        
        Vector finalAngle = ApplySmoothing(currentAngles, humanizedAngle, dt, settings);
        finalAngle += RCS::GetAimCompensation(reinterpret_cast<uintptr_t>(localPawn), activeGroup);

        
        NormalizeAngle(finalAngle);

        Utils::SafeWrite<Vector>(viewAnglesAddr, finalAngle);
    }
    else
    {
        lastTargetIndex = -1;
        lastTargetChangeMs = 0;
        g_human.targetIndex = -1;
    }
}

void Run()
{
    __try
    {
        RunInternal();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

} 
