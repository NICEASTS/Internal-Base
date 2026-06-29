

import json
import re
import os
import sys
from datetime import datetime





SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DUMP_DIR = os.path.join(SCRIPT_DIR, "dump")
OFFSETS_H = os.path.join(SCRIPT_DIR, "src", "sdk", "memory", "Offsets.h")







GLOBAL_OFFSET_MAP = {
    
    "dwEntityList":           ("client.dll", "dwEntityList"),
    "dwLocalPlayerPawn":      ("client.dll", "dwLocalPlayerPawn"),
    "dwLocalPlayerController":("client.dll", "dwLocalPlayerController"),
    "dwViewMatrix":           ("client.dll", "dwViewMatrix"),
    "dwViewRender":           ("client.dll", "dwViewRender"),
    "dwViewAngles":           ("client.dll", "dwViewAngles"),
    "dwGlobalVars":           ("client.dll", "dwGlobalVars"),
    "dwCSGOInput":            ("client.dll", "dwCSGOInput"),
    "dwPlantedC4":            ("client.dll", "dwPlantedC4"),
    "dwGlowManager":          ("client.dll", "dwGlowManager"),
}



SCHEMA_OFFSET_MAP = {
    
    "m_iHealth":            ("C_BaseEntity", "m_iHealth"),
    "m_iTeamNum":           ("C_BaseEntity", "m_iTeamNum"),
    "m_lifeState":          ("C_BaseEntity", "m_lifeState"),
    "m_vOldOrigin":         ("C_BasePlayerPawn", "m_vOldOrigin"),
    "m_pGameSceneNode":     ("C_BaseEntity", "m_pGameSceneNode"),
    "m_pCollision":         ("C_BaseEntity", "m_pCollision"),
    "m_fFlags":             ("C_BaseEntity", "m_fFlags"),
    "m_flSimulationTime":   ("C_BaseEntity", "m_flSimulationTime"),

    
    "m_entitySpottedState": ("C_CSPlayerPawn", "m_entitySpottedState"),

    
    "m_bSpotted":           ("EntitySpottedState_t", "m_bSpotted"),
    "m_bSpottedByMask":     ("EntitySpottedState_t", "m_bSpottedByMask"),

    
    "m_vecAbsOrigin":       ("CGameSceneNode", "m_vecAbsOrigin"),
    "m_modelState":         ("CSkeletonInstance", "m_modelState"),

    
    "m_hModel":             ("CModelState", "m_hModel"),
    "m_ModelName":          ("CModelState", "m_ModelName"),

    
    "m_vecMins":            ("CCollisionProperty", "m_vecMins"),
    "m_vecMaxs":            ("CCollisionProperty", "m_vecMaxs"),

    
    "m_pCameraServices":    ("C_BasePlayerPawn", "m_pCameraServices"),
    "m_pWeaponServices":    ("C_BasePlayerPawn", "m_pWeaponServices"),
    "m_vecViewOffset":      ("C_BaseModelEntity", "m_vecViewOffset"),

    
    "m_pAimPunchServices":  ("C_CSPlayerPawn", "m_pAimPunchServices"),
    "m_predictableBaseAngle": ("CCSPlayer_AimPunchServices", "m_predictableBaseAngle"),
    "m_predictableBaseAngleVel": ("CCSPlayer_AimPunchServices", "m_predictableBaseAngleVel"),
    "m_unpredictableBaseAngle": ("CCSPlayer_AimPunchServices", "m_unpredictableBaseAngle"),
    "m_aimPunchAngle":      ("CCSPlayer_AimPunchServices", "m_unpredictableBaseAngle"),
    "m_aimPunchAngleVel":   ("CCSPlayer_AimPunchServices", "m_predictableBaseAngleVel"),
    "m_iShotsFired":        ("C_CSPlayerPawn", "m_iShotsFired"),
    "m_flFOVSensitivityAdjust": ("C_BasePlayerPawn", "m_flFOVSensitivityAdjust"),
    "m_bIsScoped":          ("C_CSPlayerPawn", "m_bIsScoped"),
    "m_bResumeZoom":        ("C_CSPlayerPawn", "m_bResumeZoom"),
    "m_bIsBuyMenuOpen":     ("C_CSPlayerPawn", "m_bIsBuyMenuOpen"),
    "m_flEmitSoundTime":    ("C_CSPlayerPawn", "m_flEmitSoundTime"),
    "m_unWeaponHash":       ("C_CSPlayerPawn", "m_unWeaponHash"),

    
    "m_flFlashDuration":        ("C_CSPlayerPawnBase", "m_flFlashDuration"),
    "m_flFlashMaxAlpha":        ("C_CSPlayerPawnBase", "m_flFlashMaxAlpha"),
    "m_flFlashScreenshotAlpha": ("C_CSPlayerPawnBase", "m_flFlashScreenshotAlpha"),
    "m_flFlashOverlayAlpha":    ("C_CSPlayerPawnBase", "m_flFlashOverlayAlpha"),
    "m_bFlashBuildUp":          ("C_CSPlayerPawnBase", "m_bFlashBuildUp"),

    
    "m_iIDEntIndex":        ("C_CSPlayerPawn", "m_iIDEntIndex"),
    "m_pBulletServices":    ("C_CSPlayerPawn", "m_pBulletServices"),

    
    "m_totalHitsOnServer":  ("CCSPlayer_BulletServices", "m_totalHitsOnServer"),

    
    "m_pObserverServices":  ("C_BasePlayerPawn", "m_pObserverServices"),

    
    "m_hObserverTarget":    ("CPlayer_ObserverServices", "m_hObserverTarget"),

    
    "m_hPawn":              ("CBasePlayerController", "m_hPawn"),

    
    "m_iFOV":               ("CCSPlayerBase_CameraServices", "m_iFOV"),
    "m_iFOVStart":          ("CCSPlayerBase_CameraServices", "m_iFOVStart"),
    "m_flFOVTime":          ("CCSPlayerBase_CameraServices", "m_flFOVTime"),
    "m_flFOVRate":          ("CCSPlayerBase_CameraServices", "m_flFOVRate"),
    "m_hZoomOwner":         ("CCSPlayerBase_CameraServices", "m_hZoomOwner"),
    "m_flLastShotFOV":      ("CCSPlayerBase_CameraServices", "m_flLastShotFOV"),

    
    "m_iDesiredFOV":        ("CBasePlayerController", "m_iDesiredFOV"),
    "m_iszPlayerName":      ("CBasePlayerController", "m_iszPlayerName"),
    "m_hPlayerPawn":        ("CCSPlayerController", "m_hPlayerPawn"),
    "m_bPawnIsAlive":       ("CCSPlayerController", "m_bPawnIsAlive"),

    
    "m_VData":              ("C_BaseEntity", "m_VData"),

    
    "m_bBombTicking":       ("C_PlantedC4", "m_bBombTicking"),
    "m_nBombSite":          ("C_PlantedC4", "m_nBombSite"),
    "m_flC4Blow":           ("C_PlantedC4", "m_flC4Blow"),
    "m_bBeingDefused":      ("C_PlantedC4", "m_bBeingDefused"),
    "m_flDefuseLength":     ("C_PlantedC4", "m_flDefuseLength"),
    "m_flDefuseCountDown":  ("C_PlantedC4", "m_flDefuseCountDown"),
    "m_bBombDefused":       ("C_PlantedC4", "m_bBombDefused"),
    "m_hBombDefuser":       ("C_PlantedC4", "m_hBombDefuser"),

    
    "m_bHasDefuser":        ("CCSPlayer_ItemServices", "m_bHasDefuser"),

    
    "m_Glow":               ("C_BaseModelEntity", "m_Glow"),
    "m_glowColorOverride":  ("CGlowProperty", "m_glowColorOverride"),
    "m_iGlowType":          ("CGlowProperty", "m_iGlowType"),
    "m_bGlowing":           ("CGlowProperty", "m_bGlowing"),

    
    "m_hActiveWeapon":      ("CPlayer_WeaponServices", "m_hActiveWeapon"),
    "m_pItemServices":      ("C_BasePlayerPawn", "m_pItemServices"),

    
    "m_zoomLevel":          ("C_CSWeaponBaseGun", "m_zoomLevel"),

    
    "m_flFOV":              ("CCSPlayerBase_CameraServices", "m_flFOV"),
    "m_bVerticalFOV":       ("C_CSGO_MapPreviewCameraPath", "m_bVerticalFOV"),

    
    "m_flViewmodelFOV":     ("C_CSPlayerPawn", "m_flViewmodelFOV"),

    
    "m_flZoomTime0":        ("CCSWeaponBaseVData", "m_flZoomTime0"),
    "m_flZoomTime1":        ("CCSWeaponBaseVData", "m_flZoomTime1"),
    "m_flZoomTime2":        ("CCSWeaponBaseVData", "m_flZoomTime2"),

    
    "sc_m_nFallbackPaintKit":    ("C_EconEntity", "m_nFallbackPaintKit"),
    "sc_m_nFallbackStatTrak":    ("C_EconEntity", "m_nFallbackStatTrak"),
    "sc_m_flFallbackWear":       ("C_EconEntity", "m_flFallbackWear"),
    "sc_m_nFallbackSeed":        ("C_EconEntity", "m_nFallbackSeed"),
    "sc_m_OriginalOwnerXuidLow": ("C_EconEntity", "m_OriginalOwnerXuidLow"),
    "sc_m_AttributeManager":     ("C_EconEntity", "m_AttributeManager"),

    
    "sc_m_Item":                 ("C_AttributeContainer", "m_Item"),

    
    "sc_m_iItemDefinitionIndex": ("C_EconItemView", "m_iItemDefinitionIndex"),
    "sc_m_iItemID":              ("C_EconItemView", "m_iItemID"),
    "sc_m_iItemIDHigh":          ("C_EconItemView", "m_iItemIDHigh"),
    "sc_m_iItemIDLow":           ("C_EconItemView", "m_iItemIDLow"),
    "sc_m_iAccountID":           ("C_EconItemView", "m_iAccountID"),
    "sc_m_iEntityQuality":       ("C_EconItemView", "m_iEntityQuality"),
    "sc_m_bInitialized":         ("C_EconItemView", "m_bInitialized"),
    "sc_m_szCustomNameOverride":   ("C_EconItemView", "m_szCustomNameOverride"),
    "sc_m_AttributeList":        ("C_EconItemView", "m_AttributeList"),
    "sc_m_NetworkedDynamicAttributes": ("C_EconItemView", "m_NetworkedDynamicAttributes"),
    "sc_m_bRestoreCustomMaterialAfterPrecache": ("C_EconItemView", "m_bRestoreCustomMaterialAfterPrecache"),

    
    "sc_m_Attributes":           ("CAttributeList", "m_Attributes"),

    
    "sc_m_hMyWeapons":           ("CPlayer_WeaponServices", "m_hMyWeapons"),
    "sc_m_hActiveWeapon_ws":     ("CPlayer_WeaponServices", "m_hActiveWeapon"),

    
    "sc_m_pChild":               ("CGameSceneNode", "m_pChild"),
    "sc_m_pNextSibling":         ("CGameSceneNode", "m_pNextSibling"),
    "sc_m_pOwner":               ("CGameSceneNode", "m_pOwner"),

    
    "sc_m_MeshGroupMask":        ("CModelState", "m_MeshGroupMask"),

    
    "sc_m_flLastSpawnTimeIndex": ("C_CSPlayerPawnBase", "m_flLastSpawnTimeIndex"),

    
    "sc_m_bNeedToReApplyGloves": ("C_CSPlayerPawn", "m_bNeedToReApplyGloves"),
    "sc_m_EconGloves":           ("C_CSPlayerPawn", "m_EconGloves"),
    "sc_m_pClippingWeapon":      ("C_CSPlayerPawn", "m_pClippingWeapon"),
    "sc_m_hHudModelArms":        ("C_CSPlayerPawn", "m_hHudModelArms"),

    
    "sc_m_pInventoryServices":   ("CCSPlayerController", "m_pInventoryServices"),

    
    "sc_m_unMusicID":            ("CCSPlayerController_InventoryServices", "m_unMusicID"),

    
    "sc_m_pEntity":              ("CEntityInstance", "m_pEntity"),

    
    "sc_m_entityFlags":          ("CEntityIdentity", "m_flags"),
}


SKIP_OFFSETS = {"m_flCurTime", "m_iFrameCount", "sc_m_pDirtyModelData", "sc_m_DirtyMeshGroupMask"}


def load_json(filename):
    
    filepath = os.path.join(DUMP_DIR, filename)
    if not os.path.exists(filepath):
        print(f"  [!] DOSYA BULUNAMADI: {filepath}")
        return None
    with open(filepath, 'r', encoding='utf-8') as f:
        return json.load(f)


def build_schema_db():
    
    db = {}
    json_files = [
        "client_dll.json",
        "engine2_dll.json",
        "schemasystem_dll.json",
        "materialsystem2_dll.json",
        "scenesystem_dll.json",
        "rendersystemdx11_dll.json",
        "resourcesystem_dll.json",
        "animationsystem_dll.json",
        "soundsystem_dll.json",
        "vphysics2_dll.json",
        "networksystem_dll.json",
        "panorama_dll.json",
        "particles_dll.json",
        "pulse_system_dll.json",
        "worldrenderer_dll.json",
        "host_dll.json",
        "steamaudio_dll.json",
    ]

    for jf in json_files:
        data = load_json(jf)
        if not data:
            continue
        for module_name, module_data in data.items():
            classes = module_data.get("classes", {})
            for class_name, class_info in classes.items():
                fields = class_info.get("fields", {})
                if class_name not in db:
                    db[class_name] = {}
                db[class_name].update(fields)

    return db


def update_offsets():
    
    print("=" * 60)
    print("  CS2 Offset Updater")
    print(f"  {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 60)

    
    if not os.path.exists(OFFSETS_H):
        print(f"\n[HATA] Offsets.h bulunamadi: {OFFSETS_H}")
        sys.exit(1)

    if not os.path.exists(DUMP_DIR):
        print(f"\n[HATA] Dump klasoru bulunamadi: {DUMP_DIR}")
        sys.exit(1)

    
    print("\n[*] offsets.json yukleniyor...")
    global_offsets = load_json("offsets.json")
    if not global_offsets:
        print("[HATA] offsets.json yuklenemedi!")
        sys.exit(1)

    
    print("[*] Schema dump dosyalari yukleniyor...")
    schema_db = build_schema_db()
    print(f"    -> {len(schema_db)} class yuklendi")

    
    print(f"\n[*] Offsets.h okunuyor: {OFFSETS_H}")
    with open(OFFSETS_H, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original_content = content

    
    pattern = re.compile(
        r'(constexpr\s+(?:uintptr_t|std::ptrdiff_t)\s+)(\w+)(\s*=\s*)(0x[0-9A-Fa-f]+)(;)'
    )

    updated_count = 0
    skipped_count = 0
    not_found_count = 0
    unchanged_count = 0

    changes = []

    def replace_offset(match):
        nonlocal updated_count, skipped_count, not_found_count, unchanged_count

        prefix = match.group(1)
        var_name = match.group(2)
        equals = match.group(3)
        old_hex = match.group(4)
        suffix = match.group(5)

        old_value = int(old_hex, 16)

        
        if var_name in SKIP_OFFSETS:
            skipped_count += 1
            return match.group(0)

        new_value = None

        
        if var_name in GLOBAL_OFFSET_MAP:
            module, key = GLOBAL_OFFSET_MAP[var_name]
            if module in global_offsets and key in global_offsets[module]:
                new_value = global_offsets[module][key]

        
        if new_value is None and var_name in SCHEMA_OFFSET_MAP:
            class_name, field_name = SCHEMA_OFFSET_MAP[var_name]
            if class_name in schema_db and field_name in schema_db[class_name]:
                new_value = schema_db[class_name][field_name]

        
        if new_value is None:
            not_found_count += 1
            return match.group(0)

        
        new_hex = f"0x{new_value:X}"
        if new_value == old_value:
            unchanged_count += 1
            return match.group(0)

        updated_count += 1
        changes.append((var_name, old_hex, new_hex))
        return f"{prefix}{var_name}{equals}{new_hex}{suffix}"

    content = pattern.sub(replace_offset, content)

    
    print("\n" + "=" * 60)
    print("  SONUCLAR")
    print("=" * 60)

    if changes:
        print(f"\n  [+] {updated_count} offset guncellendi:\n")
        
        print(f"  {'Offset':<40} {'Eski':<14} {'Yeni':<14}")
        print(f"  {'-'*40} {'-'*14} {'-'*14}")
        for name, old, new in changes:
            print(f"  {name:<40} {old:<14} {new:<14}")
    else:
        print("\n  Tum offsetler zaten guncel!")

    print(f"\n  Guncellenen:  {updated_count}")
    print(f"  Degismeyen:   {unchanged_count}")
    print(f"  Atlanan:      {skipped_count}")
    print(f"  Bulunamayan:  {not_found_count}")

    
    if content != original_content:
        
        backup_path = OFFSETS_H + ".bak"
        with open(backup_path, 'w', encoding='utf-8') as f:
            f.write(original_content)
        print(f"\n  [*] Yedek alindi: {backup_path}")

        with open(OFFSETS_H, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"  [*] Offsets.h guncellendi!")
    else:
        print(f"\n  [*] Degisiklik yok, dosya yazilmadi.")

    print("\n" + "=" * 60)


if __name__ == "__main__":
    try:
        update_offsets()
    except Exception as e:
        print(f"\n[HATA] Beklenmeyen hata: {e}")
        import traceback
        traceback.print_exc()
    
    input("\nDevam etmek icin Enter'a basin...")
