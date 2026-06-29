#include "Config.h"
#include "Globals.h"
#include "../../feature/skinchanger/SkinData.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <shlobj.h>
#include <algorithm>

namespace {
    std::string NormalizeConfigName(std::string name)
    {
        if (name.empty())
            return "";
        if (name.find(".json") == std::string::npos)
            name += ".json";
        return name;
    }

    std::string EscapeJsonString(const std::string& s)
    {
        std::string out;
        out.reserve(s.size() + 16);
        for (char c : s)
        {
            switch (c)
            {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
            }
        }
        return out;
    }

    bool ParseJsonLinesArray(const std::string& content, std::string& outText)
    {
        const size_t linesKey = content.find("\"lines\"");
        if (linesKey == std::string::npos)
            return false;

        const size_t lb = content.find('[', linesKey);
        if (lb == std::string::npos)
            return false;

        size_t i = lb + 1;
        outText.clear();

        auto skipWs = [&](size_t& p) {
            while (p < content.size() && (content[p] == ' ' || content[p] == '\n' || content[p] == '\r' || content[p] == '\t'))
                ++p;
        };

        while (i < content.size())
        {
            skipWs(i);
            if (i >= content.size()) return false;
            if (content[i] == ']') break;
            if (content[i] != '"') return false;
            ++i;

            std::string line;
            while (i < content.size())
            {
                char c = content[i++];
                if (c == '"')
                    break;
                if (c == '\\')
                {
                    if (i >= content.size()) return false;
                    char e = content[i++];
                    switch (e)
                    {
                    case 'n': line.push_back('\n'); break;
                    case 'r': line.push_back('\r'); break;
                    case 't': line.push_back('\t'); break;
                    case '\\': line.push_back('\\'); break;
                    case '"': line.push_back('"'); break;
                    default: line.push_back(e); break;
                    }
                }
                else
                {
                    line.push_back(c);
                }
            }

            outText += line;
            outText += '\n';

            skipWs(i);
            if (i < content.size() && content[i] == ',')
                ++i;
        }

        return true;
    }

    
    void ForceWriteConfig(const std::filesystem::path& filePath, const char* content)
    {
        
        if (std::filesystem::exists(filePath))
            std::filesystem::remove(filePath);

        std::ofstream out(filePath);
        if (!out.is_open())
            return;

        out << content;
    }

    void EnsureDefaultConfigsExist(const std::filesystem::path& configDir)
    {
        (void)configDir;
    }
}

namespace Config
{
    std::filesystem::path GetConfigPath()
    {
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, path))) {
            std::filesystem::path p = path;
            p /= "GhostWeave";
            if (!std::filesystem::exists(p))
                std::filesystem::create_directories(p);
            return p;
        }
        return "";
    }

    static std::filesystem::path GetFavoritePath()
    {
        return GetConfigPath() / "favorite.txt";
    }

    void Save(std::string name)
    {
        name = NormalizeConfigName(name);
        if (name.empty()) return;

        std::filesystem::path path = GetConfigPath() / name;
        std::ostringstream out;

        
        out << "esp_enabled " << Globals::esp_enabled << "\n";
        out << "esp_enabled_team " << Globals::esp_enabled_team << "\n";
        out << "esp_bind " << Globals::esp_bind << "\n";
        out << "esp_box " << Globals::esp_box << "\n";
        out << "esp_box_team " << Globals::esp_box_team << "\n";
        out << "esp_box_type " << Globals::esp_box_type << "\n";
        out << "esp_box_type_team " << Globals::esp_box_type_team << "\n";
        out << "esp_box_color " << Globals::esp_box_color[0] << " " << Globals::esp_box_color[1] << " " << Globals::esp_box_color[2] << " " << Globals::esp_box_color[3] << "\n";
        out << "esp_box_color_vis " << Globals::esp_box_color_vis[0] << " " << Globals::esp_box_color_vis[1] << " " << Globals::esp_box_color_vis[2] << " " << Globals::esp_box_color_vis[3] << "\n";
        out << "esp_box_color_team " << Globals::esp_box_color_team[0] << " " << Globals::esp_box_color_team[1] << " " << Globals::esp_box_color_team[2] << " " << Globals::esp_box_color_team[3] << "\n";
        out << "esp_box_color_vis_team " << Globals::esp_box_color_vis_team[0] << " " << Globals::esp_box_color_vis_team[1] << " " << Globals::esp_box_color_vis_team[2] << " " << Globals::esp_box_color_vis_team[3] << "\n";
        out << "esp_box_thickness " << Globals::esp_box_thickness << "\n";
        out << "esp_skeleton " << Globals::esp_skeleton << "\n";
        out << "esp_skeleton_team " << Globals::esp_skeleton_team << "\n";
        out << "esp_skeleton_color " << Globals::esp_skeleton_color[0] << " " << Globals::esp_skeleton_color[1] << " " << Globals::esp_skeleton_color[2] << " " << Globals::esp_skeleton_color[3] << "\n";
        out << "esp_skeleton_color_vis " << Globals::esp_skeleton_color_vis[0] << " " << Globals::esp_skeleton_color_vis[1] << " " << Globals::esp_skeleton_color_vis[2] << " " << Globals::esp_skeleton_color_vis[3] << "\n";
        out << "esp_skeleton_color_team " << Globals::esp_skeleton_color_team[0] << " " << Globals::esp_skeleton_color_team[1] << " " << Globals::esp_skeleton_color_team[2] << " " << Globals::esp_skeleton_color_team[3] << "\n";
        out << "esp_skeleton_color_vis_team " << Globals::esp_skeleton_color_vis_team[0] << " " << Globals::esp_skeleton_color_vis_team[1] << " " << Globals::esp_skeleton_color_vis_team[2] << " " << Globals::esp_skeleton_color_vis_team[3] << "\n";
        out << "esp_skeleton_thickness " << Globals::esp_skeleton_thickness << "\n";
        out << "esp_skeleton_type " << Globals::esp_skeleton_type << "\n";
        out << "esp_skeleton_type_team " << Globals::esp_skeleton_type_team << "\n";
        out << "esp_head " << Globals::esp_head << "\n";
        out << "esp_head_team " << Globals::esp_head_team << "\n";
        out << "esp_name " << Globals::esp_name << "\n";
        out << "esp_name_team " << Globals::esp_name_team << "\n";
        out << "esp_name_color " << Globals::esp_name_color[0] << " " << Globals::esp_name_color[1] << " " << Globals::esp_name_color[2] << " " << Globals::esp_name_color[3] << "\n";
        out << "esp_name_color_vis " << Globals::esp_name_color_vis[0] << " " << Globals::esp_name_color_vis[1] << " " << Globals::esp_name_color_vis[2] << " " << Globals::esp_name_color_vis[3] << "\n";
        out << "esp_name_color_team " << Globals::esp_name_color_team[0] << " " << Globals::esp_name_color_team[1] << " " << Globals::esp_name_color_team[2] << " " << Globals::esp_name_color_team[3] << "\n";
        out << "esp_name_color_vis_team " << Globals::esp_name_color_vis_team[0] << " " << Globals::esp_name_color_vis_team[1] << " " << Globals::esp_name_color_vis_team[2] << " " << Globals::esp_name_color_vis_team[3] << "\n";
        out << "esp_distance " << Globals::esp_distance << "\n";
        out << "esp_distance_team " << Globals::esp_distance_team << "\n";
        out << "esp_distance_color " << Globals::esp_distance_color[0] << " " << Globals::esp_distance_color[1] << " " << Globals::esp_distance_color[2] << " " << Globals::esp_distance_color[3] << "\n";
        out << "esp_distance_color_vis " << Globals::esp_distance_color_vis[0] << " " << Globals::esp_distance_color_vis[1] << " " << Globals::esp_distance_color_vis[2] << " " << Globals::esp_distance_color_vis[3] << "\n";
        out << "esp_distance_color_team " << Globals::esp_distance_color_team[0] << " " << Globals::esp_distance_color_team[1] << " " << Globals::esp_distance_color_team[2] << " " << Globals::esp_distance_color_team[3] << "\n";
        out << "esp_distance_color_vis_team " << Globals::esp_distance_color_vis_team[0] << " " << Globals::esp_distance_color_vis_team[1] << " " << Globals::esp_distance_color_vis_team[2] << " " << Globals::esp_distance_color_vis_team[3] << "\n";
        out << "esp_health " << Globals::esp_health << "\n";
        out << "esp_health_team " << Globals::esp_health_team << "\n";
        out << "esp_health_color " << Globals::esp_health_color[0] << " " << Globals::esp_health_color[1] << " " << Globals::esp_health_color[2] << " " << Globals::esp_health_color[3] << "\n";
        out << "esp_health_color_vis " << Globals::esp_health_color_vis[0] << " " << Globals::esp_health_color_vis[1] << " " << Globals::esp_health_color_vis[2] << " " << Globals::esp_health_color_vis[3] << "\n";
        out << "esp_health_color_team " << Globals::esp_health_color_team[0] << " " << Globals::esp_health_color_team[1] << " " << Globals::esp_health_color_team[2] << " " << Globals::esp_health_color_team[3] << "\n";
        out << "esp_health_color_vis_team " << Globals::esp_health_color_vis_team[0] << " " << Globals::esp_health_color_vis_team[1] << " " << Globals::esp_health_color_vis_team[2] << " " << Globals::esp_health_color_vis_team[3] << "\n";
        out << "esp_weapon " << Globals::esp_weapon << "\n";
        out << "esp_weapon_team " << Globals::esp_weapon_team << "\n";
        out << "esp_weapon_color " << Globals::esp_weapon_color[0] << " " << Globals::esp_weapon_color[1] << " " << Globals::esp_weapon_color[2] << " " << Globals::esp_weapon_color[3] << "\n";
        out << "esp_weapon_color_vis " << Globals::esp_weapon_color_vis[0] << " " << Globals::esp_weapon_color_vis[1] << " " << Globals::esp_weapon_color_vis[2] << " " << Globals::esp_weapon_color_vis[3] << "\n";
        out << "esp_weapon_color_team " << Globals::esp_weapon_color_team[0] << " " << Globals::esp_weapon_color_team[1] << " " << Globals::esp_weapon_color_team[2] << " " << Globals::esp_weapon_color_team[3] << "\n";
        out << "esp_weapon_color_vis_team " << Globals::esp_weapon_color_vis_team[0] << " " << Globals::esp_weapon_color_vis_team[1] << " " << Globals::esp_weapon_color_vis_team[2] << " " << Globals::esp_weapon_color_vis_team[3] << "\n";
        out << "esp_sound_enabled " << Globals::esp_sound_enabled << "\n";
        out << "esp_sound_enabled_team " << Globals::esp_sound_enabled_team << "\n";
        out << "esp_sound_color " << Globals::esp_sound_color[0] << " " << Globals::esp_sound_color[1] << " " << Globals::esp_sound_color[2] << " " << Globals::esp_sound_color[3] << "\n";
        out << "esp_sound_color_team " << Globals::esp_sound_color_team[0] << " " << Globals::esp_sound_color_team[1] << " " << Globals::esp_sound_color_team[2] << " " << Globals::esp_sound_color_team[3] << "\n";

        
        out << "aim_enabled " << Globals::aim_enabled << "\n";
        out << "aim_vis_check " << Globals::aim_vis_check << "\n";
        out << "aim_ignore_flash " << Globals::aim_ignore_flash << "\n";
        out << "aim_scope_only " << Globals::aim_scope_only << "\n";
        out << "aim_soft_visible " << Globals::aim_soft_visible << "\n";
        out << "aim_soft_visible_grace_ms " << Globals::aim_soft_visible_grace_ms << "\n";
        out << "aim_soft_visible_min_bones " << Globals::aim_soft_visible_min_bones << "\n";
        out << "aim_persistent " << Globals::aim_persistent << "\n";
        out << "aim_fov " << Globals::aim_fov << "\n";
        out << "aim_fov_color " << Globals::aim_fov_color[0] << " " << Globals::aim_fov_color[1] << " " << Globals::aim_fov_color[2] << " " << Globals::aim_fov_color[3] << "\n";
        out << "aim_smoothing " << Globals::aim_smoothing << "\n";
        out << "aim_hitbox_head " << Globals::aim_hitbox_head << "\n";
        out << "aim_hitbox_neck " << Globals::aim_hitbox_neck << "\n";
        out << "aim_hitbox_chest " << Globals::aim_hitbox_chest << "\n";
        out << "aim_hitbox_stomach " << Globals::aim_hitbox_stomach << "\n";
        out << "aim_hitbox_pelvis " << Globals::aim_hitbox_pelvis << "\n";
        out << "aim_key " << Globals::aim_key << "\n";
        out << "aim_key_mode " << Globals::aim_key_mode << "\n";
        out << "aim_draw_fov " << Globals::aim_draw_fov << "\n";
        out << "aim_humanize " << Globals::aim_humanize << "\n";
        out << "aim_humanize_strength " << Globals::aim_humanize_strength << "\n";
        out << "aim_humanize_jitter " << Globals::aim_humanize_jitter << "\n";
        out << "aim_humanize_curve " << Globals::aim_humanize_curve << "\n";
        out << "aim_sticky_target " << Globals::aim_sticky_target << "\n";
        out << "aim_switch_delay_ms " << Globals::aim_switch_delay_ms << "\n";
        out << "aim_dynamic_smoothing " << Globals::aim_dynamic_smoothing << "\n";
        out << "aim_smoothing_near " << Globals::aim_smoothing_near << "\n";
        out << "aim_smoothing_far " << Globals::aim_smoothing_far << "\n";
        out << "aim_group_mode independent\n";
        out << "aim_selected_weapon_group " << Globals::aim_selected_weapon_group << "\n";
        for (int i = 0; i < Globals::AIM_GROUP_COUNT; ++i) {
            const auto& group = Globals::aim_weapon_groups[i];
            out << "aim_group " << i << " "
                << group.enabled << " "
                << group.fov << " "
                << group.smoothing << " "
                << group.dynamic_smoothing << " "
                << group.smoothing_near << " "
                << group.smoothing_far << " "
                << group.scope_only << " "
                << group.hitbox_head << " "
                << group.hitbox_neck << " "
                << group.hitbox_chest << " "
                << group.hitbox_stomach << " "
                << group.hitbox_pelvis << "\n";
        }

        out << "rcs_enabled " << Globals::rcs_enabled << "\n";
        out << "rcs_mode " << Globals::rcs_mode << "\n";
        out << "rcs_horizontal " << Globals::rcs_horizontal << "\n";
        out << "rcs_vertical " << Globals::rcs_vertical << "\n";
        out << "rcs_smooth " << Globals::rcs_smooth << "\n";
        out << "rcs_group_mode independent\n";
        for (int i = 0; i < Globals::AIM_GROUP_COUNT; ++i) {
            const auto& group = Globals::rcs_weapon_groups[i];
            out << "rcs_group " << i << " "
                << group.enabled << " "
                << group.mode << " "
                << group.horizontal << " "
                << group.vertical << "\n";
        }

        
        out << "trigger_enabled " << Globals::trigger_enabled << "\n";
        out << "trigger_key " << Globals::trigger_key << "\n";
        out << "trigger_key_mode " << Globals::trigger_key_mode << "\n";
        out << "trigger_ignore_flash " << Globals::trigger_ignore_flash << "\n";

        out << "trigger_delay " << Globals::trigger_delay << "\n";
        out << "trigger_fov " << Globals::trigger_fov << "\n";
        out << "trigger_draw_fov " << Globals::trigger_draw_fov << "\n";
        out << "trigger_fov_color " << Globals::trigger_fov_color[0] << " " << Globals::trigger_fov_color[1] << " " << Globals::trigger_fov_color[2] << " " << Globals::trigger_fov_color[3] << "\n";
        out << "trigger_hitbox_head " << Globals::trigger_hitbox_head << "\n";
        out << "trigger_hitbox_neck " << Globals::trigger_hitbox_neck << "\n";
        out << "trigger_hitbox_chest " << Globals::trigger_hitbox_chest << "\n";
        out << "trigger_hitbox_stomach " << Globals::trigger_hitbox_stomach << "\n";
        out << "trigger_hitbox_pelvis " << Globals::trigger_hitbox_pelvis << "\n";
        out << "trigger_scope_only " << Globals::trigger_scope_only << "\n";
        out << "trigger_random_delay " << Globals::trigger_random_delay << "\n";
        out << "trigger_min_delay " << Globals::trigger_min_delay << "\n";
        out << "trigger_max_delay " << Globals::trigger_max_delay << "\n";
        out << "trigger_velocity_check " << Globals::trigger_velocity_check << "\n";
        out << "target_teammates " << Globals::target_teammates << "\n";
        
        out << "chams_enabled " << Globals::chams_enabled << "\n";
        out << "chams_enabled_team " << Globals::chams_enabled_team << "\n";
        out << "chams_wireframe " << Globals::chams_wireframe << "\n";
        out << "chams_wireframe_team " << Globals::chams_wireframe_team << "\n";
        out << "chams_filled " << Globals::chams_filled << "\n";
        out << "chams_filled_team " << Globals::chams_filled_team << "\n";
        out << "chams_wire_color " << Globals::chams_wire_color[0] << " " << Globals::chams_wire_color[1] << " " << Globals::chams_wire_color[2] << " " << Globals::chams_wire_color[3] << "\n";
        out << "chams_wire_color_vis " << Globals::chams_wire_color_vis[0] << " " << Globals::chams_wire_color_vis[1] << " " << Globals::chams_wire_color_vis[2] << " " << Globals::chams_wire_color_vis[3] << "\n";
        out << "chams_fill_color " << Globals::chams_fill_color[0] << " " << Globals::chams_fill_color[1] << " " << Globals::chams_fill_color[2] << " " << Globals::chams_fill_color[3] << "\n";
        out << "chams_fill_color_vis " << Globals::chams_fill_color_vis[0] << " " << Globals::chams_fill_color_vis[1] << " " << Globals::chams_fill_color_vis[2] << " " << Globals::chams_fill_color_vis[3] << "\n";
        out << "chams_wire_color_team " << Globals::chams_wire_color_team[0] << " " << Globals::chams_wire_color_team[1] << " " << Globals::chams_wire_color_team[2] << " " << Globals::chams_wire_color_team[3] << "\n";
        out << "chams_wire_color_vis_team " << Globals::chams_wire_color_vis_team[0] << " " << Globals::chams_wire_color_vis_team[1] << " " << Globals::chams_wire_color_vis_team[2] << " " << Globals::chams_wire_color_vis_team[3] << "\n";
        out << "chams_fill_color_team " << Globals::chams_fill_color_team[0] << " " << Globals::chams_fill_color_team[1] << " " << Globals::chams_fill_color_team[2] << " " << Globals::chams_fill_color_team[3] << "\n";
        out << "chams_fill_color_vis_team " << Globals::chams_fill_color_vis_team[0] << " " << Globals::chams_fill_color_vis_team[1] << " " << Globals::chams_fill_color_vis_team[2] << " " << Globals::chams_fill_color_vis_team[3] << "\n";

        
        out << "chamsv2_enabled " << Globals::chamsv2_enabled << "\n";
        out << "chamsv2_enabled_team " << Globals::chamsv2_enabled_team << "\n";
        out << "chamsv2_material_type " << Globals::chamsv2_material_type << "\n";
        out << "chamsv2_material_type_team " << Globals::chamsv2_material_type_team << "\n";
        out << "chamsv2_fill_color " << Globals::chamsv2_fill_color[0] << " " << Globals::chamsv2_fill_color[1] << " " << Globals::chamsv2_fill_color[2] << " " << Globals::chamsv2_fill_color[3] << "\n";
        out << "chamsv2_fill_color_vis " << Globals::chamsv2_fill_color_vis[0] << " " << Globals::chamsv2_fill_color_vis[1] << " " << Globals::chamsv2_fill_color_vis[2] << " " << Globals::chamsv2_fill_color_vis[3] << "\n";
        out << "chamsv2_wire_color " << Globals::chamsv2_wire_color[0] << " " << Globals::chamsv2_wire_color[1] << " " << Globals::chamsv2_wire_color[2] << " " << Globals::chamsv2_wire_color[3] << "\n";
        out << "chamsv2_wire_color_vis " << Globals::chamsv2_wire_color_vis[0] << " " << Globals::chamsv2_wire_color_vis[1] << " " << Globals::chamsv2_wire_color_vis[2] << " " << Globals::chamsv2_wire_color_vis[3] << "\n";
        out << "chamsv2_fill_color_team " << Globals::chamsv2_fill_color_team[0] << " " << Globals::chamsv2_fill_color_team[1] << " " << Globals::chamsv2_fill_color_team[2] << " " << Globals::chamsv2_fill_color_team[3] << "\n";
        out << "chamsv2_fill_color_vis_team " << Globals::chamsv2_fill_color_vis_team[0] << " " << Globals::chamsv2_fill_color_vis_team[1] << " " << Globals::chamsv2_fill_color_vis_team[2] << " " << Globals::chamsv2_fill_color_vis_team[3] << "\n";
        out << "chamsv2_wire_color_team " << Globals::chamsv2_wire_color_team[0] << " " << Globals::chamsv2_wire_color_team[1] << " " << Globals::chamsv2_wire_color_team[2] << " " << Globals::chamsv2_wire_color_team[3] << "\n";
        out << "chamsv2_wire_color_vis_team " << Globals::chamsv2_wire_color_vis_team[0] << " " << Globals::chamsv2_wire_color_vis_team[1] << " " << Globals::chamsv2_wire_color_vis_team[2] << " " << Globals::chamsv2_wire_color_vis_team[3] << "\n";

        
        out << "chamsv4_enabled " << Globals::chamsv4_enabled << "\n";
        out << "chamsv4_wallhack " << Globals::chamsv4_wallhack << "\n";
        out << "chamsv4_hidden_color " << Globals::chamsv4_hidden_color[0] << " " << Globals::chamsv4_hidden_color[1] << " " << Globals::chamsv4_hidden_color[2] << " " << Globals::chamsv4_hidden_color[3] << "\n";
        out << "chamsv4_visible_color " << Globals::chamsv4_visible_color[0] << " " << Globals::chamsv4_visible_color[1] << " " << Globals::chamsv4_visible_color[2] << " " << Globals::chamsv4_visible_color[3] << "\n";
        out << "chamsv4_players " << Globals::chamsv4_players << "\n";
        out << "chamsv4_hands " << Globals::chamsv4_hands << "\n";
        out << "chamsv4_gloves " << Globals::chamsv4_gloves << "\n";
        out << "chamsv4_team " << Globals::chamsv4_team << "\n";
        out << "chamsv4_weapons " << Globals::chamsv4_weapons << "\n";
        out << "chamsv4_weapons_guns " << Globals::chamsv4_weapons_guns << "\n";
        out << "chamsv4_weapons_knives " << Globals::chamsv4_weapons_knives << "\n";
        out << "chamsv4_chicken " << Globals::chamsv4_chicken << "\n";
        out << "chamsv4_mat_players " << Globals::chamsv4_mat_players << "\n";
        out << "chamsv4_mat_hands " << Globals::chamsv4_mat_hands << "\n";
        out << "chamsv4_mat_gloves " << Globals::chamsv4_mat_gloves << "\n";
        out << "chamsv4_mat_guns " << Globals::chamsv4_mat_guns << "\n";
        out << "chamsv4_mat_knives " << Globals::chamsv4_mat_knives << "\n";
        out << "chamsv4_mat_chicken " << Globals::chamsv4_mat_chicken << "\n";
        out << "chamsv4_mat_team " << Globals::chamsv4_mat_team << "\n";

        
        out << "menu_accent_color " << 2.0f/255.0f << " " << 132.0f/255.0f << " " << 199.0f/255.0f << " " << 1.0f << "\n";
        out << "menu_bg_color " << Globals::menu_bg_color[0] << " " << Globals::menu_bg_color[1] << " " << Globals::menu_bg_color[2] << " " << Globals::menu_bg_color[3] << "\n";
        out << "menu_sidebar_color " << Globals::menu_sidebar_color[0] << " " << Globals::menu_sidebar_color[1] << " " << Globals::menu_sidebar_color[2] << " " << Globals::menu_sidebar_color[3] << "\n";
        out << "menu_text_main_color " << Globals::menu_text_main_color[0] << " " << Globals::menu_text_main_color[1] << " " << Globals::menu_text_main_color[2] << " " << Globals::menu_text_main_color[3] << "\n";
        out << "menu_text_muted_color " << Globals::menu_text_muted_color[0] << " " << Globals::menu_text_muted_color[1] << " " << Globals::menu_text_muted_color[2] << " " << Globals::menu_text_muted_color[3] << "\n";
        out << "menu_child_bg_color " << Globals::menu_child_bg_color[0] << " " << Globals::menu_child_bg_color[1] << " " << Globals::menu_child_bg_color[2] << " " << Globals::menu_child_bg_color[3] << "\n";
        out << "menu_key " << Globals::menu_key << "\n";


        out << "visuals_fov_enabled " << Globals::visuals_fov_enabled << "\n";
        out << "visuals_fov " << Globals::visuals_fov << "\n";
        out << "wireframe_enemy_enabled " << Globals::wireframe_enemy_enabled << "\n";
        out << "wireframe_color " << Globals::wireframe_color[0] << " " << Globals::wireframe_color[1] << " " << Globals::wireframe_color[2] << " " << Globals::wireframe_color[3] << "\n";
        out << "killparticle_enabled " << Globals::killparticle_enabled << "\n";
        out << "killparticle_type " << Globals::killparticle_type << "\n";
        out << "killparticle_color " << Globals::killparticle_color[0] << " " << Globals::killparticle_color[1] << " " << Globals::killparticle_color[2] << " " << Globals::killparticle_color[3] << "\n";
        out << "hitparticle_enabled " << Globals::hitparticle_enabled << "\n";
        out << "hitparticle_style " << Globals::hitparticle_style << "\n";
        out << "hitparticle_color " << Globals::hitparticle_color[0] << " " << Globals::hitparticle_color[1] << " " << Globals::hitparticle_color[2] << " " << Globals::hitparticle_color[3] << "\n";
        out << "hitparticle_color_override " << Globals::hitparticle_color_override << "\n";
        out << "hitparticle_size " << Globals::hitparticle_size << "\n";
        out << "hitparticle_lifetime " << Globals::hitparticle_lifetime << "\n";
        out << "hitparticle_intensity " << Globals::hitparticle_intensity << "\n";
        out << "hitparticle_glow " << Globals::hitparticle_glow << "\n";
        out << "hitparticle_randomize " << Globals::hitparticle_randomize << "\n";
        out << "hitparticle_distortion " << Globals::hitparticle_distortion << "\n";
        out << "hitparticle_max_per_hit " << Globals::hitparticle_max_per_hit << "\n";
        
        out << "chams_kill_effect " << Globals::chams_kill_effect << "\n";
        out << "chams_kill_effect_style " << Globals::chams_kill_effect_style << "\n";
        out << "chams_kill_effect_duration " << Globals::chams_kill_effect_duration << "\n";
        out << "chams_kill_effect_intensity " << Globals::chams_kill_effect_intensity << "\n";
        out << "chams_kill_effect_edge_color " << Globals::chams_kill_effect_edge_color[0] << " " << Globals::chams_kill_effect_edge_color[1] << " " << Globals::chams_kill_effect_edge_color[2] << " " << Globals::chams_kill_effect_edge_color[3] << "\n";
        
        out << "deathsound_enabled " << Globals::deathsound_enabled << "\n";
        out << "deathsound_selected " << Globals::deathsound_selected << "\n";
        out << "deathsound_volume " << Globals::deathsound_volume << "\n";
        out << "skybox_changer " << Globals::skybox_changer << "\n";
        out << "skybox_color " << Globals::skybox_color[0] << " " << Globals::skybox_color[1] << " " << Globals::skybox_color[2] << " " << Globals::skybox_color[3] << "\n";
        out << "skybox_intensity " << Globals::skybox_intensity << "\n";
        
        out << "world_light_enabled " << Globals::world_light_enabled << "\n";
        out << "world_light_color " << Globals::world_light_color[0] << " " << Globals::world_light_color[1] << " " << Globals::world_light_color[2] << " " << Globals::world_light_color[3] << "\n";
        out << "world_light_intensity " << Globals::world_light_intensity << "\n";
        out << "world_walls_enabled " << Globals::world_walls_enabled << "\n";
        out << "world_walls_color " << Globals::world_walls_color[0] << " " << Globals::world_walls_color[1] << " " << Globals::world_walls_color[2] << " " << Globals::world_walls_color[3] << "\n";
        
        out << "antiflash_enabled " << Globals::antiflash_enabled << "\n";
        
        out << "autoaccept_enabled " << Globals::autoaccept_enabled << "\n";
        
        out << "smoke_color_enabled " << Globals::smoke_color_enabled << "\n";
        out << "smoke_color " << Globals::smoke_color[0] << " " << Globals::smoke_color[1] << " " << Globals::smoke_color[2] << " " << Globals::smoke_color[3] << "\n";
        
        out << "chatspam_enabled " << Globals::chatspam_enabled << "\n";
        out << "chatspam_message " << Globals::chatspam_message << "\n";
        out << "chatspam_interval " << Globals::chatspam_interval << "\n";
        
        out << "bunnyhop_enabled " << Globals::bunnyhop_enabled << "\n";
        out << "grenade_prediction_enabled " << Globals::grenade_prediction_enabled << "\n";
        out << "grenade_prediction_color " << Globals::grenade_prediction_color[0] << " " << Globals::grenade_prediction_color[1] << " " << Globals::grenade_prediction_color[2] << " " << Globals::grenade_prediction_color[3] << "\n";
        out << "grenade_prediction_hit_color " << Globals::grenade_prediction_hit_color[0] << " " << Globals::grenade_prediction_hit_color[1] << " " << Globals::grenade_prediction_hit_color[2] << " " << Globals::grenade_prediction_hit_color[3] << "\n";
        out << "grenade_prediction_thickness " << Globals::grenade_prediction_thickness << "\n";
        out << "grenade_prediction_radius " << Globals::grenade_prediction_radius << "\n";
        out << "grenade_prediction_bounce " << Globals::grenade_prediction_bounce << "\n";
        out << "grenade_prediction_friction " << Globals::grenade_prediction_friction << "\n";
        out << "grenade_prediction_max_bounces " << Globals::grenade_prediction_max_bounces << "\n";
        out << "grenade_prediction_draw_points " << Globals::grenade_prediction_draw_points << "\n";

        
        out << "sc_enabled " << Globals::sc_enabled << "\n";
        out << "sc_selected_knife " << Globals::sc_selected_knife << "\n";
        out << "sc_selected_glove_type " << Globals::sc_selected_glove_type << "\n";
        out << "sc_selected_music_kit " << Globals::sc_selected_music_kit << "\n";
        out << "sc_selected_agent_ct " << Globals::sc_selected_agent_ct << "\n";
        out << "sc_selected_agent_t " << Globals::sc_selected_agent_t << "\n";

        
        if (g_SkinManager) {
            out << "sc_skin_count " << g_SkinManager->Skins.size() << "\n";
            for (auto& skin : g_SkinManager->Skins) {
                out << "sc_skin " << (int)skin.weaponType << " " << skin.paintKit << "\n";
            }
            out << "sc_glove_def " << g_SkinManager->Gloves.defIndex << "\n";
            out << "sc_glove_paint " << g_SkinManager->Gloves.paintKit << "\n";
            out << "sc_music_kit_id " << g_SkinManager->MusicKit.id << "\n";
        }

        std::ofstream file(path);
        if (!file.is_open()) return;

        std::string raw = out.str();
        std::istringstream iss(raw);
        std::string line;

        file << "{\n";
        file << "  \"format\": \"ghostweave-config-v1\",\n";
        file << "  \"lines\": [\n";

        bool first = true;
        while (std::getline(iss, line))
        {
            if (!first) file << ",\n";
            file << "    \"" << EscapeJsonString(line) << "\"";
            first = false;
        }

        file << "\n  ]\n";
        file << "}\n";
        file.close();
        current_config = name;
        Refresh();
    }

    void Load(std::string name)
    {
        name = NormalizeConfigName(name);
        if (name.empty()) return;

        std::filesystem::path path = GetConfigPath() / name;
        std::ifstream in(path, std::ios::binary);
        if (!in.is_open()) return;

        std::stringstream readBuf;
        readBuf << in.rdbuf();
        std::string fileContent = readBuf.str();
        in.close();

        std::string parseContent = fileContent;
        size_t firstNonWs = parseContent.find_first_not_of(" \t\r\n");
        if (firstNonWs != std::string::npos && parseContent[firstNonWs] == '{')
        {
            std::string jsonLines;
            if (ParseJsonLinesArray(parseContent, jsonLines))
                parseContent = jsonLines;
        }

        std::istringstream inParse(parseContent);

        Globals::aim_vis_check = false;
        std::string key;
        bool aimGroupIndependentMode = false;
        bool aimGroupConfigLoaded = false;
        bool aimLegacyConfigLoaded = false;
        bool rcsGroupIndependentMode = false;
        bool rcsGroupConfigLoaded = false;
        while (inParse >> key)
        {
            if (key == "esp_enabled_team") { inParse >> Globals::esp_enabled_team; continue; }
            if (key == "esp_box_team") { inParse >> Globals::esp_box_team; continue; }
            if (key == "esp_box_type_team") { inParse >> Globals::esp_box_type_team; continue; }
            if (key == "esp_skeleton_team") { inParse >> Globals::esp_skeleton_team; continue; }
            if (key == "esp_skeleton_type_team") { inParse >> Globals::esp_skeleton_type_team; continue; }
            if (key == "esp_head_team") { inParse >> Globals::esp_head_team; continue; }
            if (key == "esp_name_team") { inParse >> Globals::esp_name_team; continue; }
            if (key == "esp_distance_team") { inParse >> Globals::esp_distance_team; continue; }
            if (key == "esp_weapon_team") { inParse >> Globals::esp_weapon_team; continue; }
            if (key == "esp_health_team") { inParse >> Globals::esp_health_team; continue; }
            if (key == "esp_sound_enabled_team") { inParse >> Globals::esp_sound_enabled_team; continue; }
            if (key == "chams_enabled_team") { inParse >> Globals::chams_enabled_team; continue; }
            if (key == "chams_wireframe_team") { inParse >> Globals::chams_wireframe_team; continue; }
            if (key == "chams_filled_team") { inParse >> Globals::chams_filled_team; continue; }
            if (key == "chamsv2_enabled_team") { inParse >> Globals::chamsv2_enabled_team; continue; }
            if (key == "esp_box_color_team") { inParse >> Globals::esp_box_color_team[0] >> Globals::esp_box_color_team[1] >> Globals::esp_box_color_team[2] >> Globals::esp_box_color_team[3]; continue; }
            if (key == "esp_box_color_vis_team") { inParse >> Globals::esp_box_color_vis_team[0] >> Globals::esp_box_color_vis_team[1] >> Globals::esp_box_color_vis_team[2] >> Globals::esp_box_color_vis_team[3]; continue; }
            if (key == "esp_skeleton_color_team") { inParse >> Globals::esp_skeleton_color_team[0] >> Globals::esp_skeleton_color_team[1] >> Globals::esp_skeleton_color_team[2] >> Globals::esp_skeleton_color_team[3]; continue; }
            if (key == "esp_skeleton_color_vis_team") { inParse >> Globals::esp_skeleton_color_vis_team[0] >> Globals::esp_skeleton_color_vis_team[1] >> Globals::esp_skeleton_color_vis_team[2] >> Globals::esp_skeleton_color_vis_team[3]; continue; }
            if (key == "esp_name_color_team") { inParse >> Globals::esp_name_color_team[0] >> Globals::esp_name_color_team[1] >> Globals::esp_name_color_team[2] >> Globals::esp_name_color_team[3]; continue; }
            if (key == "esp_name_color_vis_team") { inParse >> Globals::esp_name_color_vis_team[0] >> Globals::esp_name_color_vis_team[1] >> Globals::esp_name_color_vis_team[2] >> Globals::esp_name_color_vis_team[3]; continue; }
            if (key == "esp_distance_color_team") { inParse >> Globals::esp_distance_color_team[0] >> Globals::esp_distance_color_team[1] >> Globals::esp_distance_color_team[2] >> Globals::esp_distance_color_team[3]; continue; }
            if (key == "esp_distance_color_vis_team") { inParse >> Globals::esp_distance_color_vis_team[0] >> Globals::esp_distance_color_vis_team[1] >> Globals::esp_distance_color_vis_team[2] >> Globals::esp_distance_color_vis_team[3]; continue; }
            if (key == "esp_health_color_team") { inParse >> Globals::esp_health_color_team[0] >> Globals::esp_health_color_team[1] >> Globals::esp_health_color_team[2] >> Globals::esp_health_color_team[3]; continue; }
            if (key == "esp_health_color_vis_team") { inParse >> Globals::esp_health_color_vis_team[0] >> Globals::esp_health_color_vis_team[1] >> Globals::esp_health_color_vis_team[2] >> Globals::esp_health_color_vis_team[3]; continue; }
            if (key == "esp_weapon_color_team") { inParse >> Globals::esp_weapon_color_team[0] >> Globals::esp_weapon_color_team[1] >> Globals::esp_weapon_color_team[2] >> Globals::esp_weapon_color_team[3]; continue; }
            if (key == "esp_weapon_color_vis_team") { inParse >> Globals::esp_weapon_color_vis_team[0] >> Globals::esp_weapon_color_vis_team[1] >> Globals::esp_weapon_color_vis_team[2] >> Globals::esp_weapon_color_vis_team[3]; continue; }
            if (key == "esp_sound_color_team") { inParse >> Globals::esp_sound_color_team[0] >> Globals::esp_sound_color_team[1] >> Globals::esp_sound_color_team[2] >> Globals::esp_sound_color_team[3]; continue; }
            if (key == "chams_wire_color_team") { inParse >> Globals::chams_wire_color_team[0] >> Globals::chams_wire_color_team[1] >> Globals::chams_wire_color_team[2] >> Globals::chams_wire_color_team[3]; continue; }
            if (key == "chams_wire_color_vis_team") { inParse >> Globals::chams_wire_color_vis_team[0] >> Globals::chams_wire_color_vis_team[1] >> Globals::chams_wire_color_vis_team[2] >> Globals::chams_wire_color_vis_team[3]; continue; }
            if (key == "chams_fill_color_team") { inParse >> Globals::chams_fill_color_team[0] >> Globals::chams_fill_color_team[1] >> Globals::chams_fill_color_team[2] >> Globals::chams_fill_color_team[3]; continue; }
            if (key == "chams_fill_color_vis_team") { inParse >> Globals::chams_fill_color_vis_team[0] >> Globals::chams_fill_color_vis_team[1] >> Globals::chams_fill_color_vis_team[2] >> Globals::chams_fill_color_vis_team[3]; continue; }
            if (key == "chamsv2_fill_color_team") { inParse >> Globals::chamsv2_fill_color_team[0] >> Globals::chamsv2_fill_color_team[1] >> Globals::chamsv2_fill_color_team[2] >> Globals::chamsv2_fill_color_team[3]; continue; }
            if (key == "chamsv2_fill_color_vis_team") { inParse >> Globals::chamsv2_fill_color_vis_team[0] >> Globals::chamsv2_fill_color_vis_team[1] >> Globals::chamsv2_fill_color_vis_team[2] >> Globals::chamsv2_fill_color_vis_team[3]; continue; }
            if (key == "chamsv2_wire_color_team") { inParse >> Globals::chamsv2_wire_color_team[0] >> Globals::chamsv2_wire_color_team[1] >> Globals::chamsv2_wire_color_team[2] >> Globals::chamsv2_wire_color_team[3]; continue; }
            if (key == "chamsv2_wire_color_vis_team") { inParse >> Globals::chamsv2_wire_color_vis_team[0] >> Globals::chamsv2_wire_color_vis_team[1] >> Globals::chamsv2_wire_color_vis_team[2] >> Globals::chamsv2_wire_color_vis_team[3]; continue; }

            if (key == "esp_enabled") { inParse >> Globals::esp_enabled; continue; }
            if (key == "esp_bind") { inParse >> Globals::esp_bind; continue; }
            if (key == "esp_teammates") { bool legacy_tm; inParse >> legacy_tm; continue; }
            if (key == "esp_box") { inParse >> Globals::esp_box; continue; }
            if (key == "esp_box_type") { inParse >> Globals::esp_box_type; continue; }
            if (key == "esp_box_color") { inParse >> Globals::esp_box_color[0] >> Globals::esp_box_color[1] >> Globals::esp_box_color[2] >> Globals::esp_box_color[3]; continue; }
            if (key == "esp_box_color_vis") { inParse >> Globals::esp_box_color_vis[0] >> Globals::esp_box_color_vis[1] >> Globals::esp_box_color_vis[2] >> Globals::esp_box_color_vis[3]; continue; }
            if (key == "esp_box_thickness") { inParse >> Globals::esp_box_thickness; continue; }
            if (key == "esp_skeleton") { inParse >> Globals::esp_skeleton; continue; }
            if (key == "esp_skeleton_color") { inParse >> Globals::esp_skeleton_color[0] >> Globals::esp_skeleton_color[1] >> Globals::esp_skeleton_color[2] >> Globals::esp_skeleton_color[3]; continue; }
            if (key == "esp_skeleton_color_vis") { inParse >> Globals::esp_skeleton_color_vis[0] >> Globals::esp_skeleton_color_vis[1] >> Globals::esp_skeleton_color_vis[2] >> Globals::esp_skeleton_color_vis[3]; continue; }
            if (key == "esp_skeleton_thickness") { inParse >> Globals::esp_skeleton_thickness; continue; }
            if (key == "esp_skeleton_type") { inParse >> Globals::esp_skeleton_type; continue; }
            if (key == "esp_head") { inParse >> Globals::esp_head; continue; }
            if (key == "esp_name") { inParse >> Globals::esp_name; continue; }
            if (key == "esp_name_color") { inParse >> Globals::esp_name_color[0] >> Globals::esp_name_color[1] >> Globals::esp_name_color[2] >> Globals::esp_name_color[3]; continue; }
            if (key == "esp_name_color_vis") { inParse >> Globals::esp_name_color_vis[0] >> Globals::esp_name_color_vis[1] >> Globals::esp_name_color_vis[2] >> Globals::esp_name_color_vis[3]; continue; }
            if (key == "esp_distance") { inParse >> Globals::esp_distance; continue; }
            if (key == "esp_distance_color") { inParse >> Globals::esp_distance_color[0] >> Globals::esp_distance_color[1] >> Globals::esp_distance_color[2] >> Globals::esp_distance_color[3]; continue; }
            if (key == "esp_distance_color_vis") { inParse >> Globals::esp_distance_color_vis[0] >> Globals::esp_distance_color_vis[1] >> Globals::esp_distance_color_vis[2] >> Globals::esp_distance_color_vis[3]; continue; }
            if (key == "esp_health") { inParse >> Globals::esp_health; continue; }
            if (key == "esp_health_color") { inParse >> Globals::esp_health_color[0] >> Globals::esp_health_color[1] >> Globals::esp_health_color[2] >> Globals::esp_health_color[3]; continue; }
            if (key == "esp_health_color_vis") { inParse >> Globals::esp_health_color_vis[0] >> Globals::esp_health_color_vis[1] >> Globals::esp_health_color_vis[2] >> Globals::esp_health_color_vis[3]; continue; }
            if (key == "esp_weapon") { inParse >> Globals::esp_weapon; continue; }
            if (key == "esp_weapon_color") { inParse >> Globals::esp_weapon_color[0] >> Globals::esp_weapon_color[1] >> Globals::esp_weapon_color[2] >> Globals::esp_weapon_color[3]; continue; }
            if (key == "esp_weapon_color_vis") { inParse >> Globals::esp_weapon_color_vis[0] >> Globals::esp_weapon_color_vis[1] >> Globals::esp_weapon_color_vis[2] >> Globals::esp_weapon_color_vis[3]; continue; }
            if (key == "esp_sound_enabled") { inParse >> Globals::esp_sound_enabled; continue; }
            if (key == "esp_sound_color") { inParse >> Globals::esp_sound_color[0] >> Globals::esp_sound_color[1] >> Globals::esp_sound_color[2] >> Globals::esp_sound_color[3]; continue; }
            if (key == "aim_enabled") { inParse >> Globals::aim_enabled; continue; }
            if (key == "aim_vis_check") { inParse >> Globals::aim_vis_check; continue; }
            if (key == "aim_ignore_flash") { inParse >> Globals::aim_ignore_flash; continue; }
            if (key == "aim_scope_only") { inParse >> Globals::aim_scope_only; aimLegacyConfigLoaded = true; continue; }
            if (key == "aim_soft_visible") { inParse >> Globals::aim_soft_visible; continue; }
            if (key == "aim_soft_visible_grace_ms") { inParse >> Globals::aim_soft_visible_grace_ms; continue; }
            if (key == "aim_soft_visible_min_bones") { inParse >> Globals::aim_soft_visible_min_bones; continue; }
            if (key == "aim_persistent") { inParse >> Globals::aim_persistent; continue; }
            if (key == "aim_fov") { inParse >> Globals::aim_fov; aimLegacyConfigLoaded = true; continue; }
            if (key == "aim_fov_color") { inParse >> Globals::aim_fov_color[0] >> Globals::aim_fov_color[1] >> Globals::aim_fov_color[2] >> Globals::aim_fov_color[3]; continue; }
            if (key == "aim_smoothing") { inParse >> Globals::aim_smoothing; aimLegacyConfigLoaded = true; continue; }
            if (key == "aim_hitbox") { int tmp; inParse >> tmp; continue; }
            if (key == "aim_hitbox_head") { inParse >> Globals::aim_hitbox_head; aimLegacyConfigLoaded = true; continue; }
            if (key == "aim_hitbox_neck") { inParse >> Globals::aim_hitbox_neck; aimLegacyConfigLoaded = true; continue; }
            if (key == "aim_hitbox_chest") { inParse >> Globals::aim_hitbox_chest; aimLegacyConfigLoaded = true; continue; }
            if (key == "aim_hitbox_stomach") { inParse >> Globals::aim_hitbox_stomach; aimLegacyConfigLoaded = true; continue; }
            if (key == "aim_hitbox_pelvis") { inParse >> Globals::aim_hitbox_pelvis; aimLegacyConfigLoaded = true; continue; }
            if (key == "aim_key") { inParse >> Globals::aim_key; continue; }
            if (key == "aim_key_mode") { inParse >> Globals::aim_key_mode; continue; }
            if (key == "aim_draw_fov") { inParse >> Globals::aim_draw_fov; continue; }
            if (key == "aim_humanize") { inParse >> Globals::aim_humanize; continue; }
            if (key == "aim_humanize_strength") { inParse >> Globals::aim_humanize_strength; continue; }
            if (key == "aim_humanize_jitter") { inParse >> Globals::aim_humanize_jitter; continue; }
            if (key == "aim_humanize_curve") { inParse >> Globals::aim_humanize_curve; continue; }
            if (key == "aim_sticky_target") { inParse >> Globals::aim_sticky_target; continue; }
            if (key == "aim_switch_delay_ms") { inParse >> Globals::aim_switch_delay_ms; continue; }
            if (key == "aim_dynamic_smoothing") { inParse >> Globals::aim_dynamic_smoothing; aimLegacyConfigLoaded = true; continue; }
            if (key == "aim_smoothing_near") { inParse >> Globals::aim_smoothing_near; aimLegacyConfigLoaded = true; continue; }
            if (key == "aim_smoothing_far") { inParse >> Globals::aim_smoothing_far; aimLegacyConfigLoaded = true; continue; }
            if (key == "aim_group_mode") {
                std::string mode;
                inParse >> mode;
                aimGroupIndependentMode = mode == "independent";
                continue;
            }
            if (key == "aim_selected_weapon_group") { inParse >> Globals::aim_selected_weapon_group; continue; }
            if (key == "aim_group") {
                int idx = -1;
                inParse >> idx;
                if (idx >= 0 && idx < Globals::AIM_GROUP_COUNT) {
                    aimGroupConfigLoaded = true;
                    auto& group = Globals::aim_weapon_groups[idx];
                    inParse
                        >> group.enabled
                        >> group.fov
                        >> group.smoothing
                        >> group.dynamic_smoothing
                        >> group.smoothing_near
                        >> group.smoothing_far
                        >> group.scope_only
                        >> group.hitbox_head
                        >> group.hitbox_neck
                        >> group.hitbox_chest
                        >> group.hitbox_stomach
                        >> group.hitbox_pelvis;
                } else {
                    std::string discard;
                    std::getline(inParse, discard);
                }
                continue;
            }
            if (key == "rcs_enabled") { inParse >> Globals::rcs_enabled; continue; }
            if (key == "rcs_mode") { inParse >> Globals::rcs_mode; if (Globals::rcs_mode > 1) Globals::rcs_mode = 0; continue; }
            if (key == "rcs_amount") { inParse >> Globals::rcs_amount; continue; }
            if (key == "rcs_horizontal") {
                inParse >> Globals::rcs_horizontal;
                if (Globals::rcs_horizontal <= 2.0f) Globals::rcs_horizontal *= 35.0f;
                for (int i = 0; i < Globals::AIM_GROUP_COUNT; ++i) Globals::rcs_weapon_groups[i].horizontal = Globals::rcs_horizontal;
                continue;
            }
            if (key == "rcs_vertical") {
                inParse >> Globals::rcs_vertical;
                if (Globals::rcs_vertical <= 2.0f) Globals::rcs_vertical *= 45.0f;
                for (int i = 0; i < Globals::AIM_GROUP_COUNT; ++i) Globals::rcs_weapon_groups[i].vertical = Globals::rcs_vertical;
                continue;
            }
            if (key == "rcs_smooth") { inParse >> Globals::rcs_smooth; continue; }
            if (key == "rcs_calibration") { inParse >> Globals::rcs_calibration; continue; }
            if (key == "rcs_group_mode") {
                std::string mode;
                inParse >> mode;
                rcsGroupIndependentMode = mode == "independent";
                continue;
            }
            if (key == "rcs_group") {
                int idx = -1;
                inParse >> idx;
                if (idx >= 0 && idx < Globals::AIM_GROUP_COUNT) {
                    rcsGroupConfigLoaded = true;
                    auto& group = Globals::rcs_weapon_groups[idx];
                    inParse
                        >> group.enabled
                        >> group.mode
                        >> group.horizontal
                        >> group.vertical;
                    group.mode = std::clamp(group.mode, 0, 1);
                    group.horizontal = std::clamp(group.horizontal, 0.0f, 150.0f);
                    group.vertical = std::clamp(group.vertical, 0.0f, 150.0f);
                } else {
                    std::string discard;
                    std::getline(inParse, discard);
                }
                continue;
            }
            
            if (key == "trigger_enabled") { inParse >> Globals::trigger_enabled; continue; }
            if (key == "trigger_key") { inParse >> Globals::trigger_key; continue; }
            if (key == "trigger_key_mode") { inParse >> Globals::trigger_key_mode; continue; }
            if (key == "trigger_ignore_flash") { inParse >> Globals::trigger_ignore_flash; continue; }
            if (key == "trigger_delay") { inParse >> Globals::trigger_delay; continue; }
            if (key == "trigger_fov") { inParse >> Globals::trigger_fov; continue; }
            if (key == "trigger_draw_fov") { inParse >> Globals::trigger_draw_fov; continue; }
            if (key == "trigger_fov_color") { inParse >> Globals::trigger_fov_color[0] >> Globals::trigger_fov_color[1] >> Globals::trigger_fov_color[2] >> Globals::trigger_fov_color[3]; continue; }
            if (key == "trigger_hitbox_head") { inParse >> Globals::trigger_hitbox_head; continue; }
            if (key == "trigger_hitbox_neck") { inParse >> Globals::trigger_hitbox_neck; continue; }
            if (key == "trigger_hitbox_chest") { inParse >> Globals::trigger_hitbox_chest; continue; }
            if (key == "trigger_hitbox_stomach") { inParse >> Globals::trigger_hitbox_stomach; continue; }
            if (key == "trigger_hitbox_pelvis") { inParse >> Globals::trigger_hitbox_pelvis; continue; }
            if (key == "trigger_scope_only") { inParse >> Globals::trigger_scope_only; continue; }
            if (key == "trigger_random_delay") { inParse >> Globals::trigger_random_delay; continue; }
            if (key == "trigger_min_delay") { inParse >> Globals::trigger_min_delay; continue; }
            if (key == "trigger_max_delay") { inParse >> Globals::trigger_max_delay; continue; }
            if (key == "trigger_velocity_check") { inParse >> Globals::trigger_velocity_check; continue; }
            if (key == "target_teammates") { inParse >> Globals::target_teammates; continue; }
            
            if (key == "chams_enabled") { inParse >> Globals::chams_enabled; continue; }
            if (key == "chams_wireframe") { inParse >> Globals::chams_wireframe; continue; }
            if (key == "chams_filled") { inParse >> Globals::chams_filled; continue; }
            if (key == "chams_wire_color") { inParse >> Globals::chams_wire_color[0] >> Globals::chams_wire_color[1] >> Globals::chams_wire_color[2] >> Globals::chams_wire_color[3]; continue; }
            if (key == "chams_wire_color_vis") { inParse >> Globals::chams_wire_color_vis[0] >> Globals::chams_wire_color_vis[1] >> Globals::chams_wire_color_vis[2] >> Globals::chams_wire_color_vis[3]; continue; }
            if (key == "chams_fill_color") { inParse >> Globals::chams_fill_color[0] >> Globals::chams_fill_color[1] >> Globals::chams_fill_color[2] >> Globals::chams_fill_color[3]; continue; }
            if (key == "chams_fill_color_vis") { inParse >> Globals::chams_fill_color_vis[0] >> Globals::chams_fill_color_vis[1] >> Globals::chams_fill_color_vis[2] >> Globals::chams_fill_color_vis[3]; continue; }
            
            if (key == "chamsv2_enabled") { inParse >> Globals::chamsv2_enabled; continue; }
            if (key == "chamsv2_wireframe") { bool legacy_wireframe; inParse >> legacy_wireframe; continue; }
            if (key == "chamsv2_material_type") { inParse >> Globals::chamsv2_material_type; continue; }
            if (key == "chamsv2_material_type_team") { inParse >> Globals::chamsv2_material_type_team; continue; }
            if (key == "chamsv2_fill_color") { inParse >> Globals::chamsv2_fill_color[0] >> Globals::chamsv2_fill_color[1] >> Globals::chamsv2_fill_color[2] >> Globals::chamsv2_fill_color[3]; continue; }
            if (key == "chamsv2_fill_color_vis") { inParse >> Globals::chamsv2_fill_color_vis[0] >> Globals::chamsv2_fill_color_vis[1] >> Globals::chamsv2_fill_color_vis[2] >> Globals::chamsv2_fill_color_vis[3]; continue; }
            if (key == "chamsv2_wire_color") { inParse >> Globals::chamsv2_wire_color[0] >> Globals::chamsv2_wire_color[1] >> Globals::chamsv2_wire_color[2] >> Globals::chamsv2_wire_color[3]; continue; }
            if (key == "chamsv2_wire_color_vis") { inParse >> Globals::chamsv2_wire_color_vis[0] >> Globals::chamsv2_wire_color_vis[1] >> Globals::chamsv2_wire_color_vis[2] >> Globals::chamsv2_wire_color_vis[3]; continue; }
            
            if (key == "chamsv4_enabled") { inParse >> Globals::chamsv4_enabled; continue; }
            if (key == "chamsv4_wallhack") { inParse >> Globals::chamsv4_wallhack; continue; }
            if (key == "chamsv4_material_type") { int tmp; inParse >> tmp; continue; } 
            if (key == "chamsv4_hidden_color") { inParse >> Globals::chamsv4_hidden_color[0] >> Globals::chamsv4_hidden_color[1] >> Globals::chamsv4_hidden_color[2] >> Globals::chamsv4_hidden_color[3]; continue; }
            if (key == "chamsv4_visible_color") { inParse >> Globals::chamsv4_visible_color[0] >> Globals::chamsv4_visible_color[1] >> Globals::chamsv4_visible_color[2] >> Globals::chamsv4_visible_color[3]; continue; }
            if (key == "chamsv4_players") { inParse >> Globals::chamsv4_players; continue; }
            if (key == "chamsv4_hands") { inParse >> Globals::chamsv4_hands; continue; }
            if (key == "chamsv4_gloves") { inParse >> Globals::chamsv4_gloves; continue; }
            if (key == "chamsv4_team") { inParse >> Globals::chamsv4_team; continue; }
            if (key == "chamsv4_weapons") { inParse >> Globals::chamsv4_weapons; continue; }
            if (key == "chamsv4_weapons_guns") { inParse >> Globals::chamsv4_weapons_guns; continue; }
            if (key == "chamsv4_weapons_knives") { inParse >> Globals::chamsv4_weapons_knives; continue; }
            if (key == "chamsv4_chicken") { inParse >> Globals::chamsv4_chicken; continue; }
            if (key == "chamsv4_mat_players") { inParse >> Globals::chamsv4_mat_players; continue; }
            if (key == "chamsv4_mat_hands") { inParse >> Globals::chamsv4_mat_hands; continue; }
            if (key == "chamsv4_mat_gloves") { inParse >> Globals::chamsv4_mat_gloves; continue; }
            if (key == "chamsv4_mat_guns") { inParse >> Globals::chamsv4_mat_guns; continue; }
            if (key == "chamsv4_mat_knives") { inParse >> Globals::chamsv4_mat_knives; continue; }
            if (key == "chamsv4_mat_chicken") { inParse >> Globals::chamsv4_mat_chicken; continue; }
            if (key == "chamsv4_mat_team") { inParse >> Globals::chamsv4_mat_team; continue; }
            
            if (key == "menu_accent_color") { 
                inParse >> Globals::menu_accent_color[0] >> Globals::menu_accent_color[1] >> Globals::menu_accent_color[2] >> Globals::menu_accent_color[3]; 
                // Auto-correct old pink color to new blue
                if (Globals::menu_accent_color[0] > 0.9f && Globals::menu_accent_color[1] < 0.5f && Globals::menu_accent_color[2] > 0.6f) {
                    Globals::menu_accent_color[0] = 2.0f / 255.0f;
                    Globals::menu_accent_color[1] = 132.0f / 255.0f;
                    Globals::menu_accent_color[2] = 199.0f / 255.0f;
                }
                continue; 
            }
            if (key == "menu_bg_color") { inParse >> Globals::menu_bg_color[0] >> Globals::menu_bg_color[1] >> Globals::menu_bg_color[2] >> Globals::menu_bg_color[3]; continue; }
            if (key == "menu_sidebar_color") { inParse >> Globals::menu_sidebar_color[0] >> Globals::menu_sidebar_color[1] >> Globals::menu_sidebar_color[2] >> Globals::menu_sidebar_color[3]; continue; }
            if (key == "menu_text_main_color") { inParse >> Globals::menu_text_main_color[0] >> Globals::menu_text_main_color[1] >> Globals::menu_text_main_color[2] >> Globals::menu_text_main_color[3]; continue; }
            if (key == "menu_text_muted_color") { inParse >> Globals::menu_text_muted_color[0] >> Globals::menu_text_muted_color[1] >> Globals::menu_text_muted_color[2] >> Globals::menu_text_muted_color[3]; continue; }
            if (key == "menu_child_bg_color") { inParse >> Globals::menu_child_bg_color[0] >> Globals::menu_child_bg_color[1] >> Globals::menu_child_bg_color[2] >> Globals::menu_child_bg_color[3]; continue; }
            if (key == "menu_key") { inParse >> Globals::menu_key; continue; }
            
            if (key == "visuals_fov_enabled") { inParse >> Globals::visuals_fov_enabled; continue; }
            if (key == "visuals_fov") { inParse >> Globals::visuals_fov; continue; }
            if (key == "wireframe_enemy_enabled") { inParse >> Globals::wireframe_enemy_enabled; continue; }
            if (key == "wireframe_enabled") { inParse >> Globals::wireframe_enemy_enabled; continue; }
            if (key == "wireframe_color") { inParse >> Globals::wireframe_color[0] >> Globals::wireframe_color[1] >> Globals::wireframe_color[2] >> Globals::wireframe_color[3]; continue; }
            
            if (key == "killparticle_enabled") { inParse >> Globals::killparticle_enabled; continue; }
            if (key == "killparticle_type") { inParse >> Globals::killparticle_type; continue; }
            if (key == "killparticle_color") { inParse >> Globals::killparticle_color[0] >> Globals::killparticle_color[1] >> Globals::killparticle_color[2] >> Globals::killparticle_color[3]; continue; }
            
            if (key == "hitparticle_enabled") { inParse >> Globals::hitparticle_enabled; continue; }
            if (key == "hitparticle_style") { inParse >> Globals::hitparticle_style; continue; }
            if (key == "hitparticle_color") { inParse >> Globals::hitparticle_color[0] >> Globals::hitparticle_color[1] >> Globals::hitparticle_color[2] >> Globals::hitparticle_color[3]; continue; }
            if (key == "hitparticle_color_override") { inParse >> Globals::hitparticle_color_override; continue; }
            if (key == "hitparticle_size") { inParse >> Globals::hitparticle_size; continue; }
            if (key == "hitparticle_lifetime") { inParse >> Globals::hitparticle_lifetime; continue; }
            if (key == "hitparticle_intensity") { inParse >> Globals::hitparticle_intensity; continue; }
            if (key == "hitparticle_glow") { inParse >> Globals::hitparticle_glow; continue; }
            if (key == "hitparticle_randomize") { inParse >> Globals::hitparticle_randomize; continue; }
            if (key == "hitparticle_distortion") { inParse >> Globals::hitparticle_distortion; continue; }
            if (key == "hitparticle_max_per_hit") { inParse >> Globals::hitparticle_max_per_hit; continue; }
            
            if (key == "chams_kill_effect") { inParse >> Globals::chams_kill_effect; continue; }
            if (key == "chams_kill_effect_style") { inParse >> Globals::chams_kill_effect_style; continue; }
            if (key == "chams_kill_effect_duration") { inParse >> Globals::chams_kill_effect_duration; continue; }
            if (key == "chams_kill_effect_intensity") { inParse >> Globals::chams_kill_effect_intensity; continue; }
            if (key == "chams_kill_effect_edge_color") { inParse >> Globals::chams_kill_effect_edge_color[0] >> Globals::chams_kill_effect_edge_color[1] >> Globals::chams_kill_effect_edge_color[2] >> Globals::chams_kill_effect_edge_color[3]; continue; }
            
            if (key == "deathsound_enabled") { inParse >> Globals::deathsound_enabled; continue; }
            if (key == "deathsound_selected") { inParse >> Globals::deathsound_selected; continue; }
            if (key == "deathsound_volume") { inParse >> Globals::deathsound_volume; continue; }
            
            if (key == "skybox_changer") { inParse >> Globals::skybox_changer; continue; }
            if (key == "skybox_color") { inParse >> Globals::skybox_color[0] >> Globals::skybox_color[1] >> Globals::skybox_color[2] >> Globals::skybox_color[3]; continue; }
            if (key == "skybox_intensity") { inParse >> Globals::skybox_intensity; continue; }
            
            if (key == "world_light_enabled") { inParse >> Globals::world_light_enabled; continue; }
            if (key == "world_light_color") { inParse >> Globals::world_light_color[0] >> Globals::world_light_color[1] >> Globals::world_light_color[2] >> Globals::world_light_color[3]; continue; }
            if (key == "world_light_intensity") { inParse >> Globals::world_light_intensity; continue; }
            if (key == "world_walls_enabled") { inParse >> Globals::world_walls_enabled; continue; }
            if (key == "world_walls_color") { inParse >> Globals::world_walls_color[0] >> Globals::world_walls_color[1] >> Globals::world_walls_color[2] >> Globals::world_walls_color[3]; continue; }
            
            if (key == "antiflash_enabled") { inParse >> Globals::antiflash_enabled; continue; }
            
            if (key == "autoaccept_enabled") { inParse >> Globals::autoaccept_enabled; continue; }
            
            if (key == "smoke_color_enabled") { inParse >> Globals::smoke_color_enabled; continue; }
            if (key == "smoke_color") { inParse >> Globals::smoke_color[0] >> Globals::smoke_color[1] >> Globals::smoke_color[2] >> Globals::smoke_color[3]; continue; }
            
            if (key == "chatspam_enabled") { inParse >> Globals::chatspam_enabled; continue; }
            if (key == "chatspam_message") { std::string rest; std::getline(inParse, rest); if (!rest.empty() && rest[0] == ' ') rest = rest.substr(1); strncpy_s(Globals::chatspam_message, rest.c_str(), sizeof(Globals::chatspam_message) - 1); continue; }
            if (key == "chatspam_interval") { inParse >> Globals::chatspam_interval; continue; }
            
            if (key == "bunnyhop_enabled") { inParse >> Globals::bunnyhop_enabled; continue; }
            
            if (key == "grenade_prediction_enabled") { inParse >> Globals::grenade_prediction_enabled; continue; }
            if (key == "grenade_prediction_color") { inParse >> Globals::grenade_prediction_color[0] >> Globals::grenade_prediction_color[1] >> Globals::grenade_prediction_color[2] >> Globals::grenade_prediction_color[3]; continue; }
            if (key == "grenade_prediction_hit_color") { inParse >> Globals::grenade_prediction_hit_color[0] >> Globals::grenade_prediction_hit_color[1] >> Globals::grenade_prediction_hit_color[2] >> Globals::grenade_prediction_hit_color[3]; continue; }
            if (key == "grenade_prediction_thickness") { inParse >> Globals::grenade_prediction_thickness; continue; }
            if (key == "grenade_prediction_radius") { inParse >> Globals::grenade_prediction_radius; continue; }
            if (key == "grenade_prediction_bounce") { inParse >> Globals::grenade_prediction_bounce; continue; }
            if (key == "grenade_prediction_friction") { inParse >> Globals::grenade_prediction_friction; continue; }
            if (key == "grenade_prediction_max_bounces") { inParse >> Globals::grenade_prediction_max_bounces; continue; }
            if (key == "grenade_prediction_draw_points") { inParse >> Globals::grenade_prediction_draw_points; continue; }
            
            if (key == "sc_enabled") { inParse >> Globals::sc_enabled; continue; }
            if (key == "sc_selected_knife") { inParse >> Globals::sc_selected_knife; continue; }
            if (key == "sc_selected_glove_type") { inParse >> Globals::sc_selected_glove_type; continue; }
            if (key == "sc_selected_music_kit") { inParse >> Globals::sc_selected_music_kit; continue; }
            if (key == "sc_selected_agent_ct") { inParse >> Globals::sc_selected_agent_ct; continue; }
            if (key == "sc_selected_agent_t") { inParse >> Globals::sc_selected_agent_t; continue; }
            if (key == "sc_skin_count") {
                int count; inParse >> count;
                if (g_SkinManager) g_SkinManager->Skins.clear();
                continue;
            }
            if (key == "sc_skin") {
                int wepId, paintKit;
                inParse >> wepId >> paintKit;
                if (paintKit > 0 && g_SkinManager) {
                    SkinInfo_t skin;
                    skin.paintKit = paintKit;
                    skin.weaponType = static_cast<WeaponsEnum>(wepId);
                    skin.wear = 0.001f;
                    g_SkinManager->AddSkin(skin);
                }
                continue;
            }
            if (key == "sc_glove_def") {
                uint16_t def; inParse >> def;
                if (g_SkinManager) g_SkinManager->Gloves.defIndex = def;
                continue;
            }
            if (key == "sc_glove_paint") {
                int paint; inParse >> paint;
                if (g_SkinManager) g_SkinManager->Gloves.paintKit = paint;
                continue;
            }
            if (key == "sc_music_kit_id") {
                uint16_t id; inParse >> id;
                if (g_SkinManager) g_SkinManager->MusicKit.id = id;
                continue;
            }
            
            if (key == "sc_selected_knife_skin") { int tmp; inParse >> tmp; continue; }
            if (key == "sc_selected_glove_skin") { int tmp; inParse >> tmp; continue; }
            if (key == "sc_weapon_skin_count") { int tmp; inParse >> tmp; continue; }
            if (key == "sc_weapon_skin") { int a, b; inParse >> a >> b; continue; }
            
            { std::string discard; std::getline(inParse, discard); }
        }

        if (!aimGroupConfigLoaded && aimLegacyConfigLoaded)
        {
            for (int i = 0; i < Globals::AIM_GROUP_COUNT; ++i)
            {
                auto& group = Globals::aim_weapon_groups[i];
                group.enabled = true;
                group.fov = Globals::aim_fov;
                group.smoothing = Globals::aim_smoothing;
                group.dynamic_smoothing = Globals::aim_dynamic_smoothing;
                group.smoothing_near = Globals::aim_smoothing_near;
                group.smoothing_far = Globals::aim_smoothing_far;
                group.scope_only = Globals::aim_scope_only;
                group.hitbox_head = Globals::aim_hitbox_head;
                group.hitbox_neck = Globals::aim_hitbox_neck;
                group.hitbox_chest = Globals::aim_hitbox_chest;
                group.hitbox_stomach = Globals::aim_hitbox_stomach;
                group.hitbox_pelvis = Globals::aim_hitbox_pelvis;
            }
        }
        else if (aimGroupConfigLoaded && !aimGroupIndependentMode)
        {
            for (int i = 0; i < Globals::AIM_GROUP_COUNT; ++i)
                Globals::aim_weapon_groups[i].enabled = true;
        }
        if (rcsGroupConfigLoaded && !rcsGroupIndependentMode)
        {
            for (int i = 0; i < Globals::AIM_GROUP_COUNT; ++i)
                Globals::rcs_weapon_groups[i].enabled = Globals::rcs_enabled;
        }

        Globals::sc_force_update = 3;
        current_config = name;
    }

    void Delete(std::string name)
    {
        name = NormalizeConfigName(name);
        if (name.empty()) return;

        std::filesystem::path path = GetConfigPath() / name;
        if (std::filesystem::exists(path))
        {
            std::filesystem::remove(path);
            if (favorite_config == name)
                ClearFavorite();
            if (current_config == name)
                current_config.clear();
            Refresh();
        }
    }

    void Refresh()
    {
        configs.clear();
        std::filesystem::path path = GetConfigPath();
        if (std::filesystem::exists(path))
        {
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                if (entry.path().extension() == ".json")
                {
                    configs.push_back(entry.path().filename().string());
                }
            }
        }
        LoadFavoriteState();
    }

    void SetFavorite(std::string name)
    {
        name = NormalizeConfigName(name);
        if (name.empty()) return;

        std::filesystem::path configPath = GetConfigPath() / name;
        if (!std::filesystem::exists(configPath))
            return;

        std::ofstream out(GetFavoritePath(), std::ios::trunc);
        if (!out.is_open())
            return;

        out << name << "\n";
        favorite_config = name;
    }

    void ClearFavorite()
    {
        std::filesystem::path path = GetFavoritePath();
        if (std::filesystem::exists(path))
            std::filesystem::remove(path);
        favorite_config.clear();
    }

    void LoadFavoriteState()
    {
        favorite_config.clear();

        std::ifstream in(GetFavoritePath());
        if (!in.is_open())
            return;

        std::string name;
        std::getline(in, name);
        name = NormalizeConfigName(name);
        if (name.empty())
            return;

        if (std::filesystem::exists(GetConfigPath() / name))
            favorite_config = name;
    }

    bool LoadFavorite()
    {
        LoadFavoriteState();
        if (favorite_config.empty())
            return false;

        Load(favorite_config);
        return current_config == favorite_config;
    }
}
