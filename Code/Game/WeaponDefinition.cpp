#include "Game/WeaponDefinition.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"

extern Renderer* g_theRenderer;

std::vector<WeaponDefinition> WeaponDefinition::s_weaponDefinitions;

void WeaponDefinition::InitializeWeaponDefinitions(const char* path)
{
	XmlDocument doc;
	XmlResult result = doc.LoadFile(path);
	if (result != tinyxml2::XML_SUCCESS)
	{
		ERROR_AND_DIE("Failed to load " + std::string(path));
	}

	XmlElement* root = doc.RootElement();
	if (root == nullptr)
	{
		ERROR_AND_DIE(std::string(path) + " is missing a root element");
	}

	XmlElement const* weaponDefElem = root->FirstChildElement("WeaponDefinition");
	while (weaponDefElem)
	{
		WeaponDefinition weaponDef;

		std::string name = ParseXmlAttribute(*weaponDefElem, "name", "Invalid Name");
		float refireTime = ParseXmlAttribute(*weaponDefElem, "refireTime", -1.f);
		int rayCount = ParseXmlAttribute(*weaponDefElem, "rayCount", -1);
		float rayCone = ParseXmlAttribute(*weaponDefElem, "rayCone", -1.f);
		float rayRange = ParseXmlAttribute(*weaponDefElem, "rayRange", -1.f);
		FloatRange rayDamage = ParseXmlAttribute(*weaponDefElem, "rayDamage", FloatRange(-1.f, -1.f));
		float rayImpulse = ParseXmlAttribute(*weaponDefElem, "rayImpulse", -1.f);
		int projectileCount = ParseXmlAttribute(*weaponDefElem, "projectileCount", -1);
		std::string projectileActor = ParseXmlAttribute(*weaponDefElem, "projectileActor", "Invalid Actor");
		float projectileCone = ParseXmlAttribute(*weaponDefElem, "projectileCone", -1.f);
		float projectileSpeed = ParseXmlAttribute(*weaponDefElem, "projectileSpeed", -1.f);
		int meleeCount = ParseXmlAttribute(*weaponDefElem, "meleeCount", -1);
		float meleeArc = ParseXmlAttribute(*weaponDefElem, "meleeArc", -1.f);
		float meleeRange = ParseXmlAttribute(*weaponDefElem, "meleeRange", -1.f);
		FloatRange meleeDamage = ParseXmlAttribute(*weaponDefElem, "meleeDamage", FloatRange(-1.f, -1.f));
		float meleeImpulse = ParseXmlAttribute(*weaponDefElem, "meleeImpulse", -1.f);
		FloatRange recoilYaw = ParseXmlAttribute(*weaponDefElem, "recoilYaw", FloatRange(0.f, 0.f));
		FloatRange recoilPitch = ParseXmlAttribute(*weaponDefElem, "recoilPitch", FloatRange(0.f, 0.f));
		float recoilDamping = ParseXmlAttribute(*weaponDefElem, "recoilDamping", 0.f);
		float reloadDuration = ParseXmlAttribute(*weaponDefElem, "reloadDuration", 0.f);
		int capacity = ParseXmlAttribute(*weaponDefElem, "capacity", 0);
		float fireHeight = ParseXmlAttribute(*weaponDefElem, "fireHeight", 0.f);
		float blastRange = ParseXmlAttribute(*weaponDefElem, "blastRange", 0.f);
		FloatRange blastDamage = ParseXmlAttribute(*weaponDefElem, "blastDamage", FloatRange(0.f, 0.f));

		weaponDef.m_name = name;
		weaponDef.m_refireTime = refireTime;
		weaponDef.m_rayCount = rayCount;
		weaponDef.m_rayCone = rayCone;
		weaponDef.m_rayRange = rayRange;
		weaponDef.m_rayDamage = rayDamage;
		weaponDef.m_rayImpulse = rayImpulse;
		weaponDef.m_projctileCount = projectileCount;
		weaponDef.m_projectileCone = projectileCone;
		weaponDef.m_projectSpeed = projectileSpeed;
		weaponDef.m_projectileActor = projectileActor;
		weaponDef.m_meleeCount = meleeCount;
		weaponDef.m_meleeRange = meleeRange;
		weaponDef.m_meleeArc = meleeArc;
		weaponDef.m_meleeDamage = meleeDamage;
		weaponDef.m_meleeImpulse = meleeImpulse;
		weaponDef.m_blastRange = blastRange;
		weaponDef.m_blastDamage = blastDamage;
		weaponDef.m_recoilYaw = recoilYaw;
		weaponDef.m_recoilPitch = recoilPitch;
		weaponDef.m_reloadDuration = reloadDuration;
		weaponDef.m_maxCapacity = capacity;
		weaponDef.m_recoilDamping = recoilDamping;
		weaponDef.m_fireHeight = fireHeight;


		XmlElement const* HUDElem = weaponDefElem->FirstChildElement("HUD");
		if (HUDElem)
		{
			HUD hud;

			std::string shaderPath = ParseXmlAttribute(*HUDElem, "shader", "Default");
			std::string baseTexName = ParseXmlAttribute(*HUDElem, "baseTexture", "Default");
			std::string reticleTexName = ParseXmlAttribute(*HUDElem, "reticleTexture", "Default");
			std::string iconTexName = ParseXmlAttribute(*HUDElem, "iconTexture", "Default");
			Vec2 reticleSize = ParseXmlAttribute(*HUDElem, "reticleSize", Vec2(1.f, 1.f));
			Vec2 spriteSize = ParseXmlAttribute(*HUDElem, "spriteSize", Vec2(1.f, 1.f));
			Vec2 spritePivot = ParseXmlAttribute(*HUDElem, "spritePivot", Vec2(0.5f, 0.f));

			hud.m_shader = g_theRenderer->GetShader(shaderPath.c_str());
			hud.m_baseTex = g_theRenderer->CreateOrGetTextureFromFile(baseTexName.c_str());
			hud.m_reticTex = g_theRenderer->CreateOrGetTextureFromFile(reticleTexName.c_str());
			hud.m_iconTex = g_theRenderer->CreateOrGetTextureFromFile(iconTexName.c_str());
			hud.m_reticleSize = reticleSize;
			hud.m_spriteSize = spriteSize;
			hud.m_spritePivot = spritePivot;

			XmlElement const* weaponAnimElem = HUDElem->FirstChildElement("Animation");
			while (weaponAnimElem)
			{
				WeaponAnimation weaponAnim;

				std::string animName = ParseXmlAttribute(*weaponAnimElem, "name", "Idle");
				std::string animShader = ParseXmlAttribute(*weaponAnimElem, "shader", "Default");
				std::string animSpriteSheet = ParseXmlAttribute(*weaponAnimElem, "spriteSheet", "Default");
				IntVec2 animCellCount = ParseXmlAttribute(*weaponAnimElem, "cellCount", IntVec2(1, 1));
				float animSecondsPerFrame = ParseXmlAttribute(*weaponAnimElem, "secondsPerFrame", 1.f);
				int animStartFrame = ParseXmlAttribute(*weaponAnimElem, "startFrame", 0);
				int animEndFrame = ParseXmlAttribute(*weaponAnimElem, "endFrame", 0);

				Texture* animTex = g_theRenderer->CreateOrGetTextureFromFile(animSpriteSheet.c_str());

				weaponAnim.m_name = animName;
				weaponAnim.m_shader = g_theRenderer->GetShader(animShader.c_str());;
				weaponAnim.m_spriteSheet = new SpriteSheet(*animTex, animCellCount);
				weaponAnim.m_cellCount = animCellCount;
				weaponAnim.m_secondsPerFrame = animSecondsPerFrame;
				weaponAnim.m_startFrame = animStartFrame;
				weaponAnim.m_endFrame = animEndFrame;

				hud.m_weaponAnims.push_back(weaponAnim);
				weaponAnimElem = weaponAnimElem->NextSiblingElement("Animation");
			}

			weaponDef.m_hud = hud;
		}

		XmlElement const* soundsElem = weaponDefElem->FirstChildElement("Sounds");
		if (weaponDefElem)
		{
			XmlElement const* soundElem = soundsElem->FirstChildElement("Sound");
			while (soundElem)
			{
				std::string sound = ParseXmlAttribute(*soundElem, "sound", "");
				std::string soundPath = ParseXmlAttribute(*soundElem, "name", "");

				Sound s;
				s.m_name = soundPath;
				s.m_sound = sound;

				weaponDef.m_fireSound = s;

				soundElem = soundsElem->NextSiblingElement("Sound");
			}
		}

		s_weaponDefinitions.push_back(weaponDef);
 		weaponDefElem = weaponDefElem->NextSiblingElement("WeaponDefinition");
	}
}

WeaponDefinition const& WeaponDefinition::GetDefinition(std::string weaponName)
{
	for (WeaponDefinition const& def : s_weaponDefinitions)
	{
		if (def.m_name == weaponName)
		{
			return def;
		}
	}

	ERROR_AND_DIE("Invalid Weapon Definition!");
	/*static WeaponDefinition invalidDef;
	return invalidDef;*/
}

WeaponAnimation const& HUD::GetWeaponAnimation(std::string animName)
{
	for (WeaponAnimation const& w : m_weaponAnims)
	{
		if (w.m_name == animName)
		{
			return w;
		}
	}

	static WeaponAnimation weaponAnim;
	weaponAnim.m_name = "INVALID";

	return weaponAnim;
}
