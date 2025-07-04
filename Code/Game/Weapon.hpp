#pragma once
#include "Game/WeaponDefinition.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Game/Map.hpp"
#include "Game/Actor.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include <string>

class Weapon
{
public:
	Weapon() = default;
	Weapon(std::string weaponName, Map* map, Actor* owner);
	~Weapon();

	void Update(float deltaSeconds);
	void AddRecoilForce(Vec2 force);
	void Fire();
	EulerAngles GetRandomDirectionInCone(float coneDegrees);
	
	void FireRay();
	void FireProjectile();
	void FireMelee();
	void FireBlast();

	void Reload();

public:
	WeaponDefinition m_weaponDef;
	Map* m_map = nullptr;
	Actor* m_owner = nullptr;
	Timer* m_refireTimer = nullptr;


	std::string m_currentAnim = "Idle";
	std::string m_nextAnim = "Idle";
	float m_animAge = 0.f;

	SoundID m_fireSound;

	Vec2 m_recoilAngles = Vec2::ZERO;
	Vec2 m_recoilVelocity = Vec2::ZERO;
	
	Timer* m_reloadTimer = nullptr;
	int m_currentCapacity = 0;
	bool m_isReload = false;
	bool m_isRefireCooldown = true;


};