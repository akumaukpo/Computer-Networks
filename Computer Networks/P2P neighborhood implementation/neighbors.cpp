#include "stdafx.h"
#include <boost/foreach.hpp>
#include <boost/cstdint.hpp>
#ifndef foreach
#define foreach         BOOST_FOREACH
#endif

#include <iostream>
#include "time.h"
#include <list>
using namespace std;

#include "target.h"
#ifdef _WINDOWS
#ifndef WIN32_LEAN_AND_MEAN   
#define WIN32_LEAN_AND_MEAN   
#endif   
//#include <sal.h>
#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include "process.h"
#ifndef socket_t 
#define socklen_t int
#endif   
#pragma comment(lib, "iphlpapi.lib")  
#pragma comment(lib, "ws2_32.lib")
// #pragma comment(lib, "ws2.lib")   // for windows mobile 6.5 and earlier 
#endif
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>   
#include <time.h>   
#include <sys/types.h>
#include <sys/timeb.h>
#include <math.h>

#include "helperClasses.h"
#include "someFunctions.h"

#define TARGET_NUMBER_OF_NEIGHBORS 2
#define TIME_CONN_LOST 40

bool isHostInList(HostId &hostId, list<NeighborInfo> &neighbors);
void addHostToList(HostId &hostId, list<NeighborInfo> &neighbors);
void removeHostFromList(HostId &hostId, list<NeighborInfo> &neighbors);
void updateLastReceivedHelloTime(HostId &hostId, list<NeighborInfo> &neighbors);
void updateRoutingTable(list<NeighborInfo> &bidirectionalNeighbors, HostId &hostId, RoutingTable &newRoutingTable);
bool isInRoutingTable(RoutingTable &routingTable, HostId &destination);
int getDistanceTo(RoutingTable &routingTable, HostId &destination);
void setDistanceTo(RoutingTable &routingTable, HostId &destination, HostId &newNextHop, int newCost);

void receiveHello(
	Packet &pkt,
	bool &searchingForNeighborFlag,
	list<NeighborInfo> &bidirectionalNeighbors,
	list<NeighborInfo> &unidirectionalNeighbors,
	NeighborInfo &tempNeighbor,
	HostId &thisHost
) {
	HelloMessage helloMessage;
	helloMessage.getFromPacket(pkt);
	HostId sender = helloMessage.source;
	RoutingTable routingTable = helloMessage.routingTable;

	if (sender == tempNeighbor.hostId) {
		searchingForNeighborFlag = false;
	}

	// check if this host is in the list of recently-heard-neighbors
	bool isIn = false;
	foreach (HostId &hostId, helloMessage.neighbors) {
		if (thisHost == hostId) {
			isIn = true;
			break;
		}
	}

	if (isIn) {
		// if this host IS in the list of recently-heard-neighbors
		if (isHostInList(sender, unidirectionalNeighbors)) {
			// remove the sender if it is in the unidirectional list
			removeHostFromList(sender, unidirectionalNeighbors);
		}
		// add the sender to bidirectional list (if it is not already there)
		if (!isHostInList(sender, bidirectionalNeighbors)) {
			addHostToList(sender, bidirectionalNeighbors);
		}
		// update neighbor set
		updateLastReceivedHelloTime(sender, bidirectionalNeighbors);
		// update routing table
		updateRoutingTable(bidirectionalNeighbors, sender, routingTable);
	}
	else {
		// if this host IS NOT in the list of recently-heard-neighbors
		// add the sender to unidirectional list (if it is not there)
		if (!isHostInList(sender, unidirectionalNeighbors)) {
			addHostToList(sender, unidirectionalNeighbors);
		}
		// update neighbor set
		updateLastReceivedHelloTime(sender, unidirectionalNeighbors);
	}
}

void removeOldNeighbors(
	list<NeighborInfo> &bidirectionalNeighbors,
	list<NeighborInfo> &unidirectionalNeighbors,
	bool &searchingForNeighborFlag,
	NeighborInfo &tempNeighbor
) {
	time_t currentTime;
	time(&currentTime);
	list<NeighborInfo>::iterator it;
	for (it = bidirectionalNeighbors.begin(); it != bidirectionalNeighbors.end();) {
		if (difftime(currentTime, it->timeWhenLastHelloArrived) > TIME_CONN_LOST) {
			it = bidirectionalNeighbors.erase(it);
		}
		else {
			++it;
		}
	}
	for (it = unidirectionalNeighbors.begin(); it != unidirectionalNeighbors.end();) {
		if (difftime(currentTime, it->timeWhenLastHelloArrived) > TIME_CONN_LOST) {
			it = unidirectionalNeighbors.erase(it);
		}
		else {
			++it;
		}
	}
	if (searchingForNeighborFlag) {
		if (difftime(currentTime, tempNeighbor.timeWhenLastHelloArrived) > TIME_CONN_LOST) {
			searchingForNeighborFlag = false;
		}
	}
}

void makeRoutingTable(
	RoutingTable &routingTable,
	list<NeighborInfo> &bidirectionalNeighbors,
	HostId &thisHost
) {
	// Clear table
	routingTable.clear();
	// Add thisHost with cost 0
	routingTable.addEntry(thisHost, thisHost, 0);
	// for each bidirectional neighbor
	foreach (NeighborInfo &neighborInfo, bidirectionalNeighbors) {
		// for each entry in its routing table
		foreach (RoutingTableEntry &entry, neighborInfo.routingTable.table) {
			// if destination is this host, move on
			if (entry.destination == thisHost) {
				continue;
			}
			// if destination IS in my routing table
			if (isInRoutingTable(routingTable, entry.destination)) {
				// update if cost is lower than the current one
				if (getDistanceTo(routingTable, entry.destination) > entry.cost + 1) {
					setDistanceTo(routingTable, entry.destination, neighborInfo.hostId, entry.cost + 1);
				}
			}
			// if destination IS NOT in my routing table
			else {
				// add entry
				routingTable.addEntry(entry.destination, neighborInfo.hostId, entry.cost + 1);
			}
		}
	}
}

void sendHelloToAllNeighbors(
	bool searchingForNeighborFlag,
	list<NeighborInfo> &bidirectionalNeighbors,
	list<NeighborInfo> &unidirectionalNeighbors,
	NeighborInfo &tempNeighbor,
	HostId &thisHost,
	UdpSocket &udpSocket,
	RoutingTable &routingTable
) {
	HelloMessage helloMessage;
	helloMessage.type = HELLO_MESSAGE_TYPE;
	helloMessage.source = thisHost;
	helloMessage.routingTable = routingTable;
	foreach (NeighborInfo &neighborInfo, bidirectionalNeighbors) {
		helloMessage.addToNeighborsList(neighborInfo.hostId);
	}
	foreach (NeighborInfo &neighborInfo, unidirectionalNeighbors) {
		helloMessage.addToNeighborsList(neighborInfo.hostId);
	}
	Packet pkt;
	pkt.getPacketReadyForWriting();
	helloMessage.addToPacket(pkt);
	foreach (NeighborInfo &neighborInfo, bidirectionalNeighbors) {
		udpSocket.sendTo(pkt, neighborInfo.hostId);
	}
	foreach (NeighborInfo &neighborInfo, unidirectionalNeighbors) {
		udpSocket.sendTo(pkt, neighborInfo.hostId);
	}
	if (searchingForNeighborFlag) {
		udpSocket.sendTo(pkt, tempNeighbor.hostId);
	}
}

bool isHostInList(HostId &hostId, list<NeighborInfo> &neighbors) {
	foreach (NeighborInfo &neighborInfo, neighbors) {
		if (neighborInfo.hostId == hostId) {
			return true;
		}
	}
	return false;
}

void addHostToList(HostId &hostId, list<NeighborInfo> &neighbors) {
	NeighborInfo newNeighbor;
	newNeighbor.hostId = hostId;
	neighbors.push_back(newNeighbor);
}

void removeHostFromList(HostId &hostId, list<NeighborInfo> &neighbors) {
	NeighborInfo neighborToRemove(hostId);
	neighbors.remove(neighborToRemove);
}

void updateLastReceivedHelloTime(HostId &hostId, list<NeighborInfo> &neighbors) {
	foreach (NeighborInfo &neighborInfo, neighbors) {
		if (neighborInfo.hostId == hostId) {
			neighborInfo.updateTimeToCurrentTime();
			break;
		}
	}
}

void updateRoutingTable(
	list<NeighborInfo> &bidirectionalNeighbors,
	HostId &hostId,
	RoutingTable &newRoutingTable
) {
	foreach (NeighborInfo &neighborInfo, bidirectionalNeighbors) {
		if (neighborInfo.hostId == hostId) {
			neighborInfo.routingTable = newRoutingTable;
			break;
		}
	}
}

bool isInRoutingTable(
	RoutingTable &routingTable,
	HostId &destination
) {
	foreach (RoutingTableEntry &entry, routingTable.table) {
		if (entry.destination == destination) {
			return true;
		}
	}
	return false;
}

int getDistanceTo(
	RoutingTable &routingTable,
	HostId &destination
) {
	foreach (RoutingTableEntry &entry, routingTable.table) {
		if (entry.destination == destination) {
			return entry.cost;
		}
	}
	return false;
}

void setDistanceTo(
	RoutingTable &routingTable,
	HostId &destination,
	HostId &newNextHop,
	int newCost
) {
	foreach (RoutingTableEntry &entry, routingTable.table) {
		if (entry.destination == destination) {
			entry.nextHop = newNextHop;
			entry.cost = newCost;
			break;
		}
	}
}

int main(int argc, char* argv[]) {
	#ifdef _WINDOWS
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 1), &wsaData) != 0) {
			printf("WSAStartup failed: %d\n", GetLastError());
			return(1);
		}
	#endif

	bool searchingForNeighborFlag = false;
	list<NeighborInfo> bidirectionalNeighbors;
	list<NeighborInfo> unidirectionalNeighbors;
	list<HostId> allHosts;
	NeighborInfo tempNeighbor;
	HostId thisHost;
	RoutingTable routingTable;

	if (argc != 3) {
		printf("usage: MaintainNeighbors PortNumberOfThisHost FullPathAndFileNameOfListOfAllHosts\n");
		return 0;
	}

	#ifdef _WINDOWS
		srand(_getpid());
	#else
		srand(getpid());
	#endif

	fillThisHostIP(thisHost);
	thisHost._port = atoi(argv[1]);
	readAllHostsList(argv[2], allHosts, thisHost);

	printf("Running on ip %s and port %d\n", thisHost._address, thisHost._port);
	printf("press any  key to begin\n");
	char c = getchar();

	time_t lastTimeHellosWereSent;
	time(&lastTimeHellosWereSent);
	time_t currentTime;
	time_t lastTimeStatusWasShown;
	time(&lastTimeStatusWasShown);

	UdpSocket udpSocket;
	if (udpSocket.bindToPort(thisHost._port) != 0) {
		cout << "bind failed\n";
		exit(-1);
	}

	Packet pkt;
	while (1)
	{
		// Receive hello
		if (udpSocket.checkForNewPacket(pkt, 2) > 0) {
			receiveHello(
				pkt,
				searchingForNeighborFlag,
				bidirectionalNeighbors,
				unidirectionalNeighbors,
				tempNeighbor,
				thisHost
			);
		}

		// Search for more neighbors
		if (
			bidirectionalNeighbors.size() + unidirectionalNeighbors.size() < TARGET_NUMBER_OF_NEIGHBORS
			&& !searchingForNeighborFlag
		) {
			searchingForNeighborFlag = true;
			tempNeighbor = selectNeighbor(bidirectionalNeighbors, unidirectionalNeighbors, allHosts, thisHost);
		}

		// Remove neighbors that timed out
		removeOldNeighbors(
			bidirectionalNeighbors,
			unidirectionalNeighbors,
			searchingForNeighborFlag,
			tempNeighbor
		);

		// Make routing table
		makeRoutingTable(
			routingTable,
			bidirectionalNeighbors,
			thisHost
		);

		// Send hello messages to neighbors
		time(&currentTime);
		if (difftime(currentTime, lastTimeHellosWereSent) >= 10) {
			lastTimeHellosWereSent = currentTime;
			sendHelloToAllNeighbors(
				searchingForNeighborFlag,
				bidirectionalNeighbors,
				unidirectionalNeighbors,
				tempNeighbor,
				thisHost,
				udpSocket,
				routingTable
			);
		}

		// Every 10 seconds, print out the status
		time(&currentTime);
		if (difftime(currentTime, lastTimeStatusWasShown) >= 10) {
			time(&lastTimeStatusWasShown);
			showStatus(
				searchingForNeighborFlag,
				bidirectionalNeighbors,
				unidirectionalNeighbors,
				tempNeighbor,
				thisHost,
				TARGET_NUMBER_OF_NEIGHBORS
			);
		}
	}
}