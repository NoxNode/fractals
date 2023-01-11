#pragma once

#include "types.h"
#include <stdio.h>
#include <string.h>
#include "packet.h"

#ifdef _WIN32
	#include <winsock2.h>
	// #pragma comment( lib, "wsock32.lib" )
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <fcntl.h>
	#include <unistd.h>
#endif

bool InitializeUDPSockets() {
	#ifdef _WIN32
		WSADATA WsaData;
		return WSAStartup(MAKEWORD(2, 2), &WsaData) == NO_ERROR;
	#else
		return true;
	#endif
}

void ShutdownUDPSockets() {
	#ifdef _WIN32
		WSACleanup();
	#endif
}

bool CreateUDPSocket(int& socket_handle, u16 port) {
    socket_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(socket_handle <= 0) {
        printf("CreateUDPSocket() Error: failed to create socket\n");
        return false;
    }

	struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if(::bind(socket_handle, (const sockaddr*) &address, sizeof(struct sockaddr_in)) < 0) {
        printf("CreateUDPSocket() Error: failed to bind socket\n");
        return false;
    }

	// set non-blocking
	#ifdef _WIN32
		DWORD nonBlocking = 1;
		if(ioctlsocket(socket_handle, FIONBIO, &nonBlocking) != 0) {
			printf("CreateUDPSocket() Error: failed to set non-blocking\n");
			return false;
		}
	#else
		int nonBlocking = 1;
		if(fcntl(socket_handle, F_SETFL, O_NONBLOCK, nonBlocking) == -1) {
			printf("CreateUDPSocket() Error: failed to set non-blocking\n");
			return false;
		}
	#endif
	return true;
}

void CloseUDPSocket(int& socket_handle) {
	#ifdef _WIN32
		closesocket(socket_handle);
	#else
		close(socket_handle);
	#endif
}

bool SendUDPPacket(int& socket_handle, Packet& packet, struct sockaddr_in& to_address) {
	s32 sent_bytes = sendto(socket_handle, (const char*) packet.data, packet.size, 0, (struct sockaddr*) &to_address, sizeof(struct sockaddr_in));
	if(sent_bytes != packet.size) {
		printf("SendUDPPacket() Error: failed to send packet\n");
		return false;
	}
	return true;
}

bool RecvUDPPacket(int& socket_handle, Packet& packet, struct sockaddr_in& from_address) {
	#ifdef _WIN32
		typedef int socklen_t;
	#endif
	socklen_t from_len = sizeof(from_address);
	memset(packet.data, 0, packet.maxSize);
	s32 recvBytes = recvfrom(socket_handle, (char*) packet.data, packet.maxSize, 0, (sockaddr*) &from_address, &from_len);
	if(recvBytes <= 0)
		return false;
	packet.size = recvBytes;
	return true;
}
