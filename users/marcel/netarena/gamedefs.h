#pragma once

#include "Options.h"

#if 1
	#define DEBUG_RANDOM_CALLSITES 0
	#define ENABLE_GAMESTATE_DESYNC_DETECTION 1
	#define ENABLE_GAMESTATE_CRC_LOGGING (ENABLE_GAMESTATE_DESYNC_DETECTION && 1)
#else
	#define DEBUG_RANDOM_CALLSITES 0 // do not alter
	#define ENABLE_GAMESTATE_DESYNC_DETECTION 0 // do not alter
	#define ENABLE_GAMESTATE_CRC_LOGGING 0 // do not alter
#endif

#define TICKS_PER_SECOND 60

#define USE_32X32_TILES 1

#if USE_32X32_TILES
	#define BLOCK_SX 30
	#define BLOCK_SY 30
	#define PICKUP_BLOCK_SX 3
	#define PICKUP_BLOCK_SY 3
#else
	#define BLOCK_SX 64
	#define BLOCK_SY 64
	#define PICKUP_BLOCK_SX 1
	#define PICKUP_BLOCK_SY 1
#endif

#define GFX_SX 1920
#define GFX_SY 1080

#define NET_PORT 30000

#define ARENA_SX (GFX_SX / BLOCK_SX) // 30
#define ARENA_SY (GFX_SY / BLOCK_SY) // 16

#define ARENA_SX_PIXELS (ARENA_SX * BLOCK_SX)
#define ARENA_SY_PIXELS (ARENA_SY * BLOCK_SY)

#define MAX_PLAYERS 4
#define MAX_PLAYER_DISPLAY_NAME 12

#define MAX_CHARACTERS 8

#define MAX_TIMEDILATION_EFFECTS 8

#define MAX_WEAPON_STACK_SIZE 5
#define MAX_PICKUPS 10
#define MAX_MOVERS 10
#define MAX_TORCHES 10
#define MAX_SCREEN_SHAKES 4
#define MAX_COINS 30
#define MAX_AXES (MAX_PLAYERS * 2)
#define MAX_PIPEBOMBS 10
#define MAX_BARRELS 10
#define MAX_FLOOR_EFFECT_TILES 20
#define MAX_ANIM_EFFECTS 20
#define MAX_FIREBALLS 16

// demo mode
OPTION_DECLARE(bool, DEMOMODE, false);

// -- prototypes --
OPTION_DECLARE(bool, PROTO_TIMEDILATION_ON_KILL, true);
OPTION_DECLARE(float, PROTO_TIMEDILATION_ON_KILL_MULTIPLIER1, .3f);
OPTION_DECLARE(float, PROTO_TIMEDILATION_ON_KILL_MULTIPLIER2, .5f);
OPTION_DECLARE(float, PROTO_TIMEDILATION_ON_KILL_DURATION, .25f);

OPTION_DECLARE(bool, PROTO_ENABLE_LEVEL_EVENTS, true);
OPTION_DECLARE(int, PROTO_LEVEL_EVENT_INTERVAL, 60);

OPTION_DECLARE(float, EVENT_GRAVITYWELL_STRENGTH_BEGIN, 70.f);
OPTION_DECLARE(float, EVENT_GRAVITYWELL_STRENGTH_END, 40.f);
OPTION_DECLARE(float, EVENT_GRAVITYWELL_DURATION, 7.f);

OPTION_DECLARE(float, EVENT_EARTHQUAKE_DURATION, 6.f);
OPTION_DECLARE(float, EVENT_EARTHQUAKE_INTERVAL, 1.f);
OPTION_DECLARE(float, EVENT_EARTHQUAKE_INTERVAL_RAND, 1.f);
OPTION_DECLARE(float, EVENT_EARTHQUAKE_PLAYER_BOOST, -350.f);

OPTION_DECLARE(float, EVENT_TIMEDILATION_DURATION, 8.f);
OPTION_DECLARE(float, EVENT_TIMEDILATION_MULTIPLIER_BEGIN, .5f);
OPTION_DECLARE(float, EVENT_TIMEDILATION_MULTIPLIER_END, .25f);

OPTION_DECLARE(int, SPIKEWALLS_COVERAGE, 65);
OPTION_DECLARE(float, SPIKEWALLS_TIME_PREVIEW, 2.f);
OPTION_DECLARE(float, SPIKEWALLS_TIME_CLOSE, 3.f);
OPTION_DECLARE(float, SPIKEWALLS_TIME_CLOSED, 3.f);
OPTION_DECLARE(float, SPIKEWALLS_TIME_OPEN, 3.f);
OPTION_DECLARE(bool, SPIKEWALLS_OPEN_ON_LAST_MAN_STANDING, true);
// -- prototypes --

OPTION_DECLARE(int, GAMESTATE_COMPLETE_TIMER, 5);
OPTION_DECLARE(int, GAMESTATE_COMPLETE_TIME_DILATION_TIMER, 3);
OPTION_DECLARE(float, GAMESTATE_COMPLETE_TIME_DILATION_BEGIN, .4f);
OPTION_DECLARE(float, GAMESTATE_COMPLETE_TIME_DILATION_END, .0f);

OPTION_DECLARE(float, GAME_SPEED_MULTIPLIER, 1.f);

OPTION_DECLARE(float, PLAYER_RESPAWN_INVINCIBILITY_TIME, 1.3f);

OPTION_DECLARE(int, PLAYER_DAMAGE_HITBOX_SX, 46);
OPTION_DECLARE(int, PLAYER_DAMAGE_HITBOX_SY, 70);

OPTION_DECLARE(float, PLAYER_SHIELD_IMPACT_MULTIPLIER, 1.f);

OPTION_DECLARE(int, PLAYER_SPEED_MAX, 1500);
OPTION_DECLARE(int, PLAYER_JUMP_SPEED, 1250);
OPTION_DECLARE(int, PLAYER_JUMP_SPEED_FRAMES, 10);
OPTION_DECLARE(int, PLAYER_JUMP_GRACE_PIXELS, 70);
OPTION_DECLARE(int, PLAYER_DOUBLE_JUMP_SPEED, 800);
OPTION_DECLARE(int, PLAYER_WALLJUMP_SPEED, 900);
OPTION_DECLARE(int, PLAYER_WALLJUMP_RECOIL_SPEED, 350);
OPTION_DECLARE(float, PLAYER_WALLJUMP_RECOIL_TIME, 0.25f);

OPTION_DECLARE(float, PLAYER_SCREENSHAKE_STRENGTH_THRESHHOLD, 18.0f);

OPTION_DECLARE(int, PLAYER_SWORD_COLLISION_X1, 15);
OPTION_DECLARE(int, PLAYER_SWORD_COLLISION_Y1, -65);
OPTION_DECLARE(int, PLAYER_SWORD_COLLISION_X2, 100);
OPTION_DECLARE(int, PLAYER_SWORD_COLLISION_Y2, -15);
OPTION_DECLARE(float, PLAYER_SWORD_COOLDOWN, 0.f);
OPTION_DECLARE(int, PLAYER_SWORD_PUSH_SPEED, 800);
OPTION_DECLARE(int, PLAYER_SWORD_CLING_SPEED, 500);
OPTION_DECLARE(float, PLAYER_SWORD_CLING_TIME, 0.25f);
OPTION_DECLARE(bool, PLAYER_SWORD_SINGLE_BLOCK, false);
OPTION_DECLARE(float, PLAYER_SWORD_BLOCK_DESTROY_SLOWDOWN, 0.5f);

OPTION_DECLARE(float, MULTIKILL_TIMER, 1.5f);
OPTION_DECLARE(int, KILLINGSPREE_START, 3);
OPTION_DECLARE(int, KILLINGSPREE_UNSTOPPABLE, 6);

OPTION_DECLARE(float, PLAYER_FIRE_COOLDOWN, 0.4f);

OPTION_DECLARE(float, PLAYER_EFFECT_ICE_TIME, 5.f);
OPTION_DECLARE(float, PLAYER_EFFECT_ICE_IMPACT_MULTIPLIER, .1f);
OPTION_DECLARE(float, PLAYER_EFFECT_BUBBLE_TIME, 5.f);
OPTION_DECLARE(float, PLAYER_EFFECT_BUBBLE_SPEED, 200.f);
OPTION_DECLARE(float, PLAYER_EFFECT_TIMEDILATION_TIME, 4.f);
OPTION_DECLARE(float, PLAYER_EFFECT_TIMEDILATION_MULTIPLIER, .3f);
OPTION_DECLARE(bool, PLAYER_EFFECT_TIMEDILATION_ON_OTHERS, true); // if true, effect is applied on other. otherwise, own player speed is increased

OPTION_DECLARE(int, STEERING_SPEED_ON_GROUND, 550);
OPTION_DECLARE(int, STEERING_SPEED_IN_AIR, 400);
OPTION_DECLARE(int, STEERING_SPEED_JETPACK, 300);
OPTION_DECLARE(int, STEERING_SPEED_DOUBLEMELEE, 100);
OPTION_DECLARE(int, STEERING_SPEED_ZWEIHANDER, 1000);
OPTION_DECLARE(int, STEERING_SPEED_GRAPPLE, 200);

OPTION_DECLARE(int, PLAYER_WALLSLIDE_SPEED, 150);
OPTION_DECLARE(int, PLAYER_WALLSLIDE_FX_INTERVAL, 16);

OPTION_DECLARE(float, FRICTION_GROUNDED, 0.5f);

OPTION_DECLARE(float, GRAVITY, 3500.f);

OPTION_DECLARE(int, BLOCKTYPE_CONVEYOR_SPEED, 275);
OPTION_DECLARE(float, BLOCKTYPE_GRAVITY_REVERSE_MULTIPLIER, -1.f);
OPTION_DECLARE(float, BLOCKTYPE_GRAVITY_STRONG_MULTIPLIER, 2.f);
OPTION_DECLARE(int, BLOCKTYPE_SPRING_SPEED, 800);
OPTION_DECLARE(float, BLOCKTYPE_REGEN_TIME, 10.f);

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

#ifdef DEBUG
OPTION_DECLARE(int, PICKUP_INTERVAL, 1);
OPTION_DECLARE(int, PICKUP_INTERVAL_VARIANCE, 1);
#else
OPTION_DECLARE(int, PICKUP_INTERVAL, 10);
OPTION_DECLARE(int, PICKUP_INTERVAL_VARIANCE, 5);
#endif
OPTION_DECLARE(int, MAX_PICKUP_COUNT, 5);
OPTION_DECLARE(float, PICKUP_RATE_MULTIPLIER_1, 1.f);
OPTION_DECLARE(float, PICKUP_RATE_MULTIPLIER_2, 1.f);
OPTION_DECLARE(float, PICKUP_RATE_MULTIPLIER_3, 1.5f);
OPTION_DECLARE(float, PICKUP_RATE_MULTIPLIER_4, 2.f);

OPTION_DECLARE(int, PICKUP_AMMO_COUNT, 3);
OPTION_DECLARE(int, PICKUP_AMMO_WEIGHT, 10);
OPTION_DECLARE(int, PICKUP_NADE_WEIGHT, 10);
OPTION_DECLARE(int, PICKUP_SHIELD_COUNT, 1);
OPTION_DECLARE(int, PICKUP_SHIELD_WEIGHT, 10);
OPTION_DECLARE(int, PICKUP_ICE_COUNT, 3);
OPTION_DECLARE(int, PICKUP_ICE_WEIGHT, 10);
OPTION_DECLARE(int, PICKUP_BUBBLE_COUNT, 1);
OPTION_DECLARE(int, PICKUP_BUBBLE_WEIGHT, 10);
OPTION_DECLARE(int, PICKUP_TIMEDILATION_WEIGHT, 10);

// fireballs

OPTION_DECLARE(float, FIREBALL_SPEED, 1250.0f);

// death match

OPTION_DECLARE(int, DEATHMATCH_SCORE_LIMIT, 10);

// token hunt

OPTION_DECLARE(int, TOKENHUNT_SCORE_LIMIT, 10);

OPTION_DECLARE(int, TOKEN_FLEE_SPEED, 1000);
OPTION_DECLARE(float, TOKEN_DROP_TIME, 0.25f);
OPTION_DECLARE(float, TOKEN_DROP_SPEED_MULTIPLIER, 1.f);
OPTION_DECLARE(float, TOKEN_BOUNCINESS, .5f);
OPTION_DECLARE(int, TOKEN_BOUNCE_SOUND_TRESHOLD, 50);

// coin collector

OPTION_DECLARE(bool, COINCOLLECTOR_PLAYER_CAN_BE_KILLED, true);
OPTION_DECLARE(int, COINCOLLECTOR_INITIAL_COIN_DROP, 3);
OPTION_DECLARE(int, COINCOLLECTOR_SCORE_LIMIT, 15);
OPTION_DECLARE(int, COINCOLLECTOR_COIN_DROP_PERCENTAGE, 34);
OPTION_DECLARE(int, COINCOLLECTOR_COIN_LIMIT, 25);

OPTION_DECLARE(int, COIN_SPAWN_INTERVAL, 4);
OPTION_DECLARE(int, COIN_SPAWN_INTERVAL_VARIANCE, 2);
OPTION_DECLARE(int, COIN_FLEE_SPEED, 1000);
OPTION_DECLARE(int, COIN_DROP_SPEED, 700);
OPTION_DECLARE(float, COIN_DROP_TIME, 1.f);
OPTION_DECLARE(float, COIN_DROP_SPEED_MULTIPLIER, .5f);
OPTION_DECLARE(float, COIN_BOUNCINESS, .5f);
OPTION_DECLARE(int, COIN_BOUNCE_SOUND_TRESHOLD, 50);

OPTION_DECLARE(float, TORCH_FLICKER_STRENGTH, .5f);
OPTION_DECLARE(int, TORCH_FLICKER_FREQ_A, 3);
OPTION_DECLARE(int, TORCH_FLICKER_FREQ_B, 5);
OPTION_DECLARE(int, TORCH_FLICKER_FREQ_C, 7);
OPTION_DECLARE(int, TORCH_FLICKER_Y_OFFSET, -35);

OPTION_DECLARE(int, LIGHTING_DEBUG_MODE, 0);

OPTION_DECLARE(float, JETPACK_ACCEL, 1800.f);
OPTION_DECLARE(float, JETPACK_FX_INTERVAL, .1f);

OPTION_DECLARE(int, DOUBLEMELEE_ATTACK_RADIUS, 150);
OPTION_DECLARE(int, DOUBLEMELEE_SPIN_COUNT, 5);
OPTION_DECLARE(float, DOUBLEMELEE_SPIN_TIME, .1f);
OPTION_DECLARE(float, DOUBLEMELEE_GRAVITY_MULTIPLIER, .2f);

OPTION_DECLARE(int, STOMP_EFFECT_MAX_SIZE, 5);
OPTION_DECLARE(int, STOMP_EFFECT_MIN_HEIGHT, 100);
OPTION_DECLARE(int, STOMP_EFFECT_MAX_HEIGHT, 600);
OPTION_DECLARE(float, STOMP_EFFECT_PROPAGATION_INTERVAL, .1f);
OPTION_DECLARE(float, STOMP_EFFECT_DAMAGE_DURATION, .25f);
OPTION_DECLARE(int, STOMP_EFFECT_DAMAGE_HITBOX_SX, 64);
OPTION_DECLARE(int, STOMP_EFFECT_DAMAGE_HITBOX_SY, 80);

OPTION_DECLARE(float, ROCKETPUNCH_CHARGE_MIN, 0.f);
OPTION_DECLARE(float, ROCKETPUNCH_CHARGE_MAX, 0.4f);
OPTION_DECLARE(bool, ROCKETPUNCH_CHARGE_MUST_BE_MAXED, true);
OPTION_DECLARE(bool, ROCKETPUNCH_AUTO_DISCHARGE, true);
OPTION_DECLARE(int, ROCKETPUNCH_SPEED_MIN, 1200);
OPTION_DECLARE(int, ROCKETPUNCH_SPEED_MAX, 1200);
OPTION_DECLARE(bool, ROCKETPUNCH_SPEED_BASED_ON_CHARGE, true);
OPTION_DECLARE(int, ROCKETPUNCH_DISTANCE_MIN, 600);
OPTION_DECLARE(int, ROCKETPUNCH_DISTANCE_MAX, 600);
OPTION_DECLARE(bool, ROCKETPUNCH_DISTANCE_BASED_ON_CHARGE, true);
OPTION_DECLARE(bool, ROCKETPUNCH_MUST_GROUND, true);
OPTION_DECLARE(bool, ROCKETPUNCH_ONLY_IN_AIR, false);
OPTION_DECLARE(float, ROCKETPUNCH_STUN_TIME, 1.5f);

OPTION_DECLARE(float, ZWEIHANDER_CHARGE_TIME, 5.f);
OPTION_DECLARE(float, ZWEIHANDER_STUN_TIME, 2.f);
OPTION_DECLARE(int, ZWEIHANDER_STOMP_EFFECT_SIZE, 7);

OPTION_DECLARE(float, PIPEBOMB_PLAYER_SPEED_MULTIPLIER, .5f);
OPTION_DECLARE(float, PIPEBOMB_THROW_SPEED, 200.f);
OPTION_DECLARE(float, PIPEBOMB_ACTIVATION_TIME, .2f);
OPTION_DECLARE(float, PIPEBOMB_BLAST_RADIUS, 200.f);
OPTION_DECLARE(float, PIPEBOMB_BLAST_STRENGTH_NEAR, 500.f);
OPTION_DECLARE(float, PIPEBOMB_BLAST_STRENGTH_FAR, 200.f);

OPTION_DECLARE(float, AXE_THROW_SPEED, 2000);
OPTION_DECLARE(float, AXE_FADE_TIME, 6.f);

OPTION_DECLARE(float, GRAPPLE_LENGTH_MIN, 50.f);
OPTION_DECLARE(float, GRAPPLE_LENGTH_MAX, 500.f);
OPTION_DECLARE(float, GRAPPLE_PULL_UP_SPEED, 300.f);
OPTION_DECLARE(float, GRAPPLE_PULL_DOWN_SPEED, 100.f);

OPTION_DECLARE(int, UI_CHARSELECT_BASE_X, 170);
OPTION_DECLARE(int, UI_CHARSELECT_BASE_Y, 250);
OPTION_DECLARE(int, UI_CHARSELECT_SIZE_X, 310);
OPTION_DECLARE(int, UI_CHARSELECT_SIZE_Y, 400);
OPTION_DECLARE(int, UI_CHARSELECT_STEP_X, 340);
OPTION_DECLARE(int, UI_CHARSELECT_PREV_X, 45);
OPTION_DECLARE(int, UI_CHARSELECT_PREV_Y, 300);
OPTION_DECLARE(int, UI_CHARSELECT_NEXT_X, 265);
OPTION_DECLARE(int, UI_CHARSELECT_NEXT_Y, 300);
OPTION_DECLARE(int, UI_CHARSELECT_READY_X, 155);
OPTION_DECLARE(int, UI_CHARSELECT_READY_Y, 350);
OPTION_DECLARE(int, UI_CHARSELECT_PORTRAIT_X, 155);
OPTION_DECLARE(int, UI_CHARSELECT_PORTRAIT_Y, 270);

OPTION_DECLARE(int, UI_TEXTCHAT_MAX_TEXT_SIZE, 20);
OPTION_DECLARE(int, UI_TEXTCHAT_X, 50);
OPTION_DECLARE(int, UI_TEXTCHAT_Y, GFX_SY - 100);
OPTION_DECLARE(int, UI_TEXTCHAT_SX, 750);
OPTION_DECLARE(int, UI_TEXTCHAT_SY, 60);
OPTION_DECLARE(int, UI_TEXTCHAT_PADDING_X, 10);
OPTION_DECLARE(int, UI_TEXTCHAT_PADDING_Y, 0);
OPTION_DECLARE(int, UI_TEXTCHAT_FONT_SIZE, 48);
OPTION_DECLARE(int, UI_TEXTCHAT_CARET_SX, 4);
OPTION_DECLARE(int, UI_TEXTCHAT_CARET_PADDING_X, 5);
OPTION_DECLARE(int, UI_TEXTCHAT_CARET_PADDING_Y, 5);

OPTION_DECLARE(float, UI_QUICKLOOK_OPEN_TIME, .15f);
OPTION_DECLARE(float, UI_QUICKLOOK_CLOSE_TIME, .15f);

OPTION_DECLARE(int, INGAME_TEXTCHAT_FONT_SIZE, 24);
OPTION_DECLARE(int, INGAME_TEXTCHAT_PADDING_X, 7);
OPTION_DECLARE(int, INGAME_TEXTCHAT_PADDING_Y, 2);
OPTION_DECLARE(int, INGAME_TEXTCHAT_SIZE_Y, 32);
OPTION_DECLARE(int, INGAME_TEXTCHAT_OFFSET_Y, -150);
OPTION_DECLARE(float, INGAME_TEXTCHAT_DURATION, 4.f);

OPTION_DECLARE(bool, UI_PLAYER_OUTLINE, false);
OPTION_DECLARE(int, UI_PLAYER_OUTLINE_ALPHA, 30);
OPTION_DECLARE(int, UI_PLAYER_BACKDROP_ALPHA, 20);

OPTION_DECLARE(bool, VOLCANO_LOOP, false);
OPTION_DECLARE(int, VOLCANO_LOOP_TIME, 60);

#define INPUT_BUTTON_UP       (1 << 0)
#define INPUT_BUTTON_DOWN     (1 << 1)
#define INPUT_BUTTON_LEFT     (1 << 2)
#define INPUT_BUTTON_RIGHT    (1 << 3)
#define INPUT_BUTTON_A        (1 << 4)
#define INPUT_BUTTON_B        (1 << 5)
#define INPUT_BUTTON_X        (1 << 6)
#define INPUT_BUTTON_Y        (1 << 7)
#define INPUT_BUTTON_START    (1 << 8)
#define INPUT_BUTTON_L1R1     (1 << 9)
