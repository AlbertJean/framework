#include "kinect2.h"

#if ENABLE_KINECT2

#include "framework.h"

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/registration.h>
#include <libfreenect2/packet_pipeline.h>
#include <libfreenect2/logger.h>

#include <string>

Kinect2::Kinect2()
	: freenect2(nullptr)
	, device(nullptr)
	, pipeline(nullptr)
	, registration(nullptr)
	, listener(nullptr)
	, hasVideo(false)
	, hasDepth(false)
	, frameMap(nullptr)
	, video(nullptr)
	, depth(nullptr)
	, mutex(nullptr)
	, thread(nullptr)
	, stopThread(false)
{
}

Kinect2::~Kinect2()
{
	shut();
}

bool Kinect2::init()
{
	libfreenect2::setGlobalLogger(libfreenect2::createConsoleLogger(libfreenect2::Logger::Warning));
	
	Assert(freenect2 == nullptr);
	freenect2 = new libfreenect2::Freenect2();
	
	if (freenect2->enumerateDevices() == 0)
	{
		logDebug("no devices enumerated");
		shut();
		return false;
	}
	
	std::string serial;
	
	if (serial.empty())
	{
		serial = freenect2->getDefaultDeviceSerialNumber();
	}
	
	Assert(pipeline == nullptr);
	pipeline = new libfreenect2::OpenCLPacketPipeline();
	
	Assert(device == nullptr);
	device = freenect2->openDevice(serial, pipeline);
	
	if (device == nullptr)
	{
		logError("failed to create device");
		shut();
		return false;
	}
	
	//
	
	Assert(mutex == nullptr);
	Assert(thread == nullptr);
	
	mutex = SDL_CreateMutex();
	thread = SDL_CreateThread(threadMain, "Kinect2 Thread", this);
	
	return true;
}

bool Kinect2::shut()
{
	if (thread != nullptr)
	{
		SDL_LockMutex(mutex);
		{
			stopThread = true;
		}
		SDL_UnlockMutex(mutex);
		
		SDL_WaitThread(thread, nullptr);
		thread = nullptr;
		
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
		
		stopThread = false;
	}
	
	return true;
}

void Kinect2::lockBuffers()
{
	SDL_LockMutex(mutex);
}

void Kinect2::unlockBuffers()
{
	SDL_UnlockMutex(mutex);
}

void Kinect2::threadInit()
{
	const bool doColor = false;
	const bool doDepth = true;
	
	int types = 0;
	
	if (doColor)
		types |= libfreenect2::Frame::Color;
	if (doDepth)
		types |= libfreenect2::Frame::Ir | libfreenect2::Frame::Depth;
	
	Assert(listener == nullptr);
	listener = new libfreenect2::SyncMultiFrameListener(types);
	
	if (doColor)
		device->setColorFrameListener(listener);
	
	if (doDepth)
		device->setIrAndDepthFrameListener(listener);
	
	if (doColor && doDepth)
	{
		if (!device->start())
		{
			logError("failed to start device");
			shut();
			return;
		}
	}
	else
	{
		if (!device->startStreams(doColor, doDepth))
		{
			logError("failed to start device");
			shut();
			return;
		}
	}
	
	logDebug("device serial: %s", device->getSerialNumber().c_str());
	logDebug("device firmware: %s", device->getFirmwareVersion().c_str());
	
	Assert(registration == nullptr);
	registration = new libfreenect2::Registration(
		device->getIrCameraParams(),
		device->getColorCameraParams());
}

void Kinect2::threadShut()
{
	if (frameMap != nullptr)
	{
		listener->release(*(libfreenect2::FrameMap*)frameMap);
		frameMap = nullptr;
	}
	
	if (device != nullptr)
	{
		device->stop();
		device->close();
	}
	
	delete device;
	device = nullptr;
	
	delete listener;
	listener = nullptr;
	
	//delete pipeline;
	//pipeline = nullptr;
	
	delete freenect2;
	freenect2 = nullptr;
}

bool Kinect2::threadProcess()
{
	libfreenect2::Frame undistorted(512, 424, 4);
	libfreenect2::Frame registered(512, 424, 4);

	libfreenect2::FrameMap * frames = new libfreenect2::FrameMap();

	if (!listener->waitForNewFrame(*frames, 10*1000)) // 10 seconds
	{
		logError("timeout!");
		shut();
		return false;
	}

	lockBuffers();
	{
		if (frameMap != nullptr)
		{
			listener->release(*(libfreenect2::FrameMap*)frameMap);
			frameMap = nullptr;
		}
		
		frameMap = frames;
		
		//registration->apply(rgb, depth, &undistorted, &registered);
		
		video = (*frames)[libfreenect2::Frame::Color];
		depth = (*frames)[libfreenect2::Frame::Depth];
		
		hasVideo = video != nullptr;
		hasDepth = depth != nullptr;
		
		//libfreenect2::Frame * ir = (frames)[libfreenect2::Frame::Ir];
	}
	unlockBuffers();
	
	return true;
}

int Kinect2::threadMain(void * userData)
{
	Kinect2 * self = (Kinect2*)userData;
	
	self->threadInit();
	
	for (;;)
	{
		bool stop = false;
		
		SDL_LockMutex(self->mutex);
		{
			stop = self->stopThread;
		}
		SDL_UnlockMutex(self->mutex);
		
		if (stop)
		{
			break;
		}
		
		if (self->threadProcess() == false)
		{
			logDebug("thread process failed");
		}
	}
	
	self->threadShut();
	
	return 0;
}

#endif
