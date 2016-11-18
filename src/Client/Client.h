#ifdef __linux__ 
	#define Unix
#elif _WIN32
	#define Windows
#endif

#include <stdio.h>
#include <string>
#include <iostream>
#include <time.h>

#ifdef Windows
	#include <winsock2.h>
	#pragma comment(lib,"ws2_32.lib")
#else
	#include<sys/socket.h>
	#include<arpa/inet.h>
#endif



class Client
{

private:
	
	#ifdef Windows
	SOCKET client_socket, udp_socket;
	WSADATA wsa;
	#else
	int client_socket, udp_socket;
	#endif

	sockaddr_in server_inf;
	bool reconnectionFlag;
	std::string mode;
	bool is_connection;
	
public:
	Client(char*,int);
	void run();
	void uploadFile();
	int getFileSize(FILE*);
	int uploadFile(char*);
	int downloadFile(char*);
	int downloadUdpFile(char*);
	int uploadUdpFile(char*);
	int commandHandling(char*);
	bool reconnectToServer();
	int reconnectTimer(int);
	void changeMode();
	void tcpCommandRoute(std::string);
	void udpCommandRoute(std::string);
	~Client();
};


