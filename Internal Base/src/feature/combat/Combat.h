#pragma once

#include "../../sdk/memory/Offsets.h"
#include "../../sdk/memory/PatternScan.h"
#include "../../sdk/utils/Globals.h"
#include "../../sdk/utils/Utils.h"
#include <cstdint>

namespace Combat
{
    inline uintptr_t GetEntityFromHandle(uint32_t handle)
    {
        if (!handle || handle == 0xFFFFFFFF) return 0;

        uintptr_t client = Memory::GetModuleBase("client.dll");
        if (!client) return 0;

        uintptr_t entityList = Utils::SafeRead<uintptr_t>(client + Offsets::dwEntityList);
        if (!Utils::IsValidPtr(entityList)) return 0;

        uintptr_t entry = Utils::SafeRead<uintptr_t>(entityList + 0x8 * ((handle & 0x7FFF) >> 9) + 0x10);
        if (!Utils::IsValidPtr(entry)) return 0;

        return Utils::SafeRead<uintptr_t>(entry + 0x70 * (handle & 0x1FF));
    }

    inline uintptr_t GetActiveWeapon(uintptr_t pawnAddr)
    {
        uintptr_t weaponServices = Utils::SafeRead<uintptr_t>(pawnAddr + Offsets::m_pWeaponServices);
        if (!Utils::IsValidPtr(weaponServices)) return 0;

        uint32_t activeHandle = Utils::SafeRead<uint32_t>(weaponServices + Offsets::m_hActiveWeapon);
        return GetEntityFromHandle(activeHandle);
    }

    inline uint16_t GetWeaponDefinition(uintptr_t weapon)
    {
        if (!Utils::IsValidPtr(weapon)) return 0;
        uintptr_t item = weapon + Offsets::sc_m_AttributeManager + Offsets::sc_m_Item;
        return Utils::SafeRead<uint16_t>(item + Offsets::sc_m_iItemDefinitionIndex);
    }

    inline Globals::AimWeaponGroup GetWeaponGroup(uint16_t defIndex)
    {
        switch (defIndex)
        {
        case WEP_Glock:
        case WEP_UspS:
        case WEP_P2000:
        case WEP_P250:
        case WEP_FiveSeven:
        case WEP_Tec9:
        case WEP_Cz75A:
        case WEP_Deagle:
        case WEP_Revolver:
        case WEP_Elite:
            return Globals::AIM_GROUP_PISTOL;

        case WEP_Mac10:
        case WEP_Mp5SD:
        case WEP_Ump45:
        case WEP_Bizon:
        case WEP_Mp7:
        case WEP_Mp9:
        case WEP_P90:
            return Globals::AIM_GROUP_SMG;

        case WEP_Ak47:
        case WEP_Aug:
        case WEP_Famas:
        case WEP_Galil:
        case WEP_M4A4:
        case WEP_M4A1S:
        case WEP_Sg556:
            return Globals::AIM_GROUP_RIFLE;

        case WEP_Awp:
        case WEP_Ssg08:
        case WEP_G3Sg1:
        case WEP_Scar20:
            return Globals::AIM_GROUP_SNIPER;

        case WEP_M249:
        case WEP_Negev:
            return Globals::AIM_GROUP_HEAVY;

        case WEP_Xm1014:
        case WEP_Mag7:
        case WEP_Sawedoff:
        case WEP_Nova:
            return Globals::AIM_GROUP_SHOTGUN;

        default:
            return Globals::AIM_GROUP_OTHER;
        }
    }
}
