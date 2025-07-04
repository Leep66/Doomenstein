#pragma once
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Timer.hpp"
#include "Game/Map.hpp"
#include "Game/PlayerController.hpp"

enum class GameState
{
	NONE,
	ATTRACT,
	LOBBY,
	PLAYING,

	COUNT
};

class Game 
{
public:

	Game(App* owner);	
	~Game();
	void Startup();

	void Update();
	void Render() const;

	void EnterState(GameState state);
	void ExitState(GameState state);

	void Shutdown();
	
	static bool Event_KeysAndFuncs(EventArgs& args);
	bool IsMultiplayer() const;
	void JoinPlayer(ControlType type);
	void RemovePlayer(ControlType type);
	PlayerController* GetPlayerByIndex(int playerIndex) const;
	Map* GetMapByName(std::string mapName);

public:
	App* m_App = nullptr;
	RandomNumberGenerator* m_rng = nullptr;
	bool m_isDebugActive = false;
	Rgba8 m_startColor = Rgba8(0, 255, 0, 255);

	Camera m_debugCamera;
	Camera m_screenCameraA;
	Camera m_screenCameraB;
	ViewportData m_portDataA;
	ViewportData m_portDataB;
	ViewportData m_fullportData;

	bool m_isConsoleOpen = false;
	Clock* m_clock = nullptr;

	GameState m_currentState = GameState::NONE;
	GameState m_nextState = GameState::ATTRACT;

	Map* m_currentMap = nullptr;
	std::vector<Map*> m_maps;

	std::vector<Vertex_PCU> m_gridVerts;

	std::vector<PlayerController*> m_players;

	

private:

	void UpdateInput(float deltaSeconds);
	void DebugRender() const;
	void UpdateCameras(float deltaSeconds);
	void UpdateAttract(float deltaSeconds);
	void UpdateLobby(float deltaSeconds);
	void UpdateGame(float deltaSeconds);
	void UpdateListeners();
	void UpdateUI();

	void DebugKillAllEnemies();
	
	void CreateAttractText();
	void UpdateLobbyText();
	void RenderAttract() const;
	void RenderLobby() const;
	void RenderGame() const;
	void RenderUI() const;
	void RenderPlayerHud(int index) const;
	void RenderDevConsole() const;
	void RenderAttractText() const;
	void RenderLobbyTextA() const;
	void RenderLobbyTextB() const;

	void CreateAndAddVertsForGrid(std::vector<Vertex_PCU>& verts, const Vec3& center, const Vec2& size, int numRows, int numCols);
	void TogglePhysicsPause();

	void EnterAttract();
	void ExitAttract();
	void EnterLobby();
	void ExitLobby();
	void EnterPlaying();
	void ExitPlaying();
	void UpdateCameraAndViewport(Camera* cameraA, Camera* cameraB);
	void SetCameraBack(Camera* camera);
	
	void AddVertsForLobbyText(std::vector<Vertex_PCU>& verts, PlayerInfo info);
	
private:
	std::vector<Vertex_PCU> m_attractText;
	std::vector<std::vector<Vertex_PCU>> m_lobbyTexts;
	Texture* m_hudBaseTex = nullptr;

	SoundID m_gameMusic;
	SoundPlaybackID m_gamePlayback;

	SoundID m_menuMusic;
	SoundPlaybackID m_menuPlayback;

	SoundID m_clickSound;
};