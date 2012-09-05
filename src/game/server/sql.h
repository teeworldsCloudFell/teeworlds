/* CSqlScore Class by Sushi */
#ifndef GAME_SERVER_SQLSCORE_H
#define GAME_SERVER_SQLSCORE_H

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <engine/shared/jobs.h>

#include "score.h"

class CSqlScore : public IScore
{
	class CGameContext *m_pGameServer;
	class IServer *m_pServer;
	
	sql::Driver *m_pDriver;
	sql::Connection *m_pConnection;
	sql::Statement *m_pStatement;
	sql::ResultSet *m_pResults;
	
	// copy of config vars
	const char* m_pDatabase;
	const char* m_pPrefix;
	const char* m_pUser;
	const char* m_pPass;
	const char* m_pIp;
	int m_Port;
	
	CGameContext *GameServer() { return m_pGameServer; }
	IServer *Server() { return m_pServer; }

	CJob m_aLoadScoreJob[MAX_CLIENTS];
	CJob m_aSaveScoreJob[MAX_CLIENTS];
	CJob m_SaveScoreAllJob;
	CJob m_ShowRankJob;
	CJob m_ShowStatsJob;
	CJob m_ShowTop5Job;

	static int LoadScoreThread(void *pUser);
	static int SaveScoreThread(void *pUser);
	static int SaveScoreAllThread(void *pUser);
	static int ShowRankThread(void *pUser);
	static int ShowStatsThread(void *pUser);
	static int ShowTop5Thread(void *pUser);
	
	void Init();
	
	bool Connect();
	void Disconnect();
	
	// anti SQL injection
	void ClearString(char *pString, int Size);
	
public:
	
	CSqlScore(CGameContext *pGameServer);
	~CSqlScore();
	
	void LoadScore(int ClientID);
	void SaveScore(int ClientID);
	void SaveScoreAll(bool Safe, bool KeepConnection=false);
	void ShowRank(int ClientID, const char *pName, bool Search=false, bool Daily=false);
	void ShowStats(int ClientID, const char *pName, bool Search=false);
	void ShowTop5(int ClientID, int Debut=1, bool Daily=false);
};

struct CSqlScoreData
{
	CSqlScore *m_pSqlData;
	CPlayerData *m_pPlayerData;
	CPlayerData *m_pDailyPlayerData;
	int m_ClientID;
	char m_aName[16];
	char m_aClan[12];
	int m_Country;
	char m_aIP[42];
	char m_aIPRange[8];
	int m_Num;
	bool m_Search;
	bool m_Daily;
};

struct CSqlScoreAllData
{
	CSqlScore *m_pSqlData;
	CSqlScoreData *m_apPlayerSqlData[MAX_CLIENTS];
	bool m_DeletePlayerData;
	bool m_KeepConnection;
};

#endif
