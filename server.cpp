/*
 * server.cpp
 *
 *  Created on: Apr 19, 2014
 *      Author: manoj
 */

#include "server.h"

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <limits.h>
#include "common.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std;
#define TIMEOUT 5
#define DISPLAY "MNC_PROG_3>"
#define MAXBUFLEN 100

std::vector<std::vector<struct routing_info> > oRoutingTable(100);
int packetCount = 0;

/*
 * open and listen on own socket
 * */
void openListenSocket(int *listeningSocket, int myport) {
	//int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	char buf[MAXBUFLEN];
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, intToString(myport).c_str(), &hints, &servinfo))
			!= 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((*listeningSocket = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(*listeningSocket, p->ai_addr, p->ai_addrlen) == -1) {
			close(*listeningSocket);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return;
	}

	freeaddrinfo(servinfo);
	fcntl(*listeningSocket, F_SETFL, O_NONBLOCK);
}

/*
 * convert IP from buffer to string
 * */
string getIpFromBuffer(char *buffer) {
	string strIp;
	for (int i = 0; i < 4; i++) {
		int charVal = (int) buffer[i];
		charVal = (charVal + 256) % 256;
		strIp.append(intToString(charVal));
		if (i != 3) {
			strIp.append(".");
		}
	}
	return strIp;
}

/*
 * add ip to buffer as bytes
 * */
void addIpToBuffer(char *buffer, string ip) {
	vector<string> oSplitIp = splitString(ip, ".");
	if (oSplitIp.size() != 4) {
		cout << "improper ip address" << ip << endl;
		return;
	}
	/*
	 * adds 8-bit values to the buffer
	 * */
	for (int i = 0; i < 4; i++) {
		short int ip_sec = atoi(oSplitIp[i].c_str());
		buffer[i] = ip_sec;
	}
}

/*
 * displays the routing table of the server
 * */
void displayRouting(std::vector<struct server_info> oServerInfo, int myIndex) {
	cout << "Routing table" << endl;
	cout << "ID\tNext\tCost" << endl;
	for (int i = 0; i < oServerInfo.size(); i++) {
		cout << oServerInfo.at(i).id << "\t"
				<< (oRoutingTable[myIndex][i].via == INF ?
						"-" : intToString(oRoutingTable[myIndex][i].via))
				<< "\t"
				<< (oRoutingTable[myIndex][i].cost == INF ?
						"INF" : intToString(oRoutingTable[myIndex][i].cost))
				<< endl;
	}
}

/*
 * update routing table
 * This is usually called after there is a change in the neighbor
 * or the distance vectors of other nodes
 * */
void updateRoutingTable(std::vector<struct server_info> *oServerInfo,
		std::vector<struct neighbour_info> oNeighInfo, int myIndex) {
	for (int i = 0; i < oServerInfo->size(); i++) {
		if (myIndex == i) {
			continue;
		}
		int i_neigh_index = getNeighbourIndex(oNeighInfo,
				oServerInfo->at(i).id);
		if (i_neigh_index == -1) {
			oRoutingTable[myIndex][i].cost = INF;
			oRoutingTable[myIndex][i].via = INF;
		} else {
			if (oNeighInfo[i_neigh_index].status) {
				oRoutingTable[myIndex][i].cost = oNeighInfo[i_neigh_index].cost;
				oRoutingTable[myIndex][i].via = oNeighInfo[i_neigh_index].dest;
			} else {
				oRoutingTable[myIndex][i].cost = INF;
				oRoutingTable[myIndex][i].via = INF;
			}
		}
		for (int j = 0; j < oServerInfo->size(); j++) {
			int neigh_index = getNeighbourIndex(oNeighInfo,
					oServerInfo->at(j).id);
			if (neigh_index == -1) {
				continue;
			}
			if ((oRoutingTable[myIndex][i].cost
					> oRoutingTable[j][i].cost + oNeighInfo[neigh_index].cost)
					&& oNeighInfo[neigh_index].status) {
				oRoutingTable[myIndex][i].cost = oRoutingTable[j][i].cost
						+ oNeighInfo[neigh_index].cost;
				oRoutingTable[myIndex][i].via = oNeighInfo[neigh_index].dest;
			}
		}
	}
}

/*
 * send routing update to all the neighbors
 * */
void sendRoutingUpdate(std::vector<struct server_info> oServerInfo,
		std::vector<struct neighbour_info> oNeighInfo, int myId) {
	short int updateCount = oServerInfo.size();
	int myIndex = getIndexById(oServerInfo, myId);
	struct server_info myServer = oServerInfo[myIndex];
	int totalBufferSize = 8 + updateCount * 12;
	char buffer[totalBufferSize + 1];

	/*
	 * this builds the buffer with the information
	 * the packet structure is formed here
	 * */
	memcpy(buffer, &updateCount, 2);
	memcpy(buffer + 2, &(myServer.port), 2);
	addIpToBuffer(buffer + 4, myServer.ip);

	for (int i = 0; i < oServerInfo.size(); i++) {
		int iterationInc = 8 + i * 12;
		addIpToBuffer(buffer + iterationInc, oServerInfo[i].ip);
		memcpy(buffer + iterationInc + 4, &(oServerInfo[i].port), 2);
		memcpy(buffer + iterationInc + 8, &(oServerInfo[i].id), 2);
		memcpy(buffer + iterationInc + 10, &(oRoutingTable[myIndex][i].cost),
				2);
	}
	/*
	 * send the information to all the neighbours
	 * */
	for (int i = 0; i < oNeighInfo.size(); i++) {
		struct neighbour_info otemp = oNeighInfo.at(i);
		if (!otemp.status) {
			continue;
		}
		int index = getIndexById(oServerInfo, otemp.dest);
		int sockfd;
		struct addrinfo hints, *servinfo, *p;
		int rv;
		int numbytes;

		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;

		if ((rv = getaddrinfo(oServerInfo[index].ip,
				intToString(oServerInfo[index].port).c_str(), &hints, &servinfo))
				!= 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			continue;
		}

		// loop through all the results and make a socket
		for (p = servinfo; p != NULL; p = p->ai_next) {
			if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
					== -1) {
				perror("talker: socket");
				continue;
			}

			break;
		}

		if (p == NULL) {
			fprintf(stderr, "talker: failed to bind socket\n");
			continue;
		}

		if ((numbytes = sendto(sockfd, buffer, totalBufferSize, 0, p->ai_addr,
				p->ai_addrlen)) == -1) {
			perror("talker: sendto");
			return;
		}
		freeaddrinfo(servinfo);

		close(sockfd);

	}
}

/*
 * process user input and take appropriate action
 * */
void processUserInput(std::vector<struct server_info> *oServerInfo,
		std::vector<struct neighbour_info> *oNeighInfo, vector<int> *oTimers,
		int myId, int timeout) {
	string uInput;
	getline(cin, uInput);
	string errMessage;
	std::transform(uInput.begin(), uInput.end(), uInput.begin(), ::tolower);
	vector<string> arrSplit = splitString(uInput, " ");
	if (arrSplit.size() == 0) {
		cout << "Enter a valid input!" << endl;
		return;
	}
	if (arrSplit[0].compare("crash") == 0) {
		while (1) {
		}
	}
	if (arrSplit[0].compare("step") == 0) {
		/*
		 * user generated sending of data
		 * */
		sendRoutingUpdate(*oServerInfo, *oNeighInfo, myId);
		goto success;
	}
	if (arrSplit[0].compare("packets") == 0) {
		cout << "No of Packets received since last invocation: " << packetCount
				<< endl;
		packetCount = 0;
		goto success;
	}
	if (arrSplit[0].compare("display") == 0) {
		int myindex = getIndexById(*oServerInfo, myId);
		displayRouting(*oServerInfo, myindex);
		goto success;
	}
	if (arrSplit[0].compare("disable") == 0) {
		if (arrSplit.size() < 2) {
			errMessage = "not enough parameters!";
			goto error;
			return;
		}
		if (!checkInteger(arrSplit[1])) {
			errMessage = "IDs are not proper";
			goto error;
			return;
		}
		int destServerId = atoi(arrSplit[1].c_str());
		int destServerNeighIndex = getNeighbourIndex(*oNeighInfo, destServerId);
		if (destServerNeighIndex == -1) {
			errMessage = "No Such Neighbor";
			goto error;
		}
		/*
		 * removes entry for neighbor once disabled
		 * It cannot be reactivated again
		 * Also remove the entry for the neighbor in the timer
		 * */
		oNeighInfo->erase(oNeighInfo->begin() + destServerNeighIndex);

		oTimers->erase(oTimers->begin() + destServerNeighIndex + 1);
		updateRoutingTable(oServerInfo, *oNeighInfo,
				getIndexById(*oServerInfo, myId));
		goto success;
	}

	if (arrSplit[0].compare("update") == 0) {
		if (arrSplit.size() < 4) {
			errMessage = "not enough parameters!";
			goto error;
		}
		if (!checkInteger(arrSplit[1]) || !checkInteger(arrSplit[2])) {
			errMessage = "IDs are not proper";
			goto error;
		}
		int src = atoi(arrSplit[1].c_str());
		int dest = atoi(arrSplit[2].c_str());
		int cost;
		if (arrSplit[3].compare("inf") == 0) {
			cost = INF;
		} else {
			if (!checkInteger(arrSplit[3])) {
				errMessage = "incorrect cost value";
				goto error;
			}
			cost = atoi(arrSplit[3].c_str());
		}
		if (src == dest) {
			errMessage = "Source and Destination cannot be the same!";
			goto error;
		}
		if (src != myId) {
			errMessage = "Source should be your own ID!";
			goto error;
		}
		if (getIndexById(*oServerInfo, dest) == -1) {
			errMessage = "Not a valid Server ID";
		}
		int destServerNeighIndex = getNeighbourIndex(*oNeighInfo, dest);
		if (destServerNeighIndex == -1) {
			errMessage = "Only costs to neighbors can be updated!";
			goto error;
		} else {
			/*
			 * update the cost of the entry in the neighbor table
			 * and update the routing table
			 * */
			oNeighInfo->at(destServerNeighIndex).cost = cost;
		}
		updateRoutingTable(oServerInfo, *oNeighInfo,
				getIndexById(*oServerInfo, myId));
		goto success;
	}
	return;

	success: cout << arrSplit[0] << " SUCCESS" << endl;
	return;

	error: cout << arrSplit[0] << " " << errMessage << endl;
}

/*
 * accept incoming packet and process accordingly
 * and returns the index of the neighbour which sent the message
 * */
int processIncomingMessage(int listeningSocket,
		std::vector<struct server_info> *oServerInfo,
		std::vector<struct neighbour_info> oNeighInfo, int myID) {
	int numbytes;
	char buf[100];

	socklen_t addr_len;
	struct sockaddr_storage their_addr;
	addr_len = sizeof their_addr;
	/*
	 * preview the UDP message to check the header information
	 * */
	if ((numbytes = recvfrom(listeningSocket, buf, 8, MSG_PEEK,
			(struct sockaddr *) &their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}
	/*
	 * Extract the header information first as it was inserted at
	 * the sender end
	 * */
	short int updateCount, srcPort;
	memcpy(&updateCount, buf, 2);
	memcpy(&srcPort, buf + 2, 2);
	string strSourceIP = getIpFromBuffer(buf + 4);
	int myIndex = getIndexById(*oServerInfo, myID);
	int sourceIndex = getIndexByIpPort(*oServerInfo, strSourceIP, srcPort);

	if (sourceIndex == -1) {
		if ((numbytes = recvfrom(listeningSocket, buf, sizeof(buf), 0,
				(struct sockaddr *) &their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}
		return -1;
	}

	int neighIndex = getNeighbourIndex(oNeighInfo,
			oServerInfo->at(sourceIndex).id);
	if (neighIndex == -1) {
		if ((numbytes = recvfrom(listeningSocket, buf, sizeof(buf), 0,
				(struct sockaddr *) &their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}
		return -1;
	}
	packetCount++;
	char buf_whole_data[8 + 12 * updateCount];

	/*
	 * one we have the header information get the whole message
	 * */
	if ((numbytes = recvfrom(listeningSocket, buf_whole_data,
			sizeof(buf_whole_data), 0, (struct sockaddr *) &their_addr,
			&addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	for (int i = 0; i < updateCount; i++) {

		/*
		 * Extract the update information of all the nodes
		 * from the buffer received in the same order as was inserted
		 * The count was retained by the header information
		 * */
		int base = 8 + 12 * i;
		string strIP = getIpFromBuffer(buf_whole_data + base);
		short int port, serverId, serverCost;
		memcpy(&port, buf_whole_data + base + 4, 2);
		memcpy(&serverId, buf_whole_data + base + 8, 2);
		memcpy(&serverCost, buf_whole_data + base + 10, 2);
		int destIndex = getIndexById(*oServerInfo, serverId);
		oRoutingTable[sourceIndex][destIndex].cost = serverCost;
	}
	updateRoutingTable(oServerInfo, oNeighInfo, myIndex);
	return neighIndex;
}

/*
 * gets the index of the minimum value in timer
 * */
int getMinimumTime(vector<int> oTimers) {
	int min = INT_MAX;
	int index = -1;
	for (int i = 0; i < oTimers.size(); i++) {
		if (oTimers[i] <= min && oTimers[i] != -1) {
			min = oTimers[i];
			index = i;
		}
	}
	return index;
}

/*
 * lowers all the timers by specified value
 * */
void decTimerBy(vector<int> *oTimers, int dec) {
	for (int i = 0; i < oTimers->size(); i++) {
		if (oTimers->at(i) != -1) {
			oTimers->at(i) = oTimers->at(i) - dec;
		}
	}
}

/*
 * remove link from one's own distance vector table
 * */
void removeLink(std::vector<struct server_info> *oServerInfo,
		std::vector<struct neighbour_info> *oNeighInfo, int removeIndex,
		int myId) {

	int removeId = oServerInfo->at(removeIndex).id;
	int myIndex = getIndexById(*oServerInfo, myId);
	oNeighInfo->at(removeIndex).status = 0;
	updateRoutingTable(oServerInfo, *oNeighInfo, myIndex);
}

/*
 * called from main
 * initiates and maintains the console application
 * waits for user input and packet from listening socket
 * */
void startServer(std::vector<struct server_info> oServerInfo,
		std::vector<struct neighbour_info> oNeighInfo, int myid, int timeout) {

	int myServerIndex = getIndexById(oServerInfo, myid);
	int server_count = oServerInfo.size();
	oRoutingTable.resize(server_count);

	/*
	 * Initialize the routing table with either the cost of neighbor
	 * if present or infinity which is SHRT_MAX
	 * */

	for (int i = 0; i < server_count; i++) {
		oRoutingTable[i].resize(server_count);
		for (int j = 0; j < server_count; j++) {

			if (myServerIndex != i) {
				oRoutingTable[i][j].cost = INF;
				oRoutingTable[i][j].via = INF;
			} else {
				int neighIndex = getNeighbourIndex(oNeighInfo,
						oServerInfo[j].id);
				if (neighIndex == -1) {
					oRoutingTable[i][j].cost = INF;
					oRoutingTable[i][j].via = INF;
				} else {
					oRoutingTable[i][j].cost = oNeighInfo[neighIndex].cost;
					oRoutingTable[i][j].via = oNeighInfo[neighIndex].dest;
				}
			}
		}
	}

	/*
	 * set its own cost to zero
	 * and via to itself
	 * */
	oRoutingTable[myServerIndex][myServerIndex].via = myid;
	oRoutingTable[myServerIndex][myServerIndex].cost = 0;
	int myport = oServerInfo[myServerIndex].port;

	int fdmax = 0;
	int listeningSocket;
	fd_set master; // master file descriptor list

	/*
	 * Initialize the timer
	 * The first value is for timer for periodic sending of update
	 * The other values are for each neighbor which is set to thrice
	 * the value of timeout interval supplied by the user
	 * */
	vector<int> oTimers;
	oTimers.push_back(timeout);
	for (int i = 0; i < oNeighInfo.size(); i++) {
		oTimers.push_back(timeout * 3);
	}

	openListenSocket(&listeningSocket, myport);
	fdmax = fdmax > listeningSocket ? fdmax : listeningSocket;
	struct timeval tv;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	int prevTime = timeout;
	cout << DISPLAY;
	cout.flush();

	while (1) {
		FD_ZERO(&master);
		FD_SET(listeningSocket, &master);
		FD_SET(0, &master);
		if (select(fdmax + 1, &master, NULL, NULL, &tv) == -1) {
			perror("select");
			exit(4);
		}
		int iCount = 0;
		for (int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &master)) {
				if (i == 0) {
					/*
					 * if user input
					 * */
					processUserInput(&oServerInfo, &oNeighInfo, &oTimers, myid,
							timeout);
					cout << DISPLAY;
					cout.flush();
				} else if (i == listeningSocket) {
					/*
					 * if message in listening socket
					 * */
					int neighbourIndex = processIncomingMessage(listeningSocket,
							&oServerInfo, oNeighInfo, myid);
					if (neighbourIndex != -1) {
						oNeighInfo.at(neighbourIndex).status = 1;
						decTimerBy(&oTimers, prevTime - tv.tv_sec);
						oTimers[neighbourIndex + 1] = timeout * 3;
						int nextIndex = getMinimumTime(oTimers);
						prevTime = tv.tv_sec = oTimers[nextIndex];
						cout << endl << "RECEIVED A MESSAGE FROM SERVER "
								<< oNeighInfo.at(neighbourIndex).dest << endl;
						cout << DISPLAY;
						cout.flush();
					} else {
						cout << "a packet was discarded!" << endl;
					}
				}
			} else {
				iCount++;
			}
		}
		if (iCount > fdmax) {
			int index = getMinimumTime(oTimers);
			decTimerBy(&oTimers, oTimers[index]);
			if (index == 0) {
				/*
				 * this is when timer interrupt occurs for sending update
				 * */
				oTimers[0] = timeout;
				sendRoutingUpdate(oServerInfo, oNeighInfo, myid);
			} else {
				/*
				 * mark it as dead by assigning -1
				 * as after timeout it has still not responded
				 */
				oTimers[index] = -1;
				removeLink(&oServerInfo, &oNeighInfo, index - 1, myid);
				cout << "timout called for: " << index << endl;
			}
			int nextIndex = getMinimumTime(oTimers);
			prevTime = tv.tv_sec = oTimers[nextIndex];
		}
	}
}

