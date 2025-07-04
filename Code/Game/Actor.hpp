#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/ZCylinder3.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Game/ActorDefinition.hpp"
#include "Game/ActorHandle.hpp"
#include "Game/PlayerController.hpp"
#include "Game/AIController.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/IntVec2.hpp"

#include <vector>

class Game;
class Weapon;

class Actor
{
public:
	Actor(Map* owner, std::string name, const Vec3& position, 
		const EulerAngles& orientation, Vec3 velocity, bool isStatic);
	~Actor();
	void Update(float deltaSeconds);
	void Render(Camera& camera) const;

	void UpdateBeControlled(float deltaSeconds);

	void RenderSelf() const;
	void RenderBillboard(Camera& camera) const;
	void UpdatePhysics(float deltaSeconds);
	void EndUpdatePhysics();
	Mat44 GetModelToWorldTransform() const;

	Vec3 GetPosition() const { return m_position; }
	EulerAngles GetOrientation() const { return m_orientation; }
	ZCylinder3 GetCollider() const { return m_collider; }
	bool IsStatic() const { return m_isStatic; }

	float GetHealth() const { return m_health; }
	bool IsDead() const { return m_health <= 0.f; }
	bool IsGarbage() const { return m_isGarbage; }

	bool IsPlayer() const;

	bool IsEnemy(Actor* actor) { return m_actorDef.m_faction != actor->m_actorDef.m_faction; }
	bool IsProjectile() { return m_owner != nullptr; }

	void Damage(float amount, Actor* enemy);
	void CrowdControl(float slowFactor, float controlPeriod, float dragForce, Actor* enemy);
	void AddForce(const Vec3& force);
	void AddImpulse(const Vec3& impulse);
	void OnCollide(Actor* other, float impactImpulse);
	void OnPossessed(Controller* newController);
	void OnUnpossessed();
	void MoveInDirection(float deltaSeconds, const Vec3& direction, bool isRun, float slowFactor);
	void TurnInDirection(const EulerAngles& direction, float maxTurnDegrees, float slowFactor);
	void Attack();
	void EquipWeapon(int weaponIndex);
	void InitializeWeapon();
	Vec3 GetEyePosition() const;

	void Die();

	bool CanBePossessed() const;

	void SelectPrevWeapon();
	void SelectNextWeapon();

	void PlayAnimByName(std::string animName);
	void EndDrag();

	void SpawnFriends();

	IntVec2 GetTileCoords() const { return IntVec2((int)m_position.x, (int)m_position.y); }

private:
	void InitializeVerts();

public:
	ActorHandle m_handle;
	ActorDefinition m_actorDef;
	Map* m_map = nullptr;
	Vec3 m_position;
	EulerAngles m_orientation;
	EulerAngles m_angularVelocity;
	Vec3 m_velocity = Vec3::ZERO;
	Vec3 m_acceleration = Vec3::ZERO;

	int m_weaponIndex = 0;

	std::vector<Vertex_PCU> m_verts;
	std::vector<Vertex_PCU> m_wireVerts;

	std::vector<Weapon*> m_weapons;
	Weapon* m_currentWeapon = nullptr;
	Weapon* m_nextWeapon = nullptr;
	Actor* m_owner = nullptr;

	float m_health = 0.f;
	bool m_isGarbage = false;
	Timer* m_deadTimer = nullptr;

	Controller* m_controller = nullptr;
	AIController* m_aiController = nullptr;

	Rgba8 m_color;
	ZCylinder3 m_collider;
	bool m_isStatic;

	SpriteAnimationGroupDefinition* m_currentAnimGroup = nullptr;
	SpriteAnimationGroupDefinition* m_defaultAnimGroup = nullptr;
	SpriteAnimDefinition* m_currentAnim = nullptr;
	Clock* m_animClock = nullptr;

	SoundID m_deathSound;

	SoundID m_hurtSound;
	SoundID m_crowdSound;

	bool m_isSwitching = false;
	Timer* m_switchTimer = nullptr;

	bool m_isControlled = false;
	float m_slowFactor = 1.f;
	Timer* m_controlledTimer = nullptr;
	float m_dragForce = 0.f;
	Vec3 m_dragPosition = Vec3::ZERO;
	Actor* m_dragActor = nullptr;

	Timer* m_spawnTimer = nullptr;
	
};
