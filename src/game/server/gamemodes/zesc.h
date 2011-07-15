/* copyright (c) 2011 BotoX, zombie escape mod */
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

	bool m_RoundStarted;
	int m_DoorState[32];
	int m_DoorTick[32];

	int m_NukeTick;
	bool m_NukeLaunched;

	virtual void Tick();
	virtual void Snap(int SnappingClient);
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	virtual bool OnEntity(int Index, vec2 Pos);
	virtual void OnHoldpoint(int Index);
	virtual void OnZStop(int Index);
	virtual bool ZombStarted();
	virtual void StartZomb(bool Value);
	virtual void CheckZomb();
	virtual int CountPlayers();
	virtual int CountZombs();
	virtual int CountHumans();
	virtual void Reset();
	virtual int DoorState(int Index);
	virtual void SetDoorState(int Index, int State);
	virtual bool NukeLaunched();
};

#endif
