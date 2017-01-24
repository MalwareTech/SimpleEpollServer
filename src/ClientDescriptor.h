#pragma once

#include <netinet/in.h>
#include <stdexcept>


/*
	This is a base client descriptor and virtual methods should be implemented by a derived class.
	Returning false from any of the methods will result in Cleanup() being called and the client 
	descriptor being deconstructed immediately.
*/
class ClientDescriptor
{
public:
	ClientDescriptor(int fd, in_addr client_addr, uint16_t client_port, uint32_t timeout) :
		fd_(fd),
		client_addr_(client_addr),
		client_port_(client_port),
		timeout_(timeout)
	{
	}

	virtual ~ClientDescriptor()
	{

	}

	//called when a client fd becomes available for writing
	virtual bool ReadReady() { throw std::runtime_error("ReadReady() not implemented"); }

	//called when a client fd becomes available for reading
	virtual bool WriteReady() { throw std::runtime_error("WriteReady() not implemented"); }

	//called periodically to check if fd is still alive (used to implement timeout)
	virtual bool HeartBeat() { throw std::runtime_error("HeartBeat() not implemented"); }

	//called when the server is done with the client and the fd should be closed
	virtual void ServerClose() { throw std::runtime_error("ServerClose() not implemented"); }

	//called if the connection was forcibly closed by the client
	virtual void ClientClose() { throw std::runtime_error("ClientClose() not implemented"); }

	//client's unique id
	int uid() { return fd_;  }

protected:
	int fd_;
	in_addr client_addr_;
	uint16_t client_port_;
	uint32_t timeout_;
};