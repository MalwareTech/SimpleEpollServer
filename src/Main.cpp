#include "AsyncServer.h"
#include "ExampleClient.h"

int main(int argc, char *argv[])
{
	//parameters: (listen_ip, listen_port, timeout)
	AsyncServer<ExampleClient> async_server("0.0.0.0", 1337, 10);
	async_server.EventLoop();
}