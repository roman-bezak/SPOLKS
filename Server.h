#include <stdio.h>
#include <winsock2.h>

#pragma comment(lib,"ws2_32.lib")

class Server{

private:

	WSADATA wsa;
	SOCKET server_s;
	SOCKET client_s;

	sockaddr_in server;
	sockaddr_in client;

public:


	Server();
	~Server();
};

