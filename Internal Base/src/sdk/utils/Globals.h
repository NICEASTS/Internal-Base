#pragma once
#include <cstdint>
#include <string>
#include <array>
#include <Windows.h>

namespace Globals {

// ============================================================
// 武器分组枚举
// ============================================================
enum AimWeaponGroup {
    AIM_GROUP_PISTOL = 0,
    AIM_GROUP_SMG,
    AIM_GROUP_RIFLE,
    AIM_GROUP_SNIPER,
    AIM_GROUP_SHOTGUN,
    AIM_GROUP_HEAVY,
    AIM_GROUP_KNIFE,
    AIM_GROUP_GRENADE,
    AIM_GROUP_COUNT
};

// ============================================================
// RCS 武器组配置
// ============================================================
struct RCSWeaponGroup {
    bool enabled = false;
    int mode = 0;              // 0=始终开启, 1=瞄准键激活
    float horizontal = 35.0f;
    float vertical = 45.0f;
};

// ============================================================
// Aim 武器组配置
// ============================================================
struct AimWeaponGroupSettings {
    bool enabled = true;
    float fov = 7.0f;
    float smoothing = 0.4f;
    bool dynamicSmoothing = true;
    float smoothing_near = 0.16f;
    float smoothing_far = 0.30f;
    bool scope_only = false;
    bool hitbox_head = true;
    bool hitbox_neck = false;
    bool hitbox_chest = false;
    bool hitbox_stomach = false;
    bool hitbox_pelvis = false;
};

// ============================================================
// 全局配置结构体
// ============================================================
struct Globals {
    // ---------- 窗口 ----------
    HWND window = nullptr;
    bool menu_open = false;

    // ---------- 自瞄 ----------
    bool aim_enabled = true;
    int aim_key = 0;                    // VK 键码，0=始终开启
    int aim_key_mode = 0;               // 0=按住, 1=切换
    bool aim_toggle_state = false;
    bool aim_has_target = false;
    int aim_target_index = -1;
    bool target_teammates = false;
    bool aim_vis_check = true;
    
    // 新增：软可见性
    bool aim_soft_visible = false;
    int aim_soft_visible_min_bones = 1;
    int aim_soft_visible_grace_ms = 300;
    
    // 新增：粘性目标
    bool aim_sticky_target = false;
    int aim_switch_delay_ms = 200;
    
    // 新增：持久目标
    bool aim_persistent = false;
    
    // 新增：人机化
    bool aim_humanize = false;
    float aim_humanize_curve = 0.5f;
    float aim_humanize_strength = 0.3f;
    float aim_humanize_jitter = 0.1f;
    
    bool aim_ignore_flash = false;

    // ---------- RCS ----------
    bool rcs_enabled = true;
    
    // 新增：RCS 抖动
    bool rcs_jitter_enabled = false;
    float rcs_jitter_amount = 0.1f;

    // ---------- 视觉 ----------
    bool visuals_fov_enabled = false;
    float visuals_fov = 90.0f;
    bool visuals_esp_enabled = false;
    bool visuals_box_enabled = false;
    bool visuals_snaplines_enabled = false;
    bool visuals_healthbar_enabled = false;
    bool visuals_armorbar_enabled = false;
    bool visuals_name_enabled = false;

    // ---------- 武器组配置数组 ----------
    std::array<RCSWeaponGroup, AIM_GROUP_COUNT> rcs_weapon_groups;
    std::array<AimWeaponGroupSettings, AIM_GROUP_COUNT> aim_weapon_groups;

    // ---------- 构造 / 初始化 ----------
    Globals();
    void InitDefaults();
};

// 全局单例访问
Globals& Get();

// ============================================================
// 便捷宏（可选）
// ============================================================
#define g_Globals Globals::Get()

} // namespace Globals
