#include "gamedefs.h"

OPTION_DEFINE(int, PLAYER_COLLISION_SX, "Player/Collision/Width");
OPTION_DEFINE(int, PLAYER_COLLISION_SY, "Player/Collision/Height");

OPTION_DEFINE(int, PLAYER_JUMP_SPEED, "Player/Jumping/Speed");
OPTION_DEFINE(int, PLAYER_WALLJUMP_SPEED, "Player/Wall Jump/Speed");
OPTION_DEFINE(int, PLAYER_WALLJUMP_RECOIL_SPEED, "Player/Wall Jump/Recoil Speed");

OPTION_DEFINE(int, PLAYER_SWORD_COLLISION_X1, "Player/Attacks/Sword/Collision X1");
OPTION_DEFINE(int, PLAYER_SWORD_COLLISION_Y1, "Player/Attacks/Sword/Collision Y1");
OPTION_DEFINE(int, PLAYER_SWORD_COLLISION_X2, "Player/Attacks/Sword/Collision X2");
OPTION_DEFINE(int, PLAYER_SWORD_COLLISION_Y2, "Player/Attacks/Sword/Collision Y2");
OPTION_DEFINE(int, PLAYER_SWORD_PUSH_SPEED, "Player/Attacks/Sword/Push Speed");
OPTION_STEP(PLAYER_SWORD_PUSH_SPEED, 0, 0, 50);

OPTION_DEFINE(int, STEERING_SPEED_ON_GROUND, "Player/Steering/Speed On Ground");
OPTION_DEFINE(int, STEERING_SPEED_IN_AIR, "Player/Steering/Speed In Air");

OPTION_DEFINE(int, PLAYER_WALLSLIDE_SPEED, "Player/Wall Slide/Speed");

OPTION_DEFINE(float, FRICTION_GROUNDED, "Physics/Friction/On Ground");

OPTION_DEFINE(float, GRAVITY, "Physics/Gravity");
OPTION_STEP(GRAVITY, 0, 10000, 50);

// block types

OPTION_DEFINE(int, BLOCKTYPE_CONVEYOR_SPEED, "Blocks/Conveyor Speed");
OPTION_STEP(BLOCKTYPE_CONVEYOR_SPEED, 0, 10000, 10);

OPTION_DEFINE(float, BLOCKTYPE_GRAVITY_REVERSE_MULTIPLIER, "Blocks/Gravity Reverse");
OPTION_STEP(BLOCKTYPE_GRAVITY_REVERSE_MULTIPLIER, -10.f, 0.f, 0.05f);

OPTION_DEFINE(float, BLOCKTYPE_GRAVITY_STRONG_MULTIPLIER, "Blocks/Gravity Strong");
OPTION_STEP(BLOCKTYPE_GRAVITY_STRONG_MULTIPLIER, 0.f, 10.f, 0.05f);
