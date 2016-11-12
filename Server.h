#include "Os.h"
#include <stdio.h>
#include <string>
#include <ctime>

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
		SOCKET c_socket_tcp;
		SOCKET c_socket_udp;
	#else
		int c_socket_tcp;
		int c_socket_udp
	#endif

	std::string replyBuffer;
	sockaddr_in inf;
	Session c_session;

	Client(){

		c_socket_tcp = 0;
		c_socket_udp = 0;
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
		SOCKET server_s_tcp;
		SOCKET server_s_udp;
	#else
		int server_s_tcp;
		int server_s_udp;
	#endif

	int port;
	std::string mode;
	sockaddr_in server_inf;

	Client client;

public:

	
	int commandHandling(char*);
	int serverSetUp();
	void acceptTcp();
	void start(std::string);
	std::string currentDateTime();

	Server(int, std::string);
	~Server();
};

