/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_CRAP_H
#define GAME_SERVER_GAMEMODES_CRAP_H
#include <base/tl/array.h>
#include <game/server/gamecontroller.h>
#include "../door.h"

// you can subclass GAMECONTROLLER_CTF, GAMECONTROLLER_TDM etc if you want
// todo a modification with their base as well.
class CGameControllerCRAP : public IGameController
{
public:
	CGameControllerCRAP(class CGameContext *pGameServer);
	~CGameControllerCRAP();
	
	vec2 *m_pTeleporter;
	void InitTeleporter();
	
	array<CDoor> m_lDoors;
	void GetDoors();
	void SwitchDoor(CDoor *pDoor, vec2 Pos);

	virtual void Tick();

	// add more (crappy) virtual functions here if you wish
};
#endif
