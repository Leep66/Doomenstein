#pragma once
#include "Game/ActorHandle.hpp"

class Map;
class Actor;



class Controller
{
public:
	ActorHandle m_handle = ActorHandle::INVALID;
	Map* m_map = nullptr;
	

public:
	Controller() = default;
	Controller(ActorHandle actorHandle, Map* map);

	virtual void Possess(Actor* actor);
	virtual void Unpossess();

	virtual void Update(float deltaSeconds);

	virtual bool IsPlayerController() const { return false; }
	Actor* GetActor() const;
};