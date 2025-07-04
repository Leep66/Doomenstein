#pragma once
#include "Game/Controller.hpp"
#include "Game/ActorHandle.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Game/Map.hpp"

class Actor;

class AIController : public Controller
{
public:
	AIController() = default;
	AIController(ActorHandle actorHandle, Map* map);

	void Possess(Actor* actor) override;
	void Unpossess() override;
	bool IsPlayerController() const override { return false; }
	void Pathfind();

	void Update(float deltaSeconds) override;
	
	void DamagedBy(Actor* player);
	void FindTarget();


public:
	Actor* m_target = nullptr;
	bool test = false;
	std::vector<IntVec2> m_path;
	IntVec2 m_targetPos;

};