#pragma once

#include <list> // annoucements
#include <string.h> // memset
#include "arena.h"
#include "framework.h"
#include "gamedefs.h"
#include "gametypes.h"
#include "levelevents.h"
#include "physobj.h"
#include "Random.h"
#include "Vec2.h"

class BulletPool;
class Color;
class NetSpriteManager;
struct ParticleSpawnInfo;
class PlayerInstanceData;

#pragma pack(push)
#pragma pack(1)

struct Player
{
	PlayerInstanceData * m_instanceData;

	Player()
	{
		memset(this, 0, sizeof(Player));
	}

	Player(uint8_t index, uint16_t owningChannelId)
	{
		memset(this, 0, sizeof(Player));

		m_index = index;
		m_owningChannelId = owningChannelId;

		m_characterIndex = -1;
		m_facing.Set(+1.f, +1.f);

		m_spriterState = SpriterState();

		m_animAllowGravity = true;
		m_animAllowSteering = true;
		m_enableInAirAnim = true;

		m_lastSpawnIndex = -1;
	}

	void tick(float dt);
	void draw() const;
	void drawAt(bool flipX, bool flipY, int x, int y) const;
	void drawLight() const;
	void debugDraw() const;

	void playSecondaryEffects(PlayerEvent e);

	void testCollision(const CollisionShape & shape, void * arg, CollisionCB cb);

	bool getPlayerCollision(CollisionInfo & collision) const;
	void getDamageHitbox(CollisionInfo & collision) const;
	void getAttackCollision(CollisionInfo & collision) const;
	float getAttackDamage(Player * other) const;

	bool isAnimOverrideAllowed(PlayerAnim anim) const;
	float mirrorX(float x) const;
	float mirrorY(float y) const;

	bool hasValidCharacterIndex() const;
	void setDisplayName(const std::string & name);

	void setAnim(PlayerAnim anim, bool play, bool restart);
	void clearAnimOverrides();
	void setAttackDirection(int dx, int dy);
	void applyAnim();

	uint32_t getIntersectingBlocksMaskInternal(int x, int y, bool doWrap) const;
	uint32_t getIntersectingBlocksMask(int x, int y) const;

	void handleNewGame();
	void handleNewRound();

	void respawn();
	void cancelAttack();
	void handleImpact(Vec2Arg velocity);
	bool shieldAbsorb(float amount);
	bool handleDamage(float amount, Vec2Arg velocity, Player * attacker);
	bool handleIce(Vec2Arg velocity, Player * attacker);
	bool handleBubble(Vec2Arg velocity, Player * attacker);
	void awardScore(int score);
	void dropCoins(int numCoins);

	void pushWeapon(PlayerWeapon weapon, int ammo);
	PlayerWeapon popWeapon();

	void handleJumpCollision();

	void beginRocketPunch();
	void endRocketPunch(bool stunned);

	// allocation
	bool m_isUsed;

	uint8_t m_index;
	uint16_t m_owningChannelId;

	// character select
	FixedString<MAX_PLAYER_DISPLAY_NAME> m_displayName;
	bool m_isReadyUpped;

	// alive state
	bool m_isAlive;
	uint8_t m_characterIndex;
	float m_controlDisableTime;

	// input
	struct InputState
	{
		InputState()
			: m_actions(0)
		{
		}

		PlayerInput m_prevState;
		PlayerInput m_currState;
		uint32_t m_actions;

		bool wasDown(int input) { return (m_prevState.buttons & input) != 0; }
		bool isDown(int input) { return (m_currState.buttons & input) != 0; }
		bool wentDown(int input) { return !wasDown(input) && isDown(input); }
		bool wentUp(int input) { return wasDown(input) && !isDown(input); }
		void next() { m_prevState = m_currState; m_actions = 0; }
		Vec2 getAnalogDirection() const { return Vec2(m_currState.analogX / 127.f, m_currState.analogY / 127.f); }
	} m_input;

	//

	int8_t m_score;
	int16_t m_totalScore;

	//

	Vec2 m_pos;
	Vec2 m_vel;

	Vec2 m_facing;
	float m_facingAnim;

	//

	PlayerAnim m_anim;
	bool m_animPlay;

	SpriterState m_spriterState;

	//

	int8_t m_attackDirection[2];

	//

	PlayerWeapon m_weaponStack[MAX_WEAPON_STACK_SIZE];
	uint8_t m_weaponStackSize;

	//

	struct
	{
		float meleeAnimTimer;
		int meleeCounter;
		float attackDownHeight;
		bool attackDownActive;
	} m_special;

	int m_traits; // PlayerTrait

	//

	CollisionInfo m_collision;

	uint32_t m_blockMask;

	int8_t m_dirBlockMaskDir[2];
	uint32_t m_dirBlockMask[2];
	uint32_t m_oldBlockMask;

	//

	struct AttackInfo
	{
		AttackInfo()
			: attacking(false)
			, hitDestructible(false)
			, hasCollision(false)
			, collision()
			, attackVel()
			, cooldown(0.f)
		{
			memset(this, 0, sizeof(AttackInfo));
		}

		bool attacking;
		bool hitDestructible;
		bool hasCollision;
		CollisionInfo collision;
		Vec2 attackVel;
		float cooldown; // this timer needs to hit zero before the player can attack again. it's decremented AFTER the attack animation has finished
		bool allowCancelTimeDilationAttack;

		struct RocketPunch
		{
			RocketPunch()
			{
				memset(this, 0, sizeof(RocketPunch));
			}

			enum State
			{
				kState_Charge,
				kState_Attack,
				kState_Stunned
			};

			bool isActive;
			State state;
			float chargeTime;
			float maxDistance;
			float distance;
			Vec2 speed;
			float stunTime;
		} m_rocketPunch;

		struct Zweihander
		{
			enum State
			{
				kState_Idle,
				kState_Charge,
				kState_Attack,
				kState_Stunned
			};

			Zweihander();

			bool isActive() const;
			void begin(Player & player);
			void end(Player & player);

			void handleAttackAnimComplete(Player & player);
			void tick(Player & player, float dt);

			State state;
			float timer;
		} m_zweihander;
	} m_attack;

	struct TimeDilationAttack
	{
		bool isActive() const
		{
			return timeRemaining != 0.f;
		}

		void tick(float dt)
		{
			timeRemaining -= dt;
			if (timeRemaining < 0.f)
				timeRemaining = 0.f;
		}

		float timeRemaining;
	} m_timeDilationAttack;

	struct TeleportInfo
	{
		TeleportInfo()
			: cooldown(false)
			, x(0)
			, y(0)
		{
			memset(this, 0, sizeof(TeleportInfo));
		}

		bool cooldown; // when set, we're waiting for the player to exit (x, y), which is the destination of the previous teleport
		int16_t x;
		int16_t y;
	} m_teleport;

	struct JumpInfo
	{
		JumpInfo()
		{
			memset(this, 0, sizeof(JumpInfo));
		}

		float jumpVelocityLeft;

		bool cancelStarted;
		bool cancelled;
		int16_t cancelX;
		int8_t cancelFacing;
	} m_jump;

	struct ShieldInfo
	{
		ShieldInfo()
		{
			memset(this, 0, sizeof(ShieldInfo));
		}

		int shield;
	} m_shield;

	struct IceInfo
	{
		float timer;
	} m_ice;

	struct BubbleInfo
	{
		float timer;
	} m_bubble;

	float m_respawnTimer; // when this timer counts to zero, the player is automatically respawn
	bool m_canRespawn; // set when the player is allowed to respawn, which is after the death animation is done
	bool m_canTaunt; // set when the player is allowed to taunt, which is after the death animation is done. it's reset after a taunt
	bool m_isRespawn; // set after the first respawn. the first spawn is special, as the player doesn't need to press X and isn't allowed to use taunt
	int m_lastSpawnIndex;
	int m_spawnInvincibilityTicks;

	bool m_isGrounded; // set when the player is walking on ground
	bool m_isAttachedToSticky;
	bool m_isAnimDriven; // an animation is active that drives the player using animation actions/triggers
	bool m_enableInAirAnim;

	bool m_isAirDashCharged; // reset when air dash is used. set when the player hits the ground
	bool m_isInPassthrough;
	int8_t m_isHuggingWall;
	bool m_isWallSliding;

	bool m_isUsingJetpack;

	bool m_animVelIsAbsolute; // should the animation velocity be added to or replace the regular player velocity?
	bool m_animAllowGravity;
	bool m_animAllowSteering; // allow the player to control the character?
	Vec2 m_animVel;

	bool m_enterPassthrough; // if set, the player will move through passthrough blocks, without having to press DOWN. this mode is set when using the sword-down attack, and reset when the player hits the ground

	struct TokenHunt
	{
		TokenHunt()
		{
			memset(this, 0, sizeof(TokenHunt));
		}

		bool m_hasToken;
	} m_tokenHunt;
};

struct Pickup : PhysicsActor
{
	bool isAlive;

	PickupType type;
	uint8_t blockX;
	uint8_t blockY;

	Pickup()
	{
		memset(this, 0, sizeof(Pickup));
	}

	void setup(PickupType type, int blockX, int blockY);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;
};

struct Token : PhysicsActor
{
	bool m_isDropped;
	float m_dropTimer;

	Token()
	{
		memset(this, 0, sizeof(Token));
	}

	void setup(int blockX, int blockY);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;
};

struct Coin : PhysicsActor
{
	bool m_isDropped;
	float m_dropTimer;

	Coin()
	{
		memset(this, 0, sizeof(Coin));
	}

	void setup(int blockX, int blockY);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;
};

struct Mover
{
	bool m_isActive;

	FixedString<64> m_sprite;
	int m_sx;
	int m_sy;

	int m_x1;
	int m_y1;
	int m_x2;
	int m_y2;
	int m_speed;
	float m_moveMultiplier;
	float m_moveAmount;

	Mover()
	{
		memset(this, 0, sizeof(Mover));
	}

	void setSprite(const char * filename);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;

	Vec2 getPosition() const;
	Vec2 getSpeed() const;

	void getCollisionInfo(CollisionInfo & collisionInfo) const;

	bool intersects(CollisionInfo & collisionInfo) const;
};

struct PipeBomb : PhysicsActor
{
	PipeBomb()
	{
		memset(this, 0, sizeof(PipeBomb));
	}

	void setup(Vec2Arg pos, Vec2Arg vel);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;
};

struct Barrel : PhysicsActor
{
	bool m_isActive;
	Vec2 m_pos;
	int m_tick; // barrel velocity/position depends on tick number

	Barrel()
	{
		memset(this, 0, sizeof(Barrel));
	}

	void setup(Vec2Arg pos);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;
};

struct Torch
{
	bool m_isAlive;
	Vec2 m_pos;
	float m_color[4];

	Torch()
	{
		memset(this, 0, sizeof(Torch));
	}

	void setup(float x, float y, const Color & color);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;
};

struct ScreenShake
{
	bool isActive;
	Vec2 pos;
	Vec2 vel;
	float stiffness;
	float life;

	ScreenShake()
	{
		memset(this, 0, sizeof(ScreenShake));
	}

	void tick(float dt);
};

struct FloorEffect
{
	struct ActiveTile
	{
		int16_t x;
		int16_t y;
		int8_t size;
		int8_t damageSize;
		int8_t dx;
		float time;
		float damageTime;
		int8_t playerId;

		void getCollisionInfo(CollisionInfo & collisionInfo) const;
	} m_tiles[MAX_FLOOR_EFFECT_TILES];

	FloorEffect()
	{
		memset(this, 0, sizeof(FloorEffect));
	}

	void tick(GameSim & gameSim, float dt);
	void draw();

	void trySpawnAt(GameSim & gameSim, int playerId, int x, int y, int dx, int size, int damageSize);
};

//

struct GameStateData
{
	GameStateData()
	{
		memset(this, 0, sizeof(GameStateData));
	}

	uint32_t Random();
	float RandomFloat(float min, float max) { float t = (Random() & 4095) / 4095.f; return t * min + (1.f - t) * max; }
	uint32_t GetTick() const;
	float getRoundTime() const;
	void addTimeDilationEffect(float multiplier1, float multiplier2, float duration);
	LevelEvent getRandomLevelEvent();

	uint32_t m_tick;
	uint32_t m_randomSeed;

	uint32_t m_gameStartTicks;

	struct TimeDilationEffect
	{
		float multiplier1;
		float multiplier2;
		int ticks;
		int ticksRemaining;
	} m_timeDilationEffects[MAX_TIMEDILATION_EFFECTS];

	GameState m_gameState;
	GameMode m_gameMode;

	float m_roundTime;
	uint32_t m_nextRoundNumber;
	uint32_t m_roundCompleteTicks;
	uint32_t m_roundCompleteTimeDilationTicks;

	Player m_players[MAX_PLAYERS];

	// pickups

	Pickup m_pickups[MAX_PICKUPS];
	uint64_t m_nextPickupSpawnTick;

	// movers

	Mover m_movers[MAX_MOVERS];

	// pipe bombs

	PipeBomb m_pipebombs[MAX_PIPEBOMBS];

	// barrels

	Barrel m_barrels[MAX_BARRELS];

	// effects

	FloorEffect m_floorEffect;

	Torch m_torches[MAX_TORCHES];

	// level events

	struct LevelEvents
	{
		LevelEvents()
		{
			memset(this, 0, sizeof(LevelEvents));
		}

		LevelEvent_EarthQuake quake;
		LevelEvent_GravityWell gravityWell;
		LevelEvent_DestroyDestructibleBlocks destroyBlocks;
		LevelEvent_TimeDilation timeDilation;
		LevelEvent_SpikeWalls spikeWalls;
		LevelEvent_Wind wind;
		LevelEvent_BarrelDrop barrelDrop;
		LevelEvent_NightDayCycle nightDayCycle;
	} m_levelEvents;

	float m_timeUntilNextLevelEvent;

	// support for game modes

	struct TokenHunt
	{
		Token m_token;
	} m_tokenHunt;

	struct CoinCollector
	{
		CoinCollector()
			: m_nextSpawnTick(0)
		{
		}

		Coin m_coins[MAX_COINS];
		uint64_t m_nextSpawnTick;
	} m_coinCollector;
};

#pragma pack(pop)

class GameSim : public GameStateData
{
public:
	// serialized

	Arena m_arena;

	BulletPool * m_bulletPool;

	BulletPool * m_particlePool;

	ScreenShake m_screenShakes[MAX_SCREEN_SHAKES];

	// non-serialized (RPC)

	PlayerInstanceData * m_playerInstanceDatas[MAX_PLAYERS];

	struct AnnounceInfo
	{
		float timeLeft;
		std::string message;
	};

	std::list<AnnounceInfo> m_annoucements;

	GameSim();
	~GameSim();

#if ENABLE_GAMESTATE_DESYNC_DETECTION
	uint32_t calcCRC() const;
#endif

	void serialize(NetSerializationContext & context);
	void clearPlayerPtrs() const;
	void setPlayerPtrs() const;
	PlayerInstanceData * allocPlayer(uint16_t owningChannelId);
	void freePlayer(PlayerInstanceData * instanceData);

	void setGameState(::GameState gameState);
	void setGameMode(GameMode gameMode);

	void newGame();
	void newRound(const char * mapOverride);
	void endRound();

	void load(const char * filename);
	void resetGameWorld();
	void resetPlayers();
	void resetGameSim();

	void tick();
	void tickMenus();
	void tickPlay();
	void tickRoundComplete();
	void anim(float dt);

	void getCurrentTimeDilation(float & timeDilation, bool & playerAttackTimeDilation) const;

	void playSound(const char * filename, int volume = 100);

	void testCollision(const CollisionShape & shape, void * arg, CollisionCB cb);
	void testCollisionInternal(const CollisionShape & shape, void * arg, CollisionCB cb);

	void trySpawnPickup(PickupType type);
	void spawnPickup(Pickup & pickup, PickupType type, int blockX, int blockY);
	bool grabPickup(int x1, int y1, int x2, int y2, Pickup & pickup);

	void spawnToken();
	bool pickupToken(const CollisionInfo & collisionInfo);

	Coin * allocCoin();
	void spawnCoin();
	bool pickupCoin(const CollisionInfo & collisionInfo);

	uint16_t spawnBullet(int16_t x, int16_t y, uint8_t angle, BulletType type, BulletEffect effect, uint8_t ownerPlayerId);
	void spawnParticles(const ParticleSpawnInfo & spawnInfo);

	void addScreenShake(float dx, float dy, float stiffness, float life);
	Vec2 getScreenShake() const;

	void addFloorEffect(int playerId, int x, int y, int size, int damageSize);

	void addAnnouncement(const char * message, ...);
};

extern GameSim * g_gameSim;
