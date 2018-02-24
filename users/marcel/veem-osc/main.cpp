#include "framework.h"
#include "Log.h"
#include "StringEx.h"
#include <atomic>

// for inbound OSC messages
#include "osc/OscPacketListener.h"

// for outbound OSC messages
#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"

#define OSC_BUFFER_SIZE 2048

const int GFX_SX = 1000;
const int GFX_SY = 800;

std::atomic<bool> s_quitRequested(false);

static SDL_mutex * s_mutex = nullptr;

// OSC message history

struct OscMessageHistory
{
	struct Elem
	{
		std::string address;
		std::vector<float> values;
		std::vector<float> values_slow;
		std::vector<float> values_gradient;
		uint64_t lastReceiveTime = 0;
		bool isAutoGenerated = false;
		
		void init(const char * _address)
		{
			address = _address;
			
			if (String::EndsWith(address, "_slow"))
				isAutoGenerated = true;
			if (String::EndsWith(address, "_gradient"))
				isAutoGenerated = true;
		}
		
		void record(const std::vector<float> & _values)
		{
			values = _values;
		}
		
		void updateSlow(const float dt)
		{
			// make sure the arrays have the correct size
			
			while (values_slow.size() < values.size())
				values_slow.push_back(0.f);
			while (values_slow.size() > values.size())
				values_slow.pop_back();
			
			// update
			
			const float retain = std::pow(.8f, dt);
			
			for (size_t i = 0; i < values.size(); ++i)
			{
				const float oldValue = values_slow[i];
				const float newValue = values[i];
				
				values_slow[i] = oldValue * retain + newValue * (1.f - retain);
			}
		}
		
		void calculateGradient(const float dt)
		{
			// make sure the arrays have the correct size
			
			while (values_gradient.size() < values.size())
				values_gradient.push_back(0.f);
			while (values_gradient.size() > values.size())
				values_gradient.pop_back();
		}
	};
	
	std::map<std::string, Elem> elems;
	
	Elem & getElem(const char * address)
	{
		auto elemItr = elems.find(address);
		
		if (elemItr == elems.end())
		{
			auto & elem = elems[address];
			
			elem.init(address);
			
			return elem;
		}
		else
		{
			return elemItr->second;
		}
	}
};

static OscMessageHistory s_oscMessageHistory;

// OSC receiver

struct OscPacketListener : osc::OscPacketListener
{
	virtual void ProcessMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint) override
	{
		SDL_LockMutex(s_mutex);
		{
			// todo : check address
			
			const char * address = m.AddressPattern();
			
			if (address != nullptr)
			{
				// todo : record value
				
				OscMessageHistory::Elem & elem = s_oscMessageHistory.getElem(address);
				
				std::vector<float> values;
				
				for (auto aItr = m.ArgumentsBegin(); aItr != m.ArgumentsEnd(); ++aItr)
				{
					auto & a = *aItr;
					
					const float value = a.IsFloat() ? a.AsFloat() : 0.f;
					
					values.push_back(value);
				}
				
				elem.record(values);
			}
		}
		SDL_UnlockMutex(s_mutex);
	}
};

// IpEndpointName::ANY_ADDRESS

struct OscReceiver
{
	OscPacketListener * packetListener = nullptr;
	UdpListeningReceiveSocket * receiveSocket = nullptr;
	
	SDL_Thread * receiveThread = nullptr;
	
	~OscReceiver()
	{
		shut();
	}
	
	void init(const char * ipAddress, const int udpPort)
	{
		packetListener = new OscPacketListener();
	
		receiveSocket = new UdpListeningReceiveSocket(IpEndpointName(ipAddress, udpPort), packetListener);
		
		receiveThread = SDL_CreateThread(receiveThreadProc, "OSC Receive", this);
	}
	
	void shut()
	{
		LOG_DBG("terminating OSC receive thread", 0);
		
		if (receiveSocket != nullptr)
		{
			receiveSocket->AsynchronousBreak();
		}
		
		if (receiveThread != nullptr)
		{
			SDL_WaitThread(receiveThread, nullptr);
			receiveThread = nullptr;
		}
		
		LOG_DBG("terminating OSC receive thread [done]", 0);
		
		LOG_DBG("terminating OSC UDP receive socket", 0);
		
		delete receiveSocket;
		receiveSocket = nullptr;
		
		LOG_DBG("terminating OSC UDP receive socket [done]", 0);
		
		delete packetListener;
		packetListener = nullptr;
	}
	
	static int receiveThreadProc(void * obj)
	{
		OscReceiver * self = (OscReceiver*)obj;
		
		self->receiveSocket->Run();
		
		return 0;
	}
};

//

struct OscSender
{
	UdpTransmitSocket * transmitSocket;
	
	void init(const char * ipAddress, const int udpPort)
	{
		transmitSocket = new UdpTransmitSocket(IpEndpointName(ipAddress, udpPort));
	}
	
	void shut()
	{
		if (transmitSocket != nullptr)
		{
			delete transmitSocket;
			transmitSocket = nullptr;
		}
	}
	
	void send(const void * data, const int dataSize)
	{
		if (transmitSocket != nullptr)
		{
			transmitSocket->Send((char*)data, dataSize);
		}
	}
};

// timer event

static int s_timerEvent = -1;

static SDL_Thread * s_timerThread = nullptr;

static int timerThreadProc(void * obj)
{
	while (!s_quitRequested)
	{
		SDL_Event e;
		e.type = s_timerEvent;
		SDL_PushEvent(&e);
		
		SDL_Delay(1000);
	}
	
	return 0;
}

//

int main(int argc, char * argv[])
{
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	s_mutex = SDL_CreateMutex();
	
	OscReceiver * receiver = new OscReceiver();
	receiver->init("127.0.0.1", 8000);
	
	OscSender * sender = new OscSender();
	sender->init("255.255.255.255", 8000);
	
	s_timerEvent = SDL_RegisterEvents(1);
	
	s_timerThread = SDL_CreateThread(timerThreadProc, "Timer", nullptr);
	
	framework.waitForEvents = true;
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;
		
		bool repaint = false;
		bool updateOsc = false;
		
		repaint = true; // fixme
		updateOsc = true; // fixme
		
		for (auto & event : framework.events)
		{
			if (event.type == s_timerEvent)
			{
				repaint = true;
				
				updateOsc = true;
			}
		}
		
		if (true)
		{
		#if 1
			// todo : remove. fake some OSC sends
			
			char buffer[OSC_BUFFER_SIZE];
			osc::OutboundPacketStream p(buffer, OSC_BUFFER_SIZE);
			p << osc::BeginMessage("/humidity");
				p << (std::sin(framework.time) + 1.f) / 2.f;
				p << (std::cos(framework.time) + 1.f) / 2.f;
				p << osc::EndMessage;
			
			sender->send(p.Data(), p.Size());
		#endif
		}
		
		if (updateOsc)
		{
			SDL_LockMutex(s_mutex);
			{
				for (auto & elemItr : s_oscMessageHistory.elems)
				{
					auto & elem = elemItr.second;
					
					if (!elem.isAutoGenerated)
					{
						// todo : update slow-changing version
						
						const float dt = framework.timeStep;
						
						elem.updateSlow(dt);
						
						// todo : send slow-changing version
						
						{
							char address[256];
							sprintf_s(address, sizeof(address), "%s_slow", elem.address.c_str());
							
							char buffer[OSC_BUFFER_SIZE];
							osc::OutboundPacketStream p(buffer, OSC_BUFFER_SIZE);
							
							p << osc::BeginMessage(address);
							{
								for (auto & value : elem.values_slow)
									p << value;
							}
							p << osc::EndMessage;
							
							sender->send(p.Data(), p.Size());
						}
						
						// todo : update gradient version
						
						elem.calculateGradient(dt);
					
						// todo : send gradient version
						
						{
							char address[256];
							sprintf_s(address, sizeof(address), "%s_gradient", elem.address.c_str());
							
							char buffer[OSC_BUFFER_SIZE];
							osc::OutboundPacketStream p(buffer, OSC_BUFFER_SIZE);
							
							p << osc::BeginMessage(address);
							{
								for (auto & value : elem.values_gradient)
									p << value;
							}
							p << osc::EndMessage;
							
							sender->send(p.Data(), p.Size());
						}
					}
				}
			}
			SDL_UnlockMutex(s_mutex);
		}
		
		if (repaint)
		{
			OscMessageHistory history;
			
			SDL_LockMutex(s_mutex);
			{
				history = s_oscMessageHistory;
			}
			SDL_UnlockMutex(s_mutex);
			
			framework.beginDraw(rand() % 255, 0, 0, 0);
			{
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				
				gxPushMatrix();
				{
					gxTranslatef(5, 5, 0);
					setColor(colorWhite);
					
					drawText(0, 0, 12, +1, +1, "OSC history:");
					gxTranslatef(0, 16, 0);
					
					for (auto & elemItr : history.elems)
					{
						auto & elem = elemItr.second;
						
						gxPushMatrix();
						{
							drawText(0, 0, 12, +1, +1, "%s", elem.address.c_str());
							gxTranslatef(200, 0, 0);
							
							drawText(0, 0, 12, +1, +1, "Received");
							gxTranslatef(60, 0, 0);
							for (auto & value : elem.values)
							{
								drawText(0, 0, 12, +1, +1, "%.2f", value);
								gxTranslatef(32, 0, 0);
							}
						}
						gxPopMatrix();
						
						gxTranslatef(0, 16, 0);
					}
				}
				gxPopMatrix();
				
				popFontMode();
			}
			framework.endDraw();
		}
	}
	
	s_quitRequested = true;
	
	SDL_WaitThread(s_timerThread, nullptr);
	
	delete receiver;
	receiver = nullptr;
	
	SDL_DestroyMutex(s_mutex);
	s_mutex = nullptr;
	
	framework.shutdown();
	
	return 0;
}
