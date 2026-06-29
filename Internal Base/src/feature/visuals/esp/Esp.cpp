#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Esp.h"
#include "../../../sdk/entity/EntityManager.h"
#include "../../../sdk/utils/Utils.h"
#include "../../../sdk/utils/Globals.h"
#include "../../../sdk/utils/Raycasting.h"
#include "../../../sdk/memory/PatternScan.h"
#include "../../../../ext/imgui/imgui.h"
#include <algorithm>
#include <limits>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <cmath>
#include <cstdio>
#include "../../../sdk/memory/Offsets.h"




static ImVec2 CatmullRom(const ImVec2& p0, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, float t)
{
    float t2 = t * t;
    float t3 = t2 * t;
    return ImVec2(
        0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3),
        0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3)
    );
}


static const std::vector<std::vector<BoneID>> g_BoneChains = {
    
    { BoneID::Head, BoneID::Neck, BoneID::Spine, BoneID::Pelvis },
    
    { BoneID::Spine, BoneID::LeftShoulder, BoneID::LeftArm, BoneID::LeftHand },
    
    { BoneID::Spine, BoneID::RightShoulder, BoneID::RightArm, BoneID::RightHand },
    
    { BoneID::Pelvis, BoneID::LeftHip, BoneID::LeftKnee, BoneID::LeftFoot },
    
    { BoneID::Pelvis, BoneID::RightHip, BoneID::RightKnee, BoneID::RightFoot },
};

extern ImFont* esp_font;
extern ImFont* weapon_icon_font;


static Vector GetLocalEyePosition()
{
    auto localPawn = EntityManager::Get().GetLocalPawn();
    if (!localPawn) return {};

    uintptr_t pawnAddr = reinterpret_cast<uintptr_t>(localPawn);
    uintptr_t scene = Utils::SafeRead<uintptr_t>(pawnAddr + Offsets::m_pGameSceneNode);
    if (!Utils::IsValidPtr(scene)) return {};

    Vector origin = Utils::SafeRead<Vector>(scene + Offsets::m_vecAbsOrigin);
    Vector viewOffset = Utils::SafeRead<Vector>(pawnAddr + Offsets::m_vecViewOffset);
    return origin + viewOffset;
}


static bool IsBoneVisibleRaycast(const Vector& bonePos)
{
    if (!Raycasting::Get().IsLoaded())
        return true; 

    Vector eyePos = GetLocalEyePosition();
    if (eyePos.IsZero())
        return true;

    return Raycasting::Get().IsVisible(eyePos, bonePos);
}


static bool IsAnyBoneVisible(C_CSPlayerPawn* pawn)
{
    if (!Raycasting::Get().IsLoaded())
        return true; 

    Vector eyePos = GetLocalEyePosition();
    if (eyePos.IsZero())
        return true;

    static const BoneID majorBones[] = {
        BoneID::Head, BoneID::Neck, BoneID::Spine, BoneID::Pelvis,
        BoneID::LeftShoulder, BoneID::LeftArm, BoneID::LeftHand,
        BoneID::RightShoulder, BoneID::RightArm, BoneID::RightHand,
        BoneID::LeftHip, BoneID::LeftKnee, BoneID::LeftFoot,
        BoneID::RightHip, BoneID::RightKnee, BoneID::RightFoot
    };

    for (auto bone : majorBones)
    {
        Vector bonePos = Utils::GetBonePos(pawn, bone);
        if (!bonePos.IsZero() && Raycasting::Get().IsVisible(eyePos, bonePos))
            return true;
    }

    return false;
}

static bool ReadAsciiAt(uintptr_t addr, char* out, size_t maxLen)
{
    if (!addr || !out || maxLen < 2) return false;
    memset(out, 0, maxLen);
    __try {
        memcpy(out, reinterpret_cast<void*>(addr), maxLen - 1);
        out[maxLen - 1] = '\0';
        size_t n = 0;
        for (; n < maxLen - 1; n++) {
            if (out[n] == '\0') break;
        }
        if (n == 0) return false;

        for (size_t i = 0; i < n; i++) {
            unsigned char c = (unsigned char)out[i];
            if (c < 32) out[i] = ' ';
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static bool ReadWideAt(uintptr_t addr, char* out, size_t maxLen)
{
    if (!addr || !out || maxLen < 2) return false;
    wchar_t wbuf[128]{};
    __try {
        memcpy(wbuf, reinterpret_cast<void*>(addr), sizeof(wbuf) - sizeof(wchar_t));
        wbuf[(sizeof(wbuf) / sizeof(wchar_t)) - 1] = L'\0';
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    int wlen = 0;
    while (wlen < (int)(sizeof(wbuf) / sizeof(wchar_t)) - 1 && wbuf[wlen] != L'\0') wlen++;
    if (wlen <= 0) return false;

    int n = WideCharToMultiByte(CP_UTF8, 0, wbuf, wlen, out, (int)maxLen - 1, NULL, NULL);
    if (n <= 0) return false;
    out[n] = '\0';
    return true;
}

static bool LooksLikePlayerName(const char* s)
{
    if (!s || !s[0]) return false;
    int len = 0, letters = 0;
    for (; s[len] && len < 63; len++) {
        unsigned char c = (unsigned char)s[len];
        bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '_' || c == '-' || c == ' ';
        if (!ok) return false;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) letters++;
    }
    return len >= 3 && letters >= 2;
}

static bool ReadControllerName(uintptr_t controllerBase, char* out, size_t maxLen, const char** outSource = nullptr)
{
    if (!controllerBase || !out || maxLen < 2) return false;

    if (ReadAsciiAt(controllerBase + 0x6F4, out, maxLen)) {
        if (outSource) *outSource = "src:6F4-inline";
        return true;
    }
    if (ReadWideAt(controllerBase + 0x6F4, out, maxLen)) {
        if (outSource) *outSource = "src:6F4-wide";
        return true;
    }

    uintptr_t sanitizedPtr = 0;
    __try { sanitizedPtr = *(uintptr_t*)(controllerBase + Offsets::m_iszPlayerName); }
    __except (EXCEPTION_EXECUTE_HANDLER) { sanitizedPtr = 0; }
    if (sanitizedPtr && ReadAsciiAt(sanitizedPtr, out, maxLen)) {
        if (outSource) *outSource = "src:858-ptr";
        return true;
    }
    if (sanitizedPtr && ReadWideAt(sanitizedPtr, out, maxLen)) {
        if (outSource) *outSource = "src:858-wide";
        return true;
    }

    if (outSource) *outSource = "src:none";
    return false;
}

const char* GetIconFromWeaponId(uint16_t id)
{
    switch (id)
    {
    case 1: return "A"; case 2: return "B"; case 3: return "C"; case 4: return "D";
    case 7: return "W"; case 8: return "U"; case 9: return "Z"; case 10: return "R";
    case 11: return "X"; case 13: return "Q"; case 14: return "g"; case 16: return "S";
    case 17: return "K"; case 19: return "O"; case 23: return "M"; case 24: return "L";
    case 25: return "b"; case 26: return "M"; case 27: return "d"; case 28: return "f";
    case 29: return "c"; case 30: return "H"; case 31: return "h"; case 32: return "E";
    case 33: return "N"; case 34: return "P"; case 35: return "e"; case 36: return "F";
    case 38: return "Y"; case 39: return "V"; case 40: return "a"; case 42: return "]";
    case 43: return "i"; case 44: return "j"; case 45: return "k"; case 46: return "l";
    case 47: return "m"; case 48: return "n"; case 49: return "o"; case 59: return "[";
    case 60: return "T"; case 61: return "G"; case 63: return "I"; case 64: return "J";
    default:
        if (id >= 500 && id <= 526) return "]";
        return "";
    }
}




struct SoundRingInfo
{
    Vector origin;      
    double startTime;   
};

static std::unordered_map<uintptr_t, std::vector<SoundRingInfo>> g_SoundRingCache;
static std::unordered_map<uintptr_t, float> g_LastSoundVal;

static double GetTimeSeconds()
{
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

static void DrawSoundESP(ImDrawList* dl, const Entity_t& ent, C_CSPlayerPawn* localPawn, float sw, float sh, bool isEnemy)
{
    const bool soundEnabled = isEnemy ? Globals::esp_sound_enabled : Globals::esp_sound_enabled_team;
    if (!soundEnabled)
        return;

    C_CSPlayerPawn* pawn = ent.pawn;
    uintptr_t pawnAddr = reinterpret_cast<uintptr_t>(pawn);

    
    if (g_SoundRingCache.find(pawnAddr) == g_SoundRingCache.end())
    {
        g_SoundRingCache[pawnAddr] = std::vector<SoundRingInfo>();
        g_LastSoundVal[pawnAddr] = 0.0f;
    }

    
    float currentSound = Utils::SafeRead<float>(pawnAddr + Offsets::m_flEmitSoundTime, 0.0f);
    float lastSound = g_LastSoundVal[pawnAddr];

    
    if (currentSound != lastSound && currentSound != 0.0f)
    {
        g_LastSoundVal[pawnAddr] = currentSound;

        Vector origin = Utils::SafeRead<Vector>(pawnAddr + Offsets::m_vOldOrigin);

        SoundRingInfo ring;
        ring.origin = origin;
        ring.startTime = GetTimeSeconds();
        g_SoundRingCache[pawnAddr].push_back(ring);
    }

    
    auto& rings = g_SoundRingCache[pawnAddr];
    double now = GetTimeSeconds();

    for (int r = (int)rings.size() - 1; r >= 0; r--)
    {
        SoundRingInfo& ring = rings[r];
        double elapsed = now - ring.startTime;

        
        if (elapsed >= 1.0)
        {
            rings.erase(rings.begin() + r);
            continue;
        }

        
        float radius = 30.0f * (1.0f - (float)elapsed);

        
        float alpha = 1.0f - (float)elapsed;

        ImU32 ringColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
            isEnemy ? Globals::esp_sound_color[0] : Globals::esp_sound_color_team[0],
            isEnemy ? Globals::esp_sound_color[1] : Globals::esp_sound_color_team[1],
            isEnemy ? Globals::esp_sound_color[2] : Globals::esp_sound_color_team[2],
            (isEnemy ? Globals::esp_sound_color[3] : Globals::esp_sound_color_team[3]) * alpha
        ));

        
        const int segments = 72;
        std::vector<ImVec2> points;
        points.reserve(segments + 1);
        float step = (float)(M_PI * 2.0) / segments;

        for (int i = 0; i <= segments; i++)
        {
            float angle = step * i;
            float xOffset = radius * cosf(angle);
            float yOffset = radius * sinf(angle);

            Vector point3D(
                ring.origin.x + xOffset,
                ring.origin.y + yOffset,
                ring.origin.z
            );

            Vector screenPos;
            if (Utils::WorldToScreen(point3D, screenPos, (float*)Globals::ViewMatrix, sw, sh))
            {
                points.push_back(ImVec2(screenPos.x, screenPos.y));
            }
            else
            {
                
                if (points.size() > 1)
                {
                    dl->AddPolyline(points.data(), (int)points.size(), ringColor, ImDrawFlags_None, 2.0f);
                }
                points.clear();
            }
        }

        
        if (points.size() > 1)
        {
            dl->AddPolyline(points.data(), (int)points.size(), ringColor, ImDrawFlags_None, 2.0f);
        }
    }
}




static void RenderInternal()
{
    if (!Globals::esp_enabled && !Globals::esp_enabled_team)
        return;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->Flags |= ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedFill;
    const float sw = Globals::GameViewportWidth;
    const float sh = Globals::GameViewportHeight;

    if (sw <= 0 || sh <= 0)
        return;

    auto entities = EntityManager::Get().GetEntities();
    C_CSPlayerPawn* localPawn = EntityManager::Get().GetLocalPawn();
    if (!localPawn)
        return;
    C_CSPlayerPawn* observedPawn = EntityManager::Get().GetLocalObservedPawn();
    const int localTeam = Utils::SafeTeam(localPawn);
    const Vector localOrigin = Utils::SafeRead<Vector>(reinterpret_cast<uintptr_t>(localPawn) + Offsets::m_vOldOrigin);

    for (const auto& ent : entities)
    {
        C_CSPlayerPawn* pawn = ent.pawn;
        if (!Utils::IsValidPtr(reinterpret_cast<uintptr_t>(pawn))) continue;
        if (!Utils::SafeAlive(pawn)) continue;
        if (pawn == localPawn) continue;
        if (observedPawn && pawn == observedPawn) continue;

        int team = Utils::SafeTeam(pawn);
        bool isEnemy = (team != localTeam);
        const bool espEnabled = isEnemy ? Globals::esp_enabled : Globals::esp_enabled_team;
        if (!espEnabled)
            continue;
        const bool espBoxEnabled = isEnemy ? Globals::esp_box : Globals::esp_box_team;
        const bool espSkeletonEnabled = isEnemy ? Globals::esp_skeleton : Globals::esp_skeleton_team;
        const bool espHeadEnabled = isEnemy ? Globals::esp_head : Globals::esp_head_team;
        const bool espNameEnabled = isEnemy ? Globals::esp_name : Globals::esp_name_team;
        const bool espDistanceEnabled = isEnemy ? Globals::esp_distance : Globals::esp_distance_team;
        const bool espWeaponEnabled = isEnemy ? Globals::esp_weapon : Globals::esp_weapon_team;
        const bool espHealthEnabled = isEnemy ? Globals::esp_health : Globals::esp_health_team;
        const bool espSoundEnabled = isEnemy ? Globals::esp_sound_enabled : Globals::esp_sound_enabled_team;
        if (!espBoxEnabled && !espSkeletonEnabled && !espHeadEnabled && !espNameEnabled &&
            !espDistanceEnabled && !espWeaponEnabled && !espHealthEnabled && !espSoundEnabled)
            continue;
        const int espBoxType = isEnemy ? Globals::esp_box_type : Globals::esp_box_type_team;
        const int espSkeletonType = isEnemy ? Globals::esp_skeleton_type : Globals::esp_skeleton_type_team;

        const float* boxColorHidden = isEnemy ? Globals::esp_box_color : Globals::esp_box_color_team;
        const float* boxColorVisible = isEnemy ? Globals::esp_box_color_vis : Globals::esp_box_color_vis_team;
        const float* skeletonColorHidden = isEnemy ? Globals::esp_skeleton_color : Globals::esp_skeleton_color_team;
        const float* skeletonColorVisible = isEnemy ? Globals::esp_skeleton_color_vis : Globals::esp_skeleton_color_vis_team;
        const float* nameColorHidden = isEnemy ? Globals::esp_name_color : Globals::esp_name_color_team;
        const float* nameColorVisible = isEnemy ? Globals::esp_name_color_vis : Globals::esp_name_color_vis_team;
        const float* healthColorHidden = isEnemy ? Globals::esp_health_color : Globals::esp_health_color_team;
        const float* healthColorVisible = isEnemy ? Globals::esp_health_color_vis : Globals::esp_health_color_vis_team;
        const float* distanceColorHidden = isEnemy ? Globals::esp_distance_color : Globals::esp_distance_color_team;
        const float* distanceColorVisible = isEnemy ? Globals::esp_distance_color_vis : Globals::esp_distance_color_vis_team;
        const float* weaponColorHidden = isEnemy ? Globals::esp_weapon_color : Globals::esp_weapon_color_team;
        const float* weaponColorVisible = isEnemy ? Globals::esp_weapon_color_vis : Globals::esp_weapon_color_vis_team;

        Vector origin = Utils::SafeRead<Vector>(reinterpret_cast<uintptr_t>(pawn) + Offsets::m_vOldOrigin);
        if (origin.IsZero())
            continue;
        Vector headPos = Utils::GetBonePos(pawn, BoneID::Head);
        
        if (headPos.IsZero())
        {
            headPos = origin;
            headPos.z += 72.0f; 
        }
        else
        {
            headPos.z += 7.0f; 
        }

        Vector sHead, sFoot;
        const bool onScreen = Utils::WorldToScreen(headPos, sHead, (float*)Globals::ViewMatrix, sw, sh) &&
            Utils::WorldToScreen(origin, sFoot, (float*)Globals::ViewMatrix, sw, sh);
        if (!onScreen)
            continue;

        DrawSoundESP(dl, ent, localPawn, sw, sh, isEnemy);

        const bool needAnyVisibleColor =
            espBoxEnabled || espNameEnabled || espDistanceEnabled || espWeaponEnabled || espHealthEnabled;
        const bool isVis = needAnyVisibleColor ? IsAnyBoneVisible(pawn) : false;
        const ImU32 boxCol = ImGui::ColorConvertFloat4ToU32(ImVec4(
            isVis ? boxColorVisible[0] : boxColorHidden[0],
            isVis ? boxColorVisible[1] : boxColorHidden[1],
            isVis ? boxColorVisible[2] : boxColorHidden[2],
            isVis ? boxColorVisible[3] : boxColorHidden[3]
        ));
        const ImU32 nameCol = ImGui::ColorConvertFloat4ToU32(ImVec4(
            isVis ? nameColorVisible[0] : nameColorHidden[0],
            isVis ? nameColorVisible[1] : nameColorHidden[1],
            isVis ? nameColorVisible[2] : nameColorHidden[2],
            isVis ? nameColorVisible[3] : nameColorHidden[3]
        ));

        {
            float height = sFoot.y - sHead.y;
            float width = height / 1.8f; 
            float x = sHead.x - width / 2.0f;
            float y = sHead.y;

            if (espBoxEnabled)
            {
                if (espBoxType == 0) 
                {
                    dl->AddRect({ x - 1, y - 1 }, { x + width + 1, y + height + 1 }, IM_COL32(0, 0, 0, 150));
                    dl->AddRect({ x, y }, { x + width, y + height }, boxCol, 0, 0, Globals::esp_box_thickness);
                    dl->AddRect({ x + 1, y + 1 }, { x + width - 1, y + height - 1 }, IM_COL32(0, 0, 0, 150));
                }
                else 
                {
                    float lineW = width / 4.0f;
                    float lineH = height / 4.0f;

                    auto DrawLineBase = [&](float px1, float py1, float px2, float py2) {
                        dl->AddLine({px1, py1}, {px2, py2}, IM_COL32(0, 0, 0, 255), Globals::esp_box_thickness + 2.0f);
                        dl->AddLine({px1, py1}, {px2, py2}, boxCol, Globals::esp_box_thickness);
                    };

                    
                    DrawLineBase(x, y, x + lineW, y);
                    DrawLineBase(x, y, x, y + lineH);

                    
                    DrawLineBase(x + width - lineW, y, x + width, y);
                    DrawLineBase(x + width, y, x + width, y + lineH);

                    
                    DrawLineBase(x, y + height - lineH, x, y + height);
                    DrawLineBase(x, y + height, x + lineW, y + height);

                    
                    DrawLineBase(x + width - lineW, y + height, x + width, y + height);
                    DrawLineBase(x + width, y + height - lineH, x + width, y + height);
                }
            }

            if (espHealthEnabled)
            {
                int health = Utils::SafeRead<int>(reinterpret_cast<uintptr_t>(pawn) + Offsets::m_iHealth, 100);
                float hpFrac = std::clamp(health / 100.f, 0.f, 1.f);
                const ImU32 hpCol = ImGui::ColorConvertFloat4ToU32(ImVec4(
                    isVis ? healthColorVisible[0] : healthColorHidden[0],
                    isVis ? healthColorVisible[1] : healthColorHidden[1],
                    isVis ? healthColorVisible[2] : healthColorHidden[2],
                    isVis ? healthColorVisible[3] : healthColorHidden[3]
                ));
                dl->AddRectFilled({ x - 6, y - 1 }, { x - 2, y + height + 1 }, IM_COL32(0, 0, 0, 150));
                dl->AddRectFilled({ x - 5, y + height - (height * hpFrac) }, { x - 3, y + height }, hpCol);
            }

            if (espNameEnabled && ent.controller)
            {
                char buf[64]{};
                bool hasName = ReadControllerName(reinterpret_cast<uintptr_t>(ent.controller), buf, sizeof(buf), nullptr);

                if (!hasName || buf[0] == '\0') {
                    _snprintf_s(buf, sizeof(buf), _TRUNCATE, "player%d", ent.index);
                    hasName = true;
                }

                if (hasName && buf[0] != '\0') {
                    ImVec2 ts;
                    if (esp_font) {
                        ts = esp_font->CalcTextSizeA(esp_font->FontSize, FLT_MAX, 0.0f, buf);
                    } else {
                        ts = ImGui::CalcTextSize(buf);
                    }

                    ImVec2 textPos = { x + (width - ts.x) * 0.5f, y - ts.y - 2.0f };
                    if (textPos.y < 2.0f) textPos.y = 2.0f;

                    if (esp_font) {
                        dl->AddText(esp_font, esp_font->FontSize, { textPos.x + 1.0f, textPos.y + 1.0f }, IM_COL32(0, 0, 0, 255), buf);
                        dl->AddText(esp_font, esp_font->FontSize, textPos, nameCol, buf);
                    } else {
                        dl->AddText({ textPos.x + 1.0f, textPos.y + 1.0f }, IM_COL32(0, 0, 0, 255), buf);
                        dl->AddText(textPos, nameCol, buf);
                    }
                }
            }

            if (espDistanceEnabled)
            {
                if (!localOrigin.IsZero())
                {
                    float distUnits = (origin - localOrigin).Length();
                    float distMeters = distUnits * 0.0254f;

                    char distBuf[32];
                    snprintf(distBuf, sizeof(distBuf), "[ %.0fm ]", distMeters);

                    ImVec2 ts;
                    if (esp_font) {
                        ts = esp_font->CalcTextSizeA(esp_font->FontSize, FLT_MAX, 0.0f, distBuf);
                    } else {
                        ts = ImGui::CalcTextSize(distBuf);
                    }

                    ImVec2 textPos = { x + width + 4.0f, y };
                    const ImU32 distCol = ImGui::ColorConvertFloat4ToU32(ImVec4(
                        isVis ? distanceColorVisible[0] : distanceColorHidden[0],
                        isVis ? distanceColorVisible[1] : distanceColorHidden[1],
                        isVis ? distanceColorVisible[2] : distanceColorHidden[2],
                        isVis ? distanceColorVisible[3] : distanceColorHidden[3]
                    ));

                    if (esp_font) {
                        dl->AddText(esp_font, esp_font->FontSize, { textPos.x + 1.0f, textPos.y + 1.0f }, IM_COL32(0, 0, 0, 255), distBuf);
                        dl->AddText(esp_font, esp_font->FontSize, { textPos.x - 1.0f, textPos.y - 1.0f }, IM_COL32(0, 0, 0, 255), distBuf);
                        dl->AddText(esp_font, esp_font->FontSize, { textPos.x + 1.0f, textPos.y - 1.0f }, IM_COL32(0, 0, 0, 255), distBuf);
                        dl->AddText(esp_font, esp_font->FontSize, { textPos.x - 1.0f, textPos.y + 1.0f }, IM_COL32(0, 0, 0, 255), distBuf);
                        dl->AddText(esp_font, esp_font->FontSize, textPos, distCol, distBuf);
                    } else {
                        dl->AddText({ textPos.x + 1.0f, textPos.y + 1.0f }, IM_COL32(0, 0, 0, 255), distBuf);
                        dl->AddText({ textPos.x - 1.0f, textPos.y - 1.0f }, IM_COL32(0, 0, 0, 255), distBuf);
                        dl->AddText({ textPos.x + 1.0f, textPos.y - 1.0f }, IM_COL32(0, 0, 0, 255), distBuf);
                        dl->AddText({ textPos.x - 1.0f, textPos.y + 1.0f }, IM_COL32(0, 0, 0, 255), distBuf);
                        dl->AddText(textPos, distCol, distBuf);
                    }
                }
            }

            if (espWeaponEnabled)
            {
                const ImU32 weapCol = ImGui::ColorConvertFloat4ToU32(ImVec4(
                    isVis ? weaponColorVisible[0] : weaponColorHidden[0],
                    isVis ? weaponColorVisible[1] : weaponColorHidden[1],
                    isVis ? weaponColorVisible[2] : weaponColorHidden[2],
                    isVis ? weaponColorVisible[3] : weaponColorHidden[3]
                ));
                uintptr_t weaponServices = Utils::SafeRead<uintptr_t>(reinterpret_cast<uintptr_t>(pawn) + Offsets::m_pWeaponServices);
                if (weaponServices) {
                    uint32_t activeWeaponHandle = Utils::SafeRead<uint32_t>(weaponServices + Offsets::m_hActiveWeapon);
                    if (activeWeaponHandle != 0xFFFFFFFF) {
                        C_BaseEntity* weaponEnt = EntityManager::Get().GetEntityFromHandle(activeWeaponHandle);
                        if (weaponEnt) {
                            short weaponIdx = Utils::SafeRead<short>(reinterpret_cast<uintptr_t>(weaponEnt) + Offsets::sc_m_AttributeManager + Offsets::sc_m_Item + Offsets::sc_m_iItemDefinitionIndex);
                            if (weaponIdx > 0 && weaponIdx < 1000) {
                                const char* weaponIconStr = GetIconFromWeaponId(weaponIdx);
                                if (weaponIconStr && weaponIconStr[0] != '\0') {
                                    ImVec2 wTs;
                                    if (weapon_icon_font) {
                                        wTs = weapon_icon_font->CalcTextSizeA(weapon_icon_font->FontSize, FLT_MAX, 0.0f, weaponIconStr);
                                    } else {
                                        wTs = ImGui::CalcTextSize(weaponIconStr);
                                    }
                                    
                                    ImVec2 wPos = { x + (width - wTs.x) * 0.5f, y + height + 2.0f };
                                    
                                    if (weapon_icon_font) {
                                        dl->AddText(weapon_icon_font, weapon_icon_font->FontSize, { wPos.x + 1.0f, wPos.y + 1.0f }, IM_COL32(0, 0, 0, 255), weaponIconStr);
                                        dl->AddText(weapon_icon_font, weapon_icon_font->FontSize, wPos, weapCol, weaponIconStr);
                                    } else {
                                        dl->AddText({ wPos.x + 1.0f, wPos.y + 1.0f }, IM_COL32(0, 0, 0, 255), weaponIconStr);
                                        dl->AddText(wPos, weapCol, weaponIconStr);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (espSkeletonEnabled)
            {
                
                const ImU32 skelColVis = ImGui::ColorConvertFloat4ToU32(ImVec4(
                    skeletonColorVisible[0],
                    skeletonColorVisible[1],
                    skeletonColorVisible[2],
                    skeletonColorVisible[3]
                ));
                const ImU32 skelColHidden = ImGui::ColorConvertFloat4ToU32(ImVec4(
                    skeletonColorHidden[0],
                    skeletonColorHidden[1],
                    skeletonColorHidden[2],
                    skeletonColorHidden[3]
                ));

                if (espSkeletonType == 1) 
                {
                    for (const auto& chain : g_BoneChains)
                    {
                        std::vector<ImVec2> points;

                        for (auto boneId : chain)
                        {
                            Vector bonePos = Utils::GetBonePos(pawn, boneId);
                            Vector screenPos;

                            if (!Utils::WorldToScreen(bonePos, screenPos, (float*)Globals::ViewMatrix, sw, sh))
                                continue;

                            points.push_back(ImVec2(screenPos.x, screenPos.y));
                        }

                        if (points.size() < 2)
                            continue;

                        
                        points.insert(points.begin(), points.front());
                        points.push_back(points.back());

                        
                        bool chainVisible = false;
                        for (auto boneId : chain)
                        {
                            Vector bonePos = Utils::GetBonePos(pawn, boneId);
                            if (IsBoneVisibleRaycast(bonePos)) { chainVisible = true; break; }
                        }
                        ImU32 segCol = chainVisible ? skelColVis : skelColHidden;

                        ImVec2 last = {};
                        bool first = true;

                        for (size_t i = 0; i + 3 < points.size(); i++)
                        {
                            const auto& p0 = points[i];
                            const auto& p1 = points[i + 1];
                            const auto& p2 = points[i + 2];
                            const auto& p3 = points[i + 3];

                            for (float t = 0.f; t <= 1.f; t += 0.08f)
                            {
                                auto pt = CatmullRom(p0, p1, p2, p3, t);

                                if (first)
                                {
                                    last = pt;
                                    first = false;
                                    continue;
                                }

                                dl->AddLine(last, pt, segCol, Globals::esp_skeleton_thickness);
                                last = pt;
                            }
                        }
                    }
                }
                else 
                {
                    for (const auto& conn : Bones::connections)
                    {
                        Vector b1 = Utils::GetBonePos(pawn, conn.bone1);
                        Vector b2 = Utils::GetBonePos(pawn, conn.bone2);
                        Vector sb1, sb2;
                        if (Utils::WorldToScreen(b1, sb1, (float*)Globals::ViewMatrix, sw, sh) &&
                            Utils::WorldToScreen(b2, sb2, (float*)Globals::ViewMatrix, sw, sh))
                        {
                            
                            bool b1Vis = IsBoneVisibleRaycast(b1);
                            bool b2Vis = IsBoneVisibleRaycast(b2);
                            ImU32 segCol = (b1Vis || b2Vis) ? skelColVis : skelColHidden;

                            dl->AddLine({ sb1.x, sb1.y }, { sb2.x, sb2.y }, segCol, Globals::esp_skeleton_thickness);
                        }
                    }
                }
            }

            
            if (espHeadEnabled)
            {
                const ImU32 headColVis = ImGui::ColorConvertFloat4ToU32(ImVec4(
                    skeletonColorVisible[0],
                    skeletonColorVisible[1],
                    skeletonColorVisible[2],
                    skeletonColorVisible[3]
                ));
                const ImU32 headColHidden = ImGui::ColorConvertFloat4ToU32(ImVec4(
                    skeletonColorHidden[0],
                    skeletonColorHidden[1],
                    skeletonColorHidden[2],
                    skeletonColorHidden[3]
                ));

                Vector head3D = Utils::GetBonePos(pawn, BoneID::Head);
                Vector neck3D = Utils::GetBonePos(pawn, BoneID::Neck);
                Vector sHeadPos, sNeckPos;
                if (Utils::WorldToScreen(head3D, sHeadPos, (float*)Globals::ViewMatrix, sw, sh) &&
                    Utils::WorldToScreen(neck3D, sNeckPos, (float*)Globals::ViewMatrix, sw, sh))
                {
                    bool headVis = IsBoneVisibleRaycast(head3D);
                    ImU32 headCol = headVis ? headColVis : headColHidden;

                    float radius = std::abs(sHeadPos.y - sNeckPos.y) + 2.0f;
                    dl->AddCircle({ sHeadPos.x, sHeadPos.y }, radius, headCol, 0, Globals::esp_skeleton_thickness);
                }
            }
        }
    }
}

void ESP::Render()
{
    __try
    {
        RenderInternal();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}
