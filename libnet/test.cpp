#include <exception>
#include <map>
#include <SDL/SDL.h>
#include "BinaryDiff.h"
#include "ChannelHandler.h"
#include "ChannelManager.h"
#include "Log.h"
#include "NetDiag.h"
#include "NetProtocols.h"
#include "NetStats.h"
#include "Packet.h"
#include "PacketDispatcher.h"
#include "PolledTimer.h"
#include "SDL_Bitmap.h"
#include "Timer.h"

enum TestProtocol
{
	TestProtocol_RT = PROTOCOL_CUSTOM,
	TestProtocol_ClientState
};

class StateVector
{
public:
	StateVector()
		: m_allocIdx(0)
	{
		memset(m_bytes, 0, sizeof(m_bytes));
	}

	uint16_t & AllocUint16()
	{
		NetAssert(m_allocIdx + 2 <= kSize);
		uint16_t & v = reinterpret_cast<uint16_t &>(m_bytes[m_allocIdx]);
		m_allocIdx += 2;
		return v;
	}

	int16_t & AllocSint16()
	{
		NetAssert(m_allocIdx + 2 <= kSize);
		int16_t & v = reinterpret_cast<int16_t &>(m_bytes[m_allocIdx]);
		m_allocIdx += 2;
		return v;
	}

	const static uint32_t kSize = 1024;

	uint32_t m_allocIdx;
	uint8_t m_bytes[kSize];
};

class Player
{
public:
	Player(StateVector & stv)
		: m_posX(stv.AllocSint16())
		, m_posY(stv.AllocSint16())
		, m_velX(stv.AllocSint16())
		, m_velY(stv.AllocSint16())
	{
		m_posX = 0;
		m_posY = 0;
		m_velX = 0;
		m_velY = 0;
	}

	void Update()
	{
		m_posX += m_velX;
		m_posY += m_velY;

		if (m_posX < 0)
			m_posX = 0;
		if (m_posY < 0)
			m_posY = 0;
		if (m_posX > 256 - 1)
			m_posX = 256 - 1;
		if (m_posY > 256 - 1)
			m_posY = 256 - 1;
	}

	int16_t & m_posX;
	int16_t & m_posY;
	int16_t & m_velX;
	int16_t & m_velY;
};

class Block
{
public:
	inline Block(StateVector & stv)
		: m_posX(stv.AllocUint16())
		, m_posY(stv.AllocUint16())
		, m_extX(stv.AllocUint16())
		, m_extY(stv.AllocUint16())
	{
	}

	uint16_t & m_posX;
	uint16_t & m_posY;
	uint16_t & m_extX;
	uint16_t & m_extY;
};

class Map
{
public:
	Map(StateVector & stv)
	{
		m_blocks = new Block*[kMaxBlocks];

		for (uint32_t i = 0; i < kMaxBlocks; ++i)
			m_blocks[i] = new Block(stv);
	}

	~Map()
	{
		for (uint32_t i = 0; i < kMaxBlocks; ++i)
		{
			delete m_blocks[i];
			m_blocks[i] = 0;
		}

		delete[] m_blocks;
		m_blocks = 0;
	}

	const static uint32_t kMaxBlocks = 32;

	Block * * m_blocks;
};

class GameState
{
public:
	GameState(StateVector & stv)
	{
		m_map = new Map(stv);
		m_player = new Player(stv);
	}

	~GameState()
	{
		delete m_player;
		m_player = 0;

		delete m_map;
		m_map = 0;
	}

	Map * m_map;
	Player * m_player;
};

class GameStateData
{
public:
	GameStateData()
	{
		m_gameState = new GameState(m_stateVectors[0]);
	}

	~GameStateData()
	{
		delete m_gameState;
		m_gameState = 0;
	}

	void NextFrame()
	{
		memcpy(
			m_stateVectors[1].m_bytes,
			m_stateVectors[0].m_bytes,
			StateVector::kSize);
	}

	BinaryDiffResult GetDiff(uint32_t skipTreshold)
	{
		return BinaryDiff(
			m_stateVectors[0].m_bytes,
			m_stateVectors[1].m_bytes,
			StateVector::kSize,
			skipTreshold);
	}

	GameState * m_gameState;
	StateVector m_stateVectors[2];
};

//

class ClientState
{
public:
	ClientState(Channel * channel, uint32_t clientIdx)
		: m_clientIdx(clientIdx)
		, m_channel(channel)
		, m_value(0)
	{
	}

	void Draw(SDL_Surface * surface)
	{
		uint32_t n = 16;
		uint32_t x1 = m_clientIdx % n;
		uint32_t y1 = m_clientIdx / n;
		uint32_t s = surface->w / n;

		SDL_Bitmap bitmap(
			surface,
			x1 * s,
			y1 * s,
			s,
			s);

		uint32_t backColor = bitmap.Color(0.5f, 0.0f, 0.0f);
		bitmap.Clear(backColor);

		uint32_t foreColor = bitmap.Color(0.8f, 0.8f, 0.8f);
		uint32_t v = m_value;
		uint32_t x = v % s;
		uint32_t y = (v / s) % s;
		bitmap.RectFill(x, y, 2, 2, foreColor);
	}

	uint32_t m_clientIdx;
	Channel * m_channel;
	uint32_t m_value;
};

class MyChannelHandler : public ChannelHandler, public PacketListener
{
public:
	typedef std::map<uint32_t, Channel*> ChannelMap;

	MyChannelHandler()
		: m_clientIdx(0)
	{
		PacketDispatcher::RegisterProtocol(TestProtocol_RT, this);
	}

	virtual ~MyChannelHandler()
	{
	}

	virtual void OnReceive(Packet & packet, Channel * channel)
	{
		LOG_DBG("received packet");

		uint16_t value;

		if (packet.Read16(&value))
		{
			LOG_DBG("value: %05u", value);

			std::map<uint32_t, ClientState *>::iterator i = m_clientStates.find(channel->m_id);

			if (i != m_clientStates.end())
			{
				ClientState * clientState = i->second;

				clientState->m_value = value;
			}
			else
			{
				LOG_ERR("client state not found", 0);
			}
		}
		else
		{
			LOG_ERR("read error", value);
		}
	}

	virtual void SV_OnChannelConnect(Channel * channel)
	{
		LOG_INF("SV channel connect: %u", channel->m_id);

		NetAssert(m_svChannels.find(channel->m_id) == m_svChannels.end());

		m_svChannels[channel->m_id] = channel;

		//

		ClientState * clientState = new ClientState(channel, m_clientIdx);
		m_clientStates[channel->m_id] = clientState;
		m_clientIdx++;
	}
	
	virtual void SV_OnChannelDisconnect(Channel * channel)
	{
		LOG_INF("SV channel disconnect: %u", channel->m_id);

		ChannelMap::iterator i = m_svChannels.find(channel->m_id);

		if (i != m_svChannels.end())
		{
			m_svChannels.erase(i);
		}
		else
		{
			LOG_WRN("channel disconnect: channel does not exist: %u", channel->m_id);
		}

		//

		std::map<uint32_t, ClientState *>::iterator j = m_clientStates.find(channel->m_id);

		if (j != m_clientStates.end())
		{
			ClientState * clientState = j->second;

			delete clientState;
			clientState = 0;

			m_clientStates.erase(j);
		}
	}

	virtual void CL_OnChannelConnect(Channel * channel)
	{
		LOG_INF("CL channel connect: %u", channel->m_id);

		NetAssert(m_clChannels.find(channel->m_id) == m_clChannels.end());

		m_clChannels[channel->m_id] = channel;
	}
	
	virtual void CL_OnChannelDisconnect(Channel * channel)
	{
		LOG_INF("CL channel disconnect: %u", channel->m_id);

		ChannelMap::iterator i = m_clChannels.find(channel->m_id);

		if (i != m_clChannels.end())
		{
			m_clChannels.erase(i);
		}
		else
		{
			LOG_WRN("channel disconnect: channel does not exist: %u", channel->m_id);
		}
	}

	Channel * PickRandomChannel(ChannelMap & channels)
	{
		if (channels.size() == 0)
			return 0;

		uint32_t index = rand() % channels.size();

		for (ChannelMap::iterator i = channels.begin(); /* true */; ++i, --index)
		{
			if (index == 0)
				return i->second;
		}
	}

	void Disconnect(ChannelManager * channelMgr, Channel * channel)
	{
		if (channel == 0)
		{
			LOG_INF("no channels left to remove", 0);
		}
		else
		{
			channel->Disconnect();
			channelMgr->DestroyChannel(channel);
		}
	}

	void DisconnectAll(ChannelManager * channelMgr, ChannelMap & channels)
	{
		if (channels.size() == 0)
		{
			LOG_INF("no channels left to remove", 0);
		}
		else
		{
			while (channels.size() != 0)
			{
				Channel * channel = channels.begin()->second;
				Disconnect(channelMgr, channel);
			}
		}
	}

	void CL_DisconnectRandom(ChannelManager * channelMgr)
	{
		Channel * channel = PickRandomChannel(m_clChannels);
		Disconnect(channelMgr, channel);
	}

	void CL_DisconnectAll(ChannelManager * channelMgr)
	{
		DisconnectAll(channelMgr, m_clChannels);
	}

	void SV_DisconnectRandom(ChannelManager * channelMgr)
	{
		Channel * channel = PickRandomChannel(m_svChannels);
		Disconnect(channelMgr, channel);
	}

	void SV_DisconnectAll(ChannelManager * channelMgr)
	{
		DisconnectAll(channelMgr, m_svChannels);
	}

	void ShowChannelList(ChannelMap & channels, const char * name)
	{
		LOG_INF("%s:", name);
		if (channels.empty())
		{
			LOG_INF("(empty list)");
		}
		else
		{
			for (ChannelMap::iterator i = channels.begin(); i != channels.end(); ++i)
			{
				Channel * channel = i->second;
				LOG_INF("[%09u] => [%09u]",
					channel->m_id,
					channel->m_destinationId);
			}
		}
	}

	void Show()
	{
		ShowChannelList(m_svChannels, "server channels");
		ShowChannelList(m_clChannels, "client channels");
		NetStats::Show();
	}

	void Send(ChannelMap & channels)
	{
		for (ChannelMap::iterator i = channels.begin(); i != channels.end(); ++i)
		{
			Channel * channel = i->second;

			PacketBuilder<3> packetBuilder;

			const uint8_t protocolId = TestProtocol_RT;
			const uint16_t value = rand();

			packetBuilder.Write8(&protocolId);
			packetBuilder.Write16(&value);

			const Packet packet = packetBuilder.ToPacket();

			channel->Send(packet);
		}
	}

	void CL_Send()
	{
		Send(m_clChannels);
	}

	void SV_Send()
	{
		Send(m_svChannels);
	}

	void SendRT(ChannelMap & channels)
	{
		for (ChannelMap::iterator i = channels.begin(); i != channels.end(); ++i)
		{
			Channel * channel = i->second;

			PacketBuilder<3> packetBuilder;

			const uint8_t protocolId = TestProtocol_RT;
			const uint16_t value = rand();

			packetBuilder.Write8(&protocolId);
			packetBuilder.Write16(&value);

			const Packet packet = packetBuilder.ToPacket();

			channel->SendReliable(packet);
		}
	}

	void CL_SendRT()
	{
		SendRT(m_clChannels);
	}

	void SV_SendRT()
	{
		SendRT(m_svChannels);
	}

	ChannelMap m_svChannels;
	ChannelMap m_clChannels;
	uint32_t m_clientIdx;
	std::map<uint32_t, ClientState *> m_clientStates;
};

void TestGameUpdate()
{
#if 0
	for (uint32_t i = 0; i < 8; ++i)
	{
		const BinaryDiffResult result = gameData.GetDiff(i);
		const uint32_t overhead = 4;
		const uint32_t byteCount = result.m_diffCount * overhead + result.m_diffBytes;
		LOG_INF("byteCount @ treshold %u: %u", i, byteCount);
		for (const BinaryDiffEntry * entry = result.m_diffs; entry; entry = entry->m_next)
			LOG_INF("entry: offset: %09u, size: %09u", entry->m_offset, entry->m_size);
		if (!BinaryDiffValidate(
			gameData.m_stateVectors[0].m_bytes,
			gameData.m_stateVectors[1].m_bytes,
			StateVector::kSize,
			result.m_diffs))
		{
			LOG_ERR("binary diff validation failed", 0);
			NetAssert(false);
		}
	}
#endif

	GameStateData gameData;
	int moveX1 = 0;
	int moveY1 = 0;
	int moveX2 = 0;
	int moveY2 = 0;
	bool stop = false;
	while (stop == false)
	{
		SDL_Delay(50);

		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (SDL_EVENTMASK(e.type) & SDL_KEYEVENTMASK)
			{
				if (e.key.keysym.sym == SDLK_LEFT)
					moveX1 = e.key.state ? -1 : 0;
				if (e.key.keysym.sym == SDLK_RIGHT)
					moveX2 = e.key.state ? +1 : 0;
				if (e.key.keysym.sym == SDLK_UP)
					moveY1 = e.key.state ? -1 : 0;
				if (e.key.keysym.sym == SDLK_DOWN)
					moveY2 = e.key.state ? +1 : 0;
				if (e.key.keysym.sym == SDLK_ESCAPE)
					stop = true;
			}
		}

		gameData.m_gameState->m_player->m_velX = moveX1 + moveX2;
		gameData.m_gameState->m_player->m_velY = moveY1 + moveY2;
		gameData.m_gameState->m_player->Update();
		
		const BinaryDiffResult result = gameData.GetDiff(4);
		LOG_INF("diff bytes: %u bytes", result.m_diffBytes);

		gameData.NextFrame();
	}
}

int main(int argc, char * argv[])
{
	try
	{
		const uint32_t displaySx = 256;
		const uint32_t displaySy = 256;

		if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
			throw std::exception();//"failed to initialize SDL");

		SDL_Surface * surface = SDL_SetVideoMode(displaySx, displaySy, 32, 0);

		TestGameUpdate();

		printf("select mode:\n");
		printf("S = server\n");
		printf("C = client\n");
		printf("B = both server & client\n");

		char mode = 'U';

		while (mode == 'U')
		{
			SDL_Event e;
			if (SDL_PollEvent(&e))
			{
				if (e.type == SDL_KEYDOWN)
				{
					if (e.key.keysym.sym == SDLK_s)
						mode = 'S';
					if (e.key.keysym.sym == SDLK_c)
						mode = 'C';
					if (e.key.keysym.sym == SDLK_b)
						mode = 'B';
				}
			}
		}

		printf("mode = %c\n", mode);

		printf("         A: add client channel\n");
		printf("LSHIFT + A: add 100 client channels\n");
		printf("         D: remove client channel\n");
		printf("LSHIFT + D: remove all client channels\n");

		bool isServer = mode == 'S' || mode == 'B';
		bool isClient = mode == 'C' || mode == 'B';

		uint16_t serverPort = 3300;
		uint16_t listenPort = isServer ? serverPort : serverPort + 1;

		//

		Timer timer;
		PolledTimer polledTimer;
		polledTimer.Initialize(&timer);
		polledTimer.Start();
		
		MyChannelHandler channelHandler;
		
		ChannelManager channelMgr;
		channelMgr.Initialize(&channelHandler, listenPort, isServer);

		PacketDispatcher::RegisterProtocol(PROTOCOL_CHANNEL, &channelMgr);

		bool stop = false;
		bool randomize1 = false;
		bool randomize2 = false;

		while (stop == false)
		{
			SDL_Delay(10);
			//SDL_Delay(1);
			//SDL_Delay(0);

			SDL_Event e;

			while (SDL_PollEvent(&e))
			{
				if (e.type == SDL_KEYDOWN)
				{
					if (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_q)
						stop = true;
					if (isClient && e.key.keysym.sym == SDLK_a)
					{
						uint32_t count = (e.key.keysym.mod & KMOD_SHIFT) ? 100 : 1;

						for (uint32_t i = 0; i < count; ++i)
						{
							Channel * clientChannel = channelMgr.CL_CreateChannel();
							NetAssert(clientChannel != 0);
							LOG_INF("created client channel %09u", clientChannel->m_id);
							
							//NetAddress address(74, 125, 79, 104, 4850);
							NetAddress address(127, 0, 0, 1, serverPort);
							
							clientChannel->Connect(address);

							//channelHandler.CL_AddChannel(clientChannel);
						}
					}
					if (e.key.keysym.sym == SDLK_d)
					{
						bool all = (e.key.keysym.mod & KMOD_SHIFT) != 0;

						if (isClient)
						{
							if (all)
								channelHandler.CL_DisconnectAll(&channelMgr);
							else
								channelHandler.CL_DisconnectRandom(&channelMgr);
						}
						else if (isServer)
						{
							if (all)
								channelHandler.SV_DisconnectAll(&channelMgr);
							else
								channelHandler.SV_DisconnectRandom(&channelMgr);
						}
					}
					if (e.key.keysym.sym == SDLK_l)
					{
						channelHandler.Show();
					}
					if (e.key.keysym.sym == SDLK_t)
					{
						randomize1 = true;
					}
					if (e.key.keysym.sym == SDLK_r)
					{
						randomize2 = true;
					}
				}
				if (e.type == SDL_KEYUP)
				{
					if (e.key.keysym.sym == SDLK_t)
					{
						randomize1 = false;
					}
					if (e.key.keysym.sym == SDLK_r)
					{
						randomize2 = false;
					}
				}
			}

			if (randomize1)
			{
				if (isClient)
					channelHandler.CL_Send();
				else if (isServer)
					channelHandler.SV_Send();
			}

			if (randomize2)
			{
				if (isClient)
					channelHandler.CL_SendRT();
				else if (isServer)
					channelHandler.SV_SendRT();
			}

			uint32_t time = polledTimer.TimeMS_get();
			
			channelMgr.Update(time);

			if (SDL_LockSurface(surface) < 0)
				throw std::exception();//"failed to lock surface");

			SDL_Bitmap bitmap(surface, 0, 0, displaySx, displaySy);

			uint32_t c = bitmap.Color(isClient ? 1.0f : 0.0f, isServer ? 1.0f : 0.0f, 0.0f);

			bitmap.Clear(c);

#if 1
			for (std::map<uint32_t, ClientState *>::iterator i = channelHandler.m_clientStates.begin(); i != channelHandler.m_clientStates.end(); ++i)
			{
				ClientState * clientState = i->second;

				clientState->Draw(surface);
			}
#endif

			SDL_UnlockSurface(surface);

			SDL_Flip(surface);
		}
		
		channelMgr.Shutdown();

		SDL_FreeSurface(surface);
		
		return 0;
	}
	catch (std::exception & e)
	{
		printf("error: %s\n", e.what());
		return -1;
	}
}
