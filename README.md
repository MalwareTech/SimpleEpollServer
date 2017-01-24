# SimpleEpollServer
An example epoll imlementation with C++11

This is an example edge-triggered epoll server I created when I was learning to write code for linux, it povides:
* AsyncServer - a template class which implements the epoll handler
* ClientDescriptor - a base class used as a client handler
* ExampleClient - a subclass of ClientDescriptor which implements a basic echo server

The following methods must be implemented when deriving ClientDescriptor:
```cpp
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
```
