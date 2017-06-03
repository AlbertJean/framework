#include "vfxNodeTimeline.h"

#include "tinyxml2.h" // fixme

VfxNodeTimeline::VfxNodeTimeline()
	: VfxNodeBase()
	, time(0.0)
	, isPlaying(false)
	, eventTriggerData()
	, beatTriggerData()
	, timeline()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Timeline, kVfxPlugType_String);
	addInput(kInput_Duration, kVfxPlugType_Float);
	addInput(kInput_Bpm, kVfxPlugType_Float);
	addInput(kInput_Loop, kVfxPlugType_Bool);
	addInput(kInput_AutoPlay, kVfxPlugType_Bool);
	addInput(kInput_Speed, kVfxPlugType_Float);
	addInput(kInput_Play, kVfxPlugType_Trigger);
	addInput(kInput_Pause, kVfxPlugType_Trigger);
	addInput(kInput_Resume, kVfxPlugType_Trigger);
	addInput(kInput_Time, kVfxPlugType_Float);
	addOutput(kOutput_EventTrigger, kVfxPlugType_Trigger, &eventTriggerData);
	addOutput(kOutput_BeatTrigger, kVfxPlugType_Trigger, &beatTriggerData);
}

void VfxNodeTimeline::tick(const float dt)
{
	const std::string & timelineText = getInputString(kInput_Timeline, "");
	const double duration = getInputFloat(kInput_Duration, 0.f);
	const double bpm = getInputFloat(kInput_Bpm, 0.f);
	const bool loop = getInputBool(kInput_Loop, true);
	const double speed = getInputFloat(kInput_Speed, 1.f);
	
	timeline = VfxTimeline();
	
	tinyxml2::XMLDocument d;
	
	if (d.Parse(timelineText.c_str()) == tinyxml2::XML_SUCCESS)
	{
		tinyxml2::XMLElement * e = d.FirstChildElement();
		
		if (e != nullptr)
			timeline.load(e);
	}
	
	if (isPlaying)
	{
		double oldTime = time;
		
		double newTime;
		
		if (tryGetInput(kInput_Time)->isConnected())
		{
			newTime = getInputFloat(kInput_Time, 0.f);
		}
		else
		{
			newTime = time + dt * speed;
		}
		
		if (tryGetInput(kInput_Duration)->isConnected() && duration > 0.0)
		{
			while (newTime < 0.0)
			{
				handleTimeSegment(oldTime, 0.0, bpm);
				
				if (loop)
				{
					newTime += duration;
					
					oldTime = duration;
				}
				else
				{
					newTime = 0.0;
					
					oldTime = 0.0;
				}
			}
			
			while (newTime >= duration)
			{
				handleTimeSegment(oldTime, duration, bpm);
				
				if (loop)
				{
					newTime -= duration;
					
					oldTime = 0.0;
				}
				else
				{
					newTime = duration;
					
					oldTime = duration;
				}
			}
		}
		
		handleTimeSegment(oldTime, newTime, bpm);
		
		time = newTime;
	}
}

void VfxNodeTimeline::init(const GraphNode & node)
{
	const bool autoPlay = getInputBool(kInput_AutoPlay, true);
	
	if (autoPlay)
	{
		play();
	}
}

void VfxNodeTimeline::handleTrigger(const int srcSocketIndex, const VfxTriggerData & data)
{
	if (srcSocketIndex == kInput_Play)
	{
		play();
	}
	else if (srcSocketIndex == kInput_Pause)
	{
		pause();
	}
	else if (srcSocketIndex == kInput_Resume)
	{
		resume();
	}
}

void VfxNodeTimeline::getDescription(VfxNodeDescription & d)
{
	d.add("time: %.2fs, isPlaying: %d", time, isPlaying);
}

int VfxNodeTimeline::calculateMarkerIndex(const double time, const double _bpm)
{
	const double bpm = _bpm > 0.0 ? _bpm : 60.0;
	const double beat = time * bpm / 60.0;
	
	int index = -1;
	
	while (index + 1 < timeline.numKeys && beat > timeline.keys[index + 1].beat)
		index++;
	
	return index;
}

void VfxNodeTimeline::handleTimeSegment(const double oldTime, const double newTime, const double bpm)
{
	const int oldMarkerIndex = calculateMarkerIndex(oldTime, bpm);
	const int newMarkerIndex = calculateMarkerIndex(newTime, bpm);
	
	Assert(newMarkerIndex < timeline.numKeys);
	
	if (oldMarkerIndex <= newMarkerIndex)
	{
		for (int i = oldMarkerIndex; i < newMarkerIndex; ++i)
		{
			const VfxTimeline::Key & keyToTrigger = timeline.keys[i + 1];
			
			eventTriggerData.setInt(keyToTrigger.id);
			
			trigger(kOutput_EventTrigger);
		}
	}
	else
	{
		for (int i = oldMarkerIndex; i > newMarkerIndex; --i)
		{
			const VfxTimeline::Key & keyToTrigger = timeline.keys[i];
			
			eventTriggerData.setInt(keyToTrigger.id);
			
			trigger(kOutput_EventTrigger);
		}
	}
}

void VfxNodeTimeline::play()
{
	isPlaying = true;
}

void VfxNodeTimeline::pause()
{
	isPlaying = false;
}

void VfxNodeTimeline::resume()
{
	isPlaying = true;
}
