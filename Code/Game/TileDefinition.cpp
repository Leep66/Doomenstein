#include "Game/TileDefinition.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Core/EngineCommon.hpp"

std::vector<TileDefinition> TileDefinition::s_tileDefinitions;

void TileDefinition::SetTileDefinition(std::string name, bool isSolid, Rgba8 mapImagePixelColor, IntVec2 floorSpriteCoords, IntVec2 ceilingSpriteCoords, IntVec2 wallSpriteCoords)
{
	TileDefinition tileDef;
	tileDef.m_name = name;
	tileDef.m_isSolid = isSolid;
	tileDef.m_mapImagePixelColor = mapImagePixelColor;
	tileDef.m_floorSpriteCoords = floorSpriteCoords;
	tileDef.m_ceilingSpriteCoords = ceilingSpriteCoords;
	tileDef.m_wallSpriteCoords = wallSpriteCoords;

	s_tileDefinitions.push_back(tileDef);
}

void TileDefinition::InitializeTileDefinitions(const char* path)
{
	XmlDocument doc;
	XmlResult result = doc.LoadFile(path);
	if (result != tinyxml2::XML_SUCCESS)
	{
		ERROR_AND_DIE("Failed to load Data/TileDefinition.xml");
	}

	XmlElement* root = doc.RootElement();
	if (root == nullptr)
	{
		ERROR_AND_DIE("TileDefinition.xml is missing a root element");
	}

	XmlElement* tileElem = root->FirstChildElement();

	while (tileElem)
	{
		std::string name = ParseXmlAttribute(*tileElem, "name", "Invalid Tile Name");
		bool isSolid = ParseXmlAttribute(*tileElem, "isSolid", false);
		Rgba8 mapImagePixelColor = ParseXmlAttribute(*tileElem, "mapImagePixelColor", Rgba8::RED);
		IntVec2 floorSpriteCoords = ParseXmlAttribute(*tileElem, "floorSpriteCoords", IntVec2(-1, -1));
		IntVec2 ceilingSpriteCoords = ParseXmlAttribute(*tileElem, "ceilingSpriteCoords", IntVec2(-1, -1));
		IntVec2 wallSpriteCoords = ParseXmlAttribute(*tileElem, "wallSpriteCoords", IntVec2(-1, -1));

		SetTileDefinition(name, isSolid, mapImagePixelColor, floorSpriteCoords, ceilingSpriteCoords, wallSpriteCoords);

		tileElem = tileElem->NextSiblingElement();
	}

}

TileDefinition const& TileDefinition::GetDefinition(std::string const& name)
{
	for (TileDefinition const& def : s_tileDefinitions) {
		if (def.m_name == name) {
			return def;
		}
	}
	static TileDefinition invalidDef;
	return invalidDef;
}

const TileDefinition* TileDefinition::GetDefinitionForColor(const Rgba8& color)
{
	for (const TileDefinition& def : s_tileDefinitions)
	{
		if (def.m_mapImagePixelColor == color)
		{
			return &def;
		}
	}
	return nullptr;
}
