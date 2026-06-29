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

    struct RuntimeSettings
    {
        bool enabled = false;
        int mode = 0;
        float horizontal = 35.0f;
        float vertical = 45.0f;
    };

    // 全局状态
    static Vector lastPunch = { 0, 0, 0 };
    static uint32_t lastWeaponHash = 0;
    static float pendingMoveX = 0.0f;
    static float pendingMoveY = 0.0f;
    static int lastShotsFired = 0;

    // 随机数生成器用于抖动
    static std::random_device rd;
    static std::mt19937 rng(rd());
    static std::uniform_real_distribution<float> jitterDist(-0.3f, 0.3f);

    static RuntimeSettings BuildRuntimeSettings(Globals::AimWeaponGroup group)
    {
        RuntimeSettings settings;
        if (group >= 0 && group < Globals::AIM_GROUP_COUNT)
        {
            const auto& groupSettings = Globals::rcs_weapon_groups[group];
            settings.enabled = groupSettings.enabled;
            settings.mode = std::clamp(groupSettings.mode, 0, 1);
            settings.horizontal = std::clamp(groupSettings.horizontal, 0.0f, 150.0f);
            settings.vertical = std::clamp(groupSettings.vertical, 0.0f, 150.0f);
        }
        return settings;
    }

    static void ResetState()
    {
        lastPunch = { 0, 0, 0 };
        pendingMoveX = 0.0f;
        pendingMoveY = 0.0f;
        lastShotsFired = 0;
    }

    static bool IsAimModeActive()
    {
        if (Globals::aim_key == 0)
            return true;

        if (Globals::aim_key_mode == 1)
            return Globals::aim_toggle_state;

        return (GetAsyncKeyState(Globals::aim_key) & 0x8000) != 0;
    }

    static Vector ReadPunchAngle(uintptr_t pawnAddr)
    {
        if (!Utils::IsValidPtr(pawnAddr))
            return {};

        // 优先从 AimPunchServices 读取
        uintptr_t aimPunchServices = Utils::SafeRead<uintptr_t>(pawnAddr + Offsets::m_pAimPunchServices);
        if (Utils::IsValidPtr(aimPunchServices))
        {
            Vector punch = Utils::SafeRead<Vector>(aimPunchServices + Offsets::m_aimPunchAngle);
            if (std::isfinite(punch.x) && std::isfinite(punch.y) && std::isfinite(punch.z))
                return punch;
        }

        // Fallback: 直接从 pawn 读取
        Vector fallback = Utils::SafeRead<Vector>(pawnAddr + Offsets::m_aimPunchAngle);
        if (std::isfinite(fallback.x) && std::isfinite(fallback.y) && std::isfinite(fallback.z))
            return fallback;

        return {};
    }

    // 修复：正确实现浮点数取整（向下取整）
    static void ApplyMouseMove(float moveXf, float moveYf)
    {
        // 添加微小的随机抖动使RCS更自然（如果启用）
        if (Globals::rcs_jitter_enabled)
        {
            float jitterAmount = Globals::rcs_jitter_amount;
            moveXf += jitterDist(rng) * jitterAmount;
            moveYf += jitterDist(rng) * jitterAmount;
        }

        pendingMoveX += moveXf;
        pendingMoveY += moveYf;

        // 使用 floor 处理负数，避免截断误差
        int moveX = static_cast<int>(std::floor(pendingMoveX));
        int moveY = static_cast<int>(std::floor(pendingMoveY));
        pendingMoveX -= static_cast<float>(moveX);
        pendingMoveY -= static_cast<float>(moveY);

        if (moveX != 0 || moveY != 0)
            mouse_event(MOUSEEVENTF_MOVE, moveX, moveY, 0, 0);
    }

    // 统一的RCS补偿计算（供Aimbot调用）
    Vector GetAimCompensation(uintptr_t pawnAddr, Globals::AimWeaponGroup group)
    {
        if (!Globals::rcs_enabled || !Utils::IsValidPtr(pawnAddr))
            return {};

        RuntimeSettings settings = BuildRuntimeSettings(group);
        if (!settings.enabled)
            return {};

        if (settings.mode == 1 && !IsAimModeActive())
            return {};

        int shotsFired = Utils::SafeRead<int>(pawnAddr + Offsets::m_iShotsFired);
        const bool firing = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        if (shotsFired <= 1 || !firing)
            return {};

        Vector punchAngle = ReadPunchAngle(pawnAddr);
        if (!std::isfinite(punchAngle.x) || !std::isfinite(punchAngle.y))
            return {};

        // 获取灵敏度缩放
        float sensitivityScale = Utils::SafeRead<float>(pawnAddr + Offsets::m_flFOVSensitivityAdjust, 1.0f);
        if (sensitivityScale <= 0.0f || !std::isfinite(sensitivityScale))
            sensitivityScale = 1.0f;

        // 计算补偿值（角度转鼠标移动）
        float horizontalScale = settings.horizontal / 25.0f * sensitivityScale;
        float verticalScale = settings.vertical / 25.0f * sensitivityScale;

        return {
            -punchAngle.x * verticalScale,
            -punchAngle.y * horizontalScale,
            0.0f
        };
    }

    void Run()
    {
        if (!Globals::rcs_enabled)
        {
            ResetState();
            return;
        }

        if (Menu::IsOpen)
        {
            ResetState();
            return;
        }

        auto localPawn = EntityManager::Get().GetLocalPawn();
        if (!localPawn || !Utils::SafeAlive(localPawn))
        {
            ResetState();
            return;
        }

        uintptr_t pawnAddr = reinterpret_cast<uintptr_t>(localPawn);
        if (!Utils::IsValidPtr(pawnAddr))
        {
            ResetState();
            return;
        }

        // 获取当前武器
        uintptr_t activeWeapon = Combat::GetActiveWeapon(pawnAddr);
        uint16_t weaponDef = Combat::GetWeaponDefinition(activeWeapon);
        Globals::AimWeaponGroup activeGroup = Combat::GetWeaponGroup(weaponDef);

        RuntimeSettings settings = BuildRuntimeSettings(activeGroup);
        if (!settings.enabled)
        {
            ResetState();
            return;
        }

        if (settings.mode == 1 && !IsAimModeActive())
        {
            ResetState();
            return;
        }

        // 如果有瞄准目标，暂停RCS（避免与Aimbot冲突）
        if (Globals::aim_has_target)
        {
            // 不清除状态，保持RCS准备就绪
            return;
        }

        // 检测武器切换
        uint32_t currentWeaponHash = Utils::SafeRead<uint32_t>(pawnAddr + Offsets::m_unWeaponHash);
        if (currentWeaponHash != lastWeaponHash)
        {
            ResetState();
            lastWeaponHash = currentWeaponHash;
        }

        Vector punchAngle = ReadPunchAngle(pawnAddr);
        int shotsFired = Utils::SafeRead<int>(pawnAddr + Offsets::m_iShotsFired);
        const bool firing = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        // 只有在射击且子弹数增加时才应用RCS
        if (shotsFired > 1 && firing)
        {
            // 计算相对于上一帧的视角变化
            Vector delta = punchAngle - lastPunch;

            // 只补偿正向的pitch（枪口上抬）
            if (delta.x > 0.0f)
                delta.x = 0.0f;

            // 获取灵敏度缩放
            float sensitivityScale = Utils::SafeRead<float>(pawnAddr + Offsets::m_flFOVSensitivityAdjust, 1.0f);
            if (sensitivityScale <= 0.0f || !std::isfinite(sensitivityScale))
                sensitivityScale = 1.0f;

            // 计算鼠标移动量
            float moveX = -delta.y * (settings.horizontal / 25.0f) * sensitivityScale;
            float moveY = -delta.x * (settings.vertical / 25.0f) * sensitivityScale;

            // 限制最大移动量，防止抖动过大
            const float MAX_MOVE = 50.0f;
            moveX = std::clamp(moveX, -MAX_MOVE, MAX_MOVE);
            moveY = std::clamp(moveY, -MAX_MOVE, MAX_MOVE);

            if (std::isfinite(moveX) && std::isfinite(moveY))
                ApplyMouseMove(moveX, moveY);
        }
        else if (!firing || shotsFired <= 0)
        {
            // 停止射击时重置累积移动
            pendingMoveX = 0.0f;
            pendingMoveY = 0.0f;
        }

        lastPunch = punchAngle;
        lastShotsFired = shotsFired;
    }

}