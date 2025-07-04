#pragma once
#include "Engine/Math/FloatRange.hpp"
#include "Game/Sound.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/Shader.hpp"
#include <string>
#include <vector>

struct WeaponAnimation
{
	std::string m_name = "Idle";

	Shader* m_shader = nullptr;
	SpriteSheet* m_spriteSheet = nullptr;

	IntVec2 m_cellCount = IntVec2(1, 1);
	float m_secondsPerFrame = 1.f;
	int m_startFrame = 0;
	int m_endFrame = 0;
	Sound m_sound;

};

struct HUD
{
	Shader* m_shader = nullptr;
	Texture* m_baseTex = nullptr;
	Texture* m_reticTex = nullptr;
	Texture* m_iconTex = nullptr;

	Vec2 m_reticleSize = Vec2(1.f, 1.f);
	Vec2 m_spriteSize = Vec2(1.f, 1.f);
	Vec2 m_spritePivot = Vec2(0.5f, 0.f);


	std::vector<WeaponAnimation> m_weaponAnims;
	WeaponAnimation const& GetWeaponAnimation(std::string animName);
};

struct WeaponDefinition
{
	std::string m_name;
	float m_refireTime = 0.f;
	int m_rayCount = 0;
	float m_rayCone = 0.f;
	float m_rayRange = 0.f;
	FloatRange m_rayDamage = FloatRange(0.f, 0.f);
	float m_rayImpulse = 0.f;
	int m_projctileCount = 0;
	float m_projectileCone = 0.f;
	float m_projectSpeed = 0.f;
	std::string m_projectileActor;
	int m_meleeCount = 0;
	float m_meleeRange = 0.f;
	float m_meleeArc = 0.f;
	FloatRange m_meleeDamage = FloatRange(0.f, 0.f);
	float m_meleeImpulse = 0.f;
	float m_blastRange = 0.f;
	FloatRange m_blastDamage = FloatRange(0.f, 0.f);

	FloatRange m_recoilYaw = FloatRange(0.f, 0.f);
	FloatRange m_recoilPitch = FloatRange(0.f, 0.f);
	float m_recoilDamping = 0.f;

	float m_reloadDuration = 0.f;
	int m_maxCapacity = 0;
	float m_fireHeight = 0.f;

	HUD m_hud;
	Sound m_fireSound;

	static std::vector<WeaponDefinition> s_weaponDefinitions;

	static void InitializeWeaponDefinitions(const char* path);

	static WeaponDefinition const& GetDefinition(std::string weaponName);

	
};