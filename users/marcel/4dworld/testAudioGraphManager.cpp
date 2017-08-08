#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "Calc.h"
#include "framework.h"
#include "Path.h"
#include "soundmix.h" // AudioVoiceManager. todo : move to its own source file
#include "StringEx.h"
#include "Vec3.h"
#include "../libparticle/ui.h"

extern const int GFX_SX;
extern const int GFX_SY;

extern bool STEREO_OUTPUT;

//

#define BALL_CAGE_SIZE 16.f
#define FIELD_SIZE 20.f
#define FIELD_SIZE_FOR_FLYING (FIELD_SIZE * .8f)

#define MACHINE_V2 1

//

static bool inputIsCaptured = false;

//

static void doSlider(float & value, const char * name, const float smoothness, const float dt)
{
	UiElem & elem = g_menu->getElem(name);
	
	bool & isDragging = elem.getBool(0, false);
	float & desiredValue = elem.getFloat(1, value);
	
	const int sx = g_uiState->sx;
	const int sy = 16;
	
	const int x1 = g_drawX;
	const int y1 = g_drawY;
	const int x2 = x1 + sx;
	const int y2 = y1 + sy - 1;
	
	if (g_doActions)
	{
		elem.tick(x1, y1, x2, y2);
		
		if (elem.isActive)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				isDragging = true;
			}
			
			if (mouse.wentUp(BUTTON_LEFT))
			{
				isDragging = false;
			}
			
			if (isDragging)
			{
				desiredValue = saturate((mouse.x - x1) / float(x2 - x1));
			}
		}
		else
		{
			isDragging = false;
		}
		
		//
		
		const float retain = std::powf(smoothness, dt);
		
		value = lerp(desiredValue, value, retain);
	}
	
	if (g_doDraw)
	{
		setColor(0, 0, 255/2);
		drawRect(x1, y1, x1 + (x2 - x1) * desiredValue, y2);
		
		pushBlend(BLEND_ADD);
		setColor(0, 100, 255/2);
		drawRect(x1, y1, x1 + (x2 - x1) * value, y2);
		popBlend();
		
		setColor(colorWhite);
		drawTextArea(x1, y1, x2 - x1, y2 - y1, 12, 0, 0, "%s", name);
		
		setColor(0, 0, 255);
		drawRectLine(x1, y1, x2, y2);
	}
	
	g_drawY += sy;
}

//

struct WorldInterface
{
	virtual ~WorldInterface()
	{
	}
	
	virtual Vec2 xformWorldToScreen(const Vec3 & p) = 0;
	virtual Vec3 xformScreenToWorld(const float x, const float y, const float worldY) = 0;
	
	virtual void rippleSound(const Vec3 & p, const float amount = 1.f) = 0;
	virtual void rippleFlight(const Vec3 & p) = 0;
	
	virtual float measureSound(const Vec3 & p) = 0;
	virtual float measureFlight(const Vec3 & p) = 0;
};

static WorldInterface * g_world = nullptr;

enum EntityType
{
	kEntity_Unknown,
	kEntity_TestInstance,
	kEntity_Ball,
	kEntity_Bird,
	kEntity_Machine,
	kEntity_Voice
};

struct EntityBase
{
	EntityType type;
	
	bool dead;
	
	AudioGraphInstance * graphInstance;
	
	EntityBase()
		: type(kEntity_Unknown)
		, dead(false)
		, graphInstance(nullptr)
	{
	}
	
	virtual ~EntityBase()
	{
	}
	
	virtual void tick(const float dt) = 0;
	
	virtual void kill()
	{
		dead = true;
	}
};

struct TestInstance : EntityBase
{
	TestInstance(const char * filename)
		: EntityBase()
	{
		type = kEntity_TestInstance;
		
		graphInstance = g_audioGraphMgr->createInstance(filename);
	}
	
	virtual ~TestInstance() override
	{
		g_audioGraphMgr->free(graphInstance);
	}
	
	virtual void tick(const float dt) override
	{
	}
};

struct Ball : EntityBase
{
	Vec3 pos;
	Vec3 vel;
	
	Ball()
		: EntityBase()
		, pos(0.f, 10.f, 0.f)
		, vel(0.f, 0.f, 0.f)
	{
		type = kEntity_Ball;
		
		graphInstance = g_audioGraphMgr->createInstance("ballTest.xml");
	}
	
	virtual ~Ball() override
	{
		g_audioGraphMgr->free(graphInstance);
	}
	
	virtual void tick(const float dt) override
	{
		// physics
		
		vel[1] += -10.f * dt;
		
		pos += vel * dt;
		
		if (pos[1] < 0.f)
		{
			pos[1] *= -1.f;
			vel[1] *= -1.f;
			
			graphInstance->audioGraph->triggerEvent("bounce");
		}
		
		for (int i = 0; i < 3; ++i)
		{
			if (i == 1)
				continue;
			
			if (pos[i] < -BALL_CAGE_SIZE)
			{
				pos[i] = -BALL_CAGE_SIZE;
				vel[i] *= -1.f;
			}
			else if (pos[i] > +BALL_CAGE_SIZE)
			{
				pos[i] = +BALL_CAGE_SIZE;
				vel[i] *= -1.f;
			}
		}
		
		
		graphInstance->audioGraph->setMemf("pos", pos[0], pos[1], pos[2]);
		graphInstance->audioGraph->setMemf("vel", vel[0], vel[1], vel[2]);
		
		// alive state
		
		if (graphInstance->audioGraph->isFLagSet("dead"))
		{
			dead = true;
		}
	}
	
	virtual void kill() override
	{
		graphInstance->audioGraph->setFlag("kill");
	}
};

struct Oneshot : EntityBase
{
	Vec3 pos;
	Vec3 vel;
	float velGrow;
	Vec3 dim;
	float dimGrow;
	
	float timer;
	bool doRampDown;
	
	std::function<void()> onKill;
	
	Oneshot(const char * filename, const float time, const bool _doRampDown)
		: EntityBase()
		, pos()
		, vel()
		, velGrow(1.f)
		, dim(1.f, 1.f, 1.f)
		, dimGrow(1.f)
		, timer(time)
		, doRampDown(_doRampDown)
	{
		graphInstance = g_audioGraphMgr->createInstance(filename);
	}
	
	virtual ~Oneshot() override
	{
		g_audioGraphMgr->free(graphInstance);
	}
	
	virtual void tick(const float dt) override
	{
		pos += vel * dt;
		vel *= std::powf(velGrow, dt);
		dim *= std::powf(dimGrow, dt);
		
		graphInstance->audioGraph->setMemf("pos", pos[0], pos[1], pos[2]);
		graphInstance->audioGraph->setMemf("vel", vel[0], vel[1], vel[2]);
		graphInstance->audioGraph->setMemf("dim", dim[0], dim[1], dim[2]);
		
		//
		
		if (timer != -1.f)
		{
			timer -= dt;
			
			if (timer <= 0.f)
			{
				if (doRampDown)
				{
					graphInstance->audioGraph->setFlag("voice.4d.rampDown");
				}
				else
				{
					kill();
				}
			}
		}
		
		if (graphInstance->audioGraph->isFLagSet("voice.4d.rampedDown"))
		{
			kill();
		}
	}
	
	virtual void kill() override
	{
		if (onKill != nullptr)
		{
			onKill();
		}
		
		EntityBase::kill();
	}
};

struct Bird : EntityBase
{
	enum State
	{
		kState_Idle, // lookout for danger. and sing or call if it's safe to do so
		kState_Singing, // sing a song. temporarily blind for any dangers
		kState_Calling, // perform a single call. temporarily blind for any dangers
		kState_Flying, // move to a new spot which seems safe
		kState_Settle // settle after flying
	};
	
	State state;
	
	Vec3 currentPos;
	Vec3 desiredPos;
	
	float songTimer;
	float songAnimTimer;
	float songAnimTimerRcp;
	float callAnimTimer;
	float callAnimTimerRcp;
	float moveTimer;
	float settleTimer;
	
	float previousSoundLevel;
	float soundLevelSlowChanging;
	float soundLevelFastChanging;
	float soundPerturbance;
	
	bool mouseHover;
	
	Bird()
		: EntityBase()
		, state(kState_Idle)
		, currentPos()
		, desiredPos()
		, songTimer(0.f)
		, songAnimTimer(0.f)
		, songAnimTimerRcp(0.f)
		, callAnimTimer(0.f)
		, callAnimTimerRcp(0.f)
		, moveTimer(0.f)
		, settleTimer(0.f)
		, previousSoundLevel(0.f)
		, soundLevelSlowChanging(0.f)
		, soundLevelFastChanging(0.f)
		, soundPerturbance(0.f)
		, mouseHover(false)
	{
		type = kEntity_Bird;
		
		graphInstance = g_audioGraphMgr->createInstance("e-bird1.xml");
		
		const int birdType = rand() % 3;
		if (birdType == 0)
			graphInstance->audioGraph->triggerEvent("bird1");
		if (birdType == 1)
			graphInstance->audioGraph->triggerEvent("bird2");
		if (birdType == 2)
			graphInstance->audioGraph->triggerEvent("bird3");
	}
	
	virtual ~Bird() override
	{
		g_audioGraphMgr->free(graphInstance);
	}
	
	void beginSongTimer()
	{
		songTimer = 5.f + random(0.f, 2.f);
	}
	
	void beginSong()
	{
		graphInstance->audioGraph->triggerEvent("sing-begin");
		
		g_world->rippleSound(currentPos);
		
		songAnimTimer = 5.f;
		songAnimTimerRcp = 1.f / songAnimTimer;
	}
	
	void endSong()
	{
		songAnimTimer = 0.f;
		songAnimTimerRcp = 0.f;
	}
	
	void beginCall()
	{
		graphInstance->audioGraph->triggerEvent("call-begin");
		
		g_world->rippleSound(currentPos);
		
		callAnimTimer = 2.f;
		callAnimTimerRcp = 1.f / callAnimTimer;
	}
	
	void endCall()
	{
		callAnimTimer = 0.f;
		callAnimTimerRcp = 0.f;
	}
	
	void beginFlyTimer()
	{
		moveTimer = random(60.f, 100.f);
	}
	
	void beginFlying()
	{
		const float currentRadius = Vec2(currentPos[0], currentPos[1]).CalcSize();
		const float currentAngle = std::atan2f(currentPos[2], currentPos[0]);
		
		float desiredRadius;
		float desiredAngle;
		
		if (currentRadius == 0.f)
		{
			desiredRadius = random(FIELD_SIZE_FOR_FLYING/4.f, FIELD_SIZE_FOR_FLYING);
			desiredAngle = random(0.f, float(M_PI) * 2.f);
		}
		else
		{
			desiredRadius = random(FIELD_SIZE_FOR_FLYING/4.f, FIELD_SIZE_FOR_FLYING);
			desiredAngle = random(0.f, float(M_PI) * 2.f);
			
			//desiredRadius = clamp(currentRadius + random(-FIELD_SIZE_FOR_FLYING/5.f, +FIELD_SIZE_FOR_FLYING/5.f), FIELD_SIZE_FOR_FLYING/4.f, FIELD_SIZE_FOR_FLYING);
			//desiredAngle = currentAngle + random(-2.f, +2.f);
		}
		
		desiredPos[0] = std::cos(desiredAngle) * desiredRadius;
		desiredPos[2] = std::sin(desiredAngle) * desiredRadius;
		desiredPos[1] = random(6.f, 8.f);
		
		graphInstance->audioGraph->triggerEvent("sing-end");
		graphInstance->audioGraph->triggerEvent("call-end");
		
		graphInstance->audioGraph->triggerEvent("fly-begin");
	}
	
	void endFlying()
	{
		graphInstance->audioGraph->triggerEvent("fly-end");
	}
	
	void beginSettleTimer()
	{
		settleTimer = random(3.f, 5.f);
	}
	
	void tickSoundLevels(const float measuredSoundLevel, const float dt)
	{
		{
			const float soundLevelAbs = std::abs(measuredSoundLevel);
			const float soundLevelRetain = std::powf(.75f, dt);
			soundLevelSlowChanging = lerp(soundLevelAbs, soundLevelSlowChanging, soundLevelRetain);
		}
		
		{
			const float soundLevelAbs = std::abs(measuredSoundLevel);
			const float soundLevelRetain = std::powf(.5f, dt);
			soundLevelFastChanging = lerp(soundLevelAbs, soundLevelFastChanging, soundLevelRetain);
		}
		
		soundPerturbance = std::max(0.f, soundLevelFastChanging - soundLevelSlowChanging);
	}
	
	virtual void tick(const float dt) override
	{
		// update movement
		
		const float retain = std::powf(.3f, dt);
		
		const auto oldPos = currentPos;
		
		currentPos = lerp(desiredPos, currentPos, retain);
		
		const auto newPos = currentPos;
		
		float speed = 0.f;
		
		if (dt > 0.f)
		{
			speed = (newPos - oldPos).CalcSize() / dt;
		}
		
		//
		
		const float soundLevel = g_world->measureSound(currentPos);
		
		tickSoundLevels(soundLevel, dt);
		
		// update mouse hover state
		
		const float distanceToMouse = (currentPos - g_world->xformScreenToWorld(mouse.x, mouse.y, currentPos[1])).CalcSize();
		
		mouseHover = distanceToMouse < 1.f;
		
		// kill ?
		
		if (mouseHover && keyboard.wentDown(SDLK_BACKSPACE))
		{
			kill();
		}
		
		// evaluate based on current state
		
		switch (state)
		{
		case kState_Idle:
			{
				songTimer = std::max(0.f, songTimer - dt);
				
				if (songTimer == 0.f)
				{
					if ((rand() % 8) == 0)
					{
						beginCall();
						
						logDebug("idle -> calling");
						state = kState_Calling;
						break;
					}
					else
					{
						beginSong();
						
						logDebug("idle -> singing");
						state = kState_Singing;
						break;
					}
				}
				
				//
				
				moveTimer = std::max(0.f, moveTimer - dt);
				
				if (soundLevelFastChanging > soundLevelSlowChanging * 1.2f || moveTimer == 0.f)
				{
					beginFlying();
					
					logDebug("idle -> flying");
					state = kState_Flying;
					break;
				}
			}
			break;
			
		case kState_Singing:
			{
				songAnimTimer = std::max(0.f, songAnimTimer - dt);
				
				if (songAnimTimer == 0.f)
				{
					endSong();
					
					beginSongTimer();
					
					logDebug("singing -> idle");
					state = kState_Idle;
					break;
				}
				
				/*
				if (songAnimTimer * songAnimTimerRcp <= .5f && soundLevelFastChanging > soundLevelSlowChanging * 1.5f)
				{
					endSong();
					
					beginFlying();
					
					logDebug("singing -> flying");
					state = kState_Flying;
					break;
				}
				*/
			}
			break;
			
		case kState_Calling:
			{
				callAnimTimer = std::max(0.f, callAnimTimer - dt);
				
				if (callAnimTimer == 0.f)
				{
					endCall();
					
					beginSongTimer();
					
					logDebug("calling -> idle");
					state = kState_Idle;
					break;
				}
			}
			break;
			
		case kState_Flying:
			{
				const float strength = speed * dt;
				
				if (strength > 0.f)
				{
					g_world->rippleFlight(oldPos);
				}
				
				//
				
				if ((currentPos - desiredPos).CalcSize() <= .5f)
				{
					endFlying();
					
					beginSettleTimer();
					
					logDebug("flying -> settle");
					state = kState_Settle;
					break;
				}
			}
			break;
			
		case kState_Settle:
			{
				settleTimer = std::max(0.f, settleTimer - dt);
				
				if (settleTimer == 0.f)
				{
					beginSongTimer();
					
					beginFlyTimer();
					
					logDebug("settle -> idle");
					state = kState_Idle;
					break;
				}
			}
			break;
		}
		
		// alive state
		
		if (graphInstance->audioGraph->isFLagSet("dead"))
		{
			dead = true;
		}
		
		//
		
		graphInstance->audioGraph->setMemf("pos", currentPos[0], currentPos[1], currentPos[2]);
		
		//
		
		previousSoundLevel = soundLevel;
	}
	
	virtual void kill() override
	{
		graphInstance->audioGraph->setFlag("kill");
	}
};

struct Voices : EntityBase
{
	Voices()
		: EntityBase()
	{
		type = kEntity_Voice;
		
		graphInstance = g_audioGraphMgr->createInstance("e-voices1.xml");
	}
	
	virtual ~Voices() override
	{
		g_audioGraphMgr->free(graphInstance);
	}
	
	virtual void tick(const float dt) override
	{
		// alive state
		
		if (graphInstance->audioGraph->isFLagSet("dead"))
		{
			dead = true;
		}
	}
	
	virtual void kill() override
	{
		graphInstance->audioGraph->setFlag("kill");
	}
};

struct Machine : EntityBase
{
	enum InputState
	{
		kInputState_Idle,
		kInputState_Drag,
		kInputState_Tension
	};
	
	struct DragState
	{
		Vec3 dragPos;
	};
	
#if MACHINE_V2 == 0
	Vec3 pos;
#endif
	Vec3 worldPos;
	Vec3 desiredWorldPos;
	float workTimer;
	float walkTimer;
	float currentTension;
	float desiredTension;
	
	DragState dragState;
	
	InputState inputState;
	bool mouseHover;
	
	Machine()
		: EntityBase()
	#if MACHINE_V2 == 0
		, pos()
	#endif
		, worldPos()
		, desiredWorldPos()
		, workTimer(0.f)
		, walkTimer(0.f)
		, currentTension(.5f)
		, desiredTension(.5f)
		, dragState()
		, inputState(kInputState_Idle)
		, mouseHover(false)
	{
		type = kEntity_Machine;
		
	#if MACHINE_V2
		graphInstance = g_audioGraphMgr->createInstance("machines1.xml");
	#else
		graphInstance = g_audioGraphMgr->createInstance("e-machine1.xml");
	#endif
		
		randomize();
	}
	
	virtual ~Machine() override
	{
		g_audioGraphMgr->free(graphInstance);
	}
	
	void randomize()
	{
		const float angle = random(0.f, 2.f * float(M_PI));
		const float distance = random(8.f, 12.f);
		
	#if MACHINE_V2
		worldPos[0] = std::cos(angle) * distance;
		worldPos[2] = std::cos(angle) * distance;
		worldPos[1] = 3.f;
		
		desiredWorldPos = worldPos;
	#else
		pos[0] = 0.f;
		pos[1] = 2.f;
		pos[2] = distance;
		
		graphInstance->audioGraph->setMemf("pos", pos[0], pos[1], pos[2]);
		graphInstance->audioGraph->setMemf("rot", Calc::RadToDeg(angle));
		
		const Mat4x4 toWorld = Mat4x4(true).RotateY(angle).Translate(pos);
		worldPos = toWorld * pos;
		
		desiredWorldPos = worldPos;
	#endif
	
		desiredTension = random(0.f, 1.f);
	}
	
	virtual void tick(const float dt) override
	{
		{
			const float retain = std::powf(.2f, dt);
			
			auto oldPos = worldPos;
			
			worldPos = lerp(desiredWorldPos, worldPos, retain);
			
			auto newPos = worldPos;
			
			const float rippleStrength = (newPos - oldPos).CalcSize() * 10.f;
			
			g_world->rippleSound(newPos, random(-rippleStrength, +rippleStrength));
		}
		
		{
			const float retain = std::powf(.85f, dt);
			
			currentTension = lerp(desiredTension, currentTension, retain);
		}
		
		//
		
		workTimer = std::max(0.f, workTimer - dt);
		
		if (workTimer == 0.f)
		{
			workTimer = random(4.f, 6.f);
			
			const float rippleStrength = 50.f;
			
			g_world->rippleSound(worldPos, random(-rippleStrength, +rippleStrength));
			
			graphInstance->audioGraph->triggerEvent("work");
			
			graphInstance->audioGraph->triggerEvent("next");
		}
		
		workTimer = std::max(0.f, workTimer - dt);
		
		//
		
		if (walkTimer == 0.f)
		{
			walkTimer = random(5.f, 8.f);
			
			desiredWorldPos[0] += random(-5.f, +5.f);
			desiredWorldPos[2] += random(-5.f, +5.f);
		}
		
		walkTimer = std::max(0.f, walkTimer - dt);
		
		// update input state
		
		if (inputIsCaptured)
		{
			inputState = kInputState_Idle;
			
			mouseHover = false;
		}
		else
		{
			// update mouse hover state
		
			const float distanceToMouse = (worldPos - g_world->xformScreenToWorld(mouse.x, mouse.y, worldPos[1])).CalcSize();
			
			mouseHover = distanceToMouse < 1.f;
			
			if (inputState == kInputState_Idle)
			{
				if (mouseHover && mouse.wentDown(BUTTON_LEFT))
				{
					dragState.dragPos = worldPos;
					
					inputState = kInputState_Drag;
				}
				else if (mouseHover && mouse.wentDown(BUTTON_RIGHT))
				{
					inputState = kInputState_Tension;
				}
				else if (mouseHover && keyboard.wentDown(SDLK_BACKSPACE))
				{
					kill();
				}
			}
			else if (inputState == kInputState_Drag)
			{
				dragState.dragPos = g_world->xformScreenToWorld(mouse.x, mouse.y, dragState.dragPos[1]);
				
				if (mouse.wentUp(BUTTON_LEFT))
				{
					desiredWorldPos = dragState.dragPos;
					
					dragState = DragState();
					
					inputState = kInputState_Idle;
				}
			}
			else if (inputState == kInputState_Tension)
			{
				desiredTension = saturate(desiredTension - mouse.dy / 100.f);
				
				if (mouse.wentUp(BUTTON_RIGHT))
				{
					inputState = kInputState_Idle;
				}
			}
		}
		
	#if MACHINE_V2
		graphInstance->audioGraph->setMemf("pos", worldPos[0], worldPos[1], worldPos[2]);
	#else
		const float angle = atan2f(worldPos[2], worldPos[0]);
		const float distance = worldPos.CalcSize();
		pos[2] = distance;
		graphInstance->audioGraph->setMemf("pos", pos[0], pos[1], pos[2]);
		graphInstance->audioGraph->setMemf("rot", Calc::RadToDeg(angle));
	#endif
	
		graphInstance->audioGraph->setMemf("tension", currentTension);
		
		// alive state
		
		if (graphInstance->audioGraph->isFLagSet("dead"))
		{
			dead = true;
		}
		
		//
		
		inputIsCaptured |= (inputState != kInputState_Idle);
	}
	
	virtual void kill() override
	{
		graphInstance->audioGraph->setFlag("kill");
	}
};

//

#include "wavefield.h"

struct World : WorldInterface
{
	enum InputState
	{
		kInputState_Idle,
		kInputState_Wavefield
	};
	
	struct Parameters
	{
		float wavefieldC;
		float wavefieldD;
		
		float birdVolume;
		float voiceVolume;
		float machineVolume;
		float craziness;
		
		Parameters()
			: wavefieldC(1000.f)
			, wavefieldD(1.f)
			, birdVolume(1.f)
			, voiceVolume(1.f)
			, machineVolume(1.f)
			, craziness(0.f)
		{
		}
		
		void lerpTo(const Parameters & other, const float dt)
		{
			{
				const float retain = std::powf(.1f, dt);
				
				wavefieldC = lerp(other.wavefieldC, wavefieldC, retain);
				wavefieldD = lerp(other.wavefieldD, wavefieldD, retain);
			}
			
			birdVolume = other.birdVolume;
			voiceVolume = other.voiceVolume;
			machineVolume = other.machineVolume;
			craziness = other.craziness;
		}
	};
	
	AudioGraphInstance * globalsInstance;
	
	std::vector<EntityBase*> entities;
	
	int currentNumOneshots;
	int desiredNumOneshots;
	
	Mat4x4 worldToScreen;
	
	Wavefield2D wavefield;
	Mat4x4 wavefieldToWorld;
	Mat4x4 wavefieldToScreen;
	
	InputState inputState;
	
	Parameters desiredParams;
	Parameters currentParams;
	
	bool showBirdDebugs;
	
	World()
		: globalsInstance(nullptr)
		, entities()
		, currentNumOneshots(0)
		, desiredNumOneshots(0)
		, worldToScreen(true)
		, wavefield()
		, wavefieldToWorld(true)
		, wavefieldToScreen(true)
		, inputState(kInputState_Idle)
		, desiredParams()
		, currentParams()
		, showBirdDebugs(false)
	{
	}
	
	void init()
	{
		globalsInstance = g_audioGraphMgr->createInstance("globals.xml");
		
		//
		
		worldToScreen = Mat4x4(true).Translate(GFX_SX/3, GFX_SY/2, 0).Scale(16.f, 16.f, 1.f);
		
		wavefield.init(64);
		wavefield.randomize();
		
		wavefieldToWorld = Mat4x4(true).Scale(FIELD_SIZE, FIELD_SIZE, 1.f).Translate(-1.f, -1.f, 0.f).Scale(2.f, 2.f, 1.f).Scale(1.f / wavefield.numElems, 1.f / wavefield.numElems, 1.f);
		wavefieldToScreen = worldToScreen * wavefieldToWorld;
	}
	
	void shut()
	{
		for (auto entity : entities)
		{
			delete entity;
			entity = nullptr;
		}
		
		entities.clear();
		
		g_audioGraphMgr->free(globalsInstance);
	}
	
	void addBall()
	{
		Ball * ball = new Ball();
			
		ball->pos[0] = random(-20.f, +20.f);
		ball->pos[1] = random(+10.f, +20.f);
		ball->pos[2] = random(-20.f, +20.f);
		ball->vel[0] = random(-3.f, +3.f);
		ball->vel[1] = random(-5.f, +5.f);
		ball->vel[2] = random(-3.f, +3.f);
		
		entities.push_back(ball);
	}
	
	void addBird()
	{
		Bird * bird = new Bird();
		
		bird->beginFlying();
		
		bird->state = Bird::kState_Flying;
		
		entities.push_back(bird);
	}
	
	void addVoices()
	{
		Voices * voices = new Voices();
		
		entities.push_back(voices);
	}
	
	void addMachine()
	{
		Machine * machine = new Machine();
		
		entities.push_back(machine);
	}
	
	void killEntity()
	{
		if (entities.empty() == false)
		{
			EntityBase *& entity = entities.back();
			
			entity->kill();
		}
	}
	
	Oneshot * doOneshot()
	{
		const char * filename = (rand() % 2) == 0 ? "oneshotTest.xml" : "oneshotTest2.xml";
		
		Oneshot * oneshot = new Oneshot(filename, random(1.f, 5.f), true);
		
		const float sizeX = 6.f;
		const float sizeY = 1.f;
		const float sizeZ = 6.f;
		
		oneshot->pos = Vec3(
			random(-sizeX, +sizeX),
			random(-sizeY, +sizeY),
			random(-sizeZ, +sizeZ));
		oneshot->pos *= 2.f;
		
		oneshot->vel[0] = random(-10.f, +10.f);
		oneshot->vel[1] = 6.f;
		oneshot->vel[2] = random(-10.f, +10.f);
		oneshot->velGrow = 2.f;
		
		oneshot->dimGrow = 2.f;
		
		oneshot->graphInstance->audioGraph->setMemf("delay", random(0.0002f, 0.2f));
		entities.push_back(oneshot);
		
		return oneshot;
	}
	
	bool tick(const float dt)
	{
		if (inputIsCaptured == true)
		{
			switch (inputState)
			{
			case kInputState_Idle:
				break;
			
			case kInputState_Wavefield:
				inputState = kInputState_Idle;
				break;
			}
		}
		else
		{
			switch (inputState)
			{
			case kInputState_Idle:
				{
					if (mouse.wentDown(BUTTON_LEFT))
					{
						inputState = kInputState_Wavefield;
						break;
					}
				}
				break;
			
			case kInputState_Wavefield:
				{
					const Vec2 mouseScreen(mouse.x, mouse.y);
					const Vec2 mouseWavefield = wavefieldToScreen.Invert().Mul4(mouseScreen);
					
					const float impactX = mouseWavefield[0];
					const float impactY = mouseWavefield[1];
					
					wavefield.doGaussianImpact(impactX, impactY, 3, currentParams.wavefieldD);
					
					if (mouse.wentUp(BUTTON_LEFT))
					{
						inputState = kInputState_Idle;
						break;
					}
				}
				break;
			}
		}
		
		//
		
		currentParams.lerpTo(desiredParams, dt);
		
		//
		
		globalsInstance->audioGraph->setMemf("crazy", currentParams.craziness);
		
		//
		
		if (currentNumOneshots < desiredNumOneshots)
		{
			currentNumOneshots++;
			
			auto oneshot = doOneshot();
			
			oneshot->onKill = [&]
			{
				currentNumOneshots--;
			};
		}
		
		//
		
		for (int i = 0; i < 10; ++i)
		{
			wavefield.tick(dt / 10.f, currentParams.wavefieldC, 0.2, 0.5, true);
		}
		
		//
		
		for (auto entity : entities)
		{
			entity->tick(dt);
			
			if (entity->type == kEntity_Bird)
			{
				entity->graphInstance->audioGraph->setMemf("vol", currentParams.birdVolume);
			}
			
			if (entity->type == kEntity_Voice)
			{
				entity->graphInstance->audioGraph->setMemf("vol", currentParams.voiceVolume);
			}
			
			if (entity->type == kEntity_Machine)
			{
				entity->graphInstance->audioGraph->setMemf("vol", currentParams.machineVolume);
			}
		}
		
		//
		
		for (auto entityItr = entities.begin(); entityItr != entities.end(); )
		{
			EntityBase *& entity = *entityItr;
			
			if ((*entityItr)->dead)
			{
				delete entity;
				entity = nullptr;
				
				entityItr = entities.erase(entityItr);
			}
			else
			{
				entityItr++;
			}
		}
		
		return (inputState != kInputState_Idle);
	}
	
	void draw()
	{
		gxPushMatrix();
		{
			gxMultMatrixf(wavefieldToScreen.m_v);
			
			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (int x = 0; x < wavefield.numElems; ++x)
				{
					for (int y = 0; y < wavefield.numElems; ++y)
					{
						const float pValue = saturate(.5f + wavefield.p[x][y] / 1.f);
						const float fValue = saturate(wavefield.f[x][y] / 1.f);
						const float vValue = saturate(.5f + wavefield.v[x][y] * .5f);
						setColorf(0.1f, pValue, fValue, 1.f);
						hqFillCircle(x + .5f, y + .5f, vValue * .25f);
					}
				}
			}
			hqEnd();
			
			for (int ox = 0; ox < 2; ++ox)
			{
				for (int oy = 0; oy < 2; ++oy)
				{
					const Vec2 world = wavefieldToWorld.Mul4(Vec2(wavefield.numElems * ox, wavefield.numElems * oy));
					
					setColor(colorWhite);
					drawText(
						ox * wavefield.numElems,
						oy * wavefield.numElems,
						1.5f,
						0, 0,
						"(%.2f, %.2f)",
						world[0], world[1]);
				}
			}
		}
		gxPopMatrix();
		
		gxPushMatrix();
		{
			gxMultMatrixf(worldToScreen.m_v);
			
			const float rect[4][2] =
			{
				{ -6, -6 },
				{ +6, -6 },
				{ +6, +6 },
				{ -6, +6 }
			};
			
			hqBegin(HQ_LINES, true);
			{
				for (int i = 0; i < 4; ++i)
				{
					const auto v1 = rect[(i + 0) % 4];
					const auto v2 = rect[(i + 1) % 4];
					
					setColor(255, 255, 255, 127);
					hqLine(v1[0], v1[1], 1.f, v2[0], v2[1], 1.f);
				}
			}
			hqEnd();
			
			for (int i = 0; i < 4; ++i)
			{
				const auto v = rect[(i + 0) % 4];
				
				setColor(colorWhite);
				drawText(
					v[0],
					v[1],
					.8f,
					0, 0,
					"(%.2f, %.2f)",
					v[0], v[1]);
			}
			
			for (auto entity : entities)
			{
				if (entity->type == kEntity_Machine)
				{
					Machine * machine = (Machine*)entity;
					
					if (machine->inputState == Machine::kInputState_Drag)
					{
						hqBegin(HQ_LINES);
						{
							setColorf(1, 1, 1, .5f);
							hqLine(machine->worldPos[0], machine->worldPos[2], 1.f, machine->dragState.dragPos[0], machine->dragState.dragPos[2], 1.f);
						}
						hqEnd();
					}

					hqBegin(HQ_FILLED_CIRCLES);
					{
						setColorf(1.f, 1.f, 1.f, .7f);
						hqFillCircle(machine->worldPos[0], machine->worldPos[2], .1f + .8f * machine->currentTension);
					}
					hqEnd();
					
					hqBegin(HQ_STROKED_CIRCLES);
					{
						setColorf(1.f, 1.f, 1.f, 1.f);
						hqStrokeCircle(machine->worldPos[0], machine->worldPos[2], .1f + .8f * machine->desiredTension, 3.f);
					}
					hqEnd();
					
					hqBegin(HQ_STROKED_CIRCLES);
					{
						if (machine->mouseHover)
							setColor(colorWhite);
						else
							setColorf(1.f, 0.f, 0.f);
						hqStrokeCircle(machine->worldPos[0], machine->worldPos[2], 1.f, 3.f);
					}
					hqEnd();
				}
			}
			
			for (auto entity : entities)
			{
				if (entity->type == kEntity_Bird)
				{
					Bird * bird = (Bird*)entity;
					
					if (bird->songAnimTimer > 0.f)
					{
						hqBegin(HQ_STROKED_CIRCLES);
						{
							const float t = 1.f - std::fmodf(bird->songAnimTimer * bird->songAnimTimerRcp * 3.f, 1.f);
							
							setColorf(1.f, 1.f, 1.f, 1.f - t);
							hqStrokeCircle(bird->currentPos[0], bird->currentPos[2], .5f + 5.f * t, 3.f);
						}
						hqEnd();
					}
					
					if (bird->callAnimTimer > 0.f)
					{
						hqBegin(HQ_STROKED_CIRCLES);
						{
							const float t = 1.f - bird->callAnimTimer * bird->callAnimTimerRcp;
							
							setColorf(.5f, .5f, 1.f, 1.f - t);
							hqStrokeCircle(bird->currentPos[0], bird->currentPos[2], .5f + 5.f * t, 3.f);
						}
						hqEnd();
					}
					
					hqBegin(HQ_FILLED_CIRCLES);
					{
						setColor(bird->mouseHover ? colorYellow : colorWhite);
						hqFillCircle(bird->currentPos[0], bird->currentPos[2], .5f);
					}
					hqEnd();
					
					if (showBirdDebugs)
					{
						drawText(
							bird->currentPos[0],
							bird->currentPos[2] + 1.f,
							1.2f, 0, 1,
							"lFast=%.2f, lSlow=%.2f, p=%.2f",
							bird->soundLevelFastChanging,
							bird->soundLevelSlowChanging,
							bird->soundPerturbance);
					}
				}
			}
		}
		gxPopMatrix();
	}
	
	Vec2 constrainSamplePosition(const Vec2 & p, const int edge) const
	{
		return p.Max(Vec2(edge, edge)).Min(Vec2(wavefield.numElems-1-edge, wavefield.numElems-1-edge));
	}
	
	virtual Vec2 xformWorldToScreen(const Vec3 & p) override
	{
		return worldToScreen.Mul4(Vec2(p[0], p[2]));
	}
	
	virtual Vec3 xformScreenToWorld(const float x, const float y, const float worldY) override
	{
		auto r = worldToScreen.Invert().Mul4(Vec2(x, y));
		return Vec3(r[0], worldY, r[1]);
	}
	
	virtual void rippleSound(const Vec3 & p, const float amount) override
	{
		const Vec2 samplePosition = constrainSamplePosition(wavefieldToWorld.Invert().Mul4(Vec2(p[0], p[2])), 3);
		
		wavefield.doGaussianImpact(samplePosition[0], samplePosition[1], 3, amount);
	}
	
	virtual void rippleFlight(const Vec3 & p) override
	{
		const Vec2 samplePosition = constrainSamplePosition(wavefieldToWorld.Invert().Mul4(Vec2(p[0], p[2])), 2);
		
		wavefield.doGaussianImpact(samplePosition[0], samplePosition[1], 2, .05f);
	}
	
	virtual float measureSound(const Vec3 & p) override
	{
		const Vec2 samplePosition = wavefieldToWorld.Invert().Mul4(Vec2(p[0], p[2]));
		
		return wavefield.sample(samplePosition[0], samplePosition[1]);
	}
	
	virtual float measureFlight(const Vec3 & p) override
	{
		const Vec2 samplePosition = wavefieldToWorld.Invert().Mul4(Vec2(p[0], p[2]));
		
		return wavefield.sample(samplePosition[0], samplePosition[1]);
	}
};

//

void testAudioGraphManager()
{
	SDL_mutex * mutex = SDL_CreateMutex();
	
	//
	
	fillPcmDataCache("birds", true, false);
	fillPcmDataCache("testsounds", true, true);
	fillPcmDataCache("voices", true, false);
	
	//
	
	//fillHrirSampleSetCache("binaural/CIPIC/subject10", "cipic.10", kHRIRSampleSetType_Cipic);
	//fillHrirSampleSetCache("binaural/CIPIC/subject11", "cipic.11", kHRIRSampleSetType_Cipic);
	//fillHrirSampleSetCache("binaural/CIPIC/subject12", "cipic.12", kHRIRSampleSetType_Cipic);
	//fillHrirSampleSetCache("binaural/CIPIC/subject147", "cipic.147", kHRIRSampleSetType_Cipic);
	
	//
	
	{
		PortAudioObject pa;
		if (pa.isSupported(0, 16))
			STEREO_OUTPUT = false;
	}
	
	//
	
	const int kNumChannels = 16;
	
	AudioVoiceManager voiceMgr;
	voiceMgr.init(kNumChannels);
	voiceMgr.outputStereo = STEREO_OUTPUT;
	g_voiceMgr = &voiceMgr;
	
	//
	
	AudioGraphManager audioGraphMgr;
	audioGraphMgr.init(mutex);
	
	Assert(g_audioGraphMgr == nullptr);
	g_audioGraphMgr = &audioGraphMgr;
	
	//
	
	World * world = nullptr;
	
	//
	
	std::string oscIpAddress = "192.168.1.10";
	//std::string oscIpAddress = "127.0.0.1";
	int oscUdpPort = 2000;
	
	AudioUpdateHandler audioUpdateHandler;
	
	audioUpdateHandler.init(mutex, oscIpAddress.c_str(), oscUdpPort);
	audioUpdateHandler.voiceMgr = &voiceMgr;
	audioUpdateHandler.audioGraphMgr = &audioGraphMgr;
	
	//
	
	PortAudioObject pa;
	
	pa.init(SAMPLE_RATE, STEREO_OUTPUT ? 2 : kNumChannels, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
	
	//
	
	UiState uiState;
	
	std::string activeInstanceName;
	
	bool interact = true;
	bool developer = false;
	std::string testInstanceFilename;
	bool graphList = true;
	bool instanceList = false;
	
	auto doMenus = [&](const bool doActions, const bool doDraw, const float dt) -> bool
	{
		uiState.sx = 200;
		uiState.x = GFX_SX - uiState.sx - 10 - 200 - 10;
		uiState.y = 10;
		
		makeActive(&uiState, doActions, doDraw);
		
		pushMenu("control");
		{
			doDrawer(interact, "control");
			if (interact)
			{
				if (world == nullptr)
				{
					if (doButton("create world"))
					{
						world = new World();
						world->init();
						
						g_world = world;
					}
				}
				else
				{
					if (doButton("randomize wavefield"))
						world->wavefield.randomize();
					doTextBox(world->desiredNumOneshots, "oneshots.num", dt);
					if (doButton("add ball"))
						world->addBall();
					if (doButton("add bird"))
						world->addBird();
					if (doButton("add voices"))
						world->addVoices();
					if (doButton("add machine"))
						world->addMachine();
					if (doButton("kill selected"))
					{
						if (g_audioGraphMgr->selectedFile && g_audioGraphMgr->selectedFile->activeInstance)
						{
							auto graphInstance = g_audioGraphMgr->selectedFile->activeInstance;
							
							for (auto e : world->entities)
							{
								if (e->graphInstance == graphInstance)
									e->kill();
							}
						}
					}
					doBreak();
					
					doSlider(world->desiredParams.birdVolume, "bird volume", .6f, dt);
					doBreak();
					doSlider(world->desiredParams.voiceVolume, "voice volume", .6f, dt);
					doBreak();
					doSlider(world->desiredParams.machineVolume, "machine volume", .6f, dt);
					doBreak();
					doSlider(world->desiredParams.craziness, "craziness", .6f, dt);
				}
			}
		}
		popMenu();
		
		doBreak();
		
		pushMenu("developer");
		{
			if (world != nullptr && doDrawer(developer, "developer"))
			{
				doCheckBox(world->showBirdDebugs, "show bird debugs", false);
				
				doTextBox(world->desiredParams.wavefieldC, "wavefield.c", dt);
				doTextBox(world->desiredParams.wavefieldD, "wavefield.d", dt);
				doBreak();
		
				if (doButton("kill entity"))
					world->killEntity();
				if (doButton("do oneshot"))
					world->doOneshot();
				doBreak();
				//
				
				bool addTestInstance = false;
				bool killTestInstances = false;
				
				if (doTextBox(testInstanceFilename, "filename", dt) == kUiTextboxResult_EditingComplete && !testInstanceFilename.empty())
				{
					addTestInstance = true;
					killTestInstances = true;
				}
				
				addTestInstance |= doButton("add test instance") && !testInstanceFilename.empty();
				killTestInstances |= doButton("kill test instances");
				
				if (killTestInstances)
				{
					for (auto & entity : world->entities)
					{
						if (entity->type == kEntity_TestInstance)
						{
							entity->kill();
						}
					}
				}
				
				if (addTestInstance)
				{
					std::string fullFilename = testInstanceFilename;
					if (Path::GetExtension(fullFilename, true) != "xml")
						fullFilename = Path::ReplaceExtension(fullFilename, "xml");
					TestInstance * instance = new TestInstance(fullFilename.c_str());
					world->entities.push_back(instance);
					g_audioGraphMgr->selectInstance(instance->graphInstance);
				}
			}
		}
		popMenu();
		
		doBreak();
		
		pushMenu("graphList");
		{
			doDrawer(graphList, "graphs");
			if (graphList)
			{
				for (auto & fileItr : audioGraphMgr.files)
				{
					auto & filename = fileItr.first;
					
					if (doButton(filename.c_str()))
					{
						audioGraphMgr.selectFile(filename.c_str());
					}
				}
			}
		}
		popMenu();
		
		/*
		doBreak();
		
		pushMenu("instanceList");
		{
			static float sliderValue = 0.f;
			doSlider(sliderValue, "slider");
			
			doDrawer(instanceList, "instances");
			if (instanceList)
			{
				const std::string voiceCount = String::FormatC("voices: %d", voiceMgr.numChannelsUsed());
				doLabel(voiceCount.c_str(), 0.f);
				
				for (auto & fileItr : audioGraphMgr.files)
				{
					auto & filename = fileItr.first;
					auto file = fileItr.second;
					
					for (auto & instance : file->instanceList)
					{
						std::string name = String::FormatC("%s: %p", filename.c_str(), instance.audioGraph);
						
						if (doButton(name.c_str()))
						{
							if (name != activeInstanceName)
							{
								activeInstanceName = name;
								
								audioGraphMgr.selectInstance(&instance);
							}
						}
					}
				}
			}
		}
		popMenu();
		*/
		
		return uiState.activeElem != nullptr;
	};
	
	do
	{
		framework.process();
		
		//
		
		const float dt = std::min(1.f / 20.f, framework.timeStep);
		
		inputIsCaptured = false;
		
		/*
		bool graphEditHasInputCapture =
			audioGraphMgr.selectedFile != nullptr &&
			audioGraphMgr.selectedFile->graphEdit->state != GraphEdit::kState_Idle &&
			audioGraphMgr.selectedFile->graphEdit->state != GraphEdit::kState_Hidden &&
			audioGraphMgr.selectedFile->graphEdit->state != GraphEdit::kState_HiddenIdle;
		
		bool inputIsCaptured = false;
		*/
		
		//if (graphEditHasInputCapture == false)
		{
			inputIsCaptured |= doMenus(true, false, dt);
		}
		
		inputIsCaptured |= audioGraphMgr.tickEditor(dt, inputIsCaptured);
		
		if (audioGraphMgr.selectedFile)
		{
			inputIsCaptured |= audioGraphMgr.selectedFile->graphEdit->state != GraphEdit::kState_Hidden;
		}
		
		if (world != nullptr)
		{
			inputIsCaptured |= world->tick(dt);
		}
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			pushFontMode(FONT_SDF);
			{
				if (world != nullptr)
				{
					world->draw();
				}
				
				audioGraphMgr.drawEditor();
				
				doMenus(false, true, dt);
				
				if (audioGraphMgr.selectedFile && audioGraphMgr.selectedFile->activeInstance)
				{
					int64_t audioCpuTime = 0;
					
					SDL_LockMutex(audioUpdateHandler.mutex);
					{
						audioCpuTime = audioUpdateHandler.cpuTime;
					}
					SDL_UnlockMutex(audioUpdateHandler.mutex);
					
					setColor(colorGreen);
					drawText(GFX_SX/2, 20, 20, 0, 0, "- 4DWORLD :: %s: %p :: %.2f%% -",
						audioGraphMgr.selectedFile->filename.c_str(),
						audioGraphMgr.selectedFile->activeInstance->audioGraph,
						audioCpuTime / 1000000.f * 100.f);
				}
			}
			popFontMode();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_ESCAPE));
	
	pa.shut();
	
	//
	
	audioUpdateHandler.shut();
	
	//
	
	if (world != nullptr)
	{
		g_world = nullptr;
		
		world->shut();
		
		delete world;
		world = nullptr;
	}
	
	//
	
	Assert(g_audioGraphMgr == &audioGraphMgr);
	g_audioGraphMgr = nullptr;
	
	audioGraphMgr.shut();
	
	//
	
	voiceMgr.shut();
	
	Assert(g_voiceMgr == &voiceMgr);
	g_voiceMgr = nullptr;
	
	//
	
	SDL_DestroyMutex(mutex);
	mutex = nullptr;
	
	//
	
	exit(0);
}
