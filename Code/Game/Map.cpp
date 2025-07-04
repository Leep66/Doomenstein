#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/MapDefinition.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Game/Tile.hpp"
#include "Game/Actor.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/ZCylinder3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Core/Timer.hpp"
#include "Game/PlayerController.hpp"
#include "Game/AIController.hpp"
#include "Engine/Core/Vertex_PCU.hpp"

extern Renderer* g_theRenderer;
extern InputSystem* g_theInput;


Map::Map(Game* game, const MapDefinition* definition, double physicsTimeStep)
	: m_game(game)
	, m_definition(definition)
	, m_physicsTimer(new Timer((float)physicsTimeStep, game->m_clock))
{

	if (!m_definition)
	{
		ERROR_AND_DIE("MapDefinition is null in Map constructor.");
	}
	m_physicsTimer->Start();
	m_spriteSheet = new SpriteSheet(*m_definition->m_spriteSheetTexture, m_definition->m_spriteSheetCellCount);
	m_exposureTimer = new Timer(m_exposurePeriod, m_game->m_clock);
	CreateTiles();
	CreateGeometry();
	CreateBuffers();
	CreateDefaultLightConstants();
	m_shader = definition->m_shader;
	InitializeActors();
	UpdateSky(m_waveIndex);
}

Map::~Map()
{
	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;

	delete m_indexBuffer;
	m_indexBuffer = nullptr;

	m_game->RemovePlayer(KEYBOARD);
	m_game->RemovePlayer(CONTROLLER);

	m_texture = nullptr;

	m_game = nullptr;

	for (Actor* a : m_actors)
	{
		delete a;
		a = nullptr;
	}
	m_actors.clear();

	delete m_physicsTimer;
	m_physicsTimer = nullptr;

	m_tiles.clear();

	m_infos.clear();

	m_vertexes.clear();
	m_indexes.clear();

	
}

void Map::Update(float deltaSeconds)
{
	if (m_game->GetPlayerByIndex(0) && !m_game->IsMultiplayer())
	{
		if (g_theInput->WasKeyJustPressed('N'))
		{
			DebugPossessNext();
		}
	}

	UpdateWave();

	/*if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
	{
		if (m_playerA->m_mode == FREEFLY)
		{
			Ray3 ray;
			ray.m_rayStart = m_playerA->m_position;
			ray.m_rayForwardNormal = m_playerA->GetFwdNormal();
			ray.m_rayMaxLength = 10.f;

			RaycastResult3D result = RaycastAll(ray.m_rayStart, ray.m_rayForwardNormal, ray.m_rayMaxLength);

			DebugAddRaycastResult(result, 0.01f, 10.f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::X_RAY);
		}
		
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		Ray3 ray;
		ray.m_rayStart = m_playerA->m_position;
		ray.m_rayForwardNormal = m_playerA->GetFwdNormal();
		ray.m_rayMaxLength = 0.25f;

		RaycastResult3D result = RaycastAll(ray.m_rayStart, ray.m_rayForwardNormal, ray.m_rayMaxLength);

		DebugAddRaycastResult(result, 0.01f, 10.f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::X_RAY);
	}*/

	std::string debugText;

	if (g_theInput->WasKeyJustPressed(KEYCODE_F2))
	{
		m_lights.m_sunDirection.x -= 1;
		debugText = Stringf("Sun Direction x: %.0f", m_lights.m_sunDirection.x);
		DebugAddMessage(debugText, 2.f);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F3))
	{
		m_lights.m_sunDirection.x += 1;
		debugText = Stringf("Sun Direction x: %.0f", m_lights.m_sunDirection.x);
		DebugAddMessage(debugText, 2.f);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F4))
	{
		m_lights.m_sunDirection.y -= 1;
		debugText = Stringf("Sun Direction y: %.0f", m_lights.m_sunDirection.y);
		DebugAddMessage(debugText, 2.f);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F5))
	{
		m_lights.m_sunDirection.y += 1;
		debugText = Stringf("Sun Direction y: %.0f", m_lights.m_sunDirection.y);
		DebugAddMessage(debugText, 2.f);
	}

	/*if (g_theInput->WasKeyJustPressed(KEYCODE_F6))
	{
		m_lights.m_sunIntensity -= 0.05f;
		m_lights.m_sunIntensity = GetClamped(m_lights.m_sunIntensity, 0.0f, 1.0f);

		debugText = Stringf("Sun Intensity: %.2f", m_lights.m_sunIntensity);
		DebugAddMessage(debugText, 2.f);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F7))
	{
		m_light.m_sunIntensity += 0.05f;
		m_light.m_sunIntensity = GetClamped(m_light.m_sunIntensity, 0.0f, 1.0f);

		debugText = Stringf("Sun Intensity: %.2f", m_light.m_sunIntensity);
		DebugAddMessage(debugText, 2.f);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		m_light.m_ambientIntensity -= 0.05f;
		m_light.m_ambientIntensity = GetClamped(m_light.m_ambientIntensity, 0.0f, 1.0f);
		debugText = Stringf("Ambient Intensity: %.2f", m_light.m_ambientIntensity);
		DebugAddMessage(debugText, 2.f);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F9))
	{
		m_light.m_ambientIntensity += 0.05f;
		m_light.m_ambientIntensity = GetClamped(m_light.m_ambientIntensity, 0.0f, 1.0f);
		debugText = Stringf("Ambient Intensity: %.2f", m_light.m_ambientIntensity);
		DebugAddMessage(debugText, 2.f);
	}*/

	g_theRenderer->SetLightConstants(m_lights);



	while (m_physicsTimer && m_physicsTimer->DecrementPeriodIfElapsed())
	{
		for (Actor* actor : m_actors)
		{
			if (!actor || actor->IsDead()) continue;

			if (actor->m_actorDef.m_simulated)
			{
				actor->UpdatePhysics((float)m_physicsTimer->m_period);

			}
		}

		CollideActors();
		CollideActorsWithMap();

		for (Actor* actor : m_actors)
		{
			if (!actor || actor->IsDead()) continue;
			actor->EndUpdatePhysics();
		}
	}

	for (Actor* actor : m_actors)
	{
		if (actor)
		{
			if (actor->m_deadTimer->DecrementPeriodIfElapsed())
			{
				actor->m_isGarbage = true;

				if (actor->m_actorDef.m_name == "Marine")
				{
					if (m_game->IsMultiplayer())
					{
						for (int i = 0; i < (int)m_game->m_players.size(); ++i)
						{
							PlayerController* player = m_game->m_players[i];
							if (player && player->GetActor() == actor)
							{
								SpawnNewMarine(i);
								break;
							}
						}
					}
					else
					{
						SpawnNewMarine(0);
					}
					
				}
			}
			actor->Update(deltaSeconds);
		}
	}
	
	DeleteGarbagesAndUnpossessControllers();

	
}

void Map::Render(Camera& camera)
{
	std::vector<Vertex_PCU> skyVerts = m_skyVerts;
	Mat44 transform;
	transform.SetTranslation3D(camera.GetPosition());
	transform.AppendScaleUniform3D(0.5f);
	TransformVertexArray3D(skyVerts, transform);
	
	g_theRenderer->BindShader(g_theRenderer->GetShader("Default"));
	g_theRenderer->BindTexture(m_definition->m_skyboxTex);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->DrawVertexArray(skyVerts);
	
	g_theRenderer->BindShader(m_shader);
	g_theRenderer->BindTexture(nullptr);

	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);

	g_theRenderer->BindTexture(&m_spriteSheet->GetTexture());
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->SetModelConstants();

	g_theRenderer->CopyCPUToGPU(m_vertexBuffer, m_indexBuffer, m_vertexes.data(), m_indexes.data(), (unsigned int)m_vertexes.size(), (unsigned int)m_indexes.size());
	g_theRenderer->DrawIndexedVertexBuffer(m_vertexBuffer, m_indexBuffer, (unsigned int)m_indexes.size());
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(g_theRenderer->GetShader("Default"));

	for (Actor* actor : m_actors)
	{
		if (actor)
		{
			actor->Render(camera);
		}
		
	}
	g_theRenderer->BindShader(g_theRenderer->GetShader("Default"));

	

}

void Map::UpdateWave()
{
	if (!HasEnemies())
	{
		if (m_waveIndex < (int)m_definition->m_waves.size() - 1)
		{
			
			m_waveIndex++;
			UpdateSky(m_waveIndex);
			SpawnWave(m_waveIndex);
		}
		else
		{
			UpdateSky(m_waveIndex);
			SpawnWave(m_waveIndex);
		}
		
	}
	else
	{
		if (m_exposureTimer->DecrementPeriodIfElapsed())
		{
			m_exposureTimer->Stop();
			for (Actor* a : m_actors)
			{
				if (a && !a->IsDead() && a->m_actorDef.m_faction == ActorFaction::DEMON)
				{
					int randomIndex = 0;
					if (m_game->IsMultiplayer())
					{
						randomIndex = m_game->m_rng->RollRandomIntInRange(0, 1);
					}
					 
					Actor* playerActor = m_game->GetPlayerByIndex(randomIndex)->GetActor();

					a->m_aiController->m_target = playerActor;
					
				}
			}
		}
	}
}

void Map::UpdateSky(int waveIndex)
{
	m_skyVerts.clear();

	const int maxWaveIndex = (const int) m_definition->m_waves.size();
	float t = GetClamped(static_cast<float>(waveIndex) / maxWaveIndex, 0.f, 1.f);

	Rgba8 color;
	color.r = 255;
	color.g = static_cast<unsigned char>(Interpolate(255.f, 0.f, t));
	color.b = static_cast<unsigned char>(Interpolate(255.f, 0.f, t));
	color.a = 255;

	m_skyVerts.clear();
	AddVertsForSkySphere3D(m_skyVerts, color);

	float r = RangeMapClamped((float)color.r, 0.f, 255.f, 0.f, 1.f);
	float g = RangeMapClamped((float)color.g, 0.f, 255.f, 0.f, 1.f);
	float b = RangeMapClamped((float)color.b, 0.f, 255.f, 0.f, 1.f);


	m_lights.m_sunColor = Vec4(r, g, b, 1.f);
}

bool Map::HasEnemies()
{
	for (Actor* a : m_actors)
	{
		if (!a || a->IsDead()) continue;
		if (a->m_actorDef.m_faction == ActorFaction::DEMON)
		{
			return true;
		}
	}
	return false;
}

void Map::CreateTiles()
{
	m_dimensions = m_definition->m_image.GetDimensions();
	int tileCount = m_dimensions.x * m_dimensions.y;

	const Image& mapImage = m_definition->m_image;
	const Rgba8* imageData = static_cast<const Rgba8*> (mapImage.GetRawData());

	if (!imageData)
	{
		ERROR_AND_DIE("Failed to load mpa image data.");
	}

	m_tiles.resize(tileCount);

	for (int y = 0; y < m_dimensions.y; ++y)
	{
		for (int x = 0; x < m_dimensions.x; ++x)
		{
			int tileIndex = y * m_dimensions.x + x;
			const Rgba8& pixelColor = imageData[tileIndex];
			const TileDefinition* tileDef = TileDefinition::GetDefinitionForColor(pixelColor);
			if (!tileDef)
			{
				ERROR_AND_DIE("No matching TileDefinition for color.");
			}


			Vec3 tileMins(static_cast<float>(x), static_cast<float>(y), 0.f);
			Vec3 tileMaxs = tileMins + Vec3(1.f, 1.f, 1.f);
			AABB3 bounds(tileMins, tileMaxs);

			m_tiles[tileIndex] = Tile(bounds, tileDef);
		}
	}

}

void Map::CreateGeometry()
{
	m_vertexes.clear();
	m_indexes.clear();

	for (int y = 0; y < m_dimensions.y; ++y)
	{
		for (int x = 0; x < m_dimensions.x; ++x)
		{
			int tileIndex = y * m_dimensions.x + x;
			const Tile& tile = m_tiles[tileIndex];
			const TileDefinition* tileDef = tile.GetTileDefinition();

			const AABB3& bounds = tile.GetBounds();

			
			if (tileDef->m_floorSpriteCoords != IntVec2(-1, -1))
			{
				Vec2 uvAtMins, uvAtMaxs;
				const SpriteDefinition& spriteDef = m_spriteSheet->GetSpriteDef(tileDef->m_floorSpriteCoords);
				spriteDef.GetUVs(uvAtMins, uvAtMaxs);
				AddGeometryForFloor(m_vertexes, m_indexes, bounds, AABB2(uvAtMins, uvAtMaxs));
			}
			
			if (tileDef->m_ceilingSpriteCoords != IntVec2(-1, -1))
			{
				Vec2 uvAtMins, uvAtMaxs;
				const SpriteDefinition& spriteDef = m_spriteSheet->GetSpriteDef(tileDef->m_ceilingSpriteCoords);
				spriteDef.GetUVs(uvAtMins, uvAtMaxs);

				//AddGeometryForCeiling(m_vertexes, m_indexes, bounds, AABB2(uvAtMins, uvAtMaxs));

			}

			if (tileDef->m_wallSpriteCoords != IntVec2(-1, -1))
			{
				Vec2 uvAtMins, uvAtMaxs;
				const SpriteDefinition& spriteDef = m_spriteSheet->GetSpriteDef(tileDef->m_wallSpriteCoords);
				spriteDef.GetUVs(uvAtMins, uvAtMaxs);

				AddGeometryForWall(m_vertexes, m_indexes, bounds, AABB2(uvAtMins, uvAtMaxs));
			}

		}
	}

	CreateBuffers();
}


void Map::AddGeometryForWall(std::vector<Vertex_PCUTBN>& vertexes, std::vector<unsigned int>& indexes, const AABB3& bounds, const AABB2& UVs) const
{
	Rgba8 color = Rgba8::WHITE;

	AddVertsForAABB3DWall(vertexes, indexes, bounds, color, UVs);
}




void Map::AddGeometryForFloor(std::vector<Vertex_PCUTBN>& vertexes, std::vector<unsigned int>& indexes, const AABB3& bounds, const AABB2& UVs) const
{
	Vec3 bottomLeft = Vec3(bounds.m_mins.x, bounds.m_mins.y, bounds.m_mins.z);
	Vec3 bottomRight = Vec3(bounds.m_maxs.x, bounds.m_mins.y, bounds.m_mins.z);
	Vec3 topRight = Vec3(bounds.m_maxs.x, bounds.m_maxs.y, bounds.m_mins.z);
	Vec3 topLeft = Vec3(bounds.m_mins.x, bounds.m_maxs.y, bounds.m_mins.z);

	Rgba8 color = Rgba8::WHITE;

	AddVertsForQuad3D(vertexes, indexes, bottomLeft, bottomRight, topRight, topLeft, color, UVs);
}



void Map::AddGeometryForCeiling(std::vector<Vertex_PCUTBN>& vertexes, std::vector<unsigned int>& indexes, const AABB3& bounds, const AABB2& UVs) const
{
	Vec3 bottomLeft = Vec3(bounds.m_mins.x, bounds.m_maxs.y, bounds.m_maxs.z);
	Vec3 bottomRight = Vec3(bounds.m_maxs.x, bounds.m_maxs.y, bounds.m_maxs.z);
	Vec3 topRight = Vec3(bounds.m_maxs.x, bounds.m_mins.y, bounds.m_maxs.z);
	Vec3 topLeft = Vec3(bounds.m_mins.x, bounds.m_mins.y, bounds.m_maxs.z);

	Rgba8 color = Rgba8::WHITE;

	AddVertsForQuad3D(vertexes, indexes, bottomLeft, bottomRight, topRight, topLeft, color, UVs);
}



void Map::CreateBuffers()
{
	if (m_vertexBuffer)
	{
		delete m_vertexBuffer;
		m_vertexBuffer = nullptr;
	}

	if (m_indexBuffer)
	{
		delete m_indexBuffer;
		m_indexBuffer = nullptr;
	}

	int initialVertexBufferSize = sizeof(Vertex_PCUTBN);
	int initialIndexBufferSize = sizeof(unsigned int);

	m_vertexBuffer = g_theRenderer->CreateVertexBuffer(initialVertexBufferSize, sizeof(Vertex_PCUTBN));
	m_indexBuffer = g_theRenderer->CreateIndexBuffer(initialIndexBufferSize, sizeof(unsigned int));
}

bool Map::IsPositionInBounds(Vec3 position, const float tolerance) const
{

	if (position.x >= 0 - tolerance && position.x < m_dimensions.x + tolerance &&
		position.y >= 0 - tolerance && position.y < m_dimensions.y + tolerance)
	{
		if (position.z >= 0 - tolerance)
		{
			return true;
		}
	}
	return false;
}


bool Map::AreCoordsInBounds(int x, int y) const
{
	if (x >= 0 && x < m_dimensions.x && y >= 0 && y < m_dimensions.y)
	{
		return true;
	}
	return false;
}

const Tile* Map::GetTile(int x, int y) const
{
	if (AreCoordsInBounds(x, y))
	{
		return &m_tiles[y * m_dimensions.x + x];
	}
	return nullptr;
}

Tile Map::GetTile(int x, int y)
{
	if (AreCoordsInBounds(x, y))
	{
		return m_tiles[y * m_dimensions.x + x];
	}
	return Tile();
}

bool Map::IsTileSolid(int x, int y) const
{
	if (AreCoordsInBounds(x, y))
	{
		Tile tile = m_tiles[y * m_dimensions.x + x];

		return tile.GetTileDefinition()->m_isSolid;
	}
	return false;
}

void Map::InitializeActors()
{
	m_actors.reserve(1000);

	for (SpawnInfo info : m_definition->m_spawnInfos)
	{
		if (info.m_actorName != "SpawnPoint")
		{
			SpawnActor(info);
		}
		else
		{
			m_infos.push_back(info);
		}
	}

	std::vector<SpawnInfo> tempInfo = m_infos;

	int numPlayers = (int)m_game->m_players.size();
	for (int i = 0; i < numPlayers; ++i)
	{
		PlayerController* player = m_game->GetPlayerByIndex(i);
		if (!player) continue;

		Actor* spawnedActor = nullptr;
		const int maxTries = (int)tempInfo.size();
		for (int attempt = 0; attempt < maxTries; ++attempt)
		{
			int randomIndex = m_game->m_rng->RollRandomIntInRange(0, (int)tempInfo.size() - 1);
			SpawnInfo spawnInfo = tempInfo[randomIndex];

			bool isOccupied = false;
			for (Actor* actor : m_actors)
			{
				if (!actor) continue;
				float distanceSquared = GetDistanceSquared3D(actor->m_position, spawnInfo.m_actorPos);
				if (distanceSquared < 1.0f) 
				{
					isOccupied = true;
					break;
				}
			}

			if (!isOccupied && player->m_handle != ActorHandle::INVALID)
			{
				SpawnInfo marineInfo;
				marineInfo.m_actorName = "Marine";
				marineInfo.m_actorOri = spawnInfo.m_actorOri;
				marineInfo.m_actorPos = spawnInfo.m_actorPos;
				marineInfo.m_actorVel = spawnInfo.m_actorVel;

				spawnedActor = SpawnActor(marineInfo);
				

				if (i == 0)
				{
					m_curPossessedIndex = spawnedActor->m_handle.GetIndex();
				}

				tempInfo.erase(tempInfo.begin() + randomIndex);
				break;
			}
		}

		if (!spawnedActor && !tempInfo.empty())
		{
			int fallbackIndex = m_game->m_rng->RollRandomIntInRange(0, (int)tempInfo.size() - 1);
			SpawnInfo fallbackSpawn = tempInfo[fallbackIndex];

			SpawnInfo marineInfo;
			marineInfo.m_actorName = "Marine";
			marineInfo.m_actorOri = fallbackSpawn.m_actorOri;
			marineInfo.m_actorPos = fallbackSpawn.m_actorPos;
			marineInfo.m_actorVel = fallbackSpawn.m_actorVel;

			spawnedActor = SpawnActor(marineInfo);

			player->m_map = this;
			player->Possess(spawnedActor);

			if (i == 0)
			{
				m_curPossessedIndex = spawnedActor->m_handle.GetIndex();
			}

			tempInfo.erase(tempInfo.begin() + fallbackIndex);

		}
	}
	SpawnWave(0);

	
}


void Map::CollideActors()
{
	for (int i = 0; i < (int) m_actors.size(); ++i)
	{
		Actor* actorA = m_actors[i];
		if (!actorA) continue;

		for (int j = i + 1; j < (int) m_actors.size(); ++j)
		{
			Actor* actorB = m_actors[j];
			if (!actorB) continue;

			if (actorA->IsStatic() && actorB->IsStatic()) continue;
			bool hasDead = actorA->IsDead() || actorB->IsDead();
			bool bothProjectile = actorA->IsProjectile() && actorB->IsProjectile();
			if (hasDead || bothProjectile) continue;


			if (actorA->m_owner == actorB || actorB->m_owner == actorA) continue;
			CollideActors(actorA, actorB);
		}
	}

}

void Map::CollideActors(Actor* actorA, Actor* actorB)
{
	ZCylinder3 colliderA = actorA->GetCollider();
	ZCylinder3 colliderB = actorB->GetCollider();

	Vec2 actorCenterA = Vec2(actorA->m_position.x, actorA->m_position.y);
	float radiusA = colliderA.m_radius;
	float minA = actorA->m_position.z;
	float maxA = minA + (colliderA.m_maxZ - colliderA.m_minZ);
	FloatRange minMaxZA = FloatRange(minA, maxA);

	Vec2 actorCenterB = Vec2(actorB->m_position.x, actorB->m_position.y);
	float radiusB = colliderB.m_radius;
	float minB = actorB->m_position.z;
	float maxB = minB + (colliderB.m_maxZ - colliderB.m_minZ);
	FloatRange minMaxZB = FloatRange(minB, maxB);

	if (DoZCylindersOverlap3D(actorCenterA, radiusA, minMaxZA, actorCenterB, radiusB, minMaxZB))
	{
		if (actorA->IsProjectile() && actorA->m_owner != actorB)
		{
			actorA->OnCollide(actorB, actorA->m_actorDef.m_impulseOnCollide);
		}
		else if (actorB->IsProjectile() && actorB->m_owner != actorA)
		{
			actorB->OnCollide(actorA, actorB->m_actorDef.m_impulseOnCollide);
		}


		if (!actorA->IsStatic() && !actorB->IsStatic())
		{
			PushDiscsOutOfEachOther2D(actorCenterA, radiusA, actorCenterB, radiusB);

			actorA->m_position = Vec3(actorCenterA.x, actorCenterA.y, actorA->m_position.z);
			actorB->m_position = Vec3(actorCenterB.x, actorCenterB.y, actorB->m_position.z);
		}
		else if (actorA->IsStatic())
		{
			PushDiscOutOfDisc2D(actorCenterB, radiusB, actorCenterA, radiusA);

			actorB->m_position = Vec3(actorCenterB.x, actorCenterB.y, actorB->m_position.z);
		}
		else if (actorB->IsStatic())
		{
			PushDiscOutOfDisc2D(actorCenterA, radiusA, actorCenterB, radiusB);

			actorA->m_position = Vec3(actorCenterA.x, actorCenterA.y, actorA->m_position.z);
		}
	}
}

void Map::CollideActorsWithMap()
{
	for (Actor* actor : m_actors)
	{
		if ( actor && !actor->IsStatic() && !actor->IsDead())
		{
			CollideActorWithMap(actor);
		}
	}
}

void Map::CollideActorWithMap(Actor* actor)
{
	for (int y = 0; y < m_dimensions.y; ++y)
	{
		for (int x = 0; x < m_dimensions.x; ++x)
		{

			const Tile& tile = m_tiles[y * m_dimensions.x + x];

			ColliderActorWithFloorAndCeiling(actor, tile);

			if (!tile.GetTileDefinition()->m_isSolid) continue;

			CollideActorWithWall(actor, tile);
		}
	}
}

void Map::CollideActorWithWall(Actor* actor, Tile tile)
{
	ZCylinder3 collider = actor->m_collider;
	Vec2 centerXY = Vec2(actor->m_position.x, actor->m_position.y);
	FloatRange minMaxZ = FloatRange(actor->m_position.z, actor->m_position.z + (collider.m_maxZ - collider.m_minZ));
	float radius = collider.m_radius;

	AABB2 floorBounds = tile.GetFloorBounds();

	if (DoZCylinderAndAABBOverlap3D(centerXY, radius, minMaxZ, tile.GetBounds()))
	{
		if (actor->IsProjectile()) actor->Die();
		PushDiscOutOfAABB2D(centerXY, radius, floorBounds);

		actor->m_position = Vec3(centerXY.x, centerXY.y, actor->m_position.z);
	}
}

void Map::ColliderActorWithFloorAndCeiling(Actor* actor, Tile tile)
{
	float minZ = tile.GetBounds().m_mins.z;
	float maxZ = tile.GetBounds().m_maxs.z - (actor->m_collider.m_maxZ - actor->m_collider.m_minZ);

	if (actor->m_position.z > maxZ + 5.f && actor->IsProjectile())
	{
		actor->m_position.z = maxZ + 5.f;
		actor->Die();
	}

	/*else if (actor->m_position.z > maxZ)
	{
		actor->m_position.z = maxZ;
	}*/

	if (actor->m_position.z < minZ)
	{
		actor->m_position.z = minZ;
		if (actor->IsProjectile()) actor->Die();
	}


}

void Map::DeleteGarbagesAndUnpossessControllers()
{
	for (int i = 0; i < (int) m_actors.size(); ++i)
	{
		if (m_actors[i] && m_actors[i]->IsGarbage())
		{
			delete m_actors[i];
			m_actors[i] = nullptr;
		}

	}
}

bool Map::IsTileWalkable(IntVec2 tileCoords)
{
	Tile tile = GetTile(tileCoords.x, tileCoords.y);

	return !tile.GetTileDefinition()->m_isSolid;
}

bool Map::IsTileEmpty(IntVec2 tileCoords)
{
	for (Actor* actor : m_actors)
	{
		if (!actor || actor->IsDead()) continue;
		Vec2 actorPos2D = actor->GetPosition().GetXY();
		IntVec2 actorTile = IntVec2((int)actorPos2D.x, (int)actorPos2D.y);

		if (actorTile == tileCoords)
		{
			return false;
		}
	}

	return true;
}

bool Map::IsTileNest(IntVec2 tileCoords)
{
	return tileCoords.x >= 0 && tileCoords.x < 7 &&
		tileCoords.y >= 0 && tileCoords.y < 4;
}

bool Map::IsTileInMap(IntVec2 tileCoords)
{
	return tileCoords.x >= 0 && tileCoords.x < m_dimensions.x &&
		tileCoords.y >= 0 && tileCoords.y < m_dimensions.y;
}

Vec2 Map::GetTileCenter(IntVec2 tileCoords)
{
	return Vec2(tileCoords.x + 0.5f, tileCoords.y + 0.5f);
}

int Map::GetRemainingEnemies()
{
	int num = 0;

	for (Actor* a : m_actors)
	{
		if (a && a->m_actorDef.m_faction == ActorFaction::DEMON && !a->IsDead())
		{
			num++;
		}
	}
	return num;
}

RaycastResult3D Map::RaycastAll(const Vec3& start, const Vec3& direction, float distance, Actor** out_hitActor) const
{
	RaycastResult3D firstImpactResult;
	firstImpactResult.m_rayStart = start;
	firstImpactResult.m_rayForwardNormal = direction;
	firstImpactResult.m_rayMaxLength = distance;
	firstImpactResult.m_impactDist = distance;

	bool hitActorValid = false;
	Actor* hitActorCandidate = nullptr;

	RaycastResult3D resultXY = RaycastWorldXY(start, direction, distance);
	if (resultXY.m_didImpact && resultXY.m_impactDist < firstImpactResult.m_impactDist)
	{
		firstImpactResult = resultXY;
		hitActorValid = false;
	}

	RaycastResult3D resultZ = RaycastWorldZ(start, direction, distance);
	if (resultZ.m_didImpact && resultZ.m_impactDist < firstImpactResult.m_impactDist)
	{
		firstImpactResult = resultZ;
		hitActorValid = false;
	}

	RaycastResult3D resultActors = RaycastLivingActors(start, direction, distance, &hitActorCandidate);
	if (resultActors.m_didImpact && resultActors.m_impactDist < firstImpactResult.m_impactDist)
	{
		firstImpactResult = resultActors;
		hitActorValid = true;
	}

	if (out_hitActor)
	{
		if (hitActorValid)
			*out_hitActor = hitActorCandidate;
		else
			*out_hitActor = nullptr;
	}

	return firstImpactResult;
}

RaycastResult3D Map::RaycastAllIgnoreFriend(const Vec3& start, const Vec3& direction, float distance, Actor** out_hitActor, ActorFaction ignoredFaction) const
{
	RaycastResult3D firstImpactResult;
	firstImpactResult.m_rayStart = start;
	firstImpactResult.m_rayForwardNormal = direction;
	firstImpactResult.m_rayMaxLength = distance;
	firstImpactResult.m_impactDist = distance;

	bool hitActorValid = false;
	Actor* hitActorCandidate = nullptr;

	RaycastResult3D resultXY = RaycastWorldXY(start, direction, distance);
	if (resultXY.m_didImpact && resultXY.m_impactDist < firstImpactResult.m_impactDist)
	{
		firstImpactResult = resultXY;
		hitActorValid = false;
	}

	RaycastResult3D resultZ = RaycastWorldZ(start, direction, distance);
	if (resultZ.m_didImpact && resultZ.m_impactDist < firstImpactResult.m_impactDist)
	{
		firstImpactResult = resultZ;
		hitActorValid = false;
	}

	RaycastResult3D resultActors = RaycastEnemyActors(start, direction, distance, &hitActorCandidate, ignoredFaction);
	if (resultActors.m_didImpact && resultActors.m_impactDist < firstImpactResult.m_impactDist)
	{
		firstImpactResult = resultActors;
		hitActorValid = true;
	}

	if (out_hitActor)
	{
		if (hitActorValid)
			*out_hitActor = hitActorCandidate;
		else
			*out_hitActor = nullptr;
	}

	return firstImpactResult;
}




RaycastResult3D Map::RaycastWorldXY(const Vec3& start, const Vec3& direction, float maxDistance) const
{
	RaycastResult3D result;
	result.m_rayStart = start;
	result.m_rayForwardNormal = direction;
	result.m_rayMaxLength = maxDistance;

	Vec2 startPosition(start.x, start.y);
	Vec2 forwardNormal(direction.x, direction.y);

	int tileX = static_cast<int>(floor(startPosition.x));
	int tileY = static_cast<int>(floor(startPosition.y));



	float fwdDistPerXCrossing = (forwardNormal.x == 0.f) ? FLT_MAX : 1.0f / fabsf(forwardNormal.x);
	float fwdDistPerYCrossing = (forwardNormal.y == 0.f) ? FLT_MAX : 1.0f / fabsf(forwardNormal.y);

	int tileStepDirectionX = (forwardNormal.x < 0) ? -1 : 1;
	int tileStepDirectionY = (forwardNormal.y < 0) ? -1 : 1;

	float xAtFirstXCrossing = tileX + (tileStepDirectionX + 1) / 2.0f;
	float xDistToFirstXCrossing = xAtFirstXCrossing - startPosition.x;
	float fwdDistAtNextXCrossing = fabsf(xDistToFirstXCrossing) * fwdDistPerXCrossing;

	float yAtFirstYCrossing = tileY + (tileStepDirectionY + 1) / 2.0f;
	float yDistToFirstYCrossing = yAtFirstYCrossing - startPosition.y;
	float fwdDistAtNextYCrossing = fabsf(yDistToFirstYCrossing) * fwdDistPerYCrossing;

	float currentFwdDist = 0.f;

	while (currentFwdDist <= maxDistance) {
		if (fwdDistAtNextXCrossing < fwdDistAtNextYCrossing) {
			if (fwdDistAtNextXCrossing > maxDistance) {
				break;
			}
			tileX += tileStepDirectionX;
			currentFwdDist = fwdDistAtNextXCrossing;
			fwdDistAtNextXCrossing += fwdDistPerXCrossing;

			if (IsTileSolid(tileX, tileY)) {
				result.m_impactPos = start + result.m_rayForwardNormal * currentFwdDist;

				if (result.m_impactPos.z > 1.f || result.m_impactPos.z < 0.f) 
				{
					continue;
				}

				result.m_didImpact = true;
				result.m_impactNormal = Vec3(-(float)tileStepDirectionX, 0.f, 0.f);
				result.m_impactDist = currentFwdDist;
				return result;
			}
		}
		else {
			if (fwdDistAtNextYCrossing > maxDistance) {
				break;
			}
			tileY += tileStepDirectionY;
			currentFwdDist = fwdDistAtNextYCrossing;
			fwdDistAtNextYCrossing += fwdDistPerYCrossing;

			if (IsTileSolid(tileX, tileY)) {
				result.m_impactPos = start + result.m_rayForwardNormal * currentFwdDist;

				if (result.m_impactPos.z > 1.f || result.m_impactPos.z < 0.f) 
				{
					continue;
				}

				result.m_didImpact = true;
				result.m_impactNormal = Vec3(0.f, -(float)tileStepDirectionY, 0.f);
				result.m_impactDist = currentFwdDist;
				return result;
			}
		}
	}

	result.m_didImpact = false;
	result.m_impactPos = start + result.m_rayForwardNormal * maxDistance;
	result.m_impactDist = maxDistance;
	result.m_impactNormal = Vec3(0.f, 0.f, 0.f);
	return result;

}



RaycastResult3D Map::RaycastWorldZ(const Vec3& start, const Vec3& direction, float distance) const
{
	RaycastResult3D result;
	result.m_rayStart = start;
	result.m_rayForwardNormal = direction;
	result.m_rayMaxLength = distance;

	float vz = direction.z;

	if (vz < 0.0f)
	{
		float t = start.z / -vz;
		if (t >= 0.0f && t <= distance)
		{
			result.m_didImpact = true;
			result.m_impactDist = t;
			result.m_impactPos = start + direction * t;
			result.m_impactNormal = Vec3(0.0f, 0.0f, 1.0f);
			return result;
		}
	}

	result.m_didImpact = false;
	result.m_impactDist = distance;
	result.m_impactPos = start + direction * distance;

	result.m_impactNormal = Vec3::ZERO;
	return result;
}



RaycastResult3D Map::RaycastWorldActors(const Vec3& start, const Vec3& direction, float distance, Actor** out_hitActor) const
{
	RaycastResult3D firstImpactResult;
	firstImpactResult.m_rayStart = start;
	firstImpactResult.m_rayForwardNormal = direction;
	firstImpactResult.m_rayMaxLength = distance;
	firstImpactResult.m_impactDist = distance;

	if (out_hitActor != nullptr)
	{
		*out_hitActor = nullptr;
	}

	for (Actor* actor : m_actors)
	{
		if (!actor) continue;

		const ZCylinder3& collider = actor->GetCollider();
		Vec2 centerXY = Vec2(actor->m_position.x, actor->m_position.y);
		float minZ = actor->m_position.z;
		float maxZ = minZ + (collider.m_maxZ - collider.m_minZ);

		RaycastResult3D result = RaycastVsCylinderZ3D(start, direction, distance, centerXY, FloatRange(minZ, maxZ), collider.m_radius);

		if (result.m_didImpact && result.m_impactDist < firstImpactResult.m_impactDist)
		{
			firstImpactResult = result;

			if (out_hitActor != nullptr)
			{
				*out_hitActor = actor;
			}
		}
	}

	return firstImpactResult;
}

RaycastResult3D Map::RaycastLivingActors(const Vec3& start, const Vec3& direction, float distance, Actor** out_hitActor) const
{
	RaycastResult3D firstImpactResult;
	firstImpactResult.m_rayStart = start;
	firstImpactResult.m_rayForwardNormal = direction;
	firstImpactResult.m_rayMaxLength = distance;
	firstImpactResult.m_impactDist = distance;

	if (out_hitActor != nullptr)
	{
		*out_hitActor = nullptr;
	}

	for (Actor* actor : m_actors)
	{
		if (!actor || actor->m_owner)
		{
			continue;
		}

		const ZCylinder3& collider = actor->GetCollider();
		Vec2 centerXY = Vec2(actor->m_position.x, actor->m_position.y);
		float minZ = actor->m_position.z;
		float maxZ = minZ + (collider.m_maxZ - collider.m_minZ);

		RaycastResult3D result = RaycastVsCylinderZ3D(start, direction, distance, centerXY, FloatRange(minZ, maxZ), collider.m_radius);

		if (result.m_didImpact && result.m_impactDist < firstImpactResult.m_impactDist)
		{
			firstImpactResult = result;

			if (out_hitActor != nullptr)
			{
				*out_hitActor = actor;
			}
		}
	}

	return firstImpactResult;
}

RaycastResult3D Map::RaycastEnemyActors(const Vec3& start, const Vec3& direction, float distance, Actor** out_hitActor, ActorFaction ignoredFaction) const
{
	RaycastResult3D firstImpactResult;
	firstImpactResult.m_rayStart = start;
	firstImpactResult.m_rayForwardNormal = direction;
	firstImpactResult.m_rayMaxLength = distance;
	firstImpactResult.m_impactDist = distance;

	if (out_hitActor != nullptr)
	{
		*out_hitActor = nullptr;
	}

	for (Actor* actor : m_actors)
	{
		if (!actor || actor->m_owner || actor->m_actorDef.m_faction == ignoredFaction)
		{
			continue;
		}

		const ZCylinder3& collider = actor->GetCollider();
		Vec2 centerXY = Vec2(actor->m_position.x, actor->m_position.y);
		float minZ = actor->m_position.z;
		float maxZ = minZ + (collider.m_maxZ - collider.m_minZ);

		RaycastResult3D result = RaycastVsCylinderZ3D(start, direction, distance, centerXY, FloatRange(minZ, maxZ), collider.m_radius);

		if (result.m_didImpact && result.m_impactDist < firstImpactResult.m_impactDist)
		{
			firstImpactResult = result;

			if (out_hitActor != nullptr)
			{
				*out_hitActor = actor;
			}
		}
	}

	return firstImpactResult;
}


void Map::CreateDefaultLightConstants()
{
	m_lights.m_sunColor = Vec4(1.f, 1.f, 1.f, 1.f);
	m_lights.m_sunDirection = Vec3(2.f, 1.f, -1.f);
}

Actor* Map::SpawnActor(const SpawnInfo spawnInfo)
{
	std::string name = spawnInfo.m_actorName;
	Vec3 position = spawnInfo.m_actorPos;
	EulerAngles orientation = spawnInfo.m_actorOri;
	Vec3 velocity = spawnInfo.m_actorVel;

	Actor* actor = new Actor(this, name, position, orientation, velocity, false);
	unsigned int uid = GetNextUID();

	unsigned int index = (unsigned int)m_actors.size();
	for (unsigned int i = 0; i < m_actors.size(); ++i)
	{
		if (m_actors[i] == nullptr)
		{
			index = i;
			break;
		}
	}

	ActorHandle handle(uid, index);
	actor->m_handle = handle;

	ActorDefinition actorDef = actor->m_actorDef;

	if (actorDef.m_aiEnabled)
	{
		AIController* aiController = new AIController(handle, this);
		aiController->Possess(actor);
	}

	if (actorDef.m_dieOnSpawn)
	{
		actor->Die();
	}

	if (index < m_actors.size())
	{
		m_actors[index] = actor;
	}
	else
	{
		m_actors.push_back(actor);
	}

	return actor;
}

Actor* Map::SpawnNewMarine(int playerIndex)
{
	if (playerIndex < 0 || playerIndex >= (int)m_game->m_players.size())
		return nullptr;

	PlayerController* player = m_game->GetPlayerByIndex(playerIndex);
	if (!player)
		return nullptr;

	const int maxTries = (int)m_infos.size();
	Actor* marineActor = nullptr;

	for (int attempt = 0; attempt < maxTries; ++attempt)
	{
		int randomIndex = m_game->m_rng->RollRandomIntInRange(0, (int)m_infos.size() - 1);
		SpawnInfo randomInfo = m_infos[randomIndex];

		bool isOccupied = false;
		for (Actor* actor : m_actors)
		{
			if (!actor) continue;
			float distSquared = GetDistanceSquared3D(actor->m_position, randomInfo.m_actorPos);
			if (distSquared < 1.0f) 
			{
				isOccupied = true;
				break;
			}
		}

		if (!isOccupied)
		{
			SpawnInfo marineInfo;
			marineInfo.m_actorName = "Marine";
			marineInfo.m_actorOri = randomInfo.m_actorOri;
			marineInfo.m_actorPos = randomInfo.m_actorPos;
			marineInfo.m_actorVel = randomInfo.m_actorVel;

			marineActor = SpawnActor(marineInfo);

			player->m_map = this;
			player->Possess(marineActor);

			if (playerIndex == 0)
			{
				m_curPossessedIndex = marineActor->m_handle.GetIndex();
			}


			break;
		}
	}

	return marineActor;
}

void Map::SpawnWave(int waveIndex)
{
	Wave wave = m_definition->m_waves[waveIndex];

	for (WaveActor wa : wave.m_waveActors)
	{
		std::string name = wa.m_name;
		int num = wa.m_num;
		for (int i = 0; i < num; ++i)
		{
			SpawnInfo info;
			info.m_actorName = name;
			info.m_actorOri = EulerAngles(m_game->m_rng->RollRandomFloatInRange(0.f, 360.f), 0.f, 0.f);
			Vec2 validPos = GetValidSpawnPos();
			info.m_actorPos = Vec3(validPos.x, validPos.y, 0.f);

			SpawnActor(info);
		}
	}

	m_exposureTimer->Start();
}

Vec2 Map::GetValidSpawnPos()
{
	bool isValid = false;
	IntVec2 coords;
	while (!isValid)
	{
		coords = IntVec2(m_game->m_rng->RollRandomIntInRange(0, m_dimensions.x - 1), m_game->m_rng->RollRandomIntInRange(0, m_dimensions.y - 1));
		if (IsTileWalkable(coords) && IsTileEmpty(coords) && !IsTileNest(coords)) isValid = true;
	}

	return Vec2((float)coords.x + 0.5f, (float)coords.y + 0.5f);
}



Actor* Map::SpawnProjectile(const SpawnInfo spawnInfo)
{
	std::string name = spawnInfo.m_actorName;
	Vec3 position = spawnInfo.m_actorPos;
	EulerAngles orientation = spawnInfo.m_actorOri;
	Vec3 velocity = spawnInfo.m_actorVel;

	Actor* projectile = new Actor(this, name, position, orientation, velocity, false);

	m_actors.push_back(projectile);
	return projectile;
}

Actor* Map::GetActorByHandle(const ActorHandle handle) const
{
	if ((int)m_actors.size() > 0)
	{
		for (Actor* actor : m_actors)
		{
			if (!actor) continue;
			if (actor->m_handle.GetIndex() == handle.GetIndex())
			{
				return actor;
			}
		}
	}

	return nullptr;
}

unsigned int Map::GetNextUID()
{
	m_curActorUID++;

	return m_curActorUID;
}

void Map::DebugPossessNext()
{
	const int actorCount = (int)m_actors.size();
	if (actorCount == 0 || !m_game->m_players[0]) return;

	int startIndex = m_curPossessedIndex;
	for (int offset = 1; offset <= actorCount; ++offset)
	{
		int nextIndex = (startIndex + offset) % actorCount;
		Actor* nextActor = m_actors[nextIndex];

		if (nextActor && nextActor->CanBePossessed())
		{
			m_game->m_players[0]->Possess(nextActor);
			m_curPossessedIndex = nextIndex;
			break;
		}
	}
}

void Map::DebugRender() const
{
	if (m_game->m_players.size() >= 1 && m_game->m_players[0])
	{
		DebugRenderWorld(m_game->m_players[0]->GetCamera());
	}

	if (m_game->IsMultiplayer() && m_game->m_players.size() >= 2 && m_game->m_players[1])
	{
		DebugRenderWorld(m_game->m_players[1]->GetCamera());
	}

}

bool Map::AreActorsEnemies(Actor* self, Actor* other) const
{
	return self->m_actorDef.m_faction != other->m_actorDef.m_faction;
}

Actor* Map::GetClosestVisibleEnemy(Actor* self)
{
	Actor* closestEnemy = nullptr;
	float closestDistanceSquared = 999999999.f;

	for (Actor* other : m_actors)
	{
		if (!other || other == self || other->IsDead() || other->IsGarbage())
			continue;

		if (!self->IsEnemy(other))
			continue;

		if (other->IsProjectile())
			continue;

		if (!other->m_actorDef.m_isVisible)
			continue;

		float distSq = GetDistanceSquared3D(self->m_position, other->m_position);
		if (distSq < closestDistanceSquared)
		{
			closestEnemy = other;
			closestDistanceSquared = distSq;
		}
	}

	return closestEnemy;
}

Vec2 Map::GetValidSpawnFriendPos(Vec3 selfPos)
{
	IntVec2 startTile = IntVec2((int)selfPos.x, (int)selfPos.y);

	const int searchRadius = 10;

	for (int r = 1; r <= searchRadius; ++r)
	{
		for (int dx = -r; dx <= r; ++dx)
		{
			for (int dy = -r; dy <= r; ++dy)
			{
				if (abs(dx) != r && abs(dy) != r)
					continue;

				IntVec2 checkCoords = startTile + IntVec2(dx, dy);
				if (!IsTileInMap(checkCoords))
					continue;

				if (!IsTileWalkable(checkCoords))
					continue;

				if (IsTileEmpty(checkCoords))
				{
					Vec2 spawnPos = Vec2(GetTileCenter(checkCoords).x, GetTileCenter(checkCoords).y);
					return spawnPos;
				}
			}
		}
	}
	return GetValidSpawnPos();
}




