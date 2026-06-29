#include "Misc.h"
#include <Windows.h>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <algorithm>
#include <mutex>
#include <cmath>
#include <mmsystem.h>
#include "../../sdk/utils/Globals.h"
#include "../../sdk/utils/Utils.h"
#include "../../sdk/utils/Vector.h"
#include "../../sdk/memory/Offsets.h"
#include "../../sdk/memory/PatternScan.h"
#include "../../sdk/entity/EntityManager.h"
#include "../../sdk/utils/CCSGOInput.h"
#include "../../../ext/imgui/imgui.h"
#include "../../sdk/interfaces/Interfaces.h"

#pragma comment(lib, "winmm.lib")

namespace fs = std::filesystem;

namespace
{
	constexpr uint8_t MOVETYPE_NOCLIP = 7;
	constexpr uint8_t MOVETYPE_LADDER = 9;
}





static bool s_hitsoundInitialized = false;
static int  s_previousTotalHits = 0;
static float s_hitmarkerAlpha = 0.f;
static std::chrono::steady_clock::time_point s_hitmarkerStartTime;
static float s_lastLocalHitTime = -1000.0f;

static float Misc_GetTime()
{
	return static_cast<float>(GetTickCount64()) / 1000.0f;
}

void Misc::InitHitsound()
{
	if (s_hitsoundInitialized) return;
	s_hitsoundInitialized = true;

	Globals::hitsound_files.clear();

	try {
		if (fs::exists(Globals::hitsound_dir) && fs::is_directory(Globals::hitsound_dir))
		{
			for (const auto& entry : fs::directory_iterator(Globals::hitsound_dir))
			{
				if (!entry.is_regular_file()) continue;

				std::string ext = entry.path().extension().string();
				for (auto& c : ext) c = (char)tolower(c);

				if (ext == ".wav")
				{
					Globals::hitsound_files.push_back(entry.path().filename().string());
				}
			}
		}
	}
	catch (...) {}
}

void Misc::PlayHitsound()
{
	if (!Globals::hitsound_enabled) return;
	if (Globals::hitsound_selected < 0 || Globals::hitsound_selected >= (int)Globals::hitsound_files.size()) return;

	try {
		std::string fullPath = Globals::hitsound_dir + "\\" + Globals::hitsound_files[Globals::hitsound_selected];
		PlaySoundA(fullPath.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
	}
	catch (...) {}
}





static bool s_deathsoundInitialized = false;
static std::unordered_map<uintptr_t, bool> s_ds_enemyAlive;

void Misc::InitDeathSound()
{
	if (s_deathsoundInitialized) return;
	s_deathsoundInitialized = true;

	Globals::deathsound_files.clear();

	try {
		if (fs::exists(Globals::deathsound_dir) && fs::is_directory(Globals::deathsound_dir))
		{
			for (const auto& entry : fs::directory_iterator(Globals::deathsound_dir))
			{
				if (!entry.is_regular_file()) continue;

				std::string ext = entry.path().extension().string();
				for (auto& c : ext) c = (char)tolower(c);

				if (ext == ".wav")
				{
					Globals::deathsound_files.push_back(entry.path().filename().string());
				}
			}
		}
	}
	catch (...) {}
}

static void PlayDeathSoundFile(const std::string& filePath)
{
	
	PlaySoundA(filePath.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

void Misc::DeathSoundUpdate()
{
	if (!Globals::deathsound_enabled)
	{
		s_ds_enemyAlive.clear();
		return;
	}
	if (Globals::deathsound_selected < 0 || Globals::deathsound_selected >= (int)Globals::deathsound_files.size())
		return;

	auto entities = EntityManager::Get().GetEntities();
	std::unordered_set<uintptr_t> seen;
	seen.reserve(entities.size());

	float now = Misc_GetTime();
	for (const auto& ent : entities)
	{
		if (!ent.isEnemy || !ent.pawn)
			continue;

		uintptr_t pawnAddr = reinterpret_cast<uintptr_t>(ent.pawn);
		if (!Utils::IsValidPtr(pawnAddr))
			continue;

		seen.insert(pawnAddr);
		bool aliveNow = Utils::SafeAlive(ent.pawn);
		auto it = s_ds_enemyAlive.find(pawnAddr);
		bool wasAlive = (it == s_ds_enemyAlive.end()) ? aliveNow : it->second;

		
		if (wasAlive && !aliveNow && (now - s_lastLocalHitTime) <= 1.0f)
		{
			std::string fullPath = Globals::deathsound_dir + "\\" + Globals::deathsound_files[Globals::deathsound_selected];
			PlayDeathSoundFile(fullPath);
		}

		s_ds_enemyAlive[pawnAddr] = aliveNow;
	}

	
	for (auto it = s_ds_enemyAlive.begin(); it != s_ds_enemyAlive.end();)
	{
		if (seen.find(it->first) == seen.end())
			it = s_ds_enemyAlive.erase(it);
		else
			++it;
	}
}







void Misc::ThirdPerson()
{
	static bool wasEnabled = false;

	if (!Globals::thirdperson_enabled) {
		
		if (wasEnabled) {
			__try {
				uintptr_t client = Memory::GetModuleBase("client.dll");
				if (client) {
					
					uintptr_t inputAddr = client + Offsets::dwCSGOInput;
					if (Utils::IsValidPtr(inputAddr)) {
						Utils::SafeWrite<bool>(inputAddr + 0x229, false);
					}
				}
				wasEnabled = false;
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {}
		}
		return;
	}

	
	static bool lastState = false;
	static uintptr_t tpResetAddr = 0;

	__try {
		uintptr_t client = Memory::GetModuleBase("client.dll");
		if (!client) return;

		
		if (Globals::thirdperson_enabled != lastState) {
			if (!tpResetAddr) {
				
				tpResetAddr = Memory::PatternScan("client.dll", "48 8B 40 ? ? ? ? 75 ? BA ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 85 C0 75 ? 48 8B 05 ? ? ? ? 48 8B 40 ? ? ? ? 75 ? 44 88 7F");
			}

			if (tpResetAddr) {
				DWORD oldProtect;
				
				if (VirtualProtect((void*)(tpResetAddr + 7), 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
					*(uint8_t*)(tpResetAddr + 7) = Globals::thirdperson_enabled ? 0xEB : 0x75;
					VirtualProtect((void*)(tpResetAddr + 7), 1, oldProtect, &oldProtect);
				}
			}
			lastState = Globals::thirdperson_enabled;
		}

		
		uintptr_t inputAddr = client + Offsets::dwCSGOInput;
		if (Utils::IsValidPtr(inputAddr)) {
			Utils::SafeWrite<bool>(inputAddr + 0x229, true);
		}

		wasEnabled = true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}
}





void Misc::AntiFlash()
{
	if (!Globals::antiflash_enabled) return;

	__try {
		uintptr_t client = Memory::GetModuleBase("client.dll");
		if (!client) return;

		uintptr_t localPawnAddr = Utils::SafeRead<uintptr_t>(client + Offsets::dwLocalPlayerPawn);
		if (!Utils::IsValidPtr(localPawnAddr) || localPawnAddr < 0x1000000) return;

		int health = Utils::SafeRead<int>(localPawnAddr + Offsets::m_iHealth);
		if (health <= 0) return;

		
		*(float*)(localPawnAddr + Offsets::m_flFlashDuration) = 0.0f;
		*(float*)(localPawnAddr + Offsets::m_flFlashMaxAlpha) = 0.0f;
		*(float*)(localPawnAddr + Offsets::m_flFlashScreenshotAlpha) = 0.0f;
		*(float*)(localPawnAddr + Offsets::m_flFlashOverlayAlpha) = 0.0f;
		*(bool*)(localPawnAddr + Offsets::m_bFlashBuildUp) = false;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}
}




void Misc::SmokeColorChanger()
{
	if (!Globals::smoke_color_enabled) return;

	__try {
		uintptr_t client = Memory::GetModuleBase("client.dll");
		if (!client) return;

		uintptr_t entityList = Utils::SafeRead<uintptr_t>(client + Offsets::dwEntityList);
		if (!Utils::IsValidPtr(entityList)) return;

		
		for (int i = 0; i < 1024; i++)
		{
			
			uintptr_t entry = Utils::SafeRead<uintptr_t>(entityList + (8 * ((i & 0x7FFF) >> 9) + 16));
			if (!Utils::IsValidPtr(entry)) continue;

			uintptr_t entity = Utils::SafeRead<uintptr_t>(entry + (112 * (i & 0x1FF)));
			if (!Utils::IsValidPtr(entity) || entity < 0x1000000) continue;

			
			uintptr_t entityIdentity = Utils::SafeRead<uintptr_t>(entity + 0x10);
			if (!Utils::IsValidPtr(entityIdentity) || entityIdentity < 0x1000000) continue;

			
			uintptr_t designerNamePtr = Utils::SafeRead<uintptr_t>(entityIdentity + 0x20);
			if (!Utils::IsValidPtr(designerNamePtr)) continue;

			
			char className[64] = {};
			if (!Utils::SafeReadString(designerNamePtr, className, sizeof(className))) continue;

			
			if (strcmp(className, "smokegrenade_projectile") != 0) continue;

			
			Vector smokeColor = {
				Globals::smoke_color[0] * 255.f,
				Globals::smoke_color[1] * 255.f,
				Globals::smoke_color[2] * 255.f
			};

			Utils::SafeWrite<Vector>(entity + Offsets::m_vSmokeColor, smokeColor);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}
}




static bool ReadHitsoundData(int& outTotalHits)
{
	__try {
		uintptr_t client = Memory::GetModuleBase("client.dll");
		if (!client) return false;

		uintptr_t localPawnAddr = Utils::SafeRead<uintptr_t>(client + Offsets::dwLocalPlayerPawn);
		if (!Utils::IsValidPtr(localPawnAddr) || localPawnAddr < 0x1000000) return false;

		int health = Utils::SafeRead<int>(localPawnAddr + Offsets::m_iHealth);
		if (health <= 0) return false;

		uintptr_t pBulletServices = Utils::SafeRead<uintptr_t>(localPawnAddr + Offsets::m_pBulletServices);
		if (!Utils::IsValidPtr(pBulletServices) || pBulletServices < 0x1000000) return false;

		outTotalHits = Utils::SafeRead<int>(pBulletServices + Offsets::m_totalHitsOnServer);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

void Misc::HitManagerUpdate()
{
	if (!Globals::hitsound_enabled && !Globals::hitmarker_enabled && !Globals::killparticle_enabled) return;

	int totalHits = 0;
	if (!ReadHitsoundData(totalHits)) return;

	if (totalHits != s_previousTotalHits)
	{
		if (totalHits == 0 && s_previousTotalHits != 0)
		{
			
		}
		else
		{
			if (totalHits > s_previousTotalHits)
				s_lastLocalHitTime = Misc_GetTime();

			if (Globals::hitsound_enabled && Globals::hitsound_selected >= 0)
				PlayHitsound();

			if (Globals::hitmarker_enabled)
			{
				s_hitmarkerAlpha = 255.f;
				s_hitmarkerStartTime = std::chrono::steady_clock::now();
			}
		}
	}

	s_previousTotalHits = totalHits;
}

void Misc::HitmarkerRender()
{
	if (!Globals::hitmarker_enabled) return;

	if (s_hitmarkerAlpha > 0.f)
	{
		auto now = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_hitmarkerStartTime).count();
		
		if (duration >= 500)
		{
			s_hitmarkerAlpha = 0.f;
		}
		else
		{
			s_hitmarkerAlpha = 255.f * (1.f - (float)duration / 500.f);
		}

		if (s_hitmarkerAlpha > 0.f)
		{
			ImDrawList* dl = ImGui::GetBackgroundDrawList();
			ImVec2 center = ImVec2(Globals::GameViewportX + Globals::ScreenWidth / 2.f, Globals::GameViewportY + Globals::ScreenHeight / 2.f);
			
			const float SIZE = 12.f; 
			const float GAP = 4.f;   
			
			
			ImU32 colBg = IM_COL32(0, 0, 0, (int)s_hitmarkerAlpha);
			
			
			ImU32 colFg = ImGui::ColorConvertFloat4ToU32(ImVec4(
				Globals::hitmarker_color[0],
				Globals::hitmarker_color[1],
				Globals::hitmarker_color[2],
				(s_hitmarkerAlpha / 255.f) * Globals::hitmarker_color[3]
			));

			
			dl->AddLine(ImVec2(center.x - SIZE, center.y - SIZE), ImVec2(center.x - GAP, center.y - GAP), colBg, 3.0f);
			dl->AddLine(ImVec2(center.x - SIZE, center.y + SIZE), ImVec2(center.x - GAP, center.y + GAP), colBg, 3.0f);
			dl->AddLine(ImVec2(center.x + SIZE, center.y - SIZE), ImVec2(center.x + GAP, center.y - GAP), colBg, 3.0f);
			dl->AddLine(ImVec2(center.x + SIZE, center.y + SIZE), ImVec2(center.x + GAP, center.y + GAP), colBg, 3.0f);
			
			
			dl->AddLine(ImVec2(center.x - SIZE, center.y - SIZE), ImVec2(center.x - GAP, center.y - GAP), colFg, 1.5f);
			dl->AddLine(ImVec2(center.x - SIZE, center.y + SIZE), ImVec2(center.x - GAP, center.y + GAP), colFg, 1.5f);
			dl->AddLine(ImVec2(center.x + SIZE, center.y - SIZE), ImVec2(center.x + GAP, center.y - GAP), colFg, 1.5f);
			dl->AddLine(ImVec2(center.x + SIZE, center.y + SIZE), ImVec2(center.x + GAP, center.y + GAP), colFg, 1.5f);
		}
	}
}





struct SpectatorData {
	std::vector<std::string> names;
};

static SpectatorData s_specData;


static uintptr_t ResolveHandle(uintptr_t entityListAddr, uint32_t handle)
{
	__try {
		if (!handle || handle == 0xFFFFFFFF || !entityListAddr) return 0;

		uintptr_t listPtr = Utils::SafeRead<uintptr_t>(entityListAddr);
		if (!Utils::IsValidPtr(listPtr)) return 0;

		uintptr_t entry = Utils::SafeRead<uintptr_t>(listPtr + (8 * ((handle & 0x7FFF) >> 9) + 16));
		if (!Utils::IsValidPtr(entry)) return 0;

		uintptr_t entPtr = Utils::SafeRead<uintptr_t>(entry + (112 * (handle & 0x1FF)));
		if (!Utils::IsValidPtr(entPtr)) return 0;

		return entPtr;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return 0;
	}
}


static uintptr_t GetObserverTarget(uintptr_t pawnAddr)
{
	__try {
		if (!pawnAddr || !Utils::IsValidPtr(pawnAddr)) return 0;

		uintptr_t observerServices = Utils::SafeRead<uintptr_t>(pawnAddr + Offsets::m_pObserverServices);
		if (!Utils::IsValidPtr(observerServices) || observerServices < 0x1000000) return 0;

		uint32_t targetHandle = Utils::SafeRead<uint32_t>(observerServices + Offsets::m_hObserverTarget);
		if (!targetHandle || targetHandle == 0xFFFFFFFF) return 0;

		uintptr_t client = Memory::GetModuleBase("client.dll");
		if (!client) return 0;
		uintptr_t entityListAddr = client + Offsets::dwEntityList;

		return ResolveHandle(entityListAddr, targetHandle);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return 0;
	}
}


static bool ReadPlayerName(uintptr_t controllerAddr, char* outBuf, size_t maxLen)
{
	__try {
		if (!controllerAddr || !Utils::IsValidPtr(controllerAddr)) return false;
		
		
		uintptr_t nameBufferPtr = Utils::SafeRead<uintptr_t>(controllerAddr + Offsets::m_iszPlayerName);
		if (!nameBufferPtr || !Utils::IsValidPtr(nameBufferPtr)) return false;
		
		return Utils::SafeReadString(nameBufferPtr, outBuf, maxLen);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

void Misc::SpectatorListUpdate()
{
	s_specData.names.clear();

	if (!Globals::speclist_enabled) return;

	try {
		auto entities = EntityManager::Get().GetEntities();
		auto* localPawn = EntityManager::Get().GetLocalPawn();
		uintptr_t localPawnAddr = reinterpret_cast<uintptr_t>(localPawn);

		if (!localPawnAddr || !Utils::IsValidPtr(localPawnAddr)) return;

		
		uintptr_t spectatedPawn = 0;
		bool localAlive = Utils::SafeAlive(localPawn);
		if (!localAlive)
		{
			spectatedPawn = GetObserverTarget(localPawnAddr);
		}

		uintptr_t client = Memory::GetModuleBase("client.dll");
		if (!client) return;
		uintptr_t entityListAddr = client + Offsets::dwEntityList;

		for (const auto& ent : entities)
		{
			uintptr_t ctrlAddr = reinterpret_cast<uintptr_t>(ent.controller);
			uintptr_t pawnAddr = reinterpret_cast<uintptr_t>(ent.pawn);

			if (!ctrlAddr || !pawnAddr) continue;
			if (pawnAddr == localPawnAddr) continue; 

			
			if (Utils::SafeAlive(ent.pawn)) continue;

			
			uint32_t pawnHandle = Utils::SafeRead<uint32_t>(ctrlAddr + Offsets::m_hPawn);
			uintptr_t resolvedPawn = 0;
			if (pawnHandle && pawnHandle != 0xFFFFFFFF)
				resolvedPawn = ResolveHandle(entityListAddr, pawnHandle);
			if (!resolvedPawn)
				resolvedPawn = pawnAddr;

			
			uintptr_t specTarget = GetObserverTarget(resolvedPawn);
			if (!specTarget) continue;

			
			bool isSpectatingUs = (specTarget == localPawnAddr);
			bool isSpectatingOurTarget = (spectatedPawn != 0 && specTarget == spectatedPawn);

			if (isSpectatingUs || isSpectatingOurTarget)
			{
				char nameBuf[128] = {};
				if (ReadPlayerName(ctrlAddr, nameBuf, sizeof(nameBuf)) && nameBuf[0] != '\0')
				{
					s_specData.names.push_back(std::string(nameBuf));
				}
			}
		}
	}
	catch (...) {}
}

void Misc::SpectatorListRender()
{
	if (!Globals::speclist_enabled) return;

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

	float fontHeight = ImGui::GetFontSize();
	float requiredHeight = (float)s_specData.names.size() * (fontHeight + 5.0f) + 35.0f;
	if (requiredHeight < 50.0f) requiredHeight = 50.0f;

	ImGui::SetNextWindowSize(ImVec2(180.0f, requiredHeight), ImGuiCond_Always);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.078f, 0.078f, 0.078f, 0.90f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.298f, 0.298f, 0.298f, 0.4f));
	ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.055f, 0.055f, 0.055f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.055f, 0.055f, 0.055f, 1.0f));

	ImGui::Begin("Spectators", nullptr, flags);

	if (s_specData.names.empty())
	{
		ImGui::TextDisabled("No spectators");
	}
	else
	{
		for (const auto& name : s_specData.names)
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5.0f);
			ImGui::Text("%s", name.c_str());
		}
	}

	ImGui::End();
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar();
}






static bool s_bt_active = false;                
static bool s_bt_wasTicking = false;             
static std::chrono::steady_clock::time_point s_bt_plantTime; 
static int  s_bt_bombSite = -1;                  

static bool s_bt_defusing = false;               
static bool s_bt_wasDefusing = false;             
static bool s_bt_defuserHasKit = false;           
static std::chrono::steady_clock::time_point s_bt_defuseStartTime; 

static constexpr float BOMB_TIMER = 40.f;        
static constexpr float DEFUSE_TIME_KIT = 5.f;    
static constexpr float DEFUSE_TIME_NOKIT = 10.f;  


struct BombState {
	bool valid = false;
	bool ticking = false;
	bool defused = false;
	bool beingDefused = false;
	bool defuserHasKit = false;
	int bombSite = -1;
	uintptr_t bombAddress = 0;
	float c4BlowTime = 0.f;
};

static BombState ReadBombState()
{
	BombState state = {};

	uintptr_t client = Memory::GetModuleBase("client.dll");
	if (!client) return state;

	
	uintptr_t pBombSystem = Utils::SafeRead<uintptr_t>(client + Offsets::dwPlantedC4);
	if (!Utils::IsValidPtr(pBombSystem)) return state;

	uintptr_t bomb = Utils::SafeRead<uintptr_t>(pBombSystem);
	if (!Utils::IsValidPtr(bomb) || bomb < 0x1000000) return state;

	state.bombAddress = bomb;

	
	bool ticking = Utils::SafeRead<bool>(bomb + Offsets::m_bBombTicking);
	bool defused = Utils::SafeRead<bool>(bomb + Offsets::m_bBombDefused);
	
	state.valid = true;
	state.ticking = ticking;
	state.defused = defused;
	
	
	state.c4BlowTime = Utils::SafeRead<float>(bomb + Offsets::m_flC4Blow);

	if (!state.ticking) return state;

	
	state.bombSite = Utils::SafeRead<int>(bomb + Offsets::m_nBombSite);

	
	bool beingDefused = Utils::SafeRead<bool>(bomb + Offsets::m_bBeingDefused);
	state.beingDefused = beingDefused;

	if (beingDefused) {
		
		uint32_t defuserHandle = Utils::SafeRead<uint32_t>(bomb + Offsets::m_hBombDefuser);
		if (defuserHandle && defuserHandle != 0xFFFFFFFF) {
			
			C_BaseEntity* defuserEnt = EntityManager::Get().GetEntityFromHandle(defuserHandle);
			uintptr_t defuserAddr = reinterpret_cast<uintptr_t>(defuserEnt);
			if (Utils::IsValidPtr(defuserAddr) && defuserAddr > 0x1000000) {
				
				uintptr_t itemServices = Utils::SafeRead<uintptr_t>(defuserAddr + Offsets::m_pItemServices);
				if (Utils::IsValidPtr(itemServices)) {
					
					state.defuserHasKit = Utils::SafeRead<bool>(itemServices + Offsets::m_bHasDefuser);
				}
			}
		}
	}

	return state;
}

static uintptr_t s_bt_lastBombAddress = 0;
static float s_bt_lastC4BlowTime = 0.f;

void Misc::BombTimerUpdate()
{
	if (!Globals::bombtimer_enabled) {
		s_bt_active = false;
		s_bt_wasDefusing = false;
		return;
	}

	BombState state = ReadBombState();

	
	if (!state.valid || !state.ticking || state.defused) {
		s_bt_active = false;
		s_bt_defusing = false;
		s_bt_wasDefusing = false;
		return;
	}

	auto now = std::chrono::steady_clock::now();

	
	if (state.bombAddress != s_bt_lastBombAddress || state.c4BlowTime != s_bt_lastC4BlowTime) {
		s_bt_lastBombAddress = state.bombAddress;
		s_bt_lastC4BlowTime = state.c4BlowTime;
		
		s_bt_plantTime = now;
		s_bt_bombSite = state.bombSite;
		s_bt_active = true;
		s_bt_defusing = false;
		s_bt_wasDefusing = false;
	}

	
	
	if (!s_bt_active) return;

	
	float elapsed = std::chrono::duration<float>(now - s_bt_plantTime).count();
	if (elapsed >= BOMB_TIMER) {
		s_bt_active = false;
		s_bt_wasTicking = false;
		return;
	}

	
	if (state.bombSite >= 0)
		s_bt_bombSite = state.bombSite;

	
	if (state.beingDefused) {
		if (!s_bt_wasDefusing) {
			
			s_bt_defuseStartTime = now;
			s_bt_defuserHasKit = state.defuserHasKit;
		}
		s_bt_defusing = true;
		s_bt_wasDefusing = true;
	}
	else {
		
		s_bt_defusing = false;
		s_bt_wasDefusing = false;
	}
}

void Misc::BombTimerRender()
{
	if (!Globals::bombtimer_enabled || !s_bt_active)
		return;

	ImDrawList* dl = ImGui::GetBackgroundDrawList();
	if (!dl) return;

	auto now = std::chrono::steady_clock::now();
	float elapsed = std::chrono::duration<float>(now - s_bt_plantTime).count();
	float remaining = BOMB_TIMER - elapsed;

	if (remaining <= 0.f) {
		s_bt_active = false;
		return;
	}

	
	float vpX = Globals::GameViewportX;
	float vpW = Globals::GameViewportWidth;

	
	const float panelW = 280.f;
	const float panelH = 70.f;
	const float panelX = vpX + (vpW - panelW) * 0.5f;
	const float panelY = Globals::GameViewportY + 12.f;
	const float rounding = 8.f;
	const float barH = 6.f;
	const float padding = 12.f;

	float frac = std::clamp(remaining / BOMB_TIMER, 0.f, 1.f);

	
	float r = 1.0f;
	float g = frac;
	float b = 0.0f;
	ImU32 timerColor = ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 1.0f));

	
	dl->AddRectFilled(ImVec2(panelX, panelY), ImVec2(panelX + panelW, panelY + panelH), IM_COL32(15, 15, 15, 220), rounding);
	dl->AddRect(ImVec2(panelX, panelY), ImVec2(panelX + panelW, panelY + panelH), IM_COL32(60, 60, 60, 120), rounding);

	
	dl->AddRectFilled(ImVec2(panelX + 1, panelY + 1), ImVec2(panelX + panelW - 1, panelY + 3), timerColor);

	
	const char* siteLabel = s_bt_bombSite == 0 ? "BOMB - A" : (s_bt_bombSite == 1 ? "BOMB - B" : "BOMB");
	dl->AddText(ImVec2(panelX + padding, panelY + 10), IM_COL32(200, 200, 200, 255), siteLabel);

	
	char timerText[32];
	snprintf(timerText, sizeof(timerText), "%.1fs", remaining);
	ImVec2 timerSize = ImGui::CalcTextSize(timerText);
	dl->AddText(ImVec2(panelX + panelW - padding - timerSize.x, panelY + 10), timerColor, timerText);

	
	float barY = panelY + 34.f;
	float barX = panelX + padding;
	float barW = panelW - padding * 2;
	dl->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + barW, barY + barH), IM_COL32(40, 40, 40, 200), 3.f);
	dl->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + barW * frac, barY + barH), timerColor, 3.f);

	
	if (s_bt_defusing)
	{
		float defuseTime = s_bt_defuserHasKit ? DEFUSE_TIME_KIT : DEFUSE_TIME_NOKIT;
		float defuseElapsed = std::chrono::duration<float>(now - s_bt_defuseStartTime).count();
		float defuseRemaining = defuseTime - defuseElapsed;
		if (defuseRemaining < 0.f) defuseRemaining = 0.f;

		bool canDefuse = defuseRemaining < remaining;

		char defuseText[64];
		snprintf(defuseText, sizeof(defuseText), "DEFUSING [%s]: %.1fs",
			s_bt_defuserHasKit ? "KIT" : "NO KIT", defuseRemaining);

		ImU32 defuseColor = canDefuse ? IM_COL32(0, 200, 255, 255) : IM_COL32(255, 80, 80, 255);

		ImVec2 defuseSize = ImGui::CalcTextSize(defuseText);
		float defuseX = panelX + (panelW - defuseSize.x) * 0.5f;
		dl->AddText(ImVec2(defuseX, panelY + 48), defuseColor, defuseText);
	}
	else
	{
		
		if (remaining < 10.f)
		{
			const char* warnText = "! EXPLODING SOON !";
			ImVec2 warnSize = ImGui::CalcTextSize(warnText);
			float warnX = panelX + (panelW - warnSize.x) * 0.5f;

			float blink = (float)(fmod(ImGui::GetTime() * 4.0, 2.0) < 1.0 ? 1.0 : 0.4);
			dl->AddText(ImVec2(warnX, panelY + 48), IM_COL32(255, 50, 50, (int)(255 * blink)), warnText);
		}
	}
}





struct BulletTrace {
	float startPos[3]; 
	float endPos[3];   
	float spawnTime;   
	float totalDist;   
};

static std::vector<BulletTrace> s_bt_traces;
static std::mutex s_bt_traceMutex;
static int s_bt_lastShotsFired = 0;

static constexpr float BT_RAY_LENGTH = 8000.0f;
static constexpr float BT_BULLET_SPEED = 8000.0f;

static float BT_GetTime()
{
	return Misc_GetTime();
}


static bool BT_ReadAndDetectShot()
{
	uintptr_t client = Memory::GetModuleBase("client.dll");
	if (!client) return false;

	uintptr_t localPawnAddr = Utils::SafeRead<uintptr_t>(client + Offsets::dwLocalPlayerPawn);
	if (!Utils::IsValidPtr(localPawnAddr) || localPawnAddr < 0x1000000) return false;

	
	int currentShots = Utils::SafeRead<int>(localPawnAddr + Offsets::m_iShotsFired);

	if (currentShots > s_bt_lastShotsFired && s_bt_lastShotsFired >= 0)
	{
		s_bt_lastShotsFired = currentShots;

		
		uintptr_t sceneNode = Utils::SafeRead<uintptr_t>(localPawnAddr + Offsets::m_pGameSceneNode);
		if (!Utils::IsValidPtr(sceneNode)) return false;

		Vector origin = Utils::SafeRead<Vector>(sceneNode + Offsets::m_vecAbsOrigin);
		Vector viewOffset = Utils::SafeRead<Vector>(localPawnAddr + Offsets::m_vecViewOffset);
		float eyeX = origin.x + viewOffset.x;
		float eyeY = origin.y + viewOffset.y;
		float eyeZ = origin.z + viewOffset.z;

		if (eyeX == 0.f && eyeY == 0.f && eyeZ == 0.f) return false;

		
		float pitch = 0.f, yaw = 0.f;
		uintptr_t viewAnglesAddr = client + Offsets::dwViewAngles;
		pitch = Utils::SafeRead<float>(viewAnglesAddr);
		yaw = Utils::SafeRead<float>(viewAnglesAddr + 4);

		
		constexpr float PI_F = 3.14159265358979323846f;
		constexpr float D2R = PI_F / 180.0f;
		float cp = cosf(pitch * D2R);
		float sp = sinf(pitch * D2R);
		float cy = cosf(yaw * D2R);
		float sy = sinf(yaw * D2R);
		float dirX = cp * cy;
		float dirY = cp * sy;
		float dirZ = -sp;

		
		BulletTrace t;
		t.startPos[0] = eyeX;
		t.startPos[1] = eyeY;
		t.startPos[2] = eyeZ;
		t.endPos[0] = eyeX + dirX * BT_RAY_LENGTH;
		t.endPos[1] = eyeY + dirY * BT_RAY_LENGTH;
		t.endPos[2] = eyeZ + dirZ * BT_RAY_LENGTH;
		t.spawnTime = BT_GetTime();

		float dx = t.endPos[0] - eyeX;
		float dy = t.endPos[1] - eyeY;
		float dz = t.endPos[2] - eyeZ;
		t.totalDist = sqrtf(dx * dx + dy * dy + dz * dz);

		std::lock_guard<std::mutex> lock(s_bt_traceMutex);
		s_bt_traces.push_back(t);
		return true;
	}

	s_bt_lastShotsFired = currentShots;
	return false;
}

void Misc::BulletTracerUpdate()
{
	if (!Globals::bullettracer_enabled) {
		std::lock_guard<std::mutex> lock(s_bt_traceMutex);
		s_bt_traces.clear();
		s_bt_lastShotsFired = 0;
		return;
	}

	BT_ReadAndDetectShot();
}

void Misc::BulletTracerRender()
{
	if (!Globals::bullettracer_enabled) return;

	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	if (!draw) return;

	float now = BT_GetTime();
	float trailLife = Globals::bullettracer_traillife;
	float thickness = Globals::bullettracer_thickness;

	
	int colR = (int)(Globals::bullettracer_color[0] * 255.f);
	int colG = (int)(Globals::bullettracer_color[1] * 255.f);
	int colB = (int)(Globals::bullettracer_color[2] * 255.f);

	std::lock_guard<std::mutex> lock(s_bt_traceMutex);

	
	float maxAge = trailLife + 2.0f;
	s_bt_traces.erase(
		std::remove_if(s_bt_traces.begin(), s_bt_traces.end(),
			[now, maxAge](const BulletTrace& t) { return (now - t.spawnTime) > maxAge; }),
		s_bt_traces.end());

	float screenW = Globals::GameViewportWidth;
	float screenH = Globals::GameViewportHeight;

	for (const auto& t : s_bt_traces)
	{
		float age = now - t.spawnTime;

		
		float travelTime = t.totalDist / BT_BULLET_SPEED;
		if (travelTime < 0.01f) travelTime = 0.01f;
		float bulletFrac = age / travelTime;
		if (bulletFrac > 1.0f) bulletFrac = 1.0f;

		
		float trailAge = age - travelTime;
		float trailAlpha = 1.0f;
		if (trailAge > 0.0f)
		{
			trailAlpha = 1.0f - (trailAge / trailLife);
			if (trailAlpha <= 0.0f) continue;
			trailAlpha *= trailAlpha; 
		}

		
		constexpr int SEGMENTS = 16;
		ImVec2 pts[SEGMENTS + 1];
		bool   ok[SEGMENTS + 1] = {};

		for (int s = 0; s <= SEGMENTS; s++)
		{
			float segFrac = (float)s / (float)SEGMENTS * bulletFrac;
			
			float pos3d[3];
			pos3d[0] = t.startPos[0] + (t.endPos[0] - t.startPos[0]) * segFrac;
			pos3d[1] = t.startPos[1] + (t.endPos[1] - t.startPos[1]) * segFrac;
			pos3d[2] = t.startPos[2] + (t.endPos[2] - t.startPos[2]) * segFrac;

			Vector worldPos(pos3d[0], pos3d[1], pos3d[2]);
			Vector screenPos;
			ok[s] = Utils::WorldToScreen(worldPos, screenPos, Globals::ViewMatrix, screenW, screenH);
			if (ok[s]) {
				pts[s] = ImVec2(screenPos.x, screenPos.y);
			}
		}

		for (int s = 0; s < SEGMENTS; s++)
		{
			if (!ok[s] || !ok[s + 1]) continue;

			float segFrac = (float)s / (float)SEGMENTS;
			
			float brightness = 0.2f + 0.8f * segFrac;
			int alpha = (int)(trailAlpha * brightness * 220.0f);
			if (alpha <= 0) continue;
			if (alpha > 255) alpha = 255;

			
			ImU32 lineCol = IM_COL32(colR, colG, colB, alpha);
			draw->AddLine(pts[s], pts[s + 1], lineCol, thickness);

			
			int glowA = (int)(trailAlpha * brightness * 40.0f);
			if (glowA > 255) glowA = 255;
			ImU32 glowCol = IM_COL32(colR, colG, colB, glowA);
			draw->AddLine(pts[s], pts[s + 1], glowCol, thickness * 3.5f);
		}

		
		if (bulletFrac < 1.0f)
		{
			float headPos[3];
			headPos[0] = t.startPos[0] + (t.endPos[0] - t.startPos[0]) * bulletFrac;
			headPos[1] = t.startPos[1] + (t.endPos[1] - t.startPos[1]) * bulletFrac;
			headPos[2] = t.startPos[2] + (t.endPos[2] - t.startPos[2]) * bulletFrac;

			Vector headWorld(headPos[0], headPos[1], headPos[2]);
			Vector headScreen;
			if (Utils::WorldToScreen(headWorld, headScreen, Globals::ViewMatrix, screenW, screenH))
			{
				int ha = (int)(trailAlpha * 255.0f);
				draw->AddCircleFilled(ImVec2(headScreen.x, headScreen.y), 5.0f, IM_COL32(colR, colG, colB, ha));
				draw->AddCircleFilled(ImVec2(headScreen.x, headScreen.y), 2.5f, IM_COL32(255, 255, 255, ha));
			}
		}

		
		if (bulletFrac >= 1.0f && trailAlpha > 0.05f)
		{
			Vector endWorld(t.endPos[0], t.endPos[1], t.endPos[2]);
			Vector endScreen;
			if (Utils::WorldToScreen(endWorld, endScreen, Globals::ViewMatrix, screenW, screenH))
			{
				float sz = 4.0f * trailAlpha;
				draw->AddCircleFilled(ImVec2(endScreen.x, endScreen.y), sz,
					IM_COL32(colR, colG, colB, (int)(trailAlpha * 200.0f)));
				draw->AddCircle(ImVec2(endScreen.x, endScreen.y), sz * 1.8f,
					IM_COL32(255, 255, 255, (int)(trailAlpha * 120.0f)), 0, 1.5f);
			}
		}
	}
}

struct KillParticlePoint {
	Vector velocity;
	float startSize;
	float life;
};

struct KillParticleBurst {
	Vector origin;
	float spawnTime;
	int type;
	std::vector<KillParticlePoint> points;
};

static std::unordered_map<uintptr_t, bool> s_kp_enemyAlive;
static std::vector<KillParticleBurst> s_kp_bursts;
static std::mutex s_kp_mutex;

static void KP_SpawnBurst(const Vector& origin, int type)
{
	KillParticleBurst burst{};
	burst.origin = origin;
	burst.spawnTime = Misc_GetTime();
	burst.type = (type < 0 || type > 3) ? 0 : type;
	burst.points.reserve(36);

	constexpr float pi = 3.14159265358979323846f;
	float base = fmodf(origin.x * 0.0137f + origin.y * 0.0091f + origin.z * 0.0053f, 6.2831853f);
	int count = 26;
	if (burst.type == 1) count = 32;
	else if (burst.type == 2) count = 36;
	else if (burst.type == 3) count = 20;

	for (int i = 0; i < count; ++i)
	{
		float a = base + (2.0f * pi * (float)i / (float)count);
		KillParticlePoint p{};
		if (burst.type == 1)
		{
			float speed = 160.0f + 24.0f * (float)(i % 5);
			p.velocity.x = cosf(a) * speed;
			p.velocity.y = sinf(a) * speed;
			p.velocity.z = 30.0f + 16.0f * (float)((i + 2) % 4);
			p.startSize = 2.0f + (float)(i % 3);
			p.life = 0.42f + 0.18f * ((float)(i % 6) / 5.0f);
		}
		else if (burst.type == 2)
		{
			float spiral = 40.0f + 8.0f * (float)(i % 7);
			p.velocity.x = cosf(a) * spiral;
			p.velocity.y = sinf(a) * spiral;
			p.velocity.z = 120.0f + 6.0f * (float)(i % 8);
			p.startSize = 1.7f + (float)(i % 4) * 0.55f;
			p.life = 0.85f + 0.35f * ((float)(i % 5) / 4.0f);
		}
		else if (burst.type == 3)
		{
			float offset = (float)i - (float)(count / 2);
			p.velocity.x = cosf(base + 1.57f) * offset * 18.0f;
			p.velocity.y = sinf(base + 1.57f) * offset * 18.0f;
			p.velocity.z = 180.0f - fabsf(offset) * 6.5f;
			p.startSize = 2.8f + (float)(i % 2) * 1.4f;
			p.life = 0.55f + 0.15f * ((float)(i % 4) / 3.0f);
		}
		else
		{
			float h = 0.35f + 0.65f * ((float)(i % 5) / 4.0f);
			float speed = 75.0f + 18.0f * (float)(i % 7);
			p.velocity.x = cosf(a) * speed;
			p.velocity.y = sinf(a) * speed;
			p.velocity.z = 45.0f + h * 120.0f;
			p.startSize = 2.0f + (float)(i % 4);
			p.life = 0.65f + 0.25f * ((float)(i % 6) / 5.0f);
		}
		burst.points.push_back(p);
	}

	std::lock_guard<std::mutex> lock(s_kp_mutex);
	s_kp_bursts.push_back(std::move(burst));
}

void Misc::KillParticleUpdate()
{
	if (!Globals::killparticle_enabled)
	{
		std::lock_guard<std::mutex> lock(s_kp_mutex);
		s_kp_enemyAlive.clear();
		s_kp_bursts.clear();
		return;
	}
	if (Globals::killparticle_type < 0 || Globals::killparticle_type > 3)
		Globals::killparticle_type = 0;

	auto entities = EntityManager::Get().GetEntities();
	std::unordered_set<uintptr_t> seen;
	seen.reserve(entities.size());

	float now = Misc_GetTime();
	for (const auto& ent : entities)
	{
		if (!ent.isEnemy || !ent.pawn)
			continue;

		uintptr_t pawnAddr = reinterpret_cast<uintptr_t>(ent.pawn);
		if (!Utils::IsValidPtr(pawnAddr))
			continue;

		seen.insert(pawnAddr);
		bool aliveNow = Utils::SafeAlive(ent.pawn);
		auto it = s_kp_enemyAlive.find(pawnAddr);
		bool wasAlive = (it == s_kp_enemyAlive.end()) ? aliveNow : it->second;

		if (wasAlive && !aliveNow && (now - s_lastLocalHitTime) <= 1.0f)
		{
			Vector deathPos = ent.pawn->m_vOldOrigin();
			uintptr_t sceneNode = Utils::SafeRead<uintptr_t>(pawnAddr + Offsets::m_pGameSceneNode);
			if (Utils::IsValidPtr(sceneNode))
			{
				Vector absOrigin = Utils::SafeRead<Vector>(sceneNode + Offsets::m_vecAbsOrigin);
				if (absOrigin.x != 0.0f || absOrigin.y != 0.0f || absOrigin.z != 0.0f)
					deathPos = absOrigin;
			}
			KP_SpawnBurst(deathPos, Globals::killparticle_type);
		}

		s_kp_enemyAlive[pawnAddr] = aliveNow;
	}

	for (auto it = s_kp_enemyAlive.begin(); it != s_kp_enemyAlive.end();)
	{
		if (seen.find(it->first) == seen.end())
			it = s_kp_enemyAlive.erase(it);
		else
			++it;
	}

	{
		std::lock_guard<std::mutex> lock(s_kp_mutex);
		s_kp_bursts.erase(
			std::remove_if(s_kp_bursts.begin(), s_kp_bursts.end(),
				[now](const KillParticleBurst& b) { return (now - b.spawnTime) > 1.25f; }),
			s_kp_bursts.end());
	}
}

void Misc::KillParticleRender()
{
	if (!Globals::killparticle_enabled)
		return;

	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	if (!draw)
		return;

	float now = Misc_GetTime();
	float screenW = Globals::GameViewportWidth;
	float screenH = Globals::GameViewportHeight;
	int baseR = (int)(Globals::killparticle_color[0] * 255.0f);
	int baseG = (int)(Globals::killparticle_color[1] * 255.0f);
	int baseB = (int)(Globals::killparticle_color[2] * 255.0f);
	float baseA = Globals::killparticle_color[3];

	std::lock_guard<std::mutex> lock(s_kp_mutex);
	for (const auto& burst : s_kp_bursts)
	{
		float burstAge = now - burst.spawnTime;
		if (burstAge < 0.0f)
			continue;

		for (const auto& point : burst.points)
		{
			float t = burstAge / point.life;
			if (t <= 0.0f || t >= 1.0f)
				continue;

			Vector world = burst.origin;
			float vx = point.velocity.x;
			float vy = point.velocity.y;
			float vz = point.velocity.z;
			if (burst.type == 2)
			{
				float swirl = burstAge * 9.0f + (point.startSize * 0.7f);
				vx += cosf(swirl) * 28.0f;
				vy += sinf(swirl) * 28.0f;
			}
			world.x += vx * burstAge;
			world.y += vy * burstAge;
			world.z += vz * burstAge - ((burst.type == 1) ? 90.0f : 140.0f) * burstAge * burstAge;

			Vector screen;
			if (!Utils::WorldToScreen(world, screen, Globals::ViewMatrix, screenW, screenH))
				continue;

			float fade = 1.0f - t;
			float alpha = fade * fade * baseA;
			float sizeMul = 0.7f + fade * 0.8f;
			if (burst.type == 1) sizeMul = 0.45f + fade * 0.35f;
			else if (burst.type == 2) sizeMul = 0.6f + fade * 1.0f;
			else if (burst.type == 3) sizeMul = 0.9f + fade * 1.15f;
			float size = point.startSize * sizeMul;
			int a = (int)(alpha * 255.0f);
			if (a <= 0)
				continue;

			draw->AddCircleFilled(ImVec2(screen.x, screen.y), size, IM_COL32(baseR, baseG, baseB, a));
			if (burst.type == 1)
			{
				draw->AddCircleFilled(ImVec2(screen.x, screen.y), size * 0.35f, IM_COL32(255, 220, 120, a));
			}
			else if (burst.type == 2)
			{
				draw->AddCircle(ImVec2(screen.x, screen.y), size * 1.2f, IM_COL32(baseR, baseG, baseB, a / 2), 0, 1.2f);
				draw->AddCircleFilled(ImVec2(screen.x, screen.y), size * 0.3f, IM_COL32(255, 255, 255, a));
			}
			else if (burst.type == 3)
			{
				draw->AddCircleFilled(ImVec2(screen.x, screen.y), size * 0.55f, IM_COL32(255, 255, 255, a));
				draw->AddCircle(ImVec2(screen.x, screen.y), size * 1.6f, IM_COL32(baseR, baseG, baseB, a / 2), 0, 1.5f);
			}
			else
			{
				draw->AddCircleFilled(ImVec2(screen.x, screen.y), size * 0.4f, IM_COL32(255, 255, 255, a));
			}
		}
	}
}





struct HitParticle {
	Vector pos;
	Vector velocity;
	float baseSize;
	float life;       
	float age;        
	float drag;       
	float gravity;    
	float rotation;   
	float rotSpeed;
	int   layer;      
};

struct HitParticleBurst {
	Vector origin;
	float  spawnTime;
	int    style;
	float  lifetime;   
	float  sizeMul;    
	float  intensity;  
	float  glowMul;    
	float  colorR, colorG, colorB, colorA;
	std::vector<HitParticle> particles;
};


struct HitDistortion {
	Vector origin;
	float  spawnTime;
	float  maxRadius;
	float  life;
};

static std::vector<HitParticleBurst> s_hp_bursts;
static std::vector<HitDistortion>    s_hp_distortions;
static std::mutex                    s_hp_mutex;
static int                           s_hp_lastTotalHits = 0;
static bool                          s_hp_initialized = false;


static float HP_PseudoRand(float seed, int idx)
{
	uint32_t h = (uint32_t)(seed * 1000.0f) ^ (uint32_t)(idx * 2654435761u);
	h ^= h >> 16; h *= 0x45d9f3b; h ^= h >> 16;
	return (float)(h & 0xFFFF) / 65535.0f;
}

static void HP_GetPresetColor(int style, int idx, float rand01,
	float& outR, float& outG, float& outB, float& outA)
{
	switch (style)
	{
	case 0: 
	{
		float r = 0.72f + rand01 * 0.28f;
		float g = 0.02f + rand01 * 0.08f;
		float b = 0.01f + rand01 * 0.04f;
		outR = r; outG = g; outB = b; outA = 0.92f;
		break;
	}
	case 1: 
	{
		float t = (float)(idx % 3) / 2.0f;
		outR = 0.55f + t * 0.35f;
		outG = 0.02f + t * 0.06f + rand01 * 0.04f;
		outB = 0.01f + rand01 * 0.02f;
		outA = 0.95f;
		
		if ((idx % 7) == 0) { outR = 0.95f; outG = 0.15f; outB = 0.05f; }
		break;
	}
	case 2: 
	{
		float pulse = sinf(rand01 * 6.283f) * 0.5f + 0.5f;
		outR = 0.3f + pulse * 0.4f;
		outG = 0.7f + pulse * 0.3f;
		outB = 1.0f;
		outA = 0.88f;
		if ((idx % 4) == 0) { outR = 1.0f; outG = 1.0f; outB = 1.0f; outA = 0.95f; }
		break;
	}
	case 3: 
	{
		outR = 0.75f + rand01 * 0.25f;
		outG = 0.82f + rand01 * 0.18f;
		outB = 1.0f;
		outA = 0.55f + rand01 * 0.2f;
		break;
	}
	case 4: 
	{
		float t = rand01;
		outR = 1.0f;
		outG = 0.65f + t * 0.35f;
		outB = 0.15f + t * 0.25f;
		outA = 0.9f + t * 0.1f;
		if ((idx % 5) == 0) { outR = 1.0f; outG = 1.0f; outB = 0.9f; } 
		break;
	}
	default:
		outR = 1.0f; outG = 1.0f; outB = 1.0f; outA = 1.0f;
		break;
	}
}

static void HP_SpawnBurst(const Vector& hitPos, int style)
{
	float now = Misc_GetTime();
	int maxParts = Globals::hitparticle_max_per_hit;
	if (maxParts < 8)  maxParts = 8;
	if (maxParts > 64) maxParts = 64;

	HitParticleBurst burst{};
	burst.origin = hitPos;
	burst.spawnTime = now;
	burst.style = style;
	burst.lifetime = Globals::hitparticle_lifetime;
	burst.sizeMul = Globals::hitparticle_size;
	burst.intensity = Globals::hitparticle_intensity;
	burst.glowMul = Globals::hitparticle_glow;

	
	burst.colorR = Globals::hitparticle_color[0];
	burst.colorG = Globals::hitparticle_color[1];
	burst.colorB = Globals::hitparticle_color[2];
	burst.colorA = Globals::hitparticle_color[3];

	burst.particles.reserve(maxParts);

	constexpr float PI = 3.14159265358979323846f;
	float seed = hitPos.x * 0.017f + hitPos.y * 0.013f + hitPos.z * 0.009f + now * 7.3f;
	bool randomize = Globals::hitparticle_randomize;

	for (int i = 0; i < maxParts; ++i)
	{
		HitParticle p{};
		p.pos = hitPos;
		p.age = 0.0f;

		float r0 = HP_PseudoRand(seed, i * 3);
		float r1 = HP_PseudoRand(seed, i * 3 + 1);
		float r2 = HP_PseudoRand(seed, i * 3 + 2);

		float angle = (2.0f * PI * (float)i / (float)maxParts);
		if (randomize) angle += (r0 - 0.5f) * 0.8f;

		float elevation = (r1 - 0.5f) * PI * 0.7f;
		float cosEl = cosf(elevation);

		switch (style)
		{
		case 0: 
		{
			float speed = 60.0f + r0 * 110.0f;
			p.velocity.x = cosf(angle) * cosEl * speed;
			p.velocity.y = sinf(angle) * cosEl * speed;
			p.velocity.z = sinf(elevation) * speed * 0.6f + 40.0f + r2 * 30.0f;
			p.baseSize = 1.8f + r1 * 2.5f;
			p.life = 0.5f + r2 * 0.4f;
			p.drag = 0.92f + r0 * 0.04f;
			p.gravity = 180.0f + r1 * 60.0f;
			p.layer = i % 3;
			break;
		}
		case 1: 
		{
			float speed = 40.0f + r0 * 80.0f;
			p.velocity.x = cosf(angle) * cosEl * speed;
			p.velocity.y = sinf(angle) * cosEl * speed;
			p.velocity.z = 30.0f + r2 * 60.0f;
			p.baseSize = 2.2f + r1 * 3.5f;
			p.life = 0.6f + r2 * 0.5f;
			p.drag = 0.88f + r0 * 0.06f;
			p.gravity = 220.0f + r1 * 80.0f;
			p.layer = i % 4;
			
			if ((i % 5) == 0) {
				p.baseSize *= 1.8f;
				p.velocity.x *= 0.5f; p.velocity.y *= 0.5f;
				p.velocity.z += 40.0f;
				p.gravity *= 1.4f;
			}
			break;
		}
		case 2: 
		{
			float speed = 100.0f + r0 * 160.0f;
			p.velocity.x = cosf(angle) * cosEl * speed;
			p.velocity.y = sinf(angle) * cosEl * speed;
			p.velocity.z = sinf(elevation) * speed * 0.4f;
			p.baseSize = 1.4f + r1 * 2.0f;
			p.life = 0.35f + r2 * 0.3f;
			p.drag = 0.95f;
			p.gravity = -15.0f; 
			p.layer = i % 3;
			break;
		}
		case 3: 
		{
			float ringSpeed = 140.0f + r0 * 60.0f;
			p.velocity.x = cosf(angle) * ringSpeed;
			p.velocity.y = sinf(angle) * ringSpeed;
			p.velocity.z = (r1 - 0.5f) * 20.0f;
			p.baseSize = 2.0f + r1 * 1.5f;
			p.life = 0.3f + r2 * 0.25f;
			p.drag = 0.97f;
			p.gravity = 0.0f;
			p.layer = 0;
			break;
		}
		case 4: 
		{
			float speed = 150.0f + r0 * 250.0f;
			p.velocity.x = cosf(angle) * cosEl * speed;
			p.velocity.y = sinf(angle) * cosEl * speed;
			p.velocity.z = sinf(elevation) * speed * 0.5f + 20.0f;
			p.baseSize = 0.8f + r1 * 1.2f;
			p.life = 0.15f + r2 * 0.25f;
			p.drag = 0.98f;
			p.gravity = 60.0f;
			p.layer = i % 2;
			break;
		}
		}

		p.rotation = r0 * 6.283f;
		p.rotSpeed = (r1 - 0.5f) * 12.0f;

		burst.particles.push_back(p);
	}

	std::lock_guard<std::mutex> lock(s_hp_mutex);

	
	while (s_hp_bursts.size() >= 8)
		s_hp_bursts.erase(s_hp_bursts.begin());

	s_hp_bursts.push_back(std::move(burst));

	
	if (Globals::hitparticle_distortion)
	{
		HitDistortion d{};
		d.origin = hitPos;
		d.spawnTime = now;
		d.maxRadius = 60.0f + (style == 3 ? 40.0f : 0.0f);
		d.life = 0.35f;
		s_hp_distortions.push_back(d);
	}
}

void Misc::HitParticleUpdate()
{
	if (!Globals::hitparticle_enabled)
	{
		std::lock_guard<std::mutex> lock(s_hp_mutex);
		s_hp_bursts.clear();
		s_hp_distortions.clear();
		s_hp_lastTotalHits = 0;
		s_hp_initialized = false;
		return;
	}

	
	int totalHits = 0;
	if (!ReadHitsoundData(totalHits)) return;

	if (!s_hp_initialized) {
		s_hp_lastTotalHits = totalHits;
		s_hp_initialized = true;
		return;
	}

	
	if (totalHits > s_hp_lastTotalHits && s_hp_lastTotalHits >= 0)
	{
		
		uintptr_t client = Memory::GetModuleBase("client.dll");
		if (client)
		{
			uintptr_t localPawnAddr = Utils::SafeRead<uintptr_t>(client + Offsets::dwLocalPlayerPawn);
			if (Utils::IsValidPtr(localPawnAddr) && localPawnAddr > 0x1000000)
			{
				
				uintptr_t localScene = Utils::SafeRead<uintptr_t>(localPawnAddr + Offsets::m_pGameSceneNode);
				Vector localOrigin = { 0, 0, 0 };
				if (Utils::IsValidPtr(localScene))
					localOrigin = Utils::SafeRead<Vector>(localScene + Offsets::m_vecAbsOrigin);
				Vector viewOff = Utils::SafeRead<Vector>(localPawnAddr + Offsets::m_vecViewOffset);
				Vector eyePos = { localOrigin.x + viewOff.x, localOrigin.y + viewOff.y, localOrigin.z + viewOff.z };

				
				uintptr_t viewAnglesAddr = client + Offsets::dwViewAngles;
				float pitch = Utils::SafeRead<float>(viewAnglesAddr);
				float yaw = Utils::SafeRead<float>(viewAnglesAddr + 4);
				constexpr float D2R = 3.14159265f / 180.0f;
				float cp = cosf(pitch * D2R), sp = sinf(pitch * D2R);
				float cy = cosf(yaw * D2R), sy = sinf(yaw * D2R);
				Vector fwd = { cp * cy, cp * sy, -sp };

				
				auto entities = EntityManager::Get().GetEntities();
				float bestDot = -999.0f;
				Vector bestHitPos = eyePos;
				bestHitPos.z += 40.0f; 

				for (const auto& ent : entities)
				{
					if (!ent.isEnemy || !ent.pawn) continue;
					uintptr_t pawnAddr = reinterpret_cast<uintptr_t>(ent.pawn);
					if (!Utils::IsValidPtr(pawnAddr)) continue;

					bool alive = Utils::SafeAlive(ent.pawn);
					if (!alive) continue;

					uintptr_t sceneNode = Utils::SafeRead<uintptr_t>(pawnAddr + Offsets::m_pGameSceneNode);
					if (!Utils::IsValidPtr(sceneNode)) continue;

					Vector entPos = Utils::SafeRead<Vector>(sceneNode + Offsets::m_vecAbsOrigin);
					entPos.z += 40.0f; 

					
					float dx = entPos.x - eyePos.x;
					float dy = entPos.y - eyePos.y;
					float dz = entPos.z - eyePos.z;
					float dist = sqrtf(dx * dx + dy * dy + dz * dz);
					if (dist < 1.0f) continue;

					float ndx = dx / dist, ndy = dy / dist, ndz = dz / dist;
					float dot = fwd.x * ndx + fwd.y * ndy + fwd.z * ndz;

					if (dot > bestDot)
					{
						bestDot = dot;
						bestHitPos = entPos;
					}
				}

				if (bestDot > 0.5f) 
				{
					HP_SpawnBurst(bestHitPos, Globals::hitparticle_style);
				}
			}
		}
	}
	else if (totalHits == 0 && s_hp_lastTotalHits != 0)
	{
		
	}

	s_hp_lastTotalHits = totalHits;

	
	float now = Misc_GetTime();
	{
		std::lock_guard<std::mutex> lock(s_hp_mutex);
		s_hp_bursts.erase(
			std::remove_if(s_hp_bursts.begin(), s_hp_bursts.end(),
				[now](const HitParticleBurst& b) { return (now - b.spawnTime) > (b.lifetime * 1.5f + 0.1f); }),
			s_hp_bursts.end());
		s_hp_distortions.erase(
			std::remove_if(s_hp_distortions.begin(), s_hp_distortions.end(),
				[now](const HitDistortion& d) { return (now - d.spawnTime) > d.life; }),
			s_hp_distortions.end());
	}
}

void Misc::HitParticleRender()
{
	if (!Globals::hitparticle_enabled) return;

	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	if (!draw) return;

	float now = Misc_GetTime();
	float screenW = Globals::GameViewportWidth;
	float screenH = Globals::GameViewportHeight;

	std::lock_guard<std::mutex> lock(s_hp_mutex);

	
	for (const auto& dist : s_hp_distortions)
	{
		float age = now - dist.spawnTime;
		if (age < 0.0f || age > dist.life) continue;

		float t = age / dist.life;
		float fade = 1.0f - t * t;

		Vector screen;
		if (!Utils::WorldToScreen(dist.origin, screen, Globals::ViewMatrix, screenW, screenH))
			continue;

		float radius = dist.maxRadius * t * 0.8f;

		
		Vector offsetWorld = dist.origin;
		offsetWorld.x += radius;
		Vector screenOff;
		if (!Utils::WorldToScreen(offsetWorld, screenOff, Globals::ViewMatrix, screenW, screenH))
			continue;

		float screenRadius = fabsf(screenOff.x - screen.x);
		if (screenRadius < 2.0f) continue;

		int alpha = (int)(fade * 45.0f);
		if (alpha <= 0) continue;

		
		draw->AddCircle(ImVec2(screen.x, screen.y), screenRadius,
			IM_COL32(200, 220, 255, alpha), 48, 2.5f);
		draw->AddCircle(ImVec2(screen.x, screen.y), screenRadius * 0.85f,
			IM_COL32(255, 255, 255, alpha / 2), 48, 1.2f);
	}

	
	for (auto& burst : s_hp_bursts)
	{
		float burstAge = now - burst.spawnTime;
		if (burstAge < 0.0f) continue;

		bool useOverride = Globals::hitparticle_color_override;

		for (auto& p : burst.particles)
		{
			p.age = burstAge;

			float lifeT = p.age / (p.life * burst.lifetime / 0.65f);
			if (lifeT <= 0.0f || lifeT >= 1.0f) continue;

			
			float dt = p.age;
			Vector world;
			world.x = p.pos.x + p.velocity.x * dt * powf(p.drag, dt * 60.0f);
			world.y = p.pos.y + p.velocity.y * dt * powf(p.drag, dt * 60.0f);
			world.z = p.pos.z + p.velocity.z * dt * powf(p.drag, dt * 60.0f)
				- 0.5f * p.gravity * dt * dt;

			Vector screen;
			if (!Utils::WorldToScreen(world, screen, Globals::ViewMatrix, screenW, screenH))
				continue;

			
			Vector camPos;
			
			uintptr_t client = Memory::GetModuleBase("client.dll");
			float worldDist = 500.0f; 
			if (client) {
				uintptr_t lpAddr = Utils::SafeRead<uintptr_t>(client + Offsets::dwLocalPlayerPawn);
				if (Utils::IsValidPtr(lpAddr) && lpAddr > 0x1000000) {
					uintptr_t sn = Utils::SafeRead<uintptr_t>(lpAddr + Offsets::m_pGameSceneNode);
					if (Utils::IsValidPtr(sn)) {
						Vector lp = Utils::SafeRead<Vector>(sn + Offsets::m_vecAbsOrigin);
						float dx2 = world.x - lp.x, dy2 = world.y - lp.y, dz2 = world.z - lp.z;
						worldDist = sqrtf(dx2 * dx2 + dy2 * dy2 + dz2 * dz2);
					}
				}
			}

			float distScale = 500.0f / (worldDist + 50.0f);
			if (distScale > 3.0f) distScale = 3.0f;
			if (distScale < 0.15f) continue; 

			
			float fadeIn = (lifeT < 0.05f) ? lifeT / 0.05f : 1.0f;
			float fadeOut = 1.0f - lifeT;
			fadeOut = fadeOut * fadeOut; 
			float alpha = fadeIn * fadeOut * burst.intensity;

			
			float sizeAnim = 1.0f;
			if (burst.style == 0 || burst.style == 1) {
				
				sizeAnim = (lifeT < 0.15f) ? (lifeT / 0.15f) : (0.6f + 0.4f * fadeOut);
			}
			else if (burst.style == 2) {
				
				sizeAnim = (lifeT < 0.1f) ? (lifeT / 0.1f) * 1.3f : fadeOut * 1.3f;
			}
			else if (burst.style == 3) {
				
				sizeAnim = 0.5f + lifeT * 2.0f;
			}
			else if (burst.style == 4) {
				
				sizeAnim = fadeOut;
			}

			float size = p.baseSize * burst.sizeMul * distScale * sizeAnim;
			if (size < 0.3f) continue;

			
			float cr, cg, cb, ca;
			if (useOverride) {
				cr = burst.colorR;
				cg = burst.colorG;
				cb = burst.colorB;
				ca = burst.colorA;
			}
			else {
				float rand01 = HP_PseudoRand(burst.spawnTime, (int)(p.rotation * 100.0f));
				HP_GetPresetColor(burst.style, p.layer, rand01, cr, cg, cb, ca);
			}

			int r = (int)(cr * 255.0f);
			int g = (int)(cg * 255.0f);
			int b = (int)(cb * 255.0f);
			int a = (int)(alpha * ca * 255.0f);
			if (a <= 0) continue;
			if (a > 255) a = 255;

			ImVec2 center(screen.x, screen.y);

			

			
			if (burst.glowMul > 0.01f)
			{
				float glowSize = size * (2.0f + burst.glowMul);
				int glowAlpha = (int)(a * 0.12f * burst.glowMul);
				if (glowAlpha > 0 && glowAlpha <= 255) {
					draw->AddCircleFilled(center, glowSize, IM_COL32(r, g, b, glowAlpha), 16);
				}
			}

			
			{
				float midSize = size * 1.4f;
				int midAlpha = (int)(a * 0.35f);
				if (midAlpha > 0 && midAlpha <= 255) {
					draw->AddCircleFilled(center, midSize, IM_COL32(r, g, b, midAlpha), 12);
				}
			}

			
			{
				draw->AddCircleFilled(center, size, IM_COL32(r, g, b, a), 12);
			}

			
			if (burst.style != 3) 
			{
				float coreSize = size * 0.35f;
				int coreAlpha = (int)(a * 0.6f * fadeOut);
				if (coreAlpha > 0 && coreAlpha <= 255 && coreSize > 0.3f) {
					
					int wR = r + (int)((255 - r) * 0.6f);
					int wG = g + (int)((255 - g) * 0.5f);
					int wB = b + (int)((255 - b) * 0.3f);
					draw->AddCircleFilled(center, coreSize, IM_COL32(wR, wG, wB, coreAlpha), 8);
				}
			}

			
			if (burst.style == 3 && size > 2.0f)
			{
				float ringAlpha = alpha * ca;
				int rA = (int)(ringAlpha * 180.0f);
				if (rA > 0 && rA <= 255) {
					draw->AddCircle(center, size * 1.2f, IM_COL32(r, g, b, rA), 32, 2.0f * distScale);
					draw->AddCircle(center, size * 0.8f, IM_COL32(r, g, b, rA / 2), 32, 1.2f * distScale);
				}
			}

			
			if (burst.style == 4 && lifeT < 0.6f && size > 1.0f)
			{
				float trailLen = size * 1.8f;
				float vLen = sqrtf(p.velocity.x * p.velocity.x + p.velocity.y * p.velocity.y + p.velocity.z * p.velocity.z);
				if (vLen > 10.0f)
				{
					Vector trailEnd;
					float trailDt = dt - 0.008f;
					if (trailDt < 0.0f) trailDt = 0.0f;
					trailEnd.x = p.pos.x + p.velocity.x * trailDt * powf(p.drag, trailDt * 60.0f);
					trailEnd.y = p.pos.y + p.velocity.y * trailDt * powf(p.drag, trailDt * 60.0f);
					trailEnd.z = p.pos.z + p.velocity.z * trailDt * powf(p.drag, trailDt * 60.0f)
						- 0.5f * p.gravity * trailDt * trailDt;
					Vector trailScreen;
					if (Utils::WorldToScreen(trailEnd, trailScreen, Globals::ViewMatrix, screenW, screenH))
					{
						int tA = (int)(a * 0.5f);
						if (tA > 0) {
							draw->AddLine(ImVec2(trailScreen.x, trailScreen.y), center,
								IM_COL32(r, g, b, tA), 1.2f * distScale);
						}
					}
				}
			}
		}
	}
}





struct HitLogEntry {
	std::string hitboxName;
	std::string playerName;
	int damage;
	float spawnTime;
	float yOffset;        
	float targetYOffset;  
	float alpha;          
	bool isKill;          
};


struct WorldDamageNumber {
	Vector worldPos;      
	int damage;
	float spawnTime;
	float scale;          
	float alpha;          
	bool isKill;
};

static std::vector<HitLogEntry> s_hitlog_entries;
static std::vector<WorldDamageNumber> s_world_damage_numbers;
static std::mutex s_hitlog_mutex;
static int s_hitlog_lastTotalHits = 0;
static bool s_hitlog_initialized = false;
static std::unordered_map<uintptr_t, int> s_hitlog_lastHealth;


static const char* GetHitboxName(int hitgroup)
{
	switch (hitgroup)
	{
	case 1: return "HEAD";
	case 2: return "CHEST";
	case 3: return "STOMACH";
	case 4: return "LEFT ARM";
	case 5: return "RIGHT ARM";
	case 6: return "LEFT LEG";
	case 7: return "RIGHT LEG";
	case 0: return "BODY";
	default: return "BODY";
	}
}


static int TraceHitbox(const Vector& eyePos, const Vector& direction, C_CSPlayerPawn* targetPawn)
{
	if (!Utils::IsValidPtr(reinterpret_cast<uintptr_t>(targetPawn)))
		return 0;

	
	Vector headPos = Utils::GetBonePos(targetPawn, BoneID::Head);
	Vector neckPos = Utils::GetBonePos(targetPawn, BoneID::Neck);
	Vector spinePos = Utils::GetBonePos(targetPawn, BoneID::Spine);
	Vector pelvisPos = Utils::GetBonePos(targetPawn, BoneID::Pelvis);
	Vector leftArmPos = Utils::GetBonePos(targetPawn, BoneID::LeftArm);
	Vector rightArmPos = Utils::GetBonePos(targetPawn, BoneID::RightArm);
	Vector leftKneePos = Utils::GetBonePos(targetPawn, BoneID::LeftKnee);
	Vector rightKneePos = Utils::GetBonePos(targetPawn, BoneID::RightKnee);
	Vector leftFootPos = Utils::GetBonePos(targetPawn, BoneID::LeftFoot);
	Vector rightFootPos = Utils::GetBonePos(targetPawn, BoneID::RightFoot);

	
	auto DistanceToRay = [&](const Vector& bonePos) -> float {
		if (bonePos.IsZero()) return 9999.0f;
		
		Vector toPoint = bonePos - eyePos;
		float proj = toPoint.Dot(direction);
		if (proj < 0.0f) return 9999.0f;
		
		Vector closest = eyePos + direction * proj;
		return (bonePos - closest).Length();
	};

	
	struct BoneHit {
		int hitgroup;
		float distance;
	};

	std::vector<BoneHit> hits;

	
	float headDist = DistanceToRay(headPos);
	if (headDist < 12.0f) hits.push_back({ 1, headDist });

	
	float neckDist = DistanceToRay(neckPos);
	if (neckDist < 15.0f) hits.push_back({ 2, neckDist });
	
	float spineDist = DistanceToRay(spinePos);
	if (spineDist < 15.0f) hits.push_back({ 2, spineDist });

	
	float pelvisDist = DistanceToRay(pelvisPos);
	if (pelvisDist < 15.0f) hits.push_back({ 3, pelvisDist });

	
	float leftArmDist = DistanceToRay(leftArmPos);
	if (leftArmDist < 8.0f) hits.push_back({ 4, leftArmDist });

	
	float rightArmDist = DistanceToRay(rightArmPos);
	if (rightArmDist < 8.0f) hits.push_back({ 5, rightArmDist });

	
	float leftKneeDist = DistanceToRay(leftKneePos);
	if (leftKneeDist < 10.0f) hits.push_back({ 6, leftKneeDist });
	
	float leftFootDist = DistanceToRay(leftFootPos);
	if (leftFootDist < 10.0f) hits.push_back({ 6, leftFootDist });

	
	float rightKneeDist = DistanceToRay(rightKneePos);
	if (rightKneeDist < 10.0f) hits.push_back({ 7, rightKneeDist });
	
	float rightFootDist = DistanceToRay(rightFootPos);
	if (rightFootDist < 10.0f) hits.push_back({ 7, rightFootDist });

	
	if (hits.empty()) return 0; 

	BoneHit closest = hits[0];
	for (const auto& hit : hits)
	{
		if (hit.distance < closest.distance)
			closest = hit;
	}

	return closest.hitgroup;
}

void Misc::HitLogUpdate()
{
	if (!Globals::hitlog_enabled && !Globals::hitlog_draw_damage)
	{
		std::lock_guard<std::mutex> lock(s_hitlog_mutex);
		s_hitlog_entries.clear();
		s_world_damage_numbers.clear();
		s_hitlog_lastHealth.clear();
		s_hitlog_lastTotalHits = 0;
		s_hitlog_initialized = false;
		return;
	}

	
	int totalHits = 0;
	if (!ReadHitsoundData(totalHits)) return;

	if (!s_hitlog_initialized) {
		s_hitlog_lastTotalHits = totalHits;
		s_hitlog_initialized = true;
		return;
	}

	
	if (totalHits > s_hitlog_lastTotalHits && s_hitlog_lastTotalHits >= 0)
	{
		uintptr_t client = Memory::GetModuleBase("client.dll");
		if (client)
		{
			uintptr_t localPawnAddr = Utils::SafeRead<uintptr_t>(client + Offsets::dwLocalPlayerPawn);
			if (Utils::IsValidPtr(localPawnAddr) && localPawnAddr > 0x1000000)
			{
				
				uintptr_t localScene = Utils::SafeRead<uintptr_t>(localPawnAddr + Offsets::m_pGameSceneNode);
				Vector localOrigin = { 0, 0, 0 };
				if (Utils::IsValidPtr(localScene))
					localOrigin = Utils::SafeRead<Vector>(localScene + Offsets::m_vecAbsOrigin);
				Vector viewOff = Utils::SafeRead<Vector>(localPawnAddr + Offsets::m_vecViewOffset);
				Vector eyePos = { localOrigin.x + viewOff.x, localOrigin.y + viewOff.y, localOrigin.z + viewOff.z };

				
				uintptr_t viewAnglesAddr = client + Offsets::dwViewAngles;
				float pitch = Utils::SafeRead<float>(viewAnglesAddr);
				float yaw = Utils::SafeRead<float>(viewAnglesAddr + 4);
				constexpr float D2R = 3.14159265f / 180.0f;
				float cp = cosf(pitch * D2R), sp = sinf(pitch * D2R);
				float cy = cosf(yaw * D2R), sy = sinf(yaw * D2R);
				Vector fwd = { cp * cy, cp * sy, -sp };

				
				auto entities = EntityManager::Get().GetEntities();
				float bestDot = -999.0f;
				uintptr_t bestPawn = 0;
				int bestPrevHealth = 100;
				int bestCurrentHealth = 100;

				for (const auto& ent : entities)
				{
					if (!ent.isEnemy || !ent.pawn) continue;
					uintptr_t pawnAddr = reinterpret_cast<uintptr_t>(ent.pawn);
					if (!Utils::IsValidPtr(pawnAddr)) continue;

					uintptr_t sceneNode = Utils::SafeRead<uintptr_t>(pawnAddr + Offsets::m_pGameSceneNode);
					if (!Utils::IsValidPtr(sceneNode)) continue;

					Vector entPos = Utils::SafeRead<Vector>(sceneNode + Offsets::m_vecAbsOrigin);
					entPos.z += 40.0f;

					
					float dx = entPos.x - eyePos.x;
					float dy = entPos.y - eyePos.y;
					float dz = entPos.z - eyePos.z;
					float dist = sqrtf(dx * dx + dy * dy + dz * dz);
					if (dist < 1.0f) continue;

					float ndx = dx / dist, ndy = dy / dist, ndz = dz / dist;
					float dot = fwd.x * ndx + fwd.y * ndy + fwd.z * ndz;

					if (dot > bestDot)
					{
						bestDot = dot;
						bestPawn = pawnAddr;
						bestCurrentHealth = Utils::SafeRead<int>(pawnAddr + Offsets::m_iHealth);
						
						
						auto it = s_hitlog_lastHealth.find(pawnAddr);
						if (it != s_hitlog_lastHealth.end())
							bestPrevHealth = it->second;
						else
							bestPrevHealth = bestCurrentHealth; 
					}
				}

				if (bestDot > 0.5f && bestPawn != 0)
				{
					
					int damage = bestPrevHealth - bestCurrentHealth;
					bool isKill = false;
					
					
					if (damage <= 0)
					{
						
						bool alive = Utils::SafeAlive(reinterpret_cast<C_CSPlayerPawn*>(bestPawn));
						if (!alive || bestCurrentHealth <= 0)
						{
							damage = 0;
							isKill = true;
						}
						else
						{
							
							s_hitlog_lastTotalHits = totalHits;
							return;
						}
					}
					
					if (damage > 100) damage = 100;

					
					C_CSPlayerPawn* pawn = reinterpret_cast<C_CSPlayerPawn*>(bestPawn);
					int hitgroup = TraceHitbox(eyePos, fwd, pawn);
					const char* hitboxName = GetHitboxName(hitgroup);

					
					std::string playerName = "Enemy";
					for (const auto& ent : entities)
					{
						if (reinterpret_cast<uintptr_t>(ent.pawn) == bestPawn && ent.controller)
						{
							uintptr_t ctrlAddr = reinterpret_cast<uintptr_t>(ent.controller);
							char nameBuf[128] = {};
							if (ReadPlayerName(ctrlAddr, nameBuf, sizeof(nameBuf)) && nameBuf[0] != '\0')
							{
								playerName = std::string(nameBuf);
							}
							break;
						}
					}

					
					HitLogEntry entry;
					entry.hitboxName = hitboxName;
					entry.playerName = playerName;
					entry.damage = damage;
					entry.spawnTime = Misc_GetTime();
					entry.yOffset = -80.0f;  
					entry.targetYOffset = 0.0f;
					entry.alpha = 0.0f;
					entry.isKill = isKill;

					std::lock_guard<std::mutex> lock(s_hitlog_mutex);
					
					
					for (auto& e : s_hitlog_entries)
					{
						e.targetYOffset += 70.0f;
					}

					
					s_hitlog_entries.push_back(entry);

					
					while (s_hitlog_entries.size() > (size_t)Globals::hitlog_max_visible)
					{
						s_hitlog_entries.erase(s_hitlog_entries.begin());
					}

					
					if (Globals::hitlog_draw_damage)
					{
						
						Vector hitPos = { 0, 0, 0 };
						
						
						switch (hitgroup)
						{
						case 1: 
							hitPos = Utils::GetBonePos(pawn, BoneID::Head);
							break;
						case 2: 
							hitPos = Utils::GetBonePos(pawn, BoneID::Spine);
							break;
						case 3: 
							hitPos = Utils::GetBonePos(pawn, BoneID::Pelvis);
							break;
						case 4: 
							hitPos = Utils::GetBonePos(pawn, BoneID::LeftArm);
							break;
						case 5: 
							hitPos = Utils::GetBonePos(pawn, BoneID::RightArm);
							break;
						case 6: 
							hitPos = Utils::GetBonePos(pawn, BoneID::LeftKnee);
							break;
						case 7: 
							hitPos = Utils::GetBonePos(pawn, BoneID::RightKnee);
							break;
						default:
							hitPos = Utils::GetBonePos(pawn, BoneID::Spine);
							break;
						}

						if (!hitPos.IsZero())
						{
							WorldDamageNumber worldDmg;
							worldDmg.worldPos = hitPos;
							worldDmg.damage = damage;
							worldDmg.spawnTime = Misc_GetTime();
							worldDmg.scale = 0.5f;
							worldDmg.alpha = 1.0f;
							worldDmg.isKill = isKill;
							s_world_damage_numbers.push_back(worldDmg);
						}
					}

					
					s_hitlog_lastHealth[bestPawn] = bestCurrentHealth;
				}
			}
		}
	}
	else if (totalHits == 0 && s_hitlog_lastTotalHits != 0)
	{
		
		s_hitlog_lastHealth.clear();
	}

	s_hitlog_lastTotalHits = totalHits;

	
	auto entities = EntityManager::Get().GetEntities();
	std::unordered_set<uintptr_t> seenPawns;
	
	for (const auto& ent : entities)
	{
		if (!ent.isEnemy || !ent.pawn) continue;
		uintptr_t pawnAddr = reinterpret_cast<uintptr_t>(ent.pawn);
		if (!Utils::IsValidPtr(pawnAddr)) continue;

		seenPawns.insert(pawnAddr);
		
		
		int health = Utils::SafeRead<int>(pawnAddr + Offsets::m_iHealth);
		s_hitlog_lastHealth[pawnAddr] = health;
	}
	
	
	for (auto it = s_hitlog_lastHealth.begin(); it != s_hitlog_lastHealth.end();)
	{
		if (seenPawns.find(it->first) == seenPawns.end())
			it = s_hitlog_lastHealth.erase(it);
		else
			++it;
	}

	
	float now = Misc_GetTime();
	{
		std::lock_guard<std::mutex> lock(s_hitlog_mutex);
		s_hitlog_entries.erase(
			std::remove_if(s_hitlog_entries.begin(), s_hitlog_entries.end(),
				[now](const HitLogEntry& e) { 
					return (now - e.spawnTime) > Globals::hitlog_duration; 
				}),
			s_hitlog_entries.end());
		
		
		s_world_damage_numbers.erase(
			std::remove_if(s_world_damage_numbers.begin(), s_world_damage_numbers.end(),
				[now](const WorldDamageNumber& d) { 
					return (now - d.spawnTime) > 2.0f; 
				}),
			s_world_damage_numbers.end());
	}
}

void Misc::HitLogRender()
{
	if (!Globals::hitlog_enabled && !Globals::hitlog_draw_damage) return;

	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	if (!draw) return;

	float now = Misc_GetTime();
	float slideSpeed = Globals::hitlog_slide_speed;
	float fadeSpeed = Globals::hitlog_fade_speed;

	std::lock_guard<std::mutex> lock(s_hitlog_mutex);

	
	if (Globals::hitlog_draw_damage)
	{
		float screenW = Globals::GameViewportWidth;
		float screenH = Globals::GameViewportHeight;

		for (auto& dmg : s_world_damage_numbers)
		{
			float age = now - dmg.spawnTime;
			if (age < 0.0f || age > 2.0f) continue;

			
			float growPhase = 0.3f;   
			float holdPhase = 0.5f;   
			float fadePhase = 1.2f;   

			
			if (age < growPhase)
			{
				float t = age / growPhase;
				t = t * t * (3.0f - 2.0f * t); 
				dmg.scale = 0.5f + t * 1.0f; 
			}
			else if (age < growPhase + holdPhase)
			{
				dmg.scale = 1.5f;
			}
			else
			{
				float t = (age - growPhase - holdPhase) / fadePhase;
				dmg.scale = 1.5f + t * 0.3f; 
			}

			
			if (age < growPhase + holdPhase)
			{
				dmg.alpha = 1.0f;
			}
			else
			{
				float t = (age - growPhase - holdPhase) / fadePhase;
				dmg.alpha = 1.0f - t;
			}

			if (dmg.alpha <= 0.0f) continue;

			
			Vector worldPos = dmg.worldPos;
			worldPos.z += age * 30.0f; 

			
			Vector screenPos;
			if (!Utils::WorldToScreen(worldPos, screenPos, Globals::ViewMatrix, screenW, screenH))
				continue;

			
			char damageText[32];
			if (dmg.isKill)
			{
				snprintf(damageText, sizeof(damageText), "KILL");
			}
			else
			{
				snprintf(damageText, sizeof(damageText), "-%d", dmg.damage);
			}

			
			ImU32 textColor;
			if (dmg.isKill)
			{
				textColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.2f, 0.2f, dmg.alpha));
			}
			else
			{
				textColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
					Globals::hitlog_damage_color[0],
					Globals::hitlog_damage_color[1],
					Globals::hitlog_damage_color[2],
					dmg.alpha
				));
			}

			
			ImFont* font = ImGui::GetFont();
			float baseFontSize = font->FontSize;
			float fontSize = baseFontSize * dmg.scale;

			
			ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, damageText);
			ImVec2 textPos(screenPos.x - textSize.x * 0.5f, screenPos.y - textSize.y * 0.5f);

			
			ImU32 shadowColor = IM_COL32(0, 0, 0, (int)(dmg.alpha * 200.0f));
			for (int ox = -1; ox <= 1; ox++)
			{
				for (int oy = -1; oy <= 1; oy++)
				{
					if (ox == 0 && oy == 0) continue;
					draw->AddText(font, fontSize, 
						ImVec2(textPos.x + ox * 2, textPos.y + oy * 2), 
						shadowColor, damageText);
				}
			}

			
			draw->AddText(font, fontSize, textPos, textColor, damageText);
		}
	}

	
	if (!Globals::hitlog_enabled) return;

	
	float startX = Globals::GameViewportX + 20.0f;
	float startY = Globals::GameViewportY + 20.0f;

	for (auto& entry : s_hitlog_entries)
	{
		float age = now - entry.spawnTime;
		float lifetime = Globals::hitlog_duration;

		
		if (age < slideSpeed)
		{
			float t = age / slideSpeed;
			t = t * t * (3.0f - 2.0f * t); 
			entry.yOffset = -80.0f + t * (entry.targetYOffset + 80.0f);
			entry.alpha = t;
		}
		else
		{
			
			float diff = entry.targetYOffset - entry.yOffset;
			entry.yOffset += diff * 0.15f; 
			entry.alpha = 1.0f;
		}

		
		if (age > lifetime - fadeSpeed)
		{
			float fadeT = (lifetime - age) / fadeSpeed;
			entry.alpha = fadeT;
		}

		if (entry.alpha <= 0.0f) continue;

		
		float posX = startX;
		float posY = startY + entry.yOffset;

		
		float panelW = entry.isKill ? 320.0f : 280.0f;
		float panelH = 60.0f;
		float rounding = 8.0f;

		
		int bgAlpha = (int)(entry.alpha * 220.0f);
		draw->AddRectFilled(
			ImVec2(posX, posY), 
			ImVec2(posX + panelW, posY + panelH),
			IM_COL32(18, 18, 18, bgAlpha), 
			rounding
		);

		
		int borderAlpha = (int)(entry.alpha * 100.0f);
		draw->AddRect(
			ImVec2(posX, posY), 
			ImVec2(posX + panelW, posY + panelH),
			IM_COL32(60, 60, 60, borderAlpha), 
			rounding, 
			0, 
			1.5f
		);

		
		ImU32 accentColor;
		if (entry.isKill)
		{
			accentColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.2f, 0.2f, entry.alpha));
		}
		else
		{
			accentColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
				Globals::hitlog_damage_color[0],
				Globals::hitlog_damage_color[1],
				Globals::hitlog_damage_color[2],
				entry.alpha
			));
		}
		draw->AddRectFilled(
			ImVec2(posX + 1, posY + 1), 
			ImVec2(posX + panelW - 1, posY + 3),
			accentColor
		);

		
		float textY = posY + 15.0f;
		int textAlpha = (int)(entry.alpha * 255.0f);

		if (entry.isKill)
		{
			
			
			ImVec2 skullCenter(posX + 22.0f, textY + 8.0f);
			float skullSize = 10.0f;
			
			
			draw->AddCircleFilled(skullCenter, skullSize, IM_COL32(255, 50, 50, textAlpha), 16);
			draw->AddCircleFilled(ImVec2(skullCenter.x, skullCenter.y + skullSize * 0.6f), skullSize * 0.7f, IM_COL32(255, 50, 50, textAlpha), 16);
			
			
			draw->AddCircleFilled(ImVec2(skullCenter.x - 4.0f, skullCenter.y - 2.0f), 2.5f, IM_COL32(0, 0, 0, textAlpha), 8);
			draw->AddCircleFilled(ImVec2(skullCenter.x + 4.0f, skullCenter.y - 2.0f), 2.5f, IM_COL32(0, 0, 0, textAlpha), 8);
			
			
			draw->AddTriangleFilled(
				ImVec2(skullCenter.x, skullCenter.y + 2.0f),
				ImVec2(skullCenter.x - 2.0f, skullCenter.y + 5.0f),
				ImVec2(skullCenter.x + 2.0f, skullCenter.y + 5.0f),
				IM_COL32(0, 0, 0, textAlpha)
			);

			
			char killText[256];
			snprintf(killText, sizeof(killText), "%s OLDU", entry.playerName.c_str());
			
			ImU32 killColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.3f, 0.3f, entry.alpha));
			
			ImFont* currentFont = ImGui::GetFont();
			float fontSize = currentFont->FontSize * 1.2f;
			
			draw->AddText(
				currentFont,
				fontSize,
				ImVec2(posX + 50.0f + 1, textY + 1),
				IM_COL32(0, 0, 0, textAlpha / 2),
				killText
			);
			draw->AddText(
				currentFont,
				fontSize,
				ImVec2(posX + 50.0f, textY),
				killColor,
				killText
			);
		}
		else
		{
			
			
			draw->AddText(
				ImVec2(posX + 15.0f, textY),
				IM_COL32(200, 200, 200, textAlpha),
				"HIT"
			);

			
			ImU32 hitboxColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
				Globals::hitlog_hitbox_color[0],
				Globals::hitlog_hitbox_color[1],
				Globals::hitlog_hitbox_color[2],
				entry.alpha
			));
			
			draw->AddText(
				ImVec2(posX + 55.0f, textY),
				hitboxColor,
				entry.hitboxName.c_str()
			);

			
			char damageText[32];
			snprintf(damageText, sizeof(damageText), "-%d HP", entry.damage);
			
			ImU32 damageColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
				Globals::hitlog_damage_color[0],
				Globals::hitlog_damage_color[1],
				Globals::hitlog_damage_color[2],
				entry.alpha
			));

			ImFont* currentFont = ImGui::GetFont();
			float fontSize = currentFont->FontSize * 1.3f;
			
			ImVec2 damageSize = ImGui::CalcTextSize(damageText);
			float damageX = posX + panelW - damageSize.x - 15.0f;
			
			
			draw->AddText(
				currentFont,
				fontSize,
				ImVec2(damageX + 1, textY + 1),
				IM_COL32(0, 0, 0, textAlpha / 2),
				damageText
			);
			draw->AddText(
				currentFont,
				fontSize,
				ImVec2(damageX, textY),
				damageColor,
				damageText
			);
		}
	}
}





static std::chrono::steady_clock::time_point s_chatspam_lastSend;
static bool s_chatspam_initialized = false;

void Misc::ChatSpamUpdate()
{
	if (!Globals::chatspam_enabled) {
		s_chatspam_initialized = false;
		return;
	}

	if (!Interfaces::m_pEngine) return;

	
	if (!s_chatspam_initialized) {
		s_chatspam_lastSend = std::chrono::steady_clock::now();
		s_chatspam_initialized = true;
		return;
	}

	
	auto now = std::chrono::steady_clock::now();
	float elapsed = std::chrono::duration<float>(now - s_chatspam_lastSend).count();

	if (elapsed < Globals::chatspam_interval) return;
	if (Globals::chatspam_message[0] == '\0') return;

	s_chatspam_lastSend = now;

	
	char cmd[300] = {};
	snprintf(cmd, sizeof(cmd), "say %s", Globals::chatspam_message);

	__try {
		Interfaces::m_pEngine->ExecuteClientCMD(cmd);
	} __except (EXCEPTION_EXECUTE_HANDLER) {}
}





void Misc::Render()
{
	
	HitmarkerRender();
	
	
	SpectatorListRender();

	
	BombTimerRender();

	
	BulletTracerRender();
	KillParticleRender();
	HitParticleRender();
	HitLogRender();
}

void Misc::Run() {
	
	HitManagerUpdate();
	
	SpectatorListUpdate();
	
	BombTimerUpdate();
	
	BulletTracerUpdate();
	KillParticleUpdate();
	HitParticleUpdate();
	HitLogUpdate();
	DeathSoundUpdate();
	ThirdPerson();
	AntiFlash();
	SmokeColorChanger();
	ChatSpamUpdate();
}

void Misc::run_bunnyhop(c_user_cmd* cmd)
{
    if (!Globals::bunnyhop_enabled || !cmd)
        return;

    uintptr_t client = Memory::GetModuleBase("client.dll");
    if (!client)
        return;

    uintptr_t localPawnAddr = Utils::SafeRead<uintptr_t>(client + Offsets::dwLocalPlayerPawn);
    if (!localPawnAddr)
        return;

    auto* localPawn = reinterpret_cast<C_CSPlayerPawn*>(localPawnAddr);
    if (!localPawn->IsAlive())
        return;

    const uint8_t moveType = Utils::SafeRead<uint8_t>(localPawnAddr + Offsets::m_MoveType);
    if (moveType == MOVETYPE_NOCLIP || moveType == MOVETYPE_LADDER)
        return;

    const uint32_t flags = Utils::SafeRead<uint32_t>(localPawnAddr + Offsets::m_fFlags);
    const bool onGround = (flags & 1U) != 0;

    
    if ((cmd->m_button_state & IN_JUMP) && !onGround)
    {
        cmd->m_button_state  &= ~IN_JUMP;
        cmd->m_button_state2 &= ~IN_JUMP;
        cmd->m_button_state3 &= ~IN_JUMP;
    }
}

void Misc::run_auto_strafe(c_user_cmd* cmd)
{
    
    if (!Globals::bunnyhop_enabled || !cmd)
        return;

    uintptr_t client = Memory::GetModuleBase("client.dll");
    if (!client)
        return;

    uintptr_t localPawnAddr = Utils::SafeRead<uintptr_t>(client + Offsets::dwLocalPlayerPawn);
    if (!localPawnAddr)
        return;

    auto* localPawn = reinterpret_cast<C_CSPlayerPawn*>(localPawnAddr);
    if (!localPawn->IsAlive())
        return;

    const uint8_t moveType = Utils::SafeRead<uint8_t>(localPawnAddr + Offsets::m_MoveType);
    if (moveType == MOVETYPE_NOCLIP || moveType == MOVETYPE_LADDER)
        return;

    const uint32_t flags = Utils::SafeRead<uint32_t>(localPawnAddr + Offsets::m_fFlags);
    const bool onGround = (flags & 1U) != 0;
    if (onGround)
        return;

    __try
    {
        struct FakePBBase { char pad[0x50]; float flForwardMove; float flSideMove; };
        uintptr_t baseCmdAddr = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(cmd) + 0x18 + 0x28);
        if (!baseCmdAddr)
            return;

        auto* baseCmd = reinterpret_cast<FakePBBase*>(baseCmdAddr);
        baseCmd->flForwardMove = 0.0f;

        const bool keyA = (GetAsyncKeyState('A') & 0x8000) != 0;
        const bool keyD = (GetAsyncKeyState('D') & 0x8000) != 0;
        const bool keyW = (GetAsyncKeyState('W') & 0x8000) != 0;

        float side;
        if (keyA)
            side = 1.0f;
        else if (keyD)
            side = -1.0f;
        else
        {
            static bool s_flip = false;
            s_flip = !s_flip;
            side = s_flip ? 1.0f : -1.0f;
        }

        if (keyW)
            side = -side;

        baseCmd->flSideMove = side * 450.0f;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return;
    }
}
