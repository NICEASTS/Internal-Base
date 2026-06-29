#pragma once
#include <string>
#include <vector>
#include "bhop/BhopV2.h"

namespace Misc {
	void Render();
	void Run();

	
	void InitHitsound();       
	void HitManagerUpdate();     
	void PlayHitsound();       
	void HitmarkerRender();    

	
	void InitDeathSound();       
	void DeathSoundUpdate();     

	
	void SpectatorListUpdate();  
	void SpectatorListRender();  

	
	void BombTimerUpdate();      
	void BombTimerRender();      

	
	void BulletTracerUpdate();   
	void BulletTracerRender();   

	
	void KillParticleUpdate();
	void KillParticleRender();

	
	void HitParticleUpdate();
	void HitParticleRender();

	
	void HitLogUpdate();
	void HitLogRender();

	
	void ThirdPerson();

	
	void AntiFlash();

	
	void SmokeColorChanger();

	
	void ChatSpamUpdate();

	
	void run_bunnyhop(c_user_cmd* cmd);

	
	void run_auto_strafe(c_user_cmd* cmd);
}
