// UDPtest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINDOWS // comment this line out for linux
#ifdef _WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#ifndef socket_t 
#define socklen_t int
#endif   
#pragma comment(lib, "iphlpapi.lib")  
#pragma comment(lib, "ws2_32.lib")
// #pragma comment(lib, "ws2.lib")   // for windows mobile 6.5 and earlier 
#else
#include <sys/time.h>
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <sys/select.h> /* this might not be needed*/
#ifndef SOCKET 
#define  SOCKET int
#endif   
#endif    
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>   
#include <time.h>   
//#include <sys/select.h> /* this might not be needed*/ 


void main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("usage: udpTest 1 for server and udpTest 0 for client\n");
		exit(0);
	}
	int	serverOrClient = atoi(argv[1]);
	bool isServer;
	if (serverOrClient == 1)
	{
		printf("running as server\n");
		isServer = true;
	}
	else
	{
		printf("running as client\n");
		isServer = false;
	}

#ifdef _WINDOWS
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed: %d\n", iResult);
		exit(-1);
	}
#endif

	SOCKET TCPSock;
	TCPSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	int TCPServerPort = 10000;  // our server will listen on this port

								//This will make the socket receive data from a particular port. Also, when data is sent with this socket, the source port of the data will be this port.

	struct sockaddr_in my_addr;    // my address information   
	my_addr.sin_family = AF_INET;         // host byte order
	my_addr.sin_port = htons(TCPServerPort);     // short, network byte order
	my_addr.sin_addr.s_addr = INADDR_ANY; // auto-fill with my IP
	memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct
										  //Now we bind the socket to this port as follows

	int ret = bind(TCPSock, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
	printf("bind returned %d if not zero, then there was a problem\n", ret); // maybe we should do something to recover or exit

	listen(TCPSock, 3);

//	int TCPServerPort = 10000;  // our server will listen on this port
	char ServerIPAddress[] = "127.0.0.1";  //put the server IP here

	struct sockaddr_in DestAddr;
	memset(&DestAddr, 0, sizeof(DestAddr));					// clear struct
	DestAddr.sin_family = AF_INET;					// must be this
	DestAddr.sin_port = htons(TCPServerPort);	// set the port to write to
	DestAddr.sin_addr.s_addr = inet_addr(ServerIPAddress);  // set destination IP address
	memset(&(DestAddr.sin_zero), '\0', 8);  // zero the rest of the struct        

	// int ret = connect(TCPSock, (struct sockaddr *)&DestAddr, sizeof(DestAddr));
	printf("connect returned %d\n", ret);

	SOCKET sockRECTCP;  // this is the sock we use to send and receive data with 
	sockRECTCP = accept(TCPSock, 0, 0);
	printf("accepted -  connected\n");
	struct sockaddr_in  name;
	socklen_t sl = sizeof(name);
	ret = getpeername(sockRECTCP, (struct sockaddr *)&name, &sl);
	printf("connected to IP:%s on port:%d\n", inet_ntoa(name.sin_addr), ntohs(name.sin_port));

	char buf[300];
	//sprintf(buf, "hello there\n");
	ret = send(TCPSock, buf, 200, 0);
	printf("send returned %d\n", ret);
	ret = recv(TCPSock, buf, 200, 0);
	printf("recv: %s", buf);
	printf("write returned %d\n", ret);
	shutdown(TCPSock, 2);  // all done with the connection, shut it down
	printf("done\n");

//	char buf[200];
	ret = recv(sockRECTCP, buf, 200, 0);
	printf("recv = %s\n", buf);
//	sprintf(buf, "go away\n");
	ret = send(sockRECTCP, buf, 200, 0);
	printf("send returned %d\n", ret);
	shutdown(sockRECTCP, 2);
	printf("done\n");

}

