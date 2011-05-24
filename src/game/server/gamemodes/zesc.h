/* copyright (c) 2007 rajh and gregwar. Score stuff */
#ifndef GAME_SERVER_GAMEMODES_ZESC_H
#define GAME_SERVER_GAMEMODES_ZESC_H

#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>

class CGameControllerZESC : public IGameController
{
public:

	CGameControllerZESC(class CGameContext *pGameServer);
	~CGameControllerZESC();

	vec2 *m_pTeleporter;
	class CFlag *m_apFlags[2];

	void InitTeleporter();

	int m_RoundStarted;
	bool m_DoorState[32];
	int m_DoorTick[32];

	virtual void Tick();
	virtual void Snap(int SnappingClient);
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	virtual bool OnEntity(int Index, vec2 Pos);
	virtual void OnHoldpoint(int Index);
	virtual int ZombStarted();
	virtual void StartZomb(int x);
	virtual void CheckZomb();
	virtual int CountPlayers();
	virtual int CountZombs();
	virtual int CountHumans();
	virtual void Reset();
	virtual bool DoorState(int Index);
	virtual void SetDoorState(int Index, bool State);
};

#endif
