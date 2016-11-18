#include "Server.h"
#include <algorithm>
#include <string>
#include <Ws2tcpip.h>
#include <iostream>
#include <ctime>
#include <time.h>
#include <vector>
#include <sstream>


std::vector<std::string> split(const std::string &s, char delim) {
	std::stringstream ss(s);
	std::string item;
	std::vector<std::string> tokens;
	while (std::getline(ss, item, delim)) {
		tokens.push_back(item);
	}
	return tokens;
}

Server::Server(int port){

	this->port = port;

}

Server::~Server()
{
	closesocket(this->server_socket);
	closesocket(this->udp_server_socket);
	WSACleanup();
}

bool Server::start(){

	puts("Server start");
	int maxPackageSize = 1024;
	int maxConnectionsListening = 100;
	int addrLen = sizeof(struct sockaddr_in);
	int activity = -1;
	int bytesRecv = -1;
	fd_set readfds;

	char *reply_buffer;
	reply_buffer =  (char*) malloc((maxPackageSize + 1) * sizeof(char));

	this->serverSetUp();

	while(true){

		FD_ZERO(&readfds);
		FD_SET(this->server_socket, &readfds);
		FD_SET(this->udp_server_socket, &readfds);

		for ( int i = 0 ; i < this->clients.size() ; i++){ 
			if(this->clients[i].c_socket > 0)
				FD_SET( this->clients[i].c_socket, &readfds);
		}

		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;

		activity = select( 0 , &readfds , NULL , NULL , &timeout);

		if ( activity == SOCKET_ERROR ){
			printf("select call failed with error code : %d" , WSAGetLastError());
			exit(EXIT_FAILURE);
		}

		if (FD_ISSET(this->server_socket , &readfds)) {
			if ((this->new_clientSocket = accept(this->server_socket , (struct sockaddr *)&this->new_clientInf, (int *)&addrLen))<0)
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}else{

				bool is_Old_client = false;

				for (int j = 0; j < this->clients.size(); j++){
					if(!strcmp(inet_ntoa(this->new_clientInf.sin_addr),this->clients[j].getIpAddress())){
						if(this->clients[j].c_session.operation_status == "BAD_UPLOAD" || this->clients[j].c_session.operation_status == "BAD_DOWNLOAD"){
							this->clients[j].c_socket = this->new_clientSocket;
							this->clients[j].inf = this->new_clientInf;
							is_Old_client = true;
						}
						break;
					}

				}
				if(!is_Old_client){
					Client n_client(this->new_clientSocket, this->new_clientInf);
					this->clients.push_back(n_client);

				}
			}

			printf("New connection , ip is : %s , port : %d \n" , inet_ntoa(this->new_clientInf.sin_addr) , ntohs(this->new_clientInf.sin_port));

		}

		if (FD_ISSET(this->udp_server_socket , &readfds)){

			char buf[1024];
			int len = 0;
			int slen = sizeof(this->new_clientInf);
			len = recvfrom(this->udp_server_socket, buf, 1024, 0, (struct sockaddr *)&this->new_clientInf, &slen);
			buf[len] = '\0';
			
			//int id = -1;
			//if( (id = this->checkIsUdpSession(this->new_clientInf)) >= 0){
			//	
			//	puts("est");

			//	//if(this->udp_sessions[id].operation_status == "UPLOAD")reciveFileUdpProcessing();
			//	//	if(this->udp_sessions[id].operation_status == "DOWNLOAD")downloadFileUdpProcessing();
			//		

			//}
			printf("%d",udp_sessions.size());
			if(this->searchEscapeChars(buf,len,0,"UDP"))this->commandDefaultRouting(-1,buf,this->new_clientInf);
			
		}

		for (int i = 0; i < this->clients.size(); i++) {          


			if(this->clients[i].c_session.operation_status == "UPLOAD"){
				reciveFileProcessing(i);
				continue;
			}
			if(this->clients[i].c_session.operation_status == "DOWNLOAD"){
				downloadFileProcessing(i);
				continue;
			}

			if (FD_ISSET( this->clients[i].c_socket , &readfds)) { 

				bytesRecv = recv( this->clients[i].c_socket , reply_buffer, maxPackageSize, 0);

				if( bytesRecv == SOCKET_ERROR){
					int error_code = WSAGetLastError();
					if(error_code == WSAECONNRESET)
					{
						if(this->clients[i].c_session.operation_status == "BAD_UPLOAD" || this->clients[i].c_session.operation_status == "BAD_DOWNLOAD")continue;
						closesocket(this->clients[i].c_socket);
						this->clients.erase(this->clients.begin() + i);
						printf("Client with index %d, was be remove\n",i);
						continue;
					}
					else{
						printf("recv failed with error code : %d" , error_code);
					}
				}
				if ( bytesRecv == 0){

					if(this->clients[i].c_session.operation_status == "BAD_UPLOAD" || this->clients[i].c_session.operation_status == "BAD_DOWNLOAD")continue;
					closesocket(this->clients[i].c_socket);
					this->clients.erase(this->clients.begin() + i);
					printf("Client with index %d, was be remove\n",i);
				}

				else{

					if(this->searchEscapeChars(reply_buffer,bytesRecv,i,"TCP"))this->commandDefaultRouting(i,NULL,this->new_clientInf);

				}
			}
		}

	}//main while(true)

	return true;
}

int Server::checkIsUdpSession(sockaddr_in inf){

	for (int i = 0; i < this->udp_sessions.size(); i++){
		if( inet_ntoa(udp_sessions[i].inf.sin_addr) == inet_ntoa(inf.sin_addr) )return i;
	}

	return -1;
}
int getFileSize(char *filename){

	FILE *file = fopen(filename,"r");
	long pos = ftell(file);
	fseek(file, 0L, SEEK_END);
	long fileSize = ftell(file);
	fseek(file, pos, SEEK_SET);
	return fileSize;
}
int FileExists(const char *filename)
{
	FILE *fp = fopen (filename, "r");
	if (fp!=NULL) fclose (fp);
	return (fp!=NULL);
}
void Server::downloadFileProcessing(int i){

	const int bufferSize = 1500;
	char buffer[bufferSize];
	int s_size = 0;
	int b_read = 0;
	int recv_Size = -1;

	if(this->clients[i].c_session.d_file == NULL)//если файл не открыт, открываем
		this->clients[i].c_session.d_file = fopen(this->clients[i].c_session.filename.c_str(),"r+b");

	if(this->clients[i].c_session.sendigSize < this->clients[i].c_session.fileSize){
		b_read = fread(buffer,1,sizeof(buffer),this->clients[i].c_session.d_file);
		s_size = send(this->clients[i].c_socket, buffer, b_read, 0);
		if (s_size < 0) {
			this->clients[i].c_session.operation_status = "BAD_DOWNLOAD";
			puts("Bad connection...");
			return;
		}else this->clients[i].c_session.sendigSize += s_size;		
	}else {

		this->clients[i].c_session.clear();
		this->clients[i].replyBuffer.clear();

	}
}
void Server::reciveFileProcessing(int i){

	const int bufferSize = 1500;
	char buffer[bufferSize];
	int recv_Size = -1;


	if(this->clients[i].c_session.d_file == NULL)//если файл не открыт, значит он и не создан, создаём
		this->clients[i].c_session.d_file = fopen(this->clients[i].c_session.filename.c_str(),"w+b");

	recv_Size = recv(this->clients[i].c_socket, buffer, sizeof(buffer), 0);
	if(recv_Size > 0){
		fwrite(buffer,1,recv_Size,this->clients[i].c_session.d_file);
		this->clients[i].c_session.reciveSize += recv_Size;
	}else {
		this->clients[i].c_session.operation_status = "BAD_UPLOAD";
		puts("Bad connection...");
		return;
	}

	if(this->clients[i].c_session.reciveSize == this->clients[i].c_session.fileSize){
		//puts("stop reciv");
		this->clients[i].c_session.clear();
		this->clients[i].replyBuffer.clear();

	}


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
void Server::downloadInit(int i){

	char buffer[100];

	if(this->clients[i].replyBuffer[strlen("DOWNLOAD")] == ' '){
		this->clients[i].replyBuffer.erase(this->clients[i].replyBuffer.begin(), this->clients[i].replyBuffer.begin() + strlen("DOWNLOAD")+1);
		std::vector<std::string> a = split(this->clients[i].replyBuffer,' ');

		if(a.size() != 1)puts("error");
		else a[0].erase(a[0].end() - 2, a[0].end());

		if(this->clients[i].c_session.operation_status == "BAD_DOWNLOAD" && this->clients[i].c_session.filename == a[0]){
			itoa(getFileSize(const_cast<char*>(this->clients[i].c_session.filename.c_str())),buffer,10);
			send(this->clients[i].c_socket,buffer,sizeof(buffer),0);
			itoa(1,buffer,10);
			send(this->clients[i].c_socket,buffer,sizeof(buffer),0);
			int r_b = recv(this->clients[i].c_socket,buffer,sizeof(buffer),0);
			this->clients[i].c_session.sendigSize = atoi(buffer);
			fseek(this->clients[i].c_session.d_file, this->clients[i].c_session.sendigSize, 0);
			this->clients[i].c_session.operation_status = "DOWNLOAD";
		}else {

			if(FileExists(a[0].c_str())){
				itoa(getFileSize(const_cast<char*>(a[0].c_str())),buffer,10);
				send(this->clients[i].c_socket,buffer,sizeof(buffer),0);
				itoa(-1,buffer,10);
				send(this->clients[i].c_socket,buffer,sizeof(buffer),0);
				this->clients[i].c_session.setSessionData(a[0],"DOWNLOAD",getFileSize(const_cast<char*>(a[0].c_str())),0,-1);

			}else {
				itoa(0,buffer,10);
				send(this->clients[i].c_socket,buffer,sizeof(buffer),0);
				itoa(-2,buffer,10);
				send(this->clients[i].c_socket,buffer,sizeof(buffer),0);
			}
			this->clients[i].replyBuffer.clear();
		}

	}else {
		send( this->clients[i].c_socket , "Unknow command, try again" , strlen("Unknow command, try again") , 0 );
		this->clients[i].replyBuffer.clear();

	}

}
void Server::uploadInit(int i){

	char buffer[100];
	std::cout<< this->clients[i].replyBuffer;
	if(this->clients[i].replyBuffer[strlen("UPLOAD")] == ' '){

		this->clients[i].replyBuffer.erase(this->clients[i].replyBuffer.begin(), this->clients[i].replyBuffer.begin() + strlen("UPLOAD")+1);
		std::vector<std::string> a = split(this->clients[i].replyBuffer,' ');
		if(a.size() != 2)puts("error");
		else a[1].erase(a[1].end() - 2, a[1].end());

		if(this->clients[i].c_session.operation_status == "BAD_UPLOAD" && this->clients[i].c_session.filename == a[0]){

			itoa(this->clients[i].c_session.reciveSize,buffer,10);
			send(this->clients[i].c_socket,buffer,sizeof(buffer),0);
			this->clients[i].c_session.operation_status = "UPLOAD";

		}else {

			itoa(-1,buffer,10);
			send(this->clients[i].c_socket,buffer,sizeof(buffer),0);
			this->clients[i].c_session.clear();
			this->clients[i].c_session.setSessionData(a[0],"UPLOAD",atoi(a[1].c_str()),-1,0);
			this->clients[i].replyBuffer.clear();

			//while wiil be
		}

	}else {

		send( this->clients[i].c_socket , "Unknow command, try again" , strlen("Unknow command, try again") , 0 );
		this->clients[i].replyBuffer.clear();

	}

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
void Server::commandDefaultRouting(int i, char *buffer, sockaddr_in addr){

	int slen=sizeof(addr);

	if(i >= 0 ){

		switch (this->commandHandling(const_cast<char*>(this->clients[i].replyBuffer.c_str())))
		{
		case -1://Unknow command
			send( this->clients[i].c_socket , "Unknow command, try again" , strlen("Unknow command, try again") , 0 );
			this->clients[i].replyBuffer.clear();
			break;

		case 0://ECHO
			if(this->clients[i].replyBuffer[strlen("ECHO")] == ' ')
				this->clients[i].replyBuffer.erase(this->clients[i].replyBuffer.begin(), this->clients[i].replyBuffer.begin() + strlen("ECHO")+1);
			else this->clients[i].replyBuffer.clear();
			send( this->clients[i].c_socket , this->clients[i].replyBuffer.c_str() , strlen(this->clients[i].replyBuffer.c_str()) , 0 );
			this->clients[i].replyBuffer.clear();
			break;

		case 1://TIME
			send( this->clients[i].c_socket , currentDateTime().c_str() , strlen(currentDateTime().c_str()) , 0 );
			this->clients[i].replyBuffer.clear();
			break;

		case 2://UPLOAD
			uploadInit(i);
			break;

		case 3://DOWNLOAD
			downloadInit(i);
			break;

		case 4://CLOSE
			closesocket(this->clients[i].c_socket);
			this->clients.erase(this->clients.begin() + i);
			break;

		}//switch
	
	}else {
	

		switch (this->commandHandling(buffer))
		{
			case -1://Unknow command
				sendto( this->udp_server_socket , "Unknow command, try again" , strlen("Unknow command, try again"), 0, (struct sockaddr *) &addr, slen );
				break;

			case 0://ECHO
				{
					std::string temp(buffer);				
					if(temp[strlen("ECHO")] == ' ')
						temp.erase(temp.begin(), temp.begin() + strlen("ECHO")+1);
					else temp.clear();
					sendto( this->udp_server_socket , temp.c_str(), strlen(temp.c_str()), 0, (struct sockaddr *) &addr, slen );
				}
				break;

			case 1://TIME
				sendto( this->udp_server_socket , currentDateTime().c_str() , strlen(currentDateTime().c_str()) , 0, (struct sockaddr *) &addr, slen );
				break;

			case 2://UPLOAD
				{
					puts("ppush Up");
					int id = -1;
					bool isOld = false;

					if( (id = this->checkIsUdpSession(this->new_clientInf)) >= 0 ){
						if(udp_sessions[id].operation_status == "BAD_UPLOAD"){
							this->udp_sessions[id].inf = this->new_clientInf;
							isOld = true;
							puts("old uplo");
						}
					}

					if(!isOld){
						UdpSession temp(this->new_clientInf);
						temp.operation_status = "UPLOAD";
						this->udp_sessions.push_back(temp);
						this->uploadUdpInit(this->udp_sessions.size()-1,buffer);
					}else uploadUdpInit(id,buffer);

					
				}

				break;

			//case 3://DOWNLOAD
				/*{
					puts("ppush DOw");
					int id = -1;
					bool isOld = false;

					if( (id = this->checkIsUdpSession(this->new_clientInf)) >= 0 ){
						if(udp_sessions[id].operation_status == "BAD_DOWNLOAD"){
							this->udp_sessions[id].inf = this->new_clientInf;
							isOld = true;
							puts("old down");
						}
					}

					if(!isOld){
						UdpSession temp(this->new_clientInf);
						temp.operation_status = "DOWNLOAD";
						this->udp_sessions.push_back(temp);
						this->downloadUdpInit(this->udp_sessions.size()-1);
					}else downloadUdpInit(id);

				}
				
				break;*/

			}//switch
	
	}

}


void Server::uploadUdpInit(int i,char *command){

	char buffer[100];
	int slen=sizeof(this->udp_sessions[i].inf);

	std::string replyBuffer(command);

	if(replyBuffer[strlen("UPLOAD")] == ' '){

		replyBuffer.erase(replyBuffer.begin(), replyBuffer.begin() + strlen("UPLOAD")+1);
		std::vector<std::string> a = split(replyBuffer,' ');
		if(a.size() != 2)puts("error");
		else a[1].erase(a[1].end() - 2, a[1].end());

		if(udp_sessions[i].operation_status == "BAD_UPLOAD" && udp_sessions[i].filename == a[0]){

			itoa(udp_sessions[i].reciveSize,buffer,10);
			sendto( this->udp_server_socket ,buffer, sizeof(buffer), 0, (struct sockaddr *) &this->udp_sessions[i].inf, slen );
			udp_sessions[i].operation_status = "UPLOAD";

		}else {

			itoa(-1,buffer,10);
			sendto( this->udp_server_socket ,buffer, sizeof(buffer), 0, (struct sockaddr *) &this->udp_sessions[i].inf, slen );
			udp_sessions[i].clear();
			udp_sessions[i].setSessionData(a[0],"UPLOAD",atoi(a[1].c_str()),-1,0);
		}

	}else {

		sendto( this->udp_server_socket ,buffer, sizeof(buffer), 0, (struct sockaddr *) &this->udp_sessions[i].inf, slen );

	}
}

void Server::reciveFileUdpProcessing(int i){

}

bool Server::searchEscapeChars(char *command_buf, int _bytes_recv, int client_id, std::string mode){
	
	bool issetEndOfCommand = false;
	command_buf[_bytes_recv] = '\0';
	std::string temp(command_buf);
	std::string sh_one("\r\n");
	std::string sh_two("\n");
	
	if(mode == "TCP"){

		std::size_t found = temp.find(sh_one);
		if (found!=std::string::npos){
			temp[found] = '\0';
			this->clients[client_id].replyBuffer.append(temp);
			issetEndOfCommand = true;
		}else{

			found = temp.find(sh_two);
			if (found!=std::string::npos){
				temp[found] = '\0';
				this->clients[client_id].replyBuffer.append(temp);
				issetEndOfCommand = true;
			}else {
				issetEndOfCommand = false;
				this->clients[client_id].replyBuffer.append(temp);
			}				
		}

		return issetEndOfCommand;

	}else {
		
		std::size_t found = temp.find(sh_one);
		if (found!=std::string::npos){
			command_buf[found] = '\0';
			issetEndOfCommand = true;
		}else{

			found = temp.find(sh_two);
			if (found!=std::string::npos){
				command_buf[found] = '\0';
				issetEndOfCommand = true;
			}else {
				issetEndOfCommand = false;
			}				
		}

		return issetEndOfCommand;

	}
	
}
void Server::serverSetUp(){

	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0){
		printf("Failed. Error Code : %d",WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	printf("Initialised\n");

	if((this->server_socket = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET){
		printf("Could not create socket : %d" , WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	if((this->udp_server_socket = socket(AF_INET , SOCK_DGRAM , 0 )) == INVALID_SOCKET){
		printf("Could not create socket : %d" , WSAGetLastError());
		exit(EXIT_FAILURE);
	}


	printf("Socket created\n");

	this->serverInf.sin_family = AF_INET;
	this->serverInf.sin_addr.s_addr = INADDR_ANY;
	this->serverInf.sin_port = htons( this->port );

	if( bind(this->server_socket ,(struct sockaddr *)&this->serverInf , sizeof(this->serverInf)) == SOCKET_ERROR){
		printf("Bind failed with error code : %d" , WSAGetLastError());
		exit(EXIT_FAILURE);
	}

    //Bind
	if( bind(this->udp_server_socket ,(struct sockaddr *)&this->serverInf , sizeof(this->serverInf)) == SOCKET_ERROR)
    {
        printf("!Bind failed with error code : %d" , WSAGetLastError());
        exit(EXIT_FAILURE);
    }

	puts("Bind done");
	listen(this->server_socket , 10);
	puts("Waiting for incoming connections...");
	puts("===================================");

}