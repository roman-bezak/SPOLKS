#include "Client.h"




std::string currentDateTime() {
 time_t rawtime;
  struct tm * timeinfo;
  char buffer[80];

  time (&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(buffer,80,"Current server time: %d-%m-%Y %I:%M:%S",timeinfo);
  std::string str(buffer);

  return str;
}

Client::Client(char* ip_addres, int port){

	this->mode = "TCP";
	
	this->server_inf.sin_addr.s_addr = inet_addr(ip_addres);
    this->server_inf.sin_family = AF_INET;
    this->server_inf.sin_port = htons( port );
	
	#ifdef Windows
	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
		printf("Failed. Error Code : %d",WSAGetLastError());
	#endif

    printf("Initialised.\n");
    
	#ifdef Windows
	if((this->client_socket = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
        printf("Could not create socket : %d" , WSAGetLastError());

	if((this->udp_socket = socket(AF_INET , SOCK_DGRAM , 0 )) == INVALID_SOCKET)
        printf("Could not create socket : %d" , WSAGetLastError());
	#else
	if((this->client_socket = socket(AF_INET , SOCK_STREAM , 0 )) == -1)
        printf("Could not create socket");

	if((this->udp_socket = socket(AF_INET , SOCK_DGRAM , 0 )) == -1)
        printf("Could not create socket");

	#endif


	
	printf("Socket created.\n");
	
	int iTimeout = 100;
	setsockopt( this->client_socket,
                        SOL_SOCKET,
                        SO_RCVTIMEO,
                        (const char *)&iTimeout,
                        sizeof(iTimeout) );

	setsockopt( this->client_socket,
                        SOL_SOCKET,
						SO_SNDTIMEO,
                        (const char *)&iTimeout,
                        sizeof(iTimeout) );

	if (connect(this->client_socket , (struct sockaddr *)&this->server_inf , sizeof(this->server_inf)) < 0){
        puts("Connect error");
		is_connection = false;
	}else { 
		puts("Connected");
		is_connection = true;
	}

	reconnectionFlag = false;
	 
}

bool Client::reconnectToServer(){
	if((this->client_socket = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET){
        printf("Could not create socket : %d" , WSAGetLastError());
		return false;
	}

	int iTimeout = 100;
	int iRet = setsockopt( this->client_socket,
                        SOL_SOCKET,
                        SO_RCVTIMEO,
                        (const char *)&iTimeout,
                        sizeof(iTimeout) );
		setsockopt( this->client_socket,
                        SOL_SOCKET,
						SO_SNDTIMEO,
                        (const char *)&iTimeout,
                        sizeof(iTimeout) );

	if (connect(this->client_socket , (struct sockaddr *)&this->server_inf , sizeof(this->server_inf)) < 0){
        //puts("Connect error...");
		return false;
    }else {
		puts("Reconnected success");
		return true;
	}
}

int Client::getFileSize(FILE *file){

	long pos = ftell(file);
    fseek(file, 0L, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, pos, SEEK_SET);
    return fileSize;
}

FILE *safe_fopen(char const *path, char const *mode)
{
    FILE *f = std::fopen(path, mode);
    if (f == NULL)
        throw std::runtime_error(std::strerror(errno));
    return f;
}
int Client::reconnectTimer(int min_t){
	
	clock_t start = clock();
	clock_t end = 1;
	while( (end-start)/1000 < min_t ){
		if(reconnectToServer())return 1;
		else end = clock();
		Sleep(500);
	}

	return 0;
}
int Client::uploadFile(char *filename){

	FILE *file = NULL;

	try{
	   file = safe_fopen(filename,"r+b");
	}catch(...){
	   std::cout << "Open file ERROR.";
	   return 0;
	}

	const int buffer_size = 1024;
    char buffer[buffer_size];
	char buf[100];
	long long int file_size = getFileSize(file);
    long long int total_bytes_s = 0;
	int s_bytes = 0;
	int r_bytes = 0;
    size_t bytes_read = 0;
	clock_t start, end = 0;
	bool needClaculatedSpeed = true;

	sprintf(buffer, "UPLOAD %s %llu\r\n", filename, file_size);

	s_bytes = send(this->client_socket, buffer, sizeof(buffer), 0);

	if (s_bytes < 0) {
		fclose(file);
		return 1;
	
	}else{

		r_bytes = recv(this->client_socket,buf,sizeof(buf),0);
		if(r_bytes > 0){
			if(atoi(buf) > -1){
				total_bytes_s = atoi(buf);
				fseek(file,atoi(buf),0);

			}
		}else return 1;
	}
		
		puts("Start upload...");

		if(total_bytes_s)needClaculatedSpeed = false;
		start = clock();
		while (total_bytes_s < file_size){

			bytes_read = fread(buffer, 1, sizeof(buffer), file);
			s_bytes = send(this->client_socket, buffer, bytes_read, 0);
			
			if (s_bytes < 0) {
				fclose(file);
				return 1;
			}
			total_bytes_s += s_bytes;
			
		}
		end = clock();

		double ellapsed = double(end - start)/1000;
		double speed = 0;
		if(total_bytes_s)
			speed = (double)total_bytes_s/ellapsed;
		if(needClaculatedSpeed)
			printf("File (%s), was upload, total bytes send: %llu \nEllaspsed time(in seconds): %f \nSpeed mb/s: %f",filename,total_bytes_s, ellapsed, speed/1024/1024);
		else printf("File (%s), was upload, total bytes send: %llu",filename,total_bytes_s);

	fclose(file);
	return 0;
}


int Client::commandHandling(char* com){

	char *m[] = {
		"UPLOAD",
		"DOWNLOAD"
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
int Client::downloadFile(char* filename){
	
	FILE *file = NULL;
	try{
	   file = safe_fopen(filename,"a+b");
	}catch(...){
	   std::cout << "Open file ERROR.";
	   return 0;
	}

	const int buffer_size = 1024;
    char buffer[buffer_size];
	char buf[100];
    long long int total_bytes_reciv = 0;
	long long int file_size = 0;
	int s_bytes = 0;
	int r_bytes = 0;
    size_t bytes_read = 0;
	clock_t start, end = 0;
	bool needClaculatedSpeed = true;

	sprintf(buffer, "DOWNLOAD %s\r\n", filename);

	s_bytes = send(this->client_socket, buffer, sizeof(buffer), 0);
	if (s_bytes < 0) {
		fclose(file);
		return 1;

	}else{
		r_bytes = recv(this->client_socket,buf,sizeof(buf),0);
		file_size = atoi(buf);
		r_bytes = recv(this->client_socket,buf,sizeof(buf),0);
		if(r_bytes > 0){
			if(atoi(buf) == -1){
				total_bytes_reciv = 0;
			}else if(atoi(buf) == -2) {
				puts("File not found");
				fclose(file);
				remove(filename);
				return 0;
			}else {
				itoa(getFileSize(file),buf,10);
				total_bytes_reciv = getFileSize(file);
				send(this->client_socket,buf,sizeof(buf),0);
			}
		}else {
			fclose(file);
			return 1;
		}
	}

	puts("Start download...");

	if(total_bytes_reciv)needClaculatedSpeed = false;
	start = clock();

	while( (r_bytes = recv(this->client_socket,buffer,sizeof(buffer),0)) > 0){
		fwrite(buffer,1,r_bytes,file);
		total_bytes_reciv += r_bytes;
	}

	end = clock();
	if(getFileSize(file) != file_size)return 1;

	double ellapsed = double(end - start)/1000;
	double speed = 0;
	if(total_bytes_reciv)
		speed = (double)total_bytes_reciv/ellapsed;
	if(needClaculatedSpeed)
		printf("File (%s), was download, total bytes recive: %llu \nEllaspsed time(in seconds): %f \nSpeed mb/s: %f",filename,total_bytes_reciv, ellapsed, speed/1024/1024);
	else printf("File (%s), was download, total bytes recive: %llu",filename,total_bytes_reciv);

	fclose(file);
	return 0;
}
int Client::uploadUdpFile(char *filename){
	
	FILE *file = NULL;

	try{
	   file = safe_fopen(filename,"r+b");
	}catch(...){
	   std::cout << "Open file ERROR.";
	   return 0;
	}

	int slen = sizeof(this->server_inf);
	const int buffer_size = 1024;
    char buffer[buffer_size];
	char buf[100];
	long long int file_size = getFileSize(file);
    long long int total_bytes_s = 0;
	int s_bytes = 0;
	int r_bytes = 0;
    size_t bytes_read = 0;
	clock_t start, end = 0;
	bool needClaculatedSpeed = true;

	sprintf(buffer, "UPLOAD %s %llu\r\n", filename, file_size);

	s_bytes = sendto( this->udp_socket , buffer, sizeof(buffer), 0, (struct sockaddr *) &this->server_inf, slen );

	if (s_bytes < 0) {
		fclose(file);
		return 1;
	
	}else{

		r_bytes = recvfrom(this->udp_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&this->server_inf, &slen);
		if(r_bytes > 0){
			if(atoi(buf) > -1){
				total_bytes_s = atoi(buf);
				fseek(file,atoi(buf),0);

			}
		}else return 1;
	}
		
		puts("Start upload...");

		if(total_bytes_s)needClaculatedSpeed = false;
		start = clock();
		while (total_bytes_s < file_size){

			bytes_read = fread(buffer, 1, sizeof(buffer), file);
			s_bytes = sendto( this->udp_socket , buffer, bytes_read, 0, (struct sockaddr *) &this->server_inf, slen );
			if (s_bytes < 0) {
				fclose(file);
				return 1;
			}
			total_bytes_s += s_bytes;
			
		}
		end = clock();

		double ellapsed = double(end - start)/1000;
		double speed = 0;
		if(total_bytes_s)
			speed = (double)total_bytes_s/ellapsed;
		if(needClaculatedSpeed)
			printf("File (%s), was upload, total bytes send: %llu \nEllaspsed time(in seconds): %f \nSpeed mb/s: %f",filename,total_bytes_s, ellapsed, speed/1024/1024);
		else printf("File (%s), was upload, total bytes send: %llu",filename,total_bytes_s);

	fclose(file);
	return 0;
}
int Client::downloadUdpFile(char * filename){

		//sendto( this->udp_server_socket ,"Unknow command, try again", strlen("Unknow command, try again"), 0, (struct sockaddr *) &this->udp_sessions[i].inf, slen );
	//recv_Size = recvfrom(this->udp_server_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&this->udp_sessions[i].inf, &slen);

	FILE *file = NULL;
	try{
	   file = safe_fopen(filename,"a+b");
	}catch(...){
	   std::cout << "Open file ERROR.";
	   return 0;
	}

	const int buffer_size = 1024;
    char buffer[buffer_size];
	char buf[100];
    long long int total_bytes_reciv = 0;
	long long int file_size = 0;
	int s_bytes = 0;
	int r_bytes = 0;
    size_t bytes_read = 0;
	clock_t start, end = 0;
	bool needClaculatedSpeed = true;
	int slen = sizeof(this->udp_socket);

	sprintf(buffer, "DOWNLOAD %s\r\n", filename);
	s_bytes = sendto( this->udp_socket, buffer, sizeof(buffer), 0, (struct sockaddr *) &this->server_inf, slen );
	if (s_bytes < 0) {
		fclose(file);
		return 1;

	}else{
		r_bytes = recvfrom(this->udp_socket, buf, sizeof(buf), 0, (struct sockaddr *)&this->server_inf, &slen);
		file_size = atoi(buf);
		r_bytes = recvfrom(this->udp_socket, buf, sizeof(buf), 0, (struct sockaddr *)&this->server_inf, &slen);
		if(r_bytes > 0){
			if(atoi(buf) == -1){
				total_bytes_reciv = 0;
			}else if(atoi(buf) == -2) {
				puts("File not found");
				fclose(file);
				remove(filename);
				return 0;
			}else {
				itoa(getFileSize(file),buf,10);
				total_bytes_reciv = getFileSize(file);
				sendto( this->udp_socket, buf, sizeof(buf), 0, (struct sockaddr *) &this->server_inf, slen );
			}
		}else {
			fclose(file);
			return 1;
		}
	}

	puts("Start download...");

	if(total_bytes_reciv)needClaculatedSpeed = false;
	start = clock();

	while( (r_bytes =  recvfrom(this->udp_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&this->server_inf, &slen)) > 0){
		fwrite(buffer,1,r_bytes,file);
		total_bytes_reciv += r_bytes;
	}

	end = clock();
	if(getFileSize(file) != file_size)return 1;

	double ellapsed = double(end - start)/1000;
	double speed = 0;
	if(total_bytes_reciv)
		speed = (double)total_bytes_reciv/ellapsed;
	if(needClaculatedSpeed)
		printf("File (%s), was download, total bytes recive: %llu \nEllaspsed time(in seconds): %f \nSpeed mb/s: %f",filename,total_bytes_reciv, ellapsed, speed/1024/1024);
	else printf("File (%s), was download, total bytes recive: %llu",filename,total_bytes_reciv);

	fclose(file);
	return 0;

}
void Client::run(){
	

	std::string command = "";
	int bytesRec = 0;
	const int maxBuffSize = 1024;
	char replyBuff[maxBuffSize+1] = {};


	while (is_connection){


		while (this->reconnectionFlag){
			if(reconnectToServer()){
				this->reconnectionFlag = false;
				break;
			}
			Sleep(1000);
		}

		command.clear();
		printf("\n\n%s >>", mode.c_str());
		std::getline(std::cin,command);
		if(command == "MODE"){

			this->changeMode();
			continue;
		}
		command.append("\r\n");

		if(this->mode == "TCP")this->tcpCommandRoute(command);
		else this->udpCommandRoute(command);
	}


}


void Client::tcpCommandRoute(std::string command){

		switch (this->commandHandling(const_cast<char*>(command.c_str()))){

			case 0://UPLOAD
				if(command[strlen("UPLOAD")] == ' '){
					command.erase(command.begin(), command.begin() + strlen("UPLOAD")+1);
					command.erase(command.end() - 2, command.end());
					while(true){
						if(uploadFile(const_cast<char*>(command.c_str())) == 0)break;
						else{
							if(reconnectTimer(30))continue;
							else {
								puts("Problems with the connection during Uploading file. Reconnect to server...");
								reconnectionFlag = true;
								break;
							}
						}
					}
				}

			break;
			case 1://DOWNLOAD
				if(command[strlen("DOWNLOAD")] == ' '){
					command.erase(command.begin(), command.begin() + strlen("DOWNLOAD")+1);
					command.erase(command.end() - 2, command.end());
					while(true){
						if(downloadFile(const_cast<char*>(command.c_str())) == 0)break;
						else{
							if(reconnectTimer(30))continue;
							else {
								puts("Problems with the connection during Download file. Reconnect to server...");
								reconnectionFlag = true;
								break;
							}
						}
					}
				}
			break;

			default:
					{

						char buff[1024];
						int bytesRec = 0;
						if( send(this->client_socket, command.c_str(), strlen(command.c_str()), 0) != strlen(command.c_str()) ) 
							perror("send failed");
						//do{
							bytesRec = recv( this->client_socket , buff, 1024, 0);
							if(bytesRec > 0){
								buff[bytesRec] = '\0';
								printf("<<%s",buff);
							}
							if(bytesRec == 0){
								printf("Disconnect from server.");
								is_connection = false;
							}
						//}while(bytesRec > 0);
					}
		}
}

void Client::udpCommandRoute(std::string command){

		switch (this->commandHandling(const_cast<char*>(command.c_str()))){

			case 0://UPLOAD
				if(command[strlen("UPLOAD")] == ' '){
					command.erase(command.begin(), command.begin() + strlen("UPLOAD")+1);
					command.erase(command.end() - 2, command.end());
					while(true){
						if(uploadUdpFile(const_cast<char*>(command.c_str())) == 0)break;
						else{
							if(reconnectTimer(30))continue;
							else {
								puts("Problems with the connection during Uploading file. Reconnect to server...");
								reconnectionFlag = true;
								break;
							}
						}
					}
				}

			break;
			case 1://DOWNLOAD
				if(command[strlen("DOWNLOAD")] == ' '){
					command.erase(command.begin(), command.begin() + strlen("DOWNLOAD")+1);
					command.erase(command.end() - 2, command.end());

					while(true){
						if(downloadUdpFile(const_cast<char*>(command.c_str())) == 0)break;
						else{
							if(reconnectTimer(30))continue;
							else {
								puts("Problems with the connection during Download file. Reconnect to server...");
								reconnectionFlag = true;
								break;
							}
						}
					}
				}
			break;

			default:
					{
						char buff[1024];
						int bytesRec = 0;

						int slen = sizeof(this->server_inf);
						if( sendto(this->udp_socket, command.c_str(), strlen(command.c_str()), 0,(struct sockaddr *)&this->server_inf, slen ) != strlen(command.c_str()) ) 
							perror("send failed");

							bytesRec = recvfrom(this->udp_socket, buff, 1024, 0, (struct sockaddr *)&this->server_inf, &slen);

							if(bytesRec > 0){
								buff[bytesRec] = '\0';
								printf("<<%s",buff);
							}
							if(bytesRec == 0){
								printf("Disconnect from server.");
								is_connection = false;
							}

					}
		}
}


void Client::changeMode(){

	if(mode == "TCP")mode = "UDP";
	else mode = "TCP";
}

Client::~Client()
{
	#ifdef Windows
		closesocket(this->client_socket);
		closesocket(this->udp_socket);
		WSACleanup();
	#else
		close(this->client_socket);
		close(this->udp_socket);
	#endif

}

