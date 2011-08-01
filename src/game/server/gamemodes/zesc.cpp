/* copyright (c) 2007 rajh, teleporter */
/* copyright (c) 2011 BotoX, zombie escape mod */
#include <engine/shared/config.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <stdio.h>
#include <string.h>
#include <game/server/entities/flag.h>
#include <game/server/entities/door.h>
#include "zesc.h"

CGameControllerZESC::CGameControllerZESC(class CGameContext *pGameServer) : IGameController(pGameServer)
{
	m_pGameType = "zESC";
	m_GameFlags = GAMEFLAG_TEAMS|GAMEFLAG_FLAGS;
	m_apFlags[TEAM_RED] = 0;
	m_apFlags[TEAM_BLUE] = 0;
	m_RoundStarted = false;
	m_NukeTick = 0;
	m_NukeLaunched = false;
	for(int i = 0; i < 47; i++)
	{
		m_DoorState[i] = DOOR_CLOSED;
		if(i >= 32)
			m_DoorState[i] = DOOR_OPEN;
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

	if(!ZombStarted() || GameServer()->m_pController->m_ZombWarmup > (g_Config.m_SvZWarmup-1)*Server()->TickSpeed() || GameServer()->m_World.m_Paused)
		return;

	GameServer()->m_pController->DoTeamScoreWincheck();

	// Damn fuckin door stuff
	for(int i = 0; i < 47; i++)
	{
		if(m_DoorTick[i] > 0)
		{
			m_DoorTick[i]--;
			if(m_DoorState[i] == DOOR_CLOSED)
			{
				if(m_DoorTick[i] == 5*Server()->TickSpeed())
				{
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "(All) Door %d opening in 5 seconds.", i+1);
					GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
				}
				else if(!m_DoorTick[i])
				{
					SetDoorState(i, DOOR_OPEN);
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "(All) Door %d is open. Run!", i+1);
					GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf); 
				}
			}
			else if(!m_DoorTick[i])
			{
				if(m_DoorState[i] == DOOR_ZCLOSING && i < 32)
				{
					SetDoorState(i, DOOR_ZCLOSED);
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "(Zombies) Door %d closed. Reopening in %d seconds.", i+1, GetDoorTime(i));
					GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
					m_DoorTick[i] = Server()->TickSpeed()*GetDoorTime(i);
				}
				else if(m_DoorState[i] == DOOR_ZCLOSED && i < 32)
				{
					SetDoorState(i, DOOR_REOPEN);
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "(Zombies) Door %d is open. Run!", i+1);
					GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
				}
				else if(m_DoorState[i] == DOOR_ZCLOSING && i >= 32)
				{
					SetDoorState(i, DOOR_ZCLOSED);
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "(Zombies) ZDoor %d closed. Reopening in %d seconds.", i-31, GetDoorTime(i));
					GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
					m_DoorTick[i] = Server()->TickSpeed()*GetDoorTime(i);
				}
				else if(m_DoorState[i] == DOOR_ZCLOSED && i >= 32)
				{
					SetDoorState(i, DOOR_REOPEN);
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "(Zombies) ZDoor %d is open. Run!", i-31);
					GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
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
		if(!F->m_pCarryingCharacter && !F->m_AtStand)
		{
			if(Server()->Tick() > F->m_DropTick + Server()->TickSpeed()*30)
			{
				GameServer()->CreateSoundGlobal(SOUND_CTF_RETURN);
				F->Reset();
			}
			else
			{
				F->m_Vel.y += GameServer()->m_World.m_Core.m_Tuning.m_Gravity;
				GameServer()->Collision()->MoveBox(&F->m_Pos, &F->m_Vel, vec2(F->ms_PhysSize, F->ms_PhysSize), 0.5f);
			}
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

				// take the flag
				if(F->m_AtStand)
					F->m_GrabTick = Server()->Tick();

				F->m_AtStand = 0;
				F->m_pCarryingCharacter = apCloseCCharacters[i];
				F->m_pCarryingCharacter->GetPlayer()->m_Score += 100;
				m_NukeTick = Server()->TickSpeed()*g_Config.m_SvNukeTime;

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

	if(m_aTeamscore[TEAM_RED] < 100 && m_aTeamscore[TEAM_BLUE] < 100)
	{
		pGameDataObj->m_TeamscoreRed = CountZombs();
		pGameDataObj->m_TeamscoreBlue = CountHumans();
	}
	else
	{
		pGameDataObj->m_TeamscoreRed = m_aTeamscore[TEAM_RED];
		pGameDataObj->m_TeamscoreBlue = m_aTeamscore[TEAM_BLUE];
	}

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
	if(m_DoorTick[Index] > 0 || (m_DoorState[Index] >= DOOR_OPEN && m_DoorState[Index] <= DOOR_REOPEN && m_DoorState[Index] != 1) || !ZombStarted() || GameServer()->m_pController->m_ZombWarmup > (g_Config.m_SvZWarmup-1)*Server()->TickSpeed() || GetDoorTime(Index) == -1)
		return;

	m_DoorTick[Index] = Server()->TickSpeed()*GetDoorTime(Index);
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "(All) Door %d opening in %d seconds.", Index+1, GetDoorTime(Index));
	GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
}

void CGameControllerZESC::OnZStop(int Index)
{
	if(m_DoorState[Index] || !ZombStarted() || GameServer()->m_pController->m_ZombWarmup > (g_Config.m_SvZWarmup-1)*Server()->TickSpeed() || GetDoorTime(Index) == -1)
		return;

	SetDoorState(Index, DOOR_ZCLOSING);
	m_DoorTick[Index] = Server()->TickSpeed()*GetDoorTime(Index);
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "(Zombies) Door %d closing in %d seconds.", Index+1, GetDoorTime(Index));
	if(Index >= 32)
		str_format(aBuf, sizeof(aBuf), "(Zombies) ZDoor %d closing in %d seconds.", Index-31, GetDoorTime(Index));
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
		if(m_apFlags[TEAM_RED] && m_apFlags[TEAM_RED]->m_pCarryingCharacter)
		{
			GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", "flag_return");
			GameServer()->CreateSoundGlobal(SOUND_CTF_RETURN);
			m_apFlags[TEAM_RED]->Reset();
		}
		return;
	}
	GameServer()->m_pController->m_SuddenDeath = 0;

	if(!m_RoundStarted && !GameServer()->m_pController->m_ZombWarmup)
	{
		StartZomb(true);
		GameServer()->m_pController->ZombWarmup(g_Config.m_SvZWarmup);
	}
	if(!CountHumans() || !CountZombs())
	{
		if(m_NukeLaunched || (!m_apFlags[TEAM_RED] && !m_apFlags[TEAM_BLUE]))
		{
			m_aTeamscore[TEAM_RED] = 0;
			m_aTeamscore[TEAM_BLUE] = 0;
			if(!CountHumans())
			{
				m_aTeamscore[TEAM_RED] = 100;
			}
			if(!CountZombs())
			{
				m_aTeamscore[TEAM_BLUE] = 100;
				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_BLUE)
						GameServer()->m_apPlayers[i]->m_Score += 10;
				}
				if(m_apFlags[TEAM_RED] && m_apFlags[TEAM_RED]->m_pCarryingCharacter && m_apFlags[TEAM_RED]->m_pCarryingCharacter->GetPlayer())
					m_apFlags[TEAM_RED]->m_pCarryingCharacter->GetPlayer()->m_Score += 100;
			}
		}
		if(m_RoundStarted && !GameServer()->m_pController->m_ZombWarmup)
			GameServer()->m_pController->EndRound();
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
	for(int i = 0; i < 47; i++)
	{
		m_DoorState[i] = DOOR_CLOSED;
		if(i >= 32)
			m_DoorState[i] = DOOR_OPEN;
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

int CGameControllerZESC::GetDoorTime(int Index)
{
	// Please don't blame me xD
	if(m_DoorState[Index] == DOOR_CLOSED)
	{
		switch(Index)
		{
		case 0:
			return g_Config.m_SvDoor1OpenTime;
		case 1:
			return g_Config.m_SvDoor2OpenTime;
		case 2:
			return g_Config.m_SvDoor3OpenTime;
		case 3:
			return g_Config.m_SvDoor4OpenTime;
		case 4:
			return g_Config.m_SvDoor5OpenTime;
		case 5:
			return g_Config.m_SvDoor6OpenTime;
		case 6:
			return g_Config.m_SvDoor7OpenTime;
		case 7:
			return g_Config.m_SvDoor8OpenTime;
		case 8:
			return g_Config.m_SvDoor9OpenTime;
		case 9:
			return g_Config.m_SvDoor10OpenTime;
		case 10:
			return g_Config.m_SvDoor11OpenTime;
		case 11:
			return g_Config.m_SvDoor12OpenTime;
		case 12:
			return g_Config.m_SvDoor13OpenTime;
		case 13:
			return g_Config.m_SvDoor14OpenTime;
		case 14:
			return g_Config.m_SvDoor15OpenTime;
		case 15:
			return g_Config.m_SvDoor16OpenTime;
		case 16:
			return g_Config.m_SvDoor17OpenTime;
		case 17:
			return g_Config.m_SvDoor18OpenTime;
		case 18:
			return g_Config.m_SvDoor19OpenTime;
		case 19:
			return g_Config.m_SvDoor20OpenTime;
		case 20:
			return g_Config.m_SvDoor21OpenTime;
		case 21:
			return g_Config.m_SvDoor22OpenTime;
		case 22:
			return g_Config.m_SvDoor23OpenTime;
		case 23:
			return g_Config.m_SvDoor24OpenTime;
		case 24:
			return g_Config.m_SvDoor25OpenTime;
		case 25:
			return g_Config.m_SvDoor26OpenTime;
		case 26:
			return g_Config.m_SvDoor27OpenTime;
		case 27:
			return g_Config.m_SvDoor28OpenTime;
		case 28:
			return g_Config.m_SvDoor29OpenTime;
		case 29:
			return g_Config.m_SvDoor30OpenTime;
		case 30:
			return g_Config.m_SvDoor31OpenTime;
		case 31:
			return g_Config.m_SvDoor32OpenTime;
		}
	}
	else if((m_DoorState[Index] == DOOR_ZCLOSING || m_DoorState[Index] == DOOR_OPEN) && Index < 32)
	{
		switch(Index)
		{
		case 0:
			return g_Config.m_SvDoor1CloseTime;
		case 1:
			return g_Config.m_SvDoor2CloseTime;
		case 2:
			return g_Config.m_SvDoor3CloseTime;
		case 3:
			return g_Config.m_SvDoor4CloseTime;
		case 4:
			return g_Config.m_SvDoor5CloseTime;
		case 5:
			return g_Config.m_SvDoor6CloseTime;
		case 6:
			return g_Config.m_SvDoor7CloseTime;
		case 7:
			return g_Config.m_SvDoor8CloseTime;
		case 8:
			return g_Config.m_SvDoor9CloseTime;
		case 9:
			return g_Config.m_SvDoor10CloseTime;
		case 10:
			return g_Config.m_SvDoor11CloseTime;
		case 11:
			return g_Config.m_SvDoor12CloseTime;
		case 12:
			return g_Config.m_SvDoor13CloseTime;
		case 13:
			return g_Config.m_SvDoor14CloseTime;
		case 14:
			return g_Config.m_SvDoor15CloseTime;
		case 15:
			return g_Config.m_SvDoor16CloseTime;
		case 16:
			return g_Config.m_SvDoor17CloseTime;
		case 17:
			return g_Config.m_SvDoor18CloseTime;
		case 18:
			return g_Config.m_SvDoor19CloseTime;
		case 19:
			return g_Config.m_SvDoor20CloseTime;
		case 20:
			return g_Config.m_SvDoor21CloseTime;
		case 21:
			return g_Config.m_SvDoor22CloseTime;
		case 22:
			return g_Config.m_SvDoor23CloseTime;
		case 23:
			return g_Config.m_SvDoor24CloseTime;
		case 24:
			return g_Config.m_SvDoor25CloseTime;
		case 25:
			return g_Config.m_SvDoor26CloseTime;
		case 26:
			return g_Config.m_SvDoor27CloseTime;
		case 27:
			return g_Config.m_SvDoor28CloseTime;
		case 28:
			return g_Config.m_SvDoor29CloseTime;
		case 29:
			return g_Config.m_SvDoor30CloseTime;
		case 30:
			return g_Config.m_SvDoor31CloseTime;
		case 31:
			return g_Config.m_SvDoor32CloseTime;
		}
	}
	else if(m_DoorState[Index] == DOOR_ZCLOSED && Index < 32)
	{
		switch(Index)
		{
		case 0:
			return g_Config.m_SvDoor1ReopenTime;
		case 1:
			return g_Config.m_SvDoor2ReopenTime;
		case 2:
			return g_Config.m_SvDoor3ReopenTime;
		case 3:
			return g_Config.m_SvDoor4ReopenTime;
		case 4:
			return g_Config.m_SvDoor5ReopenTime;
		case 5:
			return g_Config.m_SvDoor6ReopenTime;
		case 6:
			return g_Config.m_SvDoor7ReopenTime;
		case 7:
			return g_Config.m_SvDoor8ReopenTime;
		case 8:
			return g_Config.m_SvDoor9ReopenTime;
		case 9:
			return g_Config.m_SvDoor10ReopenTime;
		case 10:
			return g_Config.m_SvDoor11ReopenTime;
		case 11:
			return g_Config.m_SvDoor12ReopenTime;
		case 12:
			return g_Config.m_SvDoor13ReopenTime;
		case 13:
			return g_Config.m_SvDoor14ReopenTime;
		case 14:
			return g_Config.m_SvDoor15ReopenTime;
		case 15:
			return g_Config.m_SvDoor16ReopenTime;
		case 16:
			return g_Config.m_SvDoor17ReopenTime;
		case 17:
			return g_Config.m_SvDoor18ReopenTime;
		case 18:
			return g_Config.m_SvDoor19ReopenTime;
		case 19:
			return g_Config.m_SvDoor20ReopenTime;
		case 20:
			return g_Config.m_SvDoor21ReopenTime;
		case 21:
			return g_Config.m_SvDoor22ReopenTime;
		case 22:
			return g_Config.m_SvDoor23ReopenTime;
		case 23:
			return g_Config.m_SvDoor24ReopenTime;
		case 24:
			return g_Config.m_SvDoor25ReopenTime;
		case 25:
			return g_Config.m_SvDoor26ReopenTime;
		case 26:
			return g_Config.m_SvDoor27ReopenTime;
		case 27:
			return g_Config.m_SvDoor28ReopenTime;
		case 28:
			return g_Config.m_SvDoor29ReopenTime;
		case 29:
			return g_Config.m_SvDoor30ReopenTime;
		case 30:
			return g_Config.m_SvDoor31ReopenTime;
		case 31:
			return g_Config.m_SvDoor32ReopenTime;
		}
	}
	else if((m_DoorState[Index] == DOOR_ZCLOSING || m_DoorState[Index] == DOOR_OPEN) && Index >= 32)
	{
		switch(Index)
		{
		case 32:
			return g_Config.m_SvZDoor1CloseTime;
		case 33:
			return g_Config.m_SvZDoor2CloseTime;
		case 34:
			return g_Config.m_SvZDoor3CloseTime;
		case 35:
			return g_Config.m_SvZDoor4CloseTime;
		case 36:
			return g_Config.m_SvZDoor5CloseTime;
		case 37:
			return g_Config.m_SvZDoor6CloseTime;
		case 38:
			return g_Config.m_SvZDoor7CloseTime;
		case 39:
			return g_Config.m_SvZDoor8CloseTime;
		case 40:
			return g_Config.m_SvZDoor9CloseTime;
		case 41:
			return g_Config.m_SvZDoor10CloseTime;
		case 42:
			return g_Config.m_SvZDoor11CloseTime;
		case 43:
			return g_Config.m_SvZDoor12CloseTime;
		case 44:
			return g_Config.m_SvZDoor13CloseTime;
		case 45:
			return g_Config.m_SvZDoor14CloseTime;
		case 46:
			return g_Config.m_SvZDoor15CloseTime;
		case 47:
			return g_Config.m_SvZDoor16CloseTime;
		}
	}
	else if(m_DoorState[Index] == DOOR_ZCLOSED && Index >= 32)
	{
		switch(Index)
		{
		case 32:
			return g_Config.m_SvZDoor1ReopenTime;
		case 33:
			return g_Config.m_SvZDoor2ReopenTime;
		case 34:
			return g_Config.m_SvZDoor3ReopenTime;
		case 35:
			return g_Config.m_SvZDoor4ReopenTime;
		case 36:
			return g_Config.m_SvZDoor5ReopenTime;
		case 37:
			return g_Config.m_SvZDoor6ReopenTime;
		case 38:
			return g_Config.m_SvZDoor7ReopenTime;
		case 39:
			return g_Config.m_SvZDoor8ReopenTime;
		case 40:
			return g_Config.m_SvZDoor9ReopenTime;
		case 41:
			return g_Config.m_SvZDoor10ReopenTime;
		case 42:
			return g_Config.m_SvZDoor11ReopenTime;
		case 43:
			return g_Config.m_SvZDoor12ReopenTime;
		case 44:
			return g_Config.m_SvZDoor13ReopenTime;
		case 45:
			return g_Config.m_SvZDoor14ReopenTime;
		case 46:
			return g_Config.m_SvZDoor15ReopenTime;
		case 47:
			return g_Config.m_SvZDoor16ReopenTime;
		}
	}
	return -1;
}