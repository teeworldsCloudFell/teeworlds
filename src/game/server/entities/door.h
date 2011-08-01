/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_DOOR_H
#define GAME_SERVER_ENTITIES_DOOR_H

#include <game/server/entity.h>
/**********************************************************************************************
	Door States:
				0 = Open
				1 = Closed
				2 = Closing for zombies
				3 = Closed for zombies
				4 = Reopened
	Doors:
				0-32 = Normal Doors
				32-48 = Zombie Doors
***********************************************************************************************/
enum
{
	DOOR_OPEN = 0,
	DOOR_CLOSED,
	DOOR_ZCLOSING,
	DOOR_ZCLOSED,
	DOOR_REOPEN
};

class CDoor : public CEntity
{
public:
	CDoor(CGameWorld *pGameWorld, int Index);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	int m_Index;
	int m_State;
};

#endif
