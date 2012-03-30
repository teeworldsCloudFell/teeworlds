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
		return 3;
	case HITEM_GUN:
		return 1;
	case HITEM_SHOTGUN:
		return 2;
	case HITEM_GRENADE:
		return 2;
	case HITEM_RIFLE:
		return 1;
	case ZITEM_HAMMER:
		return 2;
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
				pChr->IncreaseArmor(10);
			}
			else
				pChr->GiveWeapon(m_Subtype, 10);
			if(m_Subtype == WEAPON_GRENADE)
				GameServer()->CreateSound(m_Pos, SOUND_PICKUP_GRENADE);
			else if(m_Subtype == WEAPON_SHOTGUN || m_Subtype == WEAPON_GUN)
				GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);
			else if(m_Subtype == WEAPON_RIFLE)
				GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);

			char aBuf[128];
			switch(m_Item)
			{
			case HITEM_HAMMER:
				str_copy(aBuf, "Special item hammer: You can build 10 zdoors with it. Use with care!", sizeof(aBuf));
				break;
			case HITEM_GUN:
				str_copy(aBuf, "Gun upgraded. +Knockback -Regentime", sizeof(aBuf));
				break;
			case HITEM_SHOTGUN:
				str_copy(aBuf, "Shotgun upgraded. +Knockback", sizeof(aBuf));
				break;
			case HITEM_GRENADE:
				str_copy(aBuf, "Grenade upgraded. +Knockback +Burntime", sizeof(aBuf));
				break;
			case HITEM_RIFLE:
				str_copy(aBuf, "Rifle upgraded. +Freezetime", sizeof(aBuf));
				break;
			}

			GameServer()->SendBroadcast(aBuf, pChr->GetPlayer()->GetCID());
			GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), m_Subtype);
			pChr->SetEmote(EMOTE_SURPRISE, Server()->Tick() + 1200 * Server()->TickSpeed() / 1000);
		}
		else if(pChr->GetPlayer()->GetTeam() == TEAM_BLUE && m_Item != ZITEM_HAMMER)
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "You need to be at least level %d for this item!", GetLevel());
			GameServer()->SendBroadcast(aBuf, pChr->GetPlayer()->GetCID());
		}
		else if(pChr->GetPlayer()->GetTeam() == TEAM_BLUE && m_Item == ZITEM_HAMMER)
			GameServer()->SendBroadcast("This is a zombie item, you can't pick it up.", pChr->GetPlayer()->GetCID());
		else if(pChr->GetPlayer()->GetTeam() == TEAM_RED && m_Item != ZITEM_HAMMER)
			GameServer()->SendBroadcast("This is a human item, you can't pick it up.", pChr->GetPlayer()->GetCID());
		else if(pChr->GetPlayer()->GetTeam() == TEAM_RED && pChr->GetPlayer()->m_Score/10 >= GetLevel() && m_Item == ZITEM_HAMMER)
		{
			pChr->m_Item = m_Item;
			GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SWITCH);
			pChr->SetEmote(EMOTE_ANGRY, Server()->Tick() + 1200 * Server()->TickSpeed() / 1000);
			GameServer()->SendBroadcast("Special item hammer: You have less knockback, burntime and freeze time.", pChr->GetPlayer()->GetCID());
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
