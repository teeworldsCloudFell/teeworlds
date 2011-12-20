/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_RECORDS_H
#define ENGINE_RECORDS_H

#include <engine/shared/protocol.h>
#include <base/tl/sorted_array.h>

#include "kernel.h"

struct CRecordInfo
{
	char m_aName[MAX_NAME_LENGTH];
	int m_Score;
	unsigned m_NameHash;

	bool operator<(const CRecordInfo& other) { return (this->m_Score > other.m_Score); }
};

class IRecords : public IInterface
{
	MACRO_INTERFACE("records", 0)
public:

	virtual void Init() = 0;

	virtual int NumRecords() const = 0;
	virtual const CRecordInfo *GetRecord(int Index) const = 0;
	virtual void GetRank(const char *pName, int *pScore, int *pRank) = 0;

	virtual void AddRecord(const char *pName, int Score) = 0;
	virtual void RemoveRecord(const char *pName) = 0;
};

#endif
