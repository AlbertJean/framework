#include <algorithm>
#include <vector>
#include "ChannelManager.h"
#include "ChannelTypes.h"
#include "NetDiag.h"
#include "NetProtocols.h"
#include "PacketDispatcher.h"

ChannelManager::ChannelManager()
	: m_socket(0)
	, m_handler(0)
	, m_listenChannel(0)
{
}

ChannelManager::~ChannelManager()
{
	NetAssert(m_socket.get() == 0);
	NetAssert(m_channels.empty());
	NetAssert(m_handler == 0);
	NetAssert(m_listenChannel == 0);
}

bool ChannelManager::Initialize(ChannelHandler * handler, uint16_t serverPort, bool enableServer)
{
	m_handler = handler;

	m_socket = new NetSocket();

	if (serverPort != 0)
	{
		if (m_socket->Bind(serverPort) == false)
			return false;
	}

	if (enableServer)
	{
		m_listenChannel = SV_CreateChannel();
	}

	return true;
}

void ChannelManager::Shutdown()
{
	while (m_channels.size())
	{
		Channel * channel = m_channels.begin()->second;

		DestroyChannel(channel);
	}

	m_listenChannel = 0;

	m_handler = 0;

	m_socket = SharedNetSocket();
}

Channel * ChannelManager::SV_CreateChannel()
{
	return CreateChannelEx(ChannelType_Server, ChannelSide_Server);
}

Channel * ChannelManager::CL_CreateChannel()
{
	return CreateChannelEx(ChannelType_Client, ChannelSide_Client);
}

Channel * ChannelManager::CreateChannelEx(ChannelType channelType, ChannelSide channelSide)
{
	Channel * channel = new Channel(channelType, channelSide, (1 << PROTOCOL_CHANNEL));

	channel->Initialize(this, m_socket);
	channel->m_id = m_channelIds.Allocate();

	m_channels[channel->m_id] = channel;

	return channel;
}

void ChannelManager::DestroyChannel(Channel * channel)
{
	if (m_handler && channel->m_channelType == ChannelType_Client)
	{
		/**/ if (channel->m_channelSide == ChannelSide_Client)
			m_handler->CL_OnChannelDisconnect(channel);
		else if (channel->m_channelSide == ChannelSide_Server)
			m_handler->SV_OnChannelDisconnect(channel);
		else
			NetAssert(false);
	}

	m_channels.erase(channel->m_id);

	m_channelIds.Free(channel->m_id);

	delete channel;
}

void ChannelManager::DestroyChannelQueued(Channel * channel)
{
	NetAssert(std::find(m_destroyedChannels.begin(), m_destroyedChannels.end(), channel) == m_destroyedChannels.end());

	m_destroyedChannels.push_back(channel);
}

void ChannelManager::Update(uint32_t time)
{
	for (ChannelMapItr i = m_channels.begin(); i != m_channels.end(); ++i)
		i->second->Update(time);

	// Destroy disconnected client channels.
	for (size_t i = 0; i < m_destroyedChannels.size(); ++i)
		DestroyChannel(m_destroyedChannels[i]);

	m_destroyedChannels.clear();
}

void ChannelManager::OnReceive(Packet & packet, Channel * channel)
{
	uint8_t messageId;

	if (packet.Read8(&messageId))
	{
		switch (messageId)
		{
		case CHANNELMSG_TRUNK:
			HandleTrunk(packet, channel);
			break;
		case CHANNELMSG_CONNECT:
			HandleConnect(packet, channel);
			break;
		case CHANNELMSG_CONNECT_OK:
			HandleConnectOK(packet, channel);
			break;
		case CHANNELMSG_CONNECT_ERROR:
			HandleConnectError(packet, channel);
			break;
		case CHANNELMSG_CONNECT_ACK:
			HandleConnectAck(packet, channel);
			break;
		case CHANNELMSG_DISCONNECT:
			HandleDisconnect(packet, channel);
			break;
		case CHANNELMSG_PING:
			channel->HandlePing(packet);
			break;
		case CHANNELMSG_PONG:
			channel->HandlePong(packet);
			break;
		case CHANNELMSG_RT_UPDATE:
			channel->HandleRTUpdate(packet);
			break;
		case CHANNELMSG_RT_ACK:
			channel->HandleRTAck(packet);
			break;
		case CHANNELMSG_UNPACK:
			HandleUnpack(packet, channel);
			break;
		default:
			LOG_ERR("chanmgr: message: unknown message type: %u", static_cast<uint32_t>(messageId));
			NetAssert(false);
			break;
		}
	}
	else
	{
		LOG_ERR("chanmgr: message: failed to read from packet", 0);
		NetAssert(false);
	}
}

void ChannelManager::HandleTrunk(Packet & packet, Channel * channel)
{
	uint16_t channelId;

	if (packet.Read16(&channelId))
	{
#if LIBNET_CHANNELMGR_LOG_TRUNK == 1
		LOG_DBG("chanmgr: trunk: redirecting to channel %u", static_cast<uint32_t>(channelId));
#endif

		Channel * channel2 = FindChannel(channelId);

		if (channel2 != 0)
		{
			Packet packet2;

			if (packet.ExtractTillEnd(packet2))
			{
				PacketDispatcher::Dispatch(packet2, channel2);
			}
			else
			{
				LOG_ERR("failed to extract packet", 0);
				NetAssert(false);
			}
		}
		else
		{
			LOG_WRN("chanmgr: trunk: trunk to unknown channel [trunk %09u => %09u]",
				static_cast<uint32_t>(channel->m_id),
				static_cast<uint32_t>(channelId));
			NetAssert(false);
		}
	}
	else
	{
		LOG_ERR("chanmgr: trunk: failed to read from packet", 0);
		NetAssert(false);
	}
}

void ChannelManager::HandleConnect(Packet & packet, Channel * channel)
{
	uint16_t channelId;

	if (packet.Read16(&channelId))
	{
		LOG_INF("chanmgr: connect: received connection attempt from client channel [%09u]",
			static_cast<uint32_t>(channelId));
#if defined(WINDOWS)
		LOG_INF("chanmgr: connect: peer address: %u.%u.%u.%u:%u",
			packet.m_rcvAddress.GetSockAddr()->sin_addr.S_un.S_un_b.s_b1,
			packet.m_rcvAddress.GetSockAddr()->sin_addr.S_un.S_un_b.s_b2,
			packet.m_rcvAddress.GetSockAddr()->sin_addr.S_un.S_un_b.s_b3,
			packet.m_rcvAddress.GetSockAddr()->sin_addr.S_un.S_un_b.s_b4,
			packet.m_rcvAddress.GetSockAddr()->sin_port);
#endif

		Channel * newChannel = CreateChannelEx(ChannelType_Client, ChannelSide_Server);
		newChannel->m_destinationId = channelId;
		newChannel->m_address = packet.m_rcvAddress;

		LOG_INF("chanmgr: connect: created new server channel [%09u]",
			static_cast<uint32_t>(newChannel->m_id));

		PacketBuilder<6> replyBuilder;

		const uint8_t protocolId = PROTOCOL_CHANNEL;
		const uint8_t messageId = CHANNELMSG_CONNECT_OK;
		const uint16_t destinationId = channelId;
		const uint16_t newDestinationId = newChannel->m_id;

		replyBuilder.Write8(&protocolId);
		replyBuilder.Write8(&messageId);
		replyBuilder.Write16(&destinationId);
		replyBuilder.Write16(&newDestinationId);

		Packet reply = replyBuilder.ToPacket();

		newChannel->Send(reply);

		LOG_INF("chanmgr: connect: sent ConnectOK to client channel %09u with request to change destination ID to %09u",
			static_cast<uint32_t>(newChannel->m_destinationId),
			static_cast<uint32_t>(newDestinationId));
	}
	else
	{
		LOG_ERR("chanmgr: connect: failed to read from packet", 0);
		NetAssert(false);
	}
}

void ChannelManager::HandleConnectOK(Packet & packet, Channel * channel)
{
	uint16_t channelId;
	uint16_t newDestinationId;

	if (packet.Read16(&channelId) &&
		packet.Read16(&newDestinationId))
	{
		LOG_INF("chanmgr: connect-ok: received OK from server channel %09u for client channel %09u",
			static_cast<uint32_t>(newDestinationId),
			static_cast<uint32_t>(channelId));

		Channel * channel2 = FindChannel(channelId);

		if (channel2 && channel2 == channel)
		{
			channel2->m_destinationId = newDestinationId;

			LOG_INF("chanmgr: connect-ok: updated destination server channel ID to %09u",
				static_cast<uint32_t>(channel2->m_destinationId));

			PacketBuilder<4> replyBuilder;

			const uint8_t protocolId = PROTOCOL_CHANNEL;
			const uint8_t messageId = CHANNELMSG_CONNECT_ACK;
			const uint16_t destinationId = channel2->m_destinationId;

			replyBuilder.Write8(&protocolId);
			replyBuilder.Write8(&messageId);
			replyBuilder.Write16(&destinationId);

			Packet reply = replyBuilder.ToPacket();

			channel2->Send(reply);

			LOG_INF("chanmgr: connect-ok: sent ACK to server channel %u", channel2->m_destinationId);

			if (m_handler)
				m_handler->CL_OnChannelConnect(channel);

			channel2->m_protocolMask = 0xffffffff;
		}
		else
		{
			if (channel2 == 0)
			{
				LOG_ERR("chanmgr: connect-ok: unknown channel %u", channelId);
				NetAssert(false);
			}
			else
			{
				LOG_ERR("chanmgr: connect-ok: channel mismatch %u", channelId);
				NetAssert(false);
			}
		}
	}
	else
	{
		LOG_ERR("chanmgr: connect-ok: failed to read from packet", 0);
		NetAssert(false);
	}
}

void ChannelManager::HandleConnectError(Packet & packet, Channel * channel)
{
	LOG_ERR("chanmgr: connect-error: received error from channel %09u",
		static_cast<uint32_t>(channel->m_id));
	NetAssert(false);
}

void ChannelManager::HandleConnectAck(Packet & packet, Channel * channel)
{
	// TODO: When ACK, send client channelID for validation purposes.
	uint16_t channelId;

	if (packet.Read16(&channelId))
	{
		if (channel->m_id == channelId)
		{
			LOG_INF("chanmgr: connect-ack: received ACK for server channel %09u from client channel %09u",
				static_cast<uint32_t>(channelId),
				static_cast<uint32_t>(channel->m_destinationId));

			if (m_handler)
				m_handler->SV_OnChannelConnect(channel);

			channel->m_protocolMask = 0xffffffff;
		}
		else
		{
			LOG_ERR("chanmgr: connect-ack: channel mismatch %09u, expected %09u",
				static_cast<uint32_t>(channelId),
				static_cast<uint32_t>(channel->m_id));
			NetAssert(false);
		}
	}
	else
	{
		LOG_ERR("chanmgr: connect-ack: failed to read from packet", 0);
		NetAssert(false);
	}
}

void ChannelManager::HandleDisconnect(Packet & packet, Channel * channel)
{
	uint16_t channelId;

	if (packet.Read16(&channelId))
	{
		if (channel->m_destinationId == channelId)
		{
			LOG_INF("chanmgr: disconnect: received disconnect for server channel %u from client channel %u",
				channel->m_id,
				channel->m_destinationId);

			DestroyChannelQueued(channel);
		}
		else
		{
			LOG_ERR("chanmgr: disconnect: channel mismatch %09u, expected %09u",
				static_cast<uint32_t>(channelId),
				static_cast<uint32_t>(channel->m_destinationId));
			NetAssert(false);
		}
	}
	else
	{
		LOG_ERR("chanmgr: disconnect: failed to read from packet", 0);
		NetAssert(false);
	}
}

void ChannelManager::HandleUnpack(Packet & packet, Channel * channel)
{
	uint16_t size;

	if (packet.Read16(&size))
	{
		while (size > 0)
		{
			uint16_t size2;

			if (packet.Read16(&size2))
			{
				size -= 2;

				Packet extracted;

				if (packet.Extract(extracted, size2))
				{
					packet.Skip(size2);

					PacketDispatcher::Dispatch(extracted, channel);

					size -= size2;
				}
				else
				{
					LOG_ERR("chanmgr: unpack: failed to read from packet", 0);
					NetAssert(false);
					break;
				}
			}
			else
			{
				LOG_ERR("chanmgr: unpack: failed to read from packet", 0);
				NetAssert(false);
				break;
			}
		}
	}
	else
	{
		LOG_ERR("chanmgr: unpack: failed to read from packet", 0);
		NetAssert(false);
	}
}

Channel * ChannelManager::FindChannel(uint32_t id)
{
	ChannelMapItr i = m_channels.find(id);

	if (i == m_channels.end())
	{
		return 0;
	}
	else
	{
		return i->second;
	}
}
