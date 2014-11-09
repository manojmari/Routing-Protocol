/*
 * common.h
 *
 *  Created on: Feb 17, 2014
 *      Author: manoj
 */

#ifndef COMMON_H_
#define COMMON_H_
#include <string>
#define MAXDATASIZE 1024
#define MAXCLIENTCONNECTION 3
#define strDisplayMsg "Enter command-> "
#include <string>
#include <vector>
#include <limits.h>

#define INF SHRT_MAX

struct routing_info {
	short int cost;
	short int via;
};

struct server_info {
	short int id;
	char ip[50];
	short int port;
};

struct neighbour_info {
	int source;
	int dest;
	short int cost;
	int status;
};

std::vector<std::string> splitString(std::string s, std::string delimiters);
bool checkInteger(std::string input);
std::string intToString(int number);
int getIndexById(std::vector<struct server_info> oServerList, int id);
int getNeighbourIndex(std::vector<neighbour_info> oNeighList, int dest);
int getIndexByIpPort(std::vector<server_info> oServerList, std::string strIp,
		short int port);

#endif /* COMMON_H_ */
