#pragma once
#include "Engine/Math/AABB3.hpp"
#include "Game/TileDefinition.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"

class Tile
{
public:
	Tile() = default;
	Tile(const AABB3& bounds, const TileDefinition* tileDef = nullptr) 
		: m_bounds(bounds)
		, m_tileDefinition(tileDef) 
	{};

	const AABB3& GetBounds() const { return m_bounds; }

	const AABB2 GetFloorBounds() const { return AABB2(Vec2(m_bounds.m_mins.x, m_bounds.m_mins.y), Vec2(m_bounds.m_maxs.x, m_bounds.m_maxs.y)); }

	const TileDefinition* GetTileDefinition() const { return m_tileDefinition; }

private:
	AABB3 m_bounds;

	const TileDefinition* m_tileDefinition = nullptr;
};