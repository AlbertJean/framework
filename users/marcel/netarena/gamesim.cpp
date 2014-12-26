#include "framework.h"
#include "gamedefs.h"
#include "gamesim.h"
#include "main.h"

OPTION_DECLARE(int, g_pickupTimeBase, 10);
OPTION_DEFINE(int, g_pickupTimeBase, "Pickup/Spawn Interval (Sec)");
OPTION_DECLARE(int, g_pickupTimeRandom, 5);
OPTION_DEFINE(int, g_pickupTimeRandom, "Pickup/Spawn Interval Random (Sec)");
OPTION_DECLARE(int, g_pickupMax, 5);
OPTION_DEFINE(int, g_pickupMax, "Pickup/Maximum Pickup Count");

//

static const char * s_pickupSprites[kPickupType_COUNT] =
{
	"pickup-ammo.png",
	"pickup-nade.png"
};

//

uint32_t GameSim::GameState::Random()
{
	m_randomSeed += 1;
	m_randomSeed *= 16807;
	return m_randomSeed;
}

uint32_t GameSim::GameState::GetTick()
{
	return m_tick;
}

uint32_t GameSim::calcCRC() const
{
	uint32_t result = 0;

	const uint8_t * bytes = (uint8_t*)&m_state;
	const uint32_t numBytes = sizeof(m_state);

	for (uint32_t i = 0; i < numBytes; ++i)
		result = result * 13 + bytes[i];

	return result;
}

void GameSim::serialize(NetSerializationContext & context)
{
	uint32_t crc;

	if (context.IsSend())
		crc = calcCRC();

	context.Serialize(crc);
	context.SerializeBytes(&m_state, sizeof(m_state));

	m_arena.serialize(context);

	if (context.IsRecv())
		Assert(crc == calcCRC());

	// todo : serialize player animation state, since it affects game play
}

void GameSim::tick()
{
	const uint32_t tick = m_state.GetTick();

	// pickup spawning

	if (tick >= m_state.m_nextPickupSpawnTick)
	{
		int weights[kPickupType_COUNT] =
		{
			PICKUP_AMMO_WEIGHT,
			PICKUP_NADE_WEIGHT
		};

		int totalWeight = 0;

		for (int i = 0; i < kPickupType_COUNT; ++i)
		{
			totalWeight += weights[i];
			weights[i] = totalWeight;
		}

		int value = m_state.Random() % totalWeight;

		PickupType type = kPickupType_COUNT;

		for (int i = 0; type == kPickupType_COUNT; ++i)
			if (value < weights[i])
				type = (PickupType)i;

		trySpawnPickup(type);

		m_state.m_nextPickupSpawnTick = tick + (g_pickupTimeBase + (m_state.Random() % g_pickupTimeRandom)) * TICKS_PER_SECOND;
	}

	m_state.m_tick++;
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
}

void GameSim::trySpawnPickup(PickupType type)
{
	for (int i = 0; i < MAX_PICKUPS; ++i)
	{
		Pickup & pickup = m_state.m_pickups[i];

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
						if (self->m_state.m_pickups[i].blockX == x && self->m_state.m_pickups[i].blockY == y)
							return true;
					return false;
				}))
			{
				const int index = m_state.Random() % numLocations;
				int spawnX = x[index];
				int spawnY = y[index];

				spawnPickup(pickup, type, spawnX, spawnY);
			}

			break;
		}
	}
}

void GameSim::spawnPickup(Pickup & pickup, PickupType type, int blockX, int blockY)
{
	const char * filename = s_pickupSprites[type];

	Sprite sprite(filename);

	pickup.isAlive = true;
	pickup.type = type;
	pickup.blockX = blockX;
	pickup.blockY = blockY;
	pickup.x1 = blockX * BLOCK_SX + (BLOCK_SX - sprite.getWidth()) / 2;
	pickup.y1 = blockY * BLOCK_SY + BLOCK_SY - sprite.getHeight();
	pickup.x2 = pickup.x1 + sprite.getWidth();
	pickup.y2 = pickup.y1 + sprite.getHeight();
	pickup.spriteId = g_app->netAddSprite(filename, pickup.x1, pickup.y1);
}

Pickup * GameSim::grabPickup(int x1, int y1, int x2, int y2)
{
	for (int i = 0; i < MAX_PICKUPS; ++i)
	{
		Pickup & pickup = m_state.m_pickups[i];

		if (pickup.isAlive)
		{
			if (x2 >= pickup.x1 && x1 < pickup.x2 &&
				y2 >= pickup.y1 && y1 < pickup.y2)
			{
				m_state.m_grabbedPickup = pickup;
				pickup.isAlive = false;

				g_app->netRemoveSprite(m_state.m_grabbedPickup.spriteId);

				g_app->netPlaySound("gun-pickup.ogg");

				return &m_state.m_grabbedPickup;
			}
		}
	}

	return 0;
}

void GameSim::addScreenShake(Vec2 delta, float stiffness, float life)
{
	for (int i = 0; i < MAX_SCREEN_SHAKES; ++i)
	{
		ScreenShake & shake = m_screenShakes[i];
		if (!shake.isActive)
		{
			shake.isActive = true;
			shake.life = life;
			shake.stiffness = stiffness;

			shake.pos = delta;
			shake.vel.Set(0.f, 0.f);

			return;
		}
	}
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
