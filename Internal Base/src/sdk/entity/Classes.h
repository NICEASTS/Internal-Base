#pragma once
#include <cstdint>
#include "../memory/Offsets.h"
#include "../utils/Vector.h"
#include "../utils/SafeMemory.h"

#define SCHEMA(type, name, offset) \
    type name() const { \
        if (!this) return type{}; \
        return *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(this) + offset); \
    }

class GlobalVars {
public:
    SCHEMA(float, m_flCurTime, Offsets::m_flCurTime);
    SCHEMA(int, m_iFrameCount, Offsets::m_iFrameCount);
};

class CGameSceneNode
{
public:
    SCHEMA(uintptr_t, m_modelState, Offsets::m_modelState);
    SCHEMA(Vector, m_vecAbsOrigin, Offsets::m_vecAbsOrigin);
};

class CCollisionProperty {
public:
    SCHEMA(Vector, m_vecMins, Offsets::m_vecMins);
    SCHEMA(Vector, m_vecMaxs, Offsets::m_vecMaxs);
};

class C_BaseEntity
{
public:
    SCHEMA(int, m_iHealth, Offsets::m_iHealth);
    SCHEMA(uint8_t, m_iTeamNum, Offsets::m_iTeamNum);
    SCHEMA(uint8_t, m_lifeState, Offsets::m_lifeState);
    SCHEMA(Vector, m_vOldOrigin, Offsets::m_vOldOrigin);
    SCHEMA(uintptr_t, m_pGameSceneNode, Offsets::m_pGameSceneNode);
    SCHEMA(uintptr_t, m_pCollision, Offsets::m_pCollision);

    bool IsVisible() const {
        if (!this) return false;
        uintptr_t spottedStateAddr = reinterpret_cast<uintptr_t>(this) + Offsets::m_entitySpottedState;
        if (!SafeMemory::IsValidPtr(spottedStateAddr)) return false;

        bool bSpotted = SafeMemory::Read<bool>(spottedStateAddr + Offsets::m_bSpotted);
        uint32_t mask = SafeMemory::Read<uint32_t>(spottedStateAddr + Offsets::m_bSpottedByMask);
        
        return bSpotted || (mask != 0);
    }

    bool IsAlive() const { return m_iHealth() > 0; }
};

class C_CSPlayerPawn : public C_BaseEntity
{
public:
    SCHEMA(Vector, m_vecViewOffset, Offsets::m_vecViewOffset);
    SCHEMA(int, m_iShotsFired, Offsets::m_iShotsFired);
    SCHEMA(uintptr_t, m_pWeaponServices, Offsets::m_pWeaponServices);
    SCHEMA(uintptr_t, m_pAimPunchServices, Offsets::m_pAimPunchServices);

    Vector m_aimPunchAngle() const {
        uintptr_t services = m_pAimPunchServices();
        if (!SafeMemory::IsValidPtr(services)) return {};
        return SafeMemory::Read<Vector>(services + Offsets::m_aimPunchAngle);
    }

    bool IsVisible() const {
        return C_BaseEntity::IsVisible();
    }
};

class CPlayer_WeaponServices {
public:
    SCHEMA(uint32_t, m_hActiveWeapon, Offsets::m_hActiveWeapon);
};

class CWeaponVData {
public:
    SCHEMA(uintptr_t, m_szName, 0x640);
};
class C_CSWeaponBase : public C_BaseEntity {
public:
    SCHEMA(uintptr_t, m_VData, Offsets::m_VData);
};

class C_CSPlayerController : public C_BaseEntity
{
public:
    SCHEMA(uint32_t, m_hPlayerPawn, Offsets::m_hPlayerPawn);
    uintptr_t m_iszPlayerNameAddr() const {
        return reinterpret_cast<uintptr_t>(this) + Offsets::m_iszPlayerName;
    }
    SCHEMA(bool, m_bPawnIsAlive, Offsets::m_bPawnIsAlive);
};
