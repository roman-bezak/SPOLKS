#include <stdio.h>
#include <iostream>
#include "Server.h"

int main(int argc , char *argv[])
{
	Server server(8888);
	server.start();

    return 0;
}