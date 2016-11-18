#include<stdio.h>
#include<winsock2.h>
#include <string>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

class Client
{

private:

	WSADATA wsa;
	SOCKET client_socket, udp_socket;
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


