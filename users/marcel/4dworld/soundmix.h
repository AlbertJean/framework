/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <list>
#include <string>

#define AUDIO_UPDATE_SIZE 256

#define SAMPLE_RATE 44100

#ifdef MACOS
	#define ALIGN16 __attribute__((aligned(16)))
	#define ALIGN32 __attribute__((aligned(32)))
#else
	#define ALIGN16
	#define ALIGN32
#endif

struct Osc4DStream;

struct PcmData
{
	float * samples;
	int numSamples;

	PcmData();
	~PcmData();

	void free();
	void alloc(const int numSamples);

	bool load(const char * filename, const int channel);
};

//

struct AudioSource
{
	virtual void generate(ALIGN16 float * __restrict samples, const int numSamples) = 0;
};

struct AudioSourceMix : AudioSource
{
	struct Input
	{
		AudioSource * source;
		float gain;
	};

	std::list<Input> inputs;
	
	bool normalizeGain;

	AudioSourceMix();

	virtual void generate(ALIGN16 float * __restrict samples, const int numSamples) override;

	Input * add(AudioSource * source, const float gain);
	void remove(Input * input);

	Input * tryGetInput(AudioSource * source);
};

struct AudioSourceSine : AudioSource
{
	float phase;
	float phaseStep;
	
	AudioSourceSine();
	
	void init(const float phase, const float frequency);
	
	virtual void generate(ALIGN16 float * __restrict samples, const int numSamples) override;
};

struct AudioSourcePcm : AudioSource
{
	const PcmData * pcmData;
	
	int samplePosition;
	
	bool isPlaying;
	
	bool hasRange;
	int rangeBegin;
	int rangeEnd;
	
	AudioSourcePcm();
	
	void init(const PcmData * pcmData, const int samplePosition);
	
	void setRange(const int begin, const int length);
	void setRangeNorm(const float begin, const float length);
	void clearRange();
	
	void play();
	void stop();
	void pause();
	void resume();
	void resetSamplePosition();
	void setSamplePosition(const int position);
	void setSamplePositionNorm(const float position);

	virtual void generate(ALIGN16 float * __restrict samples, const int numSamples) override;
};

//

#include "osc4d.h"
#include "paobject.h"
#include "Vec3.h"

struct SDL_mutex;

struct AudioVoice
{
	struct SpatialCompressor
	{
		bool enable = false;
		float attack = 1.f;
		float release = 1.f;
		float minimum = 0.f;
		float maximum = 1.f;
		float curve = 0.f;
		bool invert = false;
		
		bool operator!=(const SpatialCompressor & other) const
		{
			return
				enable != other.enable ||
				attack != other.attack ||
				release != other.release ||
				minimum != other.minimum ||
				maximum != other.maximum ||
				curve != other.curve ||
				invert != other.invert;
		}
	};
	
	struct Doppler
	{
		bool enable = true;
		float scale = 1.f;
		float smooth = .2f;
		
		bool operator!=(const Doppler & other) const
		{
			return
				enable != other.enable ||
				scale != other.scale ||
				smooth != other.smooth;
		}
	};
	
	struct DistanceIntensity
	{
		bool enable = false;
		float threshold = 0.f;
		float curve = 0.f;
		
		bool operator!=(const DistanceIntensity & other) const
		{
			return
				enable != other.enable ||
				threshold != other.threshold ||
				curve != other.curve;
		}
	};
	
	struct DistanceDamping
	{
		bool enable = false;
		float threshold = 0.f;
		float curve = 0.f;
		
		bool operator!=(const DistanceDamping & other) const
		{
			return
				enable != other.enable ||
				threshold != other.threshold ||
				curve != other.curve;
		}
	};
	
	struct DistanceDiffusion
	{
		bool enable = false;
		float threshold = 0.f;
		float curve = 0.f;
		
		bool operator!=(const DistanceDiffusion & other) const
		{
			return
				enable != other.enable ||
				threshold != other.threshold ||
				curve != other.curve;
		}
	};
	
	struct Spatialisation
	{
		Vec3 color;
		std::string name;
		float gain;
		
		Vec3 pos;
		Vec3 size;
		Vec3 rot;
		Osc4D::OrientationMode orientationMode;
		Vec3 orientationCenter;
		
		SpatialCompressor spatialCompressor;
		float articulation;
		Doppler doppler;
		DistanceIntensity distanceIntensity;
		DistanceDamping distanceDampening;
		DistanceDiffusion distanceDiffusion;
		Osc4D::SubBoost subBoost;
		
		bool globalEnable;
		
		Spatialisation()
			: color(1.f, 0.f, 0.f)
			, name()
			, gain(1.f)
			, pos()
			, size(1.f, 1.f, 1.f)
			, rot()
			, orientationMode(Osc4D::kOrientation_Static)
			, orientationCenter(0.f, 2.f, 0.f)
			, spatialCompressor()
			, articulation(0.f)
			, doppler()
			, distanceIntensity()
			, distanceDampening()
			, distanceDiffusion()
			, subBoost(Osc4D::kBoost_None)
			, globalEnable(true)
		{
		}
	};
	
	int channelIndex;
	
	Spatialisation spat;
	Spatialisation lastSentSpat;
	
	AudioSource * source;
	
	AudioVoice()
		: channelIndex(-1)
		, spat()
		, lastSentSpat()
		, source(nullptr)
	{
	}
};

struct AudioVoiceManager : PortAudioHandler
{
	SDL_mutex * mutex;
	
	int numChannels;
	std::list<AudioVoice> voices;
	bool outputMono;
	
	struct Spatialisation
	{
		float globalGain;
		Vec3 globalPos;
		Vec3 globalSize;
		Vec3 globalRot;
		Vec3 globalPlode;
		Vec3 globalOrigin;
		
		Spatialisation()
			: globalGain(1.f)
			, globalPos()
			, globalSize()
			, globalRot()
			, globalPlode(1.f, 1.f, 1.f)
			, globalOrigin()
		{
		}
	};
	
	Spatialisation spat;
	Spatialisation lastSentSpat;
	
	AudioVoiceManager();
	
	void init(const int numChannels);
	void shut();
	
	bool allocVoice(AudioVoice *& voice, AudioSource * source);
	void freeVoice(AudioVoice *& voice);
	
	void updateChannelIndices();
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		void * outputBuffer,
		int framesPerBuffer) override;
	
	bool generateOsc(Osc4DStream & stream);
};

extern AudioVoiceManager * g_voiceMgr;
