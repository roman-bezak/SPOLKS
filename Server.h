#include "Os.h"
#include <stdio.h>
#include <string>

#ifdef Windows
	#include <winsock2.h>
#else
	#include<sys/socket.h>
	#include<arpa/inet.h>
#endif


#pragma comment(lib,"ws2_32.lib")

struct Session{

	FILE *d_file;
	std::string filename;
	std::string operation_status;
	int reciveSize;
	int sendigSize;
	int fileSize;

	Session(){
		d_file = NULL;
		operation_status = "DEFAULT";
		filename = " ";
		reciveSize = -1;
		sendigSize = -1;
		fileSize = -1;
	}

	void clear(){
		if(d_file != NULL){
			fclose(d_file);
			d_file = NULL;
		}
		operation_status = "DEFAULT";
		reciveSize = -1;
		sendigSize = -1;
		fileSize = -1;
		filename = " ";
	}
	void setSessionData(std::string file_name,std::string o_s, int file_size, int sending_size, int recive_size){
		filename = file_name;
		operation_status = o_s;
		fileSize = file_size;
		sendigSize = sending_size;
		reciveSize = recive_size;
	}
};

struct Client{

	#ifdef Windows
		SOCKET c_socket;
	#else
		int c_socket;
	#endif

	std::string replyBuffer;
	sockaddr_in inf;
	Session c_session;


	#ifdef Windows
		Client(SOCKET s,sockaddr_in i)
	#else
		Client(int s,sockaddr_in i)
	#endif
		{
			c_socket = s;
			inf = i;
			replyBuffer = "";
		}

	char* getIpAddress(){
		return inet_ntoa(this->inf.sin_addr);
	}
	int getPort(){
		return ntohs(this->inf.sin_port);
	}

};


class Server{

private:

	#ifdef Windows
		WSADATA wsa;
		SOCKET server_s;
		SOCKET client_s;
	#else
		int server_s;
		int client_s;
	#endif

	int port;
	sockaddr_in server_inf;
	sockaddr_in client_inf;

public:

	Server(int);
	~Server();
};

