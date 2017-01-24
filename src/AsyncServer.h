#pragma once

#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <vector>
#include <errno.h>
#include <stdexcept>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <string>
#include <map>
#include "ClientDescriptor.h"

template <class ClientDescriptorType> class AsyncServer
{
public:
	AsyncServer(const char *listen_addr, uint16_t listen_port, uint32_t timeout_secs) :
		listen_fd_(-1),
		epoll_fd_(-1),
		timeout_secs_(timeout_secs),
		last_socket_check_(0)
	{
		sockaddr_in sin = { 0 };

		sin.sin_addr.s_addr = inet_addr(listen_addr);
		sin.sin_family = AF_INET;
		sin.sin_port = htons(listen_port);

		listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
		if(listen_fd_ <= 0)
			throw std::runtime_error("socket() failed, error code: " + std::to_string(errno));

		if(bind(listen_fd_, reinterpret_cast<sockaddr *>(&sin), sizeof(sin)))
			throw std::runtime_error("bind() failed, error code: " + std::to_string(errno));

		if(SetNonblocking(listen_fd_) == false)
			throw std::runtime_error("SetNonBlocking() failed, error code: " + std::to_string(errno));

		if(listen(listen_fd_, SOMAXCONN) == -1)
			throw std::runtime_error("listen() failed, error code: " + std::to_string(errno));

		epoll_fd_ = epoll_create1(0);
		if(epoll_fd_ == -1)
			throw std::runtime_error("epoll_create1() failed, error code: " + std::to_string(errno));

		epoll_event e_event;
		e_event.events = EPOLLIN;
		e_event.data.fd = listen_fd_;

		if(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &e_event) == -1)
			throw std::runtime_error("epoll_ctl() failed, error code: " + std::to_string(errno));

		events_ = new epoll_event[64];
	}

	~AsyncServer()
	{
		if(listen_fd_ != -1)
			close(listen_fd_);

		if(epoll_fd_ != -1)
			close(epoll_fd_);

		delete[] events_;
	}

	void EventLoop()
	{
		while(1)
		{
			int num_fds = epoll_wait(epoll_fd_, events_, 64, 1000);
			if(num_fds != -1)
			{
				//iterate signaled fds
				for(int i = 0; i < num_fds; i++)
				{
					//notifications on listening fd are incoming client connections
					if(events_[i].data.fd == listen_fd_)
					{
						HandleAccept();
					} else {
						HandleClient(events_[i]);
					}
				}
			}

			//perform cleanup every second and remove timed-out sockets
			if((last_socket_check_ + 1) < time(0) && clients_.size() > 0)
			{
				std::map<int, ClientDescriptor *>::iterator it = clients_.begin();
				while(it != clients_.end()) 
				{
					ClientDescriptor *client = (*it).second;
					
					if(!client->HeartBeat())
					{
						//if HeartBeat() returns false remove fd from map and close
						it = clients_.erase(it);
						client->ServerClose();
						delete client;
					} else {
						it++;
					}
				}

				last_socket_check_ = time(0);
			}
		}
	}

private:
	bool SetNonblocking(int fd)
	{
		int flags = fcntl(fd, F_GETFL, 0);
		if(flags == -1)
			return false;

		flags |= O_NONBLOCK;

		if(fcntl(fd, F_SETFL, flags) == -1)
			return false;

		return true;
	}

	//called whenever an EPOLLIN event occurs on the server fd
	bool HandleAccept()
	{
		sockaddr_in client_sin;
		socklen_t sin_size = sizeof(client_sin);
		ClientDescriptorType *client;

		int client_fd = accept(listen_fd_, reinterpret_cast<sockaddr *>(&client_sin), &sin_size);
		if(client_fd == -1)
		{
			printf("accept() failed, error code: %d\n", errno);
			return false;
		}

		if(!SetNonblocking(client_fd))
		{
			printf("failed to put fd into non-blocking mode, error code: %d\n", errno);
			return false;
		}

		//allocate and initialize a new descriptor for the client
		client = new ClientDescriptorType(client_fd, client_sin.sin_addr, 
										  ntohs(client_sin.sin_port), 
										  timeout_secs_);

		epoll_event ev;
		ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;	//client events will be handled in edge-triggered mode
		ev.data.ptr = client;						//we will pass client descriptor with every event

		if(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) == 1)
		{
			printf("epoll_ctl() failed, error code: %d\n", errno);
			delete client;
			return false;
		}

		//store new client descriptor into the map of clients
		clients_[client_fd] = client;

		printf("[+] new client: %s:%d\n", inet_ntoa(client_sin.sin_addr), ntohs(client_sin.sin_port));
		return true;
	}

	//called whenever and EPOLLIN event occurs on a client fd
	bool HandleClient(epoll_event ev)
	{
		//retrieve client descriptor address from the data parameter
		ClientDescriptor *client = reinterpret_cast<ClientDescriptor *>(ev.data.ptr);

		//we got some data from the client
		if(ev.events & EPOLLIN)
		{
			if(!client->ReadReady())
			{
				RemoveClient(client);
				client->ServerClose();
				delete client;
				return false;
			}
		}

		//the client closed the connection (should be after EPOLLIN as client can send data then close)
		if(ev.events & EPOLLRDHUP)
		{
			RemoveClient(client);
			client->ClientClose();
			delete client;
			return false;
		}

		//fd is ready to be written
		if(ev.events & EPOLLOUT)
		{
			if(!client->WriteReady())
			{
				RemoveClient(client);
				client->ServerClose();
				delete client;
				return false;
			}
		}

		return true;
	}

	void RemoveClient(ClientDescriptor *client)
	{
		std::map<int, ClientDescriptor *>::iterator it = clients_.find(client->uid());
		clients_.erase(it);
	}

private:
	int listen_fd_, epoll_fd_;
	epoll_event *events_;
	std::map<int, ClientDescriptor *> clients_;
	uint32_t timeout_secs_;
	time_t last_socket_check_;
};