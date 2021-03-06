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

#include "osc4d.h"
#include "paobject.h"
#include <atomic>
#include <stdint.h>
#include <vector>

//

struct AudioGraphManager;
struct AudioVoiceManager;
struct Osc4DStream;

struct SDL_mutex;

//

extern Osc4DStream * g_oscStream;

//

struct AudioUpdateTask
{
	virtual ~AudioUpdateTask()
	{
	}
	
	virtual void preAudioUpdate(const float dt)
	{
	}
	
	virtual void postAudioUpdate(const float dt)
	{
	}
};

//

struct AudioUpdateHandler : PortAudioHandler
{
	struct Command
	{
		enum Type
		{
			kType_None,
			kType_ForceOscSync
		};
		
		Type type;
		
		Command()
			: type(kType_None)
		{
		}
	};
	
	std::vector<AudioUpdateTask*> updateTasks;
	
	AudioVoiceManager * voiceMgr;
	
	AudioGraphManager * audioGraphMgr;
	
	Osc4DStream * oscStream;
	
	double time;
	
	SDL_mutex * mutex;
	
	std::atomic<int64_t> msecsPerTick;
	std::atomic<int64_t> msecsPerSecond;
	int64_t msecsTotal;
	int64_t nextCpuTimeUpdate;
	
	AudioUpdateHandler();
	virtual ~AudioUpdateHandler();
	
	void init(SDL_mutex * mutex, const char * ipAddress, const int udpPort);
	void shut();
	
	void setOscEndpoint(const char * ipAddress, const int udpPort);
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		const int numInputChannels,
		void * outputBuffer,
		const int framesPerBuffer) override;
};

extern float * g_audioInputChannels;
extern int g_numAudioInputChannels;
