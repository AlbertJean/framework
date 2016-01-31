#include "Calc.h"
#include "framework.h"
#include "OptionMenu.h"
#include "Options.h"

#include "audio.h"
#include "audiostream/AudioOutput.h"

#include <Windows.h>

#define SX 1920
#define SY 1080

#define WORLD_RADIUS 400
#define WORLD_X 0
#define WORLD_Y 0
#define MAX_LEMMINGS 100

#define LEMMING_SPRITE Spriter("Art Assets/Animation/Shoon/Shoon_anim_000.scml")
#define HEART_SPRITE Spriter("Art Assets/Animation/Planet_heart/Heart_Life.scml")
#define HEART_FACE_SPRITE Spriter("Art Assets/Animation/Faces_Earth/Earth_Faces_000.scml")
#define PLAYER_SPRITE Spriter("Art Assets/Animation/Shoon/Shoon_anim_000.scml")
#define SUN_SPRITE Spriter("Art Assets/Animation/Planet_heart/Heart_Life.scml")
#define SUN_FACE_SPRITE Spriter("Art Assets/Sun/Sun_animations.scml")
#define PARTICLE_SPRITE Spriter("Art Assets/Animation/Particules/Particules.scml")
#define DIAMOND_SPRITE Spriter("Art Assets/Animation/Diamonds/Diamond.scml")

int activeAudioSet = 0;

OPTION_DECLARE(bool, DEBUG_DRAW, true);
OPTION_DEFINE(bool, DEBUG_DRAW, "Debug Draw");

OPTION_DECLARE(int, SUN_DISTANCE, 700);
OPTION_DEFINE(int, SUN_DISTANCE, "Sun/Distance");
OPTION_STEP(SUN_DISTANCE, 0, 1000, 50);

OPTION_DECLARE(float, H, 1.0f);
OPTION_DEFINE(float, H, "H");
OPTION_STEP(H, 0.0f, 1.0f, 0.01f);

OPTION_DECLARE(float, S, 1.0f);
OPTION_DEFINE(float, S, "S");
OPTION_STEP(S, 0.0f, 1.0f, 0.01f);

OPTION_DECLARE(float, L, 1.0f);
OPTION_DEFINE(float, L, "L");
OPTION_STEP(L, 0.0f, 1.0f, 0.01f);

struct SoundInfo
{
	std::string fmt;
	int count;
	float volume;
};

static std::map<std::string, SoundInfo> s_sounds;

static void addSound(const char * name, const char * fmt, int count, float volume = 1.f)
{
	SoundInfo & s = s_sounds[name];
	s.fmt = fmt;
	s.count = count;
	s.volume = volume;
}

static void initSound()
{
	// worshipper
	addSound("pray", "Sound Assets/SFX/SFX_Chief_Convert_to Prayer.ogg", 1);
	addSound("pray_complete", "Sound Assets/SFX/SFX_Worshipper_Prayer_Complete.ogg", 1);
	addSound("pray_pickup", "Sound Assets/SFX/SFX_Prayer_PickUp.ogg", 1);
	addSound("stop_prayer_hit", "Sound Assets/SFX/SFX_Chief_Stop_Prayer_Hit_%02d.ogg", 4);

	addSound("pickup", "Sound Assets/SFX/SFX_Worshiper_Picked_Up_%02d.ogg", 4);
	addSound("throw", "Sound Assets/SFX/SFX_Worshipper_Throw_%02d.ogg", 5);

	// earth
	addSound("earth_transition_frozen", "Sound Assets/SFX/SFX_Earth_Transition_to_Frozen.ogg", 1);
	addSound("earth_transition_cold", "Sound Assets/SFX/SFX_Earth_Transition_to_Cold.ogg", 1);
	addSound("earth_transition_warm", "Sound Assets/SFX/SFX_Earth_Transition_to_Warm.ogg", 1);
	addSound("earth_transition_hot", "Sound Assets/SFX/SFX_Earth_Transition_to_Hot.ogg", 1);

	// sun
	addSound("sun_transition_depressed", "Sound Assets/SFX/SFX_Sun_Depressed.ogg", 1);
	addSound("sun_transition_unhappy", "Sound Assets/SFX/SFX_Sun_Unhappy.ogg", 1);
	addSound("sun_transition_happy", "Sound Assets/SFX/SFX_Sun_Happy_New.ogg", 1);
	addSound("sun_transition_manichappy", "Sound Assets/SFX/SFX_Sun_Maniac_Happy.ogg", 1);
}

static void playSound(const char * name)
{
	log("playSound: %s", name);

	if (s_sounds.count(name) == 0)
	{
		logError("sound not found: %s", name);
		return;
	}

	SoundInfo & s = s_sounds[name];
	char temp[256];
	sprintf(temp, s.fmt.c_str(), 1 + (rand() % s.count));
	Sound(temp).play(s.volume * 100);
}

//

class Sun
{
public:
	enum Emotion
	{
		kEmotion_Happy,
		kEmotion_Neutral,
		kEmotion_Sad
	};

	Sun()
	{
		angle = 0.0f;
		distance = 700.0f;
		speed = 0.1f;
		prays_needed = 2;
		emotion = kEmotion_Neutral;
		face = 0;
		actual_face = 0;
	}

	void spawn()
	{
		spriteState.startAnim(SUN_SPRITE, 0);
		spriteFaceState.startAnim(SUN_FACE_SPRITE, actual_face);

		color = colorYellow;
		targetColor = color;
	}

	void tickEmotion()
	{
		static Emotion lastEmotion = kEmotion_Neutral;

		if (emotion != lastEmotion)
		{
			lastEmotion = emotion;

		#ifndef DEBUG
			if (emotion == kEmotion_Sad)
				playSound("sun_transition_depressed");
			if (emotion == kEmotion_Neutral)
				playSound("sun_transition_unhappy");
			if (emotion == kEmotion_Happy)
				playSound("sun_transition_happy");
			// todo
			//if (emotion == kEmotion_Lucide)
			//	playSound("sun_transition_manichappy");
		#endif
		}

		if (face != actual_face)
		{
			actual_face = face;
			spriteFaceState.startAnim(SUN_FACE_SPRITE, actual_face);
		}
		if (emotion == kEmotion_Happy)
		{
			targetColor = colorRed;
			activeAudioSet = 3;
			face = 2;
		}
		else if (emotion == kEmotion_Neutral)
		{
			targetColor = colorYellow;
			activeAudioSet = 2;
			face = 1;
		}
		else if (emotion == kEmotion_Sad)
		{
			targetColor = colorBlue;
			activeAudioSet = 1;
			face = 1;
		}

		color = color.interp(targetColor, 0.01f);
	}

	void tick(float dt)
	{
		distance = SUN_DISTANCE;

		angle += dt * speed;
		spriteState.updateAnim(SUN_SPRITE, dt);
		spriteFaceState.updateAnim(SUN_FACE_SPRITE, dt);

		if (prays_needed > 0)
			emotion = kEmotion_Sad;
		else if (prays_needed < 0)
			emotion = kEmotion_Happy;
		else
			emotion = kEmotion_Neutral;

		tickEmotion();
	}

	void draw()
	{
		spriteState.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + distance);
		spriteState.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + distance);
		spriteState.scale = 1.5f;
		setColor(color);
		SUN_SPRITE.draw(spriteState);
		spriteFaceState.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + distance);
		spriteFaceState.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + distance);
		spriteFaceState.scale = 0.4f;
		setColor(color);
		SUN_FACE_SPRITE.draw(spriteFaceState);
	}

	SpriterState spriteState;
	SpriterState spriteFaceState;
	float angle;
	float distance;
	float speed;
	int sun_state; // 0 not happy, 1 it's okey, 2, too happy
	int prays_needed;
	Emotion emotion;
	Color color;
	Color targetColor;
	int face;
	int actual_face;
};


class Heart_Faces
{
public:
	Heart_Faces()
	{
		animation = 0;
	}

	void update_animation(int anim)
	{
		if (animation != anim)
			spriteState.startAnim(HEART_FACE_SPRITE, anim);
		animation = anim;
	}

	void tick(float dt)
	{
		spriteState.updateAnim(HEART_FACE_SPRITE, dt);
	}

	void draw(Color color)
	{
		spriteState.x = WORLD_X;
		spriteState.y = WORLD_Y;
		spriteState.scale = 1.0f;
		setColor(color);
		HEART_FACE_SPRITE.draw(spriteState);
	}

	SpriterState spriteState;
	int animation;
};

class Lemming
{
public:
	Lemming()
	{
		isActive = false;
		pray = false;
		angle = 0.0f;
		speed = 0.0f;
		state = 2;
		sunHit = false;
		sunHitProcessed = false;
		distance = 0.0f;
		real_distance = 0.0f;
		diamond_distance = 150.0f;
		diamond = true;
	}

	void spawn()
	{
		spriteState.startAnim(LEMMING_SPRITE, state);
		spriteStateDiamond.startAnim(DIAMOND_SPRITE, 0);
		spriteStateParticles.startAnim(PARTICLE_SPRITE, 0);
	}

	void tick(float dt)
	{
		const int oldState = state;

		real_distance += (distance - real_distance) * dt * 10.0f;
		bool animIsDone = spriteState.updateAnim(LEMMING_SPRITE, dt);
		if (animIsDone)
		{
			if (!state)
				state = 1;
			if (state == 3)
				state = 2;
			spriteState.startAnim(LEMMING_SPRITE, state);
		}
		if (state == 1)
		{
			if (diamond)
				spriteStateDiamond.updateAnim(DIAMOND_SPRITE, dt);
			spriteStateParticles.updateAnim(PARTICLE_SPRITE, dt);
			if (sunHit)
				diamond_distance += 200.0f * dt;
			if (!sunHit && diamond_distance > 150.0f && diamond)
				diamond_distance -= 400.0f * dt;
			if (diamond_distance > 450.0f)
			{
				state = 3;
				spriteStateDiamond.stopAnim(DIAMOND_SPRITE);
				diamond = false;
			}
		}

		if (state != oldState)
		{
			if (state == 1)
				playSound("pray_complete");
		}

		angle += speed * dt;
		speed = speed * powf(0.01f, dt);
	}

	void draw()
	{
		spriteState.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + real_distance);
		spriteState.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + real_distance);
		spriteState.scale = 0.4f;
		spriteState.angle = angle * Calc::rad2deg + 90;
		setColor(colorWhite);
		LEMMING_SPRITE.draw(spriteState);
		if (state == 1)
		{
			spriteStateParticles.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + real_distance + 50);
			spriteStateParticles.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + real_distance + 50);
			spriteStateParticles.scale = 0.4f;
			spriteStateParticles.angle = angle * Calc::rad2deg + 90;
			setColor(colorWhite);
			PARTICLE_SPRITE.draw(spriteStateParticles);
			if (diamond)
			{
				spriteStateDiamond.x = WORLD_X + cosf(angle) * (WORLD_RADIUS + real_distance + diamond_distance);
				spriteStateDiamond.y = WORLD_Y + sinf(angle) * (WORLD_RADIUS + real_distance + diamond_distance);
				spriteStateDiamond.scale = 0.4f;
				spriteStateDiamond.angle = angle * Calc::rad2deg + 90;
				setColor(colorWhite);
				DIAMOND_SPRITE.draw(spriteStateDiamond);
			}
		}
	}

	void doPray()
	{
		pray = (pray ? false : true);
		state = (pray ? 0 : 3);
		spriteState.startAnim(LEMMING_SPRITE, state);
	}

	bool isActive;
	SpriterState spriteState;
	SpriterState spriteStateParticles;
	SpriterState spriteStateDiamond;
	float angle;
	float speed;
	bool pray;
	int state;
	bool sunHit;
	bool sunHitProcessed;
	float diamond_distance;
	bool diamond;
	float distance;
	float real_distance;
};

class Player
{
public:
	SpriterState spriteState;
	float angle;
	float max_speed;
	float speed;
	Lemming *selected_lemming;


	Player()
	{
		max_speed = 0.6;
		speed = 0.0;
		angle = 0.0;
	}

	void spawn()
	{
		spriteState.startAnim(PLAYER_SPRITE, 0);
	}

	void tick(float dt)
	{
		// input

		float accel = 2;
		float currentAccel = 0;

		if (keyboard.isDown(SDLK_RIGHT) || gamepad[0].getAnalog(0, ANALOG_X) > +.5f)
			currentAccel += accel;
		if (keyboard.isDown(SDLK_LEFT) || gamepad[0].getAnalog(0, ANALOG_X) < -.5f)
			currentAccel -= accel;
		if (keyboard.wentUp(SDLK_SPACE) || gamepad[0].wentUp(GAMEPAD_A))
		{
			if (selected_lemming)
			{
				selected_lemming->distance = 0.0f;
				selected_lemming->speed = speed * 2.f;

				playSound("throw");
			}
			selected_lemming = 0;
		}

		speed += dt * currentAccel;

		if (speed < -max_speed)
			speed = -max_speed;
		if (speed > max_speed)
			speed = max_speed;

		if (currentAccel == 0)
			speed = speed * powf(0.01f, dt);

		angle += speed * dt;
		if (selected_lemming)
		{
			selected_lemming->angle = angle;
			selected_lemming->distance = 50.0f;
		}
		spriteState.updateAnim(PLAYER_SPRITE, dt);

		// lemmings

	}

	void draw()
	{
		spriteState.x = WORLD_X + cosf(angle) * WORLD_RADIUS;
		spriteState.y = WORLD_Y + sinf(angle) * WORLD_RADIUS;
		spriteState.scale = 0.6f;
		spriteState.angle = angle * Calc::rad2deg + 90;
		setColor(colorWhite);
		PLAYER_SPRITE.draw(spriteState);
	}

};

Lemming	*check_collision_player(Player player, Lemming lemmings[], int nbr_lem)
{
	Lemming	* result = 0;
	int nearest = INT_MAX;

	int dx;
	int dy;
	for (int i = 0; i < nbr_lem; ++i)
	{
		if (!lemmings[i].isActive)
			continue;

		dx = player.spriteState.x - lemmings[i].spriteState.x;
		dy = player.spriteState.y - lemmings[i].spriteState.y;
		int d = sqrtf(dx * dx + dy * dy);
		if (d < 100 && d < nearest)
		{
			result = (&lemmings[i]);
			nearest = d;
		}
	}
	return (result);
}

int check_collision_sun(Sun sun, Lemming lemmings[], int nbr_lem)
{
	const float angle = 10.f * Calc::deg2rad;
	const float cosAngle = cosf(angle);

	for (int i = 0; i < nbr_lem; ++i)
	{
		lemmings[i].sunHit = false;

		if (!lemmings[i].isActive)
			continue;
		if (!lemmings[i].pray)
			continue;

		float dx1 = sun.spriteState.x;
		float dy1 = sun.spriteState.y;
		{
			float d1 = sqrtf(dx1 * dx1 + dy1 * dy1);
			dx1 /= d1;
			dy1 /= d1;
		}

		float dx2 = lemmings[i].spriteState.x;
		float dy2 = lemmings[i].spriteState.y;
		{
			float d2 = sqrtf(dx2 * dx2 + dy2 * dy2);
			dx2 /= d2;
			dy2 /= d2;
		}

		float dot = dx1 * dx2 + dy1 * dy2;

		if (dot >= 0.f && dot >= cosAngle)
			lemmings[i].sunHit = true;
	}
	return (0);
}

Player player;
Lemming lemmings[MAX_LEMMINGS];
Sun sun;
Heart_Faces heart_faces;

static void doTitleScreen()
{
	Sprite background("Art Assets/Wallpaper.jpg");
	background.drawEx(0, 0);

	Spriter spriter("Art Assets/Sun/Sun_Animations.scml");
	SpriterState spriterState;
	spriterState.startAnim(spriter, 0);
	spriterState.x = SX/2;
	spriterState.y = SY/2;
	spriterState.scale = .5f;
	
	bool stop = false;
	float stopTime = 0.f;
	bool wasInside = false;

	while (!stop)
	{
		framework.process();

		const float dt = framework.timeStep;

		int dx = mouse.x - SX/2;
		int dy = mouse.y - SY/2;
		int d = sqrtf(dx * dx + dy * dy);
		bool inside = d <= 400;

		if (inside && !wasInside)
			spriterState.startAnim(spriter, rand() % spriter.getAnimCount());
		if (!inside)
			spriterState.stopAnim(spriter);

		wasInside = inside;

		if (stopTime == 0.f && mouse.wentDown(BUTTON_LEFT) && inside)
		{
			stopTime = 1.f;

			spriterState.animSpeed = 1.5f;
		}

		if (stopTime > 0.f)
		{
			stopTime -= dt;

			if (stopTime <= 0.f)
				stop = true;
		}

		spriterState.updateAnim(spriter, dt);

		framework.beginDraw(0, 0, 0, 0);
		{
			setColorMode(COLOR_ADD);
			{
				setColor(inside ? Color(63, 31, 15) : colorBlack);
				spriter.draw(spriterState);
			}
			setColorMode(COLOR_MUL);
		}
		framework.endDraw();
	}
}

int main(int argc, char * argv[])
{
	if (1 == 2)
	{
		framework.fullscreen = false;
		framework.minification = 2;
	}
	else
	{
		framework.fullscreen = true;
	}

	srand(GetTickCount());

	if (framework.init(0, 0, SX, SY))
	{
	#if !defined(DEBUG)
		framework.fillCachesWithPath("Art Assets", true);
		framework.fillCachesWithPath("Sound Assets", true);
	#endif

	#ifndef DEBUG
		doTitleScreen();
	#endif

		OptionMenu optionsMenu;

		AudioOutput_OpenAL ao;
		MyAudioStream mas;
		mas.Open("Sound Assets/Music/Loops/Music_Layer_%02d_Loop.ogg", "Sound Assets/Music/Loops/Choir_Layer_%02d_Loop.ogg", "Sound Assets/AMB/AMB_STATE_%02d_LOOP.ogg");
		ao.Initialize(2, 48000, 1 << 14);
		ao.Play();
		AudioStream_Capture asc;
		asc.mSource = &mas;

		fftInit();

		struct AudioSet
		{
			float volume[12];
		} audioSets[4] =
		{
			{ 1, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0 },
			{ 1, 1, 0, 0,  1, 1, 0, 0,  0, 1, 0, 0 },
			{ 1, 1, 1, 0,  1, 1, 1, 0,  0, 0, 1, 0 },
			{ 0, 0, 1, 1,  1, 1, 1, 1,  0, 0, 0, 1 }
		};

		initSound();

		SpriterState heartState;

		heartState.x = WORLD_X;
		heartState.y = WORLD_Y;
		heartState.animSpeed = 0.1f;
		heartState.startAnim(HEART_SPRITE, 0);

		for (int i = 0; i < MAX_LEMMINGS / 10; ++i)
		{
			lemmings[i].isActive = true;
			lemmings[i].angle = random(0.f, 1.f) * 2.f * M_PI;
			lemmings[i].spawn();
		}
		sun.spawn();
		sun.angle = random(0.f, 1.f) * 2.f * M_PI;
		player.spawn();
		player.angle = random(0.f, 1.f) * 2.f * M_PI;

		bool stop = false;

		int frameStart = 2;

		while (!stop)
		{
			const float dt = 1.f / 60.f;

			// process

			framework.process();

			asc.mTime = framework.time;
			ao.Update(&asc);
			fftProcess(framework.time);

			static bool doOptions = false;

			if (keyboard.wentDown(SDLK_F5))
				doOptions = !doOptions;

			if (doOptions)
			{
				OptionMenu * menu = &optionsMenu;

				menu->Update();

				if (keyboard.isDown(SDLK_UP) || gamepad[0].isDown(DPAD_UP))
					menu->HandleAction(OptionMenu::Action_NavigateUp, dt);
				if (keyboard.isDown(SDLK_DOWN) || gamepad[0].isDown(DPAD_DOWN))
					menu->HandleAction(OptionMenu::Action_NavigateDown, dt);
				if (keyboard.wentDown(SDLK_RETURN) || gamepad[0].wentDown(GAMEPAD_A))
					menu->HandleAction(OptionMenu::Action_NavigateSelect);
				if (keyboard.wentDown(SDLK_BACKSPACE) || gamepad[0].wentDown(GAMEPAD_B))
				{
					if (menu->HasNavParent())
						menu->HandleAction(OptionMenu::Action_NavigateBack);
					else
						doOptions = false;
				}
				if (keyboard.isDown(SDLK_LEFT) || gamepad[0].isDown(DPAD_LEFT))
					menu->HandleAction(OptionMenu::Action_ValueDecrement, dt);
				if (keyboard.isDown(SDLK_RIGHT) || gamepad[0].isDown(DPAD_RIGHT))
					menu->HandleAction(OptionMenu::Action_ValueIncrement, dt);
			}

			// input

			if (keyboard.wentDown(SDLK_ESCAPE))
			{
				stop = true;
			}

		#ifdef DEBUG
			if (keyboard.wentDown(SDLK_q))
				activeAudioSet = 0;
			if (keyboard.wentDown(SDLK_w))
				activeAudioSet = 1;
			if (keyboard.wentDown(SDLK_e))
				activeAudioSet = 2;
			if (keyboard.wentDown(SDLK_r))
				activeAudioSet = 3;
		#endif

			for (int c = 0; c < 12; ++c)
				mas.targetVolume[c] = audioSets[activeAudioSet].volume[c] / 8.f;

		#ifdef DEBUG
			if (keyboard.wentDown(SDLK_s))
				playSound("pickup");
		#endif

			// logic

			heartState.updateAnim(HEART_SPRITE, dt);
			
			if (sun.emotion == sun.kEmotion_Happy)
				heart_faces.update_animation(3);
			else if (sun.emotion == sun.kEmotion_Neutral)
				heart_faces.update_animation(1);
			else if (sun.emotion == sun.kEmotion_Sad)
				heart_faces.update_animation(2);
			heart_faces.tick(dt);
			player.tick(dt);
			sun.tick(dt);

			for (int i = 0; i < MAX_LEMMINGS; ++i)
				if (lemmings[i].isActive)
					lemmings[i].tick(dt);
			Lemming *le = check_collision_player(player, lemmings, MAX_LEMMINGS);
			if (le)
			{
				if (keyboard.wentDown(SDLK_SPACE) || gamepad[0].wentDown(GAMEPAD_A))
				{
					player.selected_lemming = le;
					playSound("pickup");
				}
				if (keyboard.wentDown(SDLK_UP) || gamepad[0].wentDown(GAMEPAD_B))
				{
					le->doPray();
					if (le->pray)
						playSound("pray");
					else
						playSound("stop_prayer_hit");
				}
			}

			check_collision_sun(sun, lemmings, MAX_LEMMINGS);

			for (int i = 0; i < MAX_LEMMINGS; ++i)
			{
				if (!lemmings[i].isActive)
					continue;
				if (!lemmings[i].sunHitProcessed && lemmings[i].sunHit && lemmings[i].diamond_distance >= 450.0f)
				{
					--sun.prays_needed;
					lemmings[i].sunHitProcessed = true;
					playSound("pray_pickup");
				}
			}

			// draw

			framework.beginDraw(0, 0, 0, 0);
			{
				setColor(colorWhite);

				gxPushMatrix();

				static float targetZoom = 1.f;
				static float zoom = -1.f;

				const int PLAYER_SIZE = 250;
				const float px1 = player.spriteState.x - PLAYER_SIZE;
				const float py1 = player.spriteState.y - PLAYER_SIZE;
				const float px2 = player.spriteState.x + PLAYER_SIZE;
				const float py2 = player.spriteState.y + PLAYER_SIZE;

				const int SUN_SIZE = 400;
				const float sx1 = sun.spriteState.x - SUN_SIZE;
				const float sy1 = sun.spriteState.y - SUN_SIZE;
				const float sx2 = sun.spriteState.x + SUN_SIZE;
				const float sy2 = sun.spriteState.y + SUN_SIZE;

				const float vx1 = Calc::Min(px1, sx1);
				const float vy1 = Calc::Min(py1, sy1);
				const float vx2 = Calc::Max(px2, sx2);
				const float vy2 = Calc::Max(py2, sy2);

				const float midX = (vx1 + vx2) / 2.f;
				const float midY = (vy1 + vy2) / 2.f;

				//targetZoom = 1.0 / (abs((sun.spriteState.x - player.spriteState.x)));
				const float boxSizeX = vx2 - vx1;
				const float boxSizeY = vy2 - vy1;
				const float scaleX = SX / boxSizeX;
				const float scaleY = SY / boxSizeY;

				targetZoom = Calc::Min(scaleX, scaleY);

				frameStart--;

				if (frameStart > 0)
					zoom = targetZoom;
				else
				{
					const float t = powf(.05f, dt);
					zoom = t * zoom + (1.f - t) * targetZoom;
				}

				gxPushMatrix();
				{
					const float zoomMin = .5f;
					const float zoomMax = 1.2f;
					const float zoomAmount = .25f;
					const float backgroundZoom = Calc::Clamp(zoom / zoomMin * zoomAmount + (1.f - zoomAmount), 1.f, 2.f);

					gxTranslatef(SX/2, SY/2, 0.f);
					gxScalef(backgroundZoom, backgroundZoom, 1.f);
					gxTranslatef(-SX/2, -SY/2, 0.f);
					Sprite background("Art Assets/Wallpaper.jpg");
					background.filter = FILTER_LINEAR;
					background.draw();
				}
				gxPopMatrix();

				gxTranslatef(SX/2, SY/2, 0);
				gxScalef(zoom, zoom, 1);
				gxTranslatef(-midX, -midY, 0.f);
				static Color earth_color = colorWhite;
				Color earth_target_color = colorWhite;
				if (sun.emotion == sun.kEmotion_Happy)
					earth_target_color = Color::fromHSL(1.000f, 0.920f, 0.480f);
				else if (sun.emotion == sun.kEmotion_Neutral)
					earth_target_color = Color::fromHSL(0.320f, 1.000f, 0.710f);
				else if (sun.emotion == sun.kEmotion_Sad)
					earth_target_color = Color::fromHSL(0.520f, 1.000f, 0.620f);
				Sprite earth("Art Assets/planete.png");
				earth_color = earth_color.interp(earth_target_color, 0.004f);
				setColor(earth_color);
				earth.drawEx(-earth.getWidth() / 2, -earth.getHeight() / 2, 0.0, 1.0, 1.0, true, FILTER_POINT);
				setColor(earth_color);
				HEART_SPRITE.draw(heartState);
				heart_faces.draw(earth_color);

				/*
				glBegin(GL_LINE_LOOP);
				{
					int n = Calc::Min(50, kFFTComplexSize);
					float a = 0.f;
					float s = 1.f / n * 2.f * M_PI;

					gxColor4f(1.f, 0.f, 0.f, 1.f);

					for (int i = 0; i < n; ++i, a += s)
					{
						float v = fftPowerValue(i) * 5.f + 200.f;
						float x = cosf(a) * v;
						float y = sinf(a) * v;
						gxVertex2f(x, y);
					}
				}
				glEnd();
				*/

				for (int i = 0; i < MAX_LEMMINGS; ++i)
					if (lemmings[i].isActive)
						lemmings[i].draw();

				player.draw();
				sun.draw();

				gxPopMatrix();
			}

			if (frameStart > 0)
			{
				setColor(colorBlack);
				drawRect(0, 0, SX, SY);
			}

			if (DEBUG_DRAW)
			{
				setFont("calibri.ttf");
				setColor(colorWhite);

				drawText(10, 10, 24, +1, +1, "Debug Draw!");

				for (int i = 0; i < 12; ++i)
				{
					setColorf(1.f, 1.f, 1.f, .25f + 4.f * mas.volume[i]);
					drawText(10, 40 + 30 * i, 24, +1, +1, mas.source[i].FileName_get());
				}
			}

			if (doOptions)
				optionsMenu.Draw(100, 100, 400, 600);

			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
