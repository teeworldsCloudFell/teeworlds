#include "door.h"
#include "entities/door_laser.h"
#include "entities/door_switch.h"

CDoor::CDoor(CGameContext *pGameServer, array<CDoor::CDoorNode> lNodes, array<vec2> lSwitch) : m_pGameServer(pGameServer), m_lNodes(lNodes), m_lSwitch(lSwitch)
{
	m_TurnedOn = true;
	m_SwitchTick = 0;
}

void CDoor::Reset()
{
	m_TurnedOn = true;
	m_SwitchTick = 0;
}

void CDoor::Init()
{
	// turn on the laser
	for(int i = 0; i < m_lNodes.size(); i++)
		new CDoorLaser(&GameServer()->m_World, m_lNodes[0].m_Pos, m_lNodes[i].m_Pos, this);
	
	// init switch
	for(int i = 0; i < m_lSwitch.size(); i++)
		new CDoorSwitch(&GameServer()->m_World, m_lSwitch[i], this);
}

bool CDoor::Switch(vec2 Pos)
{	
	// switch the door
	m_TurnedOn ^= 1;
	
	GameServer()->CreateSound(Pos, SOUND_WEAPON_NOAMMO);
		
	return true;
}
