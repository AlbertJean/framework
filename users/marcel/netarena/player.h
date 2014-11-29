#pragma once

#include <string>
#include "netobject.h"
#include "Vec2.h"

class Dictionary;
class Sprite;

class PlayerPos_NS : public NetSerializable
{
	virtual void SerializeStruct()
	{
		Serialize(x);
		Serialize(y);

		//

		bool facing;
		
		facing = xFacing < 0 ? true : false;
		Serialize(facing);
		xFacing = facing ? -1 : +1;

		facing = yFacing < 0 ? true : false;
		Serialize(facing);
		yFacing = facing ? -1 : +1;
	}

public:
	PlayerPos_NS(NetSerializableObject * owner)
		: NetSerializable(owner)
		, x(0.f)
		, y(0.f)
		, xFacing(+1)
		, yFacing(+1)
	{
	}

	float & operator[](int index)
	{
		return (index == 0) ? x : y;
	}

	float x;
	float y;

	int xFacing;
	int yFacing;
};

class PlayerState_NS : public NetSerializable
{
	virtual void SerializeStruct()
	{
		Serialize(isAlive);
	}

public:
	PlayerState_NS(NetSerializableObject * owner)
		: NetSerializable(owner)
		, isAlive(false)
	{
	}

	bool isAlive;
};

class PlayerAnim_NS : public NetSerializable
{
	virtual void SerializeStruct();

public:
	PlayerAnim_NS(NetSerializableObject * owner)
		: NetSerializable(owner)
		, m_anim(0)
		, m_play(false)
	{
	}

	void SetAnim(int anim, bool play)
	{
		if (anim != m_anim || play != m_play)
		{
			m_anim = anim;
			m_play = play;
			SetDirty();
		}
	}

	int m_anim;
	bool m_play;
};

class Player : public NetObject
{
	friend class PlayerAnim_NS;

	PlayerPos_NS m_pos;
	Vec2 m_vel;
	PlayerState_NS m_state;
	PlayerAnim_NS m_anim;

	bool m_isAuthorative;

	struct AttackInfo
	{
		AttackInfo()
			: attacking(false)
			, timeLeft(0.f)
			, attackVel()
		{
		}

		bool attacking;
		float timeLeft;
		Vec2 attackVel;
	} m_attack;

	struct
	{
		float x1;
		float y1;
		float x2;
		float y2;
	} m_collision;

	bool m_isGrounded;
	bool m_isAttachedToSticky;

	Sprite * m_sprite;

	static void handleAnimationAction(const std::string & action, const Dictionary & args);

	// ReplicationObject
	virtual bool RequiresUpdating() const { return true; }

	// NetObject
	virtual NetObjectType getType() const { return kNetObjectType_Player; }

public:
	Player(uint16_t owningChannelId = 0);
	~Player();

	void tick(float dt);
	void draw();

	uint32_t getIntersectingBlocksMask(float x, float y) const;

	struct InputState
	{
		InputState()
			: m_prevButtons(0)
			, m_currButtons(0)
		{
		}

		uint16_t m_prevButtons;
		uint16_t m_currButtons;

		bool wasDown(int input) { return (m_prevButtons & input) != 0; }
		bool isDown(int input) { return (m_currButtons & input) != 0; }
		bool wentDown(int input) { return !wasDown(input) && isDown(input); }
		bool wentUp(int input) { return wasDown(input) && !isDown(input); }
		void next() { m_prevButtons = m_currButtons; }
	} m_input;
};
