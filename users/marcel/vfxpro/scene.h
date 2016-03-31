#pragma once

#include "types.h"
#include <string>
#include <vector>

namespace tinyxml2
{
	class XMLElement;
}

struct DrawableList;
struct Effect;

struct SceneEffect;
struct SceneAction;
struct SceneEvent;
struct SceneLayer;
struct Scene;

class Surface;

//

extern Scene * g_currentScene;
extern SceneLayer * g_currentSceneLayer;
extern Surface * g_currentSurface;

//

template <typename T, T * R>
struct ScopedBlock
{
	T m_oldValue;

	ScopedBlock(T value)
	{
		m_oldValue = *R;

		*R = value;
	}

	~ScopedBlock()
	{
		*R = m_oldValue;
	}
};

typedef ScopedBlock<Scene*, &g_currentScene> ScopedSceneBlock;
typedef ScopedBlock<SceneLayer*, &g_currentSceneLayer> ScopedSceneLayerBlock;
typedef ScopedBlock<Surface*, &g_currentSurface> ScopedSurfaceBlock;

//

struct SceneEffect
{
	std::string m_name;
	float m_strength;
	Effect * m_effect;

	SceneEffect();
	~SceneEffect();

	bool load(const tinyxml2::XMLElement * xmlEffect);
};

//

struct SceneLayer : TweenFloatCollection
{
	enum BlendMode
	{
		kBlendMode_Add,
		kBlendMode_Subtract,
		kBlendMode_Alpha,
		kBlendMode_Opaque
	};

	Scene * m_scene;

	std::string m_name;
	BlendMode m_blendMode;
	bool m_autoClear;
	bool m_copyPreviousLayer;

	TweenFloat m_opacity;

	std::vector<SceneEffect*> m_effects;

	Surface * m_surface;

	bool m_debugEnabled;

	SceneLayer(Scene * scene);
	~SceneLayer();

	void load(const tinyxml2::XMLElement * xmlLayer);

	void tick(const float dt);
	void draw(DrawableList & drawableList);
	void draw();
};

//

struct SceneAction
{
	enum ActionType
	{
		kActionType_None,
		kActionType_Tween,
		kActionType_Signal
	};

	ActionType m_type;

	struct Tween
	{
		enum TargetType
		{
			kTargetType_Layer,
			kTargetType_Effect
		};

		TargetType m_targetType;
		std::string m_targetName;
		std::string m_varName;
		float m_tweenTo;
		float m_tweenTime;
		EaseType m_easeType;
		float m_easeParam;
		bool m_replaceTween;
		bool m_addTween;
		float m_preDelay;
		float m_postDelay;
	} m_tween;

	struct Signal
	{
		std::string m_targetName;
		std::string m_message;
	} m_signal;

	SceneAction();

	bool load(const tinyxml2::XMLElement * xmlAction);
};

//

struct SceneEvent
{
	std::string m_name;
	std::vector<SceneAction*> m_actions;

	SceneEvent();
	~SceneEvent();

	void execute(Scene & scene);

	void load(const tinyxml2::XMLElement * xmlEvent);
};

//

struct SceneMidiMap
{
	enum MapType
	{
		kMapType_Event,
		kMapType_EffectVar
	};

	int id;
	MapType type;
	bool liveEnabled; // if true, the MIDI map is active during live performances. otherwise, the mapping is ignored

	std::string event;
	std::string effect;
	std::string var;
};

//

struct Scene
{
	std::string m_filename;
	std::string m_name;
	std::vector<SceneLayer*> m_layers;
	std::vector<SceneEvent*> m_events;
	std::vector<SceneMidiMap> m_midiMaps;

	float m_time;

	Scene();
	~Scene();

	void tick(const float dt);
	void draw(DrawableList & drawableList);

	SceneEffect * findEffectByName(const char * name);

	void triggerEvent(const char * name);

	bool load(const char * filename);

	void clear();
	bool reload();
};
