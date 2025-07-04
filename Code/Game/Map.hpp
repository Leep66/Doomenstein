#pragma once
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include <vector>

class Game;
struct MapDefinition;
struct Vertex_PCUTBN;
struct Vertex_PCU;
struct AABB3;
struct AABB2;
struct Vec3;
class Tile;
class Actor;
class SpriteSheet;
class Actor;
struct ActorHandle;
struct SpawnInfo;
class Timer;
class PlayerController;

enum class ActorFaction
{
	INVALID = -1,
	NEUTRAL,
	MARINE,
	DEMON,

	NUM
};

struct Node
{
	IntVec2 position;
	Node* parent = nullptr;
	float gCost = 0.f;
	float hCost = 0.f;
	float fCost() const { return gCost + hCost; }

	bool operator==(const Node& other) const { return position == other.position; }
};



class Map
{
public:
	Map(Game* game, const MapDefinition* definition, double physicasTimeStep);
	~Map();

	void Update(float deltaSeconds);
	void Render(Camera& camera);

	void UpdateWave();
	void UpdateSky(int waveIndex);
	bool HasEnemies();

	void CreateTiles();
	void CreateGeometry();
	void AddGeometryForWall(std::vector<Vertex_PCUTBN>& vertexes, std::vector<unsigned int>& indexes, const AABB3& bounds, const AABB2& UVs) const;
	void AddGeometryForFloor(std::vector<Vertex_PCUTBN>& vertexes, std::vector<unsigned int>& indexes, const AABB3& bounds, const AABB2& UVs) const;
	void AddGeometryForCeiling(std::vector<Vertex_PCUTBN>& vertexes, std::vector<unsigned int>& indexes, const AABB3& bounds, const AABB2& UVs) const;
	void CreateBuffers();

	bool IsPositionInBounds(Vec3 position, const float tolerance = 0.f) const;
	bool AreCoordsInBounds(int x, int y) const;
	const Tile* GetTile(int x, int y) const;
	Tile GetTile(int x, int y);
	bool IsTileSolid(int x, int y) const;

	void InitializeActors();
	
	void CollideActors();
	void CollideActors(Actor* actorA, Actor* actorB);
	void CollideActorsWithMap();
	void CollideActorWithMap(Actor* actor);
	void CollideActorWithWall(Actor* actor, Tile tile);
	void ColliderActorWithFloorAndCeiling(Actor* actor, Tile tile);

	void DeleteGarbagesAndUnpossessControllers();
	bool IsTileWalkable(IntVec2 tileCoords);
	bool IsTileEmpty(IntVec2 tileCoords);
	bool IsTileNest(IntVec2 tileCoords);
	bool IsTileInMap(IntVec2 tileCoords);

	Vec2 GetTileCenter(IntVec2 tileCoords);
	int GetRemainingEnemies();


	RaycastResult3D RaycastAll(const Vec3& start, const Vec3& direction, float distance, Actor** out_hitActor=nullptr) const;
	RaycastResult3D RaycastAllIgnoreFriend(const Vec3& start, const Vec3& direction, float distance, Actor** out_hitActor = nullptr, ActorFaction ignoredFaction = ActorFaction::INVALID) const;
	RaycastResult3D RaycastWorldXY(const Vec3& start, const Vec3& direction, float distance) const;
	RaycastResult3D RaycastWorldZ(const Vec3& start, const Vec3& direction, float distance) const;
	RaycastResult3D RaycastWorldActors(const Vec3& start, const Vec3& direction, float distance, Actor** out_hitActor=nullptr) const;
	RaycastResult3D RaycastLivingActors(const Vec3& start, const Vec3& direction, float distance, Actor** out_hitActor=nullptr) const;
	RaycastResult3D RaycastEnemyActors(const Vec3& start, const Vec3& direction, float distance, Actor** out_hitActor = nullptr, ActorFaction ignoredFaction = ActorFaction::INVALID) const;

	void CreateDefaultLightConstants();

	Actor* SpawnActor(const SpawnInfo spawnInfo);
	Actor* SpawnNewMarine(int playerIndex);
	void SpawnWave(int waveIndex);
	Vec2 GetValidSpawnPos();

	Actor* SpawnProjectile(const SpawnInfo spawnInfo);
	Actor* GetActorByHandle(const ActorHandle handle) const;

	unsigned int GetNextUID();
	void DebugPossessNext();

	void DebugRender() const;

	bool AreActorsEnemies(Actor* self, Actor* other) const;
	Actor* GetClosestVisibleEnemy(Actor* self);

	Vec2 GetValidSpawnFriendPos(Vec3 selfPos);

	Game* m_game = nullptr;
	
	std::vector<Actor*> m_actors;

	Lights m_lights;

	Timer* m_physicsTimer = nullptr;
	int m_totalDeath = 0;

	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer* m_indexBuffer = nullptr;
	const MapDefinition* m_definition = nullptr;
	int m_waveIndex = 0;
	Timer* m_exposureTimer = nullptr;
	float m_exposurePeriod = 180.f;
protected:
	std::vector<Tile> m_tiles;
	IntVec2 m_dimensions;

	unsigned int m_curActorUID = 1;
	std::vector<SpawnInfo> m_infos;


	unsigned int m_curPossessedIndex = 1;

	std::vector<Vertex_PCUTBN> m_vertexes;
	std::vector<unsigned int> m_indexes;
	const Texture* m_texture = nullptr;
	Shader* m_shader = nullptr;
	
	SpriteSheet* m_spriteSheet = nullptr;
	std::vector<Vertex_PCU> m_skyVerts;
	bool m_complete = false;
	
	
	
};