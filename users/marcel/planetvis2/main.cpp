#include "Calc.h"
#include "D:/temp/planetvis2/data/CubeSides.txt"
#include "framework.h"

#include "Noise.h"

#define GFX_SX 1024
#define GFX_SY 1024

#define DO_QUADTREE 1
#define DO_CUBE_QUADS 0
#define DO_CUBE_QUADS_INFOS 0
#define DO_CUBE_POINTS 0
#define DO_CUBE_LINES 0
#define DO_CUBE_SAMPLING 1
#define DO_CUBE_SAMPLING_COLORS 1
#define DO_CUBE_SAMPLING_QUADS 1
#define DO_TEXTURE_ARRAY 1
#define DO_TEXTURE_ARRAY_TEST 0
#define DO_TEXTURE_ARRAY_RESIDENCY 1
#define DO_ADDTIVE_BLENDING 0

#if DO_QUADTREE

struct Cube;
struct CubeSide;
struct QuadNode;
struct QuadParams;
struct QuadTree;

static int calculateSphereHeight(const int x, const int y, const QuadParams & params);
static int calculateSphereDistance(const int level, const int x, const int y, const int size, const QuadParams & params);
static void drawPointGrid(const int x, const int y, const int size, const int step, const QuadParams & params);
static void calculateMipInfo(const int baseSize, const int pageSize, int & numLevels, int & lastLevel, int & numAllocatedLevels);
static void setPlanarTexShader(const Cube * cube, const int debugCubeSideIndex);
static void drawFractalSphere();

struct QuadParams
{
	QuadParams()
		: tree(nullptr)
		, cube(nullptr)
		, cubeSide(nullptr)
		, cubeSideIndex(-1)
		, viewX(0)
		, viewY(0)
		, viewZ(0)
		, debugLod(-1)
		, debugCubeSideIndex(-1)
	{
	}

	QuadTree * tree;
	
	Cube * cube;
	CubeSide * cubeSide;
	int cubeSideIndex;

	int viewX;
	int viewY;
	int viewZ;

	int debugLod;
	int debugCubeSideIndex;
};

struct QuadNode
{
	QuadNode * m_children;

	bool m_isResident;

	QuadNode()
		: m_children(nullptr)
		, m_isResident(false)
	{
	}

	~QuadNode()
	{
		delete[] m_children;
		m_children = nullptr;
	}

	void subdivide(const int size, const int maxSize);
	
	void makeResident(const int level, const int x, const int y, const int size, const QuadParams & params, const bool isResident);
	
	void traverse(const int level, const int x, const int y, const int size, const QuadParams & params);
	bool traverseImpl(const int level, const int x, const int y, const int size, const QuadParams & params);
	
	size_t memUsage(const Cube * cube) const;
};

struct QuadTree
{
	QuadNode m_root;

	int m_initSize;

#if !DO_TEXTURE_ARRAY
	GLuint m_texture;
	int m_pageSize;

	int m_numLevels;
	int m_lastLevel;
	int m_numAllocatedLevels;
#endif

	QuadTree()
		: m_root()
		, m_initSize(0)
	#if !DO_TEXTURE_ARRAY
		, m_texture(0)
		, m_pageSize(0)
		, m_numLevels(0)
		, m_lastLevel(0)
		, m_numAllocatedLevels(0)
	#endif
	{
	}

	void init(const int size, const int pageSize)
	{
		m_initSize = size;

	#if !DO_TEXTURE_ARRAY
		// create sparse texture

		glGenTextures(1, &m_texture);
		checkErrorGL();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
		checkErrorGL();

		GLint pageSx = 0;
		GLint pageSy = 0;
		glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_X_ARB, 1, &pageSx);
		glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_Y_ARB, 1, &pageSy);
		checkErrorGL();

		m_pageSize = std::max(pageSx, pageSy);

		calculateMipInfo(m_initSize, m_pageSize, m_numLevels, m_lastLevel, m_numAllocatedLevels);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, m_numAllocatedLevels - 1);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
		checkErrorGL();

		glTexStorage2D(GL_TEXTURE_2D, m_numAllocatedLevels, GL_RGBA8, m_initSize, m_initSize);
		checkErrorGL();
#endif

		//

	#if DO_TEXTURE_ARRAY
		m_root.subdivide(size, pageSize);
	#else
		m_root.subdivide(size, m_pageSize);
	#endif
	}
};

struct CubeSide
{
	QuadTree m_quadTree;

	int m_planeEquation[4];
	int m_transform[3][3];

	//

	void init(const int size, const int pageSize, const int axis[3][3]);
	void eval(const QuadParams & params);

	void applyTransform();

	void doTransform(const int x, const int y, const int z, int r[3]) const;
	void doTransformTranspose(const int x, const int y, const int z, int r[3]) const;

	int calcDistance(const int x, const int y, const int z) const;
};

struct Cube
{
	CubeSide m_sides[6];

	int m_initSize;

#if DO_TEXTURE_ARRAY
	GLuint m_texture;
	int m_pageSize;
	int m_numLevels;
	int m_lastLevel;
	int m_numAllocatedLevels;

#if DO_TEXTURE_ARRAY_RESIDENCY
	struct ResidencyMapLevel
	{
		uint8_t * m_map;
		int m_mapSize;

		ResidencyMapLevel()
			: m_map(nullptr)
			, m_mapSize(0)
		{
		}

		~ResidencyMapLevel()
		{
			alloc(0);
		}

		void alloc(const int mapSize)
		{
			delete[] m_map;
			m_map = nullptr;
			m_mapSize = 0;

			if (mapSize > 0)
			{
				m_map = new uint8_t[mapSize * mapSize * 4];
				memset(m_map, 0x00, mapSize * mapSize * 4);

				m_mapSize = mapSize;
			}
		}

		void setMipLevels(const int level, const int x, const int y, const int size, const int pageSize)
		{
			const int rx1 = x          / pageSize;
			const int ry1 = y          / pageSize;
			const int rx2 = (x + size) / pageSize;
			const int ry2 = (y + size) / pageSize;

			for (int y = ry1; y < ry2; ++y)
			{
				const int lineSize = m_mapSize * 4;
			
				uint8_t * r = m_map + y * lineSize;

				for (int x = rx1; x < rx2; ++x)
				{
					//Assert(level + 1 > r[x * 4 + 0]);

					//r[x * 4 + 0] = level + 1;
					r[x * 4 + 0] = level * 24;
				}
			}
		}
	};

	GLuint m_residencyMap;
	ResidencyMapLevel m_residencyMapLevels[6];
#endif
#endif

	Cube()
		: m_initSize(0)
	#if DO_TEXTURE_ARRAY
		, m_texture(0)
		, m_pageSize(0)
		, m_numLevels(0)
		, m_lastLevel(0)
		, m_numAllocatedLevels(0)
#if DO_TEXTURE_ARRAY_RESIDENCY
		, m_residencyMap(0)
		, m_residencyMapLevels()
#endif
	#endif
	{
	}

	void init(const int size)
	{
		m_initSize = size;

	#if DO_TEXTURE_ARRAY
		glGenTextures(1, &m_texture);
		checkErrorGL();

		glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
		checkErrorGL();

		GLint pageSx = 0;
		GLint pageSy = 0;
		GLint pageSz = 0;
		glGetInternalformativ(GL_TEXTURE_2D_ARRAY, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_X_ARB, 1, &pageSx);
		glGetInternalformativ(GL_TEXTURE_2D_ARRAY, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_Y_ARB, 1, &pageSy);
		glGetInternalformativ(GL_TEXTURE_2D_ARRAY, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_Z_ARB, 1, &pageSz);
		checkErrorGL();

		/*
		GLint maxSparseTextureSize = 0;
		GLint maxSparseTexture3DSize = 0;
		GLint maxSparseTextureArraySize = 0;
		glGetIntegerv(GL_MAX_SPARSE_TEXTURE_SIZE_ARB, &maxSparseTextureSize);
		glGetIntegerv(GL_MAX_SPARSE_3D_TEXTURE_SIZE_ARB, &maxSparseTexture3DSize);
		glGetIntegerv(GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS_ARB, &maxSparseTextureArraySize);
		*/

		//

		m_pageSize = std::max(pageSx, pageSy);

		calculateMipInfo(m_initSize, m_pageSize, m_numLevels, m_lastLevel, m_numAllocatedLevels);

		//

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		checkErrorGL();

		//glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
		//checkErrorGL();

		glTexStorage3D(GL_TEXTURE_2D_ARRAY, m_numAllocatedLevels, GL_RGBA8, m_initSize, m_initSize, 6);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, m_numAllocatedLevels - 1);
		checkErrorGL();

		// allocate residency map

#if DO_TEXTURE_ARRAY_RESIDENCY
		const int residencyMapSize = m_initSize / m_pageSize;

		glGenTextures(1, &m_residencyMap);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_residencyMap);

		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, residencyMapSize, residencyMapSize, 6);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
		checkErrorGL();

		for (int i = 0; i < 6; ++i)
		{
			m_residencyMapLevels[i].alloc(residencyMapSize);
		}
#endif
	#endif

		//

		for (int s = 0; s < 6; ++s)
		{
			Mat4x4 m;
	
			if (s == 0) m.MakeLookat(Vec3(0.f, 0.f, 0.f), Vec3(0.f, -1.f, 0.f), Vec3(1.f, 0.f, 0.f));
			if (s == 1) m.MakeLookat(Vec3(0.f, 0.f, 0.f), Vec3(0.f, +1.f, 0.f), Vec3(1.f, 0.f, 0.f));
			if (s == 2) m.MakeLookat(Vec3(0.f, 0.f, 0.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 0.f, -1.f));
			if (s == 3) m.MakeLookat(Vec3(0.f, 0.f, 0.f), Vec3(+1.f, 0.f, 0.f), Vec3(0.f, 0.f, 1.f));
			if (s == 4) m.MakeLookat(Vec3(0.f, 0.f, 0.f), Vec3(0.f, 0.f, -1.f), Vec3(0.f, 1.f, 0.f));
			if (s == 5) m.MakeLookat(Vec3(0.f, 0.f, 0.f), Vec3(0.f, 0.f, +1.f), Vec3(0.f, 1.f, 0.f));
			
			//

			int axis[3][3];

			for (int x = 0; x < 3; ++x)
			{
				for (int y = 0; y < 3; ++y)
				{
					axis[x][y] =
						m(x, y) < -.5f ? -1 :
						m(x, y) > +.5f ? +1 :
						0;
				}
			}

		#if DO_TEXTURE_ARRAY
			const int pageSize = m_pageSize;
		#else
			const int pageSize = 0;
		#endif

			m_sides[s].init(size, pageSize, axis);
		}
	}

	void eval(const QuadParams & params)
	{
		Assert(params.cube == this);

		for (int i = 0; i < 6; ++i)
		{
			QuadParams localParams = params;
			localParams.cubeSide = &m_sides[i];
			localParams.cubeSideIndex = i;

			m_sides[i].eval(localParams);
		}

#if DO_CUBE_SAMPLING_QUADS
		for (int i = 0; params.debugCubeSideIndex >= 0 && i < 6; ++i)
		{
			gxPushMatrix();
			{
				m_sides[i].applyTransform();

				setPlanarTexShader(params.cube, params.debugCubeSideIndex);
				{
					const int x = 0;
					const int y = 0;
					const int size = m_initSize;

					const float cubeSizeRcp = 1.f / size;

					gxBegin(GL_QUADS);
					{
						const int x1 = x;
						const int y1 = y;
						const int x2 = x + size;
						const int y2 = y + size;
						const int z = size/16;

						gxNormal3f(0.f, 0.f, 1.f);
						gxTexCoord2f(x1 * cubeSizeRcp, y1 * cubeSizeRcp); gxVertex3f(x1, y1, z);
						gxTexCoord2f(x2 * cubeSizeRcp, y1 * cubeSizeRcp); gxVertex3f(x2, y1, z);
						gxTexCoord2f(x2 * cubeSizeRcp, y2 * cubeSizeRcp); gxVertex3f(x2, y2, z);
						gxTexCoord2f(x1 * cubeSizeRcp, y2 * cubeSizeRcp); gxVertex3f(x1, y2, z);
					}
					gxEnd();
				}
				clearShader();
			}
			gxPopMatrix();
		}
#endif

	#if DO_TEXTURE_ARRAY
#if DO_TEXTURE_ARRAY_RESIDENCY
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_residencyMap);
		for (int i = 0; i < 6; ++i)
		{
			glTexSubImage3D(
				GL_TEXTURE_2D_ARRAY,
				0,
				0, 0, i,
				m_residencyMapLevels[i].m_mapSize, m_residencyMapLevels[i].m_mapSize, 1,
				GL_RGBA,
				GL_UNSIGNED_BYTE,
				m_residencyMapLevels[i].m_map);
			checkErrorGL();
		}
#endif
	#endif
	}

	size_t memUsage() const
	{
		size_t result = 0;

		for (int i = 0; i < 6; ++i)
		{
			result += m_sides[i].m_quadTree.m_root.memUsage(this);
		}

		return result;
	}
};

void QuadNode::subdivide(const int size, const int maxSize)
{
	Assert(size > maxSize);

	m_children = new QuadNode[4];

	const int childSize = size / 2;

	if (childSize > maxSize)
	{
		for (int i = 0; i < 4; ++i)
		{
			m_children[i].subdivide(childSize, maxSize);
		}
	}
}

void QuadNode::makeResident(const int level, const int x, const int y, const int size, const QuadParams & params, const bool isResident)
{
	Assert(isResident != m_isResident);
	m_isResident = isResident;

#if DO_TEXTURE_ARRAY
	Assert((x % params.cube->m_pageSize) == 0);
	Assert((y % params.cube->m_pageSize) == 0);
	Assert((size % params.cube->m_pageSize) == 0);

	const int levelSize = params.cube->m_pageSize << level;
	const int mipIndex = params.cube->m_numAllocatedLevels - level - 1;
	const int scale = params.cube->m_initSize / levelSize;

	Assert(levelSize >= params.cube->m_pageSize && levelSize <= params.cube->m_initSize);
	Assert(mipIndex >= 0 && mipIndex < params.cube->m_numAllocatedLevels);
	Assert((scale * levelSize) == params.cube->m_initSize);

	const int px = x / scale;
	const int py = y / scale;
	const int pz = params.cubeSideIndex;
	
	const int sx = params.cube->m_pageSize;
	const int sy = params.cube->m_pageSize;
	const int sz = 1;

	Assert(px >= 0 && px <= (params.cube->m_initSize >> mipIndex));
	Assert(py >= 0 && py <= (params.cube->m_initSize >> mipIndex));
	Assert((px % params.cube->m_pageSize) == 0);
	Assert((py % params.cube->m_pageSize) == 0);
	Assert(pz >= 0 && pz < 6);

	glBindTexture(GL_TEXTURE_2D_ARRAY, params.cube->m_texture);
	checkErrorGL();
#else
	const int levelSize = params.tree->m_pageSize << level;
	const int mipIndex = params.tree->m_numAllocatedLevels - level - 1;
	const int scale = params.tree->m_initSize / levelSize;

	Assert(mipIndex >= 0);
	Assert(scale >= 1);

	const int px = x / scale;
	const int py = y / scale;
	const int pz = params.cubeSideIndex;
	const int sx = params.tree->m_pageSize;
	const int sy = params.tree->m_pageSize;

	Assert((px % params.tree->m_pageSize) == 0);
	Assert((py % params.tree->m_pageSize) == 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, params.tree->m_texture);
	checkErrorGL();
#endif

#if DO_TEXTURE_ARRAY
	if (!isResident)
	{
		uint8_t * pixels = new uint8_t[sx * sy * 4];
		memset(pixels, 0x00, sx * sy * 4);

		glTexSubImage3D(
			GL_TEXTURE_2D_ARRAY,
			mipIndex,
			px, py, pz,
			sx, sy, sz,
			GL_RGBA, GL_UNSIGNED_BYTE,
			pixels);
		checkErrorGL();

		delete[] pixels;
		pixels = nullptr;
	}

	glTexPageCommitmentARB(
		GL_TEXTURE_2D_ARRAY,
		mipIndex,
		px, py, pz,
		sx, sy, sz,
		isResident);
	checkErrorGL();
#else
	glTexPageCommitmentARB(
		GL_TEXTURE_2D,
		mipIndex,
		px, py, 0,
		sx, sy, 1,
		isResident);
	checkErrorGL();
#endif

	if (isResident)
	{
#if DO_TEXTURE_ARRAY_RESIDENCY
		params.cube->m_residencyMapLevels[params.cubeSideIndex].setMipLevels(level + 1, x, y, size, params.cube->m_pageSize);
#endif

		uint8_t * pixels = new uint8_t[sx * sy * 4];
		uint8_t * pixelPtr = pixels;

		for (int y = 0; y < sy; ++y)
		{
			for (int x = 0; x < sx; ++x)
			{
#if 1
				const int cx = (px + x) * scale;
				const int cy = (py + y) * scale;
#if 1
				const float v = scaled_octave_noise_2d(8.f, .5f, .005f,
				//const float v = scaled_octave_noise_2d(2.f, .5f, .001f,
					64,
					255,
					cx,
					cy);// - (rand() % 32);

			#if 0
				const int c = (int(v) * ((level + 4) * 20)) >> 8;
			#else
				const int c = int(v);
			#endif
#elif 1
				const int c = (level + 4) * 20;
#elif 0
				float v =
					std::sin(cx + cy) +
					std::sin(cx) +
					std::sin(cy);

				v = (v + 3.f) / 6.f * 255.f;

				const int c = int(v);
#endif

				*pixelPtr++ = c;
				*pixelPtr++ = c;
				*pixelPtr++ = c;
				*pixelPtr++ = 255;
#else
				*pixelPtr++ = x >> 0;
				*pixelPtr++ = y >> 1;
				*pixelPtr++ = (x + y) >> 2;
				*pixelPtr++ = 255;
#endif
			}
		}

	#if DO_TEXTURE_ARRAY
		glTexSubImage3D(
			GL_TEXTURE_2D_ARRAY,
			mipIndex,
			px, py, pz,
			sx, sy, sz,
			GL_RGBA, GL_UNSIGNED_BYTE,
			pixels);
		checkErrorGL();
	#else
		glTexSubImage2D(GL_TEXTURE_2D,
			mipIndex,
			px, py,
			sx, sy,
			GL_RGBA, GL_UNSIGNED_BYTE,
			pixels);
		checkErrorGL();
	#endif

		delete[] pixels;
		pixels = nullptr;
	}

	if (m_children != nullptr && isResident == false)
	{
		const int childSize = size / 2;

		const int x1 = x;
		const int x2 = x + childSize;
		const int y1 = y;
		const int y2 = y + childSize;

		if (m_children[0].m_isResident)
			m_children[0].makeResident(level + 1, x1, y1, childSize, params, false);
		if (m_children[1].m_isResident)
			m_children[1].makeResident(level + 1, x2, y1, childSize, params, false);
		if (m_children[2].m_isResident)
			m_children[2].makeResident(level + 1, x2, y2, childSize, params, false);
		if (m_children[3].m_isResident)
			m_children[3].makeResident(level + 1, x1, y2, childSize, params, false);
	}

#if DO_TEXTURE_ARRAY_RESIDENCY
	if (isResident == false)
	{
		params.cube->m_residencyMapLevels[params.cubeSideIndex].setMipLevels(level, x, y, size, params.cube->m_pageSize);
	}
#endif
}

void QuadNode::traverse(const int level, const int x, const int y, const int size, const QuadParams & params)
{
	const bool doTraverse = traverseImpl(level, x, y, size, params) || (level == 0);

	// todo : remove children check ?

	if (doTraverse)
	{
		if (!m_isResident)
			makeResident(level, x, y, size, params, true);
	}
	else
	{
		if (m_isResident)
			makeResident(level, x, y, size, params, false);
	}

#if DO_CUBE_QUADS
	//if (level == 0)
	if (level == 0 && (params.debugCubeSideIndex < 0 || params.cubeSideIndex == params.debugCubeSideIndex))
	{
		const bool shadered = true;
		const bool textured = false;

	#if DO_CUBE_SAMPLING_QUADS && 0
		setPlanarTexShader(params.cube, params.debugCubeSideIndex);
	#elif DO_TEXTURE_ARRAY
		Shader shader("TexLodArray");
		setShader(shader);
	#else
		Shader shader("TexLod");
		setShader(shader);
	#endif

		if (shadered)
		{
		#if DO_CUBE_SAMPLING_QUADS && 0
		#elif DO_TEXTURE_ARRAY
			shader.setTextureArray("texture", 0, params.cube->m_texture);
			shader.setImmediate("textureIndex", params.cubeSideIndex);
			shader.setImmediate("lod", params.debugLod);
			shader.setImmediate("numLods", params.cube->m_numAllocatedLevels);
		#else
			shader.setTexture("texture", 0, params.cubeSide->m_quadTree.m_texture);
			shader.setImmediate("lod", params.debugLod);
			shader.setImmediate("numLods", params.tree->m_numAllocatedLevels);
		#endif
		}
	#if !DO_TEXTURE_ARRAY
		else if (textured)
			gxSetTexture(params.cubeSide->m_quadTree.m_texture);
	#endif
		{
			const int sphereDistance = calculateSphereDistance(level, x, y, size, params);
			const float cubeSizeRcp = 1.f / params.tree->m_initSize;

			if (textured)
				setColor(255, 255, 255, 127);
			else
				//setColor(255, sphereHeight * 255 / (params.tree->initSize/2), 0, 31);
				setColor(colorWhite);

			gxBegin(GL_QUADS);
			{
				const int x1 = x;
				const int y1 = y;
				const int x2 = x + size;
				const int y2 = y + size;
				const int z = 0;
				//const int z = - level * 256;
				//const int z = sphereDistance + 128;

				gxNormal3f(0.f, 0.f, 1.f);
				gxTexCoord2f(x1 * cubeSizeRcp, y1 * cubeSizeRcp); gxVertex3f(x1, y1, z);
				gxTexCoord2f(x2 * cubeSizeRcp, y1 * cubeSizeRcp); gxVertex3f(x2, y1, z);
				gxTexCoord2f(x2 * cubeSizeRcp, y2 * cubeSizeRcp); gxVertex3f(x2, y2, z);
				gxTexCoord2f(x1 * cubeSizeRcp, y2 * cubeSizeRcp); gxVertex3f(x1, y2, z);
			}
			gxEnd();
		}
		clearShader();
		gxSetTexture(0);
	}
#endif

	if (m_children == nullptr || doTraverse == false)
	{
		const int ds = calculateSphereDistance(level, x, y, size, params);
		const float d = std::sqrt(float(ds));

#if DO_CUBE_POINTS
	#if DO_CUBE_SAMPLING
		setPlanarTexShader(params.cube, params.debugCubeSideIndex);
		{
			drawPointGrid(x, y, size, size/32, params);
		}
		clearShader();
	#else
		setColor(colorWhite);
		drawPointGrid(x, y, size, size/32, params);
	#endif
#endif

#if DO_CUBE_QUADS_INFOS
		setColorf(1.f, 1.f, 1.f, 1.f / (d / 200.f + 1.f));
		gxBegin(GL_LINE_LOOP);
		{
			const int x1 = x;
			const int y1 = y;
			const int x2 = x + size;
			const int y2 = y + size;
			const int z = 0;
			//const int z = level * 256;

			gxVertex3f(x1, y1, z);
			gxVertex3f(x2, y1, z);
			gxVertex3f(x2, y2, z);
			gxVertex3f(x1, y2, z);
		}
		gxEnd();

		setColor(colorWhite);
		drawText(x + size/2, y + size/2, 20, 0, 0, "%d", int(d));
#endif
	}

	if (m_children != nullptr && doTraverse)
	{
		const int childSize = size / 2;

		const int x1 = x;
		const int x2 = x + childSize;
		const int y1 = y;
		const int y2 = y + childSize;

		m_children[0].traverse(level + 1, x1, y1, childSize, params);
		m_children[1].traverse(level + 1, x2, y1, childSize, params);
		m_children[2].traverse(level + 1, x2, y2, childSize, params);
		m_children[3].traverse(level + 1, x1, y2, childSize, params);
	}
}

bool QuadNode::traverseImpl(const int level, const int x, const int y, const int size, const QuadParams & params)
{
	if (level == 0)
		return true;
	else
	{
		//const int ds = calculateDistanceSq(params.viewX, params.viewY, params.viewZ, x, y, size);
		const int ds = calculateSphereDistance(level, x, y, size, params);

		//if (ds * 4 < size * size)
		//if (ds < size * size)
		//if (ds < size)
		if (ds < size * 3)
			return true;
		else
		{
			if (keyboard.isDown(SDLK_c) && m_children == nullptr)
				return true;
			else
				return false;
		}
	}
}

size_t QuadNode::memUsage(const Cube * cube) const
{
	size_t result = 0;

	if (m_isResident)
	{
	#if DO_TEXTURE_ARRAY
		result += cube->m_pageSize * cube->m_pageSize * 4;
	#else
		result += params.tree->m_pageSize * params.tree->m_pageSize;
	#endif
	}

	if (m_children != nullptr)
	{
		for (int i = 0; i < 4; ++i)
		{
			result += m_children[i].memUsage(cube);
		}
	}

	return result;
}

//

void CubeSide::init(const int size, const int pageSize, const int axis[3][3])
{
	m_quadTree.init(size, pageSize);

	m_planeEquation[0] = axis[2][0];
	m_planeEquation[1] = axis[2][1];
	m_planeEquation[2] = axis[2][2];
	m_planeEquation[3] = - size / 2;

	for (int a = 0; a < 3; ++a)
		for (int e = 0; e < 3; ++e)
			m_transform[a][e] = axis[a][e];
}

void CubeSide::eval(const QuadParams & params)
{
	Assert(params.cubeSide == this);

	static int i = 0;
	static int d = 0;
	if (i == 0 && keyboard.wentDown(SDLK_SPACE))
		d = (d + 1) % 6;

	int t[3];

	gxPushMatrix();
	if (i == d && false)
	{
		const int cubeSize = m_quadTree.m_initSize;
		gxTranslatef(-cubeSize/2, -cubeSize/2, -cubeSize/2);

		setColor(colorWhite);
		gxBegin(GL_LINES);
		{
			const int cubeSize = m_quadTree.m_initSize;
			int v1[3];
			int v2[3];
			doTransformTranspose(cubeSize/2 + 0, cubeSize/2 + 0, cubeSize/2 +   0, v1);
			doTransformTranspose(cubeSize/2 + 0, cubeSize/2 + 0, cubeSize/2 + 512, v2);
			gxVertex3f(v1[0], v1[1], v1[2]);
			gxVertex3f(v2[0], v2[1], v2[2]);
		}
		gxEnd();
	}
	gxPopMatrix();

	doTransform(params.viewX, params.viewY, params.viewZ, t);

	QuadParams localParams = params;
	localParams.tree = &m_quadTree;
	localParams.viewX = t[0];
	localParams.viewY = t[1];
	localParams.viewZ = t[2];

	gxPushMatrix();
	{
		applyTransform();

#if DO_CUBE_LINES
		setColor(colorWhite);
		drawRectLine(0, 0, m_quadTree.m_initSize, m_quadTree.m_initSize);

	#if 1
		gxBegin(GL_LINES);
		{
			setColor(255, 255, 0);
			gxVertex3f(localParams.viewX, localParams.viewY, localParams.viewZ * .9f);
			gxVertex3f(localParams.viewX, localParams.viewY, 0.f);

			setColor(127, 127, 0);
			gxVertex3f(localParams.viewX, localParams.viewY, 0.f);
			gxVertex3f(localParams.viewX, 0.f, 0.f);

			gxVertex3f(localParams.viewX, localParams.viewY, 0.f);
			gxVertex3f(0.f, localParams.viewY, 0.f);
		}
		gxEnd();

		setColor(colorRed);
		drawCircle(localParams.viewX, localParams.viewY, 64.f, 20);
	#endif
#endif

	#if DO_CUBE_POINTS && 0
		if (i == d || true)
		{
			setColor(255, 255, 255, 63);
			drawPointGrid(0, 0, quadTree.initSize, 256, localParams);
		}
	#endif
		i = (i + 1) % 6;

	#if DO_CUBE_POINTS && DO_CUBE_SAMPLING && 0
		{
			setPlanarTexShader(localParams.cube, localParams.debugCubeSideIndex);
			{
				drawPointGrid(0, 0, m_quadTree.m_initSize, 64, localParams);
			}
			clearShader();
		}
	#endif

	#if DO_TEXTURE_ARRAY
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D_ARRAY, localParams.cube->m_texture);
		//checkErrorGL();
	#else
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, localParams.cubeSide->m_quadTree.m_texture);
		checkErrorGL();
	#endif

	#if 0
		gxTranslatef(+quadTree.initSize/2, +quadTree.initSize/2, +quadTree.initSize/2);
		const float s = 1.f / sqrt(2.f);
		gxScalef(s, s, s);
		gxTranslatef(-quadTree.initSize/2, -quadTree.initSize/2, -quadTree.initSize/2);
	#endif

		m_quadTree.m_root.traverse(0, 0, 0, m_quadTree.m_initSize, localParams);
	}
	gxPopMatrix();
}

void CubeSide::applyTransform()
{
	Mat4x4 matT(true);

	for (int a = 0; a < 3; ++a)
	{
		for (int e = 0; e < 3; ++e)
		{
			matT(a, e) = m_transform[a][e];
		}

		matT(3, a) += m_planeEquation[a] * m_planeEquation[3];
	}

	const Mat4x4 mat = matT.Translate(-m_quadTree.m_initSize/2, -m_quadTree.m_initSize/2, 0);

	gxMultMatrixf(mat.m_v);
}

void CubeSide::doTransform(const int x, const int y, const int z, int r[3]) const
{
	const int cubeSize2 = m_quadTree.m_initSize / 2;

	const int v[3] = { x - cubeSize2, y - cubeSize2, z - cubeSize2 };

	for (int a = 0; a < 3; ++a)
	{
		r[a] =
			v[0] * m_transform[a][0] +
			v[1] * m_transform[a][1] +
			v[2] * m_transform[a][2] +
			cubeSize2;
	}
}

void CubeSide::doTransformTranspose(const int x, const int y, const int z, int r[3]) const
{
	const int cubeSize2 = m_quadTree.m_initSize / 2;

	const int v[3] = { x - cubeSize2, y - cubeSize2, z - cubeSize2 };

	for (int a = 0; a < 3; ++a)
	{
		r[a] =
			v[0] * m_transform[0][a] +
			v[1] * m_transform[1][a] +
			v[2] * m_transform[2][a] +
			cubeSize2;
	}
}

int CubeSide::calcDistance(const int x, const int y, const int z) const
{
	return
		m_planeEquation[0] * x +
		m_planeEquation[1] * y +
		m_planeEquation[2] * z +
		m_planeEquation[3];
}

//

static int calculateSphereHeight(const int x, const int y, const QuadParams & params)
{
	const int tr = params.tree->m_initSize/2;

	const int dx = x - tr;
	const int dy = y - tr;
	const int dSq = dx * dx + dy * dy;
	const int rSq = tr * tr - dSq;

	const int r = rSq < 0 ? 0 : std::sqrt(rSq);

	return r;
}

static int calculateSphereDistance(const int level, const int x, const int y, const int size, const QuadParams & params)
{
	const int sphereRadius = params.tree->m_initSize/2;

	int maxSphereHeight = 0;

	if (level == 0)
	{
		maxSphereHeight = sphereRadius;
	}
	else
	{
		const int sx[2] = { x, x + size };
		const int sy[2] = { y, y + size };

		for (int ix = 0; ix < 2; ++ix)
		{
			for (int iy = 0; iy < 2; ++iy)
			{
				const int height = calculateSphereHeight(sx[ix], sy[iy], params);

				maxSphereHeight = std::max(maxSphereHeight, height);
			}
		}
	}

	const int x1 = x;
	const int x2 = x + size;
	const int y1 = y;
	const int y2 = y + size;

	const int64_t dx = params.viewX < x1 ? x1 - params.viewX : params.viewX > x2 ? params.viewX - x2 : 0;
	const int64_t dy = params.viewY < y1 ? y1 - params.viewY : params.viewY > y2 ? params.viewY - y2 : 0;

	const int sphereZ = sphereRadius - maxSphereHeight;
	const int pz = params.viewZ < sphereZ ? params.viewZ : sphereZ;
	const int64_t dz = sphereZ - params.viewZ;

	const int result = std::sqrt(dx * dx + dy * dy + dz * dz);

	//logDebug("sphereHeight: %d", result);

	return result;
}

static void drawPointGrid(const int x, const int y, const int size, const int step, const QuadParams & params)
{
	const int cubeSize = params.tree->m_initSize;
	const int cubeRadius = cubeSize / 2;

	glPointSize(2);
	gxBegin(GL_POINTS);
	{
		for (int ox = 0; ox < size; ox += step)
		{
			for (int oy = 0; oy < size; oy += step)
			{
				const int px = x + ox;
				const int py = y + oy;

				const int height = calculateSphereHeight(px, py, params);
				const int distance = calculateSphereDistance(~0, px, py, step, params);

				const int pz = cubeRadius - height;

			#if DO_CUBE_SAMPLING
				gxTexCoord2f(
					px / float(params.tree->m_initSize),
					py / float(params.tree->m_initSize));

				const int64_t dx = cubeRadius - px;
				const int64_t dy = cubeRadius - py;
				const int64_t dz = cubeRadius - pz;
				gxNormal3f(
					dx / float(cubeRadius),
					dy / float(cubeRadius),
					dz / float(cubeRadius));
			#endif

				gxColor3ub(0, 0, height * 255 / cubeRadius);
				//gxColor3ub(0, 0, 127);
				//gxColor3ub(0, 0, distance * 255 / cubeRadius);
				//gxVertex2f(px, py);
				//gxVertex3f(px, py, distance);
				gxVertex3f(px, py, pz);
			}
		}
	}
	gxEnd();
	glPointSize(1);
}

static void calculateMipInfo(const int baseSize, const int pageSize, int & numLevels, int & lastLevel, int & numAllocatedLevels)
{
	lastLevel = 0;

	while (baseSize / (1 << lastLevel) > pageSize)
	{
		lastLevel++;
	}

	//

	numLevels = 1;

	while ((1 << (numLevels - 1)) < baseSize)
	{
		numLevels++;
	}

	//

	numAllocatedLevels = lastLevel + 1;

	//

	Assert((1 << (numLevels - 1)) == baseSize);
	Assert((1 << lastLevel) == (baseSize / pageSize));
	Assert((1 << (numLevels - numAllocatedLevels)) == pageSize);
}

#endif

static void setPlanarTexShader(const Cube * cube, const int debugCubeSideIndex)
{
	CubeSideInfo cubeSideInfo;
	for (int s = 0; s < 6; ++s)
	{
		for (int x = 0; x < 3; ++x)
			for (int y = 0; y < 3; ++y)
				cubeSideInfo.transforms[s * 9 + y * 3 + x] = cube->m_sides[s].m_transform[x][y];
	}
	cubeSideInfo.testValue = std::fmodf(framework.time, .5f);
	static ShaderBufferRw buffer;
	buffer.setDataRaw(&cubeSideInfo, sizeof(cubeSideInfo));

	static Shader shader("PlanarTex");
	setShader(shader);
	shader.setTextureArray("texture", 0, cube->m_texture);
	shader.setImmediate("lod", -1.f);
	shader.setImmediate("numLods", cube->m_numAllocatedLevels);
	shader.setImmediate("debugCubeSideIndex", debugCubeSideIndex);
	shader.setImmediate("debugColors", DO_CUBE_SAMPLING_COLORS);
	shader.setBufferRw("CubeSideInfoBlock", buffer);
}

struct Vertex
{
	float p[3];

	Vertex()
	{
	}

	Vertex(const float x, const float y, const float z)
	{
		p[0] = x;
		p[1] = y;
		p[2] = z;
	}

	static void interp(const Vertex & v1, const Vertex & v2, Vertex & vout)
	{
		for (int i = 0; i < 3; ++i)
			vout.p[i] = (v1.p[i] + v2.p[i]) * .5f;
	}
};

static void subdivideTriangle(const Vertex * vertices, Vertex *& outVertices, int & outNumTrianglesLeft)
{
	Assert(outNumTrianglesLeft >= 4);

	*outVertices++ = vertices[0];
	Vertex::interp(vertices[0], vertices[1], *outVertices++);
	Vertex::interp(vertices[2], vertices[0], *outVertices++);

	*outVertices++ = vertices[1];
	Vertex::interp(vertices[1], vertices[2], *outVertices++);
	Vertex::interp(vertices[0], vertices[1], *outVertices++);

	*outVertices++ = vertices[2];
	Vertex::interp(vertices[2], vertices[0], *outVertices++);
	Vertex::interp(vertices[1], vertices[2], *outVertices++);

	Vertex::interp(vertices[0], vertices[1], *outVertices++);
	Vertex::interp(vertices[1], vertices[2], *outVertices++);
	Vertex::interp(vertices[2], vertices[0], *outVertices++);

	outNumTrianglesLeft -= 4;
	//outNumTrianglesLeft -= 3;
}

static void drawFractalSpherePart(const Vertex & v1, const Vertex & v2, const Vertex & v3)
{
	const int kNumTriangles = 1024 * 16;
	static Vertex vertices[kNumTriangles * 3];

	Vertex * start = vertices;
	Vertex * end = vertices;
	int numTrianglesLeft = kNumTriangles;
	int numTrianglesUsed = 0;

	*end++ = v1;
	*end++ = v2;
	*end++ = v3;
	numTrianglesUsed += 1;
	numTrianglesLeft -= 1;

	while (numTrianglesLeft >= numTrianglesUsed * 4)
	{
		const int oldCount = numTrianglesLeft;

		for (int i = 0; i < numTrianglesUsed; ++i)
		{
			subdivideTriangle(start, end, numTrianglesLeft);

			start += 3;
		}

		const int newCount = numTrianglesLeft;

		const int t = end - start;

		numTrianglesUsed = oldCount - newCount;
	}

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	gxBegin(GL_TRIANGLES);
	{
		for (int i = numTrianglesUsed * 3; i != 0; --i, ++start)
		{
			const float dx = start->p[0];
			const float dy = start->p[1];
			const float dz = start->p[2];
			const float ds = std::sqrt(dx * dx + dy * dy + dz * dz);

			const float px = dx / ds;
			const float py = dy / ds;
			const float pz = dz / ds;

			gxNormal3f(-px, -py, -pz);
			gxVertex3f(px, py, pz);
		}
	}
	gxEnd();

	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

static void drawFractalSphere()
{
	const Vertex v[6] =
	{
		Vertex(+0.f, -1.f, +0.f),
		Vertex(+0.f, +1.f, +0.f),

		Vertex(-1.f, -1.f, -1.f),
		Vertex(+1.f, +0.f, -1.f),
		Vertex(+1.f, +0.f, +1.f),
		Vertex(-1.f, +0.f, +1.f),
	};

	drawFractalSpherePart(v[2], v[3], v[0]);
	drawFractalSpherePart(v[3], v[4], v[0]);
	drawFractalSpherePart(v[4], v[5], v[0]);
	drawFractalSpherePart(v[5], v[2], v[0]);

	drawFractalSpherePart(v[2], v[3], v[1]);
	drawFractalSpherePart(v[3], v[4], v[1]);
	drawFractalSpherePart(v[4], v[5], v[1]);
	drawFractalSpherePart(v[5], v[2], v[1]);
}

#if DO_TEXTURE_ARRAY_TEST

struct TextureArrayObject
{
	GLuint texture;

	int m_size;
	int m_pageSize;
	int m_numSlices;
	int m_numMips;

	TextureArrayObject()
		: texture(0)
		, m_size(0)
		, m_pageSize(0)
		, m_numSlices(0)
		, m_numMips(0)
	{
	}

	void init(const int size)
	{
		m_size = size;

		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
		checkErrorGL();

		GLint pageSx = 0;
		GLint pageSy = 0;
		GLint pageSz = 0;
		glGetInternalformativ(GL_TEXTURE_2D_ARRAY, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_X_ARB, 1, &pageSx);
		glGetInternalformativ(GL_TEXTURE_2D_ARRAY, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_Y_ARB, 1, &pageSy);
		glGetInternalformativ(GL_TEXTURE_2D_ARRAY, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_Z_ARB, 1, &pageSz);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		checkErrorGL();

		m_pageSize = std::max(pageSx, pageSy);

		m_numSlices = 6;
		
		m_numMips = 1;
		for (int mipSize = size; mipSize > m_pageSize; mipSize /= 2)
			m_numMips++;

		glTexStorage3D(GL_TEXTURE_2D_ARRAY, m_numMips, GL_RGBA8, size, size, m_numSlices);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, m_numMips - 1);
		checkErrorGL();

		uint8_t * pixels = new uint8_t[m_pageSize * m_pageSize * 4];
		memset(pixels, 0xff, m_pageSize * m_pageSize * 4);

		for (int sliceIndex = 0; sliceIndex < m_numSlices; ++sliceIndex)
		{
			for (int mipSize = size, mipIndex = 0; mipIndex < m_numMips; mipSize /=2, ++mipIndex)
			{
				const int numPagesX = (mipSize + m_pageSize - 1) / m_pageSize;
				const int numPagesY = (mipSize + m_pageSize - 1) / m_pageSize;

				for (int x = 0; x < numPagesX; ++x)
				{
					for (int y = 0; y < numPagesY; ++y)
					{
						if (((sliceIndex + x + y) % 8) == 0)
						{
							const int px = x * m_pageSize;
							const int py = y * m_pageSize;
							const int pz = sliceIndex;

							const int sx = m_pageSize;
							const int sy = m_pageSize;
							const int sz = 1;

							glTexPageCommitmentARB(
								GL_TEXTURE_2D_ARRAY,
								mipIndex,
								px, py, pz,
								sx, sy, sz,
								GL_TRUE);
							checkErrorGL();

							glTexSubImage3D(
								GL_TEXTURE_2D_ARRAY,
								mipIndex,
								px, py, pz,
								sx, sy, sz,
								GL_RGBA, GL_UNSIGNED_BYTE,
								pixels);
							checkErrorGL();
						}
					}
				}
			}
		}

		delete[] pixels;
		pixels = nullptr;
	}

	void randomize()
	{
		uint8_t * pixels = new uint8_t[m_pageSize * m_pageSize * 4];
		memset(pixels, 0xff, m_pageSize * m_pageSize * 4);

		uint8_t * zeroes = new uint8_t[m_pageSize * m_pageSize * 4];
		memset(zeroes, 0x00, m_pageSize * m_pageSize * 4);

		for (int sliceIndex = 0; sliceIndex < m_numSlices; ++sliceIndex)
		{
			//for (int mipSize = m_size, mipIndex = 0; mipIndex < m_numMips; mipSize /=2, ++mipIndex)
			for (int mipSize = m_size, mipIndex = 0; mipIndex < 1; mipSize /=2, ++mipIndex)
			{
				const int numPagesX = (mipSize + m_pageSize - 1) / m_pageSize;
				const int numPagesY = (mipSize + m_pageSize - 1) / m_pageSize;

				for (int x = 0; x < numPagesX; ++x)
				{
					for (int y = 0; y < numPagesY; ++y)
					{
						//if ((rand() % 1000) == 0)
						if ((rand() % 100) == 0)
						{
							const int px = x * m_pageSize;
							const int py = y * m_pageSize;
							const int pz = sliceIndex;

							const int sx = m_pageSize;
							const int sy = m_pageSize;
							const int sz = 1;

							const bool isResident = (rand() % 10) == 0;
							//const bool isResident = true;
							//const bool isResident = false;

							if (!isResident)
							{
								glTexSubImage3D(
									GL_TEXTURE_2D_ARRAY,
									mipIndex,
									px, py, pz,
									sx, sy, sz,
									GL_RGBA, GL_UNSIGNED_BYTE,
									zeroes);
								checkErrorGL();
							}

							glTexPageCommitmentARB(
								GL_TEXTURE_2D_ARRAY,
								mipIndex,
								px, py, pz,
								sx, sy, sz,
								isResident);
							checkErrorGL();

							if (isResident)
							{
								glTexSubImage3D(
									GL_TEXTURE_2D_ARRAY,
									mipIndex,
									px, py, pz,
									sx, sy, sz,
									GL_RGBA, GL_UNSIGNED_BYTE,
									pixels);
								checkErrorGL();
							}
						}
					}
				}
			}
		}

		delete[] pixels;
		pixels = nullptr;

		delete[] zeroes;
		zeroes = nullptr;
	}
};

#endif

struct Camera
{
	Vec3 position;
	Vec3 rotation;

	float speed;

	Camera()
		: position()
		, rotation()
		, speed(100.f)
	{
	}

	void tick(const float dt)
	{
		rotation[0] -= mouse.dy / 100.f;
		rotation[1] -= mouse.dx / 100.f;

		if (gamepad[0].isConnected)
		{
			rotation[0] -= gamepad[0].getAnalog(1, ANALOG_Y) * dt;
			rotation[1] -= gamepad[0].getAnalog(1, ANALOG_X) * dt;
		}

		Mat4x4 mat;

		calculateTransform(mat);

		const Vec3 xAxis(mat(0, 0), mat(0, 1), mat(0, 2));
		const Vec3 yAxis(mat(1, 0), mat(1, 1), mat(1, 2));
		const Vec3 zAxis(mat(2, 0), mat(2, 1), mat(2, 2));

		Vec3 direction;

		if (keyboard.isDown(SDLK_UP))
			direction += zAxis;
		if (keyboard.isDown(SDLK_DOWN))
			direction -= zAxis;
		if (keyboard.isDown(SDLK_LEFT))
			direction -= xAxis;
		if (keyboard.isDown(SDLK_RIGHT))
			direction += xAxis;
		if (keyboard.isDown(SDLK_a))
			direction += yAxis;
		if (keyboard.isDown(SDLK_z))
			direction -= yAxis;

		if (gamepad[0].isConnected)
		{
			direction -= zAxis * gamepad[0].getAnalog(0, ANALOG_Y);
			direction += xAxis * gamepad[0].getAnalog(0, ANALOG_X);
		}

		position += direction * speed * dt;
	}

	void calculateTransform(Mat4x4 & matrix) const
	{
		matrix = Mat4x4(true).Translate(position).RotateY(rotation[1]).RotateX(rotation[0]);
	}
};

enum ControllerMode
{
	kControl_FreeCam,
	kControl_ViewPos,
	kControl_ViewCam,
	kControl_COUNT
};

int main(int argc, char * argv[])
{
	changeDirectory("data");

	framework.enableRealTimeEditing = true;
	framework.enableDepthBuffer = true;

	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
	#if DO_QUADTREE
		Cube * cube = new Cube();

		cube->init(16 * 1024);
	#endif

	#if DO_TEXTURE_ARRAY_TEST
		TextureArrayObject textureArrayObject;
		textureArrayObject.init(2 * 1024);
	#endif

		//

		ControllerMode controllerMode = kControl_FreeCam;

		Camera camera;

		camera.position[2] = - (cube->m_initSize/2 + 500);
		camera.speed = 10000.f;

		int lod = -1;
		int debugCubeSideIndex = -1;

	#if DO_TEXTURE_ARRAY_TEST
		int taIndex = 0;b
		int taLod = 0;
	#endif

		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;

			if (keyboard.wentDown(SDLK_F1))
			{
				controllerMode = static_cast<ControllerMode>((controllerMode + 1) % kControl_COUNT);
			}

		#if DO_QUADTREE
			if (keyboard.wentDown(SDLK_1))
				lod = std::max(lod - 1, -1);
			if (keyboard.wentDown(SDLK_2))
			{
			#if DO_TEXTURE_ARRAY
				lod = std::min(lod + 1, cube->m_numAllocatedLevels - 1);
			#else
				lod = std::min(lod + 1, cube->m_sides[0].m_quadTree.m_numAllocatedLevels - 1);
			#endif
			}

			if (keyboard.wentDown(SDLK_o))
				debugCubeSideIndex = std::max(debugCubeSideIndex - 1, -1);
			if (keyboard.wentDown(SDLK_p))
				debugCubeSideIndex = std::min(debugCubeSideIndex + 1, 5);
		#endif

		#if DO_TEXTURE_ARRAY_TEST
			if (keyboard.wentDown(SDLK_1))
				taIndex--;
			if (keyboard.wentDown(SDLK_2))
				taIndex++;
			if (keyboard.wentDown(SDLK_q))
				taLod--;
			if (keyboard.wentDown(SDLK_w))
				taLod++;
		#endif

			const float dt = framework.timeStep;

		#if DO_QUADTREE
			const int cubeSize = cube->m_sides[0].m_quadTree.m_initSize;

			int viewPos[3];

			if (controllerMode == kControl_ViewPos)
			{
				viewPos[0] = +(mouse.x / float(GFX_SX) - .5f) * 1.f * cubeSize + cubeSize/2;
				viewPos[1] = -(mouse.y / float(GFX_SY) - .5f) * 1.f * cubeSize + cubeSize/2;
				viewPos[2] = cubeSize;
			}
			else if (controllerMode == kControl_ViewCam)
			{
				viewPos[0] = cubeSize/2 + std::cos(framework.time / 12.34f) * cubeSize * .1f;
				viewPos[1] = cubeSize/2 + std::cos(framework.time / 23.45f) * cubeSize * .1f;
				//viewPos[2] = cubeSize;

				viewPos[2] = cubeSize/2 + cubeSize*4/3 * (mouse.x / float(GFX_SX));
			}
			else
			{
				viewPos[0] = cubeSize/2 + std::cos(framework.time / 12.34f) * cubeSize * .03f;
				viewPos[1] = cubeSize/2 + std::cos(framework.time / 23.45f) * cubeSize * .02f;
				viewPos[2] = cubeSize;
			}

			if (controllerMode == kControl_FreeCam)
			{
				const float sphereRadius = cube->m_sides[0].m_quadTree.m_initSize/2.f;
				const Vec3 deltaFromSphereCenter = camera.position;
				const float distanceFromSphereCenter = deltaFromSphereCenter.CalcSize();
				const float distanceFromSphereShell = distanceFromSphereCenter - sphereRadius;
				//const float speed = std::abs(distanceFromSphereShell) / sphereRadius * 5000.f + 400.f;
				const float speed = 50.f;
				camera.speed = speed;

				camera.tick(dt);

			#if 1
				viewPos[0] = int(camera.position[0]) + cubeSize/2;
				viewPos[1] = int(camera.position[1]) + cubeSize/2;
				viewPos[2] = int(camera.position[2]) + cubeSize/2;
			#endif
			}
			else if (controllerMode == kControl_ViewPos)
			{
				camera.position.SetZero();
				camera.rotation.SetZero();
			}
			else if (controllerMode == kControl_ViewCam)
			{
				camera.position.SetZero();
				camera.rotation.SetZero();
			}
		#endif

			framework.beginDraw(0, 0, 0, 0);
			{
			#if DO_QUADTREE
				Mat4x4 matP;
				matP.MakePerspectiveLH(Calc::DegToRad(90.f), GFX_SY / float(GFX_SX), 1.f, 100000.f);
				
				Mat4x4 matV;
				if (controllerMode == kControl_ViewCam)
				{
					Vec3 position = Vec3(viewPos[0], viewPos[1], viewPos[2]) - Vec3(cubeSize/2, cubeSize/2, cubeSize/2);
					//position = position * (mouse.x / float(GFX_SX) * 2.f);
					const Vec3 target = Vec3(0.f, 0.f, 0.f);
					matV.MakeLookat(position, target, Vec3(0.f, 1.f, 0.f));
				}
				else
				{
					camera.calculateTransform(matV);
					matV = matV.CalcInv();
				}

				matP = matP * matV;

				gxMatrixMode(GL_PROJECTION);
				gxPushMatrix();
				gxLoadMatrixf(matP.m_v);
				gxMatrixMode(GL_MODELVIEW);
				gxPushMatrix();
				//gxLoadMatrixf(matV.m_v);
				gxLoadIdentity();
				{
#if DO_ADDTIVE_BLENDING
					setBlend(BLEND_ADD);
#else
					glEnable(GL_DEPTH_TEST);
					glDepthFunc(GL_LESS);

					setBlend(BLEND_OPAQUE);
#endif

					QuadParams params;
					params.tree = nullptr;
					params.cube = cube;
					params.cubeSide = nullptr;
					params.viewX = viewPos[0];
					params.viewY = viewPos[1];
					params.viewZ = viewPos[2];
					params.debugLod = lod;
					params.debugCubeSideIndex = debugCubeSideIndex;

					setFont("calibri.ttf");

					cube->eval(params);

					gxPushMatrix();
					{
						const float scale = cubeSize/2;
						gxScalef(scale, scale, scale);

						setColor(255, 255, 255, 63);
						setPlanarTexShader(cube, -1);
						drawFractalSphere();
					}
					gxPopMatrix();

					if (controllerMode != kControl_FreeCam)
					{
						gxPushMatrix();
						{
							gxTranslatef(-cubeSize/2, -cubeSize/2, -cubeSize/2);

							glPointSize(10.f);
							gxBegin(GL_POINTS);
							{
								gxColor4f(1.f, 1.f, .5f, 1.f);
								gxVertex3f(params.viewX, params.viewY, params.viewZ);
							}
							gxEnd();
							glPointSize(1.f);
						}
						gxPopMatrix();
					}

#if !DO_ADDTIVE_BLENDING
					glDisable(GL_DEPTH_TEST);
#endif

					setBlend(BLEND_ALPHA);
				}
				gxMatrixMode(GL_PROJECTION);
				gxPopMatrix();
				gxMatrixMode(GL_MODELVIEW);
				gxPopMatrix();
			#endif

			#if DO_TEXTURE_ARRAY && 0
				Shader shader("TextureArray");
				setShader(shader);
				shader.setTextureArray("texture", 0, cube->m_texture);
				const float t = framework.time / 5.f;
				shader.setImmediate("textureIndex", int(t) % 6);
				shader.setImmediate("lod", int(t * cube->m_numAllocatedLevels) % cube->m_numAllocatedLevels);
				{
					setColor(colorWhite);
					drawRect(0, 0, GFX_SX, GFX_SY);
				}
				clearShader();
			#endif

#if DO_TEXTURE_ARRAY_RESIDENCY
				setBlend(BLEND_OPAQUE);
				Shader shader("TextureArray");
				setShader(shader);
				shader.setTextureArray("texture", 0, cube->m_residencyMap);
				shader.setImmediate("textureIndex", 0);
				shader.setImmediate("lod", 0);
				{
					drawRect(0, 0, GFX_SX/4, GFX_SY/4);
				}
				clearShader();
				setBlend(BLEND_ALPHA);
#endif

			#if DO_TEXTURE_ARRAY_TEST
				textureArrayObject.randomize();

				Shader shader("TextureArray");
				setShader(shader);
				shader.setTextureArray("texture", 0, textureArrayObject.texture);
				shader.setImmediate("textureIndex", taIndex);
				shader.setImmediate("lod", taLod);
				{
					setColor(colorWhite);
					drawRect(0, 0, GFX_SX, GFX_SY);
				}
				clearShader();
			#endif

				setColor(colorWhite);
				setFont("calibri.ttf");
				drawText(10, 10, 24, +1, +1, "estimated memory usage: %.2f Mb", cube->memUsage() / 1024.f / 1024.f);
			}
			framework.endDraw();
		}

	#if DO_QUADTREE
		delete cube;
		cube = nullptr;
	#endif

		framework.shutdown();
	}

	return 0;
}