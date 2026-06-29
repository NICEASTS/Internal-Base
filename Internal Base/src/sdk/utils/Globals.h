#pragma once
#include <cstdint>
#include <Windows.h>
#include <vector>
#include <string>
#include <set>
#include "../../feature/skinchanger/SkinData.h"

namespace Globals
{
    inline float ViewMatrix[16] = { 0.f };
    inline int ScreenWidth = 0;
    inline int ScreenHeight = 0;

    
    
    inline float GameViewportX = 0.f;      
    inline float GameViewportY = 0.f;      
    inline float GameViewportWidth = 0.f;  
    inline float GameViewportHeight = 0.f; 

    inline bool esp_enabled = false;
    inline bool esp_enabled_team = false;
    inline int  esp_bind = VK_F1;

    inline bool esp_box = false;
    inline bool esp_box_team = false;
    inline int esp_box_type = 1; 
    inline int esp_box_type_team = 1; 
    inline float esp_box_color[4] = { 1.f, 0.f, 0.f, 1.f };
    inline float esp_box_color_vis[4] = { 1.f, 0.f, 0.f, 1.f };
    inline float esp_box_color_team[4] = { 0.2f, 0.7f, 1.0f, 1.0f };
    inline float esp_box_color_vis_team[4] = { 0.2f, 0.7f, 1.0f, 1.0f };
    inline float esp_box_thickness = 1.5f;

    inline bool esp_skeleton = false;
    inline bool esp_skeleton_team = false;
    inline float esp_skeleton_color[4] = { 1.f, 1.f, 1.f, 0.9f };
    inline float esp_skeleton_color_vis[4] = { 1.f, 1.f, 1.f, 0.9f };
    inline float esp_skeleton_color_team[4] = { 0.2f, 0.7f, 1.0f, 0.9f };
    inline float esp_skeleton_color_vis_team[4] = { 0.2f, 0.7f, 1.0f, 0.9f };
    inline float esp_skeleton_thickness = 1.8f;
    inline int esp_skeleton_type = 1; 
    inline int esp_skeleton_type_team = 0;
    inline bool esp_head = false;
    inline bool esp_head_team = false;

    inline bool esp_name = false;
    inline bool esp_name_team = false;
    inline float esp_name_color[4] = { 1.f, 1.f, 1.f, 1.f };
    inline float esp_name_color_vis[4] = { 1.f, 1.f, 1.f, 1.f };
    inline float esp_name_color_team[4] = { 0.85f, 0.95f, 1.0f, 1.f };
    inline float esp_name_color_vis_team[4] = { 0.85f, 0.95f, 1.0f, 1.f };

    inline bool esp_distance = false;
    inline bool esp_distance_team = false;
    inline float esp_distance_color[4] = { 1.f, 1.f, 1.f, 1.f };
    inline float esp_distance_color_vis[4] = { 1.f, 1.f, 1.f, 1.f };
    inline float esp_distance_color_team[4] = { 0.85f, 0.95f, 1.0f, 1.f };
    inline float esp_distance_color_vis_team[4] = { 0.85f, 0.95f, 1.0f, 1.f };

    inline bool esp_weapon = false;
    inline bool esp_weapon_team = false;
    inline float esp_weapon_color[4] = { 1.f, 1.f, 1.f, 1.f };
    inline float esp_weapon_color_vis[4] = { 1.f, 1.f, 1.f, 1.f };
    inline float esp_weapon_color_team[4] = { 0.85f, 0.95f, 1.0f, 1.f };
    inline float esp_weapon_color_vis_team[4] = { 0.85f, 0.95f, 1.0f, 1.f };

    inline bool esp_health = false;
    inline bool esp_health_team = false;
    inline float esp_health_color[4] = { 0.f, 1.f, 0.f, 1.f };
    inline float esp_health_color_vis[4] = { 0.f, 1.f, 0.f, 1.f };
    inline float esp_health_color_team[4] = { 0.15f, 0.9f, 0.95f, 1.f };
    inline float esp_health_color_vis_team[4] = { 0.15f, 0.9f, 0.95f, 1.f };

    
    inline bool esp_glow = false;
    inline bool esp_glow_team = false;
    inline bool esp_glow_dead = false;
    inline float esp_glow_color[4] = { 1.f, 0.f, 0.f, 1.f };
    inline float esp_glow_color_vis[4] = { 0.f, 1.f, 0.f, 1.f };
    inline float esp_glow_color_team[4] = { 0.2f, 0.7f, 1.0f, 1.f };
    inline float esp_glow_color_vis_team[4] = { 0.2f, 1.0f, 0.7f, 1.f };
    inline float esp_glow_thickness = 3.0f;

    
    inline bool esp_sound_enabled = false;
    inline bool esp_sound_enabled_team = false;
    inline float esp_sound_color[4] = { 1.f, 0.6f, 0.f, 1.f };
    inline float esp_sound_color_team[4] = { 0.2f, 0.8f, 1.0f, 1.f };

    inline bool trigger_enabled = false;
    inline int trigger_key = 0;
    inline int trigger_key_mode = 0;
    inline bool trigger_toggle_state = false;
    inline bool trigger_ignore_flash = false;
    inline float trigger_delay = 0.0f;
    inline float trigger_fov = 7.0f;
    inline bool trigger_draw_fov = false;
    inline float trigger_fov_color[4] = { 1.f, 1.f, 0.f, 0.6f };
    
    
    inline bool trigger_hitbox_head = true;
    inline bool trigger_hitbox_neck = false;
    inline bool trigger_hitbox_chest = false;
    inline bool trigger_hitbox_stomach = false;
    inline bool trigger_hitbox_pelvis = false;

    
    inline bool trigger_scope_only = false;
    inline bool trigger_random_delay = false;
    inline int trigger_min_delay = 0;
    inline int trigger_max_delay = 15;
    inline bool trigger_velocity_check = false;

     
    inline bool rcs_enabled = false;
    inline int rcs_mode = 0; // legacy config compatibility: 0 standalone, 1 aiming
    inline float rcs_amount = 0.5f;       // legacy config compatibility
    inline float rcs_horizontal = 35.0f;
    inline float rcs_vertical = 45.0f;
    inline float rcs_smooth = 0.55f;      // legacy config compatibility
    inline float rcs_calibration = 0.5f;  // legacy config compatibility

    
    inline bool target_teammates = false;

    
    inline bool aim_enabled = false;
    inline bool aim_vis_check = false;
    inline bool aim_ignore_flash = false;
    inline bool aim_scope_only = false;
    inline bool aim_soft_visible = false;
    inline int aim_soft_visible_grace_ms = 250;
    inline int aim_soft_visible_min_bones = 1;
    inline bool aim_persistent = false; 
    inline float aim_fov = 7.f;
    inline float aim_fov_color[4] = { 1.f, 1.f, 1.f, 0.6f };
    inline float aim_smoothing = 0.4f;   
    inline int aim_key = 0;
    inline int aim_key_mode = 0;
    inline bool aim_toggle_state = false;
    inline bool aim_has_target = false;
    inline int aim_target_index = -1;
    inline bool aim_draw_fov = false;

    
    inline bool aim_hitbox_head = true;
    inline bool aim_hitbox_neck = false;
    inline bool aim_hitbox_chest = false;
    inline bool aim_hitbox_stomach = false;
    inline bool aim_hitbox_pelvis = false;

    
    inline bool  aim_humanize = false;
    inline float aim_humanize_strength = 1.0f;  
    inline float aim_humanize_jitter = 0.5f;    
    inline float aim_humanize_curve = 0.8f;     

    
    inline bool aim_sticky_target = true;
    inline int aim_switch_delay_ms = 100;
    inline bool aim_dynamic_smoothing = true;
    inline float aim_smoothing_near = 0.16f;
    inline float aim_smoothing_far = 0.30f;

    enum AimWeaponGroup : int
    {
        AIM_GROUP_PISTOL = 0,
        AIM_GROUP_SMG,
        AIM_GROUP_RIFLE,
        AIM_GROUP_SNIPER,
        AIM_GROUP_HEAVY,
        AIM_GROUP_SHOTGUN,
        AIM_GROUP_OTHER,
        AIM_GROUP_COUNT
    };

    struct AimGroupSettings
    {
        bool enabled = false;
        float fov = 7.0f;
        float smoothing = 0.4f;
        bool dynamic_smoothing = true;
        float smoothing_near = 0.16f;
        float smoothing_far = 0.30f;
        bool scope_only = false;
        bool hitbox_head = true;
        bool hitbox_neck = false;
        bool hitbox_chest = false;
        bool hitbox_stomach = false;
        bool hitbox_pelvis = false;
    };

    struct RcsGroupSettings
    {
        bool enabled = true;
        int mode = 0;
        float horizontal = 35.0f;
        float vertical = 45.0f;
    };

    inline int aim_selected_weapon_group = AIM_GROUP_RIFLE;
    inline AimGroupSettings aim_weapon_groups[AIM_GROUP_COUNT] = {
        { true, 6.0f, 0.55f, true, 0.22f, 0.36f, false, true, false, false, false, false },
        { true, 7.0f, 0.48f, true, 0.18f, 0.32f, false, true, false, true, false, false },
        { true, 6.5f, 0.42f, true, 0.16f, 0.30f, false, true, true, false, false, false },
        { true, 4.0f, 0.34f, true, 0.12f, 0.24f, true, true, false, false, false, false },
        { true, 7.5f, 0.55f, true, 0.22f, 0.38f, false, true, false, true, false, false },
        { true, 8.0f, 0.50f, true, 0.20f, 0.34f, false, true, false, true, true, false },
        { true, 7.0f, 0.45f, true, 0.18f, 0.32f, false, true, false, false, false, false }
    };

    inline RcsGroupSettings rcs_weapon_groups[AIM_GROUP_COUNT] = {
        { true, 0, 32.0f, 42.0f },
        { true, 0, 34.0f, 48.0f },
        { true, 0, 38.0f, 54.0f },
        { true, 1, 24.0f, 32.0f },
        { true, 0, 44.0f, 62.0f },
        { true, 0, 30.0f, 38.0f },
        { true, 0, 35.0f, 45.0f }
    };

    
    inline bool chams_enabled = false;
    inline bool chams_enabled_team = false;
    inline bool chams_wireframe = true;
    inline bool chams_wireframe_team = true;
    inline bool chams_filled = true;
    inline bool chams_filled_team = true;
    inline float chams_wire_color[4] = { 0.f, 1.f, 1.f, 1.f };   
    inline float chams_wire_color_vis[4] = { 0.f, 1.f, 1.f, 1.f };
    inline float chams_fill_color[4] = { 0.f, 0.8f, 0.f, 0.35f }; 
    inline float chams_fill_color_vis[4] = { 0.f, 0.8f, 0.f, 0.35f };
    inline float chams_wire_color_team[4] = { 0.2f, 0.7f, 1.0f, 1.f };
    inline float chams_wire_color_vis_team[4] = { 0.2f, 0.7f, 1.0f, 1.f };
    inline float chams_fill_color_team[4] = { 0.15f, 0.55f, 0.95f, 0.35f };
    inline float chams_fill_color_vis_team[4] = { 0.15f, 0.55f, 0.95f, 0.35f };

    
    inline bool chamsv2_enabled = false;
    inline bool chamsv2_enabled_team = false;
    inline int  chamsv2_material_type = 0; 
    inline int  chamsv2_material_type_team = 0; 
    inline float chamsv2_fill_color[4] = { 0.8f, 0.2f, 1.0f, 0.6f };   
    inline float chamsv2_fill_color_vis[4] = { 0.8f, 0.2f, 1.0f, 0.6f };
    inline float chamsv2_wire_color[4] = { 0.0f, 1.0f, 1.0f, 1.0f };   
    inline float chamsv2_wire_color_vis[4] = { 0.0f, 1.0f, 1.0f, 1.0f };
    inline float chamsv2_fill_color_team[4] = { 0.2f, 0.55f, 1.0f, 0.6f };
    inline float chamsv2_fill_color_vis_team[4] = { 0.2f, 0.55f, 1.0f, 0.6f };
    inline float chamsv2_wire_color_team[4] = { 0.65f, 0.9f, 1.0f, 1.0f };
    inline float chamsv2_wire_color_vis_team[4] = { 0.65f, 0.9f, 1.0f, 1.0f };

    
    inline bool chamsv4_enabled = false;
    inline bool chamsv4_wallhack = true;
    inline float chamsv4_hidden_color[4] = { 1.0f, 0.0f, 0.0f, 0.6f };     
    inline float chamsv4_visible_color[4] = { 0.0f, 1.0f, 0.0f, 0.6f };    
    
    inline bool chamsv4_players = true;       
    inline bool chamsv4_hands = false;        
    inline bool chamsv4_gloves = false;       
    inline bool chamsv4_team = false;         
    inline bool chamsv4_weapons = false;      
    inline bool chamsv4_weapons_guns = true;  
    inline bool chamsv4_weapons_knives = true;
    inline bool chamsv4_chicken = false;      
    
    
    inline int chamsv4_mat_players = 0;
    inline int chamsv4_mat_hands = 0;
    inline int chamsv4_mat_gloves = 0;
    inline int chamsv4_mat_guns = 0;
    inline int chamsv4_mat_knives = 0;
    inline int chamsv4_mat_chicken = 0;
    inline int chamsv4_mat_team   = 0;       

    
    inline bool world_light_enabled = false;          
    inline float world_light_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; 
    inline float world_light_intensity = 3.0f;        
    inline bool world_walls_enabled = false;           
    inline float world_walls_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; 

    
    inline bool visuals_fov_enabled = false;
    inline float visuals_fov = 90.f;

    
    inline float menu_accent_color[4] = { 2.0f / 255.0f, 132.0f / 255.0f, 199.0f / 255.0f, 1.f }; 
    inline float menu_bg_color[4] = { 24.f / 255.f, 24.f / 255.f, 24.f / 255.f, 1.f };
    inline float menu_sidebar_color[4] = { 30.f / 255.f, 30.f / 255.f, 30.f / 255.f, 1.f };
    inline float menu_text_main_color[4] = { 1.f, 1.f, 1.f, 1.f };
    inline float menu_text_muted_color[4] = { 105.f / 255.f, 105.f / 255.f, 105.f / 255.f, 1.f };
    inline float menu_child_bg_color[4] = { 32.f / 255.f, 32.f / 255.f, 32.f / 255.f, 1.f };
    
    inline int menu_key = VK_INSERT;
    inline bool show_settings = false;

    
    
    
    inline bool sc_enabled = false;
    
    
    inline volatile int sc_force_update = 0;

    
    inline WeaponsEnum sc_current_weapon_def = WEP_NONE;

    
    inline int sc_selected_knife = 0;       

    
    inline int sc_selected_glove_type = -1;  
    inline int sc_selected_glove_skin = -1;  

    
    inline int sc_selected_music_kit = -1;   

    
    inline int sc_selected_agent_ct = -1;    
    inline int sc_selected_agent_t = -1;     

    
    
    inline std::set<int> sc_fix_skin_weapons;

    
    
    
    inline bool hitsound_enabled = false;
    inline int  hitsound_selected = -1;      
    inline std::vector<std::string> hitsound_files;  
    inline std::string hitsound_dir = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Counter-Strike Global Offensive\\game\\csgo\\ascania\\wav";
    inline float hitsound_volume = 1.0f;     

    
    
    
    inline bool deathsound_enabled = false;
    inline int  deathsound_selected = -1;      
    inline std::vector<std::string> deathsound_files;  
    inline std::string deathsound_dir = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Counter-Strike Global Offensive\\game\\csgo\\ascania\\wav\\death";
    inline float deathsound_volume = 1.0f;     

    
    
    
    inline bool speclist_enabled = false;

    
    
    
    inline bool hitmarker_enabled = false;
    inline float hitmarker_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; 

    
    
    
    inline bool grenade_prediction_enabled = false;
    inline float grenade_prediction_color[4] = { 0.20f, 1.0f, 0.20f, 0.95f };
    inline float grenade_prediction_hit_color[4] = { 1.0f, 0.35f, 0.25f, 1.0f };
    inline float grenade_prediction_thickness = 2.0f;
    inline float grenade_prediction_radius = 5.0f;
    inline float grenade_prediction_bounce = 0.45f;
    inline float grenade_prediction_friction = 0.40f;
    inline int grenade_prediction_max_bounces = 12;
    inline bool grenade_prediction_draw_points = true;

    
    
    
    inline bool bombtimer_enabled = false;

    
    
    
    inline bool  bullettracer_enabled = false;
    inline float bullettracer_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; 
    inline float bullettracer_traillife = 2.5f;   
    inline float bullettracer_thickness = 2.0f;
    inline bool  killparticle_enabled = false;
    inline int   killparticle_type = 0;
    inline float killparticle_color[4] = { 1.0f, 0.45f, 0.1f, 1.0f };

    
    
    
    inline bool  hitparticle_enabled = false;
    inline int   hitparticle_style = 0;        
    inline float hitparticle_color[4] = { 0.85f, 0.08f, 0.08f, 1.0f }; 
    inline bool  hitparticle_color_override = false;
    inline float hitparticle_size = 1.0f;       
    inline float hitparticle_lifetime = 0.65f;  
    inline float hitparticle_intensity = 1.0f;  
    inline float hitparticle_glow = 0.6f;       
    inline bool  hitparticle_randomize = true;
    inline bool  hitparticle_distortion = true;
    inline int   hitparticle_max_per_hit = 32;  

    
    
    
    inline bool  chams_kill_effect = false;
    inline int   chams_kill_effect_style = 0;      
    inline float chams_kill_effect_duration = 1.2f; 
    inline float chams_kill_effect_intensity = 1.0f;
    inline float chams_kill_effect_edge_color[4] = { 1.0f, 0.6f, 0.2f, 1.0f };

    inline bool  wireframe_enemy_enabled = false;
    inline float wireframe_color[4] = { 1.0f, 0.0f, 0.6f, 1.0f };
    inline bool  skybox_changer = false;
    inline float skybox_color[4] = { 0.25f, 0.55f, 1.0f, 1.0f };
    inline float skybox_intensity = 1.0f;

    
    
    
    inline bool  smoke_color_enabled = false;
    inline float smoke_color[4] = { 64.0f / 255.0f, 77.0f / 255.0f, 236.0f / 255.0f, 1.0f }; 

    
    
    
    inline bool  hitlog_enabled = false;
    inline bool  hitlog_draw_damage = false;    
    inline float hitlog_duration = 3.5f;        
    inline float hitlog_slide_speed = 0.25f;    
    inline float hitlog_fade_speed = 0.4f;      
    inline int   hitlog_max_visible = 5;        
    inline float hitlog_damage_color[4] = { 1.0f, 0.35f, 0.35f, 1.0f };  
    inline float hitlog_hitbox_color[4] = { 0.4f, 0.85f, 1.0f, 1.0f };   

    
    
    
    inline bool thirdperson_enabled = false;

    
    
    
    inline bool antiflash_enabled = false;

    
    
    
    inline bool  chatspam_enabled = false;
    inline char  chatspam_message[256] = "GhostWeave on top";
    inline float chatspam_interval = 5.0f;   

    
    
    
    inline bool bunnyhop_enabled = false;

    
    
    
    inline bool autoaccept_enabled = false;
}
