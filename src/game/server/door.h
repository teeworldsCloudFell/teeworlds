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
		int m_Team;
	};
	
	array<CDoorNode> m_lNodes;
	array<CDoorNode> m_lSwitch;
	int m_Team;
	int m_SwitchNum;

	bool m_TurnedOn;
	int m_SwitchTick;
	
	CDoor() {}
	CDoor(CGameContext *pGameServer, int Team, int SwitchNum, array<CDoorNode> lNodes, array<CDoor::CDoorNode> lSwitch);
	
	class CGameContext *GameServer() { return m_pGameServer; }
	class IServer *Server() { return m_pGameServer->Server(); }
	
	void Reset();
	void Init();
	bool Switch(vec2 Pos, bool Silent);
};

#endif
