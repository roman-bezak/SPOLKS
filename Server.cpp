#include "Server.h"


Server::Server(int _port, std::string _mode){

	this->mode = _mode;
	this->port = _port;

	this->server_inf.sin_family = AF_INET;
    this->server_inf.sin_addr.s_addr = INADDR_ANY;
	this->server_inf.sin_port = htons( this->port );

	this->serverSetUp();

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

void Server::acceptTcp(){

	int addrLen = sizeof(struct sockaddr_in);

	#ifdef Windows
	if ((this->client.c_socket_tcp = accept(this->server_s_tcp, (struct sockaddr *)&this->client.inf, (int *)&addrLen))<0)
			printf("Accept error");
	#else
		if((this->client.c_socket_tcp = accept(this->server_s_tcp, (struct sockaddr *)&this->client.inf, (socklen_t*)&addrLen)) < 0)
			printf("Accept error");
	#endif

	 timeval tv;
	 tv.tv_sec  = 1;
	 tv.tv_usec = 0;

	 setsockopt(this->client.c_socket_tcp, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&tv), sizeof(timeval));

}



void Server::start(){

	char command_buf[256];
	int bytes_recv = -1;

	if(this->mode == "TCP"){
		
		while (true){

			this->acceptTcp();

			while (true){
				
				bytes_recv = recv(this->client.c_socket_tcp,command_buf,256,0);

				if(bytes_recv == -1){
				
				}else if(bytes_recv == 0){
					
				}else {

					if(this->searchEscapeChars(command_buf,bytes_recv)){
						this->commandDefaultRouting();
					}

				}//else
			}
		}

	}else if(this->mode == "UDP"){
		//
	}
}

bool Server::searchEscapeChars(char *command_buf, int bytes_recv){

	char *command = (char*)malloc(strlen(command_buf));
	strcpy(command,command_buf);
	command[bytes_recv] = '\0';
	std::string temp(command);
	std::string sh_one("\r\n");
	std::string sh_two("\n");
	bool issetEndOfCommand = false;

	std::size_t found = temp.find(sh_one);
	if (found!=std::string::npos){
		temp[found] = '\0';
		this->client.replyBuffer.append(temp);
		issetEndOfCommand = true;
	}else{

		found = temp.find(sh_two);
		if (found!=std::string::npos){
			temp[found] = '\0';
			this->client.replyBuffer.append(temp);
			issetEndOfCommand = true;
		}else {
			issetEndOfCommand = false;
			this->client.replyBuffer.append(temp);
		}				
	}

	return issetEndOfCommand;
}

int Server::serverSetUp(){
	
	#ifdef Windows
		if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
			printf("Failed. Error Code : %d",WSAGetLastError());
	#endif

	if(mode == "TCP"){

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

		#ifdef Windows
			if((this->server_s_udp = socket(AF_INET, SOCK_DGRAM , 0 )) == INVALID_SOCKET)
				printf("Could not create socket : %d" , WSAGetLastError());
		#else
			if((this->server_s_udp = socket(AF_INET, SOCK_DGRAM , 0 )) == -1)
				printf("Could not create socket");
		#endif
 
		printf("Socket created.\n");
     
		#ifdef Windows
			if( bind(this->server_s_udp ,(struct sockaddr *)&this->server_inf , sizeof(this->server_inf)) == SOCKET_ERROR)
				printf("Bind failed with error code : %d" , WSAGetLastError());
		#else
			if( bind(this->server_s_udp,(struct sockaddr *)&this->server_inf , sizeof(this->server_inf)) < 0)
				printf("Bind failed");
		#endif
	}

	return 0;
}

void Server::commandDefaultRouting(){

	switch (this->commandHandling(const_cast<char*>(this->client.replyBuffer.c_str()))){

		case -1://Unknow command
			send( this->client.c_socket_tcp , "Unknow command, try again" , strlen("Unknow command, try again") , 0 );
			this->client.replyBuffer.clear();
			break;

		case 0://ECHO
		if(this->client.replyBuffer[strlen("ECHO")] == ' ')
			this->client.replyBuffer.erase(this->clients[i].replyBuffer.begin(), this->clients[i].replyBuffer.begin() + strlen("ECHO")+1);
		else this->clients[i].replyBuffer.clear();
		send( this->clients[i].c_socket , this->clients[i].replyBuffer.c_str() , strlen(this->clients[i].replyBuffer.c_str()) , 0 );
		this->clients[i].replyBuffer.clear();
		break;

		case 1://TIME
			send( this->client.c_socket_tcp , currentDateTime().c_str() , strlen(currentDateTime().c_str()) , 0 );
			this->client.replyBuffer.clear();
			break;
			
		//case 2://UPLOAD


		//case 3://DOWNLOAD


		//case 4://CLOSE

	}//switch
}