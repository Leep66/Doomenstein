#pragma once
#include "Game/Map.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/Sound.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/SpriteAnimationGroupDefinition.hpp"
#include <string>

struct ActorDefinition
{
	std::string m_name;
	bool m_isVisible = false;
	float m_health = 1.f;
	float m_corpseLifetime = 0.f;
	ActorFaction m_faction = ActorFaction::NEUTRAL;
	bool m_canBePossessed = false;
	bool m_dieOnSpawn = false;
	bool m_isBoss = false;
	std::string m_spawnActor = "";
	float m_spawnPeriod = 0.f;
	int m_spawnNum = 0;


	float m_physicsRadius = 0.f;
	float m_physicsHeight = 0.f;
	bool m_collidesWithWorld = false;
	bool m_collidesWithActors = false;
	bool m_dieOnCollide = false;
	FloatRange m_damageOnCollide = FloatRange(0.f, 0.f);
	float m_impulseOnCollide = 0.f;
	float m_fireHeightOffset = 0.f;

	bool m_simulated = false;
	bool m_flying = false;
	float m_walkSpeed = 0.f;
	float m_runSpeed = 0.f;
	float m_drag = 0.f;
	float m_turnSpeed = 0.f;
	float m_attackRange = 0.f;
	float m_oppositeForce = 0.f;
	
	float m_eyeHeight = 0.f;
	float m_cameraFOVDegrees = 60.f;
	
	bool m_aiEnabled = false;
	float m_sightRadius = 0.f;
	float m_sightAngle = 0.f;

	bool m_isCrowdControl = false;
	float m_controlPeriod = 0.f;
	float m_slowFactor = 1.f;
	float m_dragForce = 0.f;



	Vec2 m_visualSize = Vec2(1.f, 1.f);
	Vec2 m_pivot = Vec2(0.5f, 0.5f);
	BillboardType m_billboardType = BillboardType::NONE;
	bool m_renderLit = false;
	bool m_renderRounded = false;
	bool m_renderHealthBar = false;
	Shader* m_shader = nullptr;
	SpriteSheet* m_spriteSheet = nullptr;
	IntVec2 m_cellCount = IntVec2(1, 1);



	std::vector<SpriteAnimationGroupDefinition*> m_animationGroups;
	std::vector<Sound> m_sounds;
	std::vector<std::string> m_weaponsNames;


	static void InitializeActorDefinitions(const char* path);
	static ActorDefinition const& GetDefinition(std::string actorName);
	static std::vector<ActorDefinition> s_actorDefinitions;
	SpriteAnimationGroupDefinition* GetAnimGroupDef(std::string agName) const;

	Sound GetSoundDef(std::string soundName);


};