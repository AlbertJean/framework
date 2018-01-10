#include "audiostream/AudioStreamVorbis.h"
#include "framework.h"
#include "objects/binauralizer.h"
#include "objects/binaural_cipic.h"
#include "objects/paobject.h"
#include "soundmix.h"

#define MAX_BINAURALIZERS 10 // center + nearest + eight vertices

extern const int GFX_SX;
extern const int GFX_SY;

namespace SoundVolumesTest
{

struct AudioSource
{
	virtual void generate(const int channelIndex, float * __restrict audioBuffer, const int numSamples) = 0;
};

struct AudioSource_StreamOgg : AudioSource
{
	AudioStream_Vorbis stream;
	
	AudioSource_StreamOgg()
		: stream()
	{
	}
	
	void load(const char * filename)
	{
		stream.Open(filename, true);
	}
	
	virtual void generate(const int channelIndex, float * __restrict audioBuffer, const int numSamples) override
	{
		AudioSample * samples = (AudioSample*)alloca(sizeof(AudioSample) * numSamples);
		
		const int numProvided = stream.Provide(numSamples, samples);
		
		for (int i = 0; i < numProvided; ++i)
			audioBuffer[i] = (int(samples[i].channel[0]) + int(samples[i].channel[1])) / float(1 << 16);
		
		for (int i = numProvided; i < numSamples; ++i)
			audioBuffer[i] = 0.f;
	}
};

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

struct AudioSource_Binaural : AudioSource
{
	binaural::Binauralizer binauralizer[MAX_BINAURALIZERS];
	float gain[MAX_BINAURALIZERS];
	
	AudioSource * source;
	
	ALIGN16 float samplesL[AUDIO_UPDATE_SIZE];
	ALIGN16 float samplesR[AUDIO_UPDATE_SIZE];
	
	AudioSource_Binaural(const binaural::HRIRSampleSet * sampleSet, binaural::Mutex * mutex)
		: binauralizer()
		, source(nullptr)
	{
		for (int i = 0; i < MAX_BINAURALIZERS; ++i)
			binauralizer[i].init(sampleSet, mutex);
		
		memset(gain, 0, sizeof(gain));
	}
	
	virtual void generate(const int channelIndex, float * __restrict audioBuffer, const int numSamples) override
	{
		Assert(channelIndex < 2);
		Assert(numSamples <= AUDIO_UPDATE_SIZE);
		
		if (source == nullptr)
		{
			memset(audioBuffer, 0, numSamples * sizeof(float));
			return;
		}
		
		if (channelIndex == 0)
		{
			memset(samplesL, 0, sizeof(samplesL));
			memset(samplesR, 0, sizeof(samplesR));
			
			float sourceBuffer[numSamples];
			
			source->generate(0, sourceBuffer, numSamples);
			
			for (int i = 0; i < MAX_BINAURALIZERS; ++i)
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
		
		if (channelIndex == 0)
		{
			memcpy(audioBuffer, samplesL, numSamples * sizeof(float));
		}
		else
		{
			memcpy(audioBuffer, samplesR, numSamples * sizeof(float));
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
	AudioSource * audioSource;
	
	MyPortAudioHandler(AudioSource * _audioSource)
		: PortAudioHandler()
		, audioSource(_audioSource)
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
		
		audioSource->generate(0, channelL, AUDIO_UPDATE_SIZE);
		audioSource->generate(1, channelR, AUDIO_UPDATE_SIZE);
		
		float * __restrict destinationBuffer = (float*)outputBuffer;
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			destinationBuffer[i * 2 + 0] = channelL[i];
			destinationBuffer[i * 2 + 1] = channelR[i];
		}
	}
};

}

using namespace SoundVolumesTest;

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

static void drawPoint(Vec3Arg p, const Color & c1, const Color & c2, const Color & c3, const float size)
{
	gxBegin(GL_LINES);
	{
		setColor(c1);
		gxVertex3f(p[0]-size, p[1], p[2]);
		gxVertex3f(p[0]+size, p[1], p[2]);
		
		setColor(c2);
		gxVertex3f(p[0], p[1]-size, p[2]);
		gxVertex3f(p[0], p[1]+size, p[2]);
		
		setColor(c3);
		gxVertex3f(p[0], p[1], p[2]-size);
		gxVertex3f(p[0], p[1], p[2]+size);
	}
	gxEnd();
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

void testSoundVolumes()
{
	const int kFontSize = 16;
	
	bool enableDistanceAttenuation = true;
	
	bool enableCenter = false;
	bool enableNearest = true;
	bool enableVertices = false;
	
	bool flipUpDown = false;
	
	Camera3d camera;
	
	camera.position[0] = 0;
	camera.position[1] = +.3f;
	camera.position[2] = -1.f;
	camera.pitch = 10.f;
	
	float fov = 90.f;
	float near = .01f;
	float far = 100.f;
	
	SDL_mutex * audioMutex = SDL_CreateMutex();
	
	AudioSource_StreamOgg oggSource;
	oggSource.load("wobbly.ogg");
	
	MyMutex binauralMutex(audioMutex);
	
	binaural::HRIRSampleSet sampleSet;
	binaural::loadHRIRSampleSet_Cipic("hrtf/CIPIC/subject147", sampleSet);
	sampleSet.finalize();
	
	AudioSource_Binaural binauralSource(&sampleSet, &binauralMutex);
	binauralSource.source = &oggSource;
	
	MyPortAudioHandler paHandler(&binauralSource);
	
	PortAudioObject pa;
	pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, &paHandler);
	
	SoundVolume soundVolume;
	
	do
	{
		framework.process();

		const float dt = framework.timeStep;
		
		if (keyboard.wentDown(SDLK_a))
			enableDistanceAttenuation = !enableDistanceAttenuation;
		
		if (keyboard.wentDown(SDLK_c))
		{
			enableCenter = !enableCenter;
			enableNearest = false;
		}
		
		if (keyboard.wentDown(SDLK_n))
		{
			enableNearest = !enableNearest;
			enableCenter = false;
		}
		
		if (keyboard.wentDown(SDLK_v))
			enableVertices = !enableVertices;
		
		if (keyboard.wentDown(SDLK_f))
			flipUpDown = !flipUpDown;
		
		camera.tick(dt, true);
		
		const float scaleXY = lerp(.5f, 1.f, (std::cos(framework.time / 4.567f) + 1.f) / 2.f);
		const float scaleZ = lerp(.05f, .5f, (std::cos(framework.time / 8.765f) + 1.f) / 2.f);
		soundVolume.transform = Mat4x4(true).RotateX(framework.time / 3.456f).RotateY(framework.time / 4.567f).Scale(scaleXY, scaleXY, scaleZ);
		
		Vec3 samplePoints[MAX_BINAURALIZERS];
		int numSamplePoints = 0;
		
		Vec3 samplePointsView[MAX_BINAURALIZERS];
		float samplePointsAmount[MAX_BINAURALIZERS];
		
		const Vec3 pCameraWorld = camera.getWorldMatrix().GetTranslation();
		const Vec3 pSoundWorld = soundVolume.transform.Mul4(Vec3(0.f, 0.f, 0.f));
		const Vec3 pSoundView = camera.getViewMatrix().Mul4(pSoundWorld);
		
		if (enableCenter)
		{
			samplePoints[numSamplePoints++] = pSoundWorld;
		}
		
		const Vec3 nearestPointWorld = soundVolume.nearestPointWorld(pCameraWorld);
		
		if (enableNearest)
		{
			samplePoints[numSamplePoints++] = nearestPointWorld;
		}
		
		if (enableVertices)
		{
			for (int i = 0; i < 8; ++i)
			{
				const Vec3 pWorld = soundVolume.projectToWorld(s_cubeVertices[i]);
				
				samplePoints[numSamplePoints++] = pWorld;
			}
		}
		
		binauralMutex.lock();
		{
			// activate the binauralizers for the generated sample points
			
			for (int i = 0; i < numSamplePoints; ++i)
			{
				const Vec3 & pWorld = samplePoints[i];
				const Vec3 pView = camera.getViewMatrix().Mul4(pWorld);
				
				const float distanceToHead = pView.CalcSize();
				const float kDistanceToHeadTreshold = .1f; // 10cm. related to head size, but exact size is subjective
				
				const float fadeAmount = std::min(1.f, distanceToHead / kDistanceToHeadTreshold);
				
				samplePointsView[i] = pView;
				samplePointsAmount[i] = fadeAmount;
				
				float elevation;
				float azimuth;
				binaural::cartesianToElevationAndAzimuth(pView[2], flipUpDown ? -pView[1] : +pView[1], pView[0], elevation, azimuth);
				
				// morph to an elevation and azimuth of (0, 0) as the sound gets closer to the center of the head
				// perhaps we should add a dry-wet mix instead .. ?
				elevation = lerp(0.f, elevation, fadeAmount);
				azimuth = lerp(0.f, azimuth, fadeAmount);
				
				const float kMinDistanceToEar = .2f;
				const float clampedDistanceToEar = std::max(kMinDistanceToEar, distanceToHead);
				
				//const float gain = enableDistanceAttenuation ? .1f / (clampedDistanceToEar * clampedDistanceToEar) : 1.f;
				const float gain = enableDistanceAttenuation ? .5f / clampedDistanceToEar : 1.f;
				
				binauralSource.binauralizer[i].setSampleLocation(elevation, azimuth);
				binauralSource.gain[i] = gain / numSamplePoints;
			}
			
			// reset and mute the unused binauralizers
			
			for (int i = numSamplePoints; i < MAX_BINAURALIZERS; ++i)
			{
				binauralSource.binauralizer[i].setSampleLocation(0, 0);
				binauralSource.gain[i] = 0.f;
			}
		}
		binauralMutex.unlock();
		
		Assert(numSamplePoints <= MAX_BINAURALIZERS);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			
			projectPerspective3d(fov, near, far);
			
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			
			camera.pushViewMatrix();
			{
				gxPushMatrix();
				{
					gxScalef(10, 10, 10);
					setColor(50, 50, 50);
					drawGrid3dLine(100, 100, 0, 2, true);
				}
				gxPopMatrix();
				
				setColor(160, 160, 160);
				drawSoundVolume(soundVolume);
				
				for (int i = 0; i < numSamplePoints; ++i)
				{
					drawPoint(samplePoints[i], colorRed, colorGreen, colorBlue, .1f);
				}
			}
			camera.popViewMatrix();
			
			projectScreen2d();
			
			setColor(colorWhite);
			drawText(10, 10, kFontSize, +1, +1, "sound world pos: %.2f, %.2f, %.2f", pSoundWorld[0], pSoundWorld[1], pSoundWorld[2]);
			drawText(10, 30, kFontSize, +1, +1, "sound view pos: %.2f, %.2f, %.2f", pSoundView[0], pSoundView[1], pSoundView[2]);
			drawText(10, 50, kFontSize, +1, +1, "camera world pos: %.2f, %.2f, %.2f", pCameraWorld[0], pCameraWorld[1], pCameraWorld[2]);
			
			for (int i = 0; i < numSamplePoints; ++i)
			{
				setColor(200, 200, 200);
				drawText(10, 100 + i * 20, kFontSize, +1, +1, "sample pos: (%+.2f, %+.2f, %+.2f, world), (%+.2f, %+.2f, %+.2f, view), amount: %.2f",
					samplePoints[i][0], samplePoints[i][1], samplePoints[i][2],
					samplePointsView[i][0], samplePointsView[i][1], samplePointsView[i][2],
					samplePointsAmount[i]);
			}
			
			gxTranslatef(0, GFX_SY - 100, 0);
			setColor(colorWhite);
			drawText(10, 0, kFontSize, +1, +1, "A: toggle distance attenuation (%s)", enableDistanceAttenuation ? "on" : "off");
			drawText(10, 20, kFontSize, +1, +1, "C: toggle use center point (%s)", enableCenter ? "on" : "off");
			drawText(10, 40, kFontSize, +1, +1, "N: toggle use nearest point (%s)", enableNearest ? "on" : "off");
			drawText(10, 60, kFontSize, +1, +1, "V: toggle use vertices (%s)", enableVertices ? "on" : "off");
			drawText(10, 80, kFontSize, +1, +1, "F: flip up-down axis for HRTF (%s)", flipUpDown ? "on" : "off");
			
			popFontMode();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_ESCAPE));
	
	pa.shut();
	
	SDL_DestroyMutex(audioMutex);
	
	exit(0);
}
