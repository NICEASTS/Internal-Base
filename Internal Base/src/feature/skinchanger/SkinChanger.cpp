#include "SkinChanger.h"
#include "SkinData.h"
#include "SkinDB.h"
#include "SCLogger.h"
#include "../../sdk/memory/Offsets.h"
#include "../../sdk/memory/PatternScan.h"
#include "../../sdk/utils/Utils.h"
#include "../../sdk/utils/Globals.h"
#include "../../sdk/entity/EntityManager.h"
#include "../../../ext/minhook/MinHook.h"
#include <Windows.h>
#include <map>

static int logThrottle = 0;




#include <vector>
#include <mutex>

static std::vector<void*> g_AllocatedAttributeMemory;
static std::mutex g_MemoryMutex;
static const size_t MAX_TRACKED_ALLOCATIONS = 100; 


static void* g_GloveAttrMemory = nullptr;
static bool g_GloveAttributesWritten = false;


static uintptr_t g_LastValidPawn = 0;
static int g_FramesSinceLastValidPawn = 0;





static void SC_SafeVirtualFree(void* mem)
{
    if (!mem) return;
    __try { VirtualFree(mem, 0, MEM_RELEASE); } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static void SC_FreeOldAttributeMemory_NoLock()
{
    
    while (g_AllocatedAttributeMemory.size() > MAX_TRACKED_ALLOCATIONS) {
        void* oldMem = g_AllocatedAttributeMemory.front();
        SC_SafeVirtualFree(oldMem);
        g_AllocatedAttributeMemory.erase(g_AllocatedAttributeMemory.begin());
    }
}

static void SC_FreeOldAttributeMemory()
{
    std::lock_guard<std::mutex> lock(g_MemoryMutex);
    SC_FreeOldAttributeMemory_NoLock();
}

static void SC_CleanupAllMemory()
{
    std::lock_guard<std::mutex> lock(g_MemoryMutex);
    
    g_Logger.Log("SC_CleanupAllMemory: Freeing %d allocations", (int)g_AllocatedAttributeMemory.size());
    
    for (void* mem : g_AllocatedAttributeMemory) {
        SC_SafeVirtualFree(mem);
    }
    g_AllocatedAttributeMemory.clear();
    
    SC_SafeVirtualFree(g_GloveAttrMemory);
    g_GloveAttrMemory = nullptr;
    g_GloveAttributesWritten = false;
}

static void SC_TrackAllocation(void* mem)
{
    if (!mem) return;
    std::lock_guard<std::mutex> lock(g_MemoryMutex);
    g_AllocatedAttributeMemory.push_back(mem);
    
    
    if (g_AllocatedAttributeMemory.size() > MAX_TRACKED_ALLOCATIONS) {
        SC_FreeOldAttributeMemory_NoLock();
    }
}


static const std::map<uint16_t, uint32_t> m_subclassIdMap = {
    {500u, 3933374535u}, {503u, 3787235507u}, {505u, 4046390180u}, {506u, 2047704618u},
    {507u, 1731408398u}, {508u, 1638561588u}, {509u, 2282479884u}, {512u, 3412259219u},
    {514u, 2511498851u}, {515u, 1353709123u}, {516u, 4269888884u}, {517u, 1105782941u},
    {518u, 275962944u}, {519u, 1338637359u}, {520u, 3230445913u}, {521u, 3206681373u},
    {522u, 2595277776u}, {523u, 4029975521u}, {524u, 2463111489u}, {525u, 365028728u},
    {526u, 3845286452u}
};




static uintptr_t fnRegenerateWeaponSkins = 0;
static uintptr_t fnUpdateSubClass = 0;
static uintptr_t fnSetMeshGroupMask = 0;
static uintptr_t fnSetBodyGroup = 0;
static bool g_SetModelHookInstalled = false;
static SkinChanger::fnSetModel_t oSetModel = nullptr;

static bool SC_ShouldOverrideKnifeModel(const char* currentModel)
{
    if (!currentModel || !Globals::sc_enabled) return false;
    int knifeIdx = Globals::sc_selected_knife;
    if (knifeIdx <= 0 || knifeIdx > (int)Knives.size()) return false;

    
    return strstr(currentModel, "weapon_knife") != nullptr || strstr(currentModel, "knife_default") != nullptr;
}

static const char* SC_GetSelectedKnifeModel()
{
    int knifeIdx = Globals::sc_selected_knife;
    if (knifeIdx <= 0 || knifeIdx > (int)Knives.size()) return nullptr;
    const char* model = Knives[knifeIdx - 1].model.c_str();
    return (model && model[0]) ? model : nullptr;
}

static void __fastcall hkSetModel(void* entity, const char* modelPath)
{
    if (!oSetModel || !entity) return;

    const char* finalModel = modelPath;
    if (SC_ShouldOverrideKnifeModel(modelPath)) {
        const char* forcedKnifeModel = SC_GetSelectedKnifeModel();
        if (forcedKnifeModel)
            finalModel = forcedKnifeModel;
    }

    oSetModel(entity, finalModel);
}

static bool SafeSetupInternal()
{
    __try
    {
        uintptr_t setModelAddr = Memory::PatternScan("client.dll",
            "40 53 48 83 EC ? 48 8B D9 4C 8B C2 48 8B 0D ? ? ? ? 48 8D 54 24");
        if (setModelAddr) {
            SkinChanger::fnSetModel = reinterpret_cast<SkinChanger::fnSetModel_t>(setModelAddr);
            g_Logger.Log("SetModel found at: 0x%llX", setModelAddr);
        } else {
            g_Logger.Log("ERROR: SetModel pattern NOT FOUND!");
        }

        uintptr_t regenAddr = Memory::PatternScan("client.dll", "48 83 EC ? E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8B 10");
        if (regenAddr) {
            fnRegenerateWeaponSkins = regenAddr;
            g_Logger.Log("RegenerateWeaponSkins found at: 0x%llX", regenAddr);
            
            
            uint16_t patchValue = Offsets::sc_m_AttributeManager + Offsets::sc_m_Item + Offsets::sc_m_AttributeList + Offsets::sc_m_Attributes;
            DWORD oldProtect;
            if (VirtualProtect((void*)(fnRegenerateWeaponSkins + 0x52), sizeof(uint16_t), PAGE_EXECUTE_READWRITE, &oldProtect)) {
                *(uint16_t*)(fnRegenerateWeaponSkins + 0x52) = patchValue;
                VirtualProtect((void*)(fnRegenerateWeaponSkins + 0x52), sizeof(uint16_t), oldProtect, &oldProtect);
                g_Logger.Log("Patched RegenerateWeaponSkins offset with 0x%X", patchValue);
            }
        } else {
            g_Logger.Log("ERROR: RegenerateWeaponSkins pattern NOT FOUND!");
        }

        uintptr_t updateSubClassAddr = Memory::PatternScan("client.dll", "4C 8B DC 53 48 81 EC ? ? ? ? 48 8B 41");
        if (updateSubClassAddr) {
            fnUpdateSubClass = updateSubClassAddr;
            g_Logger.Log("UpdateSubClass found at: 0x%llX", updateSubClassAddr);
        } else {
            g_Logger.Log("ERROR: UpdateSubClass pattern NOT FOUND!");
        }

        
        uintptr_t setMeshGroupMaskAddr = Memory::PatternScan("client.dll",
            "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8D 99 ? ? ? ? 48 8B 71");
        if (setMeshGroupMaskAddr) {
            fnSetMeshGroupMask = setMeshGroupMaskAddr;
            g_Logger.Log("SetMeshGroupMask found at: 0x%llX", setMeshGroupMaskAddr);
        } else {
            g_Logger.Log("ERROR: SetMeshGroupMask pattern NOT FOUND!");
        }

        
        uintptr_t setBodyGroupAddr = Memory::PatternScan("client.dll",
            "85 D2 0F 88 ? ? ? ? 55 53");
        if (setBodyGroupAddr) {
            fnSetBodyGroup = setBodyGroupAddr;
            g_Logger.Log("SetBodyGroup found at: 0x%llX", setBodyGroupAddr);
        } else {
            g_Logger.Log("ERROR: SetBodyGroup pattern NOT FOUND!");
        }

        SkinChanger::g_Initialized = true;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        g_Logger.Log("EXCEPTION in SafeSetupInternal!");
        SkinChanger::g_Initialized = false;
        return false;
    }
}

static void SafeCallSetModel(void* entity, const char* model)
{
    if (!SkinChanger::fnSetModel || !entity || !model) return;
    __try {
        auto pfn = (void(__fastcall*)(void*, const char*))SkinChanger::fnSetModel;
        pfn(entity, model);
    } __except ((g_Logger.Log("CRASH in fnSetModel! code=0x%08X entity=0x%llX model=%s", GetExceptionCode(), (uintptr_t)entity, model), EXCEPTION_EXECUTE_HANDLER)) {
    }
}

static void SafeCallUpdateSubClass(void* entity)
{
    if (!fnUpdateSubClass || !entity) return;
    __try {
        auto pfn = (void(__fastcall*)(void*))fnUpdateSubClass;
        pfn(entity);
    } __except ((g_Logger.Log("CRASH in fnUpdateSubClass! code=0x%08X entity=0x%llX", GetExceptionCode(), (uintptr_t)entity), EXCEPTION_EXECUTE_HANDLER)) {
    }
}

static void SafeCallSetMeshGroupMask(void* sceneNode, uint64_t mask)
{
    if (!fnSetMeshGroupMask || !sceneNode) return;
    __try {
        auto pfn = (void(__fastcall*)(void*, uint64_t))fnSetMeshGroupMask;
        pfn(sceneNode, mask);
    } __except ((g_Logger.Log("CRASH in fnSetMeshGroupMask! code=0x%08X node=0x%llX mask=%llu", GetExceptionCode(), (uintptr_t)sceneNode, mask), EXCEPTION_EXECUTE_HANDLER)) {
    }
}

static void SafeCallSetBodyGroup(void* entity, uint64_t group, uint64_t value)
{
    if (!fnSetBodyGroup || !entity) return;
    __try {
        auto pfn = (void(__fastcall*)(void*, uint64_t, uint64_t))fnSetBodyGroup;
        pfn(entity, group, value);
    } __except ((g_Logger.Log("CRASH in fnSetBodyGroup! code=0x%08X entity=0x%llX group=%llu value=%llu", GetExceptionCode(), (uintptr_t)entity, group, value), EXCEPTION_EXECUTE_HANDLER)) {
    }
}








static uint64_t SC_GetMeshGroupMask(int paintKit, int weaponDefIdx = 0)
{
    if (paintKit <= 0) return 1; 
    uint64_t mask = (paintKit < 1171) ? 2 : 1;
    
    
    if (weaponDefIdx != 0 && Globals::sc_fix_skin_weapons.count(weaponDefIdx)) {
        mask = (mask == 2) ? 1 : 2;
        if (logThrottle % 300 == 0) {
            g_Logger.Log("  [FIX SKIN] Inverted mask for weapon def %d, paintKit=%d -> mask=%llu", weaponDefIdx, paintKit, mask);
        }
    }
    
    return mask;
}



static int SC_GetPaintKitForWeapon(WeaponsEnum wepType)
{
    if (!g_SkinManager) return 0;
    SkinInfo_t skin = g_SkinManager->GetSkin(wepType);
    return skin.paintKit;
}

static int SC_GetKnifePaintKit()
{
    if (!g_SkinManager) return 0;
    SkinInfo_t skin = g_SkinManager->GetSkin(WEP_CtKnife);
    if (skin.paintKit == 0)
        skin = g_SkinManager->GetSkin(WEP_TKnife);
    return skin.paintKit;
}




static uintptr_t SC_GetEntityFromHandle(uint32_t handle)
{
    if (!handle || handle == 0xFFFFFFFF) return 0;
    uintptr_t client = Memory::GetModuleBase("client.dll");
    if (!client) return 0;
    uintptr_t entityList = Utils::SafeRead<uintptr_t>(client + Offsets::dwEntityList);
    if (!entityList) return 0;
    uintptr_t entry = Utils::SafeRead<uintptr_t>(entityList + 0x8 * ((handle & 0x7FFF) >> 9) + 0x10);
    if (!Utils::IsValidPtr(entry)) return 0;
    return Utils::SafeRead<uintptr_t>(entry + 0x70 * (handle & 0x1FF));
}

static int SC_GetWeaponsRaw(uintptr_t pawn, uintptr_t* outBuffer, int maxCount)
{
    if (!pawn) return 0;
    uintptr_t weaponServices = Utils::SafeRead<uintptr_t>(pawn + Offsets::m_pWeaponServices);
    if (!weaponServices) {
        g_Logger.Log("WeaponServices is NULL (pawn=0x%llX, offset=0x%X)", pawn, Offsets::m_pWeaponServices);
        return 0;
    }
    
    
    
    uint64_t weaponCount = Utils::SafeRead<uint64_t>(weaponServices + Offsets::sc_m_hMyWeapons);
    uintptr_t weaponEntry = Utils::SafeRead<uintptr_t>(weaponServices + Offsets::sc_m_hMyWeapons + sizeof(uint64_t));
    
    if (!weaponEntry || weaponCount == 0 || weaponCount > 64) {
        if (logThrottle % 300 == 0) {
            g_Logger.Log("Bad weapon list: count=%llu entry=0x%llX (ws=0x%llX, offset=0x%X)", 
                weaponCount, weaponEntry, weaponServices, Offsets::sc_m_hMyWeapons);
        }
        return 0;
    }
    if ((int)weaponCount > maxCount) weaponCount = maxCount;
    int count = 0;
    for (uint64_t i = 0; i < weaponCount; i++)
    {
        uint32_t handle = Utils::SafeRead<uint32_t>(weaponEntry + i * sizeof(uint32_t));
        if (!handle || handle == 0xFFFFFFFF) continue;
        uintptr_t weapon = SC_GetEntityFromHandle(handle);
        if (weapon && Utils::IsValidPtr(weapon))
            outBuffer[count++] = weapon;
    }
    if (logThrottle % 300 == 0) {
        g_Logger.Log("GetWeapons: count=%llu resolved=%d entry=0x%llX", weaponCount, count, weaponEntry);
    }
    return count;
}




static uintptr_t SC_GetHudModelWeaponSceneNode(uintptr_t pawn, uintptr_t activeWeapon)
{
    if (!pawn || !activeWeapon) return 0;

    
    uint32_t armsHandle = Utils::SafeRead<uint32_t>(pawn + Offsets::sc_m_hHudModelArms);
    if (!armsHandle || armsHandle == 0xFFFFFFFF) return 0;

    uintptr_t arms = SC_GetEntityFromHandle(armsHandle);
    if (!arms || !Utils::IsValidPtr(arms)) return 0;

    uintptr_t armsSceneNode = Utils::SafeRead<uintptr_t>(arms + Offsets::m_pGameSceneNode);
    if (!armsSceneNode || !Utils::IsValidPtr(armsSceneNode)) return 0;

    
    uintptr_t childNode = Utils::SafeRead<uintptr_t>(armsSceneNode + Offsets::sc_m_pChild);
    while (childNode && Utils::IsValidPtr(childNode))
    {
        
        uintptr_t childOwner = Utils::SafeRead<uintptr_t>(childNode + Offsets::sc_m_pOwner);
        if (childOwner && Utils::IsValidPtr(childOwner))
        {
            
            uint32_t ownerHandle = Utils::SafeRead<uint32_t>(childOwner + Offsets::sc_m_hOwnerEntity);
            if (ownerHandle && ownerHandle != 0xFFFFFFFF)
            {
                uintptr_t resolvedOwner = SC_GetEntityFromHandle(ownerHandle);
                if (resolvedOwner == activeWeapon)
                {
                    
                    
                    uintptr_t hudWeaponSceneNode = Utils::SafeRead<uintptr_t>(childOwner + Offsets::m_pGameSceneNode);
                    if (hudWeaponSceneNode && Utils::IsValidPtr(hudWeaponSceneNode))
                        return hudWeaponSceneNode;
                }
            }
        }
        
        childNode = Utils::SafeRead<uintptr_t>(childNode + Offsets::sc_m_pNextSibling);
    }

    return 0;
}

static uintptr_t SC_GetActiveWeapon(uintptr_t pawn)
{
    if (!pawn) return 0;
    uintptr_t weaponServices = Utils::SafeRead<uintptr_t>(pawn + Offsets::m_pWeaponServices);
    if (!weaponServices) return 0;
    uint32_t activeHandle = Utils::SafeRead<uint32_t>(weaponServices + Offsets::m_hActiveWeapon);
    if (!activeHandle || activeHandle == 0xFFFFFFFF) return 0;
    return SC_GetEntityFromHandle(activeHandle);
}




#pragma pack(push, 1)
struct CEconItemAttribute
{
    uintptr_t vtable = 0;           
    uintptr_t owner = 0;           
    char pad_0010[32] = {};        
    uint16_t defIndex = 0;         
    char pad_0032[2] = {};         
    float value = 0.f;             
    float initValue = 0.f;         
    int32_t refundableCurrency = 0; 
    bool setBonus = false;         
    char pad_0041[7] = {};         
};
#pragma pack(pop)

struct CPtrGameVector
{
    uint64_t size = 0;
    uintptr_t ptr = 0;
};

enum EItemAttributeDef : uint16_t
{
    ATTR_Paint = 6,
    ATTR_Pattern = 7,
    ATTR_Wear = 8,
};

static void SC_WriteAttributes(uintptr_t item, int paintKit, float wear, int seed)
{
    uintptr_t attrListAddr = item + Offsets::sc_m_AttributeList + Offsets::sc_m_Attributes;
    CPtrGameVector existing;
    existing.size = Utils::SafeRead<uint64_t>(attrListAddr);
    existing.ptr = Utils::SafeRead<uintptr_t>(attrListAddr + sizeof(uint64_t));

    
    
    
    if (existing.size != 0 || existing.ptr != 0) {
        g_Logger.Log("  Clearing existing attributes: size=%llu ptr=0x%llX", existing.size, existing.ptr);
        Utils::SafeWrite<uint64_t>(attrListAddr, 0);
        Utils::SafeWrite<uintptr_t>(attrListAddr + sizeof(uint64_t), 0);
    }

    
    static CEconItemAttribute attrBuffer[3];
    
    
    attrBuffer[0] = {};
    attrBuffer[0].defIndex = ATTR_Paint;
    attrBuffer[0].value = (float)paintKit;
    attrBuffer[0].initValue = (float)paintKit;

    
    attrBuffer[1] = {};
    attrBuffer[1].defIndex = ATTR_Pattern;
    attrBuffer[1].value = (float)seed;
    attrBuffer[1].initValue = (float)seed;

    
    attrBuffer[2] = {};
    attrBuffer[2].defIndex = ATTR_Wear;
    attrBuffer[2].value = wear;
    attrBuffer[2].initValue = wear;

    
    void* memBlock = VirtualAlloc(nullptr, sizeof(CEconItemAttribute) * 3, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!memBlock) {
        g_Logger.Log("  VirtualAlloc for attributes FAILED");
        return;
    }
    memcpy(memBlock, attrBuffer, sizeof(CEconItemAttribute) * 3);
    
    
    SC_TrackAllocation(memBlock);

    
    Utils::SafeWrite<uint64_t>(attrListAddr, 3);
    Utils::SafeWrite<uintptr_t>(attrListAddr + sizeof(uint64_t), (uintptr_t)memBlock);

    g_Logger.Log("  Attributes written: memBlock=0x%llX, paint=%d wear=%.3f seed=%d (tracked)", (uintptr_t)memBlock, paintKit, wear, seed);
}

static void SC_RemoveAttributes(uintptr_t item)
{
    uintptr_t attrListAddr = item + Offsets::sc_m_AttributeList + Offsets::sc_m_Attributes;
    CPtrGameVector existing;
    existing.size = Utils::SafeRead<uint64_t>(attrListAddr);
    existing.ptr = Utils::SafeRead<uintptr_t>(attrListAddr + sizeof(uint64_t));

    if (existing.size == 0 || existing.ptr == 0) return;

    
    
    
    Utils::SafeWrite<uint64_t>(attrListAddr, 0);
    Utils::SafeWrite<uintptr_t>(attrListAddr + sizeof(uint64_t), 0);

    g_Logger.Log("  Attributes cleared from game struct (ptr=0x%llX, NOT freed - engine may still reference)", existing.ptr);
}

static void SC_ForceUpdateHud(uintptr_t weapon)
{
    
    uintptr_t identity = Utils::SafeRead<uintptr_t>(weapon + Offsets::sc_m_pEntity);
    if (!identity || !Utils::IsValidPtr(identity)) return;
    
    uint32_t oldFlags = Utils::SafeRead<uint32_t>(identity + Offsets::sc_m_entityFlags);
    Utils::SafeWrite<uint32_t>(identity + Offsets::sc_m_entityFlags, 128); 
    Sleep(1);
    Utils::SafeWrite<uint32_t>(identity + Offsets::sc_m_entityFlags, oldFlags);
    
    g_Logger.Log("  ForceUpdateHud: identity=0x%llX flags %u->128->%u", identity, oldFlags, oldFlags);
}




static void SC_ApplyWeaponSkin(uintptr_t weapon, int paintKit, float wear, int seed, int statTrak)
{
    if (!weapon || paintKit == 0) return;
    uintptr_t item = weapon + Offsets::sc_m_AttributeManager + Offsets::sc_m_Item;
    
    g_Logger.Log("ApplyWeaponSkin: weapon=0x%llX paintKit=%d item=0x%llX (AM=0x%X + Item=0x%X)", 
        weapon, paintKit, item, Offsets::sc_m_AttributeManager, Offsets::sc_m_Item);
    
    
    Utils::SafeWrite<uint32_t>(item + Offsets::sc_m_iItemIDHigh, (uint32_t)-1);
    
    
    Utils::SafeWrite<int32_t>(weapon + Offsets::sc_m_nFallbackPaintKit, paintKit);
    Utils::SafeWrite<float>(weapon + Offsets::sc_m_flFallbackWear, wear);
    Utils::SafeWrite<int32_t>(weapon + Offsets::sc_m_nFallbackSeed, seed);
    if (statTrak >= 0)
        Utils::SafeWrite<int32_t>(weapon + Offsets::sc_m_nFallbackStatTrak, statTrak);
    Utils::SafeWrite<int32_t>(item + Offsets::sc_m_iEntityQuality, statTrak >= 0 ? 9 : 0);
    
    
    Utils::SafeWrite<uint32_t>(weapon + Offsets::sc_m_OriginalOwnerXuidLow, 1);
    Utils::SafeWrite<uint32_t>(item + Offsets::sc_m_iAccountID, 1);
    
    
    SC_WriteAttributes(item, paintKit, wear, seed);
    
    
    uint32_t readBack = Utils::SafeRead<uint32_t>(item + Offsets::sc_m_iItemIDHigh);
    int32_t readPaint = Utils::SafeRead<int32_t>(weapon + Offsets::sc_m_nFallbackPaintKit);
    g_Logger.Log("  Verify: ItemIDHigh=%u (expect -1), PaintKit=%d (expect %d)", readBack, readPaint, paintKit);
}

static void SC_ApplyKnifeModel(uintptr_t pWeaponEntity, int knifeIdx)
{
    if (!SkinChanger::fnSetModel || !pWeaponEntity) {
        g_Logger.Log("ApplyKnifeModel SKIP: fnSetModel=%p entity=0x%llX", SkinChanger::fnSetModel, pWeaponEntity);
        return;
    }
    if (knifeIdx <= 0 || knifeIdx > (int)Knives.size()) {
        g_Logger.Log("ApplyKnifeModel SKIP: bad index %d (max=%d)", knifeIdx, (int)Knives.size());
        return;
    }
    const char* model = Knives[knifeIdx - 1].model.c_str();
    if (!model || model[0] == '\0') {
        g_Logger.Log("ApplyKnifeModel SKIP: empty model for index %d", knifeIdx);
        return;
    }
    g_Logger.Log("ApplyKnifeModel: entity=0x%llX knife=%s model=%s", pWeaponEntity, Knives[knifeIdx - 1].name.c_str(), model);
    SafeCallSetModel(reinterpret_cast<void*>(pWeaponEntity), model);
}


static void SC_ApplyKnifeDefinition(uintptr_t weapon)
{
    if (!weapon || !g_SkinManager) return;
    int knifeIdx = Globals::sc_selected_knife;
    if (knifeIdx <= 0 || knifeIdx > (int)Knives.size()) return;

    uint16_t knifeDef = Knives[knifeIdx - 1].defIndex;
    uintptr_t item = weapon + Offsets::sc_m_AttributeManager + Offsets::sc_m_Item;

    g_Logger.Log("ApplyKnifeDefinition: weapon=0x%llX knifeDef=%d (%s)", weapon, knifeDef, Knives[knifeIdx - 1].name.c_str());

    Utils::SafeWrite<uint32_t>(item + Offsets::sc_m_iItemIDHigh, (uint32_t)-1);
    Utils::SafeWrite<uint16_t>(item + Offsets::sc_m_iItemDefinitionIndex, knifeDef);
    Utils::SafeWrite<int32_t>(item + Offsets::sc_m_iEntityQuality, 3);
    
    
    Utils::SafeWrite<bool>(item + Offsets::sc_m_bRestoreCustomMaterialAfterPrecache, true);
    Utils::SafeWrite<bool>(item + 0x1E9, false); 

    
    auto it = m_subclassIdMap.find(knifeDef);
    if (it != m_subclassIdMap.end()) {
        g_Logger.Log("  [KNIFE] Wrote subclass ID %u to weapon+0x388", it->second);
        Utils::SafeWrite<uint32_t>(weapon + 0x380, it->second); 
        g_Logger.Log("  [KNIFE] Calling SafeCallUpdateSubClass(0x%llX)...", weapon);
        SafeCallUpdateSubClass((void*)weapon);
        g_Logger.Log("  [KNIFE] SafeCallUpdateSubClass returned.");
    }

    
    
    Utils::SafeWrite<uint32_t>(item + Offsets::sc_m_iItemIDHigh, (uint32_t)-1);
    Utils::SafeWrite<uint32_t>(weapon + Offsets::sc_m_OriginalOwnerXuidLow, 1);
    Utils::SafeWrite<uint32_t>(item + Offsets::sc_m_iAccountID, 1);

    SkinInfo_t knifeSkin = g_SkinManager->GetSkin(WEP_CtKnife);
    if (knifeSkin.paintKit == 0)
        knifeSkin = g_SkinManager->GetSkin(WEP_TKnife);

    g_Logger.Log("  KnifeSkin paintKit=%d", knifeSkin.paintKit);

    if (knifeSkin.paintKit > 0) {
        Utils::SafeWrite<int32_t>(weapon + Offsets::sc_m_nFallbackPaintKit, knifeSkin.paintKit);
        Utils::SafeWrite<float>(weapon + Offsets::sc_m_flFallbackWear, 0.001f);
        Utils::SafeWrite<int32_t>(weapon + Offsets::sc_m_nFallbackSeed, 0);
        
        
        
        SC_WriteAttributes(item, knifeSkin.paintKit, 0.001f, 0);
    }

    
    SkinChanger::g_PendingKnifeWeapon = weapon;
    SkinChanger::g_PendingKnifeIndex = knifeIdx;
    g_Logger.Log("  [KNIFE] Stored pending visuals: weapon=0x%llX knifeIdx=%d", weapon, knifeIdx);
}


static void SC_ResetGloveState()
{
    
    
    
    
    if (g_GloveAttributesWritten && SkinChanger::g_PendingGlovePawn) {
        __try {
            uintptr_t econGloves = SkinChanger::g_PendingGlovePawn + Offsets::sc_m_EconGloves;
            uintptr_t attrListAddr = econGloves + Offsets::sc_m_AttributeList + Offsets::sc_m_Attributes;
            
            Utils::SafeWrite<uint64_t>(attrListAddr, 0);
            Utils::SafeWrite<uintptr_t>(attrListAddr + sizeof(uint64_t), 0);
            g_Logger.Log("SC_ResetGloveState: cleared attribute data from game memory (pawn=0x%llX)",
                SkinChanger::g_PendingGlovePawn);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            g_Logger.Log("SC_ResetGloveState: pawn memory already freed, couldn't clear attrs");
        }
    }

    g_GloveAttrMemory = nullptr;
    g_GloveAttributesWritten = false;

    
    SkinChanger::g_PendingGlovePawn = 0;
    SkinChanger::g_PendingGloveWeaponCount = 0;
    SkinChanger::g_PendingGloveApply = false;
    memset(SkinChanger::g_PendingGloveWeapons, 0, sizeof(SkinChanger::g_PendingGloveWeapons));

    g_Logger.Log("SC_ResetGloveState: all state cleared");
}


static void SC_WriteGloveAttributes(uintptr_t econGloves, int paintKit, float wear, int seed)
{
    uintptr_t attrListAddr = econGloves + Offsets::sc_m_AttributeList + Offsets::sc_m_Attributes;
    CPtrGameVector existing;
    existing.size = Utils::SafeRead<uint64_t>(attrListAddr);
    existing.ptr = Utils::SafeRead<uintptr_t>(attrListAddr + sizeof(uint64_t));

    if (existing.size != 0 || existing.ptr != 0) {
        g_Logger.Log("  Glove attributes already exist: size=%llu ptr=0x%llX, skipping", existing.size, existing.ptr);
        return;
    }

    static CEconItemAttribute attrBuffer[3];
    attrBuffer[0] = {};
    attrBuffer[0].defIndex = ATTR_Paint;
    attrBuffer[0].value = (float)paintKit;
    attrBuffer[0].initValue = (float)paintKit;

    attrBuffer[1] = {};
    attrBuffer[1].defIndex = ATTR_Pattern;
    attrBuffer[1].value = (float)seed;
    attrBuffer[1].initValue = (float)seed;

    attrBuffer[2] = {};
    attrBuffer[2].defIndex = ATTR_Wear;
    attrBuffer[2].value = wear;
    attrBuffer[2].initValue = wear;

    void* memBlock = VirtualAlloc(nullptr, sizeof(CEconItemAttribute) * 3, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!memBlock) {
        g_Logger.Log("  VirtualAlloc for glove attributes FAILED");
        return;
    }
    memcpy(memBlock, attrBuffer, sizeof(CEconItemAttribute) * 3);

    
    g_GloveAttrMemory = memBlock;
    g_GloveAttributesWritten = true;
    SC_TrackAllocation(memBlock);

    Utils::SafeWrite<uint64_t>(attrListAddr, 3);
    Utils::SafeWrite<uintptr_t>(attrListAddr + sizeof(uint64_t), (uintptr_t)memBlock);

    g_Logger.Log("  Glove attributes written: memBlock=0x%llX, paint=%d wear=%.3f seed=%d (tracked)", (uintptr_t)memBlock, paintKit, wear, seed);
}


static void SC_RemoveGloveAttributes(uintptr_t econGloves)
{
    uintptr_t attrListAddr = econGloves + Offsets::sc_m_AttributeList + Offsets::sc_m_Attributes;
    CPtrGameVector existing;
    existing.size = Utils::SafeRead<uint64_t>(attrListAddr);
    existing.ptr = Utils::SafeRead<uintptr_t>(attrListAddr + sizeof(uint64_t));

    if (existing.size == 0 || existing.ptr == 0) return;

    
    Utils::SafeWrite<uint64_t>(attrListAddr, 0);
    Utils::SafeWrite<uintptr_t>(attrListAddr + sizeof(uint64_t), 0);

    
    
    g_GloveAttrMemory = nullptr;
    g_GloveAttributesWritten = false;
    g_Logger.Log("  Glove attributes cleared (ptr=0x%llX, NOT freed - engine safety)", existing.ptr);
}

static void SC_ApplyGloves(uintptr_t localPawn, uintptr_t* weapons, int weaponCount)
{
    if (!localPawn || !g_SkinManager) return;
    
    
    int health = Utils::SafeRead<int>(localPawn + Offsets::m_iHealth);
    uint8_t lifeState = Utils::SafeRead<uint8_t>(localPawn + Offsets::m_lifeState);
    if (health <= 0 || lifeState != 0) {
        return;
    }

    if (g_SkinManager->Gloves.defIndex == 0) {
        
        if (g_GloveAttributesWritten) {
            SC_RemoveGloveAttributes(localPawn + Offsets::sc_m_EconGloves);
        }
        if (logThrottle % 300 == 0) g_Logger.Log("ApplyGloves SKIP: Gloves.defIndex == 0");
        return;
    }

    uint16_t gloveDef = g_SkinManager->Gloves.defIndex;
    int glovePaintKit = g_SkinManager->Gloves.paintKit;
    uintptr_t econGloves = localPawn + Offsets::sc_m_EconGloves;

    
    if (!Utils::IsValidPtr(econGloves)) {
        g_Logger.Log("ApplyGloves SKIP: econGloves ptr (0x%llX) is invalid", econGloves);
        return;
    }

    
    uint64_t currentGloveItemID = Utils::SafeRead<uint64_t>(econGloves + Offsets::sc_m_iItemID);
    uint16_t currentDef = Utils::SafeRead<uint16_t>(econGloves + Offsets::sc_m_iItemDefinitionIndex);
    float lastSpawnTime = Utils::SafeRead<float>(localPawn + Offsets::sc_m_flLastSpawnTimeIndex);

    
    static const uint64_t FAKE_GLOVE_ITEM_ID = 68719476735ULL;
    static float s_lastSpawnTime = 0.f;
    static uint16_t s_lastGloveDef = 0;
    static int s_lastGlovePaintKit = 0;
    static uint8_t s_updateFrames = 0;
    static uintptr_t s_lastPawn = 0;
    static uint16_t s_cleanupCountdown = 0;

    
    
    if (s_lastPawn != 0 && s_lastPawn != localPawn) {
        g_Logger.Log("ApplyGloves: Pawn changed (0x%llX -> 0x%llX), resetting glove state",
            s_lastPawn, localPawn);
        s_lastSpawnTime = 0.f;
        s_lastGloveDef = 0;
        s_lastGlovePaintKit = 0;
        s_updateFrames = 0;
        
        g_GloveAttrMemory = nullptr;
        g_GloveAttributesWritten = false;
    }
    s_lastPawn = localPawn;

    bool needsFullUpdate = (Globals::sc_force_update > 0) ||
                           (currentGloveItemID != FAKE_GLOVE_ITEM_ID) ||
                           (lastSpawnTime != s_lastSpawnTime) ||
                           (s_lastGloveDef != gloveDef) ||
                           (s_lastGlovePaintKit != glovePaintKit);

    if (needsFullUpdate)
    {
        g_Logger.Log("ApplyGloves: def=%d paint=%d pawn=0x%llX spawnTime=%.2f",
            gloveDef, glovePaintKit, localPawn, lastSpawnTime);
        g_Logger.Log("  econGloves=0x%llX currentDef=%d currentItemID=%llu",
            econGloves, currentDef, currentGloveItemID);

        s_updateFrames = 3;
        s_cleanupCountdown = 0; 

        Utils::SafeWrite<uint16_t>(econGloves + Offsets::sc_m_iItemDefinitionIndex, gloveDef);

        Utils::SafeWrite<uint64_t>(econGloves + Offsets::sc_m_iItemID, FAKE_GLOVE_ITEM_ID);
        Utils::SafeWrite<uint32_t>(econGloves + Offsets::sc_m_iItemIDHigh, (uint32_t)(FAKE_GLOVE_ITEM_ID >> 32));
        Utils::SafeWrite<uint32_t>(econGloves + Offsets::sc_m_iItemIDLow, (uint32_t)(FAKE_GLOVE_ITEM_ID & 0xFFFFFFFF));

        uint32_t accountID = 1;
        if (weaponCount > 0 && weapons[0] && Utils::IsValidPtr(weapons[0])) {
            uintptr_t weaponItem = weapons[0] + Offsets::sc_m_AttributeManager + Offsets::sc_m_Item;
            uint32_t weaponAccountID = Utils::SafeRead<uint32_t>(weaponItem + Offsets::sc_m_iAccountID);
            if (weaponAccountID != 0)
                accountID = weaponAccountID;
        }
        Utils::SafeWrite<uint32_t>(econGloves + Offsets::sc_m_iAccountID, accountID);

        
        SC_RemoveGloveAttributes(econGloves);
        if (glovePaintKit > 0) {
            SC_WriteGloveAttributes(econGloves, glovePaintKit, 0.001f, 0);
        }

        s_lastSpawnTime = lastSpawnTime;
        s_lastGloveDef = gloveDef;
        s_lastGlovePaintKit = glovePaintKit;
    }

    
    if (s_updateFrames > 0)
    {
        Utils::SafeWrite<bool>(econGloves + Offsets::sc_m_bInitialized, true);
        SafeCallSetBodyGroup((void*)localPawn, 0, 1);
        Utils::SafeWrite<bool>(localPawn + Offsets::sc_m_bNeedToReApplyGloves, true);

        g_Logger.Log("  ApplyGloves frame %d: SetBodyGroup + NeedToReApply", s_updateFrames);
        s_updateFrames--;

        if (s_updateFrames == 0) {
            s_cleanupCountdown = 0; 
        }
    }

    
    
    
    
    
    
    if (s_updateFrames == 0 && g_GloveAttributesWritten && s_cleanupCountdown < 65535)
    {
        s_cleanupCountdown++;
        if (s_cleanupCountdown >= 60) 
        {
            uintptr_t attrListAddr = econGloves + Offsets::sc_m_AttributeList + Offsets::sc_m_Attributes;
            Utils::SafeWrite<uint64_t>(attrListAddr, 0);
            Utils::SafeWrite<uintptr_t>(attrListAddr + sizeof(uint64_t), 0);
            g_GloveAttributesWritten = false;
            s_cleanupCountdown = 65535; 
            g_Logger.Log("  SAFETY: Cleared glove attribute pointer from game memory (prevents tier0 crash)");
        }
    }
}

static void SC_ApplyMusicKit(uintptr_t localController)
{
    if (!localController || !g_SkinManager) return;
    uintptr_t inventoryServices = Utils::SafeRead<uintptr_t>(localController + Offsets::sc_m_pInventoryServices);
    
    if (!inventoryServices)
        inventoryServices = Utils::SafeRead<uintptr_t>(localController + Offsets::sc_m_pInventoryServices + 0x8);
    if (!inventoryServices) {
        if (logThrottle % 300 == 0) {
            g_Logger.Log(
                "MusicKit: inventoryServices is NULL (controller=0x%llX, offset=0x%X, fallback=0x%X)",
                localController, Offsets::sc_m_pInventoryServices, Offsets::sc_m_pInventoryServices + 0x8
            );
        }
        return;
    }
    Utils::SafeWrite<uint16_t>(inventoryServices + Offsets::sc_m_unMusicID, g_SkinManager->MusicKit.id);
}





static void SC_UpdateActiveWeaponDef(uintptr_t localPawn)
{
    if (!localPawn) { Globals::sc_current_weapon_def = WEP_NONE; return; }

    
    uintptr_t activeWeapon = SC_GetActiveWeapon(localPawn);
    if (logThrottle % 300 == 0) {
        g_Logger.Log("ActiveWeapon: pawn=0x%llX, resolved via WeaponServices=0x%llX",
            localPawn, activeWeapon);
    }

    if (!activeWeapon || !Utils::IsValidPtr(activeWeapon)) {
        Globals::sc_current_weapon_def = WEP_NONE;
        if (logThrottle % 300 == 0) {
            g_Logger.Log("  Active weapon pointer is invalid, setting WEP_NONE");
        }
        return;
    }
    
    uintptr_t activeItem = activeWeapon + Offsets::sc_m_AttributeManager + Offsets::sc_m_Item;
    uint16_t rawDefIdx = Utils::SafeRead<uint16_t>(activeItem + Offsets::sc_m_iItemDefinitionIndex);
    Globals::sc_current_weapon_def = static_cast<WeaponsEnum>(rawDefIdx);
    
    if (logThrottle % 300 == 0) {
        g_Logger.Log("  activeItem=0x%llX (weapon + 0x%X + 0x%X), defIdx=%d (%s)",
            activeItem, Offsets::sc_m_AttributeManager, Offsets::sc_m_Item,
            rawDefIdx, WeaponIdToName(rawDefIdx));
    }
}

static bool SafeCallRegenerateWeaponSkins()
{
    if (!fnRegenerateWeaponSkins) return false;
    __try {
        ((void(*)())fnRegenerateWeaponSkins)();
        return true;
    } __except ((g_Logger.Log("EXCEPTION in fnRegenerateWeaponSkins code=0x%08X fn=0x%llX", GetExceptionCode(), fnRegenerateWeaponSkins), EXCEPTION_EXECUTE_HANDLER)) {
        return false;
    }
}




static void SC_RunLogic(uintptr_t client, uintptr_t localPawn, uintptr_t localController)
{
    if (!g_SkinManager) return;
    
    logThrottle++;

    SC_UpdateActiveWeaponDef(localPawn);

    static uintptr_t weaponBuffer[64];
    int weaponCount = SC_GetWeaponsRaw(localPawn, weaponBuffer, 64);

    if (logThrottle % 300 == 0) {
        g_Logger.Log("RunLogic: pawn=0x%llX controller=0x%llX weaponCount=%d forceUpdate=%d", 
            localPawn, localController, weaponCount, Globals::sc_force_update);
        g_Logger.Log("  SkinManager: %d skins, knife=%d, glove def=%d paint=%d, music=%d",
            (int)g_SkinManager->Skins.size(), Globals::sc_selected_knife,
            g_SkinManager->Gloves.defIndex, g_SkinManager->Gloves.paintKit,
            g_SkinManager->MusicKit.id);
    }

    bool shouldUpdate = false;
    bool shouldRegenerateWeaponSkins = false;
    static int lastKnifeWeaponEntry = 0; 

    for (int w = 0; w < weaponCount; w++)
    {
        uintptr_t weapon = weaponBuffer[w];
        uintptr_t item = weapon + Offsets::sc_m_AttributeManager + Offsets::sc_m_Item;
        uint16_t defIndex = Utils::SafeRead<uint16_t>(item + Offsets::sc_m_iItemDefinitionIndex);

        if (IsKnife(defIndex))
        {
            if (Globals::sc_selected_knife > 0 && Globals::sc_selected_knife <= (int)Knives.size())
            {
                uint32_t currentIDHigh = Utils::SafeRead<uint32_t>(item + Offsets::sc_m_iItemIDHigh);
                uint16_t wantedKnifeDef = Knives[Globals::sc_selected_knife - 1].defIndex;
                auto wantedSubclassIt = m_subclassIdMap.find(wantedKnifeDef);
                uint32_t wantedSubclass = (wantedSubclassIt != m_subclassIdMap.end()) ? wantedSubclassIt->second : 0;
                uint32_t currentSubclass = Utils::SafeRead<uint32_t>(weapon + 0x380);
                
                
                
                
                bool knifeDefMismatch = (defIndex != wantedKnifeDef);
                bool knifeSubclassMismatch = (wantedSubclass != 0 && currentSubclass != wantedSubclass);
                if ((Globals::sc_force_update > 0) || knifeDefMismatch || knifeSubclassMismatch) {
                    g_Logger.Log("Applying knife definition to weapon[%d] (defIdx=%d, IDHigh=%u)", w, defIndex, currentIDHigh);
                    SC_ApplyKnifeDefinition(weapon);
                    shouldUpdate = true;
                }

                
                
                
                Utils::SafeWrite<uint32_t>(item + Offsets::sc_m_iItemIDHigh, (uint32_t)-1);
                
                SkinInfo_t knifeSkin = g_SkinManager->GetSkin(WEP_CtKnife);
                if (knifeSkin.paintKit == 0)
                    knifeSkin = g_SkinManager->GetSkin(WEP_TKnife);
                
                if (knifeSkin.paintKit > 0) {
                    Utils::SafeWrite<int32_t>(weapon + Offsets::sc_m_nFallbackPaintKit, knifeSkin.paintKit);
                    Utils::SafeWrite<float>(weapon + Offsets::sc_m_flFallbackWear, 0.001f);
                    Utils::SafeWrite<int32_t>(weapon + Offsets::sc_m_nFallbackSeed, 0);
                }
                
                Utils::SafeWrite<uint32_t>(weapon + Offsets::sc_m_OriginalOwnerXuidLow, 1);
                Utils::SafeWrite<uint32_t>(item + Offsets::sc_m_iAccountID, 1);

                
                SkinChanger::g_PendingKnifeWeapon = weapon;
                SkinChanger::g_PendingKnifeIndex = Globals::sc_selected_knife;
            }
            continue;
        }

        WeaponsEnum weaponType = static_cast<WeaponsEnum>(defIndex);
        SkinInfo_t skin = g_SkinManager->GetSkin(weaponType);
        if (skin.paintKit == 0) continue;

        uint32_t currentIDHigh = Utils::SafeRead<uint32_t>(item + Offsets::sc_m_iItemIDHigh);
        int32_t currentFallbackPK = Utils::SafeRead<int32_t>(weapon + Offsets::sc_m_nFallbackPaintKit);
        
        
        if ((Globals::sc_force_update > 0) || currentIDHigh != (uint32_t)-1 || currentFallbackPK != skin.paintKit) {
            if (logThrottle % 300 == 0 || (Globals::sc_force_update > 0)) {
                g_Logger.Log("Applying weapon skin: weapon[%d] defIdx=%d paintKit=%d (currentPK=%d, IDHigh=%u)", 
                    w, defIndex, skin.paintKit, currentFallbackPK, currentIDHigh);
            }
            SC_ApplyWeaponSkin(weapon, skin.paintKit, skin.wear, skin.seed, skin.statTrak);
            shouldUpdate = true;
            shouldRegenerateWeaponSkins = true; 
        }
    }

    
    if (shouldRegenerateWeaponSkins && fnRegenerateWeaponSkins) {
        bool regenOk = SafeCallRegenerateWeaponSkins();
        if (regenOk) {
            g_Logger.Log("Called fnRegenerateWeaponSkins()");
        }

        
        SkinChanger::g_MeshUpdateFrames = 30;
    }

    
    
    
    
    
    
    if (shouldUpdate) {
        for (int w = 0; w < weaponCount; w++) {
            uintptr_t weapon = weaponBuffer[w];
            uintptr_t item = weapon + Offsets::sc_m_AttributeManager + Offsets::sc_m_Item;

            uintptr_t attrListAddr = item + Offsets::sc_m_AttributeList + Offsets::sc_m_Attributes;
            CPtrGameVector existing;
            existing.size = Utils::SafeRead<uint64_t>(attrListAddr);
            existing.ptr = Utils::SafeRead<uintptr_t>(attrListAddr + sizeof(uint64_t));
            if (existing.size != 0 || existing.ptr != 0) {
                Utils::SafeWrite<uint64_t>(attrListAddr, 0);
                Utils::SafeWrite<uintptr_t>(attrListAddr + sizeof(uint64_t), 0);
            }

            uint16_t defIndex2 = Utils::SafeRead<uint16_t>(item + Offsets::sc_m_iItemDefinitionIndex);

            
            if (IsKnife(defIndex2) && Globals::sc_selected_knife > 0) {
                SkinChanger::g_PendingKnifeWeapon = weapon;
                SkinChanger::g_PendingKnifeIndex = Globals::sc_selected_knife;
            }
        }
    }

    
    
    if (g_SkinManager->Gloves.defIndex > 0) {
        
        int pawnHealth = Utils::SafeRead<int>(localPawn + Offsets::m_iHealth);
        uint8_t pawnLifeState = Utils::SafeRead<uint8_t>(localPawn + Offsets::m_lifeState);
        if (pawnHealth > 0 && pawnLifeState == 0) {
            SkinChanger::g_PendingGlovePawn = localPawn;
            SkinChanger::g_PendingGloveWeaponCount = weaponCount;
            for (int i = 0; i < weaponCount && i < 64; i++)
                SkinChanger::g_PendingGloveWeapons[i] = weaponBuffer[i];
            SkinChanger::g_PendingGloveApply = true;
        } else {
            
            SC_ResetGloveState();
        }
    } else {
        
        if (SkinChanger::g_PendingGloveApply) {
            SC_ResetGloveState();
        }
    }

    SC_ApplyMusicKit(localController);

    
    {
        int playerTeam = Utils::SafeRead<int>(localPawn + Offsets::m_iTeamNum);
        int agentIdx = -1;
        const char* agentModel = nullptr;
        
        if (playerTeam == 3 && Globals::sc_selected_agent_ct >= 0 && Globals::sc_selected_agent_ct < (int)AgentsCT.size()) {
            agentIdx = Globals::sc_selected_agent_ct;
            agentModel = AgentsCT[agentIdx].model.c_str();
        } else if (playerTeam == 2 && Globals::sc_selected_agent_t >= 0 && Globals::sc_selected_agent_t < (int)AgentsT.size()) {
            agentIdx = Globals::sc_selected_agent_t;
            agentModel = AgentsT[agentIdx].model.c_str();
        }
        
        if (agentModel && agentModel[0] != '\0') {
            SkinChanger::g_PendingAgentPawn = localPawn;
            strncpy_s(SkinChanger::g_PendingAgentModel, agentModel, _TRUNCATE);
            SkinChanger::g_PendingAgentApply = true;
        } else {
            SkinChanger::g_PendingAgentApply = false;
            SkinChanger::g_PendingAgentModel[0] = '\0';
        }
    }

    if (Globals::sc_force_update > 0)
        Globals::sc_force_update--;
}


static bool SafeRunOuter()
{
    __try {
        uintptr_t client = Memory::GetModuleBase("client.dll");
        if (!client) {
            SC_ResetGloveState();
            return false;
        }
        uintptr_t localPawn = Utils::SafeRead<uintptr_t>(client + Offsets::dwLocalPlayerPawn);
        uintptr_t localController = Utils::SafeRead<uintptr_t>(client + Offsets::dwLocalPlayerController);
        if (!localPawn || !Utils::IsValidPtr(localPawn)) {
            SC_ResetGloveState();
            return false;
        }
        if (!localController || !Utils::IsValidPtr(localController)) {
            SC_ResetGloveState();
            return false;
        }
        int health = Utils::SafeRead<int>(localPawn + Offsets::m_iHealth);
        uint8_t lifeState = Utils::SafeRead<uint8_t>(localPawn + Offsets::m_lifeState);
        if (health <= 0 || lifeState != 0) {
            SC_ResetGloveState();
            return false;
        }
        
        
        static int cleanupCounter = 0;
        if (++cleanupCounter > 600) {
            SC_FreeOldAttributeMemory();
            cleanupCounter = 0;
        }
        
        SC_RunLogic(client, localPawn, localController);
        return true;
    } __except ((g_Logger.Log("EXCEPTION in SafeRunOuter! code=0x%08X", GetExceptionCode()), EXCEPTION_EXECUTE_HANDLER)) {
        SC_ResetGloveState();
        SC_CleanupAllMemory(); 
        return false;
    }
}




namespace SkinChanger
{
    void Setup()
    {
        if (g_Initialized) return;

        g_Logger.Init();
        g_Logger.Log("SkinChanger::Setup() called");

        if (!g_SkinManager) {
            g_SkinManager = new SkinManager();
            g_Logger.Log("SkinManager created");

            
            
            
            
            
            
            struct { WeaponsEnum wep; int paintKit; } defaultSkins[] = {
                { WEP_Ak47,    1171 }, 
                { WEP_M4A4,    1171 }, 
                { WEP_Deagle,  1171 }, 
                { WEP_Glock,   1171 }, 
                { WEP_UspS,    1171 }, 
            };
            for (auto& ds : defaultSkins) {
                SkinInfo_t autoSkin;
                autoSkin.paintKit = ds.paintKit;
                autoSkin.weaponType = ds.wep;
                autoSkin.wear = 0.001f;
                autoSkin.seed = 0;
                autoSkin.statTrak = -1;
                g_SkinManager->AddSkin(autoSkin);
            }
            g_Logger.Log("Added %d auto default skins for regen trigger", (int)(sizeof(defaultSkins)/sizeof(defaultSkins[0])));
        }
        if (!g_SkinDB) {
            g_SkinDB = new CSkinDB();
            g_Logger.Log("SkinDB created, starting download thread...");
        }

        
        CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
            g_Logger.Log("SkinDB download thread started");
            if (g_SkinDB) {
                g_SkinDB->Dump();
                g_Logger.Log("SkinDB download completed: dumped=%d", g_SkinDB->IsDumped());
            }
            return 0;
        }, nullptr, 0, nullptr);

        g_Logger.Log("Running pattern scan for SetModel...");
        SafeSetupInternal();
        
        g_Logger.Log("Setup complete. fnSetModel=%p, g_Initialized=%d", fnSetModel, g_Initialized);
        
        
        g_Logger.Log("=== OFFSET DUMP ===");
        g_Logger.Log("sc_m_AttributeManager = 0x%X", Offsets::sc_m_AttributeManager);
        g_Logger.Log("sc_m_Item = 0x%X", Offsets::sc_m_Item);
        g_Logger.Log("sc_m_iItemDefinitionIndex = 0x%X", Offsets::sc_m_iItemDefinitionIndex);
        g_Logger.Log("sc_m_iItemIDHigh = 0x%X", Offsets::sc_m_iItemIDHigh);
        g_Logger.Log("sc_m_nFallbackPaintKit = 0x%X", Offsets::sc_m_nFallbackPaintKit);
        g_Logger.Log("sc_m_flFallbackWear = 0x%X", Offsets::sc_m_flFallbackWear);
        g_Logger.Log("sc_m_nFallbackSeed = 0x%X", Offsets::sc_m_nFallbackSeed);
        g_Logger.Log("sc_m_EconGloves = 0x%X", Offsets::sc_m_EconGloves);
        g_Logger.Log("sc_m_bNeedToReApplyGloves = 0x%X", Offsets::sc_m_bNeedToReApplyGloves);
        g_Logger.Log("sc_m_pInventoryServices = 0x%X", Offsets::sc_m_pInventoryServices);
        g_Logger.Log("sc_m_unMusicID = 0x%X", Offsets::sc_m_unMusicID);
        g_Logger.Log("sc_m_hMyWeapons = 0x%X", Offsets::sc_m_hMyWeapons);
        g_Logger.Log("m_pWeaponServices = 0x%X", Offsets::m_pWeaponServices);
        g_Logger.Log("m_pGameSceneNode = 0x%X", Offsets::m_pGameSceneNode);
        g_Logger.Log("fnSetMeshGroupMask = 0x%llX", fnSetMeshGroupMask);
        g_Logger.Log("fnSetBodyGroup = 0x%llX", fnSetBodyGroup);
        g_Logger.Log("sc_m_flLastSpawnTimeIndex = 0x%X", Offsets::sc_m_flLastSpawnTimeIndex);
        g_Logger.Log("sc_m_iItemID = 0x%X", Offsets::sc_m_iItemID);
        g_Logger.Log("sc_m_iItemIDLow = 0x%X", Offsets::sc_m_iItemIDLow);
        g_Logger.Log("sc_m_hOwnerEntity = 0x%X", Offsets::sc_m_hOwnerEntity);
        g_Logger.Log("sc_m_hHudModelArms = 0x%X", Offsets::sc_m_hHudModelArms);
        if (Offsets::sc_m_hOwnerEntity != 0x520)
            g_Logger.Log("WARNING: sc_m_hOwnerEntity mismatch! current=0x%X expected=0x520", Offsets::sc_m_hOwnerEntity);
        if (Offsets::sc_m_hHudModelArms != 0x1B58)
            g_Logger.Log("WARNING: sc_m_hHudModelArms mismatch! current=0x%X expected=0x1B58", Offsets::sc_m_hHudModelArms);
        if (Offsets::sc_m_bNeedToReApplyGloves != 0x1655)
            g_Logger.Log("WARNING: sc_m_bNeedToReApplyGloves mismatch! current=0x%X expected=0x1655", Offsets::sc_m_bNeedToReApplyGloves);
        if (Offsets::sc_m_EconGloves != 0x1658)
            g_Logger.Log("WARNING: sc_m_EconGloves mismatch! current=0x%X expected=0x1658", Offsets::sc_m_EconGloves);
        g_Logger.Log("=== END OFFSET DUMP ===");
    }

    void Run()
    {
        if (!Globals::sc_enabled) return;
        SafeRunOuter();
    }

    void InstallHooks()
    {
        if (g_SetModelHookInstalled) return;
        if (!fnSetModel) {
            g_Logger.Log("InstallHooks: SetModel pointer is null, skipping hook.");
            return;
        }

        MH_STATUS createStatus = MH_CreateHook(
            reinterpret_cast<void*>(fnSetModel),
            &hkSetModel,
            reinterpret_cast<void**>(&oSetModel)
        );

        if (createStatus != MH_OK && createStatus != MH_ERROR_ALREADY_CREATED) {
            g_Logger.Log("InstallHooks: MH_CreateHook(SetModel) failed (%d)", createStatus);
            return;
        }

        MH_STATUS enableStatus = MH_EnableHook(reinterpret_cast<void*>(fnSetModel));
        if (enableStatus != MH_OK && enableStatus != MH_ERROR_ENABLED) {
            g_Logger.Log("InstallHooks: MH_EnableHook(SetModel) failed (%d)", enableStatus);
            return;
        }

        g_SetModelHookInstalled = true;
        g_Logger.Log("InstallHooks: SetModel hook installed. target=0x%llX original=%p", fnSetModel, oSetModel);
    }

    void RunVisuals(int frameStage)
    {
        if (!Globals::sc_enabled || !g_Initialized) return;

        __try {
            uintptr_t client = Memory::GetModuleBase("client.dll");
            if (!client) {
                
                SC_CleanupAllMemory();
                SC_ResetGloveState();
                g_PendingKnifeWeapon = 0;
                g_PendingAgentPawn = 0;
                g_PendingAgentApply = false;
                g_PendingAgentModel[0] = '\0';
                g_LastValidPawn = 0;
                g_FramesSinceLastValidPawn = 0;
                return;
            }

            uintptr_t localPawn = Utils::SafeRead<uintptr_t>(client + Offsets::dwLocalPlayerPawn);
            if (!localPawn || !Utils::IsValidPtr(localPawn)) {
                
                g_FramesSinceLastValidPawn++;
                
                
                if (g_FramesSinceLastValidPawn > 300) {
                    SC_CleanupAllMemory();
                    SC_ResetGloveState();
                    g_PendingKnifeWeapon = 0;
                    g_PendingAgentPawn = 0;
                    g_PendingAgentApply = false;
                    g_PendingAgentModel[0] = '\0';
                    g_LastValidPawn = 0;
                    g_FramesSinceLastValidPawn = 0;
                }
                return;
            }
            
            
            g_FramesSinceLastValidPawn = 0;
            if (g_LastValidPawn != 0 && g_LastValidPawn != localPawn) {
                
                g_Logger.Log("Map change detected (old pawn=0x%llX, new=0x%llX), cleaning up memory", g_LastValidPawn, localPawn);
                SC_CleanupAllMemory();
                SC_ResetGloveState();
                g_PendingKnifeWeapon = 0;
                g_PendingAgentPawn = 0;
                g_PendingAgentApply = false;
                g_PendingAgentModel[0] = '\0';
            }
            g_LastValidPawn = localPawn;

            int health = Utils::SafeRead<int>(localPawn + Offsets::m_iHealth);
            uint8_t lifeState = Utils::SafeRead<uint8_t>(localPawn + Offsets::m_lifeState);
            if (health <= 0 || lifeState != 0) {
                
                g_PendingKnifeWeapon = 0;
                SC_ResetGloveState();
                return;
            }

            
            
            
            if (frameStage == 6)
            {
                
                
                if (g_PendingAgentApply && g_PendingAgentPawn && g_PendingAgentModel[0] != '\0')
                {
                    static float s_lastAgentSpawnTime = -1.f;
                    static int s_lastAgentCT = -1;
                    static int s_lastAgentT = -1;
                    
                    if (g_PendingAgentPawn == localPawn && Utils::IsValidPtr(g_PendingAgentPawn)) {
                        float spawnTime = Utils::SafeRead<float>(localPawn + Offsets::sc_m_flLastSpawnTimeIndex);
                        int playerTeam = Utils::SafeRead<int>(localPawn + Offsets::m_iTeamNum);
                        int currentAgentIdx = (playerTeam == 3) ? Globals::sc_selected_agent_ct : Globals::sc_selected_agent_t;
                        int lastAgentIdx = (playerTeam == 3) ? s_lastAgentCT : s_lastAgentT;
                        
                        bool needsApply = (spawnTime != s_lastAgentSpawnTime) || 
                                          (currentAgentIdx != lastAgentIdx) ||
                                          (Globals::sc_force_update > 0);
                        
                        if (needsApply) {
                            SafeCallSetModel((void*)localPawn, g_PendingAgentModel);
                            s_lastAgentSpawnTime = spawnTime;
                            if (playerTeam == 3) s_lastAgentCT = currentAgentIdx;
                            else s_lastAgentT = currentAgentIdx;
                            g_Logger.Log("[AGENT] Applied model: %s (team=%d, spawnTime=%.2f)", g_PendingAgentModel, playerTeam, spawnTime);
                        }
                    }
                }

                
                
                
                if (g_SkinManager)
                {
                    uintptr_t activeWeaponForMesh = SC_GetActiveWeapon(localPawn);
                    if (activeWeaponForMesh && Utils::IsValidPtr(activeWeaponForMesh))
                    {
                        uintptr_t wepItem = activeWeaponForMesh + Offsets::sc_m_AttributeManager + Offsets::sc_m_Item;
                        uint16_t wepDefIdx = Utils::SafeRead<uint16_t>(wepItem + Offsets::sc_m_iItemDefinitionIndex);

                        if (!IsKnife(wepDefIdx))
                        {
                            WeaponsEnum wepType = static_cast<WeaponsEnum>(wepDefIdx);
                            int wepPaintKit = SC_GetPaintKitForWeapon(wepType);
                            if (wepPaintKit > 0)
                            {
                                uint64_t wepMask = SC_GetMeshGroupMask(wepPaintKit, (int)wepDefIdx);

                                
                                
                                
                                
                                bool needsForceReset = (g_MeshUpdateFrames >= 28);

                                
                                uintptr_t wepSceneNode = Utils::SafeRead<uintptr_t>(activeWeaponForMesh + Offsets::m_pGameSceneNode);
                                if (wepSceneNode && Utils::IsValidPtr(wepSceneNode))
                                {
                                    if (needsForceReset)
                                        SafeCallSetMeshGroupMask((void*)wepSceneNode, 0);
                                    SafeCallSetMeshGroupMask((void*)wepSceneNode, wepMask);
                                }

                                
                                uintptr_t hudWeaponNode = SC_GetHudModelWeaponSceneNode(localPawn, activeWeaponForMesh);
                                if (hudWeaponNode && Utils::IsValidPtr(hudWeaponNode))
                                {
                                    if (needsForceReset)
                                        SafeCallSetMeshGroupMask((void*)hudWeaponNode, 0);
                                    SafeCallSetMeshGroupMask((void*)hudWeaponNode, wepMask);
                                }
                            }
                        }
                    }
                }

                
                if (g_MeshUpdateFrames > 0)
                    g_MeshUpdateFrames--;
            }

            
            
            
            if (frameStage == 7)
            {
                
                int knifeIdx = Globals::sc_selected_knife > 0 ? Globals::sc_selected_knife : g_PendingKnifeIndex;
                if (knifeIdx > 0 && knifeIdx <= (int)Knives.size())
                {
                    const char* model = Knives[knifeIdx - 1].model.c_str();
                    if (model && model[0] != '\0')
                {
                    uintptr_t activeWeapon = SC_GetActiveWeapon(localPawn);
                    uintptr_t weapon = (activeWeapon && Utils::IsValidPtr(activeWeapon)) ? activeWeapon : g_PendingKnifeWeapon;
                    
                    if (weapon && Utils::IsValidPtr(weapon))
                    {
                        uintptr_t item = weapon + Offsets::sc_m_AttributeManager + Offsets::sc_m_Item;
                        uint16_t activeDef = Utils::SafeRead<uint16_t>(item + Offsets::sc_m_iItemDefinitionIndex);
                        
                        if (IsKnife(activeDef))
                        {
                            g_PendingKnifeWeapon = weapon;
                            g_PendingKnifeIndex = knifeIdx;

                            uintptr_t sceneNode = Utils::SafeRead<uintptr_t>(weapon + Offsets::m_pGameSceneNode);
                            if (sceneNode && Utils::IsValidPtr(sceneNode))
                            {
                                void** sceneVtable = *(void***)sceneNode;
                                if (sceneVtable && Utils::IsValidPtr((uintptr_t)sceneVtable))
                                {
                                    static int delayFrames = 0;
                                    static uintptr_t lastAppliedWeapon = 0;
                                    static int lastAppliedKnife = 0;

                                    bool needsApply = (lastAppliedWeapon != weapon) || (lastAppliedKnife != knifeIdx);
                                    if (needsApply) {
                                        delayFrames++;
                                        if (delayFrames >= 8) {
                                            static int visLogThrottle = 0;
                                            if (visLogThrottle++ % 600 == 0 || needsApply) {
                                                g_Logger.Log("RunVisuals KNIFE (Stage 7): weapon=0x%llX model=%s",
                                                    weapon, model);
                                            }

                                            SafeCallSetModel((void*)weapon, model);

                                            if (sceneNode && Utils::IsValidPtr(sceneNode)) {
                                                
                                                int kPaintKit = SC_GetKnifePaintKit();
                                                uint64_t knifeMask = SC_GetMeshGroupMask(kPaintKit);
                                                SafeCallSetMeshGroupMask((void*)sceneNode, knifeMask);

                                                
                                                uintptr_t hudKnifeNode = SC_GetHudModelWeaponSceneNode(localPawn, weapon);
                                                if (hudKnifeNode && Utils::IsValidPtr(hudKnifeNode)) {
                                                    SafeCallSetMeshGroupMask((void*)hudKnifeNode, knifeMask);
                                                }

                                                g_Logger.Log("  [KNIFE] SetMeshGroupMask(%llu) paintKit=%d", knifeMask, kPaintKit);
                                            }

                                            lastAppliedWeapon = weapon;
                                            lastAppliedKnife = knifeIdx;
                                            delayFrames = 0;
                                        }
                                    } else {
                                        delayFrames = 0;
                                    }
                                }
                            }
                        }
                    }
                }
                }

                
                
                if (g_PendingGloveApply && g_PendingGlovePawn)
                {
                    
                    
                    if (g_PendingGlovePawn != localPawn) {
                        g_Logger.Log("RunVisuals (Stage 7): Stale glove pawn detected (pending=0x%llX, current=0x%llX) - resetting",
                            g_PendingGlovePawn, localPawn);
                        SC_ResetGloveState();
                    }
                    else if (Utils::IsValidPtr(g_PendingGlovePawn)) {
                        SC_ApplyGloves(g_PendingGlovePawn, g_PendingGloveWeapons, g_PendingGloveWeaponCount);
                        
                        
                    }
                }
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            g_Logger.Log("EXCEPTION in RunVisuals! code=0x%08X", GetExceptionCode());
            
            SC_CleanupAllMemory();
            SC_ResetGloveState();
            g_PendingKnifeWeapon = 0;
            g_PendingAgentPawn = 0;
            g_PendingAgentApply = false;
            g_PendingAgentModel[0] = '\0';
            g_LastValidPawn = 0;
        }
    }
}
