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
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace Aimbot {

constexpr float kPi = 3.14159265358979323846f;

struct RuntimeSettings {
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

static RuntimeSettings BuildRuntimeSettings(Globals::AimWeaponGroup group) {
    RuntimeSettings settings;
    settings.enabled = false;
    if (group >= 0 && group < Globals::AIM_GROUP_COUNT) {
        const auto& gs = Globals::aim_weapon_groups[group];
        settings.enabled = gs.enabled;
        settings.fov = std::clamp(gs.fov, 0.0f, 180.0f);
        settings.smoothing = std::clamp(gs.smoothing, 0.0f, 0.99f);
        settings.dynamicSmoothing = gs.dynamicSmoothing;
        settings.smoothingNear = std::clamp(gs.smoothing_near, 0.01f, 0.99f);
        settings.smoothingFar = std::clamp(gs.smoothing_far, 0.01f, 0.99f);
        settings.scopeOnly = gs.scope_only;
        settings.hitboxHead = gs.hitbox_head;
        settings.hitboxNeck = gs.hitbox_neck;
        settings.hitboxChest = gs.hitbox_chest;
        settings.hitboxStomach = gs.hitbox_stomach;
        settings.hitboxPelvis = gs.hitbox_pelvis;
    }
    return settings;
}

static bool IsScopedReliable(uintptr_t pawnAddr, uintptr_t activeWeapon) {
    if (!Utils::IsValidPtr(pawnAddr)) return false;
    if (Utils::SafeRead<bool>(pawnAddr + Offsets::m_bIsScoped)) return true;
    if (Utils::IsValidPtr(activeWeapon)) {
        if (Utils::SafeRead<int>(activeWeapon + Offsets::m_zoomLevel) > 0) return true;
    }
    uintptr_t camServices = Utils::SafeRead<uintptr_t>(pawnAddr + Offsets::m_pCameraServices);
    if (Utils::IsValidPtr(camServices)) {
        int fov = Utils::SafeRead<int>(camServices + Offsets::m_iFOV);
        if (fov > 0 && fov < 89) return true;
    }
    return false;
}

// 随机数生成（性能优化）
static std::random_device rd;
static thread_local std::mt19937 rng(rd());

static float RandFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

// 人机状态
struct HumanState {
    int targetIndex = -1;
    float offsetPitch = 0.f;
    float offsetYaw = 0.f;
    float offsetDecay = 1.f;
    float accumulatedTime = 0.f;
    LARGE_INTEGER lastTime = {};
    float targetOffsetPitch = 0.f;
    float targetOffsetYaw = 0.f;
    float transitionTime = 0.f;
};
static HumanState g_human;

static void ResetHumanState(int newTarget) {
    g_human.targetIndex = newTarget;
    g_human.accumulatedTime = 0.f;
    g_human.offsetDecay = 1.f;
    QueryPerformanceCounter(&g_human.lastTime);
    float angle = RandFloat(0.f, 2.f * kPi);
    float radius = RandFloat(0.3f, 1.0f);
    g_human.offsetPitch = std::sin(angle) * radius;
    g_human.offsetYaw = std::cos(angle) * radius;
    g_human.targetOffsetPitch = g_human.offsetPitch;
    g_human.targetOffsetYaw = g_human.offsetYaw;
    g_human.transitionTime = 0.f;
}

static float GetDeltaTime() {
    LARGE_INTEGER now, freq;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&freq);
    float dt = (float)(now.QuadPart - g_human.lastTime.QuadPart) / (float)freq.QuadPart;
    g_human.lastTime = now;
    return std::clamp(dt, 0.0001f, 0.1f);
}

static Vector CalculateAngle(const Vector& source, const Vector& destination) {
    Vector delta = destination - source;
    float hyp = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    return {
        (float)(std::atan2(-delta.z, hyp) * (180.0f / M_PI)),
        (float)(std::atan2(delta.y, delta.x) * (180.0f / M_PI)),
        0.0f
    };
}

static float GetFov(const Vector& viewAngle, const Vector& aimAngle) {
    Vector delta = aimAngle - viewAngle;
    while (delta.y > 180.f) delta.y -= 360.f;
    while (delta.y < -180.f) delta.y += 360.f;
    return std::sqrt(delta.x * delta.x + delta.y * delta.y);
}

static void NormalizeDelta(Vector& delta) {
    while (delta.y > 180.f) delta.y -= 360.f;
    while (delta.y < -180.f) delta.y += 360.f;
    delta.x = std::clamp(delta.x, -89.f, 89.f);
    delta.z = 0.f;
}

static void NormalizeAngle(Vector& angle) {
    while (angle.y > 180.f) angle.y -= 360.f;
    while (angle.y < -180.f) angle.y += 360.f;
    angle.x = std::clamp(angle.x, -89.f, 89.f);
    angle.z = 0.f;
}

static Vector ApplySmoothing(const Vector& current, const Vector& target, float dt, const RuntimeSettings& settings) {
    Vector delta = target - current;
    NormalizeDelta(delta);

    float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (dist < 0.01f) return target;

    float smooth = settings.smoothing;
    if (settings.dynamicSmoothing && settings.fov > 0.01f) {
        float norm = std::clamp(dist / settings.fov, 0.f, 1.f);
        smooth = settings.smoothingNear * (1.f - norm) + settings.smoothingFar * norm;
    }

    float speed = 1.01f - smooth;

    float curve = 1.0f;
    if (Globals::aim_humanize) {
        float c = std::clamp(Globals::aim_humanize_curve, 0.0f, 1.0f);
        float normDist = std::clamp(dist / std::max(settings.fov, 0.01f), 0.f, 1.f);
        float easeOut = 1.f - std::pow(1.f - normDist, 2.f);
        float easeIn = std::pow(normDist, 2.f);
        curve = easeOut * (1.f - c) + easeIn * c;
        curve = std::clamp(curve, 0.15f, 1.5f);
    }

    float multiplier = (8.f + speed * 52.f) * curve;
    float factor = 1.f - std::exp(-multiplier * dt);
    factor = std::clamp(factor, 0.001f, 1.0f);

    Vector result = current + delta * factor;
    NormalizeAngle(result);
    return result;
}

static Vector ApplyHumanization(const Vector& target, float dt, const RuntimeSettings& settings) {
    if (!Globals::aim_humanize) return target;

    Vector result = target;

    g_human.accumulatedTime += dt;
    float decaySpeed = 1.5f + settings.smoothing * 2.f;
    g_human.offsetDecay = std::exp(-g_human.accumulatedTime * decaySpeed);
    g_human.offsetDecay = std::clamp(g_human.offsetDecay, 0.f, 1.f);

    // 平滑过渡
    g_human.transitionTime += dt;
    float trans = std::clamp(g_human.transitionTime / 0.3f, 0.f, 1.f);
    g_human.offsetPitch += (g_human.targetOffsetPitch - g_human.offsetPitch) * trans * dt * 5.f;
    g_human.offsetYaw   += (g_human.targetOffsetYaw   - g_human.offsetYaw)   * trans * dt * 5.f;

    float strength = std::clamp(Globals::aim_humanize_strength, 0.0f, 2.0f);
    result.x += g_human.offsetPitch * strength * g_human.offsetDecay;
    result.y += g_human.offsetYaw   * strength * g_human.offsetDecay;

    float jitter = std::clamp(Globals::aim_humanize_jitter, 0.0f, 1.0f);
    if (jitter > 0.01f) {
        float scale = g_human.offsetDecay * 0.7f + 0.3f;
        float amount = jitter * 0.15f * scale;
        result.x += RandFloat(-amount, amount);
        result.y += RandFloat(-amount, amount);
    }

    // 随机更新偏移目标
    if (g_human.accumulatedTime > RandFloat(0.5f, 2.0f)) {
        g_human.accumulatedTime = 0.f;
        float angle = RandFloat(0.f, 2.f * kPi);
        float radius = RandFloat(0.2f, 0.8f);
        g_human.targetOffsetPitch = std::sin(angle) * radius;
        g_human.targetOffsetYaw   = std::cos(angle) * radius;
        g_human.transitionTime = 0.f;
    }

    return result;
}

// ========== 统一的可见性检查 ==========
struct VisibilityChecker {
    const Vector& localPos;
    const std::vector<BoneID>& bones;
    uint64_t nowMs;
    std::unordered_map<int, uint64_t>& lastVisible;

    bool Pass(const Entity_t& ent) const {
        if (!Globals::aim_vis_check) return true;
        if (!ent.pawn) return false;
        for (auto bone : bones) {
            Vector pos = Utils::GetBonePos(ent.pawn, bone);
            if (pos.IsZero()) continue;
            if (Raycasting::Get().IsLoaded()) {
                if (Raycasting::Get().IsVisible(localPos, pos)) return true;
            } else {
                if (ent.pawn->IsVisible()) return true;
            }
        }
        return false;
    }

    bool SoftPass(const Entity_t& ent) const {
        if (!Globals::aim_soft_visible) return true;
        if (!ent.pawn) return false;
        int visible = 0;
        for (auto bone : bones) {
            Vector pos = Utils::GetBonePos(ent.pawn, bone);
            if (pos.IsZero()) continue;
            bool vis = false;
            if (Raycasting::Get().IsLoaded())
                vis = Raycasting::Get().IsVisible(localPos, pos);
            else
                vis = ent.pawn->IsVisible();
            if (vis) visible++;
        }
        if (visible > 0) lastVisible[ent.index] = nowMs;
        int minBones = std::max(Globals::aim_soft_visible_min_bones, 1);
        if (visible >= minBones) return true;
        auto it = lastVisible.find(ent.index);
        if (it == lastVisible.end()) return false;
        uint64_t grace = (Globals::aim_soft_visible_grace_ms < 0) ? 0u : (uint64_t)Globals::aim_soft_visible_grace_ms;
        return (nowMs - it->second) <= grace;
    }
};

// ========== 统一的目标搜索器 ==========
struct BestTargetFinder {
    const std::vector<BoneID>& bones;
    float maxFov;
    const Entity_t* localPawn;
    const Vector& localPos;
    const Vector& viewAngles;
    const VisibilityChecker& vis;

    std::pair<const Entity_t*, Vector> Find(const std::vector<Entity_t>& entities) const {
        const Entity_t* best = nullptr;
        Vector bestPos = {};
        float bestFov = maxFov;

        for (const auto& ent : entities) {
            if (!ent.pawn || !Utils::SafeAlive(ent.pawn)) continue;
            if (ent.pawn == localPawn) continue;
            if (!ent.isEnemy && !Globals::target_teammates) continue;
            if (!vis.Pass(ent)) continue;
            if (!vis.SoftPass(ent)) continue;

            for (auto bone : bones) {
                Vector pos = Utils::GetBonePos(ent.pawn, bone);
                if (pos.IsZero()) continue;
                // 可选：添加小偏移
                if (Globals::aim_humanize) {
                    pos.x += RandFloat(-0.3f, 0.3f);
                    pos.y += RandFloat(-0.3f, 0.3f);
                    pos.z += RandFloat(-0.2f, 0.2f);
                }
                Vector angle = CalculateAngle(localPos, pos);
                float fov = GetFov(viewAngles, angle);
                if (fov < bestFov) {
                    bestFov = fov;
                    best = &ent;
                    bestPos = pos;
                }
            }
        }
        return {best, bestPos};
    }
};

// ========== 主循环 ==========
static void RunInternal() {
    Globals::aim_has_target = false;
    Globals::aim_target_index = -1;

    static int lastTargetIndex = -1;
    static std::unordered_map<int, uint64_t> lastVisibleMs;
    static uint64_t lastTargetChangeMs = 0;
    static bool toggleState = false;
    static bool prevDown = false;
    static uint64_t lastRunTime = 0;

    uint64_t nowMs = GetTickCount64();
    if (nowMs - lastRunTime < 1) return;
    lastRunTime = nowMs;

    // 按键处理
    bool keyDown = (Globals::aim_key != 0) && ((GetAsyncKeyState(Globals::aim_key) & 0x8000) != 0);
    bool keyActive = true;
    if (Globals::aim_key_mode == 1) {
        if (Globals::aim_key == 0) toggleState = true;
        else {
            if (keyDown && !prevDown) toggleState = !toggleState;
            prevDown = keyDown;
        }
        keyActive = toggleState;
    } else {
        keyActive = (Globals::aim_key == 0) || keyDown;
        toggleState = false;
        prevDown = keyDown;
    }
    Globals::aim_toggle_state = toggleState;

    if (!Globals::aim_enabled || !keyActive || Menu::IsOpen) {
        lastTargetIndex = -1;
        g_human.targetIndex = -1;
        return;
    }

    uintptr_t client = Memory::GetModuleBase("client.dll");
    if (!client) return;

    auto localPawn = EntityManager::Get().GetLocalPawn();
    if (!localPawn || !Utils::SafeAlive(localPawn)) {
        lastTargetIndex = -1;
        g_human.targetIndex = -1;
        return;
    }

    uintptr_t pawnAddr = reinterpret_cast<uintptr_t>(localPawn);
    if (!Utils::IsValidPtr(pawnAddr)) {
        lastTargetIndex = -1;
        g_human.targetIndex = -1;
        return;
    }

    uintptr_t activeWeapon = Combat::GetActiveWeapon(pawnAddr);
    uint16_t def = Combat::GetWeaponDefinition(activeWeapon);
    Globals::AimWeaponGroup group = Combat::GetWeaponGroup(def);
    RuntimeSettings settings = BuildRuntimeSettings(group);
    if (!settings.enabled) {
        lastTargetIndex = -1;
        g_human.targetIndex = -1;
        return;
    }

    if (!Globals::aim_ignore_flash) {
        float flash = Utils::SafeRead<float>(pawnAddr + Offsets::m_flFlashDuration);
        if (flash > 0.05f) {
            lastTargetIndex = -1;
            g_human.targetIndex = -1;
            return;
        }
    }

    if (settings.scopeOnly && !IsScopedReliable(pawnAddr, activeWeapon)) {
        lastTargetIndex = -1;
        g_human.targetIndex = -1;
        return;
    }

    uintptr_t scene = Utils::SafeRead<uintptr_t>(pawnAddr + Offsets::m_pGameSceneNode);
    if (!Utils::IsValidPtr(scene)) return;

    Vector localOrigin = Utils::SafeRead<Vector>(scene + Offsets::m_vecAbsOrigin);
    Vector viewOffset = Utils::SafeRead<Vector>(pawnAddr + Offsets::m_vecViewOffset);
    Vector localPos = localOrigin + viewOffset;

    uintptr_t viewAddr = client + Offsets::dwViewAngles;
    if (!Utils::IsValidPtr(viewAddr)) return;
    Vector currentAngles = Utils::SafeRead<Vector>(viewAddr);

    // 构建骨骼列表
    std::vector<BoneID> bones;
    if (settings.hitboxHead) bones.push_back(BoneID::Head);
    if (settings.hitboxNeck) bones.push_back(BoneID::Neck);
    if (settings.hitboxChest) bones.push_back(BoneID::Chest);
    if (settings.hitboxStomach) bones.push_back(BoneID::Spine);
    if (settings.hitboxPelvis) bones.push_back(BoneID::Pelvis);
    if (bones.empty()) return;

    float curFov = Globals::visuals_fov_enabled ? Globals::visuals_fov : 90.0f;
    float fovScale = curFov / 90.0f;
    float maxFov = settings.fov * fovScale;

    auto entities = EntityManager::Get().GetEntities();

    // 可见性检查器
    VisibilityChecker vis{localPos, bones, nowMs, lastVisibleMs};

    // 搜索目标
    const Entity_t* best = nullptr;
    Vector bestPos = {};

    // 1. Sticky（严格 FOV）
    if (Globals::aim_sticky_target && lastTargetIndex != -1) {
        BestTargetFinder finder{bones, maxFov, localPawn, localPos, currentAngles, vis};
        auto res = finder.Find(entities);
        if (res.first && res.first->index == lastTargetIndex) {
            best = res.first;
            bestPos = res.second;
        }
    }

    // 2. Persistent（放宽 FOV）
    if (!best && Globals::aim_persistent && lastTargetIndex != -1) {
        BestTargetFinder finder{bones, maxFov * 2.0f, localPawn, localPos, currentAngles, vis};
        auto res = finder.Find(entities);
        if (res.first && res.first->index == lastTargetIndex) {
            best = res.first;
            bestPos = res.second;
        }
    }

    // 3. 普通搜索
    if (!best) {
        BestTargetFinder finder{bones, maxFov, localPawn, localPos, currentAngles, vis};
        auto res = finder.Find(entities);
        best = res.first;
        bestPos = res.second;
    }

    if (best) {
        // 切换延迟
        if (lastTargetIndex != -1 && best->index != lastTargetIndex) {
            uint64_t delay = (Globals::aim_switch_delay_ms < 0) ? 0u : (uint64_t)Globals::aim_switch_delay_ms;
            if (Globals::aim_sticky_target && delay > 0 && lastTargetChangeMs != 0 &&
                (nowMs - lastTargetChangeMs) < delay)
                return;
        }

        if (best->index != lastTargetIndex)
            lastTargetChangeMs = nowMs;

        lastTargetIndex = best->index;
        Globals::aim_has_target = true;
        Globals::aim_target_index = best->index;

        if (g_human.targetIndex != best->index)
            ResetHumanState(best->index);

        float dt = GetDeltaTime();

        Vector raw = CalculateAngle(localPos, bestPos);
        Vector humanized = ApplyHumanization(raw, dt, settings);
        Vector final = ApplySmoothing(currentAngles, humanized, dt, settings);

        // RCS 补偿（统一调用）
        Vector rcs = RCS::GetAimCompensation(pawnAddr, group);
        final.x += rcs.x;
        final.y += rcs.y;
        NormalizeAngle(final);

        if (std::isfinite(final.x) && std::isfinite(final.y) && std::isfinite(final.z))
            Utils::SafeWrite<Vector>(viewAddr, final);
    } else {
        lastTargetIndex = -1;
        lastTargetChangeMs = 0;
        g_human.targetIndex = -1;
    }
}

void Run() {
    __try {
        RunInternal();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Globals::aim_has_target = false;
        Globals::aim_target_index = -1;
    }
}

} // namespace Aimbot
