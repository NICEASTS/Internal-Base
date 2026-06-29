#include "Globals.h"

namespace Globals {

// ============================================================
// 构造函数 - 初始化所有默认值
// ============================================================
Globals::Globals() {
    InitDefaults();
}

void Globals::InitDefaults() {
    // ---------- 窗口 ----------
    window = nullptr;
    menu_open = false;

    // ---------- 自瞄 ----------
    aim_enabled = true;
    aim_key = 0;
    aim_key_mode = 0;
    aim_toggle_state = false;
    aim_has_target = false;
    aim_target_index = -1;
    target_teammates = false;
    aim_vis_check = true;
    aim_ignore_flash = false;

    // 新增：软可见性
    aim_soft_visible = false;
    aim_soft_visible_min_bones = 1;
    aim_soft_visible_grace_ms = 300;

    // 新增：粘性目标
    aim_sticky_target = false;
    aim_switch_delay_ms = 200;

    // 新增：持久目标
    aim_persistent = false;

    // 新增：人机化
    aim_humanize = false;
    aim_humanize_curve = 0.5f;
    aim_humanize_strength = 0.3f;
    aim_humanize_jitter = 0.1f;

    // ---------- RCS ----------
    rcs_enabled = true;

    // 新增：RCS 抖动
    rcs_jitter_enabled = false;
    rcs_jitter_amount = 0.1f;

    // ---------- 视觉 ----------
    visuals_fov_enabled = false;
    visuals_fov = 90.0f;
    visuals_esp_enabled = false;
    visuals_box_enabled = false;
    visuals_snaplines_enabled = false;
    visuals_healthbar_enabled = false;
    visuals_armorbar_enabled = false;
    visuals_name_enabled = false;

    // ---------- 初始化 RCS 武器组 ----------
    // 手枪
    rcs_weapon_groups[AIM_GROUP_PISTOL] = {true, 0, 25.0f, 30.0f};
    // SMG
    rcs_weapon_groups[AIM_GROUP_SMG] = {true, 0, 30.0f, 35.0f};
    // 步枪
    rcs_weapon_groups[AIM_GROUP_RIFLE] = {true, 0, 35.0f, 45.0f};
    // 狙击
    rcs_weapon_groups[AIM_GROUP_SNIPER] = {true, 1, 20.0f, 25.0f};
    // 霰弹枪
    rcs_weapon_groups[AIM_GROUP_SHOTGUN] = {false, 0, 30.0f, 40.0f};
    // 重武器
    rcs_weapon_groups[AIM_GROUP_HEAVY] = {true, 0, 40.0f, 50.0f};
    // 刀
    rcs_weapon_groups[AIM_GROUP_KNIFE] = {false, 0, 0.0f, 0.0f};
    // 投掷物
    rcs_weapon_groups[AIM_GROUP_GRENADE] = {false, 0, 0.0f, 0.0f};

    // ---------- 初始化 Aim 武器组 ----------
    // 手枪
    aim_weapon_groups[AIM_GROUP_PISTOL] = {
        true, 8.0f, 0.35f, true, 0.15f, 0.28f, false,
        true, false, false, false, false
    };
    // SMG
    aim_weapon_groups[AIM_GROUP_SMG] = {
        true, 10.0f, 0.40f, true, 0.16f, 0.30f, false,
        true, false, false, false, false
    };
    // 步枪
    aim_weapon_groups[AIM_GROUP_RIFLE] = {
        true, 7.0f, 0.40f, true, 0.16f, 0.30f, false,
        true, false, false, false, false
    };
    // 狙击
    aim_weapon_groups[AIM_GROUP_SNIPER] = {
        true, 5.0f, 0.30f, true, 0.12f, 0.25f, true,
        true, false, false, false, false
    };
    // 霰弹枪
    aim_weapon_groups[AIM_GROUP_SHOTGUN] = {
        false, 15.0f, 0.50f, true, 0.20f, 0.40f, false,
        true, false, false, false, false
    };
    // 重武器
    aim_weapon_groups[AIM_GROUP_HEAVY] = {
        true, 12.0f, 0.45f, true, 0.18f, 0.35f, false,
        true, false, false, false, false
    };
    // 刀
    aim_weapon_groups[AIM_GROUP_KNIFE] = {
        false, 20.0f, 0.60f, true, 0.30f, 0.50f, false,
        true, false, false, false, false
    };
    // 投掷物
    aim_weapon_groups[AIM_GROUP_GRENADE] = {
        false, 20.0f, 0.60f, true, 0.30f, 0.50f, false,
        true, false, false, false, false
    };
}

// ============================================================
// 单例访问
// ============================================================
static Globals g_globals;

Globals& Get() {
    return g_globals;
}

} // namespace Globals
