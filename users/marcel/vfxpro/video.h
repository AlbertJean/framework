#pragma once

#include "mediaplayer_old/MPContext.h"
#include <stdint.h>

struct MediaPlayer
{
	struct Context
	{
		Context()
			: mpTickEvent(nullptr)
			, mpTickMutex(nullptr)
			, mpBufferLock(nullptr)
			, hasBegun(false)
			, stopMpThread(false)
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

			if (mpBufferLock)
			{
				SDL_DestroyMutex(mpBufferLock);
				mpBufferLock = nullptr;
			}
		}

		struct OpenParams
		{
			std::string filename;
		};

		OpenParams openParams;

		MP::Context mpContext;
		SDL_cond * mpTickEvent;
		SDL_mutex * mpTickMutex;
		SDL_mutex * mpBufferLock;

		// hacky messaging between threads
		volatile bool hasBegun;
		volatile bool stopMpThread;
	};

	Context * context;

	uint32_t texture;
	int textureSx;
	int textureSy;
	double presentTime;
	double seekTime;

	// threading related
	SDL_Thread * mpThread;
	volatile bool stopMpThreadDone;

	MediaPlayer()
		: context(nullptr)
		, texture(0)
		, textureSx(0)
		, textureSy(0)
		, presentTime(-1.0)
		, seekTime(-1.0)
		// threading related
		, mpThread(0)
		, stopMpThreadDone(false)
	{
	}

	~MediaPlayer()
	{
		close();
	}

	void openAsync(const char * filename);
	void close();
	void tickAsync(Context * context);
	void tick(Context * context);

	bool isActive(Context * context) const;
	bool presentedLastFrame(Context * context) const;
	void seek(const double time);

	void updateTexture();
	uint32_t getTexture() const;

	void startMediaPlayerThread();
	void stopMediaPlayerThread();
};
