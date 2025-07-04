#include "Game/PlayerController.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Game/Map.hpp"
#include "Game/Actor.hpp"
#include "Game/ActorHandle.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/Weapon.hpp"
#include "Game/Game.hpp"

extern Window* g_theWindow;
extern InputSystem* g_theInput;

PlayerController::PlayerController()
	: Controller()
{
	/*Actor* possessedActor = map->GetActorByHandle(actorHandle);
	m_position = possessedActor->GetPosition();
	m_velocity = Vec3::ZERO;
	m_orientation = possessedActor->GetOrientation();*/

	Mat44 mat;
	mat.SetIJK3D(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
	m_camera.SetCameraToRenderTransform(mat);

	
}

PlayerController::PlayerController(ActorHandle actorHandle, Map* map)
	: Controller(actorHandle, map)
{
	Actor* possessedActor = map->GetActorByHandle(actorHandle);
	m_position = possessedActor->GetPosition();
	m_velocity = Vec3::ZERO;
	m_orientation = possessedActor->GetOrientation();

	Mat44 mat;
	mat.SetIJK3D(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
	m_camera.SetCameraToRenderTransform(mat);

	/*float fov = 60.f;
	float aspect = g_theWindow->GetAspect();
	float zNear = 0.1f;
	float zFar = 100.f;
	m_camera.SetMode(Camera::eMode_Perspective);
	m_camera.SetPerspectiveView(aspect, fov, zNear, zFar);*/
}

void PlayerController::SetupPosAndOri(Vec3 const& position, EulerAngles const& orientation)
{
	SetPosition(position);
	SetOrientation(orientation);

	m_camera.SetPosition(position);
	m_camera.SetOrientation(orientation);
}

void PlayerController::Possess(Actor* actor)
{
	float fov = actor->m_actorDef.m_cameraFOVDegrees;
	float aspect = g_theWindow->GetAspect() * 2.f;
	float zNear = 0.1f;
	float zFar = 100.f;
	m_camera.SetMode(Camera::eMode_Perspective);
	m_camera.SetPerspectiveView(aspect, fov, zNear, zFar);

	if (actor == nullptr)
	{
		return;
	}

	if (m_handle.IsValid())
	{
		Unpossess();
	}

	m_handle = actor->m_handle;
	actor->OnPossessed(this);
	m_position = actor->m_position + Vec3(0.f, 0.f, 1.f) * actor->m_actorDef.m_eyeHeight;
	m_orientation = actor->m_orientation;

	m_camera.SetPosition(m_position);
	m_camera.SetOrientation(m_orientation);

	m_mode = POSSESSED;
	m_deadCamera = false;
}

void PlayerController::Unpossess()
{
	if (!m_handle.IsValid())
		return;

	Actor* actor = m_map->GetActorByHandle(m_handle);

	if (actor)
	{
		actor->OnUnpossessed();

	}

	m_camera.SetPosition(Vec3::ZERO);

	m_mode = FREEFLY;
}

void PlayerController::Update(float deltaSeconds)
{
	if (m_mode == FREEFLY)
	{
		deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();
	}

	if (GetActor())
	{
		UpdateInput(deltaSeconds);
		UpdateCamera(deltaSeconds);
	}
	
}

void PlayerController::UpdateDeadCamera()
{
	Actor* actor = m_map->GetActorByHandle(m_handle);

	float t = actor->m_deadTimer->GetElapsedFraction();

	float start = m_position.z;
	float end = m_position.z - actor->m_actorDef.m_eyeHeight;

	float posZ = Interpolate(start, end, t);
	m_camera.SetPosition(Vec3(m_position.x, m_position.y, posZ));

}

void PlayerController::UpdateInput(float deltaSeconds)
{
	if (g_theInput->WasKeyJustPressed('F'))
	{
		m_mode = m_mode == FREEFLY ? POSSESSED : FREEFLY;
	}

	if (m_map->m_game->IsMultiplayer())
	{
		m_mode = POSSESSED;
	}
	
	if (m_mode == POSSESSED)
	{
		XboxController controller = g_theInput->GetController(0);

		Actor* actorSelf = m_map->GetActorByHandle(m_handle);

		if (actorSelf->IsDead()) return;
		if (actorSelf->m_isControlled && actorSelf->m_slowFactor == 0.f) return;

		if (m_playerInfo.m_control == KEYBOARD)
		{
			
			MoveActorByKeyboard(deltaSeconds, actorSelf);
			RotateActorByMouse(deltaSeconds, actorSelf);

			if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE) && actorSelf->m_currentWeapon->m_currentCapacity != 0 && !actorSelf->m_currentWeapon->m_isReload)
			{
				actorSelf->m_currentWeapon->Fire();
			}

			if (actorSelf->m_actorDef.m_name == "Marine")
			{
				if (g_theInput->WasKeyJustPressed('1'))
				{
					actorSelf->EquipWeapon(0);
				}

				if (g_theInput->WasKeyJustPressed('2'))
				{
					actorSelf->EquipWeapon(1);
				}

				if (g_theInput->WasKeyJustPressed('3'))
				{
					actorSelf->EquipWeapon(2);
				}

				if (g_theInput->WasKeyJustPressed('4'))
				{
					actorSelf->EquipWeapon(3);
				}

				if (g_theInput->WasKeyJustPressed('5'))
				{
					actorSelf->EquipWeapon(4);
				}

				if (g_theInput->WasKeyJustPressed(KEYCODE_LEFTARROW))
				{
					actorSelf->SelectPrevWeapon();
				}

				if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHTARROW))
				{
					actorSelf->SelectNextWeapon();
				}

				if (g_theInput->WasKeyJustPressed('R') || actorSelf->m_currentWeapon->m_currentCapacity == 0)
				{
					if (!actorSelf->m_currentWeapon->m_isReload && 
						actorSelf->m_currentWeapon->m_currentCapacity != 
						actorSelf->m_currentWeapon->m_weaponDef.m_maxCapacity
						&& actorSelf->m_currentWeapon->m_weaponDef.m_maxCapacity > -1)
					{
						actorSelf->m_currentWeapon->Reload();
					}
				}
			}
			

		}
		else if (m_playerInfo.m_control == CONTROLLER)
		{

			MoveActorByController(deltaSeconds, actorSelf);
			RotateActorByController(deltaSeconds, actorSelf);

			if (controller.GetRightTrigger() > 0.5f && actorSelf->m_currentWeapon->m_currentCapacity > 0 && !actorSelf->m_currentWeapon->m_isReload)
			{
				actorSelf->m_currentWeapon->Fire();
			}

			if (actorSelf->m_actorDef.m_name == "Marine")
			{
				if (controller.WasButtonJustPressed(XBOX_BUTTON_X))
				{
					actorSelf->EquipWeapon(0);
				}

				if (controller.WasButtonJustPressed(XBOX_BUTTON_Y))
				{
					actorSelf->EquipWeapon(1);
				}

				if (controller.WasButtonJustPressed(XBOX_BUTTON_DPAD_UP))
				{
					actorSelf->SelectPrevWeapon();
				}

				if (controller.WasButtonJustPressed(XBOX_BUTTON_DPAD_DOWN))
				{
					actorSelf->SelectNextWeapon();
				}
			}

			if (controller.WasButtonJustPressed(XBOX_BUTTON_B) || actorSelf->m_currentWeapon->m_currentCapacity == 0)
			{
				if (!actorSelf->m_currentWeapon->m_isReload &&
					actorSelf->m_currentWeapon->m_currentCapacity !=
					actorSelf->m_currentWeapon->m_weaponDef.m_maxCapacity)
				{
					actorSelf->m_currentWeapon->Reload();
				}
			}
		}
		
	}


	

}

void PlayerController::UpdateCamera(float deltaSeconds)
{
	float fov = m_map->GetActorByHandle(m_handle)->m_actorDef.m_cameraFOVDegrees;
	float ratio = m_map->m_game->IsMultiplayer() ? 2.f : 1.f;
	float aspect = g_theWindow->GetAspect() * ratio;
	float zNear = 0.1f;
	float zFar = 100.f;
	m_camera.SetMode(Camera::eMode_Perspective);
	m_camera.SetPerspectiveView(aspect, fov, zNear, zFar);

	if (m_mode == FREEFLY)
	{
		MoveCameraByKeyboard(deltaSeconds);
		RotateCameraByMouse();

		MoveCameraByController(deltaSeconds);
		RotateCameraByController();
	}
	else
	{
		if (m_deadCamera)
		{
			UpdateDeadCamera();
			return;
		}

		Actor* possessedActor = m_map->GetActorByHandle(m_handle);
		float eyeHeight = possessedActor->m_actorDef.m_eyeHeight;

		SetupPosAndOri(possessedActor->m_position + Vec3(0.f, 0.f, 1.f) * eyeHeight, possessedActor->m_orientation);
	}


}

void PlayerController::MoveCameraByKeyboard(float deltaSeconds)
{
	Vec3 cameraFwd, cameraLeft, cameraUp;

	m_orientation.GetAsVectors_IFwd_JLeft_KUp(cameraFwd, cameraLeft, cameraUp);

	Vec3 moveDirection;

	cameraUp = Vec3(0, 0, 1);

	if (g_theInput->IsKeyDown('W')) moveDirection += cameraFwd;
	if (g_theInput->IsKeyDown('S')) moveDirection -= cameraFwd;

	if (g_theInput->IsKeyDown('A')) moveDirection += cameraLeft;
	if (g_theInput->IsKeyDown('D')) moveDirection -= cameraLeft;

	if (g_theInput->IsKeyDown('Z')) moveDirection -= cameraUp;
	if (g_theInput->IsKeyDown('C')) moveDirection += cameraUp;

	if (moveDirection != Vec3())
	{
		moveDirection = moveDirection.GetNormalized();
	}

	float speed = m_moveSpeed;
	if (g_theInput->IsKeyDown(KEYCODE_SHIFT))
	{
		speed *= m_sprintMultiplier;
	}

	m_position += moveDirection * speed * deltaSeconds;
	m_camera.SetPosition(m_position);
}

void PlayerController::RotateCameraByMouse()
{
	Vec2 mouseDelta = g_theInput->GetCursorClientDelta();

	m_orientation.m_yawDegrees -= mouseDelta.x * m_sensitivity;

	m_orientation.m_pitchDegrees += mouseDelta.y * m_sensitivity;

	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.0f, 85.0f);

	/*
	float rollDelta = 0.0f;
	if (g_theInput->IsKeyDown('Q')) rollDelta -= m_rollSpeed * deltaSeconds;
	if (g_theInput->IsKeyDown('E')) rollDelta += m_rollSpeed * deltaSeconds;

	m_orientation.m_rollDegrees += rollDelta;

	m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -45.0f, 45.0f);
	*/

	m_camera.SetOrientation(m_orientation);
}

void PlayerController::MoveCameraByController(float deltaSeconds)
{
	XboxController controller = g_theInput->GetController(0);
	Vec3 cameraFwd, cameraLeft, cameraUp;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(cameraFwd, cameraLeft, cameraUp);

	Vec3 moveDirection;

	cameraUp = Vec3(0, 0, 1);

	Vec2 leftStick = controller.GetLeftStick().GetPosition();
	moveDirection += cameraFwd * leftStick.y;
	moveDirection -= cameraLeft * leftStick.x;

	if (controller.IsButtonDown(XBOX_BUTTON_LB))
	{
		moveDirection -= cameraUp;
	}

	if (controller.IsButtonDown(XBOX_BUTTON_RB))
	{
		moveDirection += cameraUp;
	}

	if (moveDirection != Vec3())
	{
		moveDirection = moveDirection.GetNormalized();
	}

	float speed = m_moveSpeed;
	if (controller.IsButtonDown(XBOX_BUTTON_A))
	{
		speed *= m_sprintMultiplier;
	}

	m_position += moveDirection * speed * deltaSeconds;
	m_camera.SetPosition(m_position);
}

void PlayerController::RotateCameraByController()
{
	XboxController controller = g_theInput->GetController(0);
	Vec2 rightStick = controller.GetRightStick().GetPosition();
	m_orientation.m_yawDegrees -= rightStick.x;
	m_orientation.m_pitchDegrees -= rightStick.y;

	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.0f, 85.0f);

	/*float leftTrigger = controller.GetLeftTrigger();
	float rightTrigger = controller.GetRightTrigger();
	float rollDelta = (rightTrigger - leftTrigger) * m_rollSpeed * deltaSeconds;
	m_orientation.m_rollDegrees += rollDelta;

	m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -45.0f, 45.0f);*/

	m_camera.SetOrientation(m_orientation);
}

void PlayerController::MoveActorByKeyboard(float deltaSeconds, Actor* actor)
{
	Vec3 moveDirection;
	bool isRun = false;

	Vec3 fwd, left, up;

	m_orientation.GetAsVectors_IFwd_JLeft_KUp(fwd, left, up);

	fwd = Vec3(fwd.x, fwd.y, 0.f);
	left = Vec3(left.x, left.y, 0.f);

	if (g_theInput->IsKeyDown('W')) moveDirection += fwd;
	if (g_theInput->IsKeyDown('S')) moveDirection -= fwd;

	if (g_theInput->IsKeyDown('A')) moveDirection += left;
	if (g_theInput->IsKeyDown('D')) moveDirection -= left;
	if (g_theInput->IsKeyDown(KEYCODE_SHIFT)) isRun = true;


	if (moveDirection != Vec3())
	{
		moveDirection = moveDirection.GetNormalized();
		actor->MoveInDirection(deltaSeconds, moveDirection, isRun, actor->m_slowFactor);

	}
}

void PlayerController::RotateActorByMouse(float deltaSeconds, Actor* actor)
{
	UNUSED(deltaSeconds);

	Vec2 mouseDelta = g_theInput->GetCursorClientDelta();

	m_orientation.m_yawDegrees -= mouseDelta.x * m_sensitivity;
	m_orientation.m_pitchDegrees += mouseDelta.y * m_sensitivity;

	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.0f, 85.0f);

	float turnSpeed = actor->m_actorDef.m_turnSpeed;

	actor->TurnInDirection(m_orientation, turnSpeed, actor->m_slowFactor);
}
void PlayerController::MoveActorByController(float deltaSeconds, Actor* actor)
{
	XboxController controller = g_theInput->GetController(0);

	Vec3 moveDirection;
	bool isRun = false;

	Vec3 fwd, left, up;

	m_orientation.GetAsVectors_IFwd_JLeft_KUp(fwd, left, up);

	fwd = Vec3(fwd.x, fwd.y, 0.f);
	left = Vec3(left.x, left.y, 0.f);

	Vec2 leftStick = controller.GetLeftStick().GetPosition();
	moveDirection += fwd * leftStick.y;
	moveDirection -= left * leftStick.x;

	if (controller.IsButtonDown(XBOX_BUTTON_A)) isRun = true;

	if (moveDirection != Vec3::ZERO)
	{
		moveDirection = moveDirection.GetNormalized();
		actor->MoveInDirection(deltaSeconds, moveDirection, isRun, actor->m_slowFactor);
	}
}

void PlayerController::RotateActorByController(float deltaSeconds, Actor* actor)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	Vec2 rightStick = controller.GetRightStick().GetPosition();
	m_orientation.m_yawDegrees -= rightStick.x;
	m_orientation.m_pitchDegrees -= rightStick.y;

	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.0f, 85.0f);

	/*float leftTrigger = controller.GetLeftTrigger();
	float rightTrigger = controller.GetRightTrigger();
	float rollDelta = (rightTrigger - leftTrigger) * m_rollSpeed * deltaSeconds;
	m_orientation.m_rollDegrees += rollDelta;

	m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -45.0f, 45.0f);*/

	float turnSpeed = actor->m_actorDef.m_turnSpeed;

	actor->TurnInDirection(m_orientation, turnSpeed, actor->m_slowFactor);
}

void PlayerController::HandleDeadCamera()
{
	m_mode = POSSESSED;
	m_deadCamera = true;
	
}

Vec3 PlayerController::GetFwdNormal() const
{
	Vec3 fwd, left, up;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(fwd, left, up);

	return fwd;
}
