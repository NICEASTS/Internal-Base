#include <string>
#include <chrono>
#include <deque>
#include <mutex>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "Visuals.h"
#include "esp/Esp.h"
#include "chams/Chams.h"
#include "chamsv2/ChamsV2.h"
#include "enemycounter/EnemyCounter.h"
#include "../../ext/imgui/imgui.h"
#include "../../sdk/utils/Globals.h"
#include "../../sdk/memory/Offsets.h"
#include "../../sdk/memory/PatternScan.h"
#include "../../sdk/entity/EntityManager.h"
#include "../../sdk/utils/Raycasting.h"
#include "../../sdk/utils/Utils.h"

namespace
{
    struct GrenadeTrajectoryFrame
    {
        std::vector<Vector> points;
        std::vector<size_t> impactIndices;
    };

    static std::deque<GrenadeTrajectoryFrame> g_grenadeFrameQueue;
    static std::mutex g_grenadeQueueMutex;
    static GrenadeTrajectoryFrame g_lastGrenadeFrame;
    static Vector g_prevOrigin{};
    static Vector g_smoothedVelocity{};
    static auto g_prevOriginTime = std::chrono::steady_clock::now();

    static float Dot(const Vector& a, const Vector& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static float LengthSq(const Vector& v)
    {
        return Dot(v, v);
    }

    static uintptr_t GetEntityFromHandle(uint32_t handle)
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

    static uintptr_t GetActiveWeapon(uintptr_t pawnAddr)
    {
        uintptr_t weaponServices = Utils::SafeRead<uintptr_t>(pawnAddr + Offsets::m_pWeaponServices);
        if (!Utils::IsValidPtr(weaponServices)) return 0;

        uint32_t activeHandle = Utils::SafeRead<uint32_t>(weaponServices + Offsets::m_hActiveWeapon);
        return GetEntityFromHandle(activeHandle);
    }

    static uint16_t GetWeaponDefinition(uintptr_t weapon)
    {
        if (!Utils::IsValidPtr(weapon)) return 0;
        uintptr_t item = weapon + Offsets::sc_m_AttributeManager + Offsets::sc_m_Item;
        return Utils::SafeRead<uint16_t>(item + Offsets::sc_m_iItemDefinitionIndex);
    }

    static Globals::AimWeaponGroup GetAimWeaponGroup(uint16_t defIndex)
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

    static bool GetCurrentAimFov(float& outFov)
    {
        C_CSPlayerPawn* localPawn = EntityManager::Get().GetLocalPawn();
        if (!localPawn || !Utils::SafeAlive(localPawn))
            return false;

        uintptr_t activeWeapon = GetActiveWeapon(reinterpret_cast<uintptr_t>(localPawn));
        Globals::AimWeaponGroup group = GetAimWeaponGroup(GetWeaponDefinition(activeWeapon));
        if (group < 0 || group >= Globals::AIM_GROUP_COUNT)
            return false;

        const auto& settings = Globals::aim_weapon_groups[group];
        if (!settings.enabled)
            return false;

        float currentFov = Globals::visuals_fov_enabled ? Globals::visuals_fov : 90.0f;
        outFov = settings.fov * (currentFov / 90.0f);
        return outFov > 0.01f;
    }

    static Vector NormalizeSafe(const Vector& v)
    {
        float lenSq = LengthSq(v);
        if (lenSq <= 1e-8f)
            return {};
        return v * (1.0f / std::sqrt(lenSq));
    }

    static bool IsGrenadeWeapon(uint16_t defIndex)
    {
        return defIndex == WEP_HeGrenade ||
            defIndex == WEP_Molotov ||
            defIndex == WEP_IncGrenade ||
            defIndex == WEP_FlashBang ||
            defIndex == WEP_SmokeGrenade ||
            defIndex == WEP_Decoy;
    }

    static uintptr_t GetActiveWeapon(C_CSPlayerPawn* localPawn)
    {
        uintptr_t pawn = reinterpret_cast<uintptr_t>(localPawn);
        uintptr_t weaponServices = Utils::SafeRead<uintptr_t>(pawn + Offsets::m_pWeaponServices);
        if (!Utils::IsValidPtr(weaponServices))
            return 0;

        uint32_t activeHandle = Utils::SafeRead<uint32_t>(weaponServices + Offsets::m_hActiveWeapon);
        if ((activeHandle & 0x7FFF) == 0)
            return 0;

        auto* activeEntity = EntityManager::Get().GetEntityFromHandle(activeHandle);
        return reinterpret_cast<uintptr_t>(activeEntity);
    }

    static uint16_t GetActiveWeaponDefIndex(C_CSPlayerPawn* localPawn)
    {
        uintptr_t activeWeapon = GetActiveWeapon(localPawn);
        if (!Utils::IsValidPtr(activeWeapon))
            return 0;

        uintptr_t item = activeWeapon + Offsets::sc_m_AttributeManager + Offsets::sc_m_Item;
        return Utils::SafeRead<uint16_t>(item + Offsets::sc_m_iItemDefinitionIndex);
    }

    static Vector GetLocalEyePosition(C_CSPlayerPawn* localPawn)
    {
        uintptr_t pawn = reinterpret_cast<uintptr_t>(localPawn);
        uintptr_t scene = Utils::SafeRead<uintptr_t>(pawn + Offsets::m_pGameSceneNode);
        if (!Utils::IsValidPtr(scene))
            return {};

        Vector origin = Utils::SafeRead<Vector>(scene + Offsets::m_vecAbsOrigin);
        Vector viewOffset = Utils::SafeRead<Vector>(pawn + Offsets::m_vecViewOffset);
        return origin + viewOffset;
    }

    static Vector EstimatePlayerVelocity(C_CSPlayerPawn* localPawn)
    {
        uintptr_t pawn = reinterpret_cast<uintptr_t>(localPawn);
        uintptr_t scene = Utils::SafeRead<uintptr_t>(pawn + Offsets::m_pGameSceneNode);
        if (!Utils::IsValidPtr(scene))
            return {};

        Vector currentOrigin = Utils::SafeRead<Vector>(scene + Offsets::m_vecAbsOrigin);
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - g_prevOriginTime).count();

        if (dt > 0.0005f && dt < 0.250f)
        {
            Vector instantVel = (currentOrigin - g_prevOrigin) / dt;
            g_smoothedVelocity = g_smoothedVelocity * 0.65f + instantVel * 0.35f;
        }

        g_prevOrigin = currentOrigin;
        g_prevOriginTime = now;
        return g_smoothedVelocity;
    }

    static Vector CalculateThrowVelocity(const Vector& viewAngles, float throwStrength, const Vector& playerVelocity)
    {
        Vector adjustedAngles = viewAngles;
        if (adjustedAngles.x < -89.f)
            adjustedAngles.x += 360.f;
        else if (adjustedAngles.x > 89.f)
            adjustedAngles.x -= 360.f;

        adjustedAngles.x -= (90.0f - std::fabs(adjustedAngles.x)) * 10.0f / 90.0f;

        Vector direction{};
        Utils::AngleVectors(adjustedAngles, direction);

        Vector throwVelocity = direction * (throwStrength * 0.7f + 0.3f) * 1115.0f;
        return throwVelocity + playerVelocity * 1.25f;
    }

    static GrenadeTrajectoryFrame SimulateGrenadeTrajectory(const Vector& startPos, const Vector& velocity)
    {
        GrenadeTrajectoryFrame frame;
        frame.points.reserve(1024);
        frame.impactIndices.reserve(64);

        Vector currentPos = startPos;
        Vector currentVel = velocity;

        const int maxSteps = 1024;
        const int interpolationSteps = 12;
        const int maxBounces = (std::max)(1, Globals::grenade_prediction_max_bounces);
        const float sphereRadius = (std::max)(1.0f, Globals::grenade_prediction_radius);
        const float restitution = (std::clamp)(Globals::grenade_prediction_bounce, 0.05f, 0.95f);
        const float friction = (std::clamp)(Globals::grenade_prediction_friction, 0.05f, 0.98f);

        int bounceCount = 0;
        for (int step = 0; step < maxSteps && bounceCount < maxBounces; ++step)
        {
            frame.points.emplace_back(currentPos);

            currentVel.z -= CS_GRAVITY * CS_TICK_INTERVAL;
            Vector nextPos = currentPos + currentVel * CS_TICK_INTERVAL;
            bool collided = false;

            for (int i = 1; i <= interpolationSteps; ++i)
            {
                float t = static_cast<float>(i) / static_cast<float>(interpolationSteps);
                Vector samplePos = currentPos + (nextPos - currentPos) * t;

                if (Raycasting::Get().TraceHullSphere(samplePos, sphereRadius))
                {
                    frame.points.emplace_back(samplePos);
                    frame.impactIndices.emplace_back(frame.points.size() - 1);

                    Vector surfaceNormal{};
                    if (!Raycasting::Get().GetSurfaceNormal(samplePos, sphereRadius + 5.0f, surfaceNormal))
                        break;

                    surfaceNormal = NormalizeSafe(surfaceNormal);
                    float normalVelocity = Dot(currentVel, surfaceNormal);
                    Vector velocityNormal = surfaceNormal * normalVelocity;
                    Vector velocityTangent = currentVel - velocityNormal;
                    currentVel = (velocityNormal * -restitution) + (velocityTangent * friction);
                    currentPos = samplePos + surfaceNormal * (sphereRadius * 0.30f + 0.05f);
                    collided = true;
                    ++bounceCount;
                    break;
                }
            }

            if (!collided)
                currentPos = nextPos;

            if (LengthSq(currentVel) < 16.0f)
                break;
        }

        return frame;
    }

    static void ClearGrenadeFrames()
    {
        std::lock_guard<std::mutex> lock(g_grenadeQueueMutex);
        g_grenadeFrameQueue.clear();
        g_lastGrenadeFrame = {};
    }

    static void UpdateGrenadePredictionFrame()
    {
        if (!Globals::grenade_prediction_enabled)
        {
            ClearGrenadeFrames();
            return;
        }

        if (!Raycasting::Get().IsLoaded())
        {
            ClearGrenadeFrames();
            return;
        }

        C_CSPlayerPawn* localPawn = EntityManager::Get().GetLocalPawn();
        if (!localPawn || !Utils::SafeAlive(localPawn))
        {
            ClearGrenadeFrames();
            return;
        }

        uint16_t weaponDef = GetActiveWeaponDefIndex(localPawn);
        if (!IsGrenadeWeapon(weaponDef))
        {
            ClearGrenadeFrames();
            return;
        }

        bool lmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        bool rmb = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        if (!lmb && !rmb)
        {
            ClearGrenadeFrames();
            return;
        }

        float throwStrength = 1.0f;
        if (lmb && rmb)
            throwStrength = 0.0f;
        else if (rmb)
            throwStrength = 0.5f;

        uintptr_t client = Memory::GetModuleBase("client.dll");
        if (!client)
            return;

        Vector viewAngles = Utils::SafeRead<Vector>(client + Offsets::dwViewAngles);
        Vector playerVelocity = EstimatePlayerVelocity(localPawn);
        Vector throwVelocity = CalculateThrowVelocity(viewAngles, throwStrength, playerVelocity);
        Vector startPos = GetLocalEyePosition(localPawn);
        if (startPos.IsZero())
            return;

        GrenadeTrajectoryFrame frame = SimulateGrenadeTrajectory(startPos, throwVelocity);
        if (frame.points.size() < 2)
            return;

        {
            std::lock_guard<std::mutex> lock(g_grenadeQueueMutex);
            g_grenadeFrameQueue.push_back(std::move(frame));
            while (g_grenadeFrameQueue.size() > 4)
                g_grenadeFrameQueue.pop_front();
        }
    }

    static void DrawGrenadePrediction()
    {
        GrenadeTrajectoryFrame frameToDraw;
        {
            std::lock_guard<std::mutex> lock(g_grenadeQueueMutex);
            if (!g_grenadeFrameQueue.empty())
            {
                frameToDraw = std::move(g_grenadeFrameQueue.front());
                g_grenadeFrameQueue.pop_front();
                g_lastGrenadeFrame = frameToDraw;
            }
            else
            {
                frameToDraw = g_lastGrenadeFrame;
            }
        }

        if (frameToDraw.points.size() < 2)
            return;

        auto* drawList = ImGui::GetBackgroundDrawList();
        const ImU32 lineColor = ImGui::ColorConvertFloat4ToU32(*(ImVec4*)Globals::grenade_prediction_color);
        const ImU32 hitColor = ImGui::ColorConvertFloat4ToU32(*(ImVec4*)Globals::grenade_prediction_hit_color);
        const float thickness = (std::max)(1.0f, Globals::grenade_prediction_thickness);

        size_t impactCursor = 0;
        for (size_t i = 1; i < frameToDraw.points.size(); ++i)
        {
            while (impactCursor < frameToDraw.impactIndices.size() && frameToDraw.impactIndices[impactCursor] < i)
                ++impactCursor;
            bool isImpact = impactCursor < frameToDraw.impactIndices.size() && frameToDraw.impactIndices[impactCursor] == i;

            Vector screen1{}, screen2{};
            if (Utils::WorldToScreen(frameToDraw.points[i - 1], screen1, (float*)Globals::ViewMatrix, (float)Globals::ScreenWidth, (float)Globals::ScreenHeight) &&
                Utils::WorldToScreen(frameToDraw.points[i], screen2, (float*)Globals::ViewMatrix, (float)Globals::ScreenWidth, (float)Globals::ScreenHeight))
            {
                drawList->AddLine(ImVec2(screen1.x, screen1.y), ImVec2(screen2.x, screen2.y), isImpact ? hitColor : lineColor, thickness);
                if (isImpact && Globals::grenade_prediction_draw_points)
                    drawList->AddCircleFilled(ImVec2(screen2.x, screen2.y), thickness + 1.5f, hitColor, 12);
            }
        }
    }
}

static void RenderInternal()
{
    static auto lastGrenadeUpdate = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastGrenadeUpdate).count() >= 8)
    {
        UpdateGrenadePredictionFrame();
        lastGrenadeUpdate = now;
    }

    ESP::Render();
    Chams::Render();

    
    if ((Globals::chamsv2_enabled || Globals::chamsv2_enabled_team) && ChamsV2::g_MeshRenderer.IsReady())
    {
        
        struct DissolveEntry {
            ChamsV2::DissolveSnapshot snapshot;
            float startTime;
            float duration;
            int   style;
            float intensity;
            float edgeColor[3];
        };

        static std::unordered_map<uintptr_t, bool> s_chams_alive_state;
        static std::vector<DissolveEntry> s_dissolving;

        auto entities = EntityManager::Get().GetEntities();
        C_CSPlayerPawn* localPawn = EntityManager::Get().GetLocalPawn();
        C_CSPlayerPawn* observedPawn = EntityManager::Get().GetLocalObservedPawn();
        float curTime = static_cast<float>(GetTickCount64()) / 1000.0f;

        if (localPawn)
        {
            std::unordered_set<uintptr_t> seenPawns;
            seenPawns.reserve(entities.size());

            for (const auto& ent : entities)
            {
                if (!ent.pawn || !ent.controller) continue;
                if (ent.pawn == localPawn) continue;
                if (observedPawn && ent.pawn == observedPawn) continue;
                if (!Utils::IsValidPtr(reinterpret_cast<uintptr_t>(ent.pawn))) continue;

                uintptr_t pawnAddr = reinterpret_cast<uintptr_t>(ent.pawn);
                seenPawns.insert(pawnAddr);
                bool aliveNow = Utils::SafeAlive(ent.pawn);

                
                if (Globals::chams_kill_effect)
                {
                    const bool chamsv2On = ent.isEnemy ? Globals::chamsv2_enabled : Globals::chamsv2_enabled_team;
                    auto it = s_chams_alive_state.find(pawnAddr);
                    bool wasAlive = (it == s_chams_alive_state.end()) ? aliveNow : it->second;

                    if (wasAlive && !aliveNow && chamsv2On)
                    {
                        
                        if (s_dissolving.size() < 8) 
                        {
                            DissolveEntry entry{};
                            if (ChamsV2::g_MeshRenderer.CreateSnapshot(ent.pawn, ent.isEnemy, entry.snapshot))
                            {
                                entry.startTime = curTime;
                                entry.duration = Globals::chams_kill_effect_duration;
                                entry.style = Globals::chams_kill_effect_style;
                                entry.intensity = Globals::chams_kill_effect_intensity;
                                entry.edgeColor[0] = Globals::chams_kill_effect_edge_color[0];
                                entry.edgeColor[1] = Globals::chams_kill_effect_edge_color[1];
                                entry.edgeColor[2] = Globals::chams_kill_effect_edge_color[2];
                                s_dissolving.push_back(std::move(entry));
                            }
                        }
                    }
                    s_chams_alive_state[pawnAddr] = aliveNow;
                }

                
                const bool chamsv2Enabled = ent.isEnemy ? Globals::chamsv2_enabled : Globals::chamsv2_enabled_team;
                if (!chamsv2Enabled) continue;

                ChamsV2::g_MeshRenderer.RenderPlayer(ent.controller, ent.pawn, 1.0f, ent.isEnemy);
            }

            
            for (auto it = s_chams_alive_state.begin(); it != s_chams_alive_state.end(); )
            {
                if (seenPawns.find(it->first) == seenPawns.end())
                    it = s_chams_alive_state.erase(it);
                else
                    ++it;
            }

            
            for (int i = (int)s_dissolving.size() - 1; i >= 0; --i)
            {
                auto& entry = s_dissolving[i];
                float elapsed = curTime - entry.startTime;
                if (elapsed > entry.duration)
                {
                    s_dissolving.erase(s_dissolving.begin() + i);
                    continue;
                }

                float t = elapsed / entry.duration;
                
                float dissolveAmount = 1.0f - (1.0f - t) * (1.0f - t);
                dissolveAmount *= entry.intensity;
                if (dissolveAmount > 1.0f) dissolveAmount = 1.0f;

                float edgeWidth = 0.06f + (1.0f - t) * 0.06f; 
                float noiseScale = 0.12f + (entry.style == 2 ? 0.06f : 0.0f); 

                ChamsV2::g_MeshRenderer.RenderSnapshot(
                    entry.snapshot, dissolveAmount, entry.style,
                    edgeWidth, noiseScale, entry.edgeColor, entry.intensity);
            }

            
            if (!Globals::chams_kill_effect && !s_dissolving.empty())
            {
                s_dissolving.clear();
                s_chams_alive_state.clear();
            }
        }
    }

    if (Globals::aim_draw_fov && Globals::aim_enabled)
    {
        float aimFov = 0.0f;
        if (!GetCurrentAimFov(aimFov))
            aimFov = 0.0f;

        float referenceFov = 90.0f; 
        if (aimFov > 0.01f)
        {
            ImGui::GetBackgroundDrawList()->AddCircle(
                ImVec2(Globals::GameViewportX + Globals::ScreenWidth / 2.f, Globals::GameViewportY + Globals::ScreenHeight / 2.f),
                aimFov * (Globals::ScreenHeight / referenceFov),
                ImGui::ColorConvertFloat4ToU32(*(ImVec4*)Globals::aim_fov_color),
                64,
                1.0f
            );
        }
    }

    if (Globals::trigger_draw_fov && Globals::trigger_enabled)
    {
        float referenceFov = 90.0f; 
        ImGui::GetBackgroundDrawList()->AddCircle(
            ImVec2(Globals::GameViewportX + Globals::ScreenWidth / 2.f, Globals::GameViewportY + Globals::ScreenHeight / 2.f),
            Globals::trigger_fov * (Globals::ScreenHeight / referenceFov), 
            ImGui::ColorConvertFloat4ToU32(*(ImVec4*)Globals::trigger_fov_color),
            64,
            1.0f
        );
    }

    DrawGrenadePrediction();

    auto entities = EntityManager::Get().GetEntities();
    C_CSPlayerPawn* localPawn = EntityManager::Get().GetLocalPawn();
    if (localPawn)
    {
        C_CSPlayerPawn* observedPawn = EntityManager::Get().GetLocalObservedPawn();
        const int localTeam = Utils::SafeTeam(localPawn);
        const bool glowEnemyEnabled = Globals::esp_glow || Globals::wireframe_enemy_enabled;
        const bool glowTeamEnabled = Globals::esp_glow_team;
        const Vector localEyePos = GetLocalEyePosition(localPawn);
        const bool canCheckVisibility = Raycasting::Get().IsLoaded();

        auto disableGlow = [](uintptr_t glowProperty)
        {
            Utils::SafeWrite<uint32_t>(glowProperty + Offsets::m_glowColorOverride, 0u);
            Utils::SafeWrite<int>(glowProperty + Offsets::m_iGlowType, 0);
            Utils::SafeWrite<bool>(glowProperty + 0x50, false);
            Utils::SafeWrite<bool>(glowProperty + Offsets::m_bGlowing, false);
        };

        for (const auto& entity : entities)
        {
            C_CSPlayerPawn* pawn = entity.pawn;
            if (!Utils::IsValidPtr(reinterpret_cast<uintptr_t>(pawn))) continue;
            if (pawn == localPawn) continue;
            if (observedPawn && pawn == observedPawn) continue;

            uintptr_t glowProperty = reinterpret_cast<uintptr_t>(pawn) + Offsets::m_Glow;
            if (!Utils::IsValidPtr(glowProperty))
                continue;

            const int pawnTeam = Utils::SafeTeam(pawn);
            const bool isEnemy = (localTeam > 0 && pawnTeam > 0) ? (pawnTeam != localTeam) : entity.isEnemy;
            const bool glowEnabled = isEnemy ? glowEnemyEnabled : glowTeamEnabled;
            const bool isAlive = Utils::SafeAlive(pawn);

            if (!glowEnabled || (!isAlive && !Globals::esp_glow_dead))
            {
                disableGlow(glowProperty);
                continue;
            }

            const float* glowHiddenColor = isEnemy ? Globals::esp_glow_color : Globals::esp_glow_color_team;
            const float* glowVisibleColor = isEnemy ? Globals::esp_glow_color_vis : Globals::esp_glow_color_vis_team;
            bool isVisible = false;
            if (canCheckVisibility && localEyePos.Length() > 0.0f)
            {
                Vector headPos = Utils::GetBonePos(pawn, BoneID::Head);
                if (headPos.Length() > 0.0f)
                    isVisible = Raycasting::Get().IsBoneVisible(localEyePos, headPos);
            }
            const float* glowColor = isVisible ? glowVisibleColor : glowHiddenColor;

            Utils::SafeWrite<float>(glowProperty + 0x8, glowColor[0]);
            Utils::SafeWrite<float>(glowProperty + 0xC, glowColor[1]);
            Utils::SafeWrite<float>(glowProperty + 0x10, glowColor[2]);

            uint8_t r = static_cast<uint8_t>((std::clamp)(glowColor[0], 0.0f, 1.0f) * 255.0f);
            uint8_t g = static_cast<uint8_t>((std::clamp)(glowColor[1], 0.0f, 1.0f) * 255.0f);
            uint8_t b = static_cast<uint8_t>((std::clamp)(glowColor[2], 0.0f, 1.0f) * 255.0f);
            uint8_t a = static_cast<uint8_t>((std::clamp)(glowColor[3], 0.0f, 1.0f) * 255.0f);
            uint32_t color = r | (g << 8) | (b << 16) | (a << 24);
            Utils::SafeWrite<uint32_t>(glowProperty + Offsets::m_glowColorOverride, color);

            Utils::SafeWrite<int>(glowProperty + 0x38, 10000);
            Utils::SafeWrite<int>(glowProperty + 0x3C, 0);
            Utils::SafeWrite<int>(glowProperty + Offsets::m_iGlowType, 3);
            Utils::SafeWrite<bool>(glowProperty + 0x50, true);
            Utils::SafeWrite<bool>(glowProperty + Offsets::m_bGlowing, true);
        }
    }
}

void Visuals::Render()
{
    __try
    {
        RenderInternal();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        
    }

    
    __try
    {
        Chams::Flush();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    
    __try
    {
        if (Globals::chamsv2_enabled || Globals::chamsv2_enabled_team)
            ChamsV2::g_MeshRenderer.Flush();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}
