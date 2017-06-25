// TestUDP.cpp : Defines the entry point for the console application.
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

#define bufLength 300

void main(int argc, char *argv[]) // CHANGE THIS FROM _TCHAR to char
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

	SOCKET UDPSock;
	UDPSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); /*UDP Ssocket*/

	int UDPPort;
	if (isServer)
	{
		UDPPort = 10000;
		printf("Running as server\n");
	}
	else
	{
		UDPPort = 10001;
		printf("Running as client\n");
	}

	struct sockaddr_in My;
	memset(&My, 0, sizeof(My)); // clear memory
	My.sin_family = AF_INET; //address family, must be this for ipv4, or AF_INET6 for ipv6
	My.sin_addr.s_addr = htonl(INADDR_ANY); // allows socket to work send and receive on any of the machines interfaces (which machine is used to send?)
	My.sin_port = htons(UDPPort);

	/*BIND THE SOCKET TO THE PORT*/

	int ret = bind(UDPSock, (struct sockaddr *)&My, sizeof(struct sockaddr)); // ask OS to let us use the socket (why might it say we cannot?)
	printf("bind returned %d if not zero, then there was a problem\n", ret);


	if (!isServer) {
		char ToIPAddress[80];
#ifdef _WINDOWS
		sprintf_s(ToIPAddress, 80, "127.0.0.1");  // loop back address (the pkt will come back to this host)
#else
		sprintf_s(ToIPAddress, 80, "127.0.0.1");  // loop back address (the pkt will come back to this host)
#endif
		int WriteToPort = 10000; // the client will write to 10000
		struct sockaddr_in to;
		memset(&to, 0, sizeof(to));
		to.sin_addr.s_addr = inet_addr(ToIPAddress);     // we specifed the address as a string, inet_addr translates to a number in the correct byte order
		to.sin_family = AF_INET;                         // ipv4
		to.sin_port = htons(WriteToPort);                // set port address (is this the sender's port or the receiver's port

														 /* make message */

		char pkt[bufLength];
#ifdef _WINDOWS
		sprintf_s(pkt, bufLength, "hello there\n");
#else
		sprintf(pkt, "hello there\n");
#endif

		/* send message */
		ret = sendto(UDPSock, (char *)pkt, bufLength, 0, (struct sockaddr *)&to, sizeof(struct sockaddr));
		printf("sendto returned %d it should be   the number of bytes sent \n", ret);

		/* wait for return message */
		struct sockaddr_in from;
		fd_set readFd;
		struct timeval SelectTime;
		SelectTime.tv_sec = 2; /* set timeout to 2 seconds. If no messages arrives in 2 seconds, then give up */
		SelectTime.tv_usec = 0;
		FD_ZERO(&readFd);
		FD_SET(UDPSock, &readFd);
		ret = select(255, &readFd, NULL, NULL, &SelectTime); /* wait for message */
		if (FD_ISSET(UDPSock, &readFd) == 1)
		{	/* is message arrived before timeout */
			int len = sizeof(struct sockaddr);
			ret = recvfrom(UDPSock, (char *)pkt, sizeof(pkt), NULL, (struct sockaddr *)&from, &len); // sizeof(pkt) is ok because pkt is an array
			if (ret >= 0)
			{
				unsigned int fp = ntohs(from.sin_port);
				printf("Received pkt with message from %s and port %d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
				printf("received message: %s\n\n", pkt);
			}
			else
			{
#ifdef _WINDOWS
				// for windows only
				if (WSAECONNRESET == WSAGetLastError())
					printf("possibly sent a packet to an unopened port\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
				else
					printf("revfrom error %d %d\n", ret, WSAGetLastError());
			}
#endif
		}
		else
		{	// if message failed to arrive before timeout 
			printf("no response from %s\n", ToIPAddress);
		}
	}
	else
	{
		//server part 
		char buf[bufLength];
		struct sockaddr_in from;
		int len = sizeof(struct sockaddr);


		/* wait for message. This will wait forever for a message. If a timeout is needed, then use Select, as shown above */
		ret = recvfrom(UDPSock, (char *)buf, bufLength, 0, (struct sockaddr *)&from, (socklen_t *)&len);
		printf("recv returned %d\n", ret);
		printf("received data from %s from port %d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
		printf("received message: %s\n\n", buf);
		/* make message to send */
#ifdef _WINDOWS
		sprintf_s(buf, bufLength, "go away\n");
#else
		sprintf(buf, "go away\n");
#endif
		printf("%s", buf);

		/* send message */
		struct sockaddr_in to;
		memset(&to, 0, sizeof(to));
		to.sin_addr.s_addr = from.sin_addr.s_addr;
		to.sin_family = AF_INET;
		to.sin_port = from.sin_port; // set port address
		ret = sendto(UDPSock, (char *)buf, bufLength, 0, (struct sockaddr *)&to, sizeof(struct sockaddr));
		printf("sendto returned %d\n", ret);

	}
	}
