#include <algorithm>
#include "BitStream.h"
#include "Debug.h"
#include "MyProtocols.h"
#include "PacketDispatcher.h"
#include "ReplicationManager.h"
#include "Stats.h"
#include "Types.h"

ReplicationManager::ReplicationManager()
{
	m_handler = 0;
	m_tick = 0;
	m_serverObjectCreationId = 0;
}

ReplicationManager::~ReplicationManager()
{
	while (m_serverObjects.size() > 0)
		SV_RemoveObject(m_serverObjects.begin()->first);

	while (m_serverClients.size() > 0)
		SV_DestroyClient(m_serverClients.begin()->first);

	while (m_clientClients.size() > 0)
		CL_DestroyClient(m_clientClients.begin()->first);
}

int ReplicationManager::SV_CreateClient(Channel * channel, void * up)
{
	return CreateClientEx(channel, true, up);
}

int ReplicationManager::CL_CreateClient(Channel * channel, void * up)
{
	return CreateClientEx(channel, false, up);
}

void ReplicationManager::SV_DestroyClient(int clientID)
{
	ReplicationClientCollItr i = m_serverClients.find(clientID);

	AssertMsg(i != m_serverClients.end(), "client does not exist. clientId=%d", clientID);
	if (i != m_serverClients.end())
	{
		ReplicationClient * client = i->second;
		m_serverClientsCache.erase(client->m_channel);
		delete i->second;
		m_clientIDs.Free(clientID);
		m_serverClients.erase(i);
	}
}

void ReplicationManager::CL_DestroyClient(int clientID)
{
	ReplicationClientCollItr i = m_clientClients.find(clientID);

	AssertMsg(i != m_clientClients.end(), "client does not exist. clientId=%d", clientID);
	if (i != m_clientClients.end())
	{
		ReplicationClient * client = i->second;
		m_clientClientsCache.erase(client->m_channel);
		delete i->second;
		m_clientClients.erase(i);
	}
}

int ReplicationManager::SV_AddObject(const std::string & className, ReplicationObject * object)
{
	Assert(object);

	// fixme : may cause errors when object with objectID already exists on client and has not been
	// deleted (yet). free objectID's only when all clients are synced?

	uint16_t objectID = m_objectIDs.Allocate();
	uint32_t creationID = m_serverObjectCreationId++;

	object->SetObjectID(objectID);
	object->SetCreationID(creationID);

	m_serverObjects[objectID] = object;

	for (ReplicationClientCollItr i = m_serverClients.begin(); i != m_serverClients.end(); ++i)
		SyncClientObject(i->second, object);

	return objectID;
}

void ReplicationManager::SV_RemoveObject(int objectID)
{
	// todo : we should probably mark the object destroyed, and *really* destroy it once the
	//        last update serialization has taken place, to ensure any critical updates
	//        are received by the client too

	for (ReplicationClientCollItr i = m_serverClients.begin(); i != m_serverClients.end(); ++i)
	{
		ReplicationClient * client = i->second;

		ReplicationObjectStateColl createdOrDestroyed;
		ReplicationObjectStateColl active;

		client->SV_Move(objectID, client->m_createdOrDestroyed, createdOrDestroyed);
		client->SV_Move(objectID, client->m_active, active);

		Assert((createdOrDestroyed.size() + active.size()) <= 1);
		Assert(createdOrDestroyed.empty() || !createdOrDestroyed.front().m_isDestroyed);

		for (ReplicationObjectStateCollItr j = createdOrDestroyed.begin(); j != createdOrDestroyed.end(); ++j)
		{
			// object shouldn't be destroyed twice
			Assert(!j->m_isDestroyed);
			if (j->m_isDestroyed)
			{
				// but if it is, make sure that the client at least gets the message
				client->m_createdOrDestroyed.push_back(*j);
			}
			else
			{
				// object creation message wasn't sent yet. keep it off the list
			}
		}

		for (ReplicationObjectStateCollItr j = active.begin(); j != active.end(); ++j)
		{
			// add object to the destroyed object list

			Assert(!j->m_isDestroyed);
			j->m_object = 0;
			j->m_isDestroyed = true;
			client->m_createdOrDestroyed.push_back(*j);
		}
			
		AssertMsg(!(createdOrDestroyed.empty() && active.empty()), "could not find object in server client. objectId=%d", objectID);
	}

	ReplicationObjectCollItr j = m_serverObjects.find(objectID);

	AssertMsg(j != m_serverObjects.end(), "object does not exist. objectId=%d", objectID);
	if (j != m_serverObjects.end())
	{
		m_serverObjects.erase(j);
		m_objectIDs.Free(objectID);
	}
}

void ReplicationManager::SV_Shutdown()
{
}

void ReplicationManager::CL_Shutdown()
{
	for (ReplicationClientCollItr i = m_clientClients.begin(); i != m_clientClients.end(); ++i)
	{
		ReplicationClient * client = i->second;

		while (client->m_clientObjects.size() > 0)
		{
			ReplicationObject * object = client->m_clientObjects.begin()->second;
			CL_DestroyObject(client, object->GetObjectID());
		}

		//CL_DestroyClient(m_clientClients.begin()->first);
	}
}

void ReplicationManager::SV_Update()
{
	for (ReplicationClientCollItr i = m_serverClients.begin(); i != m_serverClients.end(); ++i)
	{
		ReplicationClient * client = i->second;

		// handle object creation and destruction

		for (ReplicationObjectStateCollItr j = client->m_createdOrDestroyed.begin(); j != client->m_createdOrDestroyed.end(); ++j)
		{
			if (j->m_isDestroyed)
			{
				// object has been destroyed. send destruction message to client

				BitStream bitStream;

				const uint16_t objectID = j->m_objectID;

				bitStream.Write(objectID);

				RepMgrPacketBuilder packetBuilder;
				Packet packet = MakePacket(REPMSG_DESTROY, packetBuilder, bitStream);
				client->m_channel->Send(packet, 0);
			}
			else
			{
				// object has been created. send creation message to client

				BitStream bitStream;

				const uint16_t objectID = j->m_object->GetObjectID();
				const std::string & className = j->m_object->ClassName();

				bitStream.Write(objectID);
				bitStream.WriteString(className);

				j->m_object->Serialize(bitStream, true, true);

				RepMgrPacketBuilder packetBuilder;
				Packet packet = MakePacket(REPMSG_CREATE, packetBuilder, bitStream);
				client->m_channel->Send(packet, 0);

				client->m_active.push_back(*j);
			}
		}

		client->m_createdOrDestroyed.clear();
	}

	// all clients should share the same view now with regard to the set of active objects

	if (!m_serverClients.empty())
	{
		ReplicationClient* clientForActiveObjects = m_serverClients.begin()->second; // fixme : make this nicer..

		for (ReplicationObjectStateCollItr j = clientForActiveObjects->m_active.begin(); j != clientForActiveObjects->m_active.end(); ++j)
		{
			if (j->m_object->RequiresUpdating() && j->m_object->RequiresUpdate())
			{
				// Go through server objects & replicate.

				BitStream bitStream;

				const uint16_t objectID = j->m_object->GetObjectID();

				bitStream.Write(objectID);

				if (j->m_object->Serialize(bitStream, false, true))
				{
					RepMgrPacketBuilder packetBuilder;
					Packet packet = MakePacket(REPMSG_UPDATE, packetBuilder, bitStream);

					for (ReplicationClientCollItr i = m_serverClients.begin(); i != m_serverClients.end(); ++i)
					{
						ReplicationClient* client = i->second;

						client->m_channel->Send(packet, 0);
					}
				}
			}
		}
	}

	m_tick++;
}

void ReplicationManager::CL_Update()
{
}

void ReplicationManager::CL_RegisterHandler(ReplicationHandler * handler)
{
	Assert(handler);

	m_handler = handler;
}

void ReplicationManager::OnReceive(Packet & packet, Channel * channel)
{
	NET_STAT_ADD(Replication_BytesReceived, packet.GetSize());

	uint8_t messageID;

	if (!packet.Read8(&messageID))
	{
		Assert(0);
		return;
	}

	uint16_t bitStreamSize;

	if (!packet.Read16(&bitStreamSize))
	{
		Assert(0);
		return;
	}

	Packet bitStreamPacket;

	if (!packet.Extract(bitStreamPacket, Net::BitsToBytes(bitStreamSize), true))
	{
		Assert(0);
		return;
	}

	BitStream bitStream(bitStreamPacket.GetData(), bitStreamSize);

	switch (messageID)
	{
	case REPMSG_CREATE:
		HandleCreate(bitStream, channel);
		break;
	case REPMSG_DESTROY:
		HandleDestroy(bitStream, channel);
		break;
	case REPMSG_UPDATE:
		HandleUpdate(bitStream, channel);
		break;
	default:
		AssertMsg(false, "unknown message. messageId=%d", messageID);
		break;
	}
}

void ReplicationManager::HandleCreate(BitStream & bitStream, Channel * channel)
{
	ReplicationClient * client = CL_FindClient(channel);

	if (!client)
	{
		AssertMsg(false, "received create from unknown channel. channelId=%d", channel->m_id);
		return;
	}

	uint16_t objectID;
	bitStream.Read(objectID);

	// Check if object already created. If so, ack.
	if (client->CL_FindObject(objectID) != 0)
	{
		AssertMsg(false, "object already created. objectId=%d", objectID);
		return;
	}

	std::string className;
	className = bitStream.ReadString();

	ReplicationObject * object;

	// Retrieve parameters through callback.
	if (!m_handler->OnReplicationObjectCreate1(client, className, &object))
	{
		AssertMsg(false, "unable to create object. className=%s", className.c_str());
		return;
	}

	NET_STAT_INC(Replication_ObjectsCreated);

	object->SetObjectID(objectID);
	object->Serialize(bitStream, true, false);

	m_handler->OnReplicationObjectCreate2(client, object);

	client->CL_AddObject(object);
}

void ReplicationManager::HandleDestroy(BitStream & bitStream, Channel * channel)
{
	ReplicationClient * client = CL_FindClient(channel);

	if (!client)
	{
		DB_ERR("received destroy request from unknown channel (%d)", channel->m_id);
		return;
	}

	uint16_t objectID;

	bitStream.Read(objectID);

	if (CL_DestroyObject(client, objectID))
	{
		NET_STAT_INC(Replication_ObjectsDestroyed);
	}
	else
	{
		DB_ERR("received destroy request for non-existing object (%d)", objectID);
		return;
	}
}

void ReplicationManager::HandleUpdate(BitStream & bitStream, Channel * channel)
{
	NET_STAT_INC(Replication_ObjectsUpdated);

	ReplicationClient * client = CL_FindClient(channel);

	if (!client)
	{
		DB_ERR("received update from unknown channel (%d)", channel->m_id);
		return;
	}

	uint16_t objectID;

	bitStream.Read(objectID);

	ReplicationObject * object = client->CL_FindObject(objectID);

	if (object)
	{
		object->Serialize(bitStream, false, false);
	}
	else
	{
		DB_ERR("received update for non-existing object (%d)", objectID);
		return;
	}
}

int ReplicationManager::CreateClientEx(Channel * channel, bool serverSide, void * up)
{
	Assert(channel);

	int clientID = m_clientIDs.Allocate();

	ReplicationClient * repClient = new ReplicationClient();
	repClient->Initialize(channel, up);

	if (serverSide)
	{
		m_serverClients[clientID] = repClient;
		m_serverClientsCache[repClient->m_channel] = repClient;

		SyncClient(repClient);
	}
	else
	{
		m_clientClients[clientID] = repClient;
		m_clientClientsCache[repClient->m_channel] = repClient;
	}

	return clientID;
}

void ReplicationManager::SyncClient(ReplicationClient * client)
{
	std::vector<ReplicationObject*> objects;

	for (ReplicationObjectCollItr i = m_serverObjects.begin(); i != m_serverObjects.end(); ++i)
		objects.push_back(i->second);

	std::sort(objects.begin(), objects.end(), [] (ReplicationObject* o1, ReplicationObject* o2) { return o1->GetCreationID() < o2->GetCreationID(); });

	for (std::vector<ReplicationObject*>::iterator i = objects.begin(); i != objects.end(); ++i)
		SyncClientObject(client, *i);
}

void ReplicationManager::SyncClientObject(ReplicationClient * client, ReplicationObject * object)
{
	client->SV_AddObject(object);
}

ReplicationClient * ReplicationManager::SV_FindClient(Channel * channel)
{
	ReplicationClientCacheItr i = m_serverClientsCache.find(channel);

	if (i == m_serverClientsCache.end())
		return 0;
	else
		return i->second;
}

ReplicationClient * ReplicationManager::CL_FindClient(Channel * channel)
{
	ReplicationClientCacheItr i = m_clientClientsCache.find(channel);

	if (i == m_clientClientsCache.end())
		return 0;
	else
		return i->second;
}

ReplicationObject * ReplicationManager::SV_FindObject(int objectID)
{
	ReplicationObjectCollItr i = m_serverObjects.find(objectID);

	if (i == m_serverObjects.end())
		return 0;
	else
		return i->second;
}

bool ReplicationManager::CL_DestroyObject(ReplicationClient * client, int objectID)
{
	Assert(client);

	ReplicationObject * object = client->CL_FindObject(objectID);

	if (object)
	{
		client->CL_RemoveObject(object);

		m_handler->OnReplicationObjectDestroy(client, object);

		return true;
	}

	return false;
}

Packet ReplicationManager::MakePacket(uint8_t messageID, RepMgrPacketBuilder & packetBuilder, BitStream & bitStream) const
{
	uint8_t protocolID = PROTOCOL_REPLICATION;
	packetBuilder.Write8(&protocolID);
	packetBuilder.Write8(&messageID);
	uint16_t bitStreamSize = bitStream.GetDataSize();
	packetBuilder.Write16(&bitStreamSize);
	packetBuilder.Write(bitStream.GetData(), Net::BitsToBytes(bitStream.GetDataSize()));
	return packetBuilder.ToPacket();
}
