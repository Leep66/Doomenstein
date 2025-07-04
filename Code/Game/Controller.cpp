#include "Game/Controller.hpp"
#include "Game/Actor.hpp"
#include "Game/ActorHandle.hpp"
#include "Game/Map.hpp"
#include "Engine/Core/EngineCommon.hpp"

Controller::Controller(ActorHandle actorHandle, Map* map)
	: m_handle(actorHandle)
	, m_map(map)
{
}

void Controller::Possess(Actor* actor)
{
	UNUSED(actor);
}

void Controller::Unpossess()
{
}

void Controller::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
}

Actor* Controller::GetActor() const
{
	Actor* actor = m_map->GetActorByHandle(m_handle);
	return actor;
}

