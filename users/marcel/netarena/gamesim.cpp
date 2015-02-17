#include "bullet.h"
#include "Calc.h"
#include "client.h"
#include "FileStream.h"
#include "framework.h"
#include "gamedefs.h"
#include "gamesim.h"
#include "main.h"
#include "Parse.h"
#include "Path.h"
#include "player.h"
#include "StreamReader.h"
#include "Timer.h"

OPTION_DECLARE(bool, g_noSound, false);
OPTION_DEFINE(bool, g_noSound, "Sound/Disable Sound Effects");
OPTION_ALIAS(g_noSound, "nosound");

//

void splitString(const std::string & str, std::vector<std::string> & result, char c);

//

GameSim * g_gameSim = 0;

//

static const char * s_pickupSprites[kPickupType_COUNT] =
{
	"pickup-ammo.png",
	"pickup-nade.png",
	"pickup-shield.png",
	"pickup-ice.png",
	"pickup-bubble.png"
};

#define TOKEN_SPRITE "token.png"
#define COIN_SPRITE "coin.png"

//

uint32_t GameStateData::Random()
{
	m_randomSeed += 1;
	m_randomSeed *= 16807;
	return m_randomSeed;
}

uint32_t GameStateData::GetTick()
{
	return m_tick;
}

//

void Pickup::setup(PickupType _type, int _blockX, int _blockY)
{
	const char * filename = s_pickupSprites[type];

	Sprite sprite(filename);

	type = _type;
	blockX = _blockX;
	blockY = _blockY;

	//

	*static_cast<PhysicsActor*>(this) = PhysicsActor();

	m_pos.Set(
		blockX * BLOCK_SX + BLOCK_SX / 2.f,
		blockY * BLOCK_SY + BLOCK_SY - sprite.getHeight());
	m_vel.Set(0.f, 0.f);

	m_bbMin.Set(-sprite.getWidth() / 2.f, -sprite.getHeight());
	m_bbMax.Set(+sprite.getWidth() / 2.f, 0.f);

	m_doTeleport = true;
	m_bounciness = .25f;
	m_friction = .1f;
	m_airFriction = .5f;
}

void Pickup::tick(GameSim & gameSim, float dt)
{
	PhysicsActorCBs cbs;
	PhysicsActor::tick(gameSim, dt, cbs);
}

void Pickup::draw()
{
	const char * filename = s_pickupSprites[type];

	Sprite sprite(filename);

	sprite.drawEx(m_pos[0] + m_bbMin[0], m_pos[1] + m_bbMin[1]);

	if (true)
	{
		// fixme!

		setFont("calibri.ttf");
		drawText(m_pos[0], m_pos[1], 24.f, 0.f, 0.f, "type: %d", type);
	}
}

void Pickup::drawLight()
{
	Vec2 pos = m_pos + (m_bbMin + m_bbMax) / 2.f;

	Sprite("player-light.png").drawEx(pos[0], pos[1], 0.f, 1.f);
}

//

void Token::setup(int blockX, int blockY)
{
	Sprite sprite(TOKEN_SPRITE);

	*static_cast<PhysicsActor*>(this) = PhysicsActor();

	m_bbMin.Set(-sprite.getWidth() / 2.f, -sprite.getHeight() / 2.f);
	m_bbMax.Set(+sprite.getWidth() / 2.f, +sprite.getHeight() / 2.f);
	m_pos.Set(
		(blockX + .5f) * BLOCK_SX,
		(blockY + .5f) * BLOCK_SY);
	m_vel.Set(0.f, 0.f);
	m_doTeleport = true;
	m_bounciness = -TOKEN_BOUNCINESS;
	m_noGravity = false;
	m_friction = 0.1f;
	m_airFriction = 0.9f;

	m_isDropped = true;
	m_dropTimer = 0.f;
}

void Token::tick(GameSim & gameSim, float dt)
{
	if (m_isDropped)
	{
		PhysicsActorCBs cbs;
		cbs.onBlockMask = [](PhysicsActorCBs & cbs, PhysicsActor & actor, uint32_t blockMask) 
		{
			if (blockMask & kBlockMask_Spike)
			{
				if (actor.m_vel[1] > 0.f)
				{
					actor.m_vel.Set(g_gameSim->RandomFloat(-TOKEN_FLEE_SPEED, +TOKEN_FLEE_SPEED), -TOKEN_FLEE_SPEED);
					g_gameSim->playSound("token-bounce.ogg");
				}
			}
			return false;
		};
		cbs.onBounce = [](PhysicsActorCBs & cbs, PhysicsActor & actor)
		{
			if (std::abs(actor.m_vel[1]) >= TOKEN_BOUNCE_SOUND_TRESHOLD)
				g_gameSim->playSound("token-bounce.ogg");
		};

		PhysicsActor::tick(gameSim, dt, cbs);

		m_dropTimer -= dt;
	}
}

void Token::draw()
{
	if (m_isDropped)
	{
		Sprite(TOKEN_SPRITE).drawEx(m_pos[0], m_pos[1]);
	}
}

void Token::drawLight()
{
	if (m_isDropped)
	{
		Sprite("player-light.png").drawEx(m_pos[0], m_pos[1], 0.f, 1.5f);
	}
}

//

void Coin::setup(int blockX, int blockY)
{
	Sprite sprite(TOKEN_SPRITE); // fixme : COIN_SPRITE

	*static_cast<PhysicsActor*>(this) = PhysicsActor();

	m_bbMin.Set(-sprite.getWidth() / 2.f, -sprite.getHeight() / 2.f);
	m_bbMax.Set(+sprite.getWidth() / 2.f, +sprite.getHeight() / 2.f);
	m_pos.Set(
		(blockX + .5f) * BLOCK_SX,
		(blockY + .5f) * BLOCK_SY);
	m_vel.Set(0.f, 0.f);
	m_doTeleport = true;
	m_bounciness = -TOKEN_BOUNCINESS;
	m_noGravity = false;
	m_friction = 0.1f;
	m_airFriction = 0.9f;

	m_isDropped = true;
	m_dropTimer = 0.f;
}

void Coin::tick(GameSim & gameSim, float dt)
{
	if (m_isDropped)
	{
		PhysicsActorCBs cbs;
		cbs.onBlockMask = [](PhysicsActorCBs & cbs, PhysicsActor & actor, uint32_t blockMask) 
		{
			if (blockMask & kBlockMask_Spike)
			{
				if (actor.m_vel[1] > 0.f)
				{
					actor.m_vel.Set(g_gameSim->RandomFloat(-COIN_FLEE_SPEED, +COIN_FLEE_SPEED), -COIN_FLEE_SPEED);
					g_gameSim->playSound("token-bounce.ogg");
				}
			}
			return false;
		};
		cbs.onBounce = [](PhysicsActorCBs & cbs, PhysicsActor & actor)
		{
			if (std::abs(actor.m_vel[1]) >= COIN_BOUNCE_SOUND_TRESHOLD)
				g_gameSim->playSound("token-bounce.ogg");
		};

		PhysicsActor::tick(gameSim, dt, cbs);

		m_dropTimer -= dt;
	}
}

void Coin::draw()
{
	if (m_isDropped)
	{
		Sprite(COIN_SPRITE).drawEx(m_pos[0], m_pos[1]);
	}
}

void Coin::drawLight()
{
	if (m_isDropped)
	{
		Sprite("player-light.png").drawEx(m_pos[0], m_pos[1], 0.f, 1.5f);
	}
}

//

void Torch::setup(float x, float y, const Color & color)
{
	m_isAlive = true;
	m_pos.Set(x, y);
	m_color[0] = color.r;
	m_color[1] = color.g;
	m_color[2] = color.b;
	m_color[3] = color.a;
}

void Torch::tick(GameSim & gameSim, float dt)
{
}

void Torch::draw()
{
	Sprite("torch.png").drawEx(m_pos[0], m_pos[1], 0.f, 1.f);
}

void Torch::drawLight()
{
	float a = 0.f;
	a += std::sin(g_TimerRT.Time_get() * Calc::m2PI * TORCH_FLICKER_FREQ_A);
	a += std::sin(g_TimerRT.Time_get() * Calc::m2PI * TORCH_FLICKER_FREQ_B);
	a += std::sin(g_TimerRT.Time_get() * Calc::m2PI * TORCH_FLICKER_FREQ_C);
	a = (a + 3.f) / 6.f;
	a = Calc::Lerp(1.f, a, TORCH_FLICKER_STRENGTH);

	Color color;
	color.r = m_color[0];
	color.g = m_color[1];
	color.b = m_color[2];
	color.a = a;
	setColor(color);

	Sprite("player-light.png").drawEx(m_pos[0], m_pos[1], 0.f, 1.5f);

	setColor(colorWhite);
}

//

void ScreenShake::tick(float dt)
{
	Vec2 force = pos * (-stiffness);
	vel += force * dt;
	pos += vel * dt;

	life -= dt;

	if (life <= 0.f)
		isActive = false;
}

//

FloorEffect::FloorEffect()
{
	memset(this, 0, sizeof(*this));
}

void FloorEffect::tick(GameSim & gameSim, float dt)
{
	for (int i = 0; i < MAX_FLOOR_EFFECT_TILES; ++i)
	{
		if (m_tiles[i].time)
		{
			if (m_tiles[i].damageSize > 0)
			{
				CollisionInfo collisionInfo;
				collisionInfo.x1 = m_tiles[i].x - 4;
				collisionInfo.x2 = m_tiles[i].x + 4;
				collisionInfo.y1 = m_tiles[i].y - 16;
				collisionInfo.y2 = m_tiles[i].y;

				for (int p = 0; p < MAX_PLAYERS; ++p)
				{
					if (p != m_tiles[i].playerId)
					{
						Player & player = gameSim.m_players[p];

						if (!player.m_isUsed)
							continue;
						if (!player.m_isAlive)
							continue;

						CollisionInfo playerCollision;
						player.getPlayerCollision(playerCollision);

						if (collisionInfo.intersects(playerCollision))
						{
							player.handleDamage(1.f, Vec2(m_tiles[i].dx, -1.f), &gameSim.m_players[m_tiles[i].playerId]);
						}
					}
				}
			}

			m_tiles[i].time--;

			if (m_tiles[i].time == 0 && m_tiles[i].size != 0)
			{
				const int playerId = m_tiles[i].playerId;
				const int x = m_tiles[i].x + m_tiles[i].dx;
				const int y = m_tiles[i].y;
				const int dx = m_tiles[i].dx;
				const int size = m_tiles[i].size - 1;
				const int damageSize = m_tiles[i].damageSize > 0 ? m_tiles[i].damageSize - 1 : 0;
				memset(&m_tiles[i], 0, sizeof(m_tiles[i]));

				trySpawnAt(gameSim, playerId, x, y, dx, size, damageSize);
			}
		}
	}
}

void FloorEffect::trySpawnAt(GameSim & gameSim, int playerId, int x, int y, int dx, int size, int damageSize)
{
	x = (x + ARENA_SX_PIXELS) % ARENA_SX_PIXELS;

	const Arena & arena = gameSim.m_arena;

	// try to move up

	bool foundEmpty = false;

	for (int dy = 0; dy < BLOCK_SY * 3 / 2; ++dy)
	{
		const uint32_t blockMask = arena.getIntersectingBlocksMask(x, y - dy);

		if (!(blockMask & kBlockMask_Solid))
		{
			y = y - dy;
			foundEmpty = true;
			break;
		}
	}

	if (!foundEmpty)
		return;

	// try to move down

	bool hitGround = false;

	for (int dy = 0; dy < BLOCK_SY * 3 / 2; ++dy)
	{
		const uint32_t blockMask = arena.getIntersectingBlocksMask(x, y + dy + 1);

		if (blockMask & kBlockMask_Solid)
		{
			y = y + dy;
			hitGround = true;
			break;
		}
	}

	if (hitGround)
	{
		for (int i = 0; i < MAX_FLOOR_EFFECT_TILES; ++i)
		{
			if (m_tiles[i].time == 0)
			{
				m_tiles[i].playerId = playerId;
				m_tiles[i].x = x;
				m_tiles[i].y = y;
				m_tiles[i].dx = dx;
				m_tiles[i].size = size;
				m_tiles[i].damageSize = damageSize;
				m_tiles[i].time = TICKS_PER_SECOND / 12; // fixme : gamedef

				ParticleSpawnInfo spawnInfo(
					x,
					y,
					kBulletType_ParticleA, 10,
					50.f, 100.f, 50.f);
				spawnInfo.color = damageSize > 0 ? 0xff8000a0 : 0x0000ffa0;
				g_gameSim->spawnParticles(spawnInfo);

				gameSim.playSound("grenade-frag.ogg");

				break;
			}
		}
	}
}

//

GameSim::GameSim()
	: GameStateData()
	, m_bulletPool(0)
{
	m_arena.init();

	for (int i = 0; i < MAX_PLAYERS; ++i)
		m_playerNetObjects[i] = 0;

	m_bulletPool = new BulletPool(false);

	m_particlePool = new BulletPool(true);
}

GameSim::~GameSim()
{
	delete m_particlePool;
	m_particlePool = 0;

	delete m_bulletPool;
	m_bulletPool = 0;
}

uint32_t GameSim::calcCRC() const
{
	clearPlayerPtrs();

	uint32_t result = 0;

	const uint8_t * bytes = (const uint8_t*)static_cast<const GameStateData*>(this);
	const uint32_t numBytes = sizeof(GameStateData);

	for (uint32_t i = 0; i < numBytes; ++i)
		result = result * 13 + bytes[i];

	// todo : add screen shakes, particle pool, bullet pool, arena

	setPlayerPtrs();

	return result;
}

void GameSim::serialize(NetSerializationContext & context)
{
	uint32_t crc;

	if (context.IsSend())
		crc = calcCRC();

	context.Serialize(crc);

	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_playerNetObjects[i])
			m_playerNetObjects[i]->m_player->m_netObject = 0;

	GameStateData * data = static_cast<GameStateData*>(this);

	context.SerializeBytes(data, sizeof(GameStateData));

	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_playerNetObjects[i])
			m_playerNetObjects[i]->m_player->m_netObject = m_playerNetObjects[i];

	// todo : serialize player animation state

	m_arena.serialize(context);

	m_bulletPool->serialize(context);

	if (context.IsRecv())
		Assert(crc == calcCRC());

	// todo : serialize player animation state, since it affects game play
}

void GameSim::clearPlayerPtrs() const
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_playerNetObjects[i])
			m_playerNetObjects[i]->m_player->m_netObject = 0;
}

void GameSim::setPlayerPtrs() const
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_playerNetObjects[i])
			m_playerNetObjects[i]->m_player->m_netObject = m_playerNetObjects[i];
}

void GameSim::setGameState(::GameState gameState)
{
	m_gameState = gameState;

	switch (gameState)
	{
	case kGameState_Connecting:
		break;

	case kGameState_Menus:
		resetGameSim();
		break;

	case kGameState_NewGame:
		// reset players

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			if (m_playerNetObjects[i])
			{
				Player * player = m_playerNetObjects[i]->m_player;

				player->handleNewGame();
			}
		}
		break;

	case kGameState_Play:
		{
			playSound("round-begin.ogg");

			// respawn players

			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				if (m_playerNetObjects[i])
				{
					Player * player = m_playerNetObjects[i]->m_player;

					player->handleNewRound();

					player->respawn();
				}
			}

			if (m_gameMode == kGameMode_TokenHunt)
			{
				spawnToken();
			}

			if (m_gameMode == kGameMode_CoinCollector)
			{
				for (int i = 0; i < 4; ++i)
					spawnCoin();
			}
		}
		break;
	}
}

void GameSim::setGameMode(GameMode gameMode)
{
	m_gameMode = gameMode;
}

static Color parseColor(const char * str)
{
	const uint32_t hex = std::stoul(str, 0, 16);
	const float r = ((hex >> 24) & 0xff) / 255.f;
	const float g = ((hex >> 16) & 0xff) / 255.f;
	const float b = ((hex >>  8) & 0xff) / 255.f;
	const float a = ((hex >>  0) & 0xff) / 255.f;
	return Color(r, g, b, a);
}

void GameSim::load(const char * filename)
{
	resetGameWorld();

	// load arena

	m_arena.load(filename);

	// load objects

	std::string baseName = Path::GetBaseName(filename);

	std::string objectsFilename = baseName + "-objects.txt";

	try
	{
		FileStream stream;
		stream.Open(objectsFilename.c_str(), (OpenMode)(OpenMode_Read | OpenMode_Text));
		StreamReader reader(&stream, false);
		std::vector<std::string> lines = reader.ReadAllLines();

		enum ObjectType
		{
			kObjectType_Undefined,
			kObjectType_Torch,
			kObjectType_Mover2
		};

		ObjectType objectType = kObjectType_Undefined;
		Torch * torch = 0;

		for (size_t i = 0; i < lines.size(); ++i)
		{
			if (lines[i].empty() || lines[i][0] == '#')
				continue;

			std::vector<std::string> fields;
			splitString(lines[i], fields, ':');

			if (fields.size() != 2)
			{
				LOG_WRN("syntax error: %s", lines[i].c_str());
			}
			else
			{
				if (fields[0] == "object")
				{
					objectType = kObjectType_Undefined;
					torch = 0;

					if (fields[1] == "torch")
					{
						objectType = kObjectType_Torch;

						for (int i = 0; i < MAX_TORCHES; ++i)
						{
							if (!m_torches[i].m_isAlive)
							{
								torch = &m_torches[i];
								break;
							}
						}

						if (torch == 0)
							LOG_ERR("too many torches!");
						else
							torch->m_isAlive = true;
					}
					if (fields[1] == "mover2")
						objectType = kObjectType_Mover2;
				}
				else
				{
					switch (objectType)
					{
					case kObjectType_Undefined:
						LOG_ERR("properties begin before object type is set!");
						break;

					case kObjectType_Torch:
						if (torch)
						{
							if (fields[0] == "x")
								torch->m_pos[0] = Parse::Int32(fields[1]);
							if (fields[0] == "y")
								torch->m_pos[1] = Parse::Int32(fields[1]);
							if (fields[0] == "color")
							{
								if (fields[1].length() != 8)
									LOG_ERR("invalid color format: %s", fields[1].c_str());
								else
								{
									const Color color = parseColor(fields[1].c_str());
									torch->m_color[0] = color.r;
									torch->m_color[1] = color.g;
									torch->m_color[2] = color.b;
									torch->m_color[3] = color.a;
								}
							}
						}
						break;
					case kObjectType_Mover2:
						break;
					}
				}
			}
		}
	}
	catch (std::exception & e)
	{
		LOG_ERR(e.what());
	}
}

void GameSim::resetGameWorld()
{
	// reset map

	m_arena.reset();

	// reset pickups

	for (int i = 0; i < MAX_PICKUPS; ++i)
		m_pickups[i] = Pickup();

	m_grabbedPickup = Pickup();

	m_nextPickupSpawnTick = 0;

	// reset floor effect

	m_floorEffect = FloorEffect();

	// reset torches

	for (int i = 0; i < MAX_TORCHES; ++i)
		m_torches[i].m_isAlive = false;

	m_freezeTicks = 0;

	// reset bullets

	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		if (m_bulletPool->m_bullets[i].isAlive)
			m_bulletPool->free(i);

		if (m_particlePool->m_bullets[i].isAlive)
			m_particlePool->free(i);
	}

	// reset screen shakes

	for (int i = 0; i < MAX_SCREEN_SHAKES; ++i)
		m_screenShakes[i] = ScreenShake();

	// reset token hunt game mode

	m_tokenHunt = TokenHunt();

	// reset coin collector game mode

	m_coinCollector = CoinCollector();
}

void GameSim::resetPlayers()
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		m_players[i] = Player();
	}
}

void GameSim::resetGameSim()
{
	resetGameWorld();

	resetPlayers();
}

void GameSim::tick()
{
	switch (m_gameState)
	{
	case kGameState_Menus:
		tickMenus();
		break;

	case kGameState_Play:
		tickPlay();
		break;

	case kGameState_RoundComplete:
		tickRoundComplete();
		break;
	}
}

void GameSim::tickMenus()
{
	// wait for the host to enter the next game state
}

void GameSim::tickPlay()
{
	g_gameSim = this;

	if (g_devMode && ENABLE_GAMESTATE_CRC_LOGGING)
	{
		int numPlayers = 0;
		for (int i = 0; i < MAX_PLAYERS; ++i)
			if (m_playerNetObjects[i])
				numPlayers++;

		if (g_logCRCs)
		{
			const uint32_t crc = calcCRC();
			LOG_DBG("gamesim %p: tick=%u, crc=%08x, numPlayers=%d", this, m_tick, crc, numPlayers);
		}
	}

	const float dt = 1.f / TICKS_PER_SECOND;

	const uint32_t tick = GetTick();

	if (m_freezeTicks > 0)
	{
		m_freezeTicks--;
		
		return;
	}

	// arena update
	m_arena.tick(*this);

	// player update

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (m_playerNetObjects[i])
			m_playerNetObjects[i]->m_player->tick(dt);
	}

	// picksup

	for (int i = 0; i < MAX_PICKUPS; ++i)
	{
		if (m_pickups[i].isAlive)
			m_pickups[i].tick(*this, dt);
	}

	// pickup spawning

	if (tick >= m_nextPickupSpawnTick)
	{
		int numPickups = 0;
		for (int i = 0; i < MAX_PICKUPS; ++i)
			if (m_pickups[i].isAlive)
				numPickups++;

		if (numPickups < MAX_PICKUP_COUNT)
		{
			int weights[kPickupType_COUNT] =
			{
				PICKUP_AMMO_WEIGHT,
				PICKUP_NADE_WEIGHT,
				PICKUP_SHIELD_WEIGHT,
				PICKUP_ICE_WEIGHT,
				PICKUP_BUBBLE_WEIGHT
			};

			int totalWeight = 0;

			for (int i = 0; i < kPickupType_COUNT; ++i)
			{
				totalWeight += weights[i];
				weights[i] = totalWeight;
			}

			if (DEBUG_RANDOM_CALLSITES)
				LOG_DBG("Random called from pre trySpawnPickup");
			int value = Random() % totalWeight;

			PickupType type = kPickupType_COUNT;

			for (int i = 0; type == kPickupType_COUNT; ++i)
				if (value < weights[i])
					type = (PickupType)i;

			trySpawnPickup(type);
		}

		m_nextPickupSpawnTick = tick + (PICKUP_INTERVAL + (Random() % PICKUP_INTERVAL_VARIANCE)) * TICKS_PER_SECOND;
	}

	// token

	if (m_gameMode == kGameMode_TokenHunt)
	{
		m_tokenHunt.m_token.tick(*this, dt);
	}

	// coins

	if (m_gameMode == kGameMode_CoinCollector)
	{
		for (int i = 0; i < MAX_COINS; ++i)
		{
			m_coinCollector.m_coins[i].tick(*this, dt);
		}

		if (tick >= m_coinCollector.m_nextSpawnTick)
		{
			int numCoins = 0;

			for (int i = 0; i < MAX_COINS; ++i)
				if (m_coinCollector.m_coins[i].m_isDropped)
					numCoins++;

			for (int i = 0; i < MAX_PLAYERS; ++i)
				if (m_players[i].m_isAlive)
					numCoins += m_players[i].m_score;

			if (numCoins < COINCOLLECTOR_COIN_LIMIT)
			{
				spawnCoin();
			}

			m_coinCollector.m_nextSpawnTick = tick + (COIN_SPAWN_INTERVAL + (Random() % COIN_SPAWN_INTERVAL_VARIANCE)) * TICKS_PER_SECOND;
		}
	}

	// floor effect

	m_floorEffect.tick(*this, dt);

	// torches

	for (int i = 0; i < MAX_TORCHES; ++i)
	{
		if (m_torches[i].m_isAlive)
			m_torches[i].tick(*this, dt);
	}

	anim(dt);

	m_bulletPool->tick(*this, dt);

	m_tick++;

	g_gameSim = 0;
}

void GameSim::tickRoundComplete()
{
	// wait for the host to enter the next game state
}

void GameSim::anim(float dt)
{
	// screen shakes

	for (int i = 0; i < MAX_SCREEN_SHAKES; ++i)
	{
		ScreenShake & shake = m_screenShakes[i];
		if (shake.isActive)
			shake.tick(dt);
	}

	m_particlePool->tick(*this, dt);
}

void GameSim::playSound(const char * filename, int volume)
{
	if (g_noSound || !g_app->getSelectedClient() || g_app->getSelectedClient()->m_gameSim != this)
		return;

	Sound(filename).play(volume);
}

void GameSim::trySpawnPickup(PickupType type)
{
	for (int i = 0; i < MAX_PICKUPS; ++i)
	{
		Pickup & pickup = m_pickups[i];

		if (!pickup.isAlive)
		{
			const int kMaxLocations = ARENA_SX * ARENA_SY;
			int numLocations = kMaxLocations;
			int x[kMaxLocations];
			int y[kMaxLocations];

			if (m_arena.getRandomPickupLocations(x, y, numLocations, this,
				[](void * obj, int x, int y) 
				{
					GameSim * self = (GameSim*)obj;
					for (int i = 0; i < MAX_PICKUPS; ++i)
						if (self->m_pickups[i].blockX == x && self->m_pickups[i].blockY == y)
							return true;
					return false;
				}))
			{
				if (DEBUG_RANDOM_CALLSITES)
					LOG_DBG("Random called from trySpawnPickup");
				const int index = Random() % numLocations;
				const int spawnX = x[index];
				const int spawnY = y[index];

				spawnPickup(pickup, type, spawnX, spawnY);
			}

			break;
		}
	}
}

void GameSim::spawnPickup(Pickup & pickup, PickupType type, int blockX, int blockY)
{
	pickup.isAlive = true;
	pickup.setup(type, blockX, blockY);
}

Pickup * GameSim::grabPickup(int x1, int y1, int x2, int y2)
{
	CollisionInfo collisionInfo;
	collisionInfo.x1 = x1;
	collisionInfo.y1 = y1;
	collisionInfo.x2 = x2;
	collisionInfo.y2 = y2;

	for (int i = 0; i < MAX_PICKUPS; ++i)
	{
		Pickup & pickup = m_pickups[i];

		if (pickup.isAlive)
		{
			CollisionInfo pickupCollision;
			pickup.getCollisionInfo(pickupCollision);

			if (collisionInfo.intersects(pickupCollision))
			{
				m_grabbedPickup = pickup;
				pickup.isAlive = false;

				playSound("gun-pickup.ogg");

				return &m_grabbedPickup;
			}
		}
	}

	return 0;
}

void GameSim::spawnToken()
{
	Token & token = m_tokenHunt.m_token;

	const int kMaxLocations = ARENA_SX * ARENA_SY;
	int numLocations = kMaxLocations;
	int x[kMaxLocations];
	int y[kMaxLocations];

	if (m_arena.getRandomPickupLocations(x, y, numLocations, this, 0))
	{
		const int index = Random() % numLocations;
		const int spawnX = x[index];
		const int spawnY = y[index];

		token.setup(spawnX, spawnY);
	}
}

bool GameSim::pickupToken(const CollisionInfo & collisionInfo)
{
	Token & token = m_tokenHunt.m_token;

	if (token.m_isDropped && token.m_dropTimer <= 0.f)
	{
		CollisionInfo tokenCollision;
		token.getCollisionInfo(tokenCollision);

		if (tokenCollision.intersects(collisionInfo))
		{
			token.m_isDropped = false;
			g_gameSim->playSound("token-pickup.ogg");

			return true;
		}
	}

	return false;
}

//

Coin * GameSim::allocCoin()
{
	for (int i = 0; i < MAX_COINS; ++i)
	{
		if (!m_coinCollector.m_coins[i].m_isDropped)
		{
			return &m_coinCollector.m_coins[i];
		}
	}

	return 0;
}

void GameSim::spawnCoin()
{
	Coin * coin = allocCoin();

	if (coin)
	{
		const int kMaxLocations = ARENA_SX * ARENA_SY;
		int numLocations = kMaxLocations;
		int x[kMaxLocations];
		int y[kMaxLocations];

		if (m_arena.getRandomPickupLocations(x, y, numLocations, this, 0))
		{
			const int index = Random() % numLocations;
			const int spawnX = x[index];
			const int spawnY = y[index];

			coin->setup(spawnX, spawnY);
		}
	}
}

bool GameSim::pickupCoin(const CollisionInfo & collisionInfo)
{
	for (int i = 0; i < MAX_COINS; ++i)
	{
		Coin & coin = m_coinCollector.m_coins[i];

		if (coin.m_isDropped && coin.m_dropTimer <= 0.f)
		{
			CollisionInfo coinCollision;
			coin.getCollisionInfo(coinCollision);

			if (coinCollision.intersects(collisionInfo))
			{
				coin.m_isDropped = false;
				g_gameSim->playSound("token-pickup.ogg");

				return true;
			}
		}
	}

	return false;
}

//

uint16_t GameSim::spawnBullet(int16_t x, int16_t y, uint8_t _angle, BulletType type, BulletEffect effect, uint8_t ownerPlayerId)
{
	const uint16_t id = m_bulletPool->alloc();

	if (id != INVALID_BULLET_ID)
	{
		Bullet & b = m_bulletPool->m_bullets[id];

		Assert(!b.isAlive);
		b = Bullet();
		b.isAlive = true;
		b.type = static_cast<BulletType>(type);
		b.effect = static_cast<BulletEffect>(effect);
		b.m_pos[0] = x;
		b.m_pos[1] = y;
		b.color = 0xffffffff;

		float angle = _angle / 128.f * float(M_PI);
		float velocity = 0.f;

		switch (type)
		{
		case kBulletType_A:
			velocity = BULLET_TYPE0_SPEED;
			b.maxWrapCount = BULLET_TYPE0_MAX_WRAP_COUNT;
			b.maxReflectCount = BULLET_TYPE0_MAX_REFLECT_COUNT;
			b.maxDistanceTravelled = BULLET_TYPE0_MAX_DISTANCE_TRAVELLED;
			b.maxDestroyedBlocks = 1;
			break;
		case kBulletType_B:
			velocity = BULLET_TYPE0_SPEED;
			b.maxWrapCount = BULLET_TYPE0_MAX_WRAP_COUNT;
			b.maxReflectCount = BULLET_TYPE0_MAX_REFLECT_COUNT;
			b.maxDistanceTravelled = BULLET_TYPE0_MAX_DISTANCE_TRAVELLED;
			b.maxDestroyedBlocks = 1;
			break;
		case kBulletType_Grenade:
			velocity = BULLET_GRENADE_NADE_SPEED;
			b.maxWrapCount = 100;
			b.m_noGravity = false;
			b.doBounce = true;
			b.bounceAmount = BULLET_GRENADE_NADE_BOUNCE_AMOUNT;
			b.noDamageMap = true;
			b.life = BULLET_GRENADE_NADE_LIFE;
			break;
		case kBulletType_GrenadeA:
			velocity = RandomFloat(BULLET_GRENADE_FRAG_SPEED_MIN, BULLET_GRENADE_FRAG_SPEED_MAX);
			b.maxWrapCount = 1;
			b.maxReflectCount = 0;
			b.maxDistanceTravelled = RandomFloat(BULLET_GRENADE_FRAG_RADIUS_MIN, BULLET_GRENADE_FRAG_RADIUS_MAX);
			b.maxDestroyedBlocks = 1;
			b.doDamageOwner = true;
			break;
		default:
			Assert(false);
			break;
		}

		b.setVel(angle, velocity);

		b.ownerPlayerId = ownerPlayerId;
	}

	return id;
}

void GameSim::spawnParticles(const ParticleSpawnInfo & spawnInfo)
{
	for (int i = 0; i < spawnInfo.count; ++i)
	{
		uint16_t id = m_particlePool->alloc();

		if (id != INVALID_BULLET_ID)
		{
			Bullet & b = m_particlePool->m_bullets[id];

			initBullet(*this, b, spawnInfo);

			b.doAgeAlpha = true;
		}
	}
}

void GameSim::addScreenShake(float dx, float dy, float stiffness, float life)
{
	for (int i = 0; i < MAX_SCREEN_SHAKES; ++i)
	{
		ScreenShake & shake = m_screenShakes[i];
		if (!shake.isActive)
		{
			shake.isActive = true;
			shake.life = life;
			shake.stiffness = stiffness;

			shake.pos.Set(dx, dy);
			shake.vel.Set(0.f, 0.f);
			return;
		}
	}

	if (DEBUG_RANDOM_CALLSITES)
		LOG_DBG("Random called from addScreenShake");
	m_screenShakes[Random() % MAX_SCREEN_SHAKES].isActive = false;
	addScreenShake(dx, dy, stiffness, life);
}

Vec2 GameSim::getScreenShake() const
{
	Vec2 result;

	for (int i = 0; i < MAX_SCREEN_SHAKES; ++i)
	{
		const ScreenShake & shake = m_screenShakes[i];
		if (shake.isActive)
			result += shake.pos;
	}

	return result;
}

void GameSim::addFloorEffect(int playerId, int x, int y, int size, int damageSize)
{
	m_floorEffect.trySpawnAt(*this, playerId, x, y, -BLOCK_SX, size, damageSize);
	m_floorEffect.trySpawnAt(*this, playerId, x, y, +BLOCK_SX, size, damageSize);
}
