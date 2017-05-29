#pragma once

#include "audiostream/AudioStream.h"
#include "mediaplayer_new/MPContext.h"
#include <stdint.h>

struct MediaPlayer : public AudioStream
{
	struct Context
	{
		Context()
			: mpTickEvent(nullptr)
			, mpTickMutex(nullptr)
			, mpThreadId(-1)
			, hasBegun(false)
			, stopMpThread(false)
			, hasPresentedLastFrame(false)
		{
		}

		~Context()
		{
			if (mpTickEvent)
			{
				SDL_DestroyCond(mpTickEvent);
				mpTickEvent = nullptr;
			}

			if (mpTickMutex)
			{
				SDL_DestroyMutex(mpTickMutex);
				mpTickMutex = nullptr;
			}
		}

		void tick();

		bool presentedLastFrame() const;

		struct OpenParams
		{
			OpenParams()
				: filename()
				, outputMode(MP::kOutputMode_RGBA)
			{
			}
			
			std::string filename;
			MP::OutputMode outputMode;
		};

		OpenParams openParams;

		MP::Context mpContext;
		SDL_cond * mpTickEvent;
		SDL_mutex * mpTickMutex;
		SDL_threadID mpThreadId;

		// hacky messaging between threads
		volatile bool hasBegun;
		volatile bool stopMpThread;
		volatile bool hasPresentedLastFrame;
	};

	Context * context;
	
	MP::VideoFrame * videoFrame;
	uint32_t texture;
	int textureSx;
	int textureSy;
	
	double presentTime;

	int audioChannelCount;
	int audioSampleRate;

	// threading related
	SDL_Thread * mpThread;

	MediaPlayer()
		: context(nullptr)
		, videoFrame(nullptr)
		, texture(0)
		, textureSx(0)
		, textureSy(0)
		, presentTime(-0.0001)
		, audioChannelCount(-1)
		, audioSampleRate(-1)
		// threading related
		, mpThread(0)
	{
	}

	~MediaPlayer()
	{
		close(true);
	}

	void openAsync(const char * filename, const MP::OutputMode outputMode);
	void close(const bool freeTexture);
	void tick(Context * context, const bool wantsTexture);

	bool isActive(Context * context) const;
	bool presentedLastFrame(Context * context) const;
	void seek(const double time);
	
	bool updateVideoFrame();
	void updateTexture();
	void freeTexture();
	uint32_t getTexture() const;
	bool getVideoProperties(int & sx, int & sy, double & duration) const;

	void updateAudio();
	bool getAudioProperties(int & channelCount, int & sampleRate) const;

	virtual int Provide(int numSamples, AudioSample* __restrict buffer) override;

	void startMediaPlayerThread();
	void stopMediaPlayerThread();
};
