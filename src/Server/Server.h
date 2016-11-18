#include <stdio.h>
#include <string>
#include <ctime>
#include <iostream>
#include <vector>
#include <sstream>
#include <algorithm>

#ifdef __linux__ 
	#define Unix
#elif _WIN32
	#define Windows
#endif

#ifdef Windows
	#include <winsock2.h>
	#pragma comment(lib,"ws2_32.lib")
#else
	#include<sys/socket.h>
	#include<arpa/inet.h>
#endif



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
	Client(SOCKET s,sockaddr_in i){
	#else
	Client(int s,sockaddr_in i){
	#endif
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

struct UdpSession{


	FILE *d_file;
	std::string filename;
	std::string operation_status;
	sockaddr_in inf;
	int reciveSize;
	int sendigSize;
	int fileSize;

	UdpSession(){
		d_file = NULL;
		operation_status = "DEFAULT";
		filename = " ";
		reciveSize = -1;
		sendigSize = -1;
		fileSize = -1;
	}

	UdpSession(sockaddr_in _inf){
		inf = _inf;
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

class Server
{

	private:

		#ifdef Windows
		WSADATA wsa;
		SOCKET server_socket, udp_server_socket, client_socket, new_clientSocket;
		#else
		int server_socket, udp_server_socket, client_socket, new_clientSocket;
		#endif


		sockaddr_in serverInf, new_clientInf;
		std::vector<UdpSession> udp_sessions;
		std::vector<Client> clients;
		int port;
		

	public:
		Server(int);
		bool start();
		int commandHandling(char*);
		std::string currentDateTime();

		void commandDefaultRouting(int,char*,sockaddr_in);

		void uploadInit(int);
		void uploadUdpInit(int,char*);
		void downloadInit(int);
		void downloadUdpInit(int,char*);

		void reciveFileProcessing(int);
		void downloadFileProcessing(int);

		void reciveFileUdpProcessing(int);
		void downloadFileUdpProcessing(int);

		int checkIsUdpSession(sockaddr_in);
		bool searchEscapeChars(char*, int, int, std::string);
		void serverSetUp();

		~Server();
};


