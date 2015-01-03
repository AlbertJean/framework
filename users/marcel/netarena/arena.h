#pragma once

#include "gamedefs.h"
#include "gametypes.h"
#include "NetSerializable.h"

class Arena;
class BitStream;
class GameSim;
struct Player;

enum BlockShape
{
	kBlockShape_Empty,
	kBlockShape_Opaque,
	kBlockShape_TL,
	kBlockShape_TR,
	kBlockShape_BL,
	kBlockShape_BR,
	kBlockShape_TL2a,
	kBlockShape_TL2b,
	kBlockShape_TR2a,
	kBlockShape_TR2b,
	kBlockShape_BL2a,
	kBlockShape_BL2b,
	kBlockShape_BR2a,
	kBlockShape_BR2b,
	kBlockShape_HT,
	kBlockShape_HB,
	kBlockShape_COUNT
};

enum BlockType
{
	kBlockType_Empty,
	kBlockType_Destructible,
	kBlockType_Indestructible,
	kBlockType_Slide,
	kBlockType_Moving,
	kBlockType_Sticky,
	kBlockType_Spike,
	kBlockType_Spawn,
	kBlockType_Spring,
	kBlockType_Teleport,
	kBlockType_GravityReverse,
	kBlockType_GravityDisable,
	kBlockType_GravityStrong,
	kBlockType_GravityLeft,
	kBlockType_GravityRight,
	kBlockType_ConveyorBeltLeft,
	kBlockType_ConveyorBeltRight,
	kBlockType_Passthrough,
	kBlockType_COUNT
};

static const int kBlockMask_Solid =
	(1 << kBlockType_Destructible) |
	(1 << kBlockType_Indestructible) |
	(1 << kBlockType_Slide) |
	(1 << kBlockType_Sticky) |
	(1 << kBlockType_Spring) |
	(1 << kBlockType_ConveyorBeltLeft) |
	(1 << kBlockType_ConveyorBeltRight) |
	(1 << kBlockType_Passthrough);

static const int kBlockMask_Passthrough =
	(1 << kBlockType_Passthrough);

static const int kBlockMask_Spike =
	(1 << kBlockType_Spike);

#pragma pack(push)
#pragma pack(1)

struct Block
{
	BlockType type;
	uint8_t clientData;

	BlockShape shape;
	uint16_t serverData;
};

#pragma pack(pop)

struct BlockAndDistance
{
	Block * block;
	uint8_t x;
	uint8_t y;
	int distanceSq;
};

class Arena
{
	Block m_blocks[ARENA_SX][ARENA_SY];

	void reset();

public:
	void init();

	void generate();
	void load(const char * filename);
	void serialize(NetSerializationContext & context);

	void drawBlocks();

	bool getRandomSpawnPoint(GameSim & gameSim, int & out_x, int & out_y, int & io_lastSpawnIndex, Player * playerToIgnore) const;
	bool getRandomPickupLocations(int * out_x, int * out_y, int & numLocations, void * obj, bool (*reject)(void * obj, int x, int y)) const;
	bool getTeleportDestination(GameSim & gameSim, int x, int y, int & out_x, int & out_y) const;

	uint32_t getIntersectingBlocksMask(int x1, int y1, int x2, int y2) const;
	uint32_t getIntersectingBlocksMask(int x, int y) const;

	bool getBlockRectFromPixels(int x1, int y1, int x2, int y2, int & out_x1, int & out_y1, int & out_x2, int & out_y2) const;
	bool getBlocksFromPixels(int x, int y, int x1, int y1, int x2, int y2, bool wrap, BlockAndDistance * out_blocks, int & io_numBlocks);
	Block & getBlock(int x, int y) { return m_blocks[x][y]; }

	bool handleDamageRect(GameSim & gameSim, int x, int y, int x1, int y1, int x2, int y2, bool hitDestructible);
};
