#include "framework.h"
#include "objects/audioSourceVorbis.h"
#include "objects/binauralizer.h"
#include "objects/binaural_cipic.h"
#include "objects/paobject.h"
#include "Quat.h"
#include "soundmix.h"
#include "vfxGraph.h"
#include "vfxNodes/vfxNodeDisplay.h"
#include "video.h"
#include <atomic>

#define MAX_VOLUMES 6
#define MAX_BINAURALIZERS_PER_VOLUME 10 // center + nearest + eight vertices
#define MAX_BINAURALIZERS_TOTAL (MAX_BINAURALIZERS_PER_VOLUME * MAX_VOLUMES)

const int GFX_SX = 1024;
const int GFX_SY = 768;

//

#define NUM_VIDEOCLIPS 3
#define NUM_VFXCLIPS 1

#define DRAW_GRIDS 1
#define DO_SPOKENWORD 0
#define DO_CONTROLWINDOW 0

static const float timeSeed = 1234.f;

static const char * audioFilenames[NUM_VIDEOCLIPS] =
{
	"0.1.ogg",
	"1.1.ogg",
	"2.1.ogg",
};

static const float audioGains[NUM_VIDEOCLIPS] =
{
	1.f,
	.3f,
	1.f
};

static const char * videoFilenames[NUM_VIDEOCLIPS] =
{
	"0.1280px.mp4",
	"1.1280px.mp4",
	"2.1280px.mp4",
};

//

struct MyMutex : binaural::Mutex
{
	SDL_mutex * mutex;
	
	MyMutex(SDL_mutex * _mutex)
		: mutex(_mutex)
	{
	}
	
	virtual void lock() override
	{
		SDL_LockMutex(mutex);
	}
	
	virtual void unlock() override
	{
		SDL_UnlockMutex(mutex);
	}
};

struct MultiChannelAudioSource
{
	virtual void generate(const int channelIndex, SAMPLE_ALIGN16 float * __restrict audioBuffer, const int numSamples) = 0;
};

struct MultiChannelAudioSource_SoundVolume : MultiChannelAudioSource
{
	binaural::Mutex * mutex;
	
	binaural::Binauralizer binauralizer[MAX_BINAURALIZERS_PER_VOLUME];
	float gain[MAX_BINAURALIZERS_PER_VOLUME];
	
	AudioSource * source;
	
	ALIGN16 float samplesL[AUDIO_UPDATE_SIZE];
	ALIGN16 float samplesR[AUDIO_UPDATE_SIZE];
	
	MultiChannelAudioSource_SoundVolume()
		: mutex(nullptr)
		, binauralizer()
		, source(nullptr)
	{
		memset(gain, 0, sizeof(gain));
	}
	
	void init(const binaural::HRIRSampleSet * sampleSet, binaural::Mutex * _mutex)
	{
		mutex = _mutex;
		
		for (int i = 0; i < MAX_BINAURALIZERS_PER_VOLUME; ++i)
			binauralizer[i].init(sampleSet, _mutex);
	}
	
	void fillBuffers_naive(const int numSamples)
	{
		// generate source audio
		
		memset(samplesL, 0, sizeof(samplesL));
		memset(samplesR, 0, sizeof(samplesR));
	
		ALIGN16 float sourceBuffer[AUDIO_UPDATE_SIZE];
		source->generate(sourceBuffer, numSamples);
		
		// run binauralizers over the source audio
		
		for (int i = 0; i < MAX_BINAURALIZERS_PER_VOLUME; ++i)
		{
			binauralizer[i].provide(sourceBuffer, numSamples);
			
			ALIGN16 float tempL[AUDIO_UPDATE_SIZE];
			ALIGN16 float tempR[AUDIO_UPDATE_SIZE];
			
			float gainCopy;
			
			binauralizer[i].mutex->lock();
			{
				binauralizer[i].generateLR(tempL, tempR, numSamples);
				
				gainCopy = gain[i];
			}
			binauralizer[i].mutex->unlock();
			
			if (gainCopy != 0.f)
			{
				audioBufferAdd(samplesL, tempL, numSamples, gainCopy);
				audioBufferAdd(samplesR, tempR, numSamples, gainCopy);
			}
		}
	}
	
	void fillBuffers_optimized(const int numSamples)
	{
		// generate source audio
		
		memset(samplesL, 0, sizeof(samplesL));
		memset(samplesR, 0, sizeof(samplesR));
	
		ALIGN16 float sourceBuffer[AUDIO_UPDATE_SIZE];
		source->generate(sourceBuffer, numSamples);
		
		// fetch all of the HRIR sample datas we need based on the current elevation and azimuth pairs
		
		const binaural::HRIRSampleData * samples[3 * MAX_BINAURALIZERS_PER_VOLUME];
		float sampleWeights[3 * MAX_BINAURALIZERS_PER_VOLUME];
		float gainCopy[MAX_BINAURALIZERS_PER_VOLUME];
		
		mutex->lock();
		{
			memcpy(gainCopy, gain, sizeof(gainCopy));
			
			for (int i = 0; i < MAX_BINAURALIZERS_PER_VOLUME; ++i)
			{
				binauralizer[i].sampleSet->lookup_3(
					binauralizer[i].sampleLocation.elevation,
					binauralizer[i].sampleLocation.azimuth,
					samples + i * 3,
					sampleWeights + i * 3);
			}
		}
		mutex->unlock();
		
		// combine individual HRIRs into one combined HRIR
		
		binaural::HRIRSampleData combinedHrir;
		memset(&combinedHrir, 0, sizeof(combinedHrir));
		
		const binaural::HRIRSampleData ** samplesPtr = samples;
		float * sampleWeightsPtr = sampleWeights;
		
		for (int i = 0; i < MAX_BINAURALIZERS_PER_VOLUME; ++i)
		{
			const float gain = gainCopy[i];
			
			for (int j = 0; j < 3; ++j)
			{
				audioBufferAdd(combinedHrir.lSamples, samplesPtr[j]->lSamples, binaural::HRIR_BUFFER_SIZE, sampleWeightsPtr[j] * gain);
				audioBufferAdd(combinedHrir.rSamples, samplesPtr[j]->rSamples, binaural::HRIR_BUFFER_SIZE, sampleWeightsPtr[j] * gain);
			}
			
			samplesPtr += 3;
			sampleWeightsPtr += 3;
		}
		
		// run the binauralizer over the source audio
		
		binauralizer[0].provide(sourceBuffer, numSamples);
		
		binauralizer[0].generateLR(samplesL, samplesR, numSamples, &combinedHrir);
	}
	
	virtual void generate(const int channelIndex, SAMPLE_ALIGN16 float * __restrict audioBuffer, const int numSamples) override
	{
		Assert(channelIndex < 2);
		Assert(numSamples <= AUDIO_UPDATE_SIZE);
		
		if (source == nullptr)
		{
			memset(audioBuffer, 0, numSamples * sizeof(float));
			return;
		}
		
		//
		
		if (channelIndex == 0)
		{
			fillBuffers_optimized(numSamples);
		}
		
		//
		
		if (channelIndex == 0)
		{
			audioBufferAdd(audioBuffer, samplesL, numSamples);
		}
		else
		{
			audioBufferAdd(audioBuffer, samplesR, numSamples);
		}
	}
};

struct SoundVolume
{
	Mat4x4 transform;
	
	SoundVolume()
		: transform(true)
	{
	}
	
	Vec3 projectToSound(Vec3Arg v) const
	{
		return transform.CalcInv().Mul4(v);
	}
	
	Vec3 projectToWorld(Vec3Arg v) const
	{
		return transform.Mul4(v);
	}
	
	Vec3 nearestPointWorld(Vec3Arg targetWorld) const
	{
		const Vec3 target = projectToSound(targetWorld);
		
		const float x = std::max(-1.f, std::min(+1.f, target[0]));
		const float y = std::max(-1.f, std::min(+1.f, target[1]));
		const float z = std::max(-1.f, std::min(+1.f, target[2]));
		
		return projectToWorld(Vec3(x, y, z));
	}
};

struct MyPortAudioHandler : PortAudioHandler
{
	MultiChannelAudioSource_SoundVolume audioSources[MAX_VOLUMES];
	
	AudioSource * audioSource_spokenWord;
	
	MyPortAudioHandler()
		: PortAudioHandler()
		, audioSources()
		, audioSource_spokenWord(nullptr)
	{
	}
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		const int numInputChannels,
		void * outputBuffer,
		const int framesPerBuffer) override
	{
		ALIGN16 float channelL[AUDIO_UPDATE_SIZE];
		ALIGN16 float channelR[AUDIO_UPDATE_SIZE];
		
		memset(channelL, 0, sizeof(channelL));
		memset(channelR, 0, sizeof(channelR));
		
		for (int i = 0; i < MAX_VOLUMES; ++i)
		{
			audioSources[i].generate(0, channelL, AUDIO_UPDATE_SIZE);
			audioSources[i].generate(1, channelR, AUDIO_UPDATE_SIZE);
		}
		
		ALIGN16 float spokenWord[AUDIO_UPDATE_SIZE];
		if (audioSource_spokenWord != nullptr)
			audioSource_spokenWord->generate(spokenWord, AUDIO_UPDATE_SIZE);
		else
			memset(spokenWord, 0, sizeof(spokenWord));
		
		float * __restrict destinationBuffer = (float*)outputBuffer;
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			destinationBuffer[i * 2 + 0] = channelL[i] + spokenWord[i];
			destinationBuffer[i * 2 + 1] = channelR[i] + spokenWord[i];
		}
	}
};

static void drawSoundVolume(const SoundVolume & volume)
{
	gxPushMatrix();
	{
		gxMultMatrixf(volume.transform.m_v);
		
		gxPushMatrix(); { gxTranslatef(-1, 0, 0); drawGrid3dLine(10, 10, 1, 2); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(+1, 0, 0); drawGrid3dLine(10, 10, 1, 2); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, -1, 0); drawGrid3dLine(10, 10, 2, 0); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, +1, 0); drawGrid3dLine(10, 10, 2, 0); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, 0, -1); drawGrid3dLine(10, 10, 0, 1); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, 0, +1); drawGrid3dLine(10, 10, 0, 1); } gxPopMatrix();
	}
	gxPopMatrix();
}

static const Vec3 s_cubeVertices[8] =
{
	Vec3(-1, -1, -1),
	Vec3(+1, -1, -1),
	Vec3(+1, +1, -1),
	Vec3(-1, +1, -1),
	Vec3(-1, -1, +1),
	Vec3(+1, -1, +1),
	Vec3(+1, +1, +1),
	Vec3(-1, +1, +1)
};

float videoClipBlend[3] =
{
	1.f, 0.f, 1.f
};

struct Videoclip
{
	enum EvalMode
	{
		EvalMode_Floating,
		EvalMode_Line,
		EvalMode_Circle
	};
	
	Mat4x4 transform;
	AudioSourceVorbis soundSource;
	SoundVolume soundVolume;
	MediaPlayer mp;
	float gain;
	
	int index;
	
	Videoclip()
		: transform()
		, soundSource()
		, soundVolume()
		, mp()
		, gain(0.f)
		, index(-1)
	{
	}
	
	void init(const int _index, const char * audio, const char * video, const float _gain)
	{
		index = _index;
		
		soundSource.open(audio, true);
		
		mp.openAsync(video, MP::kOutputMode_RGBA);
		
		gain = _gain;
	}
	
	float intersectRayWithPlane(Vec3Arg pos, Vec3Arg dir, Vec3 & p) const
	{
		auto & soundToWorld = soundVolume.transform;
		const Mat4x4 worldToSound = soundToWorld.CalcInv();
	
		const Vec3 pos_sound = worldToSound.Mul4(pos);
		const Vec3 dir_sound = worldToSound.Mul3(dir);
	
		const float d = pos_sound[2];
		const float dd = dir_sound[2];
	
		const float t = - d / dd;
		
		p = pos_sound + dir_sound * t;
		
		return t;
	}
	
	bool isInside(Vec3Arg pos_sound) const
	{
		return
			pos_sound[0] >= -1.f && pos_sound[0] <= +1.f &&
			pos_sound[1] >= -1.f && pos_sound[1] <= +1.f;
	}
	
	void evalVideoClipParams(const EvalMode mode, Vec3 & scale, Quat & rotation, Vec3 & position, float & opacity) const
	{
		const float t = ((index + .5f) / float(MAX_VOLUMES) - .5f) * 2.f;
		
		switch (mode)
		{
		case EvalMode_Floating:
			{
				const float time = timeSeed + framework.time;
		
				const float moveSpeed = (1.f + index / float(NUM_VIDEOCLIPS)) * .2f;
				const float moveAmount = 4.f / (index / float(NUM_VIDEOCLIPS) + 1);
				const float x = std::sin(moveSpeed * time / 11.234f) * moveAmount;
				const float y = std::sin(moveSpeed * time / 13.456f) * moveAmount;
				const float z = std::sin(moveSpeed * time / 15.678f) * moveAmount;
			
				const float scaleSpeed = 1.f + index / 5.f;
				const float scaleY = lerp(.5f, 1.f, (std::cos(scaleSpeed * time / 4.567f) + 1.f) / 2.f);
				const float scaleX = scaleY * 4.f/3.f;
				//const float scaleZ = lerp(.05f, .5f, (std::cos(scaleSpeed * time / 8.765f) + 1.f) / 2.f);
				const float scaleZ = lerp(.05f, .1f, (std::cos(scaleSpeed * time / 8.765f) + 1.f) / 2.f);
			
				const float rotateSpeed = 1.f + index / 5.f;
				
				const Mat4x4 transform = Mat4x4(true).RotateX(rotateSpeed * time / 3.456f).RotateY(rotateSpeed * time / 4.567f);
				
				scale = Vec3(scaleX, scaleY, scaleZ);
				rotation.fromMatrix(transform);
				position = Vec3(x, y, z);
				opacity = 1.f;
			}
			break;
			
		case EvalMode_Line:
			{
				scale = Vec3(1, 1, 1);
				rotation.makeIdentity();
				const float x = t * 4.f;
				const float y = 0.f;
				position = Vec3(x, y, 0);
				opacity = 1.f;
			}
			break;
			
		case EvalMode_Circle:
			{
				scale = Vec3(1, 1, 1);
				rotation.makeIdentity();
				const float x = std::cos(t * M_PI * 2.f) * 4.f;
				const float y = std::sin(t * M_PI * 2.f) * 4.f;
				position = Vec3(x, y, 0);
				opacity = 1.f;
			}
			break;
		}
	}
	
	void tick(const float dt)
	{
		Vec3 totalScale(0.f, 0.f, 0.f);
		Quat totalRotation(0.f, 0.f, 0.f, 0.f);
		Vec3 totalTranslation(0.f, 0.f, 0.f);
		float totalOpacity = 0.f;
		float totalBlend = 0.f;
		
		for (int i = 0; i < 3; ++i) // todo : count
		{
			const EvalMode evalMode = (EvalMode)i;
			const float blend = videoClipBlend[i];
			
			Vec3 scale;
			Quat rotation;
			Vec3 position;
			float opacity;
			evalVideoClipParams(evalMode, scale, rotation, position, opacity);
			
			totalScale += scale * blend;
			totalRotation += rotation * blend;
			totalTranslation += position * blend;
			totalOpacity += opacity * blend;
			
			totalBlend += blend;
		}
		
		if (totalBlend != 0.f)
		{
			totalScale /= totalBlend;
			totalRotation /= totalBlend;
			totalTranslation /= totalBlend;
			totalOpacity /= totalBlend;
		}
		
		transform = Mat4x4(true).
			Translate(totalTranslation).
			Rotate(totalRotation).
			Scale(totalScale);
	
		// update media player
		
		const double newPresentTime = soundSource.samplePosition / float(soundSource.sampleRate);
		
		if (newPresentTime < mp.presentTime)
		{
			auto openParams = mp.context->openParams;
			
			mp.close(false);
			
			mp.openAsync(openParams);
		}
		
		mp.presentTime = newPresentTime;
		
		mp.tick(mp.context, true);
	#if 0
		mp.presentTime += dt;
		
		mp.tick(mp.context, true);
		
		if (mp.presentedLastFrame(mp.context))
		{
			auto openParams = mp.context->openParams;
			
			mp.close(false);
			
			mp.openAsync(openParams);
		}
	#endif
	}
	
	void drawSolid(const bool hover)
	{
		gxPushMatrix();
		{
			gxMultMatrixf(soundVolume.transform.m_v);
			
			gxSetTexture(mp.getTexture());
			{
				setLumi(hover ? 255 : 200);
				drawRect(-1, -1, +1, +1);
			}
			gxSetTexture(0);
		}
		gxPopMatrix();
	}
	
	void drawTranslucent()
	{
	#if DRAW_GRIDS
		setColor(200, 200, 200, 63);
		drawSoundVolume(soundVolume);
	#endif
	}
};

struct Vfxclip
{
	SoundVolume soundVolume;
	VfxGraph * vfxGraph;
	
	Vfxclip()
		: soundVolume()
		, vfxGraph(nullptr)
	{
	}
	
	~Vfxclip()
	{
		delete vfxGraph;
		vfxGraph = nullptr;
	}
	
	void open(const char * filename)
	{
		GraphEdit_TypeDefinitionLibrary typeDefinitionLibrary;
		createVfxTypeDefinitionLibrary(typeDefinitionLibrary);
		
		Graph graph;
		
		if (graph.load(filename, &typeDefinitionLibrary))
		{
			vfxGraph = constructVfxGraph(graph, &typeDefinitionLibrary);
		}
	}
	
	float intersectRayWithPlane(Vec3Arg pos, Vec3Arg dir, Vec3 & p) const
	{
		auto & soundToWorld = soundVolume.transform;
		const Mat4x4 worldToSound = soundToWorld.CalcInv();
	
		const Vec3 pos_sound = worldToSound.Mul4(pos);
		const Vec3 dir_sound = worldToSound.Mul3(dir);
	
		const float d = pos_sound[2];
		const float dd = dir_sound[2];
	
		const float t = - d / dd;
		
		p = pos_sound + dir_sound * t;
		
		return t;
	}
	
	bool isInside(Vec3Arg pos_sound) const
	{
		return
			pos_sound[0] >= -1.f && pos_sound[0] <= +1.f &&
			pos_sound[1] >= -1.f && pos_sound[1] <= +1.f;
	}
	
	void tick(const float dt)
	{
		vfxGraph->tick(1024, 1024, dt);
		
		vfxGraph->traverseDraw(1024, 1024);
	}
	
	void drawSolid(const bool hover)
	{
		gxPushMatrix();
		{
			gxMultMatrixf(soundVolume.transform.m_v);
			
			const VfxNodeDisplay * displayNode = vfxGraph->getMainDisplayNode();
			
			const GLuint texture = displayNode ? displayNode->getImage()->getTexture() : 0;
			
			gxSetTexture(texture);
			{
				setLumi(255);
				drawRect(-1, -1, +1, +1);
			}
			gxSetTexture(0);
		}
		gxPopMatrix();
	}
	
	void drawTranslucent()
	{
	#if DRAW_GRIDS
		setColor(0, 0, 0, 100);
		drawSoundVolume(soundVolume);
	#endif
	}
};

#if DO_SPOKENWORD

#include "FileStream.h"
#include "StreamReader.h"
#include "StringEx.h"

struct SpokenWord
{
	std::vector<std::string> lines;
	
	float scrollT;
	
	AudioSourceVorbis soundSource;
	
	SpokenWord()
		: lines()
		, scrollT(0.f)
		, soundSource()
	{
	}
	
	void open(const char * text, const char * audio)
	{
		try
		{
			FileStream stream(text, (OpenMode)(OpenMode_Read | OpenMode_Text));
			StreamReader reader(&stream, false);
			lines = reader.ReadAllLines();
			for (auto & line : lines)
				line = String::Trim(line);
		}
		catch (std::exception & e)
		{
			logError("failed to read text: %s", e.what());
		}
		
		soundSource.open(audio, false);
	}
	
	void tick(const float dt)
	{
		scrollT -= dt / 530.f;
	}
	
	void draw()
	{
		const int kLineSpacing = 7;
		const int fontSize = 18;
		
		float sx[1024];
		float sy[1024];
		
		for (size_t i = 0; i < lines.size(); ++i)
		{
			auto & line = lines[i];
			measureTextArea(fontSize, GFX_SX*2/3, sx[i], sy[i], "%s", line.c_str());
		}
		
		int totalSy = 0;
		for (size_t i = 0; i < lines.size(); ++i)
			totalSy += sy[i] + kLineSpacing;
		
		const float scrollY = scrollT * totalSy;
		
		gxPushMatrix();
		{
			gxTranslatef(0, scrollY, 0);
			
			const int x1 = 60;
			const int y1 = 200;
			
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			{
				int x = 60;
				int y = y1;
				
				setColor(0, 0, 0, 100);
				for (size_t i = 0; i < lines.size(); ++i)
				{
					auto & line = lines[i];
					
					if (!line.empty())
						hqFillRoundedRect(x - 7, y - 3, x + sx[i] + 7, y + sy[i] + 3, 4.f);
					
					y += sy[i] + kLineSpacing;
				}
			}
			hqEnd();

			setColor(colorWhite);
			beginTextBatch();
			{
				int x = x1;
				int y = y1;
				
				for (size_t i = 0; i < lines.size(); ++i)
				{
					auto & line = lines[i];
					
					const float d = fabsf(y + scrollY - 200.f);
					const float l = powf(fmaxf(0.f, 1.f - d / 600.f), 1.4f);
					
					setLumif(l);
					drawTextArea(x, y, GFX_SX*2/3, fontSize, "%s", line.c_str());
					
					y += sy[i] + kLineSpacing;
				}
			}
			endTextBatch();
		}
		gxPopMatrix();
	}
};

#endif

struct World
{
	Videoclip videoclips[MAX_VOLUMES];
	
	Vfxclip vfxclips[NUM_VFXCLIPS];
	
	void init()
	{
		for (int i = 0; i < MAX_VOLUMES; ++i)
		{
			const int index = i % NUM_VIDEOCLIPS;
			
			const char * audioFilename = audioFilenames[index];
			const char * videoFilename = videoFilenames[index];
			
			videoclips[i].init(i, audioFilename, videoFilename, audioGains[index]);
		}
		
		for (int i = 0; i < NUM_VFXCLIPS; ++i)
		{
			vfxclips[i].open("groooplogo.xml");
		}
	}
	
	int hitTest(Vec3Arg pos, Vec3Arg dir) const
	{
		int result = -1;
		float bestDistance = std::numeric_limits<float>::infinity();
		
		for (int i = 0; i < MAX_VOLUMES; ++i)
		{
			Vec3 p;
			
			const float t = videoclips[i].intersectRayWithPlane(pos, dir, p);
			
			if (t >= 0.f)
			{
				if (videoclips[i].isInside(p))
				{
					//const float distance = pos_sound.CalcSize();
					const float distance = p.CalcSize(); // fixme : this is skewed by scale
					
					if (distance < bestDistance)
					{
						bestDistance = distance;
						result = i;
					}
				}
			}
		}
		
		return result;
	}
	
	void tick(const float dt)
	{
		for (int i = 0; i < MAX_VOLUMES; ++i)
		{
			videoclips[i].tick(dt);
		}
		
		for (int i = 0; i < NUM_VFXCLIPS; ++i)
		{
			vfxclips[i].tick(dt);
		}
	}
	
	void draw(const Camera3d & camera)
	{
		const Vec3 origin = camera.getWorldMatrix().GetTranslation();
		const Vec3 direction = camera.getWorldMatrix().GetAxis(2);
		
		const int hoverIndex = hitTest(origin, direction);
		
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glEnable(GL_LINE_SMOOTH);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		{
			pushBlend(BLEND_OPAQUE);
			{
				for (int i = 0; i < MAX_VOLUMES; ++i)
				{
					const bool hover = (i == hoverIndex);
					
					videoclips[i].drawSolid(hover);
				}
				
				for (int i = 0; i < NUM_VFXCLIPS; ++i)
				{
					vfxclips[i].drawSolid(false);
				}
			}
			popBlend();
		}
		glDisable(GL_DEPTH_TEST);
		
		glEnable(GL_DEPTH_TEST);
		{
			gxPushMatrix();
			{
				gxScalef(10, 10, 10);
				setColor(200, 200, 200, 60);
				drawGrid3dLine(100, 100, 0, 2, true);
			}
			gxPopMatrix();
			
			for (int i = 0; i < MAX_VOLUMES; ++i)
			{
				videoclips[i].drawTranslucent();
			}
		
			for (int i = 0; i < NUM_VFXCLIPS; ++i)
			{
				vfxclips[i].drawTranslucent();
			}
		}
		glDisable(GL_DEPTH_TEST);
	}
};

#if DO_CONTROLWINDOW

struct ControlWindow
{
	Window window;
	
	World & world;
	
	ControlWindow(World & _world)
		: window("Control", 300, 300, false)
		, world(_world)
	{
		window.setPosition(0, 200);
	}
	
	void tick()
	{
		pushWindow(window);
		{
		}
		popWindow();
	}
	
	void draw()
	{
		pushWindow(window);
		{
			framework.beginDraw(255, 0, 0, 0);
			{
				gxPushMatrix();
				{
					gxTranslatef(window.getWidth()/2, window.getHeight()/2, 0);
					gxScalef(10, 10, 10);
					
					hqBegin(HQ_FILLED_CIRCLES, true);
					{
						for (int i = 0; i < MAX_VOLUMES; ++i)
						{
							auto & videoclip = world.videoclips[i];
							auto pos = videoclip.soundVolume.transform.GetTranslation();
							
							setColor(colorWhite);
							hqFillCircle(pos[0], pos[2], 5.f);
						}
					}
					hqEnd();
				}
				gxPopMatrix();
			}
			framework.endDraw();
		}
		popWindow();
	}
};

#endif

static void handleAction(const std::string & action, const Dictionary & args)
{
	if (action == "filedrop")
	{
		const std::string filename = args.getString("file", "");
		
		const GLuint texture = getTexture(filename.c_str());
		
		if (texture != 0)
		{
		
		}
	}
}

int main(int argc, char * argv[])
{
	framework.actionHandler = handleAction;
	
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	const int kFontSize = 16;
	const int kFontSize2 = 10;
	
	bool enableNearest = true;
	bool enableVertices = true;
	
	bool showUi = false;
	
	Camera3d camera;
	camera.gamepadIndex = 0;
	
	const float kMoveSpeed = .2f;
	camera.maxForwardSpeed *= kMoveSpeed;
	camera.maxUpSpeed *= kMoveSpeed;
	camera.maxStrafeSpeed *= kMoveSpeed;
	
	camera.position[0] = 0;
	camera.position[1] = +.3f;
	camera.position[2] = -1.f;
	camera.pitch = 10.f;
	
	float fov = 90.f;
	float near = .01f;
	float far = 100.f;
	
	SDL_mutex * audioMutex = SDL_CreateMutex();
	
	World world;
	
	world.init();
	
#if DO_SPOKENWORD
	SpokenWord spokenWord;
	
	// 8:49 ~= 530 seconds
	spokenWord.open("wiekspreekt.txt", "wiekspreekt.ogg");
#endif

	MyMutex binauralMutex(audioMutex);
	
	binaural::HRIRSampleSet sampleSet;
	binaural::loadHRIRSampleSet_Cipic("subject147", sampleSet);
	sampleSet.finalize();
	
	MyPortAudioHandler * paHandler = new MyPortAudioHandler();
	
	int audioSourceIndex = 0;
	
	for (int i = 0; i < MAX_VOLUMES; ++i)
	{
		paHandler->audioSources[audioSourceIndex].init(&sampleSet, &binauralMutex);
		
		paHandler->audioSources[audioSourceIndex].source = &world.videoclips[i].soundSource;
		
		audioSourceIndex++;
	}
	
#if DO_SPOKENWORD
	paHandler->audioSource_spokenWord = &spokenWord.soundSource;
#endif

	PortAudioObject pa;
	pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, paHandler);
	
#if DO_CONTROLWINDOW
	ControlWindow controlWindow(world);
#endif
	
	do
	{
		framework.process();

		const float dt = framework.timeStep;
		
		// process input
		
		if (keyboard.wentDown(SDLK_n))
			enableNearest = !enableNearest;
		
		if (keyboard.wentDown(SDLK_v))
			enableVertices = !enableVertices;
		
		if (keyboard.wentDown(SDLK_TAB))
			showUi = !showUi;
		
		// update the camera
		
		camera.tick(dt, true);
		
	#if 0
		videoClipBlend[0] = (1.f + std::cos(framework.time / 3.4f)) / 2.f;
		videoClipBlend[1] = (1.f + std::cos(framework.time / 4.56f)) / 2.f;
		videoClipBlend[2] = (1.f + std::cos(framework.time / 5.67f)) / 2.f;
	#endif
	
		// update video clips
		
		world.tick(dt);
		
		// update sound volume properties
		
		for (int i = 0; i < MAX_VOLUMES; ++i)
		{
			auto & soundVolume = world.videoclips[i].soundVolume;
			
			soundVolume.transform = world.videoclips[i].transform;
		}
		
		// gather HRTF sampling points
		
		Vec3 samplePoints[MAX_BINAURALIZERS_TOTAL];
		Vec3 samplePointsView[MAX_BINAURALIZERS_TOTAL];
		float samplePointsAmount[MAX_BINAURALIZERS_TOTAL];
		int numSamplePoints = 0;
		
		const Vec3 pCameraWorld = camera.getWorldMatrix().GetTranslation();
		const Vec3 pSoundWorld = world.videoclips[0].soundVolume.transform.GetTranslation();
		const Vec3 pSoundView = camera.getViewMatrix().Mul4(pSoundWorld);
		
		for (int i = 0; i < MAX_VOLUMES; ++i)
		{
			auto & videoclip = world.videoclips[i];
			auto & soundVolume = videoclip.soundVolume;
			
			Vec3 svSamplePoints[MAX_BINAURALIZERS_PER_VOLUME];
			int numSvSamplePoints = 0;
			
			if (enableNearest)
			{
				const Vec3 nearestPointWorld = soundVolume.nearestPointWorld(pCameraWorld);
				
				svSamplePoints[numSvSamplePoints++] = nearestPointWorld;
			}
			
			if (enableVertices)
			{
				for (int j = 0; j < 8; ++j)
				{
					const Vec3 pWorld = soundVolume.projectToWorld(s_cubeVertices[j]);
					
					svSamplePoints[numSvSamplePoints++] = pWorld;
				}
			}
			
			binauralMutex.lock();
			{
				// activate the binauralizers for the generated sample points
				
				for (int j = 0; j < numSvSamplePoints; ++j)
				{
					const Vec3 & pWorld = svSamplePoints[j];
					const Vec3 pView = camera.getViewMatrix().Mul4(pWorld);
					
					const float distanceToHead = pView.CalcSize();
					const float kDistanceToHeadTreshold = .1f; // 10cm. related to head size, but exact size is subjective
					
					const float fadeAmount = std::min(1.f, distanceToHead / kDistanceToHeadTreshold);
					
					float elevation;
					float azimuth;
					binaural::cartesianToElevationAndAzimuth(pView[2], pView[1], pView[0], elevation, azimuth);
					
					// morph to an elevation and azimuth of (0, 0) as the sound gets closer to the center of the head
					// perhaps we should add a dry-wet mix instead .. ?
					elevation = lerp(0.f, elevation, fadeAmount);
					azimuth = lerp(0.f, azimuth, fadeAmount);
					
					const float kMinDistanceToEar = .2f;
					const float clampedDistanceToEar = std::max(kMinDistanceToEar, distanceToHead);
					
					//const float gain = videoclip.gain / clampedDistanceToEar;
					const float gain = videoclip.gain / (clampedDistanceToEar * clampedDistanceToEar);
					
					paHandler->audioSources[i].binauralizer[j].setSampleLocation(elevation, azimuth);
					paHandler->audioSources[i].gain[j] = gain / numSvSamplePoints;
					
					//
					
					samplePoints[numSamplePoints] = pWorld;
					samplePointsView[numSamplePoints] = pView;
					samplePointsAmount[numSamplePoints] = fadeAmount;
					numSamplePoints++;
				}
				
				// reset and mute the unused binauralizers
				
				for (int j = numSvSamplePoints; j < MAX_BINAURALIZERS_PER_VOLUME; ++j)
				{
					paHandler->audioSources[i].binauralizer[j].setSampleLocation(0, 0);
					paHandler->audioSources[i].gain[j] = 0.f;
				}
			}
			binauralMutex.unlock();
		}
		
		Assert(numSamplePoints <= MAX_BINAURALIZERS_TOTAL);
		
	#if DO_SPOKENWORD
		spokenWord.tick(dt);
	#endif
		
	#if DO_CONTROLWINDOW
		controlWindow.tick();
		
		controlWindow.draw();
	#endif
	
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			
			projectPerspective3d(fov, near, far);
			
			camera.pushViewMatrix();
			{
				world.draw(camera);
			}
			camera.popViewMatrix();
			
			projectScreen2d();
			
		#if DO_SPOKENWORD
			spokenWord.draw();
		#endif
			
			if (showUi)
			{
				setColor(colorWhite);
				drawText(10, 10, kFontSize, +1, +1, "sound world pos: %.2f, %.2f, %.2f", pSoundWorld[0], pSoundWorld[1], pSoundWorld[2]);
				drawText(10, 30, kFontSize, +1, +1, "sound view pos: %.2f, %.2f, %.2f", pSoundView[0], pSoundView[1], pSoundView[2]);
				drawText(10, 50, kFontSize, +1, +1, "camera world pos: %.2f, %.2f, %.2f", pCameraWorld[0], pCameraWorld[1], pCameraWorld[2]);
				
				beginTextBatch();
				{
					int x = 10;
					int y = 100;
					
					for (int i = 0; i < numSamplePoints; ++i)
					{
						setColor(200, 200, 200);
						drawText(x, y, kFontSize2, +1, +1,
							"sample pos: (%+.2f, %+.2f, %+.2f, world), (%+.2f, %+.2f, %+.2f, view), amount: %.2f",
							samplePoints[i][0], samplePoints[i][1], samplePoints[i][2],
							samplePointsView[i][0], samplePointsView[i][1], samplePointsView[i][2],
							samplePointsAmount[i]);
						
						y += 12;
						
						if (((i + 1) % 46) == 0)
						{
							x += 400;
							y = 100;
						}
					}
				}
				endTextBatch();
				
				gxTranslatef(0, GFX_SY - 100, 0);
				setColor(colorWhite);
				drawText(10, 40, kFontSize, +1, +1, "N: toggle use nearest point (%s)", enableNearest ? "on" : "off");
				drawText(10, 60, kFontSize, +1, +1, "V: toggle use vertices (%s)", enableVertices ? "on" : "off");
			}
			
			popFontMode();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_ESCAPE));
	
	pa.shut();
	
	delete paHandler;
	paHandler = nullptr;
	
	SDL_DestroyMutex(audioMutex);
	audioMutex = nullptr;
	
	Font("calibri.ttf").saveCache();
	
	framework.shutdown();
	
	return 0;
}
