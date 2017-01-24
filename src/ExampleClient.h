#pragma once

#include <netinet/in.h>
#include <netinet/ip.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include "ClientDescriptor.h"

class ExampleClient : public ClientDescriptor
{
public:
	ExampleClient(int fd, in_addr client_addr, uint16_t client_port, uint32_t timeout) : 
		ClientDescriptor(fd, client_addr, client_port, timeout),
		last_active_(time(0))
	{
	}

	bool ReadReady();
	bool WriteReady();
	bool HeartBeat();
	void ClientClose();
	void ServerClose();

protected:
	time_t last_active_;
};

