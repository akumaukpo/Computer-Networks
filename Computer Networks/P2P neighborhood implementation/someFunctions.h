HostId selectNeighbor(list<NeighborInfo> &bidirectionalNeighbors,
	list< NeighborInfo> &unidirectionalNeighbors,
	list<HostId> &allHosts,
	HostId &thisHost);

void fillThisHostIP(HostId &thisHost);

void readAllHostsList(char *fn, list<HostId> &allHosts, HostId &thisHost);
void removeFromList(NeighborInfo &neighborToRemove, list<NeighborInfo> &listToCheck);

void addOrUpdateList(NeighborInfo &neighborToUpdate, list<NeighborInfo> &listToCheck);

void showStatus(bool &searchingForNeighborFlag,
	list<NeighborInfo> &bidirectionalNeighbors,
	list<NeighborInfo> &unidirectionalNeighbors,
	NeighborInfo &tempNeighbor,
	HostId &thisHost,
	int targetNumberOfNeighbors);