/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMECONTROLLER_H
#define GAME_SERVER_GAMECONTROLLER_H

#include <base/vmath.h>
#include <base/tl/array.h>
#include <engine/shared/protocol.h> // MAX_CLIENTS

enum
{
	TRIGGER_ONCE=0,
	TRIGGER_PLAYERONCE,
	TRIGGER_MULTI,
};

/*
	Class: Game Controller
		Controls the main game logic. Keeping track of team and player score,
		winning conditions and specific game logic.
*/
class IGameController
{
	vec2 m_aaSpawnPoints[64];
	int m_aNumSpawnPoints;
	vec2 m_aZSpawn;

	class CGameContext *m_pGameServer;
	class IServer *m_pServer;

protected:
	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }

	struct CSpawnEval
	{
		CSpawnEval()
		{
			m_Got = false;
			m_FriendlyTeam = -1;
			m_Pos = vec2(100,100);
		}

		vec2 m_Pos;
		bool m_Got;
		int m_FriendlyTeam;
		float m_Score;
	};

	void EvaluateSpawnType(CSpawnEval *pEval);

	void CycleMap();
	void ResetGame();

	char m_aMapWish[128];

	int m_GameOverTick;

	int m_RoundCount;

	int m_GameFlags;
	int m_UnbalancedTick;
	bool m_ForceBalanced;

	int m_LastZomb;
	int m_LastZomb2;

public:
	class CTimedEvent
	{
	public:
		float m_Time;
		int64 m_Tick;
		char *m_pAction;

		CTimedEvent() { m_pAction = 0; }
		~CTimedEvent()
		{
			if(m_pAction)
				delete[] m_pAction;
		}
		CTimedEvent(float Time, int64 Tick, const char *pAction)
		{
			m_Time = Time;
			m_Tick = Tick;
			// Saving memory ftw! xD
			m_pAction = new char[str_length(pAction)+1];
			mem_zero(m_pAction, str_length(pAction)+1);
			str_copy(m_pAction, pAction, str_length(pAction)+1);
		}

	};
	array<CTimedEvent> m_lTimedEvents;

	struct CTriggeredEvent
	{
		bool m_State;
		int m_Type;
		bool m_aPlayerState[MAX_CLIENTS];
		char m_aAction[512];
		void Reset(bool Hard)
		{
			m_State = false;
			for(int i = 0; i < MAX_CLIENTS; i++)
				m_aPlayerState[i] = false;
			if(Hard)
			{
				m_Type = TRIGGER_ONCE;
				m_aAction[0] = '\0';
			}
		}
	} m_aTriggeredEvents[32];

	struct CCustomTeleport
	{
		int m_Teleport;
		int m_Team;
		void Reset()
		{
			m_Teleport = -1;
			m_Team = -1;
		}
	} m_aCustomTeleport[32];

	char m_aaOnTeamWinEvent[3][512];

	const char *m_pGameType;

	bool IsTeamplay() const;

	IGameController(class CGameContext *pGameServer);
	virtual ~IGameController();

	int m_aTeamscore[2];
	int m_ZombWarmup;
	int m_SuddenDeath;
	int m_RoundStartTick;

	void StartRound();
	void EndRound();
	void ChangeMap(const char *pToMap);

	bool IsFriendlyFire(int ClientID1, int ClientID2);

	bool IsForceBalanced();

	int ParseExec(char *pCommand, int Size);
	bool RegisterTimedEvent(float Time, const char *pCommand);
	void ResetEvents();
	bool RegisterTriggeredEvent(int ID, int Type, const char *pCommand);
	void OnTrigger(int ID, int TriggeredBy);
	int OnCustomTeleporter(int ID, int Team);

	/*

	*/
	virtual bool CanBeMovedOnBalance(int ClientID);

	virtual void Tick();

	virtual void Snap(int SnappingClient);

	/*
		Function: on_entity
			Called when the map is loaded to process an entity
			in the map.

		Arguments:
			index - Entity index.
			pos - Where the entity is located in the world.

		Returns:
			bool?
	*/
	virtual bool OnEntity(int Index, vec2 Pos);

	/*
		Function: on_CCharacter_spawn
			Called when a CCharacter spawns into the game world.

		Arguments:
			chr - The CCharacter that was spawned.
	*/
	virtual void OnCharacterSpawn(class CCharacter *pChr);

	/*
		Function: on_CCharacter_death
			Called when a CCharacter in the world dies.

		Arguments:
			victim - The CCharacter that died.
			killer - The player that killed it.
			weapon - What weapon that killed it. Can be -1 for undefined
				weapon when switching team or player suicides.
	*/
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);


	virtual void OnPlayerInfoChange(class CPlayer *pP);

	//
	virtual bool CanSpawn(int Team, vec2 *pPos);
	virtual bool ZombieSpawn(vec2 *pOutPos);

	/*

	*/
	virtual const char *GetTeamName(int Team);
	virtual int GetAutoTeam(int NotThisID);
	virtual bool CanJoinTeam(int Team, int NotThisID);
	int ClampTeam(int Team);
	void ZombWarmup(int W);
	void RandomZomb(int Mode);

	virtual void PostReset();
};

#endif
