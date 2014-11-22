#pragma once

#include "NetSerializable.h"
#include "ReplicationObject.h"

enum NetObjectType
{
	kNetObjectType_Arena,
	kNetObjectType_Player
};

class NetObject_NS : public NetSerializable
{
	virtual void SerializeStruct()
	{
		bool hasOwningChannelId = (owningChannelId != 0);
		Serialize(hasOwningChannelId);
		if (hasOwningChannelId)
			Serialize(owningChannelId);
	}

public:
	NetObject_NS(NetSerializableObject * owner)
		: NetSerializable(owner)
		, owningChannelId(0)
	{
	}

	uint16_t owningChannelId;
};

class NetObject : public NetSerializableObject, public ReplicationObject
{
	NetObject_NS m_netObject_NS;

	virtual bool Serialize(BitStream & bitStream, bool init, bool send, int channel)
	{
		return NetSerializableObject::Serialize(init, send, channel, bitStream);
	}

public:
	NetObject()
		: m_netObject_NS(this)
	{
	}

	virtual NetObjectType getType() const = 0;

	uint16_t getOwningChannelId() const { return m_netObject_NS.owningChannelId; }
	void setOwningChannelId(uint16_t channelId) { m_netObject_NS.owningChannelId = channelId; }
};
