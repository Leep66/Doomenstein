#include "Game/Weapon.hpp"
#include "Game/Game.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Game/MapDefinition.hpp"

extern AudioSystem* g_theAudio;

Weapon::Weapon(std::string weaponName, Map* map, Actor* owner)
	: m_map(map)
	, m_owner(owner)
{
	m_weaponDef = WeaponDefinition::GetDefinition(weaponName);
	m_refireTimer = new Timer(m_weaponDef.m_refireTime, m_map->m_game->m_clock);
	m_refireTimer->Start();

	std::string path = m_weaponDef.m_fireSound.m_name;
	m_fireSound = g_theAudio->CreateOrGetSound(path, AUDIO3D);
	m_reloadTimer = new Timer(m_weaponDef.m_reloadDuration, m_map->m_game->m_clock);

	m_currentCapacity = m_weaponDef.m_maxCapacity;

	m_refireTimer->ForceComplete();
	m_reloadTimer->ForceComplete();


}

Weapon::~Weapon()
{
	m_map = nullptr;
	m_owner = nullptr;
	
	delete m_refireTimer;
	m_refireTimer = nullptr;
}

void Weapon::Update(float deltaSeconds)
{
	if (m_refireTimer->DecrementPeriodIfElapsed())
	{
		m_refireTimer->Stop();
		m_isRefireCooldown = true;
	}

	if (m_owner == nullptr || m_owner->m_actorDef.m_name != "Marine") {
		return;
	}

	PlayerController* player = (PlayerController*)m_owner->m_controller;
	if (player == nullptr) {
		return;
	}

	m_recoilAngles += m_recoilVelocity * deltaSeconds;

	player->m_orientation.m_pitchDegrees -= m_recoilVelocity.x * deltaSeconds;
	player->m_orientation.m_yawDegrees += m_recoilVelocity.y * deltaSeconds;

	const float recoilDamping = m_weaponDef.m_recoilDamping;
	m_recoilVelocity *= 1.f / (1.f + recoilDamping * deltaSeconds);

	if (m_reloadTimer->DecrementPeriodIfElapsed())
	{
		m_reloadTimer->Stop();
		m_currentCapacity = m_weaponDef.m_maxCapacity;
		m_isReload = false;
		
	}
}


void Weapon::AddRecoilForce(Vec2 force)
{
	float recoilYaw = force.x;
	float recoilPitch = force.y;

	m_recoilVelocity.y += recoilYaw;
	m_recoilVelocity.x += recoilPitch;
	
}

void Weapon::Fire()
{
	

	if (m_refireTimer->IsStopped() && !m_owner->IsDead())
	{
		if (m_weaponDef.m_rayCount > 0)
		{
			FireRay();
		}
		else if (m_weaponDef.m_projctileCount > 0)
		{
			FireProjectile();
		}
		else if (m_weaponDef.m_meleeCount > 0)
		{
			FireMelee();
		}
		else if (m_weaponDef.m_blastRange > 0.f)
		{
			FireBlast();
		}

		m_isRefireCooldown = false;
		m_refireTimer->Start();

		m_nextAnim = "Attack";

		if (m_owner->m_currentAnimGroup->m_name != "Attack")
		{
			m_owner->PlayAnimByName("Attack");
		}

		g_theAudio->StartSoundAt(m_fireSound, m_owner->m_position, false, 0.5f);

		float randomYaw = m_owner->m_map->m_game->m_rng->RollRandomFloatInRange(m_weaponDef.m_recoilYaw);
		float randomPitch = m_owner->m_map->m_game->m_rng->RollRandomFloatInRange(m_weaponDef.m_recoilPitch);

		AddRecoilForce(Vec2(randomYaw, randomPitch));
	}

	

}

EulerAngles Weapon::GetRandomDirectionInCone(float coneDegrees)
{
	RandomNumberGenerator* rng = m_map->m_game->m_rng;

	float halfCone = coneDegrees * 0.5f;

	float randomYawOffset = rng->RollRandomFloatInRange(-halfCone, halfCone);
	float randomPitchOffset = rng->RollRandomFloatInRange(-halfCone, halfCone);

	EulerAngles direction;
	direction.m_yawDegrees = m_owner->m_orientation.m_yawDegrees + randomYawOffset;
	direction.m_pitchDegrees = m_owner->m_orientation.m_pitchDegrees + randomPitchOffset;
	direction.m_rollDegrees = 0.f;

	return direction;
}

void Weapon::FireRay()
{
	if (!m_owner || !m_map)
		return;

	float coneAngleDeg = m_weaponDef.m_rayCone;
	int rayCount = m_weaponDef.m_rayCount;

	Vec3 eyePos = m_owner->GetEyePosition();
	Vec3 forward = m_owner->m_orientation.GetForwardNormal();
	float rayDist = m_weaponDef.m_rayRange;

	float halfAngle = coneAngleDeg * 0.5f;
	float angleStep = (rayCount > 1) ? (coneAngleDeg / (rayCount - 1)) : 0.f;

	for (int i = 0; i < rayCount; ++i)
	{
		float angleOffset = -halfAngle + i * angleStep;

		Vec3 dir = RotateVectorInPlane(forward, angleOffset);

		Actor* hitActor = nullptr;
		RaycastResult3D result = m_map->RaycastAll(eyePos, dir, rayDist, &hitActor);


		Vec3 endPos = eyePos + dir * rayDist;

		if (result.m_didImpact && hitActor && !hitActor->IsDead())
		{
			float damage = m_map->m_game->m_rng->RollRandomFloatInRange(m_weaponDef.m_rayDamage);

			if (hitActor == m_owner)
			{
				return;
			}

			hitActor->Damage(damage, m_owner);

			float impulse = m_weaponDef.m_rayImpulse;
			Vec3 impulseVec = dir.IgnoreZ() * impulse;

			hitActor->AddImpulse(impulseVec);


			SpawnInfo info;
			info.m_actorName = "BloodSplatter";
			info.m_actorPos = result.m_impactPos;
			info.m_actorOri = EulerAngles(0.f, 0.f, 0.f);
			info.m_actorVel = Vec3::ZERO;

			m_map->SpawnActor(info);


		}

		else if (result.m_didImpact && !hitActor)
		{
			SpawnInfo info;
			info.m_actorName = "BulletHit";
			info.m_actorPos = result.m_impactPos;
			info.m_actorOri = EulerAngles(0.f, 0.f, 0.f);
			info.m_actorVel = Vec3::ZERO;

			m_map->SpawnActor(info);
		}
	}

	m_currentCapacity -= 1;
	
}

void Weapon::FireProjectile()
{
	for (int i = 0; i < m_weaponDef.m_projctileCount; i++)
	{
		SpawnInfo spawnInfo;
		spawnInfo.m_actorName = m_weaponDef.m_projectileActor;
		spawnInfo.m_actorPos = m_owner->GetEyePosition() - Vec3(0.f, 0.f, 1.f) * 0.15f;
		spawnInfo.m_actorOri = GetRandomDirectionInCone(m_weaponDef.m_projectileCone);
		spawnInfo.m_actorVel = spawnInfo.m_actorOri.GetForwardNormal() * m_weaponDef.m_projectSpeed;

		Actor* projectile = m_map->SpawnProjectile(spawnInfo);
		projectile->m_owner = m_owner;
		projectile->m_health = 1;
		
	}

	m_currentCapacity -= 1;
}

void Weapon::FireMelee()
{
	if (m_owner == nullptr || m_map == nullptr) return;

	Actor* closestTarget = m_map->GetClosestVisibleEnemy(m_owner);

	if (closestTarget)
	{
		FloatRange damageRange = m_weaponDef.m_meleeDamage;
		float damage = m_map->m_game->m_rng->RollRandomFloatInRange(damageRange.m_min, damageRange.m_max);
		closestTarget->Damage(damage, m_owner);

		closestTarget->AddImpulse(m_weaponDef.m_meleeImpulse * m_owner->m_orientation.GetForwardNormal());
	}
}

void Weapon::FireBlast()
{
	for (Actor* a : m_map->m_actors)
	{
		if (!a || a->IsDead()) continue;
		float dist = (m_owner->m_position - a->m_position).GetLengthXY();

		if (dist > m_weaponDef.m_blastRange) continue;

		float damage = m_map->m_game->m_rng->RollRandomFloatInRange(m_weaponDef.m_blastDamage.m_min, m_weaponDef.m_blastDamage.m_max);
		a->Damage(damage, m_owner);
	}

	m_owner->Die();
}

void Weapon::Reload()
{
	m_reloadTimer->Start();
	m_isReload = true;
}
