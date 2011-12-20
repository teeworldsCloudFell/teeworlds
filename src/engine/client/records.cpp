/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>

#include <engine/config.h>
#include <engine/console.h>
#include <engine/shared/config.h>

#include "records.h"

CRecords::CRecords()
{
	m_lRecords.clear();
}

void CRecords::ConAddRecord(IConsole::IResult *pResult, void *pUserData)
{
	CRecords *pSelf = (CRecords *)pUserData;
	pSelf->AddRecord(pResult->GetString(0), pResult->GetInteger(1));
}

void CRecords::ConRemoveRecord(IConsole::IResult *pResult, void *pUserData)
{
	CRecords *pSelf = (CRecords *)pUserData;
	pSelf->RemoveRecord(pResult->GetString(0));
}

void CRecords::Init()
{
	IConfig *pConfig = Kernel()->RequestInterface<IConfig>();
	if(pConfig)
		pConfig->RegisterCallback(ConfigSaveCallback, this);

	IConsole *pConsole = Kernel()->RequestInterface<IConsole>();
	if(pConsole)
	{
		pConsole->Register("add_record", "si", CFGFLAG_CLIENT, ConAddRecord, this, "Add a record");
		pConsole->Register("remove_record", "s", CFGFLAG_CLIENT, ConRemoveRecord, this, "Remove a record");
	}
}

const CRecordInfo *CRecords::GetRecord(int Index) const
{
	return &m_lRecords[max(0, Index%NumRecords())];
}

void CRecords::GetRank(const char *pName, int *pScore, int *pRank)
{
	int Score = 0;
	unsigned NameHash = str_quickhash(pName);
	for(int i = 0; i < NumRecords(); ++i)
	{
		if(m_lRecords[i].m_NameHash == NameHash)
		{
			if(pScore)
				*pScore = m_lRecords[i].m_Score;
			if(pRank)
				*pRank = i+1;
			return;
		}
	}
}

void CRecords::AddRecord(const char *pName, int Score)
{
	if(pName[0] == 0)
		return;

	// change the record if we already have it
	unsigned NameHash = str_quickhash(pName);
	for(int i = 0; i < NumRecords(); ++i)
	{
		if(m_lRecords[i].m_NameHash == NameHash)
		{
			m_lRecords[i].m_Score = Score;
			m_lRecords.sort_range();
			return;
		}
	}

	CRecordInfo Add;
	str_copy(Add.m_aName, pName, sizeof(Add.m_aName));
	Add.m_Score = Score;
	Add.m_NameHash = NameHash;
	m_lRecords.add(Add);
}

void CRecords::RemoveRecord(const char *pName)
{
	unsigned NameHash = str_quickhash(pName);
	for(int i = 0; i < NumRecords(); ++i)
	{
		if(m_lRecords[i].m_NameHash == NameHash)
		{
			RemoveRecord(i);
			return;
		}
	}
}

void CRecords::RemoveRecord(int Index)
{
	m_lRecords.remove_index(Index);
}

void CRecords::ConfigSaveCallback(IConfig *pConfig, void *pUserData)
{
	CRecords *pSelf = (CRecords *)pUserData;
	char aBuf[128];
	char aIntBuf[16];
	const char *pEnd = aBuf+sizeof(aBuf)-4;
	for(int i = 0; i < pSelf->NumRecords(); ++i)
	{
		str_copy(aBuf, "add_record ", sizeof(aBuf));

		const char *pSrc = pSelf->m_lRecords[i].m_aName;
		char *pDst = aBuf+str_length(aBuf);
		*pDst++ = '"';
		while(*pSrc && pDst < pEnd)
		{
			if(*pSrc == '"' || *pSrc == '\\') // escape \ and "
				*pDst++ = '\\';
			*pDst++ = *pSrc++;
		}
		*pDst++ = '"';
		*pDst++ = ' ';

		str_format(aIntBuf, sizeof(aIntBuf), "%d", pSelf->m_lRecords[i].m_Score);
		pSrc = aIntBuf;

		while(*pSrc && pDst < pEnd)
			*pDst++ = *pSrc++;

		*pDst++ = 0;

		pConfig->WriteLine(aBuf);
	}
}
