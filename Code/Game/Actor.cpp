#include "Game/Actor.hpp"
#include "Game/Game.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Game/Weapon.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Game/MapDefinition.hpp"

extern AudioSystem* g_theAudio;

Actor::Actor(Map* owner, std::string name, const Vec3& position, const EulerAngles& orientation, 
	Vec3 velocity, bool isStatic)
	: m_map(owner)
	, m_position(position)
	, m_orientation(orientation)
	, m_velocity(velocity)
	, m_isStatic(isStatic)
	, m_actorDef(ActorDefinition::GetDefinition(name))
{

	ActorFaction faction = m_actorDef.m_faction;
	if (m_actorDef.m_dieOnCollide)
	{
		m_color = Rgba8(20, 20, 100);
	}
	else if (faction == ActorFaction::DEMON)
	{
		m_color = Rgba8(100, 20, 20);
	}
	else if (faction == ActorFaction::MARINE)
	{
		m_color = Rgba8(20, 100, 20);
	}

	m_collider = ZCylinder3(Vec2::ZERO, 0.f, m_actorDef.m_physicsHeight, m_actorDef.m_physicsRadius);
	m_deadTimer = new Timer(m_actorDef.m_corpseLifetime, m_map->m_game->m_clock);
	InitializeVerts();
	InitializeWeapon();

	m_health = m_actorDef.m_health;
	m_animClock = new Clock(*m_map->m_game->m_clock);

	
	m_defaultAnimGroup = m_actorDef.m_animationGroups[0];
	m_currentAnimGroup = m_defaultAnimGroup;

	m_deathSound = g_theAudio->CreateOrGetSound(m_actorDef.GetSoundDef("Death").m_name);
	m_hurtSound = g_theAudio->CreateOrGetSound(m_actorDef.GetSoundDef("Hurt").m_name);
	m_crowdSound = g_theAudio->CreateOrGetSound(m_actorDef.GetSoundDef("Crowd").m_name);

	m_switchTimer = new Timer(0.5f, m_map->m_game->m_clock);

	if (m_actorDef.m_spawnPeriod > 0.f)
	{
		m_spawnTimer = new Timer(m_actorDef.m_spawnPeriod, m_map->m_game->m_clock);
		m_spawnTimer->ForceComplete();
	}
}

Actor::~Actor()
{
	m_map = nullptr;
	m_verts.clear();
	m_wireVerts.clear();

	m_owner = nullptr;
	
	delete m_deadTimer;
	m_deadTimer = nullptr;

	
	m_controller = nullptr;
	delete m_aiController;
	m_aiController = nullptr;

	for (Weapon* w : m_weapons)
	{
		delete w;
		w = nullptr;
	}
	m_weapons.clear();
	m_currentWeapon = nullptr;
	m_owner = nullptr;

	delete m_deadTimer;
	m_deadTimer = nullptr;

	if (m_controller)
	{
		m_controller->Unpossess();
		delete m_controller;
		m_controller = nullptr;
	}

	if (m_aiController)
	{
		m_aiController->Unpossess();
		delete m_aiController;
		m_aiController = nullptr;
	}

	delete m_controlledTimer;
	m_controlledTimer = nullptr;

	m_dragActor = nullptr;

	if (m_spawnTimer)
	{
		delete m_spawnTimer;
		m_spawnTimer = nullptr;
	}
}

void Actor::UpdatePhysics(float deltaSeconds)
{
	if (IsDead()) return;

	AddForce(-m_actorDef.m_drag * m_velocity);

	m_velocity += m_acceleration * deltaSeconds;
	m_position += m_velocity * deltaSeconds;
	
}

void Actor::EndUpdatePhysics()
{
	m_acceleration = Vec3::ZERO;
	
}

Mat44 Actor::GetModelToWorldTransform() const
{
	Mat44 modelMatrix;

	Mat44 translationMatrix = Mat44::MakeTranslation3D(m_position);
	Mat44 rotationMatrix = Mat44::MakeZRotationDegrees(m_orientation.m_yawDegrees);

	modelMatrix.Append(translationMatrix);
	modelMatrix.Append(rotationMatrix);

	return modelMatrix;
}

void Actor::InitializeVerts()
{
	Vec2 centerXY = m_collider.m_centerXY;
	FloatRange minMaxZ = FloatRange(m_collider.m_minZ, m_collider.m_maxZ);
	float radius = m_collider.m_radius;

	AddVertsForCylinderZ3D(m_verts, centerXY, minMaxZ, radius, 16, m_color);

	float lightPercent = 8.f;

	Rgba8 lightColor = Rgba8::ScaleColor(m_color, lightPercent);

	AddVertsForCylinderZ3D(m_wireVerts, centerXY, minMaxZ, radius, 16, lightColor);
}



void Actor::Update(float deltaSeconds)
{	

	if (m_currentAnimGroup && m_currentAnimGroup->m_scaleBySpeed)
	{
		float currentSpeed = m_velocity.GetLength();
		float runSpeed = m_actorDef.m_runSpeed;
		float scale = (runSpeed > 0.f) ? (currentSpeed / runSpeed) : 1.f;

		m_animClock->SetTimeScale(scale);
	}
	else
	{
		m_animClock->SetTimeScale(1.f);
	}

	if (m_currentAnimGroup)
	{
		float requiredSeconds = m_currentAnimGroup->GetSpriteAnimDef(Vec3(1.f, 0.f, 0.f))->GetAnimRequiredSeconds();
		float currentSeconds = m_animClock->GetTotalSeconds();

		if (requiredSeconds <= currentSeconds && !IsDead())
		{
			PlayAnimByName("Walk");
		}
	}
	
	if (m_spawnTimer && m_spawnTimer->DecrementPeriodIfElapsed())
	{
		m_spawnTimer->Stop();
	}


	if (m_controller)
	{
		m_controller->Update(deltaSeconds);
	}

	if (m_currentWeapon)
	{
		if (m_currentWeapon != m_nextWeapon)
		{
			if (m_switchTimer->GetElapsedFraction() > 0.5f)
			{
				m_currentWeapon = m_nextWeapon;
			}
		}

		if (m_switchTimer->DecrementPeriodIfElapsed())
		{
			m_isSwitching = false;
			m_switchTimer->Stop();
		}

		m_currentWeapon->Update(deltaSeconds);

		if (m_currentWeapon->m_currentAnim != m_currentWeapon->m_nextAnim)
		{
			m_currentWeapon->m_animAge = 0.f;
			m_currentWeapon->m_currentAnim = m_currentWeapon->m_nextAnim;
		}

		

		if (m_currentWeapon->m_currentAnim != "")
		{
			m_currentWeapon->m_animAge += deltaSeconds;


			WeaponAnimation weaponAnim = m_currentWeapon->m_weaponDef.m_hud.GetWeaponAnimation(m_currentWeapon->m_currentAnim);

			if (m_currentWeapon->m_currentAnim == "Attack" && m_currentWeapon->m_animAge > weaponAnim.m_secondsPerFrame * (weaponAnim.m_endFrame - weaponAnim.m_startFrame))
			{
				m_currentWeapon->m_nextAnim = "Idle";
			}
		}
	}

	UpdateBeControlled(deltaSeconds);

	if (m_actorDef.m_flying)
	{
		m_position.z = 1.f - m_actorDef.m_physicsHeight;
	}
	

	
}

void Actor::Render(Camera& camera) const
{
	PlayerController* playerController = dynamic_cast<PlayerController*>(m_controller);
	bool shouldRenderBody = true;

	if (playerController != nullptr)
	{
		if (!playerController->IsFreeCamera() && playerController->GetActor() == this)
		{
			shouldRenderBody = false;
		}
	}

	if (shouldRenderBody)
	{
		RenderBillboard(camera);
	}

	if (m_dragActor)
	{
		std::vector<Vertex_PCU> ropeVerts;
		Vec3 start = m_dragActor->m_position + Vec3(0.f, 0.f, 1.f) * m_dragActor->m_actorDef.m_physicsHeight * 0.35f +
			m_dragActor->m_orientation.GetForwardNormal() * 0.1f+
			m_dragActor->m_orientation.GetLeftNormal() * 0.09f;

		Vec3 end = m_position + Vec3(0.f, 0.f, 1.f) * m_actorDef.m_physicsHeight * 0.4f;

		AddVertsForCylinder3D(ropeVerts, start, end, 0.01f, Rgba8(50, 25, 0));
		g_theRenderer->DrawVertexArray(ropeVerts);
	}
}



void Actor::UpdateBeControlled(float deltaSeconds)
{
	if (m_controlledTimer && m_controlledTimer->DecrementPeriodIfElapsed())
	{
		EndDrag();
	}

	if (m_dragActor)
	{
		Vec3 toDragDir = (m_dragActor->m_position - m_position).GetNormalized();

		Vec3 dragForward = m_dragActor->m_orientation.GetForwardNormal();

		float facingDot = DotProduct3D(toDragDir, dragForward);

		if (facingDot > 0.f)
		{
			EndDrag();
		}
	}

	


	if (m_dragForce > 0.f)
	{
		Vec3 toTarget = m_dragPosition - m_position;
		float distance = toTarget.GetLength();

		if (distance > 0.5f) 
		{
			Vec3 direction = toTarget.GetNormalized();
			Vec3 dragVelocity = direction * (m_dragForce - m_actorDef.m_oppositeForce);

			m_position += dragVelocity * deltaSeconds;
		}
		PlayAnimByName("Hurt");
	}
}

void Actor::RenderSelf() const
{
	g_theRenderer->SetBlendMode(BlendMode::Blend_OPAQUE);

	Rgba8 color = Rgba8::WHITE;
	if (!m_actorDef.m_dieOnSpawn && IsDead())
	{
		color = Rgba8(50, 50, 50);
	}

	g_theRenderer->SetModelConstants(GetModelToWorldTransform(), color);

	g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
	g_theRenderer->DrawVertexArray(m_wireVerts);

	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->DrawVertexArray(m_verts);
}

void Actor::RenderBillboard(Camera& camera) const
{
	Mat44 modelMatrix = GetModelToWorldTransform();
	Mat44 renderMatrix = modelMatrix;
	if (m_actorDef.m_billboardType != BillboardType::NONE)
	{
		Vec3 cameraToActor = camera.GetPosition() - m_position;
		cameraToActor.z = 0.f;

		Mat44 billboardMatrix = GetBillboardMatrix(m_actorDef.m_billboardType, camera.GetCameraToWorldTransform(), m_position);

		billboardMatrix.m_values[8] = 0.f;
		billboardMatrix.m_values[9] = 0.f;
		billboardMatrix.m_values[10] = 1.f;

		renderMatrix = billboardMatrix;
	}

	Mat44 worldToActorTransform = modelMatrix.GetOrthonormalInverse();

	Vec3 facingDir = m_position - camera.GetPosition();
	facingDir.z = 0.f;

	if (facingDir.GetLengthSquared() < 0.0001f) {
		facingDir = Vec3(0.f, 1.f, 0.f);
	}
	else {
		facingDir = facingDir.GetNormalized();
	}

	Vec3 localViewDirection = worldToActorTransform.TransformVectorQuantity3D(facingDir);
	localViewDirection.z = 0.f;
	localViewDirection = localViewDirection.GetNormalized();

	if (!m_currentAnimGroup) return;

	SpriteAnimDefinition* animDef = m_currentAnimGroup->GetSpriteAnimDef(localViewDirection);

	SpriteDefinition const& spriteDef = animDef->GetSpriteDefAtTime(m_animClock->GetTotalSeconds());

	Vec2 pivotOffset = Vec2(m_actorDef.m_pivot.x * m_actorDef.m_visualSize.x, m_actorDef.m_pivot.y * m_actorDef.m_visualSize.y);
	Vec2 mins = Vec2(0.f, 0.f) - pivotOffset;
	Vec2 maxs = mins + m_actorDef.m_visualSize;

	float xLeft = mins.x;
	float xRight = maxs.x;
	float zBottom = mins.y;
	float zTop = maxs.y;

	std::vector<Vertex_PCUTBN> actorPCUTBNVerts;
	std::vector<Vertex_PCU> actorPCUVerts;

	Rgba8 color;

	if (m_slowFactor > 0.f && m_slowFactor < 1.f)
	{
		color = Rgba8(0, 127, 255);
	}
	else
	{
		color = Rgba8::WHITE;
	}

	if (m_actorDef.m_renderRounded)
	{
		
		AddVertsForRoundedQuad3D(actorPCUTBNVerts,
			Vec3(0.f, xLeft, zBottom),
			Vec3(0.f, xRight, zBottom),
			Vec3(0.f, xRight, zTop),
			Vec3(0.f, xLeft, zTop),
			color, spriteDef.GetUVs()
		);
	}
	else
	{
		AddVertsForQuad3D(
			actorPCUVerts,
			Vec3(0.f, xLeft, zBottom),
			Vec3(0.f, xRight, zBottom),
			Vec3(0.f, xRight, zTop),
			Vec3(0.f, xLeft, zTop),
			color, spriteDef.GetUVs()
		);
	}

	std::vector<Vertex_PCU> healthBarVerts;

	if (m_actorDef.m_renderHealthBar)
	{
		float hxLeft = -0.5f;
		float hxRight = 0.5f;
		float hzBottom = m_actorDef.m_physicsHeight + 0.2f;
		float hzTop = hzBottom + 0.04f;

		float healthFraction = GetClamped(m_health / m_actorDef.m_health, 0.f, m_actorDef.m_health);

		float hxMid = Interpolate(hxLeft, hxRight, healthFraction);


		AddVertsForQuad3D(
			healthBarVerts,
			Vec3(-0.01f, hxLeft - 0.01f, hzBottom - 0.01f),
			Vec3(-0.01f, hxRight + 0.01f, hzBottom - 0.01f),
			Vec3(-0.01f, hxRight + 0.01f, hzTop + 0.02f),
			Vec3(-0.01f, hxLeft - 0.01f, hzTop + 0.02f),
			Rgba8::BLACK);

		AddVertsForQuad3D(
			healthBarVerts,
			Vec3(0.f, hxLeft, hzBottom),
			Vec3(0.f, hxMid, hzBottom),
			Vec3(0.f, hxMid, hzTop),
			Vec3(0.f, hxLeft, hzTop),
			Rgba8::RED);

		
	}

	std::vector<Vertex_PCU> bossVerts;

	if (m_actorDef.m_isBoss)
	{
		float bxLeft = -0.2f;
		float bxRight = 0.2f;
		float bzBottom = m_actorDef.m_physicsHeight + 0.25f;
		float bzTop = bzBottom + 0.4f;

		AddVertsForQuad3D(
			bossVerts,
			Vec3(0.f, bxLeft, bzBottom),
			Vec3(0.f, bxRight, bzBottom),
			Vec3(0.f, bxRight, bzTop),
			Vec3(0.f, bxLeft, bzTop),
			Rgba8::WHITE);
	}



	g_theRenderer->SetModelConstants(renderMatrix);
	g_theRenderer->BindTexture(&spriteDef.GetTexture());
	//g_theRenderer->BindTexture(nullptr);

	g_theRenderer->BindShader(m_actorDef.m_shader);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);

	g_theRenderer->DrawVertexArray(actorPCUVerts);
	g_theRenderer->DrawVertexArray(actorPCUTBNVerts);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->BindShader(g_theRenderer->GetShader("Default"));
	g_theRenderer->DrawVertexArray(healthBarVerts);

	Texture* skull = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Skull.png");
	g_theRenderer->BindTexture(skull);
	g_theRenderer->DrawVertexArray(bossVerts);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetModelConstants();

}



bool Actor::IsPlayer() const
{
	ActorHandle handle1 = m_map->m_game->GetPlayerByIndex(0)->m_handle;
	ActorHandle handle2 = m_map->m_game->GetPlayerByIndex(1)->m_handle;

	return handle1 == m_handle || handle2 == m_handle;
}

void Actor::Damage(float amount, Actor* enemy)
{

	m_health -= amount;

	PlayAnimByName("Hurt");
	g_theAudio->StartSoundAt(m_hurtSound, m_position, false, 1.f);

	if (m_health <= 0.f)
	{
		m_health = 0.f;
		PlayAnimByName("Death");
	}

	if (IsDead() && m_deadTimer->IsStopped())
	{
		m_deadTimer->Start();

		if (m_actorDef.m_name == "Marine")
		{
			PlayerController* playerController = dynamic_cast<PlayerController*> (m_controller);
			if (playerController)
			{
				playerController->HandleDeadCamera();
				m_map->m_totalDeath++;
			}
			
		}
		
		if (enemy->m_actorDef.m_name == "Marine")
		{
			PlayerController* playerController = dynamic_cast<PlayerController*> (enemy->m_controller);
			playerController->m_playerKillCount++;
		}
	}

	if (m_aiController)
	{
		m_aiController->DamagedBy(enemy);
	}


}

void Actor::CrowdControl(float slowFactor, float controlPeriod, float dragForce, Actor* enemy)
{
	g_theAudio->StartSoundAt(m_crowdSound, m_position, false, 1.f);

	m_isControlled = true;
	if (m_controlledTimer)
	{
		delete m_controlledTimer;
	}
	if (dragForce > 0.f)
	{
		float dragDistance = GetDistance3D(m_position, enemy->m_position);
		float dragPeriod = dragDistance / (dragForce - m_actorDef.m_oppositeForce) - 0.2f;
		m_controlledTimer = new Timer(dragPeriod, m_map->m_game->m_clock);
	}
	else
	{
		m_controlledTimer = new Timer(controlPeriod, m_map->m_game->m_clock);
	}
	
	m_controlledTimer->Start();
	m_slowFactor = slowFactor;
	m_dragForce = dragForce;
	m_dragPosition = enemy->m_position;
	if (m_dragForce > 0.f)
	{
		m_dragActor = enemy;
		Vec3 toEnemyDir = (enemy->m_position - m_position).GetNormalized();
		m_orientation.m_yawDegrees = toEnemyDir.GetAngleAboutZDegrees();
	}
	

	

	if (m_aiController)
	{
		m_aiController->DamagedBy(enemy);
	}
}

void Actor::AddForce(const Vec3& force)
{
	m_acceleration += force;
}

void Actor::AddImpulse(const Vec3& impulse)
{
	m_velocity += impulse;
}

void Actor::OnCollide(Actor* other, float impactImpulse)
{
	if (other == nullptr || other->IsDead() || IsDead()) return;

	if (IsProjectile() && other->m_handle != m_owner->m_handle)
	{
		FloatRange damageRange = m_actorDef.m_damageOnCollide;
		float damage = m_map->m_game->m_rng->RollRandomFloatInRange(damageRange.m_min, damageRange.m_max);
		other->Damage(damage, m_owner);
		if (m_actorDef.m_isCrowdControl)
		{
			other->CrowdControl(m_actorDef.m_slowFactor, m_actorDef.m_controlPeriod, m_actorDef.m_dragForce, m_owner);
		}

		Die();

		Vec3 impulseDir = m_orientation.GetForwardNormal();
		Vec3 impulse = impactImpulse * impulseDir.IgnoreZ();
		other->AddImpulse(impulse);
	}

	
}

void Actor::OnPossessed(Controller* newController)
{
	m_controller = newController;
}

void Actor::OnUnpossessed()
{
	if (m_controller != nullptr)
	{
		m_controller = nullptr; 
	}

	if (m_aiController != nullptr)
	{
		m_controller = m_aiController; 
		m_aiController->Possess(this);
	}

}

void Actor::MoveInDirection(float deltaSeconds, const Vec3& direction, bool isRun, float slowFactor)
{
	const float speed = isRun ? m_actorDef.m_runSpeed : m_actorDef.m_walkSpeed;
	const Vec3 desiredVelocity = direction * speed;

	const Vec3 force = (desiredVelocity - m_velocity) * deltaSeconds * 3000.f * slowFactor;

	AddForce(force);
}

void Actor::TurnInDirection(const EulerAngles& direction, float maxTurnDegrees, float slowFactor)
{
	float targetYawDegrees = direction.m_yawDegrees;

	float currentYawDegrees = m_orientation.m_yawDegrees;

	float yawAngularDisp = GetShortestAngularDispDegrees(currentYawDegrees, targetYawDegrees);

	float yawClampedAngularDisp = GetClamped(yawAngularDisp, -maxTurnDegrees, maxTurnDegrees);


	m_orientation.m_yawDegrees += yawClampedAngularDisp * slowFactor;


	float targetPitchDegrees = direction.m_pitchDegrees;

	float currentPitchDegrees = m_orientation.m_pitchDegrees;

	float pitchAngularDisp = GetShortestAngularDispDegrees(currentPitchDegrees, targetPitchDegrees);

	float pitchClampedAngularDisp = GetClamped(pitchAngularDisp, -maxTurnDegrees, maxTurnDegrees);

	m_orientation.m_pitchDegrees += pitchClampedAngularDisp * slowFactor;
}



void Actor::Attack()
{
	m_currentWeapon->Fire();
}

void Actor::EquipWeapon(int weaponIndex)
{
	if (weaponIndex < 0 || weaponIndex >= (int)m_weapons.size())
	{
		ERROR_AND_DIE("Invalid Weapon Index!");
	}

	if (m_weapons[weaponIndex] == m_nextWeapon) return;
	m_isSwitching = true;
	m_switchTimer->Start();
	m_nextWeapon = m_weapons[weaponIndex];
}

void Actor::InitializeWeapon()
{
	for (std::string weaponName : m_actorDef.m_weaponsNames)
	{
		Weapon* weapon = new Weapon(weaponName, m_map, this);
		m_weapons.push_back(weapon);
	}
	
	if ((int)m_weapons.size() > 0)
	{
		m_currentWeapon = m_weapons[0];
		m_nextWeapon = m_currentWeapon;
	}

}

Vec3 Actor::GetEyePosition() const
{
	return m_position + Vec3(0.f, 0.f, 1.f) * m_actorDef.m_eyeHeight
		 + m_orientation.GetForwardNormal() * (m_actorDef.m_physicsRadius + 0.2f);
}

void Actor::Die()
{
	m_health = 0;
	m_deadTimer->Start();
	PlayAnimByName("Death");

	g_theAudio->StartSoundAt(m_deathSound, m_position, false, 0.5f);
	
	if (m_currentWeapon && m_currentWeapon->m_weaponDef.m_blastRange > 0.f)
	{
		m_currentWeapon->Fire();
	}

}

bool Actor::CanBePossessed() const
{
	return !IsDead() && !IsGarbage() && m_actorDef.m_canBePossessed;
}

void Actor::SelectPrevWeapon()
{
	if (m_weapons.empty()) return;

	m_weaponIndex--;
	if (m_weaponIndex < 0)
	{
		m_weaponIndex = (int)m_weapons.size() - 1;
	}

	m_currentWeapon = m_weapons[m_weaponIndex];
}

void Actor::SelectNextWeapon()
{
	if (m_weapons.empty()) return;

	m_weaponIndex = (m_weaponIndex + 1) % (int)m_weapons.size();
	m_currentWeapon = m_weapons[m_weaponIndex];

}

void Actor::PlayAnimByName(std::string animGroupName)
{
	if (m_currentAnimGroup != m_actorDef.GetAnimGroupDef(animGroupName))
	{
		m_currentAnimGroup = m_actorDef.GetAnimGroupDef(animGroupName);
		m_animClock->Reset();
	}
	
}

void Actor::EndDrag()
{
	m_controlledTimer->Stop();
	m_isControlled = false;
	m_slowFactor = 1.f;
	m_dragForce = 0.f;
	delete m_controlledTimer;
	m_controlledTimer = nullptr;
	m_dragPosition = Vec3::ZERO;
	PlayAnimByName("Walk");
	m_dragActor = nullptr;
	return;
}

void Actor::SpawnFriends()
{
	int num = m_actorDef.m_spawnNum;

	for (int i = 0; i < num; ++i)
	{
		SpawnInfo info;
		info.m_actorName = m_actorDef.m_spawnActor;
		Vec2 validPos = m_map->GetValidSpawnFriendPos(m_position);
		info.m_actorPos = Vec3(validPos.x, validPos.y, 0.f);
		info.m_actorOri = m_orientation;

		Actor* actor = m_map->SpawnActor(info);
		if (m_aiController->m_target)
		{
			actor->m_aiController->m_target = m_aiController->m_target;
		}
	}
	m_spawnTimer->Start();

}



