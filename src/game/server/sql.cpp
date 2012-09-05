/* CSqlScore class by Sushi */
#include <string.h>
#include <engine/shared/config.h>

#include "gamecontext.h"
#include "sql.h"

const int STATE_DONE = 2;

CSqlScore::CSqlScore(CGameContext *pGameServer)
	: m_pGameServer(pGameServer),
	m_pServer(pGameServer->Server()),
	m_pDatabase(g_Config.m_SvSqlDatabase),
	m_pPrefix(g_Config.m_SvSqlPrefix),
	m_pUser(g_Config.m_SvSqlUser),
	m_pPass(g_Config.m_SvSqlPw),
	m_pIp(g_Config.m_SvSqlIp),
	m_Port(g_Config.m_SvSqlPort)
{
	m_pDriver = 0;
	m_pConnection = 0;
	m_pStatement = 0;
	m_pResults = 0;

	Init();
}

CSqlScore::~CSqlScore()
{
}

bool CSqlScore::Connect()
{
	if(m_pConnection)
	{
		// Create Statement
		m_pStatement = m_pConnection->createStatement();

		dbg_msg("SQL", "SQL connection reused");
		return true;
	}

	try 
	{
		// Create connection
		m_pDriver = get_driver_instance();
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "tcp://%s:%d", m_pIp, m_Port);
		m_pConnection = m_pDriver->connect(aBuf, m_pUser, m_pPass);

		// Create Statement
		m_pStatement = m_pConnection->createStatement();

		// Create database if not exists
		str_format(aBuf, sizeof(aBuf), "CREATE DATABASE IF NOT EXISTS %s", m_pDatabase);
		m_pStatement->execute(aBuf);

		// Connect to specific database
		m_pConnection->setSchema(m_pDatabase);
		dbg_msg("SQL", "SQL connection established");
		return true;
	} 
	catch (sql::SQLException &e)
	{
		dbg_msg("SQL", "ERROR: SQL connection failed");
		return false;
	}
	return false;
}

void CSqlScore::Disconnect()
{
	try
	{
		delete m_pConnection;
		m_pConnection = 0;
		dbg_msg("SQL", "SQL connection disconnected");
	}
	catch (sql::SQLException &e)
	{
		dbg_msg("SQL", "ERROR: No SQL connection");
	}
}

// create tables... should be done only once
void CSqlScore::Init()
{
	// create connection
	if(Connect())
	{
		try
		{
			// create tables
			char aBuf[768];
			str_format(aBuf, sizeof(aBuf), "CREATE TABLE IF NOT EXISTS %s_savedscore(ID INT NOT NULL AUTO_INCREMENT, Name VARCHAR(16) NOT NULL, JoinName VARCHAR(16) NOT NULL, Clan VARCHAR(12), Country INT DEFAULT -1, JoinIP VARCHAR(42) DEFAULT '0.0.0.0', IP VARCHAR(42) DEFAULT '0.0.0.0', IPRange VARCHAR(8) DEFAULT '0.0', FirstJoin TIMESTAMP DEFAULT CURRENT_TIMESTAMP, LastJoin TIMESTAMP DEFAULT '0000-00-00 00:00:00', TimePlayed INT DEFAULT 0, Score INT DEFAULT 0, Kills INT DEFAULT 0, Deaths INT DEFAULT 0, FlagCaps INT DEFAULT 0, Wins INT DEFAULT 0, Losses INT DEFAULT 0, HammerKills INT DEFAULT 0, GunKills INT DEFAULT 0, ShotgunKills INT DEFAULT 0, GrenadeKills INT DEFAULT 0, RifleKills INT DEFAULT 0, NinjaKills INT DEFAULT 0, BotDetected INT DEFAULT 0, PRIMARY KEY (ID));", m_pPrefix);
			m_pStatement->execute(aBuf);
			str_format(aBuf, sizeof(aBuf), "CREATE TABLE IF NOT EXISTS %s_savedscore_daily(ID INT NOT NULL, Name VARCHAR(16) NOT NULL, Score INT DEFAULT 0, Kills INT DEFAULT 0, Deaths INT DEFAULT 0, FlagCaps INT DEFAULT 0, Wins INT DEFAULT 0, Losses INT DEFAULT 0, HammerKills INT DEFAULT 0, GunKills INT DEFAULT 0, ShotgunKills INT DEFAULT 0, GrenadeKills INT DEFAULT 0, RifleKills INT DEFAULT 0, NinjaKills INT DEFAULT 0, PRIMARY KEY (ID));", m_pPrefix);
			m_pStatement->execute(aBuf);
			dbg_msg("SQL", "Tables were created successfully");

			// delete statement
			delete m_pStatement;
			m_pStatement = 0;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("SQL", "ERROR: Tables were NOT created");
		}

		// disconnect from database
		Disconnect();
	}
}

// update stuff
int CSqlScore::LoadScoreThread(void *pUser)
{
	CSqlScoreData *pData = (CSqlScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			// check strings
			pData->m_pSqlData->ClearString(pData->m_aName, sizeof(pData->m_aName));
			pData->m_pSqlData->ClearString(pData->m_aClan, sizeof(pData->m_aClan));

			char aBuf[512];

			for(int i = 1; i <= 2; i++)
			{
				switch(i)
				{
				case 1:
					{
						// check if there is an entry with the same ip
						str_format(aBuf, sizeof(aBuf), "SELECT ID, TimePlayed, Score, Kills, Deaths, FlagCaps, Wins, Losses, HammerKills, GunKills, ShotgunKills, GrenadeKills, RifleKills, NinjaKills, BotDetected FROM %s_savedscore WHERE IP='%s';", pData->m_pSqlData->m_pPrefix, pData->m_aIP);
						pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);
					} break;
				case 2:
					{
						// check for name and iprange or name and clan
						str_format(aBuf, sizeof(aBuf), "SELECT ID, Score, Kills, Deaths, FlagCaps, Wins, Losses, HammerKills, GunKills, ShotgunKills, GrenadeKills, RifleKills, NinjaKills, BotDetected FROM %s_savedscore WHERE (Name='%s' OR JoinName='%s') AND (IPRange='%s' OR Clan='%s');", pData->m_pSqlData->m_pPrefix, pData->m_aName, pData->m_aName, pData->m_aIPRange, pData->m_aClan);
						pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);
					} break;
				}

				if(pData->m_pSqlData->m_pResults->next())
				{
					// get the playerstats
					pData->m_pPlayerData->m_ID = pData->m_pSqlData->m_pResults->getInt("ID");
					pData->m_pPlayerData->m_TimePlayed = pData->m_pSqlData->m_pResults->getInt("TimePlayed");
					pData->m_pPlayerData->m_Score = pData->m_pSqlData->m_pResults->getInt("Score");
					pData->m_pPlayerData->m_Kills = pData->m_pSqlData->m_pResults->getInt("Kills");
					pData->m_pPlayerData->m_Deaths = pData->m_pSqlData->m_pResults->getInt("Deaths");
					pData->m_pPlayerData->m_FlagCaps = pData->m_pSqlData->m_pResults->getInt("FlagCaps");
					pData->m_pPlayerData->m_Wins = pData->m_pSqlData->m_pResults->getInt("Wins");
					pData->m_pPlayerData->m_Losses = pData->m_pSqlData->m_pResults->getInt("Losses");
					pData->m_pPlayerData->m_HammerKills = pData->m_pSqlData->m_pResults->getInt("HammerKills");
					pData->m_pPlayerData->m_GunKills = pData->m_pSqlData->m_pResults->getInt("GunKills");
					pData->m_pPlayerData->m_ShotgunKills = pData->m_pSqlData->m_pResults->getInt("ShotgunKills");
					pData->m_pPlayerData->m_GrenadeKills = pData->m_pSqlData->m_pResults->getInt("GrenadeKills");
					pData->m_pPlayerData->m_RifleKills = pData->m_pSqlData->m_pResults->getInt("RifleKills");
					pData->m_pPlayerData->m_NinjaKills = pData->m_pSqlData->m_pResults->getInt("NinjaKills");
					pData->m_pPlayerData->m_BotDetected = pData->m_pSqlData->m_pResults->getInt("BotDetected");

					// delete result
					delete pData->m_pSqlData->m_pResults;

					// try to get the todays playerstat
					str_format(aBuf, sizeof(aBuf), "SELECT Score, Kills, Deaths, FlagCaps, Wins, Losses, HammerKills, GunKills, ShotgunKills, GrenadeKills, RifleKills, NinjaKills FROM %s_savedscore_daily WHERE ID=%d;", pData->m_pSqlData->m_pPrefix, pData->m_pPlayerData->m_ID);
					pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

					if(pData->m_pSqlData->m_pResults->next())
					{
						pData->m_pDailyPlayerData->m_Score = pData->m_pSqlData->m_pResults->getInt("Score");
						pData->m_pDailyPlayerData->m_Kills = pData->m_pSqlData->m_pResults->getInt("Kills");
						pData->m_pDailyPlayerData->m_Deaths = pData->m_pSqlData->m_pResults->getInt("Deaths");
						pData->m_pDailyPlayerData->m_FlagCaps = pData->m_pSqlData->m_pResults->getInt("FlagCaps");
						pData->m_pDailyPlayerData->m_Wins = pData->m_pSqlData->m_pResults->getInt("Wins");
						pData->m_pDailyPlayerData->m_Losses = pData->m_pSqlData->m_pResults->getInt("Losses");
						pData->m_pDailyPlayerData->m_HammerKills = pData->m_pSqlData->m_pResults->getInt("HammerKills");
						pData->m_pDailyPlayerData->m_GunKills = pData->m_pSqlData->m_pResults->getInt("GunKills");
						pData->m_pDailyPlayerData->m_ShotgunKills = pData->m_pSqlData->m_pResults->getInt("ShotgunKills");
						pData->m_pDailyPlayerData->m_GrenadeKills = pData->m_pSqlData->m_pResults->getInt("GrenadeKills");
						pData->m_pDailyPlayerData->m_RifleKills = pData->m_pSqlData->m_pResults->getInt("RifleKills");
						pData->m_pDailyPlayerData->m_NinjaKills = pData->m_pSqlData->m_pResults->getInt("NinjaKills");
					}
					else
					{
						str_format(aBuf, sizeof(aBuf), "INSERT IGNORE INTO %s_savedscore_daily(ID, Name, Score, Kills, Deaths, FlagCaps, Wins, Losses, HammerKills, GunKills, ShotgunKills, GrenadeKills, RifleKills, NinjaKills) VALUES ('%d', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d');", pData->m_pSqlData->m_pPrefix, pData->m_pPlayerData->m_ID, pData->m_aName, pData->m_pDailyPlayerData->m_Score, pData->m_pDailyPlayerData->m_Kills, pData->m_pDailyPlayerData->m_Deaths, pData->m_pDailyPlayerData->m_FlagCaps, pData->m_pDailyPlayerData->m_Wins, pData->m_pDailyPlayerData->m_Losses, pData->m_pDailyPlayerData->m_HammerKills, pData->m_pDailyPlayerData->m_GunKills, pData->m_pDailyPlayerData->m_ShotgunKills, pData->m_pDailyPlayerData->m_GrenadeKills, pData->m_pDailyPlayerData->m_RifleKills, pData->m_pDailyPlayerData->m_NinjaKills);
						pData->m_pSqlData->m_pStatement->execute(aBuf);
					}

					if(i == 1)
						dbg_msg("SQL", "Getting playerstat done [IP]");
					else
						dbg_msg("SQL", "Getting playerstat done [Name]");

					// delete statement and results
					delete pData->m_pSqlData->m_pStatement;
					delete pData->m_pSqlData->m_pResults;
					pData->m_pSqlData->m_pStatement = 0;
					pData->m_pSqlData->m_pResults = 0;

					// disconnect from database
					//pData->m_pSqlData->Disconnect(); // We'll print the stats right after this, keep the connection.

					delete pData;
					return 0;
				}

				// delete results
				delete pData->m_pSqlData->m_pResults;
				pData->m_pSqlData->m_pResults = 0;
			}

			// if no entry found... create a new one
			str_format(aBuf, sizeof(aBuf), "INSERT IGNORE INTO %s_savedscore(Name, JoinName, Clan, Country, JoinIP, IP, IPRange, LastJoin, Score, Kills, Deaths, FlagCaps, Wins, Losses, HammerKills, GunKills, ShotgunKills, GrenadeKills, RifleKills, NinjaKills, BotDetected) VALUES ('%s', '%s', '%s', '%d', '%s', '%s', '%s', NOW(), '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d');", pData->m_pSqlData->m_pPrefix, pData->m_aName, pData->m_aName, pData->m_aClan, pData->m_Country, pData->m_aIP, pData->m_aIP, pData->m_aIPRange, pData->m_pPlayerData->m_Score, pData->m_pPlayerData->m_Kills, pData->m_pPlayerData->m_Deaths, pData->m_pPlayerData->m_FlagCaps, pData->m_pPlayerData->m_Wins, pData->m_pPlayerData->m_Losses, pData->m_pPlayerData->m_HammerKills, pData->m_pPlayerData->m_GunKills, pData->m_pPlayerData->m_ShotgunKills, pData->m_pPlayerData->m_GrenadeKills, pData->m_pPlayerData->m_RifleKills, pData->m_pPlayerData->m_NinjaKills, pData->m_pPlayerData->m_BotDetected);
			pData->m_pSqlData->m_pStatement->execute(aBuf);

			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery("SELECT LAST_INSERT_ID();");
			if(pData->m_pSqlData->m_pResults->next())
			{
				int ID = pData->m_pSqlData->m_pResults->getInt(1);
				if(ID == 0)
					dbg_msg("SQL", "Getting playerstat failed [New]");
				else
				{
					pData->m_pPlayerData->m_ID = ID;

					str_format(aBuf, sizeof(aBuf), "INSERT IGNORE INTO %s_savedscore_daily(ID, Name, Score, Kills, Deaths, FlagCaps, Wins, Losses, HammerKills, GunKills, ShotgunKills, GrenadeKills, RifleKills, NinjaKills) VALUES ('%d', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d');", pData->m_pSqlData->m_pPrefix, pData->m_pPlayerData->m_ID, pData->m_aName, pData->m_pDailyPlayerData->m_Score, pData->m_pDailyPlayerData->m_Kills, pData->m_pDailyPlayerData->m_Deaths, pData->m_pDailyPlayerData->m_FlagCaps, pData->m_pDailyPlayerData->m_Wins, pData->m_pDailyPlayerData->m_Losses, pData->m_pDailyPlayerData->m_HammerKills, pData->m_pDailyPlayerData->m_GunKills, pData->m_pDailyPlayerData->m_ShotgunKills, pData->m_pDailyPlayerData->m_GrenadeKills, pData->m_pDailyPlayerData->m_RifleKills, pData->m_pDailyPlayerData->m_NinjaKills);
					pData->m_pSqlData->m_pStatement->execute(aBuf);

					dbg_msg("SQL", "Getting playerstat done [New]");
				}
			}

			// delete statement and results
			delete pData->m_pSqlData->m_pStatement;
			delete pData->m_pSqlData->m_pResults;
			pData->m_pSqlData->m_pStatement = 0;
			pData->m_pSqlData->m_pResults = 0;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("SQL", "ERROR: Could not get stat");
		}

		// disconnect from database
		//pData->m_pSqlData->Disconnect(); // We'll print the stats right after this, keep the connection.
	}

	delete pData;
	return 0;
}

void CSqlScore::LoadScore(int ClientID)
{
	if(m_aLoadScoreJob[ClientID].Status() != STATE_DONE)
		return;

	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_pPlayerData = PlayerData(ClientID);
	Tmp->m_pDailyPlayerData = DailyPlayerData(ClientID);
	Tmp->m_ClientID = ClientID;
	str_copy(Tmp->m_aName, Server()->ClientName(ClientID), sizeof(Tmp->m_aName));
	str_copy(Tmp->m_aClan, Server()->ClientClan(ClientID), sizeof(Tmp->m_aClan));
	Tmp->m_Country = Server()->ClientCountry(ClientID);
	Server()->GetClientAddr(ClientID, Tmp->m_aIP, sizeof(Tmp->m_aIP));
	Server()->GetClientRange(ClientID, Tmp->m_aIPRange, sizeof(Tmp->m_aIPRange));
	Tmp->m_pSqlData = this;

	GameServer()->Engine()->AddJob(&m_aLoadScoreJob[ClientID], LoadScoreThread, Tmp);
	/*void *LoadThread = thread_create(LoadScoreThread, Tmp);
	thread_detach(LoadThread);*/
}

int CSqlScore::SaveScoreThread(void *pUser)
{
	CSqlScoreData *pData = (CSqlScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			// check strings
			pData->m_pSqlData->ClearString(pData->m_aName, sizeof(pData->m_aName));
			pData->m_pSqlData->ClearString(pData->m_aClan, sizeof(pData->m_aClan));

			char aBuf[512];

			// check for ID if any
			if(pData->m_pPlayerData->m_ID > 0)
			{
				str_format(aBuf, sizeof(aBuf), "UPDATE %s_savedscore SET Name='%s', Clan='%s', Country=%d, IP='%s', IPRange='%s', LastJoin=NOW(), TimePlayed=%d, Score=%d, Kills=%d, Deaths=%d, FlagCaps=%d, Wins=%d, Losses=%d, HammerKills=%d, GunKills=%d, ShotgunKills=%d, GrenadeKills=%d, RifleKills=%d, NinjaKills=%d, BotDetected=%d WHERE ID=%d;", pData->m_pSqlData->m_pPrefix, pData->m_aName, pData->m_aClan, pData->m_Country, pData->m_aIP, pData->m_aIPRange, pData->m_pPlayerData->m_TimePlayed, pData->m_pPlayerData->m_Score, pData->m_pPlayerData->m_Kills, pData->m_pPlayerData->m_Deaths, pData->m_pPlayerData->m_FlagCaps, pData->m_pPlayerData->m_Wins, pData->m_pPlayerData->m_Losses, pData->m_pPlayerData->m_HammerKills, pData->m_pPlayerData->m_GunKills, pData->m_pPlayerData->m_ShotgunKills, pData->m_pPlayerData->m_GrenadeKills, pData->m_pPlayerData->m_RifleKills, pData->m_pPlayerData->m_NinjaKills, pData->m_pPlayerData->m_BotDetected, pData->m_pPlayerData->m_ID);
				pData->m_pSqlData->m_pStatement->execute(aBuf);

				str_format(aBuf, sizeof(aBuf), "UPDATE %s_savedscore_daily SET Name='%s', Score=%d, Kills=%d, Deaths=%d, FlagCaps=%d, Wins=%d, Losses=%d, HammerKills=%d, GunKills=%d, ShotgunKills=%d, GrenadeKills=%d, RifleKills=%d, NinjaKills=%d WHERE ID=%d;", pData->m_pSqlData->m_pPrefix, pData->m_aName, pData->m_pDailyPlayerData->m_Score, pData->m_pDailyPlayerData->m_Kills, pData->m_pDailyPlayerData->m_Deaths, pData->m_pDailyPlayerData->m_FlagCaps, pData->m_pDailyPlayerData->m_Wins, pData->m_pDailyPlayerData->m_Losses, pData->m_pDailyPlayerData->m_HammerKills, pData->m_pDailyPlayerData->m_GunKills, pData->m_pDailyPlayerData->m_ShotgunKills, pData->m_pDailyPlayerData->m_GrenadeKills, pData->m_pDailyPlayerData->m_RifleKills, pData->m_pDailyPlayerData->m_NinjaKills, pData->m_pPlayerData->m_ID);
				pData->m_pSqlData->m_pStatement->execute(aBuf);

				dbg_msg("SQL", "Updateing playerstat done [ID]");

				// delete statement
				delete pData->m_pSqlData->m_pStatement;
				pData->m_pSqlData->m_pStatement = 0;

				// disconnect from database
				pData->m_pSqlData->Disconnect();

				delete pData;
				return 0;
			}
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("SQL", "ERROR: Could not update playerstat");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}

	delete pData;
	return 0;
}

void CSqlScore::SaveScore(int ClientID)
{
	if(m_aSaveScoreJob[ClientID].Status() != STATE_DONE)
		return;

	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_pPlayerData = PlayerData(ClientID);
	Tmp->m_pDailyPlayerData = DailyPlayerData(ClientID);
	Tmp->m_ClientID = ClientID;
	str_copy(Tmp->m_aName, Server()->ClientName(ClientID), sizeof(Tmp->m_aName));
	str_copy(Tmp->m_aClan, Server()->ClientClan(ClientID), sizeof(Tmp->m_aClan));
	Tmp->m_Country = Server()->ClientCountry(ClientID);
	Server()->GetClientAddr(ClientID, Tmp->m_aIP, sizeof(Tmp->m_aIP));
	Server()->GetClientRange(ClientID, Tmp->m_aIPRange, sizeof(Tmp->m_aIPRange));
	Tmp->m_pSqlData = this;

	GameServer()->Engine()->AddJob(&m_aSaveScoreJob[ClientID], SaveScoreThread, Tmp);
	/*void *SaveThread = thread_create(SaveScoreThread, Tmp);
	thread_detach(SaveThread);*/
}

int CSqlScore::SaveScoreAllThread(void *pUser)
{
	CSqlScoreAllData *pData = (CSqlScoreAllData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(!pData->m_apPlayerSqlData[i])
					continue;

				// check strings
				pData->m_pSqlData->ClearString(pData->m_apPlayerSqlData[i]->m_aName, sizeof(pData->m_apPlayerSqlData[i]->m_aName));
				pData->m_pSqlData->ClearString(pData->m_apPlayerSqlData[i]->m_aClan, sizeof(pData->m_apPlayerSqlData[i]->m_aClan));

				char aBuf[512];

				// check for ID if any
				if(pData->m_apPlayerSqlData[i]->m_pPlayerData->m_ID > 0)
				{
					str_format(aBuf, sizeof(aBuf), "UPDATE %s_savedscore SET Name='%s', Clan='%s', Country=%d, IP='%s', IPRange='%s', TimePlayed=%d, Score=%d, Kills=%d, Deaths=%d, FlagCaps=%d, Wins=%d, Losses=%d, HammerKills=%d, GunKills=%d, ShotgunKills=%d, GrenadeKills=%d, RifleKills=%d, NinjaKills=%d, BotDetected=%d WHERE ID=%d;", pData->m_pSqlData->m_pPrefix, pData->m_apPlayerSqlData[i]->m_aName, pData->m_apPlayerSqlData[i]->m_aClan, pData->m_apPlayerSqlData[i]->m_Country, pData->m_apPlayerSqlData[i]->m_aIP, pData->m_apPlayerSqlData[i]->m_aIPRange, pData->m_apPlayerSqlData[i]->m_pPlayerData->m_TimePlayed, pData->m_apPlayerSqlData[i]->m_pPlayerData->m_Score, pData->m_apPlayerSqlData[i]->m_pPlayerData->m_Kills, pData->m_apPlayerSqlData[i]->m_pPlayerData->m_Deaths, pData->m_apPlayerSqlData[i]->m_pPlayerData->m_FlagCaps, pData->m_apPlayerSqlData[i]->m_pPlayerData->m_Wins, pData->m_apPlayerSqlData[i]->m_pPlayerData->m_Losses, pData->m_apPlayerSqlData[i]->m_pPlayerData->m_HammerKills, pData->m_apPlayerSqlData[i]->m_pPlayerData->m_GunKills, pData->m_apPlayerSqlData[i]->m_pPlayerData->m_ShotgunKills, pData->m_apPlayerSqlData[i]->m_pPlayerData->m_GrenadeKills, pData->m_apPlayerSqlData[i]->m_pPlayerData->m_RifleKills, pData->m_apPlayerSqlData[i]->m_pPlayerData->m_NinjaKills, pData->m_apPlayerSqlData[i]->m_pPlayerData->m_BotDetected, pData->m_apPlayerSqlData[i]->m_pPlayerData->m_ID);
					pData->m_pSqlData->m_pStatement->execute(aBuf);

					str_format(aBuf, sizeof(aBuf), "UPDATE %s_savedscore_daily SET Name='%s', Score=%d, Kills=%d, Deaths=%d, FlagCaps=%d, Wins=%d, Losses=%d, HammerKills=%d, GunKills=%d, ShotgunKills=%d, GrenadeKills=%d, RifleKills=%d, NinjaKills=%d WHERE ID=%d;", pData->m_pSqlData->m_pPrefix, pData->m_apPlayerSqlData[i]->m_aName, pData->m_apPlayerSqlData[i]->m_pDailyPlayerData->m_Score, pData->m_apPlayerSqlData[i]->m_pDailyPlayerData->m_Kills, pData->m_apPlayerSqlData[i]->m_pDailyPlayerData->m_Deaths, pData->m_apPlayerSqlData[i]->m_pDailyPlayerData->m_FlagCaps, pData->m_apPlayerSqlData[i]->m_pDailyPlayerData->m_Wins, pData->m_apPlayerSqlData[i]->m_pDailyPlayerData->m_Losses, pData->m_apPlayerSqlData[i]->m_pDailyPlayerData->m_HammerKills, pData->m_apPlayerSqlData[i]->m_pDailyPlayerData->m_GunKills, pData->m_apPlayerSqlData[i]->m_pDailyPlayerData->m_ShotgunKills, pData->m_apPlayerSqlData[i]->m_pDailyPlayerData->m_GrenadeKills, pData->m_apPlayerSqlData[i]->m_pDailyPlayerData->m_RifleKills, pData->m_apPlayerSqlData[i]->m_pDailyPlayerData->m_NinjaKills, pData->m_apPlayerSqlData[i]->m_pPlayerData->m_ID);
					pData->m_pSqlData->m_pStatement->execute(aBuf);

					dbg_msg("SQL", "Updateing playerstats done [ID]");
				}
			}

			// delete statement and results
			delete pData->m_pSqlData->m_pStatement;
			delete pData->m_pSqlData->m_pResults;
			pData->m_pSqlData->m_pStatement = 0;
			pData->m_pSqlData->m_pResults = 0;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("SQL", "ERROR: Could not update playerstats");
		}

		// disconnect from database
		if(!pData->m_KeepConnection)
			pData->m_pSqlData->Disconnect();
	}

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!pData->m_apPlayerSqlData[i])
			continue;

		if(pData->m_DeletePlayerData)
		{
			delete pData->m_apPlayerSqlData[i]->m_pPlayerData;
			delete pData->m_apPlayerSqlData[i]->m_pDailyPlayerData;
		}
		delete pData->m_apPlayerSqlData[i];
	}
	delete pData;
	return 0;
}

void CSqlScore::SaveScoreAll(bool Safe, bool KeepConnection)
{
	if(m_SaveScoreAllJob.Status() != STATE_DONE)
		return;

	CSqlScoreAllData *Tmp = new CSqlScoreAllData();
	Tmp->m_pSqlData = this;
	Tmp->m_KeepConnection = KeepConnection;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!GameServer()->m_apPlayers[i])
			continue;

		Tmp->m_apPlayerSqlData[i] = new CSqlScoreData();
		if(Safe)
		{
			Tmp->m_apPlayerSqlData[i]->m_pPlayerData = new CPlayerData();
			mem_copy(Tmp->m_apPlayerSqlData[i]->m_pPlayerData, PlayerData(i), sizeof(CPlayerData));
			Tmp->m_apPlayerSqlData[i]->m_pDailyPlayerData = new CPlayerData();
			mem_copy(Tmp->m_apPlayerSqlData[i]->m_pDailyPlayerData, DailyPlayerData(i), sizeof(CPlayerData));
		}
		else
		{
			Tmp->m_apPlayerSqlData[i]->m_pPlayerData = PlayerData(i);
			Tmp->m_apPlayerSqlData[i]->m_pDailyPlayerData = DailyPlayerData(i);
		}
		Tmp->m_DeletePlayerData = Safe;
		Tmp->m_apPlayerSqlData[i]->m_ClientID = i;
		str_copy(Tmp->m_apPlayerSqlData[i]->m_aName, Server()->ClientName(i), sizeof(Tmp->m_apPlayerSqlData[i]->m_aName));
		str_copy(Tmp->m_apPlayerSqlData[i]->m_aClan, Server()->ClientClan(i), sizeof(Tmp->m_apPlayerSqlData[i]->m_aClan));
		Tmp->m_apPlayerSqlData[i]->m_Country = Server()->ClientCountry(i);
		Server()->GetClientAddr(i, Tmp->m_apPlayerSqlData[i]->m_aIP, sizeof(Tmp->m_apPlayerSqlData[i]->m_aIP));
		Server()->GetClientRange(i, Tmp->m_apPlayerSqlData[i]->m_aIPRange, sizeof(Tmp->m_apPlayerSqlData[i]->m_aIPRange));
	}

	GameServer()->Engine()->AddJob(&m_SaveScoreAllJob, SaveScoreAllThread, Tmp);
	/*void *SaveAllThread = thread_create(SaveScoreAllThread, Tmp);
	thread_detach(SaveAllThread);*/
}

int CSqlScore::ShowRankThread(void *pUser)
{
	CSqlScoreData *pData = (CSqlScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			// check strings
			pData->m_pSqlData->ClearString(pData->m_aName, sizeof(pData->m_aName));

			char aBuf[512];
			// check sort methode
			if(pData->m_Daily)
				str_format(aBuf, sizeof(aBuf), "SELECT ID, Name, Score, Kills, Deaths, FlagCaps FROM %s_savedscore_daily ORDER BY `Score` DESC;", pData->m_pSqlData->m_pPrefix);
			else
				str_format(aBuf, sizeof(aBuf), "SELECT ID, Name, Score, Kills, Deaths, FlagCaps FROM %s_savedscore ORDER BY `Score` DESC;", pData->m_pSqlData->m_pPrefix);

			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

			int RowCount = 0;
			bool Found = false;
			while(pData->m_pSqlData->m_pResults->next())
			{
				RowCount++;

				if(pData->m_Search)
				{
					if(str_find_nocase(pData->m_pSqlData->m_pResults->getString("Name").c_str(), pData->m_aName))
					{
						Found = true;
						break;
					}
				}
				else if(pData->m_pSqlData->m_pResults->getInt("ID") == pData->m_pPlayerData->m_ID)
				{
					Found = true;
					break;
				}
			}

			if(!Found)
			{
				str_format(aBuf, sizeof(aBuf), "%s is not ranked", pData->m_aName);
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
			}
			else
			{
				if(pData->m_pSqlData->m_pResults->getInt("Deaths") > 0)
					str_format(aBuf, sizeof(aBuf), "%d. '%s'%s Score: %d | Kills: %d | Deaths: %d | FlagCaps: %d | K/D: %.2f", RowCount, pData->m_pSqlData->m_pResults->getString("Name").c_str(), pData->m_Daily?" todays":"", pData->m_pSqlData->m_pResults->getInt("Score"), pData->m_pSqlData->m_pResults->getInt("Kills"), pData->m_pSqlData->m_pResults->getInt("Deaths"), pData->m_pSqlData->m_pResults->getInt("FlagCaps"), (float)pData->m_pSqlData->m_pResults->getInt("Kills")/pData->m_pSqlData->m_pResults->getInt("Deaths"));
				else
					str_format(aBuf, sizeof(aBuf), "%d. '%s'%s Score: %d | Kills: %d | Deaths: %d | FlagCaps: %d | K/D: NaN", RowCount, pData->m_pSqlData->m_pResults->getString("Name").c_str(), pData->m_Daily?" todays":"", pData->m_pSqlData->m_pResults->getInt("Score"), pData->m_pSqlData->m_pResults->getInt("Kills"), pData->m_pSqlData->m_pResults->getInt("Deaths"), pData->m_pSqlData->m_pResults->getInt("FlagCaps"));
				pData->m_pSqlData->GameServer()->SendChatTarget(-1, aBuf);
			}

			dbg_msg("SQL", "Showing rank done");

			// delete results and statement
			delete pData->m_pSqlData->m_pStatement;
			delete pData->m_pSqlData->m_pResults;
			pData->m_pSqlData->m_pStatement = 0;
			pData->m_pSqlData->m_pResults = 0;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("SQL", "ERROR: Could not show rank");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}

	delete pData;
	return 0;
}

void CSqlScore::ShowRank(int ClientID, const char *pName, bool Search, bool Daily)
{
	if(m_ShowRankJob.Status() != STATE_DONE)
		return;

	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_pPlayerData = PlayerData(ClientID);
	Tmp->m_ClientID = ClientID;
	str_copy(Tmp->m_aName, pName, sizeof(Tmp->m_aName));
	Server()->GetClientAddr(ClientID, Tmp->m_aIP, sizeof(Tmp->m_aIP));
	Tmp->m_Search = Search;
	Tmp->m_Daily = Daily;
	Tmp->m_pSqlData = this;

	GameServer()->Engine()->AddJob(&m_ShowRankJob, ShowRankThread, Tmp);
	/*void *RankThread = thread_create(ShowRankThread, Tmp);
	thread_detach(RankThread);*/
}

int CSqlScore::ShowStatsThread(void *pUser)
{
	CSqlScoreData *pData = (CSqlScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			// check strings
			pData->m_pSqlData->ClearString(pData->m_aName, sizeof(pData->m_aName));

			char aBuf[512];
			str_format(aBuf, sizeof(aBuf), "SELECT ID, Name, Clan, FirstJoin, TimePlayed, Score, Kills, Deaths, FlagCaps, Wins, Losses, HammerKills, GunKills, ShotgunKills, GrenadeKills, RifleKills, NinjaKills FROM %s_savedscore ORDER BY `Score` DESC;", pData->m_pSqlData->m_pPrefix);

			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

			int RowCount = 0;
			bool Found = false;
			while(pData->m_pSqlData->m_pResults->next())
			{
				RowCount++;

				if(pData->m_Search)
				{
					if(str_find_nocase(pData->m_pSqlData->m_pResults->getString("Name").c_str(), pData->m_aName))
					{
						Found = true;
						break;
					}
				}
				else if(pData->m_pSqlData->m_pResults->getInt("ID") == pData->m_pPlayerData->m_ID)
				{
					Found = true;
					break;
				}
			}

			if(!Found)
			{
				str_format(aBuf, sizeof(aBuf), "%s is not ranked", pData->m_aName);
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
			}
			else
			{
				if(pData->m_pSqlData->m_pResults->getInt("Deaths") > 0)
					str_format(aBuf, sizeof(aBuf), "First join: %s\nRank: %d\nTime played: %d hours and %d minutes\nScore: %d\nKills: %d\nDeaths: %d\nKills/Deaths: %.2f\nFlag Captures: %d\nWins: %d\nLosses: %d\nHammer kills: %d\nGun kills: %d\nShotgun kills: %d\nGrenade kills: %d\nRifle kills: %d\nKatana kills: %d", pData->m_pSqlData->m_pResults->getString("FirstJoin").c_str(), RowCount, pData->m_pSqlData->m_pResults->getInt("TimePlayed")/60, pData->m_pSqlData->m_pResults->getInt("TimePlayed")%60, pData->m_pSqlData->m_pResults->getInt("Score"), pData->m_pSqlData->m_pResults->getInt("Kills"), pData->m_pSqlData->m_pResults->getInt("Deaths"), (float)pData->m_pSqlData->m_pResults->getInt("Kills")/pData->m_pSqlData->m_pResults->getInt("Deaths"), pData->m_pSqlData->m_pResults->getInt("FlagCaps"), pData->m_pSqlData->m_pResults->getInt("Wins"), pData->m_pSqlData->m_pResults->getInt("Losses"), pData->m_pSqlData->m_pResults->getInt("HammerKills"), pData->m_pSqlData->m_pResults->getInt("GunKills"), pData->m_pSqlData->m_pResults->getInt("ShotgunKills"), pData->m_pSqlData->m_pResults->getInt("GrenadeKills"), pData->m_pSqlData->m_pResults->getInt("RifleKills"), pData->m_pSqlData->m_pResults->getInt("NinjaKills"));
				else
					str_format(aBuf, sizeof(aBuf), "First join: %s\nRank: %d\nTime played: %d hours and %d minutes\nScore: %d\nKills: %d\nDeaths: %d\nKills/Deaths: NaN\nFlag Captures: %d\nWins: %d\nLosses: %d\nHammer kills: %d\nGun kills: %d\nShotgun kills: %d\nGrenade kills: %d\nRifle kills: %d\nKatana kills: %d", pData->m_pSqlData->m_pResults->getString("FirstJoin").c_str(), RowCount, pData->m_pSqlData->m_pResults->getInt("TimePlayed")/60, pData->m_pSqlData->m_pResults->getInt("TimePlayed")%60, pData->m_pSqlData->m_pResults->getInt("Score"), pData->m_pSqlData->m_pResults->getInt("Kills"), pData->m_pSqlData->m_pResults->getInt("Deaths"), pData->m_pSqlData->m_pResults->getInt("FlagCaps"), pData->m_pSqlData->m_pResults->getInt("Wins"), pData->m_pSqlData->m_pResults->getInt("Losses"), pData->m_pSqlData->m_pResults->getInt("HammerKills"), pData->m_pSqlData->m_pResults->getInt("GunKills"), pData->m_pSqlData->m_pResults->getInt("ShotgunKills"), pData->m_pSqlData->m_pResults->getInt("GrenadeKills"), pData->m_pSqlData->m_pResults->getInt("RifleKills"), pData->m_pSqlData->m_pResults->getInt("NinjaKills"));
				pData->m_pSqlData->GameServer()->SendMotd(pData->m_ClientID, aBuf);
			}

			dbg_msg("SQL", "Showing playerstats done");

			// delete results and statement
			delete pData->m_pSqlData->m_pStatement;
			delete pData->m_pSqlData->m_pResults;
			pData->m_pSqlData->m_pStatement = 0;
			pData->m_pSqlData->m_pResults = 0;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("SQL", "ERROR: Could not show playerstats");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}

	delete pData;
	return 0;
}

void CSqlScore::ShowStats(int ClientID, const char *pName, bool Search)
{
	if(m_ShowStatsJob.Status() != STATE_DONE)
		return;

	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_pPlayerData = PlayerData(ClientID);
	Tmp->m_ClientID = ClientID;
	str_copy(Tmp->m_aName, pName, sizeof(Tmp->m_aName));
	Server()->GetClientAddr(ClientID, Tmp->m_aIP, sizeof(Tmp->m_aIP));
	Tmp->m_Search = Search;
	Tmp->m_pSqlData = this;

	GameServer()->Engine()->AddJob(&m_ShowStatsJob, ShowStatsThread, Tmp);
	/*void *RankThread = thread_create(ShowStatsThread, Tmp);
	thread_detach(RankThread);*/
}

int CSqlScore::ShowTop5Thread(void *pUser)
{
	CSqlScoreData *pData = (CSqlScoreData *)pUser;

	// Connect to database
	if(pData->m_pSqlData->Connect())
	{
		try
		{
			char aBuf[512];
			// check sort methode
			if(pData->m_Daily)
			{
				str_format(aBuf, sizeof(aBuf), "SELECT Name, Score, Kills, Deaths, FlagCaps FROM %s_savedscore_daily ORDER BY `Score` DESC LIMIT %d, 5;", pData->m_pSqlData->m_pPrefix, pData->m_Num-1);

				// show top5
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "----------- Daily Top 5 -----------");
			}
			else
			{
				str_format(aBuf, sizeof(aBuf), "SELECT Name, Score, Kills, Deaths, FlagCaps FROM %s_savedscore ORDER BY `Score` DESC LIMIT %d, 5;", pData->m_pSqlData->m_pPrefix, pData->m_Num-1);

				// show top5
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "----------- Top 5 -----------");
			}
			pData->m_pSqlData->m_pResults = pData->m_pSqlData->m_pStatement->executeQuery(aBuf);

			int Rank = pData->m_Num;
			while(pData->m_pSqlData->m_pResults->next())
			{
				if(pData->m_pSqlData->m_pResults->getInt("Deaths") > 0)
					str_format(aBuf, sizeof(aBuf), "%d. '%s' Score: %d | Kills: %d | Deaths: %d | FlagCaps: %d | K/D: %.2f", Rank, pData->m_pSqlData->m_pResults->getString("Name").c_str(), pData->m_pSqlData->m_pResults->getInt("Score"), pData->m_pSqlData->m_pResults->getInt("Kills"), pData->m_pSqlData->m_pResults->getInt("Deaths"), pData->m_pSqlData->m_pResults->getInt("FlagCaps"), (float)pData->m_pSqlData->m_pResults->getInt("Kills")/pData->m_pSqlData->m_pResults->getInt("Deaths"));
				else
					str_format(aBuf, sizeof(aBuf), "%d. '%s' Score: %d | Kills: %d | Deaths: %d | FlagCaps: %d | K/D: NaN", Rank, pData->m_pSqlData->m_pResults->getString("Name").c_str(), pData->m_pSqlData->m_pResults->getInt("Score"), pData->m_pSqlData->m_pResults->getInt("Kills"), pData->m_pSqlData->m_pResults->getInt("Deaths"), pData->m_pSqlData->m_pResults->getInt("FlagCaps"));
				pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, aBuf);
				Rank++;
			}
			pData->m_pSqlData->GameServer()->SendChatTarget(pData->m_ClientID, "------------------------------");

			dbg_msg("SQL", "Showing top5 done");

			// delete results and statement
			delete pData->m_pSqlData->m_pStatement;
			delete pData->m_pSqlData->m_pResults;
			pData->m_pSqlData->m_pStatement = 0;
			pData->m_pSqlData->m_pResults = 0;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("SQL", "ERROR: Could not show top5");
		}

		// disconnect from database
		pData->m_pSqlData->Disconnect();
	}

	delete pData;
	return 0;
}

void CSqlScore::ShowTop5(int ClientID, int Debut, bool Daily)
{
	if(m_ShowTop5Job.Status() != STATE_DONE)
		return;

	CSqlScoreData *Tmp = new CSqlScoreData();
	Tmp->m_ClientID = ClientID;
	Tmp->m_pPlayerData = PlayerData(ClientID);
	Tmp->m_Num = Debut;
	Tmp->m_Daily = Daily;
	Tmp->m_pSqlData = this;

	GameServer()->Engine()->AddJob(&m_ShowTop5Job, ShowTop5Thread, Tmp);
	/*void *Top5Thread = thread_create(ShowTop5Thread, Tmp);
	thread_detach(Top5Thread);*/
}

// anti SQL injection
void CSqlScore::ClearString(char *pString, int Size)
{
	// check if the string is long enough to escape something
	if(Size <= 2)
	{
		if(pString[0] == '\'' || pString[0] == '\\' || pString[0] == ';')
			pString[0] = '_';
		return;
	}

	// replace ' ' ' with ' \' ' and remove '\'
	for(int i = 0; i < str_length(pString); i++)
	{
		// replace '--' with '__'
		if(pString[i] == '-' && pString[i+1] == '-')
		{
			pString[i] = '_';
			pString[++i] = '_';
			continue;
		}

		// escape ', \ and ;
		if(pString[i] == '\'' || pString[i] == '\\' || pString[i] == ';')
		{
			for(int j = Size-2; j > i; j--)
				pString[j] = pString[j-1];
			pString[i] = '\\';
			i++; // so we don't double escape
			continue;
		}
	}

	// aaand remove spaces and \ at the end xD
	for(int i = str_length(pString)-1; i >= 0; i--)
	{
		if(pString[i] == ' ' || pString[i] == '\\')
			pString[i] = 0;
		else
			break;
	}
}
