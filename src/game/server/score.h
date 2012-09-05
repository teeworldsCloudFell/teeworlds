#ifndef GAME_SERVER_INTERFACE_SCORE_H
#define GAME_SERVER_INTERFACE_SCORE_H

#include <engine/shared/protocol.h>

class CPlayerData
{
public:
	CPlayerData()
	{
		Reset();
	}

	void Reset()
	{
		m_ID = 0;
		m_Score = 0;
		m_Kills = 0;
		m_Deaths = 0;
		m_FlagCaps = 0;
		m_Wins = 0;
		m_Losses = 0;
		m_HammerKills = 0;
		m_GunKills = 0;
		m_ShotgunKills = 0;
		m_GrenadeKills = 0;
		m_RifleKills = 0;
		m_NinjaKills = 0;
		m_BotDetected = 0;
		m_TimePlayed = 0;
	}

	int m_ID;
	int m_Score;
	int m_Kills;
	int m_Deaths;
	int m_FlagCaps;
	int m_Wins;
	int m_Losses;
	int m_HammerKills;
	int m_GunKills;
	int m_ShotgunKills;
	int m_GrenadeKills;
	int m_RifleKills;
	int m_NinjaKills;
	int m_BotDetected;
	int m_TimePlayed;
};

class IScore
{
	CPlayerData m_aPlayerData[MAX_CLIENTS];
	CPlayerData m_aDailyPlayerData[MAX_CLIENTS];

public:
	IScore() {}
	virtual ~IScore() {}

	CPlayerData *PlayerData(int ClientID) { return &m_aPlayerData[ClientID]; }
	CPlayerData *DailyPlayerData(int ClientID) { return &m_aDailyPlayerData[ClientID]; }

	virtual void LoadScore(int ClientID) = 0;
	virtual void SaveScore(int ClientID) = 0;
	virtual void SaveScoreAll(bool Safe, bool KeepConnection=false) = 0;

	virtual void ShowRank(int ClientID, const char *pName, bool Search=false, bool Daily=false) = 0;
	virtual void ShowStats(int ClientID, const char *pName, bool Search=false) = 0;
	virtual void ShowTop5(int ClientID, int Debut=1, bool Daily=false) = 0;
};

#endif
