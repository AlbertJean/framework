#pragma once

#include "Options.h"

#define TICKS_PER_SECOND 60

#define BLOCK_SX 64
#define BLOCK_SY 64

#define GFX_SX 1680
#define GFX_SY 1050

#define ARENA_SX (GFX_SX / BLOCK_SX) // 26
#define ARENA_SY (GFX_SY / BLOCK_SY) // 16

#define ARENA_SX_PIXELS (ARENA_SX * BLOCK_SX)
#define ARENA_SY_PIXELS (ARENA_SY * BLOCK_SY)

OPTION_DECLARE(int, PLAYER_COLLISION_HITBOX_SX, 46);
OPTION_DECLARE(int, PLAYER_COLLISION_HITBOX_SY, 60);

OPTION_DECLARE(int, PLAYER_DAMAGE_HITBOX_SX, 46);
OPTION_DECLARE(int, PLAYER_DAMAGE_HITBOX_SY, 70);

OPTION_DECLARE(int, PLAYER_JUMP_SPEED, 1250);
OPTION_DECLARE(int, PLAYER_JUMP_GRACE_PIXELS, 70);
OPTION_DECLARE(int, PLAYER_WALLJUMP_SPEED, 900);
OPTION_DECLARE(int, PLAYER_WALLJUMP_RECOIL_SPEED, 350);
OPTION_DECLARE(float, PLAYER_WALLJUMP_RECOIL_TIME, 0.25f);

OPTION_DECLARE(int, PLAYER_SWORD_COLLISION_X1, 15);
OPTION_DECLARE(int, PLAYER_SWORD_COLLISION_Y1, -65);
OPTION_DECLARE(int, PLAYER_SWORD_COLLISION_X2, 100);
OPTION_DECLARE(int, PLAYER_SWORD_COLLISION_Y2, -15);
OPTION_DECLARE(float, PLAYER_SWORD_COOLDOWN, 0.f);
OPTION_DECLARE(int, PLAYER_SWORD_PUSH_SPEED, 800);
OPTION_DECLARE(int, PLAYER_SWORD_CLING_SPEED, 500);
OPTION_DECLARE(float, PLAYER_SWORD_CLING_TIME, 0.25f);

OPTION_DECLARE(float, PLAYER_FIRE_COOLDOWN, 0.4f);

OPTION_DECLARE(int, STEERING_SPEED_ON_GROUND, 550);
OPTION_DECLARE(int, STEERING_SPEED_IN_AIR, 400);

OPTION_DECLARE(int, PLAYER_WALLSLIDE_SPEED, 150);

OPTION_DECLARE(float, FRICTION_GROUNDED, 0.5f);

OPTION_DECLARE(float, GRAVITY, 3500.f);

OPTION_DECLARE(int, BLOCKTYPE_CONVEYOR_SPEED, 275);
OPTION_DECLARE(float, BLOCKTYPE_GRAVITY_REVERSE_MULTIPLIER, -1.f);
OPTION_DECLARE(float, BLOCKTYPE_GRAVITY_STRONG_MULTIPLIER, 2.f);
OPTION_DECLARE(int, BLOCKTYPE_SPRING_SPEED, 800);

OPTION_DECLARE(int,  BULLET_TYPE0_MAX_WRAP_COUNT, 1);
OPTION_DECLARE(int,  BULLET_TYPE0_MAX_REFLECT_COUNT, 0);
OPTION_DECLARE(int,  BULLET_TYPE0_MAX_DISTANCE_TRAVELLED, ARENA_SX * BLOCK_SX);
OPTION_DECLARE(int,  BULLET_TYPE0_SPEED, 1600);

OPTION_DECLARE(int, BULLET_GRENADE_NADE_SPEED, 1200);
OPTION_DECLARE(int, BULLET_GRENADE_NADE_BOUNCE_COUNT, 3);
OPTION_DECLARE(float, BULLET_GRENADE_NADE_BOUNCE_AMOUNT, 0.4f);
OPTION_DECLARE(float, BULLET_GRENADE_NADE_LIFE, 2.5f);
OPTION_DECLARE(float, BULLET_GRENADE_NADE_LIFE_AFTER_SETTLE, 0.8f);
OPTION_DECLARE(int, BULLET_GRENADE_FRAG_COUNT, 20);
OPTION_DECLARE(int, BULLET_GRENADE_FRAG_RADIUS_MIN, 80);
OPTION_DECLARE(int, BULLET_GRENADE_FRAG_RADIUS_MAX, 80);
OPTION_DECLARE(int, BULLET_GRENADE_FRAG_SPEED_MIN, 100);
OPTION_DECLARE(int, BULLET_GRENADE_FRAG_SPEED_MAX, 200);

OPTION_DECLARE(int, PICKUP_AMMO_COUNT, 3);
OPTION_DECLARE(int, PICKUP_AMMO_WEIGHT, 10);
OPTION_DECLARE(int, PICKUP_NADE_WEIGHT, 10);

#define INPUT_BUTTON_UP       (1 << 0)
#define INPUT_BUTTON_DOWN     (1 << 1)
#define INPUT_BUTTON_LEFT     (1 << 2)
#define INPUT_BUTTON_RIGHT    (1 << 3)
#define INPUT_BUTTON_A        (1 << 4)
#define INPUT_BUTTON_B        (1 << 5)
#define INPUT_BUTTON_X        (1 << 6)
#define INPUT_BUTTON_Y        (1 << 7)
#define INPUT_BUTTON_START    (1 << 8)
