#ifndef GAME_SERVER_ENTITIES_SWITCH_H
#define GAME_SERVER_ENTITIES_SWITCH_H

#include <game/server/entity.h>

#include "../door.h"

class CDoorSwitch : public CEntity
{
public:
	CDoorSwitch(CGameWorld *pGameWorld, vec2 Pos, CDoor *pDoor);
	
	virtual void Tick();
	virtual void Snap(int SnappingClient);
	
private:
	CDoor *m_pDoor;
	int m_ID1;
	int m_ID2;
};

#endif
