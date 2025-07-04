#include "Game/ActorDefinition.hpp"
#include "Engine/Core/EngineCommon.hpp"

extern Renderer* g_theRenderer;

std::vector<ActorDefinition> ActorDefinition::s_actorDefinitions;

void ActorDefinition::InitializeActorDefinitions(const char* path)
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

	XmlElement const* actorDefElem = root->FirstChildElement("ActorDefinition");
	while (actorDefElem)
	{
		ActorDefinition actorDef;

		std::string name = ParseXmlAttribute(*actorDefElem, "name", "Invalid Name");
		std::string factionName = ParseXmlAttribute(*actorDefElem, "faction", "Invalid Faction");
		ActorFaction faction = ActorFaction::INVALID;
		if (factionName == "Neutral") faction = ActorFaction::NEUTRAL;
		else if (factionName == "Marine") faction = ActorFaction::MARINE;
		else if (factionName == "Demon") faction = ActorFaction::DEMON;
		float health = ParseXmlAttribute(*actorDefElem, "health", -1.f);
		bool canBePossessed = ParseXmlAttribute(*actorDefElem, "canBePossessed", false);
		float corpseLifetime = ParseXmlAttribute(*actorDefElem, "corpseLifetime", -1.f);
		bool visible = ParseXmlAttribute(*actorDefElem, "visible", false);
		bool dieOnSpawn = ParseXmlAttribute(*actorDefElem, "dieOnSpawn", false);
		bool isCrowdControl = ParseXmlAttribute(*actorDefElem, "isCrowdControl", false);
		float controlPeriod = ParseXmlAttribute(*actorDefElem, "controlPeriod", 0.f);
		float slowFactor = ParseXmlAttribute(*actorDefElem, "slowFactor", 1.f);
		float dragForce = ParseXmlAttribute(*actorDefElem, "dragForce", 0.f);
		bool isBoss = ParseXmlAttribute(*actorDefElem, "isBoss", false);
		std::string spawnActor = ParseXmlAttribute(*actorDefElem, "spawnActor", "");
		float spawnPeriod = ParseXmlAttribute(*actorDefElem, "spawnPeriod", 0.f);
		int spawnNum = ParseXmlAttribute(*actorDefElem, "spawnNum", 0);


		actorDef.m_name = name;
		actorDef.m_isVisible = visible;
		actorDef.m_health = health;
		actorDef.m_corpseLifetime = corpseLifetime;
		actorDef.m_faction = faction;
		actorDef.m_canBePossessed = canBePossessed;
		actorDef.m_dieOnSpawn = dieOnSpawn;
		actorDef.m_isCrowdControl = isCrowdControl;
		actorDef.m_controlPeriod = controlPeriod;
		actorDef.m_slowFactor = slowFactor;
		actorDef.m_dragForce = dragForce;
		actorDef.m_isBoss = isBoss;
		actorDef.m_spawnActor = spawnActor;
		actorDef.m_spawnPeriod = spawnPeriod;
		actorDef.m_spawnNum = spawnNum;

		XmlElement const* collisionElem = actorDefElem->FirstChildElement("Collision");
		if (collisionElem)
		{
			float radius = ParseXmlAttribute(*collisionElem, "radius", -1.f);
			float height = ParseXmlAttribute(*collisionElem, "height", -1.f);
			bool collidesWithWorld = ParseXmlAttribute(*collisionElem, "collidesWithWorld", false);
			bool collidesWithActors = ParseXmlAttribute(*collisionElem, "collidesWithActors", false);
			FloatRange damageOnCollide = ParseXmlAttribute(*collisionElem, "damageOnCollide", FloatRange(-1.f, -1.f));
			float impulseOnCollide = ParseXmlAttribute(*collisionElem, "impulseOnCollide", -1.f);
			bool dieOnCollide = ParseXmlAttribute(*collisionElem, "dieOnCollide", false);

			actorDef.m_physicsRadius = radius;
			actorDef.m_physicsHeight = height;
			actorDef.m_collidesWithWorld = collidesWithWorld;
			actorDef.m_collidesWithActors = collidesWithActors;
			actorDef.m_dieOnCollide = dieOnCollide;
			actorDef.m_impulseOnCollide = impulseOnCollide;
			actorDef.m_damageOnCollide = damageOnCollide;


		}


		XmlElement const* physicsElem = actorDefElem->FirstChildElement("Physics");
		if (physicsElem)
		{
			bool simulated = ParseXmlAttribute(*physicsElem, "simulated", false);
			float walkSpeed = ParseXmlAttribute(*physicsElem, "walkSpeed", -1.f);
			float runSpeed = ParseXmlAttribute(*physicsElem, "runSpeed", -1.f);
			float turnSpeed = ParseXmlAttribute(*physicsElem, "turnSpeed", -1.f);
			float drag = ParseXmlAttribute(*physicsElem, "drag", -1.f);
			bool flying = ParseXmlAttribute(*physicsElem, "flying", false);
			float attackRange = ParseXmlAttribute(*physicsElem, "attackRange", 0.f);
			float fireHeightOffset = ParseXmlAttribute(*physicsElem, "fireHeightOffset", 0.f);
			float oppositeForce = ParseXmlAttribute(*physicsElem, "oppositeForce", 0.f);
			actorDef.m_simulated = simulated;
			actorDef.m_walkSpeed = walkSpeed;
			actorDef.m_runSpeed = runSpeed;
			actorDef.m_drag = drag;
			actorDef.m_turnSpeed = turnSpeed;
			actorDef.m_flying = flying;
			actorDef.m_attackRange = attackRange;
			actorDef.m_fireHeightOffset = fireHeightOffset;
			actorDef.m_oppositeForce = oppositeForce;
		}

		XmlElement const* cameraElem = actorDefElem->FirstChildElement("Camera");
		if (cameraElem)
		{
			float eyeHeight = ParseXmlAttribute(*cameraElem, "eyeHeight", -1.f);
			float cameraFOV = ParseXmlAttribute(*cameraElem, "cameraFOV", -1.f);

			actorDef.m_eyeHeight = eyeHeight;
			actorDef.m_cameraFOVDegrees = cameraFOV;
		}

		XmlElement const* aiElem = actorDefElem->FirstChildElement("AI");
		if (aiElem)
		{
			bool aiEnabled = ParseXmlAttribute(*aiElem, "aiEnabled", false);
			float sightRadius = ParseXmlAttribute(*aiElem, "sightRadius", -1.f);
			float sightAngle = ParseXmlAttribute(*aiElem, "sightAngle", -1.f);
			actorDef.m_aiEnabled = aiEnabled;
			actorDef.m_sightRadius = sightRadius;
			actorDef.m_sightAngle = sightAngle;

		}

		XmlElement const* visualElem = actorDefElem->FirstChildElement("Visuals");
		if (visualElem)
		{
			Vec2 size = ParseXmlAttribute(*visualElem, "size", Vec2::ZERO);
			Vec2 pivot = ParseXmlAttribute(*visualElem, "pivot", Vec2::ZERO);
			std::string billboardName = ParseXmlAttribute(*visualElem, "billboardType", "None");
			BillboardType billboardType = BillboardType::NONE;

			if (billboardName == "WorldUpOpposing")
			{
				billboardType = BillboardType::WORLD_UP_OPPOSING;
			}
			else if (billboardName == "WorldUpFacing")
			{
				billboardType = BillboardType::WORLD_UP_FACING;
			}
			else if (billboardName == "FullFacing")
			{
				billboardType = BillboardType::FULL_FACING;
			}
			else if (billboardName == "FullOpposing")
			{
				billboardType = BillboardType::FULL_OPPOSING;
			}

			bool renderLit = ParseXmlAttribute(*visualElem, "renderLit", false);
			bool renderRounded = ParseXmlAttribute(*visualElem, "renderRounded", false);
			bool renderHealthBar = ParseXmlAttribute(*visualElem, "renderHealthBar", false);
			std::string shaderName = ParseXmlAttribute(*visualElem, "shader", "Default");
			std::string spriteSheetName = ParseXmlAttribute(*visualElem, "spriteSheet", "Default");
			IntVec2 cellCount = ParseXmlAttribute(*visualElem, "cellCount", IntVec2::ZERO);
			Texture* spriteTex = g_theRenderer->CreateOrGetTextureFromFile(spriteSheetName.c_str());

			actorDef.m_visualSize = size;
			actorDef.m_pivot = pivot;
			actorDef.m_billboardType = billboardType;
			actorDef.m_renderLit = renderLit;
			actorDef.m_renderRounded = renderRounded;
			actorDef.m_renderHealthBar = renderHealthBar;
			actorDef.m_shader = g_theRenderer->GetShader(shaderName.c_str());
			actorDef.m_spriteSheet = new SpriteSheet(*spriteTex, cellCount);
			actorDef.m_cellCount = cellCount;

			XmlElement const* agElem = visualElem->FirstChildElement("AnimationGroup");
			while (agElem)
			{
				SpriteAnimationGroupDefinition* sagd = new SpriteAnimationGroupDefinition();
				sagd->LoadFromXmlElement(*agElem, *actorDef.m_spriteSheet);
				
				

				actorDef.m_animationGroups.push_back(sagd);
				agElem = agElem->NextSiblingElement("AnimationGroup");
			}

			

		}

 		XmlElement const* soundsElem = actorDefElem->FirstChildElement("Sounds");
		if (soundsElem)
		{
			XmlElement const* soundElem = soundsElem->FirstChildElement("Sound");

			while (soundElem)
			{
				std::string sound = ParseXmlAttribute(*soundElem, "sound", "");
				std::string soundName = ParseXmlAttribute(*soundElem, "name", "");

				Sound s;

				s.m_sound = sound;
				s.m_name = soundName;

				actorDef.m_sounds.push_back(s);

				soundElem = soundElem->NextSiblingElement("Sound");
			}
			
		}



		XmlElement const* inventroyElem = actorDefElem->FirstChildElement("Inventory");
		if (inventroyElem)
		{
			XmlElement const* weaponElem = inventroyElem->FirstChildElement("Weapon");

			while (weaponElem)
			{
				std::string weaponName = ParseXmlAttribute(*weaponElem, "name", "Invalid Weapon");
				actorDef.m_weaponsNames.push_back(weaponName);
				weaponElem = weaponElem->NextSiblingElement("Weapon");
			}

		}

		s_actorDefinitions.push_back(actorDef);
		actorDefElem = actorDefElem->NextSiblingElement("ActorDefinition");


	}
}

ActorDefinition const& ActorDefinition::GetDefinition(std::string actorName)
{
	for (ActorDefinition const& def : s_actorDefinitions)
	{
		if (def.m_name == actorName)
		{
			return def;
		}
	}

	ERROR_AND_DIE("Invalid Actor Definition!");

	/*static ActorDefinition invalidDef;
	return invalidDef;*/
}

SpriteAnimationGroupDefinition* ActorDefinition::GetAnimGroupDef(std::string agName) const
{
	for (SpriteAnimationGroupDefinition* agDef : m_animationGroups)
	{
		if (agDef->m_name == agName)
		{
			return agDef;
		}
	}

	return nullptr;
}

Sound ActorDefinition::GetSoundDef(std::string soundName)
{
	for (Sound s : m_sounds)
	{
		if (s.m_sound == soundName)
		{
			return s;
		}
	}

	return Sound();
}
