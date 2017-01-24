#include "ExampleClient.h"

bool ExampleClient::ReadReady()
{
	char buffer[1024];
	int bytes_read;
	std::string data_buffer;

	//we must drain the entire read buffer as we won't get another event until client sends more data
	while(1)
	{
		bytes_read = recv(fd_, buffer, 1024, 0);
		if(bytes_read <= 0)
			break;

		data_buffer.append(buffer, bytes_read);
	}

	//client triggered EPOLLIN but sent no data (usually due to remote socket being closed)
	if(data_buffer.length() == 0)
		return true;

	printf("[i] client %s:%d said: %s\n", inet_ntoa(client_addr_), client_port_, data_buffer.c_str());

	write(fd_, data_buffer.c_str(), data_buffer.size());

	//update last active time to prevent timeout
	last_active_ = time(0);

	return true;
}

bool ExampleClient::WriteReady()
{
/*
	during heavy network I/O fds can become unwritable and subsequent calls to write() / send() will fail,
	in this case the data which failed to send should be stored in a buffer and the operation should be
	retried when WriteReady() is called to signal the fd is writable again (this is up to you to implement).
*/
	return true;
}

bool ExampleClient::HeartBeat()
{
	//if no operations occurred during timeout period return false to signal server to close socket
	if(static_cast<time_t>(last_active_ + timeout_) < time(0))
	{
		printf("[i] connection %s:%d has timed out\n", inet_ntoa(client_addr_), client_port_);
		return false;
	}

	return true;
}

void ExampleClient::ClientClose()
{
	close(fd_);

	printf("[-] connection %s:%d closed by client\n", inet_ntoa(client_addr_), client_port_);
}

void ExampleClient::ServerClose() 
{
	close(fd_);

	printf("[-] connection %s:%d closed by server\n", inet_ntoa(client_addr_), client_port_);
}