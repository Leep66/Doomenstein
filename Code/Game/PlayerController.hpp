#pragma once
#include "Game/Controller.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Game/AIController.hpp"
#include <vector>

class Map;
struct ActorHandle;

enum CameraMode
{
	FREEFLY,
	POSSESSED
};

enum ControlType
{
	NONE = -1,
	KEYBOARD,
	CONTROLLER,

	COUNT
};

struct PlayerInfo
{
	int m_playerIndex = -1;
	ControlType m_control = NONE;
	bool m_isJoined = false;
};

class PlayerController : public Controller
{
public:
	PlayerController();
	PlayerController(ActorHandle actorHandle, Map* map);

	void SetupPosAndOri(Vec3 const& position, EulerAngles const& orientation);

	void Possess(Actor* actor) override;
	void Unpossess() override;

	void SetPosition(Vec3 position) { m_position = position; }
	void SetOrientation(EulerAngles orientation) { m_orientation = orientation; }

	bool IsPlayerController() const override { return true; }
	bool IsFreeCamera() const { return m_mode == CameraMode::FREEFLY; }

	void Update(float deltaSeconds) override;
	void UpdateDeadCamera();

	void UpdateInput(float deltaSeconds);
	void UpdateCamera(float deltaSeconds);

	void MoveCameraByKeyboard(float deltaSeconds);
	void RotateCameraByMouse();

	void MoveCameraByController(float deltaSeconds);
	void RotateCameraByController();

	void MoveActorByKeyboard(float deltaSeconds, Actor* actor);
	void RotateActorByMouse(float deltaSeconds, Actor* actor);

	void MoveActorByController(float deltaSeconds, Actor* actor);
	void RotateActorByController(float deltaSeconds, Actor* actor);

	Camera GetCamera() const { return m_camera; }

	void HandleDeadCamera();

	Vec3 GetFwdNormal() const;

public: 
	Camera m_camera;
	Vec3 m_position = Vec3::ZERO;
	Vec3 m_velocity = Vec3::ZERO;
	float m_moveSpeed = 1.f;
	float m_sprintMultiplier = 15.0f;
	float m_sensitivity = 0.075f;

	EulerAngles m_orientation;
	CameraMode m_mode = CameraMode::FREEFLY;
	bool m_deadCamera = false;

	PlayerInfo m_playerInfo;

	int m_playerKillCount = 0;

};