/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_RECORDS_H
#define ENGINE_CLIENT_RECORDS_H

#include <engine/records.h>

class CRecords : public IRecords
{
	sorted_array<CRecordInfo> m_lRecords;

	static void ConAddRecord(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveRecord(IConsole::IResult *pResult, void *pUserData);

	static void ConfigSaveCallback(IConfig *pConfig, void *pUserData);

public:
	CRecords();

	void Init();

	inline int NumRecords() const { return m_lRecords.size(); }
	const CRecordInfo *GetRecord(int Index) const;
	void GetRank(const char *pName, int *pScore, int *pRank);

	void AddRecord(const char *pName, int Score);
	void RemoveRecord(const char *pName);
	void RemoveRecord(int Index);
};

#endif
