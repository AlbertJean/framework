#pragma once

#include <map>
#include <vector>
#include "ChannelHandler.h"
#include "libnet_forward.h"
#include "libreplication_forward.h"
#include "Options.h"
#include "ReplicationHandler.h"

OPTION_EXTERN(bool, g_devMode);
OPTION_EXTERN(bool, g_clientSim);

class Client;
class Host;
class OptionMenu;
struct ParticleSpawnInfo;
struct Player;
struct PlayerInput;
class PlayerNetObject;
class StatTimerMenu;

namespace NetSessionDiscovery
{
	class Service;
}

class App : public ChannelHandler, public ReplicationHandler
{
	struct ClientInfo
	{
		uint32_t replicationId;
		PlayerNetObject * player;
	};

	bool m_isHost;

	PacketDispatcher * m_packetDispatcher;
	ChannelManager * m_channelMgr;
	RpcManager * m_rpcMgr;
	ReplicationManager * m_replicationMgr;

	NetSessionDiscovery::Service * m_discoveryService;
	Ui * m_discoveryUi;

	Host * m_host;
	std::map<Channel*, ClientInfo> m_hostClients;

	std::vector<Client*> m_clients;
	int m_selectedClient;

	std::vector<int> m_freeControllerList;

	OptionMenu * m_optionMenu;
	bool m_optionMenuIsOpen;

	StatTimerMenu * m_statTimerMenu;
	bool m_statTimerMenuIsOpen;

	//

	Client * findClientByChannel(Channel * channel);

	// ChannelHandler
	virtual void SV_OnChannelConnect(Channel * channel);
	virtual void SV_OnChannelDisconnect(Channel * channel);
	virtual void CL_OnChannelConnect(Channel * channel) { }
	virtual void CL_OnChannelDisconnect(Channel * channel) { }

	// ReplicationHandler
	virtual bool OnReplicationObjectSerializeType(ReplicationClient * client, ReplicationObject * object, BitStream & bitStream);
	virtual bool OnReplicationObjectCreateType(ReplicationClient * client, BitStream & bitStream, ReplicationObject ** out_object);
	virtual void OnReplicationObjectCreated(ReplicationClient * client, ReplicationObject * object);
	virtual void OnReplicationObjectDestroyed(ReplicationClient * client, ReplicationObject * object);

	// RpcHandler
	static void handleRpc(Channel * channel, uint32_t method, BitStream & bitStream);

public:
	App();
	~App();

	bool init(bool isHost);
	void shutdown();

	void connect(const char * address);
	void disconnectClient(int index);
	void selectClient(int index);
	Client * getSelectedClient();
	bool isSelectedClient(Client * client);
	bool isSelectedClient(Channel * channel);
	bool isSelectedClient(uint16_t channelId);

	bool tick();
	void draw();

	void netSyncGameSim(Channel * channel);
	void netPlaySound(const char * filename, uint8_t volume = 100);
	void netScreenShake(float dx, float dy, float stiffness, float life);
	void netSetPlayerInputs(uint16_t channelId, uint32_t netId, const PlayerInput & input);
	void netBroadcastPlayerInputs();
	void netSetPlayerCharacterIndex(uint16_t channelId, uint32_t netId, uint8_t characterIndex);
	uint16_t netSpawnBullet(int16_t x, int16_t y, uint8_t angle, uint8_t type, uint32_t ownerNetId);
	void netKillBullet(uint16_t id);
	void netUpdateBullet(uint16_t id);
	uint16_t netAddSprite(const char * filename, int16_t x, int16_t y);
	void netSyncSprite(uint16_t id, Channel * channel);
	void netRemoveSprite(uint16_t id);
	void netSpawnParticles(const ParticleSpawnInfo & spawnInfo);
	void netUpdateBlock(uint8_t x, uint8_t y, const Block & block);

	int allocControllerIndex();
	void freeControllerIndex(int index);
	int getControllerAllocationCount() const;

	ReplicationManager * getReplicationMgr() { return m_replicationMgr; }
};

extern App * g_app;
extern int g_updateTicks;
