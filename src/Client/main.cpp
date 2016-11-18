#include<stdio.h>
#include<winsock2.h>
#include <iostream>
#include "ClientWindows.h"
#pragma comment(lib, "ws2_32.lib")
 
int main(int argc , char *argv[])
{
	Client c("127.0.0.1",8888);
	c.run();

    return 0;
}