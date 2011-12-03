#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemodes/crap.h>
#include "door_laser.h"

CDoorLaser::CDoorLaser(CGameWorld *pGameWorld, vec2 From, vec2 To, CDoor *pDoor)
	: CEntity(pGameWorld, NETOBJTYPE_LASER)
{
	m_pDoor = pDoor;
	m_From = From;
	m_Pos = To;
	GameWorld()->InsertEntity(this);
}

void CDoorLaser::HitCharacter(vec2 From, vec2 To)
{
	vec2 At;
	CCharacter *Hit[MAX_CLIENTS] = {0};

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		Hit[i] = GameServer()->m_World.LaserIntersectCharacter(From, To, 2.5f, At, Hit);
		if(Hit[i])
		{
			Hit[i]->m_HittingDoor = true;
			Hit[i]->m_PushDirection = normalize(Hit[i]->m_OldPos - At);
		}
	}
}

void CDoorLaser::Tick()
{
	if(m_pDoor->m_TurnedOn)
		HitCharacter(m_From, m_Pos);
}

void CDoorLaser::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	if(!m_pDoor->m_TurnedOn)
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_From.x;
	pObj->m_FromY = (int)m_From.y;
	pObj->m_StartTick = Server()->Tick()-3;
}
