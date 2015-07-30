#include "Calc.h"
#include "Debugging.h"
#include "framework.h"
#include "gamedefs.h"
#include "gametypes.h"

//#pragma optimize("", off)

Vec2 getTransitionOffset(TransitionType type, float transitionAmount)
{
	switch (type)
	{
	case kTransitionType_None:
		return Vec2(0.f, 0.f);
	case kTransitionType_SlideFromLeft:
		return Vec2(-ARENA_SX_PIXELS * transitionAmount, 0.f);
	case kTransitionType_SlideFromRight:
		return Vec2(+ARENA_SX_PIXELS * transitionAmount, 0.f);
	case kTransitionType_SlideFromTop:
		return Vec2(0.f, -ARENA_SY_PIXELS * transitionAmount);
	case kTransitionType_SlideFromBottom:
		return Vec2(0.f, +ARENA_SY_PIXELS * transitionAmount);
	default:
		Assert(false);
		break;
	}

	return Vec2(0.f, 0.f);
}

void TransitionInfo::setup(TransitionType type, float time, float curve)
{
	m_type = type;
	m_time = time;
	m_curve = curve;
}

void TransitionInfo::parse(const class Dictionary & d)
{
	const std::string type = d.getString("transition_type", "");

	if (type == "none")
		m_type = kTransitionType_None;
	else if (type == "left")
		m_type = kTransitionType_SlideFromLeft;
	else if (type == "right")
		m_type = kTransitionType_SlideFromRight;
	else if (type == "top")
		m_type = kTransitionType_SlideFromTop;
	else if (type == "bottom")
		m_type = kTransitionType_SlideFromBottom;
	else
	{
		logError("unknown transition type: %s", type.c_str());
		m_type = kTransitionType_None;
	}

	m_time = d.getFloat("transition_time", 1.f);
	m_curve = d.getFloat("transition_curve", 1.f);
}

Vec2 TransitionInfo::eval(float transitionTime) const
{
	if (isActiveAtTime(transitionTime))
	{
		const float transitionProgress = std::powf(1.f - transitionTime / m_time, m_curve);

		return getTransitionOffset(m_type, transitionProgress);
	}
	else
	{
		return Vec2(0.f, 0.f);
	}
}

//

const char * g_gameModeNames[kGameMode_COUNT] =
{
	"Lobby",
	"Death Match",
	"Token Hunt",
	"Coin Collector"
};

const char * g_gameStateNames[kGameState_COUNT] =
{
	"mainMenus",
	"connecting",
	"onlineMenus",
	"newGame",
	"roundComplete"
};

const char * g_playerSpecialNames[kPlayerSpecial_COUNT] =
{
	"none",
	"RocketPunch",
	"DoubleSidedMelee",
	"StompAttack",
	"Shield",
	"Invisibility",
	"Jetpack",
	"Zweihander",
	"Axe",
	"Pipebomb",
	"Grapple"
};

Curve defaultCurve(0.f, 1.f);
Curve pipebombBlastCurve;
Curve grenadeBlastCurve;
Curve jetpackAnalogCurveX;
Curve jetpackAnalogCurveY;
Curve gravityWellFalloffCurve;

//

void PlayerInput::gather(bool useKeyboard, int gamepadIndex, bool monkeyMode)
{
	*this = PlayerInput();

	if (useKeyboard)
	{
		if (keyboard.isDown(SDLK_LEFT))
		{
			buttons |= INPUT_BUTTON_LEFT;
			analogX -= PlayerInput::AnalogRange;
		}
		if (keyboard.isDown(SDLK_RIGHT))
		{
			buttons |= INPUT_BUTTON_RIGHT;
			analogX += PlayerInput::AnalogRange;
		}
		if (keyboard.isDown(SDLK_UP))
		{
			buttons |= INPUT_BUTTON_UP;
			analogY -= PlayerInput::AnalogRange;
		}
		if (keyboard.isDown(SDLK_DOWN))
		{
			buttons |= INPUT_BUTTON_DOWN;
			analogY += PlayerInput::AnalogRange;
		}

		if (keyboard.isDown(SDLK_a) || keyboard.isDown(SDLK_SPACE) || keyboard.isDown(SDLK_RETURN))
			buttons |= INPUT_BUTTON_A;
		if (keyboard.isDown(SDLK_s))
			buttons |= INPUT_BUTTON_B;
		if (keyboard.isDown(SDLK_z))
			buttons |= INPUT_BUTTON_X;
		if (keyboard.isDown(SDLK_x))
			buttons |= INPUT_BUTTON_Y;

		if (keyboard.isDown(SDLK_d))
			buttons |= INPUT_BUTTON_START;
		if (keyboard.isDown(SDLK_c))
			buttons |= INPUT_BUTTON_L1;
		if (keyboard.isDown(SDLK_v))
			buttons |= INPUT_BUTTON_R1;

		if (keyboard.isDown(SDLK_h))
			buttons |= INPUT_BUTTON_DPAD_LEFT;
		if (keyboard.isDown(SDLK_k))
			buttons |= INPUT_BUTTON_DPAD_RIGHT;
		if (keyboard.isDown(SDLK_u))
			buttons |= INPUT_BUTTON_DPAD_UP;
		if (keyboard.isDown(SDLK_j))
			buttons |= INPUT_BUTTON_DPAD_DOWN;
	}

	if (gamepadIndex >= 0 && gamepadIndex < MAX_GAMEPAD && gamepad[gamepadIndex].isConnected)
	{
		const Gamepad & g = gamepad[gamepadIndex];

		if (g.isDown(GAMEPAD_START))
			gamepad[gamepadIndex].vibrate(.5f, .5f);

		if (g.isDown(DPAD_LEFT) || g.getAnalog(0, ANALOG_X) < -0.4f)
			buttons |= INPUT_BUTTON_LEFT;
		if (g.isDown(DPAD_RIGHT) || g.getAnalog(0, ANALOG_X) > +0.4f)
			buttons |= INPUT_BUTTON_RIGHT;
		if (g.isDown(DPAD_UP) || g.getAnalog(0, ANALOG_Y) < -0.4f)
			buttons |= INPUT_BUTTON_UP;
		if (g.isDown(DPAD_DOWN) || g.getAnalog(0, ANALOG_Y) > +0.4f)
			buttons |= INPUT_BUTTON_DOWN;

		if (g.isDown(GAMEPAD_A))
			buttons |= INPUT_BUTTON_A;
		if (g.isDown(GAMEPAD_B))
			buttons |= INPUT_BUTTON_B;
		if (g.isDown(GAMEPAD_X))
			buttons |= INPUT_BUTTON_X;
		if (g.isDown(GAMEPAD_Y))
			buttons |= INPUT_BUTTON_Y;

		if (g.isDown(GAMEPAD_START))
			buttons |= INPUT_BUTTON_START;
		if (g.isDown(GAMEPAD_L1))
			buttons |= INPUT_BUTTON_L1;
		if (g.isDown(GAMEPAD_R1))
			buttons |= INPUT_BUTTON_R1;

		if (g.isDown(DPAD_LEFT))
			buttons |= INPUT_BUTTON_DPAD_LEFT;
		if (g.isDown(DPAD_RIGHT))
			buttons |= INPUT_BUTTON_DPAD_RIGHT;
		if (g.isDown(DPAD_UP))
			buttons |= INPUT_BUTTON_DPAD_UP;
		if (g.isDown(DPAD_DOWN))
			buttons |= INPUT_BUTTON_DPAD_DOWN;

		analogX = Calc::Mid((int)(analogX + g.getAnalog(0, ANALOG_X) * PlayerInput::AnalogRange), -PlayerInput::AnalogRange, +PlayerInput::AnalogRange);
		analogY = Calc::Mid((int)(analogY + g.getAnalog(0, ANALOG_Y) * PlayerInput::AnalogRange), -PlayerInput::AnalogRange, +PlayerInput::AnalogRange);
	}

	if (monkeyMode)
	{
		if ((rand() % 2) == 0)
		{
			buttons |= INPUT_BUTTON_LEFT;
			analogX -= PlayerInput::AnalogRange;
		}
		if ((rand() % 10) == 0)
		{
			buttons |= INPUT_BUTTON_RIGHT;
			analogX += PlayerInput::AnalogRange;
		}
		if ((rand() % 10) == 0)
		{
			buttons |= INPUT_BUTTON_UP;
			analogY -= PlayerInput::AnalogRange;
		}
		if ((rand() % 10) == 0)
		{
			buttons |= INPUT_BUTTON_DOWN;
			analogY += PlayerInput::AnalogRange;
		}

		if ((rand() % 60) == 0)
			buttons |= INPUT_BUTTON_A;
		if ((rand() % 60) == 0)
			buttons |= INPUT_BUTTON_B;
		if ((rand() % 60) == 0)
			buttons |= INPUT_BUTTON_X;
		if ((rand() % 60) == 0)
			buttons |= INPUT_BUTTON_Y;

		if ((rand() % 60) == 0)
			buttons |= INPUT_BUTTON_START;
		if ((rand() % 120) == 0)
			buttons |= INPUT_BUTTON_L1;
		if ((rand() % 120) == 0)
			buttons |= INPUT_BUTTON_R1;

		if ((rand() % 600) == 0)
			buttons |= INPUT_BUTTON_DPAD_LEFT;
		if ((rand() % 600) == 0)
			buttons |= INPUT_BUTTON_DPAD_RIGHT;
		if ((rand() % 600) == 0)
			buttons |= INPUT_BUTTON_DPAD_UP;
		if ((rand() % 600) == 0)
			buttons |= INPUT_BUTTON_DPAD_DOWN;
	}
}

//

bool parsePickupType(char c, PickupType & type)
{
	switch (c)
	{
	case 'w': type = kPickupType_Gun; break;
	case 'g': type = kPickupType_Nade; break;
	case 's': type = kPickupType_Shield; break;
	case 'i': type = kPickupType_Ice; break;
	case 'b': type = kPickupType_Bubble; break;
	case 't': type = kPickupType_TimeDilation; break;
	default:
		return false;
	}

	return true;
}

bool parsePickupType(const char * s, PickupType & type)
{
	if (strlen(s) != 1)
		return false;
	else
		return parsePickupType(s[0], type);
}

//

const CollisionShape & CollisionShape::operator=(const CollisionBox & box)
{
	Vec2 p1(box.min[0], box.min[1]);
	Vec2 p2(box.max[0], box.min[1]);
	Vec2 p3(box.max[0], box.max[1]);
	Vec2 p4(box.min[0], box.max[1]);

	set(p1, p2, p3, p4);

	return *this;
}

void CollisionShape::fixup()
{
	Assert(numPoints > 0);
	Vec2 center;
	for (int i = 0; i < numPoints; ++i)
		center += points[i];
	center /= numPoints;
	const Vec2 n = getSegmentNormal(0);
	const float d1 = n * points[0];
	const float d2 = n * center - d1;
	if (d2 > 0.f)
	{
		for (int i = 0; i < numPoints/2; ++i)
			std::swap(points[i], points[numPoints - 1 - i]);
	}
}

void CollisionShape::translate(float x, float y)
{
	for (int i = 0; i < numPoints; ++i)
	{
		points[i][0] += x;
		points[i][1] += y;
	}
}

void CollisionShape::getMinMax(Vec2 & min, Vec2 & max) const
{
	Assert(numPoints != 0);

	min = points[0];
	max = points[0];

	for (int i = 1; i < numPoints; ++i)
	{
		min = min.Min(points[i]);
		max = max.Max(points[i]);
	}
}

float CollisionShape::projectedMax(Vec2Arg n) const
{
	Assert(numPoints != 0);

	float result = n * points[0];

	for (int i = 1; i < numPoints; ++i)
	{
		const float dot = n * points[i];

		if (dot < result)
			result = dot;
	}

	return result;
}

Vec2 CollisionShape::getSegmentNormal(int idx) const
{
	// todo : these could be precomputed

	const Vec2 & p1 = points[(idx + 0) % numPoints];
	const Vec2 & p2 = points[(idx + 1) % numPoints];

	const Vec2 d = (p2 - p1).CalcNormalized();
	const Vec2 n(+d[1], -d[0]);

	return n;
}

bool CollisionShape::intersects(const CollisionShape & other) const
{
	// perform SAT test

	// evaluate edges from the first shape

	for (int i = 0; i < numPoints; ++i)
	{
		const Vec2 & p = points[i];
		const Vec2 pn = getSegmentNormal(i);
		const float pd = pn * p;
		const float d = other.projectedMax(pn) - pd;

		if (d >= 0.f)
			return false;
	}

	// evaluate edges from the second shape

	for (int i = 0; i < other.numPoints; ++i)
	{
		const Vec2 & p = other.points[i];
		const Vec2 pn = other.getSegmentNormal(i);
		const float pd = pn * p;
		const float d = projectedMax(pn) - pd;

		if (d >= 0.f)
			return false;
	}

	return true;
}

bool CollisionShape::checkCollision(const CollisionShape & other, Vec2Arg delta, float & contactDistance, Vec2 & contactNormal) const
{
	float minDistance = -FLT_MAX;
	Vec2 minNormal;
	bool flip = false;
	bool any = false;

	// perform SAT test

	// evaluate edges from the first shape

	for (int i = 0; i < numPoints; ++i)
	{
		const Vec2 & p = points[i];
		const Vec2 pn = getSegmentNormal(i);
		const float pd = pn * p;
		const float d = other.projectedMax(pn) - pd;

		if (d >= 0.f)
			return false;
		else
		{
			// is this a normal in the direction we're checking for?

			if (pn * delta == 0.f)
				continue;

			any = true;

			if (d > minDistance)
			{
				minDistance = d;
				minNormal = pn;
			}
		}
	}

	// evaluate edges from the second shape

	for (int i = 0; i < other.numPoints; ++i)
	{
		const Vec2 & p = other.points[i];
		const Vec2 pn = other.getSegmentNormal(i);
		const float pd = pn * p;
		const float d = projectedMax(pn) - pd;

		if (d >= 0.f)
			return false;
		else
		{
			// is this a normal in the direction we're checking for?

			if (pn * delta == 0.f)
				continue;

			any = true;

			if (d > minDistance)
			{
				minDistance = d;
				minNormal = -pn;

				flip = true;
			}
		}
	}

	if (flip && false)
	{
		minDistance = -minDistance;
		minNormal = -minNormal;
	}

	contactDistance = minDistance;
	contactNormal = minNormal;

	return any;
}

void CollisionShape::debugDraw(bool drawNormals) const
{
	for (int i = 0; i < numPoints; ++i)
	{
		const Vec2 p1 = points[(i + 0) % numPoints];
		const Vec2 p2 = points[(i + 1) % numPoints];
		drawLine(p1[0], p1[1], p2[0], p2[1]);

		if (drawNormals)
		{
			const Vec2 m = (p1 + p2) / 2.f;
			const Vec2 n = getSegmentNormal(i);
			drawLine(m[0], m[1], m[0] + n[0] * BLOCK_SX/2.f, m[1] + n[1] * BLOCK_SX/2.f);
		}
	}
}

//

Curve::Curve()
{
	memset(this, 0, sizeof(Curve));
}

Curve::Curve(float min, float max)
{
	makeLinear(min, max);
}

void Curve::makeLinear(float v1, float v2)
{
	for (int i = 0; i < 32; ++i)
	{
		const float u = i / 31.f;
		const float v = 1.f - u;
		value[i] = v1 * v + v2 * u;
	}
}

float Curve::eval(float t) const
{
	t = t < 0.f ? 0.f : t > 1.f ? 1.f : t;
	const float s = t * 31.f;
	const int index1 = (int)s;
	const int index2 = (index1 + 1) & 31;
	const float u = s - index1;
	const float v = 1.f - u;
	const float v1 = value[index1];
	const float v2 = value[index2];
	return v1 * v + v2 * u;
}
