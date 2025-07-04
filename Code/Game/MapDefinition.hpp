#pragma once

#include <string>
#include <vector>
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Math/EulerAngles.hpp"

struct SpawnInfo
{
	std::string m_actorName;
	Vec3 m_actorPos = Vec3::ZERO;
	EulerAngles m_actorOri = EulerAngles();
	Vec3 m_actorVel = Vec3::ZERO;

	SpawnInfo() = default;
	SpawnInfo(std::string name, Vec3 pos, EulerAngles angles, Vec3 velocity)
		: m_actorName(name), m_actorPos(pos), m_actorOri(angles), m_actorVel(velocity)
	{ }


};

struct WaveActor
{
	std::string m_name = "";
	int m_num = 0;
};

struct Wave
{
	std::vector<WaveActor> m_waveActors;
};



struct MapDefinition
{
	std::string m_name = "Invalid Map Name";
	Image m_image;
	Shader* m_shader = nullptr;
	Texture* m_spriteSheetTexture = nullptr;
	IntVec2 m_spriteSheetCellCount = IntVec2(-1, -1);
	Texture* m_skyboxTex = nullptr;
	std::vector<Wave> m_waves;

	std::vector<SpawnInfo> m_spawnInfos;
	static void SetMapDefinition(std::string name, Image image,
		Shader* shader, Texture* texture, IntVec2 cellCount);

	static void InitializeMapDefinitions(const char* path);
	static void ClearDefinitions();

	static MapDefinition const& GetDefinition(int mapDefIndex);
	static MapDefinition const& GetDefinition(std::string mapName);
	static std::vector<MapDefinition> s_mapDefinitions;

};