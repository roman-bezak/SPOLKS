#include "Server.h"


Server::Server(int _port, std::string _mode){

	this->mode = _mode;
	this->port = _port;
	this->server_inf.sin_family = AF_INET;
    this->server_inf.sin_addr.s_addr = INADDR_ANY;
	this->server_inf.sin_port = htons( this->port );

}


Server::~Server(){
}

std::string Server::currentDateTime() {
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time (&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer,80,"Current server time: %d-%m-%Y %I:%M:%S",timeinfo);
	std::string str(buffer);

	return str;
}


int Server::commandHandling(char* com){

	char *m[] = {
		"ECHO",
		"TIME",
		"UPLOAD",
		"DOWNLOAD",
		"CLOSE"
	};

	char *command = (char*)malloc(strlen(com));
	strcpy(command,com);
	char split[] = " ";
	char* token = NULL;

	if(token = strtok(command, split)){
		for (int i = 0; i < sizeof(m)/sizeof(char*); i++)
			if(!strcmp(m[i], token))
				return i;
		return -1;

	}else return -1;

}

void Server::start(std::string _mode){

	int addrLen = sizeof(struct sockaddr_in);

	while (true){

		#ifdef Windows
			if ((this->client.c_socket_tcp = accept(this->server_s, (struct sockaddr *)&this->client_inf, (int *)&addrLen))<0)
				printf("Accept error");
		#else
			if((this->client.c_socket_tcp = accept(this->server_s, (struct sockaddr *)&this->client_inf, (socklen_t*)&addrLen)) < 0)
				printf("Accept error");
		#endif



	}//while
}

int Server::serverSetUp(){
		
	if(mode == "TCP"){

		#ifdef Windows
			if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
				printf("Failed. Error Code : %d",WSAGetLastError());
		#endif

		#ifdef Windows
			if((this->server_s_tcp = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
				printf("Could not create socket : %d" , WSAGetLastError());
		#else
			if((this->server_s_tcp = socket(AF_INET , SOCK_STREAM , 0 )) == -1)
				printf("Could not create socket");
		#endif
 
		printf("Socket created.\n");
     
		#ifdef Windows
			if( bind(this->server_s_tcp ,(struct sockaddr *)&this->server_inf , sizeof(this->server_inf)) == SOCKET_ERROR)
				printf("Bind failed with error code : %d" , WSAGetLastError());
		#else
			if( bind(this->server_s_tcp,(struct sockaddr *)&this->server_inf , sizeof(this->server_inf)) < 0)
				printf("bind failed");
		#endif

		puts("Bind done");
		listen(this->server_s_tcp , 1);
	}else if(mode == "UDP"){
		
	}
}