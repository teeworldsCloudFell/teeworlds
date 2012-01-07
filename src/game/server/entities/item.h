/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_ITEM_H
#define GAME_SERVER_ENTITIES_ITEM_H

#include <game/server/entity.h>

enum
{
	ZITEM_HAMMER = 1,
	HITEM_HAMMER,
	HITEM_GUN,
	HITEM_SHOTGUN,
	HITEM_GRENADE,
	HITEM_RIFLE
};

class CItem : public CEntity
{
public:
	CItem(CGameWorld *pGameWorld, int Type, int RealType);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
	int GetLevel();

private:
	bool m_Enabled;
	int m_Item;
	int m_Type;
	int m_Subtype;
};

#endif
