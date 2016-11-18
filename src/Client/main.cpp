#include<stdio.h>
#include<winsock2.h>
#include <iostream>
#include "ClientWindows.h"
#pragma comment(lib, "ws2_32.lib")
 
int main(int argc , char *argv[])
{
	Client c("192.168.43.217",8888);
	c.run();

    return 0;
}