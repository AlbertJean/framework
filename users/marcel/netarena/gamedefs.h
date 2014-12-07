#pragma once

#include "Options.h"

#define BLOCK_SX 64
#define BLOCK_SY 64

#define GFX_SX 1920
#define GFX_SY 1080

#define ARENA_SX (GFX_SX / BLOCK_SX) // 30
#define ARENA_SY (GFX_SY / BLOCK_SY) // 16

OPTION_DECLARE(int, PLAYER_COLLISION_SX, 46);
OPTION_DECLARE(int, PLAYER_COLLISION_SY, 60);

OPTION_DECLARE(int, PLAYER_JUMP_SPEED, 1250);
OPTION_DECLARE(int, PLAYER_WALLJUMP_SPEED, 900);
OPTION_DECLARE(int, PLAYER_WALLJUMP_RECOIL_SPEED, 350);

OPTION_DECLARE(int, PLAYER_SWORD_COLLISION_X1, 15);
OPTION_DECLARE(int, PLAYER_SWORD_COLLISION_Y1, -65);
OPTION_DECLARE(int, PLAYER_SWORD_COLLISION_X2, 100);
OPTION_DECLARE(int, PLAYER_SWORD_COLLISION_Y2, -15);
OPTION_DECLARE(int, PLAYER_SWORD_PUSH_SPEED, 800);

OPTION_DECLARE(int, STEERING_SPEED_ON_GROUND, 550);
OPTION_DECLARE(int, STEERING_SPEED_IN_AIR, 400);

OPTION_DECLARE(int, PLAYER_WALLSLIDE_SPEED, 150);

OPTION_DECLARE(float, FRICTION_GROUNDED, 0.5f);

OPTION_DECLARE(float, GRAVITY, 3500.f);

OPTION_DECLARE(int, BLOCKTYPE_CONVEYOR_SPEED, 275);
OPTION_DECLARE(float, BLOCKTYPE_GRAVITY_REVERSE_MULTIPLIER, -1.f);
OPTION_DECLARE(float, BLOCKTYPE_GRAVITY_STRONG_MULTIPLIER, 2.f);

#define INPUT_BUTTON_UP    (1 << 0)
#define INPUT_BUTTON_DOWN  (1 << 1)
#define INPUT_BUTTON_LEFT  (1 << 2)
#define INPUT_BUTTON_RIGHT (1 << 3)
#define INPUT_BUTTON_A     (1 << 4)
#define INPUT_BUTTON_B     (1 << 5)
#define INPUT_BUTTON_X     (1 << 6)
#define INPUT_BUTTON_Y     (1 << 7)
