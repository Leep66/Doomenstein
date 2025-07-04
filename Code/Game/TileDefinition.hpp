#pragma once
#include <string>
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <vector>



struct TileDefinition
{
	std::string m_name;
	bool m_isSolid = false;
	Rgba8 m_mapImagePixelColor;
	IntVec2 m_floorSpriteCoords;
	IntVec2 m_ceilingSpriteCoords;
	IntVec2 m_wallSpriteCoords;

	static std::vector<TileDefinition> s_tileDefinitions;

	static void SetTileDefinition(std::string name, bool isSolid, Rgba8 mapImagePixelColor, 
		IntVec2 floorSpriteCoords, IntVec2 ceilingSpriteCoords, IntVec2 wallSpriteCoords);
	static void InitializeTileDefinitions(const char* path);



	static TileDefinition const& GetDefinition(std::string const& name);
	static const TileDefinition* GetDefinitionForColor(const Rgba8& color);

};
