/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>

class CCollision
{
	class CTile *m_pTiles;
	int m_Width;
	int m_Height;
	class CLayers *m_pLayers;

	bool IsTileSolid(int x, int y);
	int GetTile(int x, int y);

public:
	enum
	{
		COLFLAG_SOLID=1,
		COLFLAG_DEATH=2,
		COLFLAG_NOHOOK=16, // Ugly. Couldn't be arsed to rewrite.
	};

	CCollision();
	void Init(class CLayers *pLayers);
	bool CheckPoint(float x, float y) { return IsTileSolid(round_to_int(x), round_to_int(y)); }
	bool CheckPoint(vec2 Pos) { return CheckPoint(Pos.x, Pos.y); }
	int GetCollisionAt(float x, float y) { return GetTile(round_to_int(x), round_to_int(y)); }
	int GetWidth() { return m_Width; };
	int GetHeight() { return m_Height; };
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	int IntersectLineAir(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	int IntersectLineWater(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	void SetTile(int x, int y, int Index);
	bool DoorBlock(vec2 Pos0, vec2 Pos1);
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces);
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity);
	void MovePoint_(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces, bool InWater);
	bool CheckPoint_(int x, int y, int InWater);
	bool TestBox(vec2 Pos, vec2 Size);
	
	// race
	int GetIndex(vec2 Pos);
	int GetIndex(vec2 PrevPos, vec2 Pos);
	vec2 GetPos(int Index);
	int GetTileIndex(int Index);
	int IsTeleport(int Index);
	int IsSpeedup(int Index);
	void GetSpeedup(int Index, vec2 *Dir, int *Force);
	int IsDoor(int x, int y);
	int GetSwitchNum(vec2 Pos);
	int GetSwitchTeam(int x, int y);
	
	class CTeleTile *m_pTele;
	class CSpeedupTile *m_pSpeedup;
	class CSwitchTile *m_pSwitch;

	class CSwitchTile *Switches() { return m_pSwitch; }
	class CLayers *Layers() { return m_pLayers; }
};

#endif
