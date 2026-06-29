#include "RCS.h"
#include "../Combat.h"
#include "../../../sdk/utils/Globals.h"
#include "../../../sdk/memory/Offsets.h"
#include "../../../sdk/utils/Utils.h"
#include "../../../sdk/entity/EntityManager.h"
#include "../../../menu/Menu.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <random>

namespace RCS {

struct RuntimeSettings {
    bool enabled = false;
    int mode = 0;
    float horizontal = 35.0f;
    float vertical = 45.0f;
};

// 状态
static Vector lastPunch = {0, 0, 0};
static uint32_t lastWeaponHash = 0;
static float pendingMoveX = 0.0f;
static float pendingMoveY = 0.0f;
static int lastShotsFired = 0;
static bool initialized = false;

// 随机数（抖动用）
static std::random_device rd;
static std::mt19937 rng(rd());
static std::uniform_real_distribution<float> jitterDist(-0.5f, 0.5f);

static RuntimeSettings BuildRuntimeSettings(Globals::AimWeaponGroup group) {
    RuntimeSettings settings;
    if (group >= 0 && group < Globals::AIM_GROUP_COUNT) {
        const auto& gs = Globals::rcs_weapon_groups[group];
        settings.enabled = gs.enabled;
        settings.mode = std::clamp(gs.mode, 0, 1);
        settings.horizontal = std::clamp(gs.horizontal, 0.0f, 150.0f);
        settings.vertical = std::clamp(gs.vertical, 0.0f, 150.0f);
    }
    return settings;
}

static void ResetState() {
    lastPunch = {0, 0, 0};
    pendingMoveX = 0.0f;
    pendingMoveY = 0.0f;
    lastShotsFired = 0;
    initialized = false;
}

static bool IsAimModeActive() {
    if (Globals::aim_key == 0)
        return true;
    if (Globals::aim_key_mode == 1)
        return Globals::aim_toggle_state;
    return (GetAsyncKeyState(Globals::aim_key) & 0x8000) != 0;
}

static Vector ReadPunchAngle(uintptr_t pawnAddr) {
    if (!Utils::IsValidPtr(pawnAddr))
        return {};

    uintptr_t services = Utils::SafeRead<uintptr_t>(pawnAddr + Offsets::m_pAimPunchServices);
    if (Utils::IsValidPtr(services)) {
        Vector punch = Utils::SafeRead<Vector>(services + Offsets::m_aimPunchAngle);
        if (std::isfinite(punch.x) && std::isfinite(punch.y) && std::isfinite(punch.z))
            return punch;
    }

    Vector fallback = Utils::SafeRead<Vector>(pawnAddr + Offsets::m_aimPunchAngle);
    if (std::isfinite(fallback.x) && std::isfinite(fallback.y) && std::isfinite(fallback.z))
        return fallback;

    return {};
}

// 修正：使用 std::floor 处理负数
static void ApplyMouseMove(float moveXf, float moveYf) {
    if (Globals::rcs_jitter_enabled) {
        float amount = std::clamp(Globals::rcs_jitter_amount, 0.0f, 2.0f);
        moveXf += jitterDist(rng) * amount;
        moveYf += jitterDist(rng) * amount;
    }

    pendingMoveX += moveXf;
    pendingMoveY += moveYf;

    int moveX = static_cast<int>(std::floor(pendingMoveX));
    int moveY = static_cast<int>(std::floor(pendingMoveY));
    pendingMoveX -= static_cast<float>(moveX);
    pendingMoveY -= static_cast<float>(moveY);

    if (moveX != 0 || moveY != 0)
        mouse_event(MOUSEEVENTF_MOVE, moveX, moveY, 0, 0);
}

// 统一的 RCS 补偿（供 Aimbot 调用）
Vector GetAimCompensation(uintptr_t pawnAddr, Globals::AimWeaponGroup group) {
    if (!Globals::rcs_enabled || !Utils::IsValidPtr(pawnAddr))
        return {};

    RuntimeSettings settings = BuildRuntimeSettings(group);
    if (!settings.enabled)
        return {};

    if (settings.mode == 1 && !IsAimModeActive())
        return {};

    int shotsFired = Utils::SafeRead<int>(pawnAddr + Offsets::m_iShotsFired);
    bool firing = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    if (shotsFired <= 1 || !firing)
        return {};

    Vector punch = ReadPunchAngle(pawnAddr);
    if (!std::isfinite(punch.x) || !std::isfinite(punch.y))
        return {};

    float sens = Utils::SafeRead<float>(pawnAddr + Offsets::m_flFOVSensitivityAdjust, 1.0f);
    if (sens <= 0.0f || !std::isfinite(sens))
        sens = 1.0f;

    float hScale = settings.horizontal / 25.0f * sens;
    float vScale = settings.vertical / 25.0f * sens;
    return {-punch.x * vScale, -punch.y * hScale, 0.0f};
}

void Run() {
    if (!Globals::rcs_enabled) {
        ResetState();
        return;
    }

    if (Menu::IsOpen) {
        ResetState();
        return;
    }

    auto localPawn = EntityManager::Get().GetLocalPawn();
    if (!localPawn || !Utils::SafeAlive(localPawn)) {
        ResetState();
        return;
    }

    uintptr_t pawnAddr = reinterpret_cast<uintptr_t>(localPawn);
    if (!Utils::IsValidPtr(pawnAddr)) {
        ResetState();
        return;
    }

    uintptr_t weapon = Combat::GetActiveWeapon(pawnAddr);
    uint16_t def = Combat::GetWeaponDefinition(weapon);
    Globals::AimWeaponGroup group = Combat::GetWeaponGroup(def);
    RuntimeSettings settings = BuildRuntimeSettings(group);
    if (!settings.enabled) {
        ResetState();
        return;
    }

    if (settings.mode == 1 && !IsAimModeActive()) {
        ResetState();
        return;
    }

    // 如果 Aimbot 正在瞄准，暂停 RCS 以免干扰
    if (Globals::aim_has_target) {
        initialized = true;
        return;
    }

    uint32_t currentHash = Utils::SafeRead<uint32_t>(pawnAddr + Offsets::m_unWeaponHash);
    if (currentHash != lastWeaponHash) {
        ResetState();
        lastWeaponHash = currentHash;
    }

    Vector punch = ReadPunchAngle(pawnAddr);
    int shots = Utils::SafeRead<int>(pawnAddr + Offsets::m_iShotsFired);
    bool firing = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

    if (!initialized) {
        lastPunch = punch;
        lastShotsFired = shots;
        initialized = true;
    }

    if (shots > 1 && firing) {
        Vector delta = punch - lastPunch;
        if (delta.x > 0.0f) delta.x = 0.0f; // 只补偿上抬

        float sens = Utils::SafeRead<float>(pawnAddr + Offsets::m_flFOVSensitivityAdjust, 1.0f);
        if (sens <= 0.0f || !std::isfinite(sens)) sens = 1.0f;

        float moveX = -delta.y * (settings.horizontal / 25.0f) * sens;
        float moveY = -delta.x * (settings.vertical / 25.0f) * sens;

        const float MAX_MOVE = 50.0f;
        moveX = std::clamp(moveX, -MAX_MOVE, MAX_MOVE);
        moveY = std::clamp(moveY, -MAX_MOVE, MAX_MOVE);

        if (std::isfinite(moveX) && std::isfinite(moveY))
            ApplyMouseMove(moveX, moveY);
    } else if (!firing || shots <= 0) {
        pendingMoveX = 0.0f;
        pendingMoveY = 0.0f;
    }

    lastPunch = punch;
    lastShotsFired = shots;
}

} // namespace RCS
