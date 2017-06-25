# Peer to Peer Neighborhood

## Description

Project developed in the Computer Networks course (CISC450) / Fall 2015 / University of Delaware

In this project, peer-to-peer neighborhoods are made and maintained. Each host maintains list of neighbors and sends hello packets to these neighbors every 10 seconds. If a host is on the neighbor list, and no hello packet is received from the host for 40 seconds, then this host is removed from the neighbor list. If a node does not have enough neighbors, then it selects an address (e.g., IP and port) at random and tries to become its neighbor. Hosts can be in one out of two lists: bidirectionalNeighbors list and unidirectionalNeighbors list. Also temporaryNeighbor may hold a host ID.

Also, the routing topology is computed using the Distance Vector algorithm. It is assumed that each link has an administrative cost of one. Thus, the routing will compute the shortest path in terms of hops. For each destination, the routing table has three entries: the destination HostId, the next hop HostId and the cost.

## How to run

Initialization code requires the port and a list of hosts to be set at the command line, e.g.:

```neighbors.exe 10000 HostList.txt```

where an example of listOfHosts.txt is already among the project's files. Note that * means this host. So this files specifies that the application can probe this host on ports 10000-10005. If a host with IP 128.4.40.10 with port 10000 is also available to probe, then include the line 128.4.40.10 10000.

The .bat file is a script that will open a bunch of MS Windows and run the program.

This project requires the Boost library: http://www.boost.org/users/download/index.html

The main file of the project is neighbors.cpp
