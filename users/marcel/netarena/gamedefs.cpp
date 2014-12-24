#include "gamedefs.h"

OPTION_DEFINE(int, PLAYER_COLLISION_HITBOX_SX, "Player/Hit Boxes/Collision Width");
OPTION_DEFINE(int, PLAYER_COLLISION_HITBOX_SY, "Player/Hit Boxes/Collision Height");

OPTION_DEFINE(int, PLAYER_DAMAGE_HITBOX_SX, "Player/Hit Boxes/Damage Width");
OPTION_DEFINE(int, PLAYER_DAMAGE_HITBOX_SY, "Player/Hit Boxes/Damage Height");

OPTION_DEFINE(int, PLAYER_JUMP_SPEED, "Player/Jumping/Speed");
OPTION_DEFINE(int, PLAYER_WALLJUMP_SPEED, "Player/Wall Jump/Speed");
OPTION_DEFINE(int, PLAYER_WALLJUMP_RECOIL_SPEED, "Player/Wall Jump/Recoil Speed");
OPTION_STEP(PLAYER_JUMP_SPEED, 0, 0, 10);
OPTION_STEP(PLAYER_WALLJUMP_SPEED, 0, 0, 10);
OPTION_STEP(PLAYER_WALLJUMP_RECOIL_SPEED, 0, 0, 10);

OPTION_DEFINE(int, PLAYER_SWORD_COLLISION_X1, "Player/Attacks/Sword/Collision X1");
OPTION_DEFINE(int, PLAYER_SWORD_COLLISION_Y1, "Player/Attacks/Sword/Collision Y1");
OPTION_DEFINE(int, PLAYER_SWORD_COLLISION_X2, "Player/Attacks/Sword/Collision X2");
OPTION_DEFINE(int, PLAYER_SWORD_COLLISION_Y2, "Player/Attacks/Sword/Collision Y2");
OPTION_DEFINE(int, PLAYER_SWORD_PUSH_SPEED, "Player/Attacks/Sword/Push Speed");
OPTION_STEP(PLAYER_SWORD_PUSH_SPEED, 0, 0, 50);

OPTION_DEFINE(int, STEERING_SPEED_ON_GROUND, "Player/Steering/Speed On Ground");
OPTION_DEFINE(int, STEERING_SPEED_IN_AIR, "Player/Steering/Speed In Air");
OPTION_STEP(STEERING_SPEED_ON_GROUND, 0, 0, 10);
OPTION_STEP(STEERING_SPEED_IN_AIR, 0, 0, 10);

OPTION_DEFINE(int, PLAYER_WALLSLIDE_SPEED, "Player/Wall Slide/Speed");
OPTION_STEP(PLAYER_WALLSLIDE_SPEED, 0, 0, 10);

OPTION_DEFINE(float, FRICTION_GROUNDED, "Physics/Friction/On Ground");
OPTION_STEP(FRICTION_GROUNDED, 0, 0, 0.01f);

OPTION_DEFINE(float, GRAVITY, "Physics/Gravity");
OPTION_STEP(GRAVITY, 0, 10000, 50);

// block types

OPTION_DEFINE(int, BLOCKTYPE_CONVEYOR_SPEED, "Blocks/Conveyor Speed");
OPTION_STEP(BLOCKTYPE_CONVEYOR_SPEED, 0, 10000, 10);

OPTION_DEFINE(float, BLOCKTYPE_GRAVITY_REVERSE_MULTIPLIER, "Blocks/Gravity Reverse");
OPTION_STEP(BLOCKTYPE_GRAVITY_REVERSE_MULTIPLIER, -10.f, 0.f, 0.05f);

OPTION_DEFINE(float, BLOCKTYPE_GRAVITY_STRONG_MULTIPLIER, "Blocks/Gravity Strong");
OPTION_STEP(BLOCKTYPE_GRAVITY_STRONG_MULTIPLIER, 0.f, 10.f, 0.05f);

OPTION_DEFINE(int, BLOCKTYPE_SPRING_SPEED, "Blocks/Spring Speed");
OPTION_STEP(BLOCKTYPE_SPRING_SPEED, 0, 0, 10);

// bullets

OPTION_DEFINE(int,  BULLET_TYPE0_MAX_WRAP_COUNT, "Bullets/Type 0/Max Wrap Count");
OPTION_DEFINE(int,  BULLET_TYPE0_MAX_REFLECT_COUNT, "Bullets/Type 0/Max Reflect Count");
OPTION_DEFINE(int,  BULLET_TYPE0_MAX_DISTANCE_TRAVELLED, "Bullets/Type 0/Max Distance Travelled");
OPTION_DEFINE(int,  BULLET_TYPE0_SPEED, "Bullets/Type 0/Speed");
OPTION_STEP(BULLET_TYPE0_MAX_DISTANCE_TRAVELLED, 0.f, 10000.f, 100.f);
OPTION_STEP(BULLET_TYPE0_SPEED, 0.f, 10000.f, 10.f);

OPTION_DEFINE(int, BULLET_GRENADE_NADE_SPEED, "Bullets/Grenade/Nade Speed");
OPTION_DEFINE(int, BULLET_GRENADE_NADE_BOUNCE_COUNT, "Bullets/Grenade/Nade Bounce Count");
OPTION_DEFINE(float, BULLET_GRENADE_NADE_BOUNCE_AMOUNT, "Bullets/Grenade/Nade Bounce Amount");
OPTION_DEFINE(float, BULLET_GRENADE_NADE_LIFE, "Bullets/Grenade/Nade Life");
OPTION_DEFINE(int, BULLET_GRENADE_FRAG_RADIUS_MIN, "Bullets/Grenade/Frag Radius Min");
OPTION_DEFINE(int, BULLET_GRENADE_FRAG_RADIUS_MAX, "Bullets/Grenade/Frag Radius Max");
OPTION_DEFINE(int, BULLET_GRENADE_FRAG_SPEED_MIN, "Bullets/Grenade/Frag Speed Min");
OPTION_DEFINE(int, BULLET_GRENADE_FRAG_SPEED_MAX, "Bullets/Grenade/Frag Speed Max");
