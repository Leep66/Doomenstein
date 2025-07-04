#include "AIController.hpp"
#include "Game/Actor.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Game/Weapon.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Game/Game.hpp"

extern Renderer* g_theRenderer;

AIController::AIController(ActorHandle actorHandle, Map* map)
	: Controller(actorHandle, map)
{
}

void AIController::Possess(Actor* actor)
{
	m_handle = actor->m_handle;
	actor->m_aiController = this;
	actor->m_controller = this;
}

void AIController::Unpossess()
{
	Actor* actor = m_map->GetActorByHandle(m_handle);
	if (actor != nullptr)
	{
		actor->m_controller = nullptr;
		actor->m_aiController = nullptr;
	}
	m_handle = ActorHandle::INVALID;
}

void AIController::Pathfind()
{
	Actor* actor = m_map->GetActorByHandle(m_handle);

	IntVec2 startPos = actor->GetTileCoords();
	m_targetPos = m_target->GetTileCoords();

	std::vector<Node*> openList;
	std::vector<Node*> closedList;

	Node* startNode = new Node{ startPos, nullptr, 0.f, (float)GetTaxicabDistance2D(startPos, m_targetPos) };
	openList.push_back(startNode);

	while (!openList.empty())
	{
		Node* current = openList[0];
		for (Node* node : openList)
		{
			if (node->fCost() < current->fCost())
				current = node;
		}

		if (current->position == m_targetPos)
		{
			m_path.clear();
			while (current != nullptr)
			{
				m_path.push_back(current->position);
				current = current->parent;
			}
			std::reverse(m_path.begin(), m_path.end());
			break;
		}

		for (int i = 0; i < (int)openList.size(); ++i)
		{
			if (openList[i] == current)
			{
				openList.erase(openList.begin() + i);
				break;
			}
		}
		closedList.push_back(current);

		const IntVec2 directions[4] = { IntVec2(1,0), IntVec2(-1,0), IntVec2(0,1), IntVec2(0,-1) };
		for (const IntVec2& dir : directions)
		{
			IntVec2 neighborPos = current->position + dir;

			if (!m_map->IsTileWalkable(neighborPos)) continue;

			bool alreadyClosed = false;
			for (Node* node : closedList)
			{
				if (node->position == neighborPos)
				{
					alreadyClosed = true;
					break;
				}
			}
			if (alreadyClosed) continue;

			Node* neighbor = nullptr;
			for (Node* node : openList)
			{
				if (node->position == neighborPos)
				{
					neighbor = node;
					break;
				}
			}

			float tentativeG = current->gCost + 1.f;

			if (neighbor)
			{
				if (tentativeG < neighbor->gCost)
				{
					neighbor->gCost = tentativeG;
					neighbor->parent = current;
				}
			}
			else
			{
				neighbor = new Node{
					neighborPos,
					current,
					tentativeG,
					(float)GetTaxicabDistance2D(neighborPos, m_targetPos)
				};
				openList.push_back(neighbor);
			}
		}
	}

	for (Node* node : openList) delete node;
	for (Node* node : closedList) delete node;
}


void AIController::Update(float deltaSeconds)
{
	Actor* aiActor = m_map->GetActorByHandle(m_handle);

	if (aiActor->IsDead()) return;
	if (!m_target)
	{
		FindTarget();
		if (!m_target) return;
	}



	if (m_target->IsDead() || !aiActor || aiActor->IsDead())
	{
		m_target = nullptr;
		m_path.clear();
		m_targetPos = IntVec2::ZERO;
		//if (aiActor) aiActor->PlayAnimByName("Walk");

		return;
	}

	if (aiActor->m_slowFactor == 0.f || aiActor->IsDead()) return;

	if (m_path.empty() || m_targetPos != m_target->GetTileCoords())
	{
		Pathfind();
	}

	Vec3 eyePos = aiActor->GetEyePosition();
	const ZCylinder3& col = m_target->GetCollider();
	float targetZ = (col.m_minZ + col.m_maxZ) * 0.5f;
	Vec3 targetMid = Vec3(m_target->m_position.x, m_target->m_position.y, targetZ);
	Vec3 dir = (targetMid - eyePos).GetNormalized();

	Actor* hitActor = nullptr;
	RaycastResult3D result = m_map->RaycastAllIgnoreFriend(
		eyePos, dir,
		aiActor->m_actorDef.m_sightRadius,
		&hitActor,
		aiActor->m_actorDef.m_faction
	);

	

	float distanceToTarget = (m_target->m_position - aiActor->m_position).GetLength();
	float attackRange = aiActor->m_actorDef.m_attackRange;
	float targetYaw = dir.GetAngleAboutZDegrees();
	EulerAngles targetEuler(targetYaw, 0.f, 0.f);

	Weapon* weapon = aiActor->m_currentWeapon;

	if (hitActor == m_target)
	{
		if (aiActor->m_spawnTimer && aiActor->m_spawnTimer->IsStopped())
		{
			aiActor->SpawnFriends();
		}

		if (weapon && distanceToTarget <= attackRange)
		{
			weapon->Fire();
			aiActor->TurnInDirection(targetEuler, aiActor->m_actorDef.m_turnSpeed * deltaSeconds, aiActor->m_slowFactor);
			aiActor->m_velocity = Vec3::ZERO;
			return;
		}
		if (distanceToTarget > attackRange)
		{
			aiActor->TurnInDirection(targetEuler, aiActor->m_actorDef.m_turnSpeed * deltaSeconds, aiActor->m_slowFactor);
			aiActor->MoveInDirection(deltaSeconds, aiActor->m_orientation.GetForwardNormal(), true, aiActor->m_slowFactor);
			return;
		}
	}
	else if (hitActor != nullptr && hitActor != m_target && hitActor->m_actorDef.m_faction == ActorFaction::MARINE)
	{
		if (!hitActor->IsDead() && !hitActor->IsGarbage())
		{
			m_target = hitActor;
			m_path.clear();
			m_targetPos = IntVec2::ZERO;
			return;
		}
	}

	if (m_path.empty())
	{
		Pathfind();
	}
	else
	{
		IntVec2 nextTile = m_path.front();
		Vec2 nextPos = m_map->GetTileCenter(nextTile);
		Vec2 toNext = nextPos - aiActor->GetPosition().GetXY();

		if (toNext.GetLength() < 0.5f)
		{
			m_path.erase(m_path.begin());
		}
		else
		{
			float nextYaw = toNext.GetOrientationDegrees();
			EulerAngles nextEuler(nextYaw, 0.f, 0.f);

			float angleDiff = GetShortestAngularDispDegrees(aiActor->m_orientation.m_yawDegrees, nextYaw);
			bool  isFacing = fabsf(angleDiff) < 30.f;

			aiActor->TurnInDirection(nextEuler, aiActor->m_actorDef.m_turnSpeed * deltaSeconds, aiActor->m_slowFactor);
			if (isFacing)
			{
				aiActor->MoveInDirection(deltaSeconds, aiActor->m_orientation.GetForwardNormal(), true, aiActor->m_slowFactor);
			}
			else
			{
				aiActor->m_velocity = Vec3::ZERO;
			}
		}
	}
}


void AIController::DamagedBy(Actor* player)
{
	if (player->m_actorDef.m_faction == ActorFaction::MARINE)
	{
		m_target = player;
	}

}

void AIController::FindTarget()
{
	Actor* aiActor = m_map->GetActorByHandle(m_handle);
	if (!aiActor) return;

	if (m_map->m_exposureTimer->IsStopped())
	{
		if (!aiActor->IsDead() && aiActor->m_actorDef.m_faction == ActorFaction::DEMON)
		{
			int randomInt = 0;
			if (m_map->m_game->IsMultiplayer())
			{
				randomInt = m_map->m_game->m_rng->RollRandomIntInRange(0, 1);
			}

			Actor* playerActor = m_map->m_game->GetPlayerByIndex(randomInt)->GetActor();

			aiActor->m_aiController->m_target = playerActor;
		}
	}

	const ActorDefinition& def = aiActor->m_actorDef;
	const float viewDist = def.m_sightRadius;
	const float fovCos = CosDegrees(def.m_sightAngle * 0.5f);

	const Vec3 eyePos = aiActor->GetEyePosition() + aiActor->m_orientation.GetForwardNormal() * (aiActor->m_actorDef.m_physicsRadius + 0.1f);
	const Vec3 forward = aiActor->m_orientation.GetForwardNormal();

	Actor* bestTarget = nullptr;
	float bestDist = viewDist;

	for (PlayerController* player : m_map->m_game->m_players)
	{
		Actor* other = player->GetActor();
		if (!other || other == aiActor || other->IsDead() || other->IsGarbage())
			continue;
		if (!m_map->AreActorsEnemies(aiActor, other) || other->IsProjectile())
			continue;

		const ZCylinder3& col = other->GetCollider();
		Vec3 targetMid = Vec3(other->m_position.x, other->m_position.y, (col.m_minZ + col.m_maxZ) * 0.5f);

		Vec3 toTarget = targetMid - eyePos;
		float dist = toTarget.GetLength();
		if (dist > viewDist)
			continue;

		Vec3 dirToTarget = toTarget / dist;
		if (DotProduct3D(forward, dirToTarget) < fovCos)
			continue;

		Actor* hitActor = nullptr;
		RaycastResult3D result = m_map->RaycastAll(eyePos, dirToTarget, viewDist, &hitActor);
		if (result.m_didImpact && hitActor == other && dist < bestDist)
		{
			bestTarget = other;
			bestDist = dist;
		}
	}

	m_target = bestTarget;
}




