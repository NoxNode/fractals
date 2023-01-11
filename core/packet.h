#pragma once
#include "types.h"
#include <stdio.h>

#ifdef _WIN32
	#include <winsock2.h>
	// #pragma comment( lib, "wsock32.lib" )
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <fcntl.h>
	#include <unistd.h>
#endif

struct Packet {
	u8* data;
	u32 size;
	u32 start;
	u32 maxSize;
};

void InitPacket(Packet& packet, u8* dataArr, u32 maxPacketSize) {
	packet.data = dataArr;
	packet.maxSize = maxPacketSize;
	packet.start = 0;
	packet.size = 0;
}

void ReinitPacket(Packet& packet) {
	packet.start = 0;
	packet.size = 0;
}

////////// general purpose functions for adding and retrieving data from packets //////////

Packet& operator<<(Packet& packet, const u8& data) {
	if(packet.size + sizeof(u8) > packet.maxSize) {
		printf("Packet << Error: packet cannot hold any more data of that size\n");
		return packet;
	}

	u8* dataArray = (u8*) (&packet.data[0] + packet.size);
	dataArray[0] = data;

	packet.size += sizeof(u8);
	return packet;
}

Packet& operator>>(Packet& packet, u8& data) {
	if((s32)packet.size - (s32)sizeof(u8) < 0) {
		printf("Packet >> Error: packet cannot extract any more data of that size\n");
		return packet;
	}

	u8* dataArray = (u8*) (&packet.data[0] + packet.start);
	data = dataArray[0];

	packet.size  -= sizeof(u8);
	packet.start += sizeof(u8);
	return packet;
}

Packet& operator<<(Packet& packet, const u16& data) {
	if(packet.size + sizeof(u16) > packet.maxSize) {
		printf("Packet << Error: packet cannot hold any more data of that size\n");
		return packet;
	}

	u16* dataArray = (u16*) (&packet.data[0] + packet.size);
	dataArray[0] = htons(data);

	packet.size += sizeof(u16);
	return packet;
}

Packet& operator>>(Packet& packet, u16& data) {
	if((s32)packet.size - (s32)sizeof(u16) < 0) {
		printf("Packet >> Error: packet cannot extract any more data of that size\n");
		return packet;
	}

	u16* dataArray = (u16*) (&packet.data[0] + packet.start);
	data = ntohs(dataArray[0]);

	packet.size  -= sizeof(u16);
	packet.start += sizeof(u16);
	return packet;
}

Packet& operator<<(Packet& packet, const u32& data) {
	if(packet.size + sizeof(u32) > packet.maxSize) {
		printf("Packet << Error: packet cannot hold any more data of that size\n");
		return packet;
	}

	u32* dataArray = (u32*) (&packet.data[0] + packet.size);
	dataArray[0] = htonl(data);

	packet.size += sizeof(u32);
	return packet;
}

Packet& operator>>(Packet& packet, u32& data) {
	if((s32)packet.size - (s32)sizeof(u32) < 0) {
		printf("Packet >> Error: packet cannot extract any more data of that size\n");
		return packet;
	}

	u32* dataArray = (u32*) (&packet.data[0] + packet.start);
	data = ntohl(dataArray[0]);

	packet.size  -= sizeof(u32);
	packet.start += sizeof(u32);
	return packet;
}

Packet& operator<<(Packet& packet, const r32& data) {
	if(packet.size + sizeof(u32) > packet.maxSize) {
		printf("Packet << Error: packet cannot hold any more data of that size\n");
		return packet;
	}

	u32* dataArray = (u32*) (&packet.data[0] + packet.size);
	u32 casted_data = *((u32*)&data);
	dataArray[0] = htonl(casted_data);

	packet.size += sizeof(u32);
	return packet;
}

Packet& operator>>(Packet& packet, r32& data) {
	if((s32)packet.size - (s32)sizeof(u32) < 0) {
		printf("Packet >> Error: packet cannot extract any more data of that size\n");
		return packet;
	}

	u32* dataArray = (u32*) (&packet.data[0] + packet.start);
	u32 uncastedData = ntohl(dataArray[0]);
	data = *((r32*)&uncastedData);

	packet.size  -= sizeof(u32);
	packet.start += sizeof(u32);
	return packet;
}

Packet& operator<<(Packet& packet, const u64& data) {
	if(packet.size + sizeof(u64) > packet.maxSize) {
		printf("Packet << Error: packet cannot hold any more data of that size\n");
		return packet;
	}

	u32* dataArray = (u32*) (&packet.data[0] + packet.size);
	dataArray[0] = htonl((u32)(data & 0x00000000FFFFFFFF));
	dataArray[1] = htonl((u32)(data >> 32));

	packet.size += sizeof(u64);
	return packet;
}

Packet& operator>>(Packet& packet, u64& data) {
	if((s32)packet.size - (s32)sizeof(u64) < 0) {
		printf("Packet >> Error: packet cannot extract any more data of that size\n");
		return packet;
	}

	u32* dataArray = (u32*) (&packet.data[0] + packet.start);
	data = (u64)ntohl(dataArray[0]);
	data |= (u64)ntohl(dataArray[1]) << 32;

	packet.size  -= sizeof(u64);
	packet.start += sizeof(u64);
	return packet;
}

Packet& operator<<(Packet& packet, const v3& data) {
	packet << data.x;
	packet << data.y;
	packet << data.z;
	return packet;
}

Packet& operator>>(Packet& packet, v3& data) {
	packet >> data.x;
	packet >> data.y;
	packet >> data.z;
	return packet;
}

Packet& operator<<(Packet& packet, const v2& data) {
	packet << data.x;
	packet << data.y;
	return packet;
}

Packet& operator>>(Packet& packet, v2& data) {
	packet >> data.x;
	packet >> data.y;
	return packet;
}
