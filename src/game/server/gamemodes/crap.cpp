/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include "crap.h"

CGameControllerCRAP::CGameControllerCRAP(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	// Exchange this to a string that identifies your game mode.
	// DM, TDM and CTF are reserved for teeworlds original modes.
	m_pGameType = "CRAP";
	m_pTeleporter = 0;
	GetDoors();
	//m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode
}

CGameControllerCRAP::~CGameControllerCRAP()
{
	delete[] m_pTeleporter;
}

void CGameControllerCRAP::InitTeleporter()
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

void CGameControllerCRAP::GetDoors()
{
	if(!GameServer()->Collision()->Switches())
	{
		dbg_msg("Doors", "No switch layer found");
		return;
	}
	
	int Height = GameServer()->Collision()->Layers()->GameLayer()->m_Height;
	int Width = GameServer()->Collision()->Layers()->GameLayer()->m_Width;
	
	m_lDoors.clear();
	
	// count all tiles
	int TileCount = 0;
	for(int i = 0; i < Width; i++)
	{
		for(int j = 0; j < Height; j++)
		{
			if(GameServer()->Collision()->IsDoor(i*32+16, j*32+16))
				TileCount++;
		}
	}
	
	array<CDoor::CDoorNode> lPos;
	lPos.set_size(TileCount);
	
	// add them to an array
	int Index = 0;
	for(int i = 0; i < Width; i++)
	{
		for(int j = 0; j < Height; j++)
		{
			if(GameServer()->Collision()->IsDoor(i*32+16, j*32+16))
			{
				lPos[Index].m_Pos = vec2(i*32+16, j*32+16);
				lPos[Index].m_Type = GameServer()->Collision()->IsDoor(i*32+16, j*32+16);
				Index++;
			}
		}
	}
	
	// create the doors finally \o/
	array<CDoor::CDoorNode> lDoor;
	for(int i = 0; i < lPos.size(); i++)
	{
		// get start node
		if(lPos[i].m_Type == TILE_DOOR_START)
		{
			lDoor.add(lPos[i]);
			lPos.remove_index(i);
			
			// get the switch number
			int SwitchNum = GameServer()->Collision()->GetSwitchNum(lDoor[0].m_Pos);
			
			if(!SwitchNum)
			{
				dbg_msg("Doors", "Door hasn't got a number");
				return;
			}
			
			// get the switch to the door
			bool FoundSwitch;
			array<vec2> lSwitch;
			for(int k = 0; k < lPos.size(); k++)
			{
				if(lPos[k].m_Type == TILE_DOOR_SWITCH && GameServer()->Collision()->GetSwitchNum(lPos[k].m_Pos) == SwitchNum)
				{
					// assign the switch
					lSwitch.add(lPos[k].m_Pos);
					
					// delete it from big nodes list
					lPos.remove_index(k);
					
					k--;
					FoundSwitch = true;
				}
			}
			if(!FoundSwitch)
			{
				dbg_msg("Doors", "No switch found for door %i", SwitchNum);
				return;
			}

			// get the door ends
			bool FoundEnd;
			for(int j = 0; j < lPos.size(); j++)
			{
				if(lPos[j].m_Type == TILE_DOOR_END && GameServer()->Collision()->GetSwitchNum(lPos[j].m_Pos) == SwitchNum && !GameServer()->Collision()->IntersectLine(lDoor[0].m_Pos, lPos[j].m_Pos, 0x0, 0x0))
				{	
					// add it to the current door list and remove it from the big nodes list
					lDoor.add(lPos[j]);
					lPos.remove_index(j);

					j--;
					FoundEnd = true;
				}
			}
			if(!FoundEnd)
			{
				dbg_msg("Doors", "No endpoint found for Door %i", SwitchNum);
				return;
			}

			// create the door and store it
			CDoor Door(GameServer(), lDoor, lSwitch);
			m_lDoors.add(Door);
			
			// free the arrays
			lSwitch.clear();
			lDoor.clear();
			
			// reset index for outer loop
			i = -1;
		}
	}
	
	// init the doors
	for(int i = 0; i < m_lDoors.size(); i++)
		m_lDoors[i].Init();
}

void CGameControllerCRAP::SwitchDoor(CDoor *pDoor, vec2 Pos)
{
	if((pDoor->m_SwitchTick + (Server()->TickSpeed()*(float)g_Config.m_SvDoorSwitchTime/10)) < Server()->Tick())
	{
		if(pDoor->Switch(Pos))
			pDoor->m_SwitchTick = Server()->Tick();
	}
}	

void CGameControllerCRAP::Tick()
{
	// this is the main part of the gamemode, this function is run every tick

	IGameController::Tick();
}
