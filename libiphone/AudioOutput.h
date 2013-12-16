#pragma once

#include "libiphone_fwd.h"

class AudioOutput
{
public:
	virtual ~AudioOutput() { }
	
	virtual void Play() = 0;
	virtual void Stop() = 0;
	virtual void Update(AudioStream* stream) = 0;
	virtual void Volume_set(float volume) = 0;
	virtual bool HasFinished_get() = 0;
};

#ifdef BBOS
	#include <AL/AL.h>
#else
	#include <OpenAL/AL.h>
#endif

class AudioOutput_OpenAL : public AudioOutput
{
public:
	AudioOutput_OpenAL();
	
	void Initialize(int numChannels, int sampleRate);
	void Shutdown();
	
	virtual void Play();
	virtual void Stop();
	virtual void Update(AudioStream* stream);
	virtual void Volume_set(float volume);
	virtual bool HasFinished_get();
	
private:
	const static int kBufferSize = 8192 * 2;
	const static int kBufferCount = 3;
	
	void CheckError();
	
	ALuint mFormat;
	ALuint mSampleRate;
	ALuint mBufferIds[kBufferCount];
	ALuint mSourceId;
	
	bool mIsPlaying;
	bool mHasFinished;
	float mVolume;
};
