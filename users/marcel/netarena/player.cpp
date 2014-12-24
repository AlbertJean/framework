#include "arena.h"
#include "bullet.h"
#include "Calc.h"
#include "framework.h"
#include "host.h"
#include "main.h"
#include "player.h"

/*

todo:

+ clash sound on attack cancel
+ random sound selection on event to avoid repeat sounds
+ fire up/down
+ fire affects destructible blocks
+ respawn on round begin
+ respawn on level load
+ avoid repeating spawn location
+ play sounds for current client only to avoid volume ramp
+ meelee X, pickup B
+ wrapping support attack
+ add pickup sound
+ add direct connect option
+ spring block tweakable
- more death feedback
- respawn delay
- character selection

- attack cancel
+ bullet teleport
+ separate player hitbox for damage
- grenades!
- ammo drop gravity
- input lock dash weg
- jump velocity reset na 10 pixels of richting change
- ammo despawn na x seconds + indicator

nice to haves:
- blood particles
- real time lighting
- fill the level with lava
- bee attack

*/

// from internal.h
void splitString(const std::string & str, std::vector<std::string> & result, char c);

OPTION_DECLARE(int, s_playerCharacterIndex, -1);
OPTION_DEFINE(int, s_playerCharacterIndex, "Player/Character Index (On Create)");
OPTION_ALIAS(s_playerCharacterIndex, "character");

OPTION_DECLARE(bool, s_unlimitedAmmo, false);
OPTION_DEFINE(bool, s_unlimitedAmmo, "Player/Unlimited Ammo");

#define WRAP_AROUND_TOP_AND_BOTTOM 1

// todo : m_isGrounded should be true when stickied too. review code and make change!

enum PlayerAnim
{
	kPlayerAnim_NULL,
	kPlayerAnim_Jump,
	kPlayerAnim_WallSlide,
	kPlayerAnim_Walk,
	kPlayerAnim_Attack,
	kPlayerAnim_Fire,
	kPlayerAnim_AirDash,
	kPlayerAnim_Spawn,
	kPlayerAnim_Die,
	kPlayerAnim_COUNT
};

struct PlayerAnimInfo
{
	const char * file;
	int prio;
} s_animInfos[kPlayerAnim_COUNT] =
{
	{ nullptr,                          0 },
	{ "char%d/jump/jump.png",           1 },
	{ "char%d/wallslide/wallslide.png", 2 },
	{ "char%d/walk/walk.png",           3 },
	{ "char%d/attack/attack.png",       4 },
	{ "char%d/shoot/shoot.png",         4 },
	{ "char%d/dash/dash.png",           4 },
	{ "char%d/spawn/spawn.png",         5 },
	{ "char%d/die/die.png",             6 }
};

//

void PlayerState_NS::SerializeStruct()
{
	uint8_t oldCharacterIndex = characterIndex;

	Serialize(isAlive);
	Serialize(score);
	Serialize(totalScore);
	Serialize(playerId);
	Serialize(characterIndex);

	if (characterIndex != oldCharacterIndex)
	{
		Player * player = static_cast<Player*>(GetOwner());

		player->handleCharacterIndexChange();
	}
}

PlayerState_NS::PlayerState_NS(NetSerializableObject * owner)
	: NetSerializable(owner)
	, isAlive(false)
	, score(0)
	, totalScore(0)
	, playerId(-1)
	, characterIndex(-1)
{
}

//

void PlayerAnim_NS::SerializeStruct()
{
	SerializeBits(m_anim, 4);
	Serialize(m_play);

	int dx = m_attackDx + 1;
	int dy = m_attackDy + 1;
	SerializeBits(dx, 2);
	SerializeBits(dy, 2);
	m_attackDx = dx - 1;
	m_attackDy = dy - 1;

	//

	Player * player = static_cast<Player*>(GetOwner());

	if (m_anim != kPlayerAnim_NULL)
	{
		delete player->m_sprite;
		player->m_sprite = 0;

		char filename[64];
		sprintf_s(filename, sizeof(filename), s_animInfos[m_anim].file, player->getCharacterIndex());

		player->m_sprite = new Sprite(filename);
		player->m_sprite->animActionHandler = Player::handleAnimationAction;
		player->m_sprite->animActionHandlerObj = player;
	}

	if (player->m_sprite)
	{
		if (m_play)
			player->m_sprite->startAnim("anim");
		else
			player->m_sprite->stopAnim();
	}
}

//

SoundBag::SoundBag()
	: m_lastIndex(-1)
	, m_random(false)
{
}

void SoundBag::load(const std::string & files, bool random)
{
	splitString(files, m_files, ',');
	m_lastIndex = -1;
	m_random = random;
}

const char * SoundBag::getRandomSound()
{
	if (m_files.empty())
		return "";
	else if (m_files.size() == 1)
		return m_files.front().c_str();
	else
	{
		for (;;)
		{
			int index;

			if (m_random)
				index = rand() % m_files.size();
			else
				index = (index + 1) % m_files.size();

			if (index != m_lastIndex)
			{
				m_lastIndex = index;

				return m_files[index].c_str();
			}
		}
	}

	return "";
}

//

void Player::handleAnimationAction(const std::string & action, const Dictionary & args)
{
	Player * player = args.getPtrType<Player>("obj", 0);
	if (player && player->m_isAuthorative)
	{
		if (g_devMode)
		{
			log("action: %s", action.c_str());
		}

		if (action == "gravity_enable")
		{
			player->m_animAllowGravity = args.getBool("enable", true);
		}
		else if (action == "steering_enable")
		{
			player->m_animAllowSteering = args.getBool("enable", true);
		}
		else if (action == "set_anim_vel")
		{
			player->m_animVel[0] = args.getFloat("x", player->m_animVel[0]);
			player->m_animVel[1] = args.getFloat("y", player->m_animVel[1]);
		}
		else if (action == "set_anim_vel_abs")
		{
			player->m_animVelIsAbsolute = args.getBool("abs", player->m_animVelIsAbsolute);
			if (args.getBool("reset", true))
				player->m_vel = Vec2();
		}
		else if (action == "set_attack_vel")
		{
			player->m_attack.attackVel[0] = args.getFloat("x", player->m_attack.attackVel[0]);
			player->m_attack.attackVel[1] = args.getFloat("y", player->m_attack.attackVel[1]);
		}
		else if (action == "sound")
		{
			g_app->netPlaySound(args.getString("file", "").c_str(), args.getInt("volume", 100));
		}
		else if (action == "char_sound")
		{
			g_app->netPlaySound(player->makeCharacterFilename(args.getString("file", "").c_str()), args.getInt("volume", 100));
		}
		else if (action == "char_soundbag")
		{
			std::string name = args.getString("name", "");

			if (player->m_sounds.count(name.c_str()) == 0)
			{
				if (player->m_props.contains(name.c_str()))
					player->m_sounds[name].load(player->m_props.getString(name.c_str(), ""), true);
			}

			g_app->netPlaySound(player->makeCharacterFilename(player->m_sounds[name].getRandomSound()), args.getInt("volume", 100));
		}
		else
		{
			logError("unknown action: %s", action.c_str());
		}
	}
}

void Player::getPlayerCollision(CollisionInfo & collision)
{
	collision.x1 = m_pos[0] + m_collision.x1;
	collision.y1 = m_pos[1] + m_collision.y1;
	collision.x2 = m_pos[0] + m_collision.x2;
	collision.y2 = m_pos[1] + m_collision.y2;
}

void Player::getDamageHitbox(CollisionInfo & collision)
{
	collision.x1 = m_pos[0] - PLAYER_DAMAGE_HITBOX_SX/2;
	collision.y1 = m_pos[1] - PLAYER_DAMAGE_HITBOX_SY;
	collision.x2 = m_pos[0] + PLAYER_DAMAGE_HITBOX_SX/2;
	collision.y2 = m_pos[1];
}

void Player::getAttackCollision(CollisionInfo & collision)
{
	float x1 = m_attack.collision.x1 * m_pos.xFacing;
	float y1 = m_attack.collision.y1;
	float x2 = m_attack.collision.x2 * m_pos.xFacing;
	float y2 = m_attack.collision.y2;

	if (m_pos.yFacing < 0)
	{
		y1 = -PLAYER_COLLISION_HITBOX_SY - y1;
		y2 = -PLAYER_COLLISION_HITBOX_SY - y2;
	}

	if (x1 > x2)
		std::swap(x1, x2);
	if (y1 > y2)
		std::swap(y1, y2);

	collision.x1 = m_pos[0] + x1;
	collision.y1 = m_pos[1] + y1;
	collision.x2 = m_pos[0] + x2;
	collision.y2 = m_pos[1] + y2;
}

float Player::getAttackDamage(Player * other)
{
	float result = 0.f;

	if (m_attack.attacking && m_attack.hasCollision)
	{
		CollisionInfo attackCollision;
		CollisionInfo otherCollision;

		getAttackCollision(attackCollision);
		other->getDamageHitbox(otherCollision);

		if (attackCollision.intersects(otherCollision))
		{
			result = 1.f;
		}
	}

	return result;
}

bool Player::isAnimOverrideAllowed(int anim) const
{
	return !m_isAnimDriven || (s_animInfos[anim].prio > s_animInfos[m_anim.m_anim].prio);
}

float Player::mirrorX(float x) const
{
	return m_pos.xFacing > 0 ? x : -x;
}

float Player::mirrorY(float y) const
{
	return m_pos.yFacing > 0 ? y : PLAYER_COLLISION_HITBOX_SY - y;
}

Player::Player(uint32_t netId, uint16_t owningChannelId)
	: m_pos(this)
	, m_state(this)
	, m_anim(this)
	, m_weaponType(kPlayerWeapon_Fire)
	, m_weaponAmmo(0)
	, m_isAuthorative(false)
	, m_blockMask(0)
	, m_isGrounded(false)
	, m_isAttachedToSticky(false)
	, m_isAnimDriven(false)
	, m_animVelIsAbsolute(false)
	, m_animAllowGravity(true)
	, m_animAllowSteering(true)
	, m_sprite(0)
	, m_spriteScale(1.f)
	, m_isRespawn(false)
	, m_isAirDashCharged(false)
	, m_isWallSliding(false)
	, m_lastSpawnIndex(-1)
{
	setNetId(netId);
	setOwningChannelId(owningChannelId);

	if (netId != 0)
	{
		m_isAuthorative = true;
	}

	m_collision.x1 = -PLAYER_COLLISION_HITBOX_SX / 2.f;
	m_collision.x2 = +PLAYER_COLLISION_HITBOX_SX / 2.f;
	m_collision.y1 = -PLAYER_COLLISION_HITBOX_SY / 1.f;
	m_collision.y2 = 0.f;

	if (s_playerCharacterIndex != -1)
	{
		g_app->netSetPlayerCharacterIndex(getOwningChannelId(), getNetId(), s_playerCharacterIndex);
	}
}

Player::~Player()
{
	delete m_sprite;
	m_sprite = 0;
}

void Player::playSecondaryEffects(PlayerEvent e)
{
	switch (e)
	{
	case kPlayerEvent_Spawn:
		break;
	case kPlayerEvent_Respawn:
		g_app->netPlaySound(makeCharacterFilename(m_sounds["respawn"].getRandomSound()));
		break;
	case kPlayerEvent_Die:
		g_app->netPlaySound(makeCharacterFilename("die/die.ogg"));
		break;
	case kPlayerEvent_Jump:
		{
			Dictionary args;
			args.setPtr("obj", this);
			args.setString("name", "jump_sounds");
			handleAnimationAction("char_soundbag", args);
			break;
		}
	case kPlayerEvent_WallJump:
		g_app->netPlaySound(makeCharacterFilename("walljump.ogg"));
		break;
	case kPlayerEvent_LandOnGround:
		g_app->netPlaySound(makeCharacterFilename("land_on_ground.ogg"), 25);
		break;
	case kPlayerEvent_StickyAttach:
		g_app->netPlaySound("player-sticky-attach.ogg");
		break;
	case kPlayerEvent_StickyRelease:
		g_app->netPlaySound("player-sticky-release.ogg");
		break;
	case kPlayerEvent_StickyJump:
		g_app->netPlaySound("player-sticky-jump.ogg");
		break;
	case kPlayerEvent_SpringJump:
		g_app->netPlaySound("player-spring-jump.ogg");
		break;
	case kPlayerEvent_SpikeHit:
		g_app->netPlaySound("player-spike-hit.ogg");
		break;
	case kPlayerEvent_ArenaWrap:
		g_app->netPlaySound("player-arena-wrap.ogg");
		break;
	case kPlayerEvent_DashAir:
		break;
	case kPlayerEvent_DestructibleDestroy:
		g_app->netPlaySound("player-arena-wrap.ogg");
		break;
	}
}

void Player::tick(float dt)
{
	m_collision.x1 = -PLAYER_COLLISION_HITBOX_SX / 2.f;
	m_collision.x2 = +PLAYER_COLLISION_HITBOX_SX / 2.f;
	m_collision.y1 = -PLAYER_COLLISION_HITBOX_SY / 1.f;
	m_collision.y2 = 0.f;

	//

	if (m_isAnimDriven)
	{
		if (!m_anim.IsDirty() && !m_sprite->animIsActive)
		{
			m_isAnimDriven = false;
			m_animVel = Vec2();
			m_animVelIsAbsolute = false;
			m_animAllowGravity = true;
			m_animAllowSteering = true;

			switch (m_anim.m_anim)
			{
			case kPlayerAnim_Attack:
				m_attack.attacking = false;
				break;
			case kPlayerAnim_Fire:
				m_attack.attacking = false;
				break;
			case kPlayerAnim_AirDash:
				break;
			case kPlayerAnim_Jump:
				break;
			case kPlayerAnim_Spawn:
				m_state.isAlive = true;
				m_state.SetDirty();
				break;
			case kPlayerAnim_Die:
				m_state.isAlive = false;
				m_state.SetDirty();
				break;
			}
		}
	}

	//

	if (!m_state.isAlive || m_input.wentDown(INPUT_BUTTON_START))
	{
		respawn();
	}

	if (m_state.isAlive)
	{
		// see if we grabbed any pickup

		Pickup * pickup = g_host->grabPickup(
			m_pos[0] + m_collision.x1,
			m_pos[1] + m_collision.y1,
			m_pos[0] + m_collision.x2,
			m_pos[1] + m_collision.y2);

		if (pickup)
		{
			switch (pickup->type)
			{
			case kPickupType_Ammo:
				m_weaponAmmo = 1;
				m_weaponType = kPlayerWeapon_Fire;
				break;
			case kPickupType_Nade:
				m_weaponAmmo = 1;
				m_weaponType = kPlayerWeapon_Grenade;
				break;
			}
		}

		m_blockMask = ~0;

		const uint32_t currentBlockMask = getIntersectingBlocksMask(m_pos[0], m_pos[1]);
		
		const bool isInPassthough = (currentBlockMask & kBlockMask_Passthrough) != 0;
		if (isInPassthough || m_input.isDown(INPUT_BUTTON_DOWN))
			m_blockMask = ~kBlockMask_Passthrough;

		const uint32_t currentBlockMaskFloor = getIntersectingBlocksMask(m_pos[0], m_pos[1] + 1.f);
		const uint32_t currentBlockMaskCeil = getIntersectingBlocksMask(m_pos[0], m_pos[1] - 1.f);

		float surfaceFriction = 0.f;
		Vec2 animVel(0.f, 0.f);

		animVel[0] += m_animVel[0] * m_pos.xFacing;
		animVel[1] += m_animVel[1] * m_pos.yFacing;

		// attack

		if (m_attack.attacking)
		{
			if (m_anim.m_anim == kPlayerAnim_Attack)
			{
				const float attackVel = std::max<float>(m_attack.attackVel[0], m_attack.attackVel[1]);
				animVel[0] += attackVel * m_anim.m_attackDx;
				animVel[1] += attackVel * m_anim.m_attackDy;
			}
			else
			{
				animVel[0] += m_attack.attackVel[0] * m_pos.xFacing;
				animVel[1] += m_attack.attackVel[1] * m_pos.yFacing;
			}

			// see if we hit anyone

			for (auto i = g_host->m_players.begin(); i != g_host->m_players.end(); ++i)
			{
				Player * other = *i;

				if (other == this)
					continue;

				float damage1 = getAttackDamage(other);
				float damage2 = other->getAttackDamage(this);

				if (damage1 != 0.f)
				{
					if (damage2 != 0.f)
					{
						//log("-> attack cancel");

						Player * players[2] = { this, other };

						for (int i = 0; i < 2; ++i)
						{
							players[i]->m_attack.attacking = false;
						}

						g_app->netPlaySound("melee-cancel.ogg");
					}
					else
					{
						//log("-> attack damage");

						other->handleDamage(1.f, Vec2(m_pos.xFacing * PLAYER_SWORD_PUSH_SPEED, 0.f), this);
					}
				}
			}

			// see if we've hit a block

			CollisionInfo attackCollision;
			getAttackCollision(attackCollision);

			m_attack.hitDestructible |= g_hostArena->handleDamageRect(
				m_pos[0],
				m_pos[1],
				attackCollision.x1,
				attackCollision.y1,
				attackCollision.x2,
				attackCollision.y2,
				!m_attack.hitDestructible);
		}
		else
		{
			if (m_input.wentDown(INPUT_BUTTON_B) && (m_weaponAmmo > 0 || s_unlimitedAmmo) && isAnimOverrideAllowed(kPlayerWeapon_Fire))
			{
				m_attack = AttackInfo();

				int anim = -1;
				BulletType bulletType = kBulletType_COUNT;

				if (m_weaponType == kPlayerWeapon_Fire)
				{
					anim = kPlayerAnim_Fire;
					bulletType = kBulletType_B;
				}
				if (m_weaponType == kPlayerWeapon_Grenade)
				{
					bulletType = kBulletType_Grenade;
					g_app->netPlaySound("grenade-throw.ogg");
				}

				if (anim != -1)
				{
					m_anim.SetAnim(anim, true, true);
					m_isAnimDriven = true;

					m_attack.attacking = true;
				}

				m_weaponAmmo = (m_weaponAmmo > 0) ? (m_weaponAmmo - 1) : 0;

				// determine attack direction based on player input

				int angle;

				if (m_input.isDown(INPUT_BUTTON_UP))
					angle = 256*1/4;
				else if (m_input.isDown(INPUT_BUTTON_DOWN))
					angle = 256*3/4;
				else if (m_pos.xFacing < 0)
					angle = 128;
				else
					angle = 0;

				Assert(bulletType != kBulletType_COUNT);
				g_app->netSpawnBullet(
					m_pos[0] + mirrorX(0.f),
					m_pos[1] - mirrorY(44.f),
					angle,
					bulletType,
					getNetId());
			}

			if (m_input.wentDown(INPUT_BUTTON_X) && isAnimOverrideAllowed(kPlayerAnim_Attack))
			{
				m_attack = AttackInfo();
				m_attack.attacking = true;

				m_anim.SetAnim(kPlayerAnim_Attack, true, true);
				m_isAnimDriven = true;

				// determine attack direction based on player input

				if (m_input.isDown(INPUT_BUTTON_UP))
					m_anim.SetAttackDirection(0, -1);
				else if (m_input.isDown(INPUT_BUTTON_DOWN))
					m_anim.SetAttackDirection(0, +1);
				else
					m_anim.SetAttackDirection(m_pos.xFacing, 0);

				// determine attack collision. basically just 3 directions: forward, up and down

				m_attack.collision.x1 = 0.f;
				m_attack.collision.x2 = (m_anim.m_attackDx == 0) ? 0.f : 50.f; // fordward?
				m_attack.collision.y1 = -PLAYER_COLLISION_HITBOX_SY/3.f*2;
				m_attack.collision.y2 = -PLAYER_COLLISION_HITBOX_SY/3.f*2 + m_anim.m_attackDy * 50.f; // up or down

				// make sure the attack collision doesn't have a zero sized area

				if (m_attack.collision.x1 == m_attack.collision.x2)
				{
					m_attack.collision.x1 -= 2.f;
					m_attack.collision.x2 += 2.f;
				}
				if (m_attack.collision.y1 == m_attack.collision.y2)
				{
					m_attack.collision.y1 -= 2.f;
					m_attack.collision.y2 += 2.f;
				}

				m_attack.hasCollision = true;
			}
		}

		if (m_isAirDashCharged && !m_isGrounded && !m_isAttachedToSticky && m_input.wentDown(INPUT_BUTTON_A))
		{
			if (isAnimOverrideAllowed(kPlayerAnim_AirDash))
			{
				if ((getIntersectingBlocksMask(m_pos[0] + m_pos.xFacing, m_pos[1]) & kBlockMask_Solid) == 0)
				{
					m_isAirDashCharged = false;

					m_anim.SetAnim(kPlayerAnim_AirDash, true, true);
					m_isAnimDriven = true;
				}
			}
		}

		// death by spike

		if (currentBlockMask & (1 << kBlockType_Spike))
		{
			if (isAnimOverrideAllowed(kPlayerAnim_Die))
			{
				m_anim.SetAnim(kPlayerAnim_Die, true, true);
				m_isAnimDriven = true;

				awardScore(-1);

				playSecondaryEffects(kPlayerEvent_SpikeHit);
			}
		}

		// teleport

		{
			int px = int(m_pos[0] + (m_collision.x1 + m_collision.x2) / 2) / BLOCK_SX;
			int py = int(m_pos[1] + (m_collision.y1 + m_collision.y2) / 2) / BLOCK_SY;

			if (px != m_teleport.x || py != m_teleport.y)
			{
				m_teleport.cooldown = false;
			}

			if (!m_teleport.cooldown && px >= 0 && px < ARENA_SX && py >= 0 && py < ARENA_SY)
			{
				const Block & block = g_hostArena->getBlock(px, py);

				if (block.type == kBlockType_Teleport)
				{
					// find a teleport destination

					int destinationX;
					int destinationY;

					if (g_hostArena->getTeleportDestination(px, py, destinationX, destinationY))
					{
						m_pos[0] = destinationX * BLOCK_SX;
						m_pos[1] = destinationY * BLOCK_SY;

						m_pos[0] += BLOCK_SX / 2;
						m_pos[1] += BLOCK_SY - 1;

						m_teleport.cooldown = true;
						m_teleport.x = destinationX;
						m_teleport.y = destinationY;
					}
				}
			}
		}

		bool playerControl =
			m_animAllowSteering &&
			!m_isAnimDriven;

		// sticky ceiling

		if (currentBlockMaskCeil & (1 << kBlockType_Sticky))
		{
			if (playerControl && m_input.wentDown(INPUT_BUTTON_A))
			{
				m_vel[1] = PLAYER_JUMP_SPEED / 2.f;

				m_isAttachedToSticky = false;

				playSecondaryEffects(kPlayerEvent_StickyJump);
			}
			else if (playerControl && m_input.wentDown(INPUT_BUTTON_DOWN))
			{
				m_isAttachedToSticky = false;

				playSecondaryEffects(kPlayerEvent_StickyRelease);
			}
			else
			{
				surfaceFriction = FRICTION_GROUNDED;

				if (!m_isAttachedToSticky)
				{
					m_isAttachedToSticky = true;

					playSecondaryEffects(kPlayerEvent_StickyAttach);
				}
			}
		}
		else
		{
			m_isAttachedToSticky = false;
		}

		// steering

		float steeringSpeed = 0.f;

		if (true)
		{
			int numSteeringFrame = 1;

			if (m_input.isDown(INPUT_BUTTON_LEFT))
				steeringSpeed -= 1.f;
			if (m_input.isDown(INPUT_BUTTON_RIGHT))
				steeringSpeed += 1.f;

			if (m_isGrounded || m_isAttachedToSticky)
			{
				if (isAnimOverrideAllowed(kPlayerAnim_Walk))
				{
					if (steeringSpeed != 0.f)
						m_anim.SetAnim(kPlayerAnim_Walk, true, false);
					else
						m_anim.SetAnim(kPlayerAnim_Walk, false, false);
				}
			}

			//

			bool isWalkingOnSolidGround = false;

			if (m_isAttachedToSticky)
				isWalkingOnSolidGround = (currentBlockMaskCeil & kBlockMask_Solid) != 0;
			else
				isWalkingOnSolidGround = (currentBlockMaskFloor & kBlockMask_Solid) != 0;

			if (isWalkingOnSolidGround)
			{
				steeringSpeed *= STEERING_SPEED_ON_GROUND;
				numSteeringFrame = 5;
			}
			else
			{
				steeringSpeed *= STEERING_SPEED_IN_AIR;
				numSteeringFrame = 5;
			}

			if (playerControl && steeringSpeed != 0.f)
			{
				float maxSteeringDelta;

				if (steeringSpeed > 0)
					maxSteeringDelta = steeringSpeed - m_vel[0];
				if (steeringSpeed < 0)
					maxSteeringDelta = steeringSpeed - m_vel[0];

				if (Calc::Sign(steeringSpeed) == Calc::Sign(maxSteeringDelta))
					m_vel[0] += maxSteeringDelta * dt * 60.f / numSteeringFrame;
			}
		}

		// gravity

		float gravity;

		m_isWallSliding = false;

		if (!m_animAllowGravity)
			gravity = 0.f;
		if (m_isAttachedToSticky)
			gravity = 0.f;
		else if (m_animVelIsAbsolute)
			gravity = 0.f;
		else
		{
			if (currentBlockMask & (1 << kBlockType_GravityDisable))
				gravity = 0.f;
			else if (currentBlockMask & (1 << kBlockType_GravityReverse))
				gravity = GRAVITY * BLOCKTYPE_GRAVITY_REVERSE_MULTIPLIER;
			else if (currentBlockMask & (1 << kBlockType_GravityStrong))
				gravity = GRAVITY * BLOCKTYPE_GRAVITY_STRONG_MULTIPLIER;
			else if (currentBlockMask & ((1 << kBlockType_GravityLeft) | (1 << kBlockType_GravityRight)))
				gravity = 0.f;
			else
				gravity = GRAVITY;

			// wall slide

			if (!m_isGrounded &&
				!m_isAttachedToSticky &&
				isAnimOverrideAllowed(kPlayerAnim_WallSlide) &&
				m_vel[0] != 0.f && Calc::Sign(m_pos.xFacing) == Calc::Sign(m_vel[0]) &&
				//Calc::Sign(m_vel[1]) == Calc::Sign(gravity) &&
				(Calc::Sign(m_vel[1]) == Calc::Sign(gravity) || Calc::Abs(m_vel[1]) <= PLAYER_JUMP_SPEED / 2.f) &&
				(getIntersectingBlocksMask(m_pos[0] + m_pos.xFacing, m_pos[1]) & kBlockMask_Solid) != 0)
			{
				m_isWallSliding = true;

				m_anim.SetAnim(kPlayerAnim_WallSlide, true, false);

				if (m_vel[1] > PLAYER_WALLSLIDE_SPEED && Calc::Sign(m_vel[1]) == Calc::Sign(gravity))
					m_vel[1] = PLAYER_WALLSLIDE_SPEED;
			}
		}

		m_vel[1] += gravity * dt;

		if (currentBlockMask & (1 << kBlockType_GravityLeft))
			m_vel[0] -= GRAVITY * dt;
		if (currentBlockMask & (1 << kBlockType_GravityRight))
			m_vel[0] += GRAVITY * dt;

		// converyor belt

		if (currentBlockMaskFloor & (1 << kBlockType_ConveyorBeltLeft))
			animVel[0] += -BLOCKTYPE_CONVEYOR_SPEED;
		if (currentBlockMaskFloor & (1 << kBlockType_ConveyorBeltRight))
			animVel[0] += +BLOCKTYPE_CONVEYOR_SPEED;

		// collision

		for (int i = 0; i < 2; ++i)
		{
			float totalDelta =
				(
					(m_animVelIsAbsolute ? 0.f : m_vel[i]) +
					animVel[i]
				) * dt;

			const float deltaSign = totalDelta < 0.f ? -1.f : +1.f;

			if (i == 1)
			{
				// update passthrough mode
				m_blockMask = ~0;
				if ((getIntersectingBlocksMask(m_pos[0], m_pos[1]) & kBlockMask_Passthrough) || m_input.isDown(INPUT_BUTTON_DOWN))
					m_blockMask = ~kBlockMask_Passthrough;
			}

			while (totalDelta != 0.f)
			{
				const float delta = (std::abs(totalDelta) < 1.f) ? totalDelta : deltaSign;

				Vec2 newPos(m_pos.x, m_pos.y);

				newPos[i] += delta;

				uint32_t newBlockMask = getIntersectingBlocksMask(newPos[0], newPos[1]);

				// ignore passthough blocks if we're moving horizontally or upwards
				// todo : update block mask each iteration. reset m_blockMask first
				//if (i != 1 || delta <= 0.f)
				//if ((!m_isGrounded || isInPassthough) && (i != 0 || delta <= 0.f))
				if ((i == 0 && isInPassthough) || (i == 1 && delta <= 0.f))
					newBlockMask &= ~kBlockMask_Passthrough;

				// make sure we stay grounded, within reason. allows the player to walk up/down slopes
				if (i == 0 && m_isGrounded)
				{
					if (newBlockMask & kBlockMask_Solid)
					{
						for (int dy = 1; dy <= 2; ++dy)
						{
							const uint32_t blockMask = getIntersectingBlocksMask(newPos[0], newPos[1] - dy);

							if ((blockMask & kBlockMask_Solid) == 0)
							{
								m_pos[1] -= dy;
								newPos[1] -= dy;
								newBlockMask = blockMask;
								break;
							}
						}
					}
					else
					{
						for (int dy = 1; dy <= 1; ++dy)
						{
							const uint32_t blockMask = getIntersectingBlocksMask(newPos[0], newPos[1] + 1.f);

							if ((blockMask & kBlockMask_Solid) == 0)
							{
								m_pos[1] += 1.f;
								newPos[1] += 1.f;
								newBlockMask = blockMask;
								break;
							}
						}
					}
				}

				if (newBlockMask & kBlockMask_Solid)
				{
					if (i == 0)
					{
						// colliding with solid object left/right of player

						if (!m_isGrounded && playerControl && m_input.wentDown(INPUT_BUTTON_A))
						{
							// wall jump

							m_vel[0] = -PLAYER_WALLJUMP_RECOIL_SPEED * deltaSign;
							m_vel[1] = -PLAYER_WALLJUMP_SPEED;

							playSecondaryEffects(kPlayerEvent_WallJump);
						}
						else
						{
							m_vel[0] = 0.f;

							if (m_isAnimDriven && m_anim.m_anim == kPlayerAnim_AirDash)
							{
								m_sprite->stopAnim();
							}
						}
					}

					if (i == 1)
					{
						// grounded

						if (newBlockMask & (1 << kBlockType_Slide))
							surfaceFriction = 0.f;
						else
							surfaceFriction = FRICTION_GROUNDED;

						if (delta >= 0.f)
						{
							if (playerControl && m_input.wentDown(INPUT_BUTTON_A))
							{
								// jumping

								m_vel[i] = -PLAYER_JUMP_SPEED;

								playSecondaryEffects(kPlayerEvent_Jump);
							}
							else if (newBlockMask & (1 << kBlockType_Spring))
							{
								// spring

								m_vel[i] = -BLOCKTYPE_SPRING_SPEED;

								playSecondaryEffects(kPlayerEvent_SpringJump);
							}
							else
							{
								m_vel[i] = 0.f;
							}
						}
						else
						{
							m_vel[i] = 0.f;
						}
					}

					totalDelta = 0.f;
				}
				else
				{
					m_pos[i] = newPos[i];

					totalDelta -= delta;
				}
			}
		}

		// grounded?

		if (m_vel[1] >= 0.f && (getIntersectingBlocksMask(m_pos[0], m_pos[1] + 1.f) & kBlockMask_Solid) != 0)
		{
			if (!m_isGrounded)
			{
				m_isGrounded = true;

				playSecondaryEffects(kPlayerEvent_LandOnGround);
			}
		}
		else
		{
			m_isGrounded = false;
		}

		// breaking

		if (steeringSpeed == 0.f)
		{
			m_vel[0] *= powf(1.f - surfaceFriction, dt * 60.f);
		}

		// air dash

		if (m_isGrounded || m_isAttachedToSticky || m_isWallSliding)
		{
			m_isAirDashCharged = true;
		}

		// animation

		if (!m_isGrounded && !m_isAttachedToSticky && !m_isWallSliding && isAnimOverrideAllowed(kPlayerAnim_Jump))
		{
			m_anim.SetAnim(kPlayerAnim_Jump, true, false);
		}

	#if !WRAP_AROUND_TOP_AND_BOTTOM
		// death by fall

		if (m_pos[1] > ARENA_SY_PIXELS)
		{
			if (isAnimOverrideAllowed(kPlayerAnim_Die))
			{
				m_anim.SetAnim(kPlayerAnim_Die, true, true);
				m_isAnimDriven = true;

				awardScore(-1);
			}
		}
	#endif

		// facing

		if (playerControl && steeringSpeed != 0.f)
			m_pos.xFacing = steeringSpeed < 0.f ? -1 : +1;
		m_pos.yFacing = m_isAttachedToSticky ? -1 : +1;

		// wrapping

		if (m_pos[0] < 0)
		{
			m_pos[0] = ARENA_SX_PIXELS;
			playSecondaryEffects(kPlayerEvent_ArenaWrap);
		}
		if (m_pos[0] > ARENA_SX_PIXELS)
		{
			m_pos[0] = 0;
			playSecondaryEffects(kPlayerEvent_ArenaWrap);
		}

	#if WRAP_AROUND_TOP_AND_BOTTOM
		if (m_pos[1] > ARENA_SY_PIXELS)
		{
			m_pos[1] = 0;
			playSecondaryEffects(kPlayerEvent_ArenaWrap);
		}

		if (m_pos[1] < 0)
		{
			m_pos[1] = ARENA_SY_PIXELS;
			playSecondaryEffects(kPlayerEvent_ArenaWrap);
		}
	#endif
	}

	m_input.next();

	//printf("x: %g\n", m_pos[0]);

	m_pos.SetDirty();
}

void Player::draw()
{
	if (!hasValidCharacterIndex())
		return;

	m_sprite->flipX = m_pos.xFacing < 0 ? true : false;
	m_sprite->flipY = m_pos.yFacing < 0 ? true : false;

	drawAt(m_pos.x, m_pos.y - (m_sprite->flipY ? PLAYER_COLLISION_HITBOX_SY : 0));

	// render additional sprites for wrap around
	drawAt(m_pos.x + ARENA_SX_PIXELS, m_pos.y - (m_sprite->flipY ? PLAYER_COLLISION_HITBOX_SY : 0));
	drawAt(m_pos.x - ARENA_SX_PIXELS, m_pos.y - (m_sprite->flipY ? PLAYER_COLLISION_HITBOX_SY : 0));
	drawAt(m_pos.x, m_pos.y - (m_sprite->flipY ? PLAYER_COLLISION_HITBOX_SY : 0) + ARENA_SY_PIXELS);
	drawAt(m_pos.x, m_pos.y - (m_sprite->flipY ? PLAYER_COLLISION_HITBOX_SY : 0) - ARENA_SY_PIXELS);

	// draw player color

	const int playerId = m_state.playerId;
	const int alpha = 127;

	if (playerId >= 0 && playerId < 4)
	{
		struct color
		{
			unsigned char r, g, b;
		} colors[4] =
		{
			{ 255, 0, 0   },
			{ 255, 255, 0 },
			{ 0, 0, 255   },
			{ 0, 255, 0   }
		};
		setColor(
			colors[playerId].r,
			colors[playerId].g,
			colors[playerId].b,
			alpha);
	}
	else
	{
		setColor(50, 50, 50, alpha);
	}
	drawRect(m_pos[0] - 50, m_pos[1] - 110, m_pos[0] + 50, m_pos[1] - 85);

	// draw score
	setFont("calibri.ttf");
	setColor(255, 255, 255);
	drawText(m_pos[0], m_pos[1] - 110, 20, 0.f, +1.f, "%d", m_state.score);
}

void Player::drawAt(int x, int y)
{
	setColor(colorWhite);
	m_sprite->drawEx(x, y, 0.f, m_spriteScale);

	if (m_anim.m_anim == kPlayerAnim_Attack)
	{
		CollisionInfo collisionInfo;
		getAttackCollision(collisionInfo);

		setColor(255, 0, 0);
		//drawRect(collisionInfo.x1, collisionInfo.y1, collisionInfo.x2, collisionInfo.y2);
		//drawLine(x, y, x + m_anim.m_attackDx * 50, y + m_anim.m_attackDy * 50);
	}
}

void Player::debugDraw()
{
	setColor(0, 31, 63, 63);
	drawRect(
		m_pos.x + m_collision.x1,
		m_pos.y + m_collision.y1,
		m_pos.x + m_collision.x2 + 1,
		m_pos.y + m_collision.y2 + 1);

	CollisionInfo damageCollision;
	getDamageHitbox(damageCollision);
	setColor(63, 31, 0, 63);
	drawRect(
		damageCollision.x1,
		damageCollision.y1,
		damageCollision.x2 + 1,
		damageCollision.y2 + 1);

	if (m_isAuthorative)
	{
		if (m_attack.attacking && m_attack.hasCollision)
		{
			CollisionInfo attackCollision;
			getAttackCollision(attackCollision);

			Font font("calibri.ttf");
			setFont(font);

			setColor(colorWhite);
			drawText(m_pos[0], m_pos[1], 14, 0.f, 0.f, "attacking");

			setColor(255, 0, 0, 63);
			drawRect(
				attackCollision.x1,
				attackCollision.y1,
				attackCollision.x2,
				attackCollision.y2);
		}
	}

	setColor(colorWhite);
}

uint32_t Player::getIntersectingBlocksMaskInternal(int x, int y, bool doWrap) const
{
#if 1
	const int x1 = (x + (int)m_collision.x1) % ARENA_SX_PIXELS;
	const int y1 = (y + (int)m_collision.y1) % ARENA_SY_PIXELS;
	const int x2 = (x + (int)m_collision.x2) % ARENA_SX_PIXELS;
	const int y2 = (y + (int)m_collision.y2) % ARENA_SY_PIXELS;
#else
	const int x1 = x + m_collision.x1;
	const int y1 = y + m_collision.y1;
	const int x2 = x + m_collision.x2;
	const int y2 = y + m_collision.y2;
#endif

	uint32_t result = 0;
	
	result |= g_hostArena->getIntersectingBlocksMask(x1, y1);
	result |= g_hostArena->getIntersectingBlocksMask(x2, y1);
	result |= g_hostArena->getIntersectingBlocksMask(x2, y2);
	result |= g_hostArena->getIntersectingBlocksMask(x1, y2);

	result |= g_hostArena->getIntersectingBlocksMask(x1, (y1 + y2) / 2);
	result |= g_hostArena->getIntersectingBlocksMask(x2, (y1 + y2) / 2);

#if 0
	if (doWrap)
	{
		const int dx = x1 < 0 ? +ARENA_SX_PIXELS : x2 >= ARENA_SX_PIXELS ? -ARENA_SX_PIXELS : 0;
		const int dy = y1 < 0 ? +ARENA_SY_PIXELS : y2 >= ARENA_SY_PIXELS ? -ARENA_SY_PIXELS : 0;

		if ((dx | dy) != 0)
		{
			result |= getIntersectingBlocksMaskInternal(x + dx, y,      false);
			result |= getIntersectingBlocksMaskInternal(x,      y + dy, false);
			result |= getIntersectingBlocksMaskInternal(x + dx, y + dy, false);
		}
	}
#endif

	return result;
}

uint32_t Player::getIntersectingBlocksMask(int x, int y) const
{
	return (getIntersectingBlocksMaskInternal(x, y, true) & m_blockMask);
}

void Player::handleNewGame()
{
	m_state.score = 0;
	m_state.totalScore = 0;
	m_state.SetDirty();
}

void Player::handleNewRound()
{
	if (m_state.score > 0)
		m_state.totalScore += m_state.score;
	m_state.score = 0;
	m_state.SetDirty();

	m_lastSpawnIndex = -1;
	m_isRespawn = false;

	m_weaponAmmo = 0;
}

void Player::respawn()
{
	if (!hasValidCharacterIndex())
		return;

	int x, y;

	if (g_hostArena->getRandomSpawnPoint(x, y, m_lastSpawnIndex))
	{
		m_pos[0] = (float)x;
		m_pos[1] = (float)y;

		m_vel[0] = 0.f;
		m_vel[1] = 0.f;

		m_state.isAlive = true;
		m_state.SetDirty();

		m_anim.SetAnim(kPlayerAnim_Walk, false, true);

		m_weaponAmmo = 0;

		m_attack = AttackInfo();

		m_blockMask = 0;

		m_isGrounded = false;
		m_isAttachedToSticky = false;
		m_isAnimDriven = false;
		m_animVelIsAbsolute = false;
		m_isAirDashCharged = false;
		m_isWallSliding = false;

		if (m_isRespawn)
			playSecondaryEffects(kPlayerEvent_Respawn);
		else
		{
			playSecondaryEffects(kPlayerEvent_Spawn);
			m_isRespawn = true;
		}
	}
}

void Player::handleDamage(float amount, Vec2Arg velocity, Player * attacker)
{
	if (m_state.isAlive)
	{
		if (isAnimOverrideAllowed(kPlayerAnim_Die))
		{
			Vec2 velDelta = velocity - m_vel;

			for (int i = 0; i < 2; ++i)
			{
				if (Calc::Sign(velDelta[i]) == Calc::Sign(velocity[i]))
					m_vel[i] += velDelta[i];
			}

			m_anim.SetAnim(kPlayerAnim_Die, true, true);
			m_isAnimDriven = true;

			if (attacker)
			{
				attacker->awardScore(1);
			}

			// fixme.. mid pos
			ParticleSpawnInfo spawnInfo(m_pos[0], m_pos[1] + mirrorY(-PLAYER_COLLISION_HITBOX_SY/2.f), kBulletType_ParticleA, 10, 50, 200, 20);
			spawnInfo.color = 0xff0000ff;
			g_app->netSpawnParticles(spawnInfo);
		}
	}
}

void Player::awardScore(int score)
{
	m_state.score += score;
	m_state.SetDirty();
}

void Player::setPlayerId(int id)
{
	m_state.playerId = id;
	m_state.SetDirty();
}

void Player::setCharacterIndex(int index)
{
	m_state.characterIndex = index;
	m_state.SetDirty();

	handleCharacterIndexChange();
}

void Player::handleCharacterIndexChange()
{
	if (hasValidCharacterIndex())
	{
		// reload character properties

		m_props.load(makeCharacterFilename("props.txt"));

		delete m_sprite;
		m_sprite = new Sprite(makeCharacterFilename("walk/walk.png"));

		m_spriteScale = m_props.getFloat("sprite_scale", 1.f);

		m_sounds["respawn"].load(m_props.getString("spawn_sounds", ""), true);
	}
}

char * Player::makeCharacterFilename(const char * filename)
{
	static char temp[64];
	sprintf_s(temp, sizeof(temp), "char%d/%s", m_state.characterIndex, filename);
	return temp;
}
