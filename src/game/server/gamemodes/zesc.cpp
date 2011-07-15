/* copyright (c) 2007 rajh, teleporter */
/* copyright (c) 2011 BotoX, zombie escape mod */
#include <engine/shared/config.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <stdio.h>
#include <string.h>
#include <game/server/entities/flag.h>
#include "zesc.h"

CGameControllerZESC::CGameControllerZESC(class CGameContext *pGameServer) : IGameController(pGameServer)
{
	m_pGameType = "zESC";
	m_GameFlags = GAMEFLAG_TEAMS;
	m_apFlags[TEAM_RED] = 0;
	m_apFlags[TEAM_BLUE] = 0;
	m_RoundStarted = false;
	m_NukeTick = 0;
	m_NukeLaunched = false;
	for(int i = 0; i < 32; i++)
	{
		m_DoorState[i] = 1;
		m_DoorTick[i] = 0;
	}
}

CGameControllerZESC::~CGameControllerZESC()
{
	delete[] m_pTeleporter;
}

void CGameControllerZESC::InitTeleporter()
{
	int ArraySize = 0;
	if(GameServer()->Collision()->Layers()->TeleLayer())
	{
		for(int i = 0; i < GameServer()->Collision()->Layers()->TeleLayer()->m_Width*GameServer()->Collision()->Layers()->TeleLayer()->m_Height; i++)
		{
			// get the array size
			if(GameServer()->Collision()->m_pTele[i].m_Number > ArraySize)
				ArraySize = GameServer()->Collision()->m_pTele[i].m_Number;
		}
	}

	if(!ArraySize)
	{
		m_pTeleporter = 0x0;
		return;
	}

	m_pTeleporter = new vec2[ArraySize];
	mem_zero(m_pTeleporter, ArraySize*sizeof(vec2));

	// assign the values
	for(int i = 0; i < GameServer()->Collision()->Layers()->TeleLayer()->m_Width*GameServer()->Collision()->Layers()->TeleLayer()->m_Height; i++)
	{
		if(GameServer()->Collision()->m_pTele[i].m_Number > 0 && GameServer()->Collision()->m_pTele[i].m_Type == TILE_TELEOUT)
			m_pTeleporter[GameServer()->Collision()->m_pTele[i].m_Number-1] = vec2(i%GameServer()->Collision()->Layers()->TeleLayer()->m_Width*32+16, i/GameServer()->Collision()->Layers()->TeleLayer()->m_Width*32+16);
	}
}

bool CGameControllerZESC::OnEntity(int Index, vec2 Pos)
{
	if(IGameController::OnEntity(Index, Pos))
		return true;

	int Team = -1;
	if(Index == ENTITY_FLAGSTAND_RED) Team = TEAM_RED;
	if(Index == ENTITY_FLAGSTAND_BLUE) Team = TEAM_BLUE;
	if(Team == -1 || m_apFlags[Team])
		return false;

	if(Team == TEAM_BLUE)
		m_apFlags[TEAM_RED] = 0;

	CFlag *F = new CFlag(&GameServer()->m_World, Team);
	F->m_StandPos = Pos;
	F->m_Pos = F->m_StandPos;
	m_apFlags[Team] = F;
	GameServer()->m_World.InsertEntity(F);
	return true;
}

int CGameControllerZESC::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	int HadFlag = 0;

	// drop flags
	for(int i = 0; i < 2; i++)
	{
		CFlag *F = m_apFlags[i];
		if(F && pKiller && pKiller->GetCharacter() && F->m_pCarryingCharacter == pKiller->GetCharacter())
			HadFlag |= 2;
		if(F && F->m_pCarryingCharacter == pVictim)
		{
			GameServer()->CreateSoundGlobal(SOUND_CTF_DROP);
			F->m_DropTick = Server()->Tick();
			F->m_pCarryingCharacter = 0;
			F->m_Vel = vec2(0,0);

			if(pKiller && pKiller->GetTeam() != pVictim->GetPlayer()->GetTeam())
				pKiller->m_Score++;

			HadFlag |= 1;
		}
	}
	return 0;
}

void CGameControllerZESC::Tick()
{
	IGameController::Tick();

	if(!ZombStarted() || GameServer()->m_pController->m_ZombWarmup > 14*Server()->TickSpeed() || GameServer()->m_World.m_Paused)
		return;

	// update flag position
	if(m_apFlags[TEAM_RED] && m_apFlags[TEAM_RED]->m_pCarryingCharacter)
		m_apFlags[TEAM_RED]->m_Pos = m_apFlags[TEAM_RED]->m_pCarryingCharacter->m_Core.m_Pos;

	// Damn fuckin door stuff
	for(int i = 0; i < 32; i++)
	{
		if(m_DoorTick[i] > 0)
		{
			m_DoorTick[i]--;
			if(m_DoorState[i] == 1)
			{
				if(m_DoorTick[i] == 5*Server()->TickSpeed())
				{
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "(All) Door %d opening in 5 seconds.", i+1);
					GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
				}
				else if(!m_DoorTick[i])
				{
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "(All) Door %d is open. Run!", i+1);
					GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf); 
					SetDoorState(i, 0);
				}
			}
			else if(!m_DoorTick[i])
			{
				if(m_DoorState[i] == 2)
				{
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "(Zombies) Door %d closed. Reopening in 10 seconds.", i+1);
					GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
					m_DoorTick[i] = Server()->TickSpeed()*10;
					SetDoorState(i, 3);
				}
				else if(m_DoorState[i] == 3)
				{
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "(Zombies) Door %d is open. Run!", i+1);
					GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
					SetDoorState(i, 4);
				}
			}
		}
	}

	if(m_NukeTick)
	{
		m_NukeTick--;
		char bBuf[128];
		str_format(bBuf, sizeof(bBuf), "Tango down in %.2f seconds!\n      Stay in the bunker!!!", m_NukeTick/(float)Server()->TickSpeed());
		GameServer()->SendBroadcast(bBuf, -1);
		if(!m_NukeTick)
		{
			GameServer()->SendBroadcast("Stay in the bunker!!!", -1);
			m_NukeLaunched = true;
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter() && !GameServer()->Collision()->IsBunker(GameServer()->m_apPlayers[i]->GetCharacter()->m_Pos))
					GameServer()->m_apPlayers[i]->Nuke();
			}
			CheckZomb();
		}
	}

	DoTeamScoreWincheck();

	// Flag
	if(m_apFlags[TEAM_BLUE])
	{
		CFlag *F = m_apFlags[TEAM_BLUE];

		CCharacter *apCloseCCharacters[MAX_CLIENTS];
		int Num = GameServer()->m_World.FindEntities(F->m_Pos, CFlag::ms_PhysSize, (CEntity**)apCloseCCharacters, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
		for(int i = 0; i < Num; i++)
		{
			if(!apCloseCCharacters[i]->IsAlive() || apCloseCCharacters[i]->GetPlayer()->GetTeam() == TEAM_SPECTATORS || GameServer()->Collision()->IntersectLine(F->m_Pos, apCloseCCharacters[i]->m_Pos, NULL, NULL))
				continue;

			if(apCloseCCharacters[i]->GetPlayer()->GetTeam() == TEAM_BLUE) //Humans Win :D
			{
				//GameServer()->SendBroadcast("Humans win!", -1);
				m_aTeamscore[TEAM_BLUE] = 100;
				apCloseCCharacters[i]->GetPlayer()->m_Score += 100;
				GameServer()->m_pController->EndRound();
			}
			else if(apCloseCCharacters[i]->GetPlayer()->GetTeam() == TEAM_RED) //Zombies win :(
			{
				//GameServer()->SendBroadcast("Zombies took over the World!", -1);
				m_aTeamscore[TEAM_RED] = 100;
				apCloseCCharacters[i]->GetPlayer()->m_Score += 100;
				GameServer()->m_pController->EndRound();
			}
		}
	}
	else if(m_apFlags[TEAM_RED])
	{
		CFlag *F = m_apFlags[TEAM_RED];

		// flag hits death-tile or left the game layer, reset it
		if(!F->m_AtStand && (GameServer()->Collision()->GetCollisionAt(F->m_Pos.x, F->m_Pos.y)&CCollision::COLFLAG_DEATH || F->GameLayerClipped(F->m_Pos)))
		{
			GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", "flag_return");
			GameServer()->CreateSoundGlobal(SOUND_CTF_RETURN);
			F->Reset();
		}

		// update flag position
		if(F->m_pCarryingCharacter)
			F->m_Pos = F->m_pCarryingCharacter->m_Core.m_Pos;
		else
		{
			CCharacter *apCloseCCharacters[MAX_CLIENTS];
			int Num = GameServer()->m_World.FindEntities(F->m_Pos, CFlag::ms_PhysSize, (CEntity**)apCloseCCharacters, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
			for(int i = 0; i < Num; i++)
			{
				if(!apCloseCCharacters[i]->IsAlive() || apCloseCCharacters[i]->GetPlayer()->GetTeam() == TEAM_SPECTATORS || GameServer()->Collision()->IntersectLine(F->m_Pos, apCloseCCharacters[i]->m_Pos, NULL, NULL))
					continue;

				F->m_GrabTick = Server()->Tick();
				F->m_pCarryingCharacter = apCloseCCharacters[i];
				F->m_pCarryingCharacter->GetPlayer()->m_Score += 100;
				m_NukeTick = Server()->TickSpeed()*15;

				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "flag_grab player='%d:%s'",
					F->m_pCarryingCharacter->GetPlayer()->GetCID(),
					Server()->ClientName(F->m_pCarryingCharacter->GetPlayer()->GetCID()));
				GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

				for(int c = 0; c < MAX_CLIENTS; c++)
				{
					if(!GameServer()->m_apPlayers[c])
						continue;

					if(GameServer()->m_apPlayers[c]->GetTeam() == F->m_pCarryingCharacter->GetPlayer()->GetTeam())
						GameServer()->CreateSoundGlobal(SOUND_CTF_GRAB_EN, GameServer()->m_apPlayers[c]->GetCID());
					else
						GameServer()->CreateSoundGlobal(SOUND_CTF_GRAB_PL, GameServer()->m_apPlayers[c]->GetCID());
				}
				break;
			}
		}
	}
}

void CGameControllerZESC::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

	CNetObj_GameData *pGameDataObj = (CNetObj_GameData *)Server()->SnapNewItem(NETOBJTYPE_GAMEDATA, 0, sizeof(CNetObj_GameData));
	if(!pGameDataObj)
		return;

	if(m_aTeamscore[TEAM_RED] < 100 && m_aTeamscore[TEAM_BLUE] < 100) {
		pGameDataObj->m_TeamscoreRed = CountZombs();
		pGameDataObj->m_TeamscoreBlue = CountHumans(); }
	else {
		pGameDataObj->m_TeamscoreRed = m_aTeamscore[TEAM_RED];
		pGameDataObj->m_TeamscoreBlue = m_aTeamscore[TEAM_BLUE]; }

	if(m_apFlags[TEAM_BLUE])
		pGameDataObj->m_FlagCarrierBlue = FLAG_ATSTAND;
	else
		pGameDataObj->m_FlagCarrierBlue = FLAG_MISSING;
	if(m_apFlags[TEAM_RED] && !m_apFlags[TEAM_BLUE])
	{
		if(m_apFlags[TEAM_RED]->m_AtStand)
			pGameDataObj->m_FlagCarrierRed = FLAG_ATSTAND;
		else if(m_apFlags[TEAM_RED]->m_pCarryingCharacter && m_apFlags[TEAM_RED]->m_pCarryingCharacter->GetPlayer())
			pGameDataObj->m_FlagCarrierRed = m_apFlags[TEAM_RED]->m_pCarryingCharacter->GetPlayer()->GetCID();
		else
			pGameDataObj->m_FlagCarrierRed = FLAG_TAKEN;
	}
	else
		pGameDataObj->m_FlagCarrierRed = FLAG_MISSING;
}

void CGameControllerZESC::OnHoldpoint(int Index)
{
	if(m_DoorTick[Index] > 0 || (m_DoorState[Index] >= 0 && m_DoorState[Index] <= 4 && m_DoorState[Index] != 1) || !ZombStarted() || GameServer()->m_pController->m_ZombWarmup > 14*Server()->TickSpeed())
		return;

	m_DoorTick[Index] = Server()->TickSpeed()*10;
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "(All) Door %d opening in 10 seconds.", Index+1);
	GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
}

void CGameControllerZESC::OnZStop(int Index)
{
	if(m_DoorState[Index] || !ZombStarted() || GameServer()->m_pController->m_ZombWarmup > 14*Server()->TickSpeed())
		return;

	SetDoorState(Index, 2);
	m_DoorTick[Index] = Server()->TickSpeed()*3;
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "(Zombies) Door %d closing in 3 seconds.", Index+1);
	GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
}

bool CGameControllerZESC::ZombStarted()
{
	return m_RoundStarted;
}

void CGameControllerZESC::StartZomb(bool Value)
{
	if(!Value)
		Reset();
	m_RoundStarted = Value;
}

void CGameControllerZESC::CheckZomb()
{
	if(CountPlayers() < 2)
	{
		StartZomb(0);
		GameServer()->m_pController->ZombWarmup(0);
		GameServer()->m_pController->m_SuddenDeath = 1;
		return;
	}
	GameServer()->m_pController->m_SuddenDeath = 0;

	if(!m_RoundStarted && !GameServer()->m_pController->m_ZombWarmup)
	{
		StartZomb(true);
		GameServer()->m_pController->ZombWarmup(15);
	}
	if(!CountHumans() || !CountZombs())
	{
		if(m_RoundStarted && !GameServer()->m_pController->m_ZombWarmup)
			GameServer()->m_pController->EndRound();

		if(m_NukeLaunched || (!m_apFlags[TEAM_RED] && !m_apFlags[TEAM_BLUE]))
		{
			if(!CountHumans())
				m_aTeamscore[TEAM_RED] = 100;
			if(!CountZombs())
				m_aTeamscore[TEAM_BLUE] = 100;
		}
		return;
	}
}

int CGameControllerZESC::CountPlayers()
{
	int NumPlayers = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && (GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS || GameServer()->m_apPlayers[i]->m_Nuked))
			NumPlayers++;
	}
	return NumPlayers;
}

int CGameControllerZESC::CountZombs()
{
	int NumZombs = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_RED)
			NumZombs++;
	}
	return NumZombs;
}

int CGameControllerZESC::CountHumans()
{
	int NumHumans = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_BLUE)
			NumHumans++;
	}
	return NumHumans;
}

void CGameControllerZESC::Reset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
			GameServer()->m_apPlayers[i]->ResetZomb();
	}
	for(int i = 0; i < 32; i++)
	{
		m_DoorState[i] = 1;
		m_DoorTick[i] = 0;
	}
	m_NukeLaunched = false;
	m_NukeTick = 0;
	GameServer()->SendBroadcast("", -1);
}

int CGameControllerZESC::DoorState(int Index)
{
	return m_DoorState[Index];
}

void CGameControllerZESC::SetDoorState(int Index, int State)
{
	m_DoorState[Index] = State;
}

bool CGameControllerZESC::NukeLaunched()
{
	if(m_NukeLaunched || m_NukeTick)
		return true;
	return false;
}
