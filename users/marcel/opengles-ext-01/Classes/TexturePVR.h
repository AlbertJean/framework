#pragma once

#include <vector>

enum PVR_TextureType
{
	PVR_TextureType_2BPP = 24,
	PVR_TextureType_4BPP
};

class TexturePVRLevel
{
public:
	TexturePVRLevel();
	TexturePVRLevel(
					int sx,
					int sy,
					int bpp,
					bool hasAlpha,
					uint8_t* data,
					int dataSize);
	void Initialize(
					int sx,
					int sy,
					int bpp,
					bool hasAlpha,
					uint8_t* data,
					int dataSize);
	~TexturePVRLevel();
	
	int m_Sx;
	int m_Sy;
	int m_Bpp;
	bool m_HasAlpha;
	uint8_t* m_Data;
	int m_DataSize;
};

class TexturePVR
{
public:
	TexturePVR();
	void Initialize();
	~TexturePVR();
	
	bool Load(char* fileName);
	bool Load(uint8_t* bytes);
	
	std::vector<TexturePVRLevel*> Levels_get();
	
public:
	std::vector<TexturePVRLevel*> m_Levels;
};
