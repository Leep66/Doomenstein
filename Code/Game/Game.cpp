#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/SimpleTriangleFont.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Game/MapDefinition.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/Actor.hpp"
#include "Game/ActorDefinition.hpp"
#include "Game/WeaponDefinition.hpp"
#include "Game/Weapon.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Core/EngineCommon.hpp"

extern App* g_theApp;
extern Renderer* g_theRenderer;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern DevConsole* g_theDevConsole;
extern EventSystem* g_theEventSystem;
extern BitmapFont* g_theFont;
extern Window* g_theWindow;

constexpr double k_physicsTimeStep = 1.f / 240.f;

Game::Game(App* owner)
	: m_App(owner)
{
	Startup();

}

Game::~Game()
{
	Shutdown();
}

void Game::Startup()
{
	MapDefinition::InitializeMapDefinitions("Data/Definitions/MapDefinitions.xml");
	TileDefinition::InitializeTileDefinitions("Data/Definitions/TileDefinitions.xml");

	ActorDefinition::InitializeActorDefinitions("Data/Definitions/ProjectileActorDefinitions.xml");
	WeaponDefinition::InitializeWeaponDefinitions("Data/Definitions/WeaponDefinitions.xml");
	ActorDefinition::InitializeActorDefinitions("Data/Definitions/ActorDefinitions.xml");

	m_clock = new Clock();


	m_screenCameraA.SetMode(Camera::eMode_Orthographic);
	m_screenCameraB.SetMode(Camera::eMode_Orthographic);
	m_debugCamera.SetMode(Camera::eMode_Orthographic);

	m_screenCameraA.SetOrthographicView(Vec2(0.f, 0.f), Vec2(1600.f, 800.f));
	m_debugCamera.SetOrthographicView(Vec2(0.f, 0.f), Vec2(1600.f, 800.f));

	//CreateAndAddVertsForGrid(m_gridVerts, Vec3(0.f, 0.f, 0.f), Vec2(100.f, 100.f), 100, 100);
	
	CreateAttractText();
	m_hudBaseTex = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Hud_Base.png");

	m_menuMusic = g_theAudio->CreateOrGetSound("Data/Audio/Music/MainMenu_InTheDark.mp2");
	m_gameMusic = g_theAudio->CreateOrGetSound("Data/Audio/Music/E1M1_AtDoomsGate.mp2");

	m_clickSound = g_theAudio->CreateOrGetSound("Data/Audio/Click.mp3");


}

void Game::Update()
{
	float deltaSeconds = m_clock->GetDeltaSeconds();

	UpdateCameras(deltaSeconds);

	if (m_currentState != m_nextState)
	{
		ExitState(m_currentState);
		m_currentState = m_nextState;
		EnterState(m_currentState);
	}
	
	if (g_theInput->WasKeyJustPressed(KEYCODE_TILDE))
	{
		m_isConsoleOpen = !m_isConsoleOpen;
		g_theDevConsole->ToggleOpen();
	}

	switch (m_currentState)
	{
	case GameState::ATTRACT:
		UpdateAttract(deltaSeconds);
		break;
	case GameState::LOBBY:
		UpdateLobby(deltaSeconds);
		break;
	case GameState::PLAYING:
		UpdateGame(deltaSeconds);
		break;
	default:
		break;
	}
	
	UpdateUI();
	
	
}

void Game::UpdateAttract(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	SetCameraBack(&m_screenCameraA);

	if (g_theInput->WasKeyJustPressed(' '))
	{
		m_nextState = GameState::LOBBY;

		JoinPlayer(KEYBOARD);
		//JoinPlayer(CONTROLLER);
		g_theAudio->StartSound(m_clickSound);

	}

	if (g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_START))
	{
		m_nextState = GameState::LOBBY;
		JoinPlayer(CONTROLLER);
		g_theAudio->StartSound(m_clickSound);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_BACK))
	{
		g_theAudio->StartSound(m_clickSound);
		FireEvent("quit");

	}

}

void Game::UpdateLobby(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	if (g_theInput->WasKeyJustPressed(KEYCODE_F11))
	{
		JoinPlayer(CONTROLLER);
	}

	if (IsMultiplayer())
	{
		if (g_theInput->WasKeyJustPressed(' ') ||
			g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_START))
		{
			m_nextState = GameState::PLAYING;
			g_theAudio->StartSound(m_clickSound);
		}

		if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
		{
			RemovePlayer(KEYBOARD);
			g_theAudio->StartSound(m_clickSound);
		}

		if (g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_BACK))
		{
			RemovePlayer(CONTROLLER);
			g_theAudio->StartSound(m_clickSound);
		}

	}
	else
	{
		if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_BACK))
		{
			m_nextState = GameState::ATTRACT;
			g_theAudio->StartSound(m_clickSound);
		}

		if (g_theInput->WasKeyJustPressed(' '))
		{
			bool keyboardPlayerExists = false;

			for (PlayerController* player : m_players)
			{
				if (player && player->m_playerInfo.m_control == KEYBOARD)
				{
					keyboardPlayerExists = true;
					break;
				}
			}

			if (keyboardPlayerExists)
			{
				m_nextState = GameState::PLAYING;
				g_theAudio->StartSound(m_clickSound);
			}
			else
			{
				JoinPlayer(KEYBOARD);
				g_theAudio->StartSound(m_clickSound);
			}
		}

		if (g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_START))
		{
			bool controllerPlayerExists = false;

			for (PlayerController* player : m_players)
			{
				if (player && player->m_playerInfo.m_control == CONTROLLER)
				{
					controllerPlayerExists = true;
					break;
				}
			}

			if (controllerPlayerExists)
			{
				m_nextState = GameState::PLAYING;
				g_theAudio->StartSound(m_clickSound);
			}
			else
			{
				JoinPlayer(CONTROLLER);
				g_theAudio->StartSound(m_clickSound);
			}
		}

	}

	UpdateCameraAndViewport(&m_screenCameraA, &m_screenCameraB);
	UpdateLobbyText();
}

void Game::UpdateGame(float deltaSeconds)
{
	UpdateInput(deltaSeconds);
	UpdateListeners();

	if (m_currentMap)
	{
		m_currentMap->Update(deltaSeconds);
	}


}

void Game::UpdateListeners()
{
	PlayerController* player0 = GetPlayerByIndex(0);
	PlayerController* player1 = GetPlayerByIndex(1);

	if (player0)
	{
		g_theAudio->UpdateListener(0, player0->m_position, player0->GetFwdNormal(), player0->m_orientation.GetUpNormal());
	}

	if (player1)
	{
		g_theAudio->UpdateListener(1, player1->m_position, player1->GetFwdNormal(), player1->m_orientation.GetUpNormal());

	}
}

void Game::UpdateUI()
{
	float time = m_clock->GetTotalSeconds();
	float FPS = 1.f / m_clock->GetDeltaSeconds();
	float scale = m_clock->GetTimeScale();
	std::string timeText = Stringf("[Game Clock] Time: %.2f", time);

	std::string FPSText = Stringf("FPS: %.1f", FPS);

	std::string scaleText = Stringf("Scale: %.1f", scale);

	float yMin = 780.f;
	float yMax = 800.f;


	AABB2 timeBox(820.f, yMin, 1100.f, yMax);
	DebugAddScreenText(timeText, timeBox, 15.f, Vec2(0.f, 0.5f), -1.f);

	AABB2 FPSBox(1220.f, yMin, 1400.f, yMax);
	DebugAddScreenText(FPSText, FPSBox, 15.f, Vec2(0.f, 0.5f), -1.f);

	AABB2 scaleBox(1420.f, yMin, 1600.f, yMax);
	DebugAddScreenText(scaleText, scaleBox, 15.f, Vec2(0.f, 0.5f), -1.f);
}

void Game::DebugKillAllEnemies()
{
	for (Actor* a : m_currentMap->m_actors)
	{
		if (a && a->m_actorDef.m_faction == ActorFaction::DEMON)
		{
			a->Die();
		}
	}
}

void Game::UpdateInput(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_BACK))
	{
		m_nextState = GameState::ATTRACT;
		g_theAudio->StartSound(m_clickSound, false, 0.5f);
		DebugRenderClear();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		g_theApp->ResetGame();
	}

	if (g_theInput->WasKeyJustPressed('P'))
	{
		m_clock->TogglePause();
	}

	if (g_theInput->WasKeyJustPressed('O'))
	{
		m_clock->StepSingleFrame();
	}

	if (g_theInput->IsKeyDown('T'))
	{
		m_clock->SetTimeScale(0.1f);
	}
	else
	{
		m_clock->SetTimeScale(1.f);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
	{
		DebugKillAllEnemies();
	}
	
	
}

void Game::Render() const
{
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	switch (m_currentState)
	{
	case GameState::ATTRACT:
		RenderAttract();
		break;
	case GameState::LOBBY:
		RenderLobby();
		break;
	case GameState::PLAYING:

		RenderGame();
		DebugRender();
		
		RenderUI();
		break;
	default:
		break;
	}

	RenderDevConsole();
}

void Game::EnterState(GameState state)
{

	switch (state)
	{
	case GameState::ATTRACT:
		EnterAttract();
		break;

	case GameState::LOBBY:
		EnterLobby();
		break;

	case GameState::PLAYING:
		EnterPlaying();
		break;
	}
}

void Game::ExitState(GameState state)
{
	switch (state)
	{
	case GameState::ATTRACT:
		ExitAttract();
		break;
	case GameState::LOBBY:
		ExitLobby();
		break;
	case GameState::PLAYING:
		ExitPlaying();
		break;
	}
}


void Game::RenderGame() const
{
	g_theRenderer->ClearScreen(Rgba8::DARK_GRAY);

	int numPlayers = 0;
	if (GetPlayerByIndex(0)) ++numPlayers;
	if (GetPlayerByIndex(1)) ++numPlayers;

	PlayerController* player0 = GetPlayerByIndex(0);
	PlayerController* player1 = GetPlayerByIndex(1);


	if (!IsMultiplayer())
	{
		if (player0)
		{
			g_theRenderer->SetViewport(m_fullportData);
			g_theRenderer->BeginCamera(player0->m_camera);

			if (m_currentMap)
			{
				m_currentMap->Render(player0->m_camera);
			}

			g_theRenderer->EndCamera(player0->m_camera);
		}
		
		else if (player1)
		{
			g_theRenderer->SetViewport(m_fullportData);
			g_theRenderer->BeginCamera(player1->m_camera);

			if (m_currentMap)
			{
				m_currentMap->Render(player1->m_camera);
			}

			g_theRenderer->EndCamera(player1->m_camera);
		}
	}
	else
	{
		if (player0 && player1)
		{
			if (player0 && player0->GetActor())
			{
				g_theRenderer->SetViewport(m_portDataA);
				g_theRenderer->BeginCamera(player0->m_camera);		
				if (m_currentMap)
					m_currentMap->Render(player0->m_camera);

				if (player1 && player1->GetActor())
					player1->GetActor()->RenderBillboard(player0->m_camera);

				g_theRenderer->EndCamera(player0->m_camera);
			}

			if (player1 && player1->GetActor())
			{
				g_theRenderer->SetViewport(m_portDataB);
				g_theRenderer->BeginCamera(player1->m_camera);
				if (m_currentMap)
					m_currentMap->Render(player1->m_camera);

				if (player0 && player0->GetActor())
					player0->GetActor()->RenderBillboard(player1->m_camera);

				g_theRenderer->EndCamera(player1->m_camera);
			}
		}
	}
}


void Game::RenderUI() const
{
	
	
	PlayerController* playerA = GetPlayerByIndex(0);
	PlayerController* playerB = GetPlayerByIndex(1);

	

	g_theRenderer->SetViewport(m_fullportData);

	g_theRenderer->BeginCamera(m_debugCamera);

	std::vector<Vertex_PCU> waveVerts;

	AABB2 waveBound = AABB2(0.f, 780.f, 200.f, 800.f);
	/*int index = m_currentMap->m_waveIndex + 1;
	std::string waveText = Stringf("Current wave: %d", index);*/

	AABB2 remainingBound = waveBound;
	remainingBound.Translate(Vec2(0.f, 0.f));
	int num = m_currentMap->GetRemainingEnemies();
	std::string remainingText = Stringf("Remaining Enemies: %d", num);

	remainingBound.Translate(Vec2(2.f, 2.f));
	g_theFont->AddVertsForTextInBox2D(waveVerts, remainingText, remainingBound, 20.f, Rgba8::BLACK, 1.f, Vec2(0.f, 0.5f), OVERRUN);
	remainingBound.Translate(Vec2(-2.f, -2.f));
	g_theFont->AddVertsForTextInBox2D(waveVerts, remainingText, remainingBound, 20.f, Rgba8::RED, 1.f, Vec2(0.f, 0.5f), OVERRUN);

	if (m_currentMap->m_exposureTimer)
	{
		AABB2 timeBound = AABB2(600.f, 600.f, 1000.f, 700.f);
		//timeBound.Translate(Vec2(0.f, -20.f));
		float time = m_currentMap->m_exposureTimer->GetRemainingTime();
		std::string timeText = Stringf("EXPOSURE TIME\n%.0f", time);
		if (m_currentMap->m_exposureTimer->IsStopped())
		{
			timeText = Stringf("YOU'VE BEEN FOUND!\nKILL THEM ALL!!!");
		}

		timeBound.Translate(Vec2(2.f, 2.f));
		g_theFont->AddVertsForTextInBox2D(waveVerts, timeText, timeBound, 30.f, Rgba8::BLACK, 1.f, Vec2(0.5f, 0.5f), OVERRUN);
		timeBound.Translate(Vec2(-2.f, -2.f));
		g_theFont->AddVertsForTextInBox2D(waveVerts, timeText, timeBound, 30.f, Rgba8::RED, 1.f, Vec2(0.5f, 0.5f), OVERRUN);

	}

	

	//g_theFont->AddVertsForTextInBox2D(waveVerts, waveText, waveBound, 20.f, Rgba8::WHITE, 1.f, Vec2(0.f, 0.5f), OVERRUN);
	
	g_theRenderer->BindTexture(&g_theFont->GetTexture());
	g_theRenderer->DrawVertexArray(waveVerts);
	g_theRenderer->BindTexture(nullptr);

	g_theRenderer->EndCamera(m_debugCamera);


	if (!IsMultiplayer())
	{
		

		g_theRenderer->SetViewport(m_fullportData);

		if (playerA)
		{
			g_theRenderer->BeginCamera(m_screenCameraA);
			
			if (!playerA->GetActor()) return;


			if (playerA->GetActor()->IsDead())
			{
				AABB2 deathBound = AABB2(0.f, 0.f, 1600.f, 800.f);
				std::vector<Vertex_PCU> deathBoundVerts;
				AddVertsForAABB2D(deathBoundVerts, deathBound, Rgba8::ScaleColor(Rgba8::BLACK, 1.f, 0.8f));
				g_theRenderer->BindTexture(nullptr);
				g_theRenderer->DrawVertexArray(deathBoundVerts);
			}
			else
			{
				RenderPlayerHud(0);
			}
			g_theRenderer->EndCamera(m_screenCameraA);
		}

		else if (playerB && !playerB->GetActor()->IsDead())
		{
			g_theRenderer->BeginCamera(m_screenCameraB);

			if (!playerB->GetActor()) return;


			if (playerB->GetActor() && playerB->GetActor()->IsDead())
			{
				AABB2 deathBound = AABB2(0.f, 0.f, 1600.f, 800.f);
				std::vector<Vertex_PCU> deathBoundVerts;
				AddVertsForAABB2D(deathBoundVerts, deathBound, Rgba8::ScaleColor(Rgba8::BLACK, 1.f, 0.8f));
				g_theRenderer->BindTexture(nullptr);
				g_theRenderer->DrawVertexArray(deathBoundVerts);
			}
			else
			{
				RenderPlayerHud(1);
			}
			g_theRenderer->EndCamera(m_screenCameraB);
		}
	}
	else
	{
		g_theRenderer->SetViewport(m_portDataA);
		g_theRenderer->BeginCamera(m_screenCameraA);

		if (GetPlayerByIndex(0))
		{
			if (!GetPlayerByIndex(0)->GetActor()) return;
			if (GetPlayerByIndex(0)->GetActor()->IsDead())
			{
				AABB2 deathBound = AABB2(0.f, 0.f, 1600.f, 800.f);
				std::vector<Vertex_PCU> deathBoundVerts;
				AddVertsForAABB2D(deathBoundVerts, deathBound, Rgba8::ScaleColor(Rgba8::BLACK, 1.f, 0.8f));
				g_theRenderer->BindTexture(nullptr);
				g_theRenderer->DrawVertexArray(deathBoundVerts);
			}
			else
			{
				RenderPlayerHud(0);
			}
		}

		g_theRenderer->EndCamera(m_screenCameraA);

		g_theRenderer->SetViewport(m_portDataB);
		g_theRenderer->BeginCamera(m_screenCameraB);

		if (GetPlayerByIndex(1))
		{
			if (GetPlayerByIndex(1)->GetActor()->IsDead())
			{
				AABB2 deathBound = AABB2(0.f, 0.f, 1600.f, 800.f);
				std::vector<Vertex_PCU> deathBoundVerts;
				AddVertsForAABB2D(deathBoundVerts, deathBound, Rgba8::ScaleColor(Rgba8::BLACK, 1.f, 0.8f));
				g_theRenderer->BindTexture(nullptr);
				g_theRenderer->DrawVertexArray(deathBoundVerts);
			}
			else
			{
				RenderPlayerHud(1);
			}
		}


		g_theRenderer->EndCamera(m_screenCameraB);
	}
}

void Game::RenderPlayerHud(int index) const
{
	//PlayerController* player = GetPlayerByIndex(index);
	if (GetPlayerByIndex(index) && GetPlayerByIndex(index)->IsFreeCamera()) return;
	if (GetPlayerByIndex(index)->GetActor()->m_actorDef.m_faction == ActorFaction::DEMON) return;

	std::vector<Vertex_PCU> hudAVerts;
	float maxAY = IsMultiplayer() ? 400.f : 800.f;
	AABB2 hudABound = AABB2(0.f, 0.f, 1600.f, maxAY);

	hudABound.ChopOffTop(0.85f, 0.f);
	AddVertsForAABB2D(hudAVerts, hudABound, Rgba8::WHITE);

	float boxPadding = IsMultiplayer() ? 10.f : 20.f;

	std::vector<Vertex_PCU> killAVerts;
	AABB2 killABound = hudABound;
	killABound.ChopOffRight(0.87f, 0.f);
	killABound.ChopOffBottom(0.f, boxPadding);

	int killACount = GetPlayerByIndex(index)->m_playerKillCount;

	std::string killAText = Stringf("%d", killACount);

	float textHeight = IsMultiplayer() ? 25.f : 50.f;

	g_theFont->AddVertsForTextInBox2D(killAVerts, killAText, killABound, textHeight);


	std::vector<Vertex_PCU> healthAVerts;
	AABB2 healthABound = hudABound;
	healthABound.ChopOffRight(0.f, 900.f);
	healthABound.ChopOffLeft(0.f, 280.f);
	healthABound.ChopOffBottom(0.f, boxPadding);

	std::string healthAText = Stringf("%.0f", m_currentMap->GetActorByHandle(GetPlayerByIndex(index)->m_handle)->m_health);
	g_theFont->AddVertsForTextInBox2D(healthAVerts, healthAText, healthABound, textHeight);


	std::vector<Vertex_PCU> deathAVerts;
	AABB2 deathABound = hudABound;
	deathABound.ChopOffLeft(0.f, 1400.f);
	deathABound.ChopOffBottom(0.f, boxPadding);

	std::string deathAText = Stringf("%d", m_currentMap->m_totalDeath);
	g_theFont->AddVertsForTextInBox2D(deathAVerts, deathAText, deathABound, textHeight);

	std::vector<Vertex_PCU> armAVerts;
	AABB2 armABound;
	float centerTopy = IsMultiplayer() ? 47.f : 94.f;
	float dimensionY = IsMultiplayer() ? 12.f : 24.f;
	armABound.SetCenter(Vec2(625.f, centerTopy));
	armABound.SetDimensions(Vec2(31.f, dimensionY));
	AddVertsForAABB2D(armAVerts, armABound, Rgba8::WHITE);

	std::vector<Vertex_PCU> armBVerts;
	AABB2 armBBound;
	armBBound.SetCenter(Vec2(670.f, centerTopy));
	armBBound.SetDimensions(Vec2(31.f, dimensionY));
	AddVertsForAABB2D(armBVerts, armBBound, Rgba8::WHITE);

	std::vector<Vertex_PCU> armCVerts;
	AABB2 armCBound;
	armCBound.SetCenter(Vec2(712.f, centerTopy));
	armCBound.SetDimensions(Vec2(31.f, dimensionY));
	AddVertsForAABB2D(armCVerts, armCBound, Rgba8::WHITE);

	float centerBoty = IsMultiplayer() ? 28.f : 56.f;
	std::vector<Vertex_PCU> armDVerts;
	AABB2 armDBound;
	armDBound.SetCenter(Vec2(625.f, centerBoty));
	armDBound.SetDimensions(Vec2(31.f, dimensionY));
	AddVertsForAABB2D(armDVerts, armDBound, Rgba8::WHITE);

	std::vector<Vertex_PCU> armEVerts;
	AABB2 armEBound;
	armEBound.SetCenter(Vec2(670.f, centerBoty));
	armEBound.SetDimensions(Vec2(31.f, dimensionY));
	AddVertsForAABB2D(armEVerts, armEBound, Rgba8::WHITE);
	
	std::vector<Vertex_PCU> ammoVerts;

	if (!GetPlayerByIndex(index)->GetActor()->IsDead())
	{

		Weapon* w = GetPlayerByIndex(index)->GetActor()->m_currentWeapon;

		Actor* playerActor = GetPlayerByIndex(index)->GetActor();
		Actor* hitActor = nullptr;
		RaycastResult3D reticleResult;
		float range;
		if (w->m_weaponDef.m_rayCount > 0)
		{
			range = w->m_weaponDef.m_rayRange;
		}
		else
		{
			range = 999.f;
		}
		reticleResult = m_currentMap->RaycastAll(playerActor->GetEyePosition(),
			playerActor->m_orientation.GetForwardNormal(), range, &hitActor);
		Rgba8 reticleColor;
		if (reticleResult.m_didImpact && hitActor && !hitActor->IsDead())
		{
			reticleColor = Rgba8::RED;
		}
		
		else
		{
			reticleColor = Rgba8::LIGHT_GRAY;
		}

		if (w->m_isReload)
		{
			reticleColor = Rgba8::DARK_GRAY;
		}

		HUD weaponHud = GetPlayerByIndex(index)->GetActor()->m_currentWeapon->m_weaponDef.m_hud;
		//Texture* hudBaseTex = weaponHud.m_baseTex;
		Texture* reticTex = weaponHud.m_reticTex;
		Vec2 reticSize = weaponHud.m_reticleSize;
		Vec2 spriteSize = weaponHud.m_spriteSize;
		Vec2 spritePivot = weaponHud.m_spritePivot;

		std::vector<Vertex_PCU> reticVerts;
		AABB2 reticBound;
		float reticBoundY = IsMultiplayer() ? 200.f : 400.f;
		reticBound.SetCenter(Vec2(800.f, reticBoundY));
		reticBound.SetDimensions(reticSize);

		AddVertsForAABB2D(reticVerts, reticBound, reticleColor);


		g_theRenderer->BindShader(weaponHud.m_shader);
		g_theRenderer->BindTexture(reticTex);
		g_theRenderer->DrawVertexArray(reticVerts);

		
		AABB2 ammoBound = hudABound;
		ammoBound.ChopOffRight(0.f, 1200.f);
		ammoBound.ChopOffLeft(0.f, 195.f);
		ammoBound.ChopOffBottom(0.f, boxPadding);
		

		if (GetPlayerByIndex(index)->GetActor()->m_currentWeapon->m_currentCapacity > -1)
		{
			std::string ammoText = Stringf("%d", GetPlayerByIndex(index)->GetActor()->m_currentWeapon->m_currentCapacity);
			g_theFont->AddVertsForTextInBox2D(ammoVerts, ammoText, ammoBound, textHeight);
		}
		else
		{
			ammoBound.ChopOffLeft(0.f, 40.f);
			ammoBound.ChopOffRight(0.f, 40.f);
			AddVertsForAABB2D(ammoVerts, ammoBound, Rgba8::WHITE);
		}
		
		

		WeaponAnimation weaponAnim = weaponHud.GetWeaponAnimation(GetPlayerByIndex(index)->GetActor()->m_currentWeapon->m_currentAnim);

		SpriteSheet sheet = *weaponAnim.m_spriteSheet;
		int startFrame = weaponAnim.m_startFrame;
		int endFrame = weaponAnim.m_endFrame;
		float framePerSeconds = weaponAnim.m_secondsPerFrame;
		SpriteAnimPlaybackType type = SpriteAnimPlaybackType::ONCE;
		SpriteAnimDefinition* animDef = new SpriteAnimDefinition(sheet, startFrame, endFrame, framePerSeconds, type);

		float age = GetPlayerByIndex(index)->GetActor()->m_currentWeapon->m_animAge;
		const SpriteDefinition& spriteDef = animDef->GetSpriteDefAtTime(age);

		Vec2 uvAtMins, uvAtMaxs;
		spriteDef.GetUVs(uvAtMins, uvAtMaxs);

		std::vector<Vertex_PCU> weaponVerts;
		AABB2 weaponBound;
		float weaponBoundY = IsMultiplayer() ? 125.f : 250.f;
		weaponBound.SetCenter(Vec2(800.f, weaponBoundY));
		Vec2 size = IsMultiplayer() ? spriteSize / 2.f : spriteSize;
		weaponBound.SetDimensions(size);

		
		if (w->m_isReload)
		{
			float t = w->m_reloadTimer->GetElapsedFraction();

			float maxDrop = 200.f;
			float sineT = sinf(t * 3.1415926f);
			float offsetY = maxDrop * sineT;
			weaponBound.Translate(Vec2(0.f, -offsetY));

			std::vector<Vertex_PCU> reloadFractionVerts;

			Vec2 screenCenter = Vec2(800.f, reticBoundY);
			Vec2 barCenter = screenCenter + Vec2(0.f, -30.f);

			float barWidth = 50.f;
			float barHeight = 5.f;

			AABB2 barBack = AABB2(
				barCenter.x - barWidth * 0.5f,
				barCenter.y - barHeight * 0.5f,
				barCenter.x + barWidth * 0.5f,
				barCenter.y + barHeight * 0.5f
			);

			AABB2 barFill = AABB2(
				barBack.m_mins,
				Vec2(barBack.m_mins.x + (barWidth * t), barBack.m_maxs.y)
			);

			AddVertsForAABB2D(reloadFractionVerts, barBack, Rgba8::BLACK);
			AddVertsForAABB2D(reloadFractionVerts, barFill, Rgba8::LIGHT_GRAY);
			

			g_theRenderer->BindTexture(nullptr);
			g_theRenderer->DrawVertexArray(reloadFractionVerts);
		}

		else if (!w->m_isRefireCooldown)
		{
			float t = w->m_refireTimer->GetElapsedFraction();

			std::vector<Vertex_PCU> refireFractionVerts;

			Vec2 screenCenter = Vec2(800.f, reticBoundY);
			Vec2 barCenter = screenCenter + Vec2(0.f, -30.f);

			float barWidth = 50.f;
			float barHeight = 5.f;

			AABB2 barBack = AABB2(
				barCenter.x - barWidth * 0.5f,
				barCenter.y - barHeight * 0.5f,
				barCenter.x + barWidth * 0.5f,
				barCenter.y + barHeight * 0.5f
			);

			AABB2 barFill = AABB2(
				barBack.m_mins,
				Vec2(barBack.m_mins.x + (barWidth * t), barBack.m_maxs.y)
			);

			AddVertsForAABB2D(refireFractionVerts, barBack, Rgba8::BLACK);
			AddVertsForAABB2D(refireFractionVerts, barFill, Rgba8::LIGHT_GRAY);


			g_theRenderer->BindTexture(nullptr);
			g_theRenderer->DrawVertexArray(refireFractionVerts);
		}



		if (GetPlayerByIndex(index)->GetActor()->m_isSwitching)
		{
			float t = GetPlayerByIndex(index)->GetActor()->m_switchTimer->GetElapsedFraction();

			float maxDrop = 200.f;
			float sineT = sinf(t * 3.1415926f);
			float offsetY = maxDrop * sineT;
			weaponBound.Translate(Vec2(0.f, -offsetY));
		}

		
		AddVertsForAABB2D(weaponVerts, weaponBound, Rgba8::WHITE, uvAtMins, uvAtMaxs);

		g_theRenderer->BindTexture(&sheet.GetTexture());
		g_theRenderer->DrawVertexArray(weaponVerts);
		g_theRenderer->BindTexture(nullptr);

		if (GetPlayerByIndex(index)->GetActor()->m_slowFactor > 0.f && GetPlayerByIndex(index)->GetActor()->m_slowFactor < 1.f)
		{
			std::vector<Vertex_PCU> freezeVerts;

			AABB2 freezeBound(0.f, 0.f, 1600.f, 800.f);
			Texture* freezeTex = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FreezeTex.png");

			AddVertsForAABB2D(freezeVerts, freezeBound, Rgba8(255, 255, 255, 100));
			g_theRenderer->BindTexture(freezeTex);
			g_theRenderer->DrawVertexArray(freezeVerts);
			g_theRenderer->BindTexture(nullptr);

		}
	}

	std::vector<Vertex_PCU> bloodyVerts;
	
	float bloodT = GetPlayerByIndex(index)->GetActor()->m_health / GetPlayerByIndex(index)->GetActor()->m_actorDef.m_health;
	float bloodY = Interpolate(800.f, 0.f, bloodT);
	AABB2 bloodyBound = AABB2(0.f, 800.f, 1600.f, 1600.f);
	bloodyBound.Translate(Vec2(0.f, -bloodY));
	AddVertsForAABB2D(bloodyVerts, bloodyBound, Rgba8(255, 255, 255, 200));

	
	g_theRenderer->BindTexture(m_hudBaseTex);
	g_theRenderer->DrawVertexArray(hudAVerts);
	g_theRenderer->BindTexture(&g_theFont->GetTexture());
	g_theRenderer->DrawVertexArray(killAVerts);
	g_theRenderer->DrawVertexArray(healthAVerts);
	g_theRenderer->DrawVertexArray(deathAVerts);

	g_theRenderer->BindTexture(g_theRenderer->CreateOrGetTextureFromFile("Data/Images/BloodyHud.png"));
	g_theRenderer->DrawVertexArray(bloodyVerts);

	g_theRenderer->BindTexture(&g_theFont->GetTexture());
	if (GetPlayerByIndex(index)->GetActor()->m_currentWeapon->m_currentCapacity < 0)
	{
		Texture* ammoTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Infinity.png");
		g_theRenderer->BindTexture(ammoTexture);
	}
	
	g_theRenderer->DrawVertexArray(ammoVerts);

	g_theRenderer->BindTexture(GetPlayerByIndex(index)->GetActor()->m_weapons[0]->m_weaponDef.m_hud.m_iconTex);
	g_theRenderer->DrawVertexArray(armAVerts);

	g_theRenderer->BindTexture(GetPlayerByIndex(index)->GetActor()->m_weapons[1]->m_weaponDef.m_hud.m_iconTex);
	g_theRenderer->DrawVertexArray(armBVerts);

	g_theRenderer->BindTexture(GetPlayerByIndex(index)->GetActor()->m_weapons[2]->m_weaponDef.m_hud.m_iconTex);
	g_theRenderer->DrawVertexArray(armCVerts);

	g_theRenderer->BindTexture(GetPlayerByIndex(index)->GetActor()->m_weapons[3]->m_weaponDef.m_hud.m_iconTex);
	g_theRenderer->DrawVertexArray(armDVerts);

	g_theRenderer->BindTexture(GetPlayerByIndex(index)->GetActor()->m_weapons[4]->m_weaponDef.m_hud.m_iconTex);
	g_theRenderer->DrawVertexArray(armEVerts);

	g_theRenderer->BindTexture(nullptr);
}

void Game::RenderDevConsole() const
{
	ViewportData debugPortData;
	debugPortData.TopLeftX = 0.f;
	debugPortData.TopLeftY = 0.f;
	debugPortData.Width = (float)g_theWindow->GetClientDimensions().x;
	debugPortData.Height = (float)g_theWindow->GetClientDimensions().y;

	g_theRenderer->SetViewport(debugPortData);
	g_theRenderer->BeginCamera(m_debugCamera);
	if (g_theDevConsole)
	{
		g_theDevConsole->Render(AABB2(0, 0, 1600, 800));
		
	}
	g_theRenderer->EndCamera(m_debugCamera);
}

void Game::RenderAttractText() const
{
	g_theRenderer->BindTexture(&g_theFont->GetTexture());
	g_theRenderer->DrawVertexArray(m_attractText);
	g_theRenderer->BindTexture(nullptr);
}

void Game::RenderLobbyTextA() const
{
	if ((int)m_lobbyTexts.size() > 0)
	{
		g_theRenderer->BindTexture(&g_theFont->GetTexture());
		g_theRenderer->DrawVertexArray(m_lobbyTexts[0]);

		g_theRenderer->BindTexture(nullptr);
	}
}

void Game::RenderLobbyTextB() const
{
	if ((int)m_lobbyTexts.size() > 1)
	{
		g_theRenderer->BindTexture(&g_theFont->GetTexture());
		g_theRenderer->DrawVertexArray(m_lobbyTexts[1]);

		g_theRenderer->BindTexture(nullptr);
	}
}

void Game::CreateAndAddVertsForGrid(std::vector<Vertex_PCU>& verts, const Vec3& center, const Vec2& size, int numRows, int numCols)
{
	float gridWidth = size.x;
	float gridHeight = size.y;

	float cellWidth = gridWidth / (float)numCols;
	float cellHeight = gridHeight / (float)numRows;

	Vec3 startPos = center - Vec3(gridWidth * 0.5f, gridHeight * 0.5f, 0.0f);

	float thinLineThickness = 0.02f;
	float thickLineThickness = 0.05f;
	float originLineThickness = 0.1f;

	Rgba8 normalColor = Rgba8(127, 127, 127, 50);
	Rgba8 thickXColor = Rgba8(255, 50, 50, 50);
	Rgba8 thickYColor = Rgba8(50, 255, 50, 50);
	Rgba8 originLineXColor = Rgba8(255, 0, 0, 255);
	Rgba8 originLineYColor = Rgba8(0, 255, 0, 255);

	for (int row = 0; row <= numRows; ++row)
	{
		float y = startPos.y + row * cellHeight;
		bool isOriginLine = (y == center.y);

		for (int col = 0; col < numCols; ++col)
		{
			float x1 = startPos.x + col * cellWidth;
			float x2 = x1 + cellWidth;

			Rgba8 lineColor = isOriginLine ? originLineXColor : (row % 5 == 0) ? thickXColor : normalColor;
			float lineThickness = isOriginLine ? originLineThickness : (row % 5 == 0) ? thickLineThickness : thinLineThickness;

			AABB3 lineBounds;
			lineBounds.m_mins = Vec3(x1, y - lineThickness * 0.5f, -lineThickness * 0.5f);
			lineBounds.m_maxs = Vec3(x2, y + lineThickness * 0.5f, lineThickness * 0.5f);

			AddVertsForAABB3D(verts, lineBounds, lineColor);
		}
	}

	for (int col = 0; col <= numCols; ++col)
	{
		float x = startPos.x + col * cellWidth;
		bool isOriginLine = (x == center.x);

		for (int row = 0; row < numRows; ++row)
		{
			float y1 = startPos.y + row * cellHeight;
			float y2 = y1 + cellHeight;

			Rgba8 lineColor = isOriginLine ? originLineYColor : (col % 5 == 0) ? thickYColor : normalColor;
			float lineThickness = isOriginLine ? originLineThickness : (col % 5 == 0) ? thickLineThickness : thinLineThickness;

			AABB3 lineBounds;
			lineBounds.m_mins = Vec3(x - lineThickness * 0.5f, y1, -lineThickness * 0.5f);
			lineBounds.m_maxs = Vec3(x + lineThickness * 0.5f, y2, lineThickness * 0.5f);

			AddVertsForAABB3D(verts, lineBounds, lineColor);
		}
	}
}

void Game::TogglePhysicsPause()
{
	if (m_currentMap->m_physicsTimer->IsStopped())
	{
		m_currentMap->m_physicsTimer->Start();
	}
	else
	{
		m_currentMap->m_physicsTimer->Stop();
	}
}

void Game::EnterAttract()
{
	SetCameraBack(&m_screenCameraA);

	m_menuPlayback = g_theAudio->StartSound(m_menuMusic, true, 0.5f);
	RemovePlayer(KEYBOARD);
	RemovePlayer(CONTROLLER);
}

void Game::ExitAttract()
{

}

void Game::EnterLobby()
{
	
}

void Game::ExitLobby()
{
	g_theAudio->StopSound(m_menuPlayback);
}

void Game::EnterPlaying()
{
	m_maps.push_back(new Map(this, &MapDefinition::GetDefinition("GMap"), k_physicsTimeStep));
	//m_maps.push_back(new Map(this, &MapDefinition::GetDefinition("TestMap"), k_physicsTimeStep));
	//m_maps.push_back(new Map(this, &MapDefinition::GetDefinition("MPMap"), k_physicsTimeStep));
	
	//m_maps.push_back(new Map(this, &MapDefinition::GetDefinition("PortalMap"), k_physicsTimeStep));

	std::string mapName = g_gameConfigBlackboard.GetValue("defaultMap", "Invalid Map");
	m_currentMap = GetMapByName(mapName);
	m_gamePlayback = g_theAudio->StartSound(m_gameMusic, true, 0.1f);
}

void Game::ExitPlaying()
{
	SetCameraBack(&m_screenCameraA);
	for (Map* m : m_maps)
	{
		delete m;
		m = nullptr;
	}
	m_currentMap = nullptr;
	m_maps.clear();

	g_theAudio->StopSound(m_gamePlayback);
}

void Game::UpdateCameraAndViewport(Camera* cameraA, Camera* cameraB)
{

	float maxX = (float)g_theWindow->GetClientDimensions().x;
	float maxY = (float)g_theWindow->GetClientDimensions().y;


	Vec2 viewAMin = Vec2::ZERO;

	float y = IsMultiplayer() ? 400.f : 800.f;

	Vec2 viewAMax = Vec2(1600.f, y);

	cameraA->SetOrthographicView(viewAMin, viewAMax);
	

	Vec2 cameraAMin = Vec2::ZERO;
	Vec2 cameraAMax = IsMultiplayer() ? Vec2(maxX, maxY / 2.f) : Vec2(maxX, maxY);
	AABB2 cameraABox = AABB2(cameraAMin, cameraAMax);

	m_portDataA.TopLeftX = cameraABox.m_mins.x;
	m_portDataA.TopLeftY = cameraABox.m_mins.y;
	m_portDataA.Width = cameraABox.m_maxs.x - cameraABox.m_mins.x;
	m_portDataA.Height = cameraABox.m_maxs.y - cameraABox.m_mins.y;



	Vec2 viewBMin = Vec2::ZERO;
	Vec2 viewBMax = Vec2(1600.f, 400.f);
	cameraB->SetOrthographicView(viewBMin, viewBMax);

	Vec2 cameraBMin = Vec2(0.f, maxY / 2.f);
	Vec2 cameraBMax = Vec2(maxX, maxY);

	AABB2 cameraBBox = AABB2(cameraBMin, cameraBMax);

	m_portDataB.TopLeftX = cameraBBox.m_mins.x;
	m_portDataB.TopLeftY = cameraBBox.m_mins.y;
	m_portDataB.Width = cameraBBox.m_maxs.x - cameraBBox.m_mins.x;
	m_portDataB.Height = cameraBBox.m_maxs.y - cameraBBox.m_mins.y;

	Vec2 fullMin = Vec2::ZERO;
	Vec2 fullMax = Vec2(maxX, maxY);
	AABB2 fullBox = AABB2(fullMin, fullMax);

	m_fullportData.TopLeftX = fullBox.m_mins.x;
	m_fullportData.TopLeftY = fullBox.m_mins.y;
	m_fullportData.Width = fullBox.m_maxs.x - fullBox.m_mins.x;
	m_fullportData.Height = fullBox.m_maxs.y - fullBox.m_mins.y;
}

void Game::SetCameraBack(Camera* camera)
{
	float maxX = (float)g_theWindow->GetClientDimensions().x;
	float maxY = (float)g_theWindow->GetClientDimensions().y;


	Vec2 viewMin = Vec2::ZERO;
	Vec2 viewMax = Vec2(1600.f, 800.f);

	camera->SetOrthographicView(viewMin, viewMax);

	Vec2 cameraMin = Vec2::ZERO;
	Vec2 cameraMax = Vec2(maxX, maxY);
	AABB2 cameraBox = AABB2(cameraMin, cameraMax);

	m_portDataA.TopLeftX = cameraBox.m_mins.x;
	m_portDataA.TopLeftY = cameraBox.m_mins.y;
	m_portDataA.Width = cameraBox.m_maxs.x - cameraBox.m_mins.x;
	m_portDataA.Height = cameraBox.m_maxs.y - cameraBox.m_mins.y;

	g_theRenderer->SetViewport(m_portDataA);

}

void Game::JoinPlayer(ControlType control)
{
	int playerIndex = 0;
	for (; playerIndex < (int)m_players.size(); ++playerIndex)
	{
		if (m_players[playerIndex] == nullptr)
			break;
	}

	if (playerIndex >= (int)m_players.size())
	{
		m_players.resize(playerIndex + 1, nullptr);
	}

	PlayerInfo info;
	info.m_control = control;
	info.m_isJoined = true;
	info.m_playerIndex = playerIndex;

	PlayerController* player = new PlayerController();
	player->m_playerInfo = info;
	m_players[playerIndex] = player;

	g_theAudio->SetNumListeners((int)m_players.size());
}


void Game::RemovePlayer(ControlType type)
{
	for (int i = 0; i < (int)m_players.size(); ++i)
	{
		if (m_players[i] != nullptr && m_players[i]->m_playerInfo.m_control == type)
		{
			delete m_players[i];
			m_players[i] = nullptr;

			break;
		}
	}
}

PlayerController* Game::GetPlayerByIndex(int playerIndex) const
{
	for (int i = 0; i < (int)m_players.size(); ++i)
	{
		if (m_players[i] && m_players[i]->m_playerInfo.m_playerIndex == playerIndex)
		{
			return m_players[i];
		}
	}
	return nullptr;
}

Map* Game::GetMapByName(std::string mapName)
{
	for (Map* m : m_maps)
	{
		if (m->m_definition->m_name == mapName)
		{
			return m;
		}
	}

	return nullptr;
}



void Game::AddVertsForLobbyText(std::vector<Vertex_PCU>& verts, PlayerInfo info)
{
	float maxX = 1600.f;
	float maxY = 800.f;


	Vec2 textMin = Vec2(0.f, 0.f);

	float y = IsMultiplayer() ? maxY / 2.f : maxY;

	Vec2 textMax = Vec2(maxX, y);

	std::string playerText = Stringf("Player %d", (info.m_playerIndex + 1));

	AABB2 playerBound = AABB2(textMin.x, textMin.y, textMax.x, textMax.y);

	playerBound.ChopOffBottom(0.6f, 0.f);

	playerBound.Translate(Vec2(2.f, 2.f));
	g_theFont->AddVertsForTextInBox2D(verts, playerText, playerBound, 50.f, Rgba8::BLACK, 0.95f, Vec2(0.5f, 0.f));
	playerBound.Translate(Vec2(-2.f, -2.f));
	g_theFont->AddVertsForTextInBox2D(verts, playerText, playerBound, 50.f, Rgba8::LIGHT_GRAY, 0.95f, Vec2(0.5f, 0.f));

	std::string controlText;
	switch (m_players[info.m_playerIndex]->m_playerInfo.m_control)
	{
	case NONE:
		controlText = "Invalid";
		break;
	case KEYBOARD:
		controlText = "Mouse and Keyboard";
		break;
	case CONTROLLER:
		controlText = "Controller";
		break;
	default:
		break;
	}

	AABB2 controlBound = AABB2(textMin.x, textMin.y, textMax.x, textMax.y);
	controlBound.ChopOffBottom(0.5f, 0.f);
	controlBound.Translate(Vec2(2.f, 2.f));
	g_theFont->AddVertsForTextInBox2D(verts, controlText, controlBound, 30.f, Rgba8::BLACK, 0.95f, Vec2(0.5f, 0.f));
	controlBound.Translate(Vec2(-2.f, -2.f));
	g_theFont->AddVertsForTextInBox2D(verts, controlText, controlBound, 30.f, Rgba8::LIGHT_GRAY, 0.95f, Vec2(0.5f, 0.f));

	std::string operationText = info.m_control == CONTROLLER ? "Press START to start game\nPress BACK to leave game" : "Press SPACE to start game\nPress ESCAPE to leave game";
	if (!IsMultiplayer())
	{
		operationText += info.m_control == CONTROLLER ? "\nPress SPACE to join player" : "\nPress START to join player";
	}
	AABB2 operationBound = AABB2(textMin.x, textMin.y, textMax.x, textMax.y);
	operationBound.ChopOffBottom(0.3f, 0.f);
	operationBound.Translate(Vec2(2.f, 2.f));
	g_theFont->AddVertsForTextInBox2D(verts, operationText, operationBound, 15.f, Rgba8::BLACK, 0.95f, Vec2(0.5f, 0.f), SHRINK_TO_FIT, 999999999, 5.f);
	operationBound.Translate(Vec2(-2.f, -2.f));

	g_theFont->AddVertsForTextInBox2D(verts, operationText, operationBound, 15.f, Rgba8::LIGHT_GRAY, 0.95f, Vec2(0.5f, 0.f), SHRINK_TO_FIT, 999999999, 5.f);
	
}


void Game::CreateAttractText()
{
	float maxX = 1600.f;
	float maxY = 800.f;

	std::string attractText = "Press SPACE to join with mouse and keyboard\nPress START to join with controller\nPress ESCAPE or BACK to exit";
	AABB2 textBounds = AABB2(0.f, 0.f, maxX, maxY);
	textBounds.ChopOffTop(0.6f, 0.f);
	textBounds.Translate(Vec2(4.f, 4.f));
	g_theFont->AddVertsForTextInBox2D(m_attractText, attractText, textBounds, 30.f, Rgba8::BLACK, 0.95f, Vec2(0.5f, 0.5f), SHRINK_TO_FIT, 999999999, 10.f);
	textBounds.Translate(Vec2(-4.f, -4.f));
	g_theFont->AddVertsForTextInBox2D(m_attractText, attractText, textBounds, 30.f, Rgba8::LIGHT_GRAY, 0.95f, Vec2(0.5f, 0.5f), SHRINK_TO_FIT, 999999999, 10.f);
	

}

void Game::UpdateLobbyText()
{
	m_lobbyTexts.clear();
	for (PlayerController* player : m_players)
	{
		
		if (player)
		{
			std::vector<Vertex_PCU> verts;
			AddVertsForLobbyText(verts, player->m_playerInfo);
			m_lobbyTexts.push_back(verts);
		}
		
	}
	
}

void Game::RenderAttract() const
{
	g_theRenderer->ClearScreen(Rgba8::DARK_GRAY);
	g_theRenderer->BeginCamera(m_screenCameraA);
	

	std::vector<Vertex_PCU> backgroundVerts;

	AddVertsForAABB2D(backgroundVerts, AABB2(0.f, 0.f, 1600.f, 800.f), Rgba8::WHITE);
	Texture* backgroundTex = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Background.png");
	g_theRenderer->BindTexture(backgroundTex);
	g_theRenderer->DrawVertexArray(backgroundVerts);

	std::vector<Vertex_PCU> iconVerts;

	AddVertsForAABB2D(iconVerts, AABB2(0.f, 500.f, 500.f, 800.f), Rgba8::WHITE);
	Texture* iconTex = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/DoomIcon.png");
	g_theRenderer->BindTexture(iconTex);
	g_theRenderer->DrawVertexArray(iconVerts);

	RenderAttractText();

	g_theRenderer->EndCamera(m_screenCameraA);
}

void Game::RenderLobby() const
{
	g_theRenderer->ClearScreen(Rgba8::DARK_GRAY);

	g_theRenderer->SetViewport(m_portDataA);
	g_theRenderer->BeginCamera(m_screenCameraA);

	RenderLobbyTextA();

	g_theRenderer->EndCamera(m_screenCameraA);

	if (IsMultiplayer())
	{
		g_theRenderer->SetViewport(m_portDataB);
		
		g_theRenderer->BeginCamera(m_screenCameraB);
		
		RenderLobbyTextB();

		g_theRenderer->EndCamera(m_screenCameraB);
	}

}

void Game::Shutdown()
{

	for (Map* m : m_maps)
	{
		delete m;
		m = nullptr;
	}
	m_currentMap = nullptr;
	m_maps.clear();

	m_gridVerts.clear();
	
	delete m_clock;
	m_clock = nullptr;
	m_hudBaseTex = nullptr;
	
}

bool Game::Event_KeysAndFuncs(EventArgs& args)
{
	UNUSED(args);

	g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, "Controls");

	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Mouse                - Aim");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "LB/RT                - Fire");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "1/X                  - Pistol");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "2/Y                  - Shotgun");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "3                    - Plasma");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "4                    - Freeze");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "5                    - Hook");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "LeftArrow/Dpad Up    - PrevWeapon");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "RightArrow/Dpad Down - NextWeapon");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "F1                   - Kill All Enemies");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "F                    - Possess Next Actor");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "W / A                - Move");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "S / D                - Strafe");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Q / E                - Roll");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Z / C                - Elevate");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Shift                - Sprint");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "H                    - Set Camera to Origin");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "P                    - Pause Game");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "O                    - Step One Frame");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "T                    - Slow Motion Mode");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "~                    - Open / Close DevConsole");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Escape               - Exit Game");

	return true;
}

bool Game::IsMultiplayer() const
{
	int validPlayerCount = 0;
	for (PlayerController* player : m_players)
	{
		if (player != nullptr && player->m_playerInfo.m_isJoined)
		{
			validPlayerCount++;
		}
	}
	return validPlayerCount >= 2;
}


void Game::DebugRender() const
{

	ViewportData debugPortData;
	debugPortData.TopLeftX = 0.f;
	debugPortData.TopLeftY = 0.f;
	debugPortData.Width = (float)g_theWindow->GetClientDimensions().x;
	debugPortData.Height = (float)g_theWindow->GetClientDimensions().y;

	g_theRenderer->SetViewport(debugPortData);
	if (m_currentMap)
	{
		m_currentMap->DebugRender();

	}
	DebugRenderScreen(m_debugCamera);
	
}

void Game::UpdateCameras(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	
}

