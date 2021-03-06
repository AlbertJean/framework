#include "PolledTimer.h"
#include "Timer.h"

PolledTimer::PolledTimer()
{
	Initialize(0);
}

void PolledTimer::Initialize(ITimer* timer)
{
	m_intervalMS = 1000;
	m_lastReadMS = 0;
	m_IsActive = false;
	m_FireImmediately = false;
	m_timer = timer;
}

void PolledTimer::Start()
{
	ClearTick();
	
	m_IsActive = true;
}

void PolledTimer::Stop()
{
	m_IsActive = false;
}

void PolledTimer::Restart()
{
	Stop();

	Start();
}

void PolledTimer::SetInterval(float seconds)
{
	SetIntervalMS((int)(seconds * 1000.0f));
}

void PolledTimer::SetIntervalMS(int miliseconds)
{
	m_intervalMS = miliseconds;

	ClearTick();
}

void PolledTimer::SetFrequency(float frequency)
{
	SetInterval(1.0f / frequency);
}

bool PolledTimer::PeekTick() const
{
	if (!m_IsActive)
		return false;
	
	int timeMS = TimeMS_get();
	
	if (timeMS >= m_lastReadMS + m_intervalMS)
		return true;
	else
		return false;
}

bool PolledTimer::ReadTick()
{
	if (!PeekTick())
		return false;

	m_lastReadMS += m_intervalMS;

	return true;
}

void PolledTimer::ClearTick()
{
	m_lastReadMS = TimeMS_get();
	
	if (m_FireImmediately)
		m_lastReadMS -= m_intervalMS;
}

int PolledTimer::TickCount_get() const
{
	return (TimeMS_get() - m_lastReadMS) / m_intervalMS;
}

float PolledTimer::TimeS_get() const
{
	return m_timer->Time_get();
}

uint32_t PolledTimer::TimeMS_get() const
{
	return static_cast<uint32_t>(m_timer->TimeMS_get());
}

uint64_t PolledTimer::TimeUS_get() const
{
	return m_timer->TimeUS_get();
}
