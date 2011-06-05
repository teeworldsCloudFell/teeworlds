/* copyright (c) 2007 rajh, race mod stuff */
/* copyright (c) 2011 BotoX, zombie escape mod */
#include <engine/shared/config.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include <stdio.h>
#include <string.h>
#include <game/server/entities/flag.h>
#include "zesc.h"

CGameControllerZESC::CGameControllerZESC(class CGameContext *pGameServer) : IGameController(pGameServer)
{
	m_pGameType = "zESC";
	m_GameFlags = GAMEFLAG_TEAMS;
	m_apFlags[2] = 0;
	m_RoundStarted = 0;
	for(int i = 0; i < 32; i++) {
		m_DoorState[i] = true;
		m_DoorTick[i] = 0; }
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

	if(Index != ENTITY_FLAGSTAND_BLUE)
		return false;

	CFlag *F = new CFlag(&GameServer()->m_World, TEAM_BLUE);
	F->m_StandPos = Pos;
	F->m_Pos = F->m_StandPos;
	m_apFlags[TEAM_BLUE] = F;
	GameServer()->m_World.InsertEntity(F);
	return true;
}

int CGameControllerZESC::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	return 0;
}

void CGameControllerZESC::Tick()
{
	IGameController::Tick();

	if(!ZombStarted() || GameServer()->m_pController->m_ZombWarmup || GameServer()->m_World.m_Paused)
		return;
	for(int i = 0; i < 32; i++) {
		if(m_DoorTick[i] > 0) {
			m_DoorTick[i]--;
			if(m_DoorTick[i] == 5*Server()->TickSpeed()) {
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "Door %d opening in 5 seconds.", i+1);
				GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf); }
			else if(m_DoorTick[i] == 0) {
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "Door %d is open. Run!", i+1);
				GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf); 
				SetDoorState(i, false); }
		}
	}

	DoTeamScoreWincheck();

	//Flag
	CFlag *F = m_apFlags[TEAM_BLUE];

	if(!F)
		return;

	CCharacter *apCloseCCharacters[MAX_CLIENTS];
	int Num = GameServer()->m_World.FindEntities(F->m_Pos, CFlag::ms_PhysSize, (CEntity**)apCloseCCharacters, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
	for(int i = 0; i < Num; i++)
	{
		if(!apCloseCCharacters[i]->IsAlive() || apCloseCCharacters[i]->GetPlayer()->GetTeam() == TEAM_SPECTATORS || GameServer()->Collision()->IntersectLine(F->m_Pos, apCloseCCharacters[i]->m_Pos, NULL, NULL))
			continue;

		m_aTeamscore[TEAM_RED] = CountZombs();
		m_aTeamscore[TEAM_BLUE] = CountHumans();

		if(apCloseCCharacters[i]->GetPlayer()->GetTeam() == TEAM_BLUE) //Humans Win :D
		{
			//GameServer()->SendBroadcast("Humans win!", -1);
			m_aTeamscore[TEAM_BLUE] += 100;
			apCloseCCharacters[i]->GetPlayer()->m_Score += 100;
			StartZomb(2);
			CheckZomb();
		}
		else if(apCloseCCharacters[i]->GetPlayer()->GetTeam() == TEAM_RED) //Zombies win :(
		{
			//GameServer()->SendBroadcast("Zombies took over the World!", -1);
			m_aTeamscore[TEAM_RED] += 100;
			apCloseCCharacters[i]->GetPlayer()->m_Score += 100;
			StartZomb(2);
			CheckZomb();
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
	pGameDataObj->m_FlagCarrierRed = FLAG_MISSING;
	if(m_apFlags[TEAM_BLUE])
		pGameDataObj->m_FlagCarrierBlue = FLAG_ATSTAND;
	else
		pGameDataObj->m_FlagCarrierBlue = FLAG_MISSING;
}

void CGameControllerZESC::OnHoldpoint(int Index)
{
	if(m_DoorTick[Index] || !m_DoorState[Index] || !ZombStarted() || GameServer()->m_pController->m_ZombWarmup > 14*Server()->TickSpeed())
		return;
	m_DoorTick[Index] = Server()->TickSpeed()*10;
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "Door %d opening in 10 seconds.", Index+1);
	GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
}

int CGameControllerZESC::ZombStarted()
{
	return m_RoundStarted;
}

void CGameControllerZESC::StartZomb(int x)
{
	if(!x)
		Reset();
	m_RoundStarted = x;
}

void CGameControllerZESC::CheckZomb()
{
	if(CountPlayers() < 2)
		return;
	if(m_RoundStarted == 1 && !GameServer()->m_pController->m_ZombWarmup && !CountZombs()) {
		StartZomb(2);
		CheckZomb();
		return; }
	if((!m_RoundStarted || m_RoundStarted == 2) && !GameServer()->m_pController->m_ZombWarmup) {
		GameServer()->m_pController->EndRound();
		return; }
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if((GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_BLUE) || m_ZombWarmup)
			return; }
	StartZomb(2);
	CheckZomb();
}

int CGameControllerZESC::CountPlayers()
{
	int NumPlayers = 0;
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			NumPlayers++; }
	return NumPlayers;
}

int CGameControllerZESC::CountZombs()
{
	int NumZombs = 0;
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_RED)
			NumZombs++; }
	return NumZombs;
}

int CGameControllerZESC::CountHumans()
{
	int NumHumans = 0;
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_BLUE)
			NumHumans++; }
	return NumHumans;
}

void CGameControllerZESC::Reset()
{
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			GameServer()->m_apPlayers[i]->ResetZomb(); }
	for(int i = 0; i < 32; i++) {
		m_DoorState[i] = true;
		m_DoorTick[i] = 0; }
}

bool CGameControllerZESC::DoorState(int Index)
{
	return m_DoorState[Index];
}

void CGameControllerZESC::SetDoorState(int Index, bool State)
{
	m_DoorState[Index] = State;
	/*if(!State)
	{
		for(int i = 0; i < MAX_CLIENTS; i++) {
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter() && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_RED)
					GameServer()->m_apPlayers[i]->GetCharacter()->m_FreezeTick = Server()->Tick() + Server()->TickSpeed()*3; }
	}*/
}
