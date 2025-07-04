#include "Game/MapDefinition.hpp"
#include "Engine/Core/EngineCommon.hpp"


std::vector<MapDefinition> MapDefinition::s_mapDefinitions;

extern Renderer* g_theRenderer;

void MapDefinition::SetMapDefinition(std::string name, Image image, Shader* shader, Texture* texture, IntVec2 cellCount)
{
	MapDefinition mapDef;
	mapDef.m_name = name;
	mapDef.m_image = image;
	mapDef.m_shader = shader;
	mapDef.m_spriteSheetTexture = texture;
	mapDef.m_spriteSheetCellCount = cellCount;

	s_mapDefinitions.push_back(mapDef);
}

void MapDefinition::InitializeMapDefinitions(const char* path)
{
	XmlDocument doc;
	XmlResult result = doc.LoadFile(path);
	if (result != tinyxml2::XML_SUCCESS)
	{
		ERROR_AND_DIE("Failed to load " + std::string(path));
	}

	XmlElement* root = doc.RootElement();
	if (root == nullptr)
	{
		ERROR_AND_DIE(std::string(path) + " is missing a root element");
	}

	XmlElement const* mapDefElem = root->FirstChildElement();
	while (mapDefElem)	
	{
		std::string name = ParseXmlAttribute(*mapDefElem, "name", "Invalid Name");
		std::string imagePath = ParseXmlAttribute(*mapDefElem, "image", "Invalid Image Path");
		std::string shaderPath = ParseXmlAttribute(*mapDefElem, "shader", "Invalid Shader Name");
		std::string texturePath = ParseXmlAttribute(*mapDefElem, "spriteSheetTexture", "Invalid Texture Path");
		IntVec2 cellCount = ParseXmlAttribute(*mapDefElem, "spriteSheetCellCount", IntVec2(-1, -1));
		std::string skyPath = ParseXmlAttribute(*mapDefElem, "skyboxTexture", "Invalid");

		Texture* skyTex = g_theRenderer->CreateOrGetTextureFromFile(skyPath.c_str());
		if (!skyTex)
		{
			ERROR_AND_DIE("Failed to load texture from: " + skyPath);
		}

		Image image(imagePath.c_str());

		Shader* shader = g_theRenderer->CreateShader(shaderPath.c_str(), VertexType::Vertex_PCUTBN);
		if (!shader)
		{
			ERROR_AND_DIE("Failed to create shader: " + shaderPath);
		}

		Texture* texture = g_theRenderer->CreateOrGetTextureFromFile(texturePath.c_str());
		if (!texture)
		{
			ERROR_AND_DIE("Failed to load texture from: " + texturePath);
		}

		MapDefinition mapDef;
		mapDef.m_name = name;
		mapDef.m_image = image;
		mapDef.m_shader = shader;
		mapDef.m_spriteSheetTexture = texture;
		mapDef.m_spriteSheetCellCount = cellCount;
		mapDef.m_skyboxTex = skyTex;

		XmlElement const* spawnInfosElem = mapDefElem->FirstChildElement("SpawnInfos");

		if (spawnInfosElem)
		{
			XmlElement const* spawnInfoElem = spawnInfosElem->FirstChildElement("SpawnInfo");
			while (spawnInfoElem)
			{
				std::string actorName = ParseXmlAttribute(*spawnInfoElem, "actor", "Invalid Actor Name");
				Vec3 position = ParseXmlAttribute(*spawnInfoElem, "position", Vec3::ZERO);
				EulerAngles orientation = ParseXmlAttribute(*spawnInfoElem, "orientation", EulerAngles());
				Vec3 velocity = ParseXmlAttribute(*spawnInfoElem, "velocity", Vec3::ZERO);

				SpawnInfo spawnInfo;
				spawnInfo.m_actorName = actorName;
				spawnInfo.m_actorPos = position;
				spawnInfo.m_actorOri = orientation;
				spawnInfo.m_actorVel = velocity;

				mapDef.m_spawnInfos.push_back(spawnInfo);
				spawnInfoElem = spawnInfoElem->NextSiblingElement("SpawnInfo");
			}
		}

		XmlElement const* wavesElem = mapDefElem->FirstChildElement("Waves");
		if (wavesElem)
		{

			XmlElement const* waveElem = wavesElem->FirstChildElement("Wave");
			while (waveElem)
			{
				Wave wave;

				XmlElement const* waveActorElem = waveElem->FirstChildElement("Actor");
				while (waveActorElem)
				{
					std::string waname = ParseXmlAttribute(*waveActorElem, "name", "");
					int num = ParseXmlAttribute(*waveActorElem, "num", 0);

					WaveActor waveActor;
					waveActor.m_name = waname;
					waveActor.m_num = num;

					wave.m_waveActors.push_back(waveActor);
					waveActorElem = waveActorElem->NextSiblingElement("Actor");
				}

				mapDef.m_waves.push_back(wave);
				waveElem = waveElem->NextSiblingElement("Wave");
			}
		}

		s_mapDefinitions.push_back(mapDef);

		mapDefElem = mapDefElem->NextSiblingElement("MapDefinition");
	}

	
}


void MapDefinition::ClearDefinitions()
{
	s_mapDefinitions.clear();
}

MapDefinition const& MapDefinition::GetDefinition(int mapDefIndex)
{
	return s_mapDefinitions[mapDefIndex];
}

MapDefinition const& MapDefinition::GetDefinition(std::string mapName)
{
	for (MapDefinition const& def : s_mapDefinitions)
	{
		if (def.m_name == mapName)
		{
			return def;
		}
	}
	ERROR_AND_DIE("Invalid Map Definition!");
	/*static MapDefinition invalidDef;
	return invalidDef;*/
}
