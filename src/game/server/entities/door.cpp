/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemodes/zesc.h>
#include "door.h"

CDoor::CDoor(CGameWorld *pGameWorld, int Index)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Index = Index;
	m_State = 1;

	Reset();

	GameWorld()->InsertEntity(this);
}

void CDoor::Reset()
{

}

void CDoor::Tick()
{
	if(!GameServer()->zESCController()->DoorState(m_Index))
		return;
	CCharacter *apCloseCCharacters[MAX_CLIENTS];
	int Num = GameServer()->m_World.FindEntities(m_Pos, 16.0f, (CEntity**)apCloseCCharacters, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
	for(int i = 0; i < Num; i++)
	{
		if(!apCloseCCharacters[i]->IsAlive() || apCloseCCharacters[i]->GetPlayer()->GetTeam() == TEAM_SPECTATORS || GameServer()->Collision()->IntersectLine(m_Pos, apCloseCCharacters[i]->m_Pos, NULL, NULL))
			continue;
		if(apCloseCCharacters[i]->m_PrevDoorPos == vec2(0.0f, 0.0f) || distance(apCloseCCharacters[i]->m_PrevDoorPos, apCloseCCharacters[i]->m_Core.m_Pos) > (16.0f * 2.0f))
			apCloseCCharacters[i]->m_PrevDoorPos = apCloseCCharacters[i]->m_PrevPos;
		apCloseCCharacters[i]->m_Core.m_Pos = apCloseCCharacters[i]->m_PrevDoorPos;
		apCloseCCharacters[i]->m_Core.m_Vel = vec2(0.f, 0.f);
	}
}

void CDoor::Snap(int SnappingClient)
{
	if(!GameServer()->zESCController()->DoorState(m_Index) || NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = POWERUP_ARMOR;
	pP->m_Subtype = 0;
}
