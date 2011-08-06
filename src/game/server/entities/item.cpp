/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "item.h"

CItem::CItem(CGameWorld *pGameWorld, int Item, int Type)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Type = POWERUP_WEAPON;
	m_Subtype = Type;
	m_Item = Item;
	m_Enabled = true;

	m_ProximityRadius = 14;

	GameWorld()->InsertEntity(this);
}

void CItem::Reset()
{
	m_Enabled = true;
}

int CItem::GetLevel()
{
	switch(m_Item)
	{
	case HITEM_HAMMER:
		return 10;
	case HITEM_GUN:
		return 3;
	case HITEM_SHOTGUN:
		return 5;
	case HITEM_GRENADE:
		return 3;
	case HITEM_RIFLE:
		return 5;
	case ZITEM_HAMMER:
		return 5;
	}
	return 0;
}

void CItem::Tick()
{
	if(!m_Enabled)
		return;

	// Check if a player intersected us
	CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, 20.0f, 0);
	if(pChr && pChr->IsAlive() && !pChr->m_Item && pChr->GetPlayer())
	{
		if(pChr->GetPlayer()->GetTeam() == TEAM_BLUE && pChr->GetPlayer()->m_Score/10 >= GetLevel() && m_Item != ZITEM_HAMMER)
		{
			pChr->m_Item = m_Item;
			if(m_Item == HITEM_HAMMER)
			{
				pChr->m_HookedItem = this;
				m_Enabled = false;
				pChr->IncreaseArmor(5);
			}
			else
				pChr->GiveWeapon(m_Subtype, 10);
			if(m_Subtype == WEAPON_GRENADE)
				GameServer()->CreateSound(m_Pos, SOUND_PICKUP_GRENADE);
			else if(m_Subtype == WEAPON_SHOTGUN || m_Subtype == WEAPON_GUN)
				GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);
			else if(m_Subtype == WEAPON_RIFLE)
				GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);

			GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), m_Subtype);

			pChr->SetEmote(EMOTE_SURPRISE, Server()->Tick() + 1200 * Server()->TickSpeed() / 1000);
		}
		else if(pChr->GetPlayer()->GetTeam() == TEAM_RED && pChr->GetPlayer()->m_Score/100 >= GetLevel() && m_Item == ZITEM_HAMMER)
		{
			pChr->m_Item = m_Item;
			GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SWITCH);
			pChr->SetEmote(EMOTE_ANGRY, Server()->Tick() + 1200 * Server()->TickSpeed() / 1000);
		}
	}
}

void CItem::Snap(int SnappingClient)
{
	if(!m_Enabled || NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = m_Type;
	pP->m_Subtype = m_Subtype;
}
