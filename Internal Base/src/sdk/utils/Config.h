#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace Config
{
    void Save(std::string name);
    void Load(std::string name);
    void Delete(std::string name);
    void Refresh();
    void SetFavorite(std::string name);
    void ClearFavorite();
    void LoadFavoriteState();
    bool LoadFavorite();
    
    inline std::vector<std::string> configs;
    inline std::string current_config = "";
    inline std::string favorite_config = "";
    
    std::filesystem::path GetConfigPath();
}
