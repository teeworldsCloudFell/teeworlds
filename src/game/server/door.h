#ifndef GAME_SERVER_DOOR_H
#define GAME_SERVER_DOOR_H

#include <base/tl/array.h>
#include "gamecontext.h"

class CDoor
{
private:
	class CGameContext *m_pGameServer;

public:
	struct CDoorNode
	{
		vec2 m_Pos;
		int m_Type;
	};
	
	array<CDoorNode> m_lNodes;
	int m_Type;
	array<vec2> m_lSwitch;
	bool m_TurnedOn;
	int m_SwitchTick;
	
	CDoor() {}
	CDoor(CGameContext *pGameServer, array<CDoorNode> lNodes, array<vec2> lSwitch);
	
	class CGameContext *GameServer() { return m_pGameServer; }
	class IServer *Server() { return m_pGameServer->Server(); }
	
	void Reset();
	void Init();
	bool Switch(vec2 Pos);
};

#endif
