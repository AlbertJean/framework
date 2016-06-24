#pragma once

#include "config.h"
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
	Scene * m_scene;

	std::string m_name;
	BlendMode m_blendMode;
	bool m_autoClear;
	Color m_clearColor;
	bool m_copyPreviousLayer;
	TweenFloat m_copyPreviousLayerAlpha;

	TweenFloat m_visible;
	TweenFloat m_opacity;

	std::vector<SceneEffect*> m_effects;

	Surface * m_surface;

	bool m_debugEnabled;

	SceneLayer(Scene * scene);
	~SceneLayer();

	bool load(const tinyxml2::XMLElement * xmlLayer);

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

	enum TweenTargetType
	{
		kTweenTargetType_Layer,
		kTweenTargetType_Effect
	};

	struct Tween
	{
		TweenTargetType m_targetType;
		std::string m_targetName;

		bool m_replaceTweens;
		bool m_addTweens;

		struct TweenSet
		{
			float m_tweenTime;

			EaseType m_easeType;
			float m_easeParam;

			float m_preDelay;
			float m_postDelay;

			struct TweenVar
			{
				std::string m_varName;
				float m_tweenTo;
				float m_preDelay;
			};

			std::vector<TweenVar> m_vars;
		};

		std::vector<TweenSet> m_tweens;
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
	int m_oscId;
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
	float min;
	float max;
};

//

struct Scene : public TweenFloatCollection, public TweenFloatModifier
{
	struct Modifier
	{
		enum Op
		{
			kOp_Mul,
			kOp_Add,
			kOp_Sub,
			kOp_Min,
			kOp_Max
		};

		static Op parseOp(const std::string & op)
		{
			if (op == "mul")
				return kOp_Mul;
			if (op == "add")
				return kOp_Add;
			if (op == "sub")
				return kOp_Sub;
			if (op == "min")
				return kOp_Min;
			if (op == "max")
				return kOp_Max;
		}

		TweenFloat * var;
		TweenFloat * mod;
		TweenFloat str;
		
		Op op;

		bool hasRange;
		float range[4]; // input/output ranges

		Modifier()
			: var(nullptr)
			, mod(nullptr)
			, str(1.f)
			, op(kOp_Mul)
			, hasRange(false)
		{
			range[0] = range[1] = range[2] = range[3] = 0.f;
		}

		float applyRange(const float value) const
		{
			if (hasRange)
			{
				const float t1 = (value - range[0]) / (range[1] - range[0]);
				const float t2 = t1 < 0.f ? 0.f : t1 > 1.f ? 1.f : t1;
				const float r = range[2] + (range[3] - range[2]) * t2;
				return r;
			}
			else
			{
				return value;
			}
		}

		float applyOp(const float value1, const float value2) const
		{
			switch (op)
			{
			case kOp_Mul:
				return value1 * value2;
			case kOp_Add:
				return value1 + value2;
			case kOp_Sub:
				return value1 - value2;
			case kOp_Min:
				return std::min(value1, value2);
			case kOp_Max:
				return std::max(value1, value2);
			default:
				Assert(false);
				return value1;
			}
		}

		float apply(const float value) const
		{
			const float v1 = applyOp(value, applyRange(*mod));
			const float v2 = value;
			
			const float t1 = str;
			const float t2 = 1.f - t1;

			const float result = v1 * t1 + v2 * t2;

			return result;
		}
	};

	std::string m_filename;
	std::string m_name;
	std::vector<SceneLayer*> m_layers;
	std::vector<SceneEvent*> m_events;
	std::vector<Modifier> m_modifiers;
	std::vector<SceneMidiMap> m_midiMaps;

	float m_time;

	TweenFloat m_varPcmVolume;
	TweenFloat m_varTime;

#if ENABLE_DEBUG_TEXT
	struct DebugText
	{
		std::string text;
		float endTime;
	};

	std::vector<DebugText> m_debugTexts;
#endif

	Scene();
	~Scene();

	void tick(const float dt);
	void draw(DrawableList & drawableList);
	void debugDraw();

	SceneLayer * findLayerByName(const char * name);
	SceneEffect * findEffectByName(const char * name);

	void triggerEvent(const char * name);
	void triggerEventByOscId(int oscId);
	void addDebugText(const char * text);

	bool load(const char * filename);

	void clear();
	bool reload();

	// TweenFloatModifier

	virtual float applyModifier(TweenFloat * tweenFloat, float value) override;
};
