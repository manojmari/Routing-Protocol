/*
 * main.cpp
 *
 *  Created on: Apr 19, 2014
 *      Author: manoj
 */

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
#include "common.h"
#include "server.h"
using namespace std;

bool serverCompare(const server_info &a, const server_info &b) {
	return a.id < b.id;
}

int main(int argc, char* argv[]) {
	char input[] = "1tri.txt";
	int timeout;
	if (argc <= 4) {
		cout << "Invalid Arguments!!" << endl;
		return 0;
	}
	if (strcmp(argv[1], "-t") == 0 && strcmp(argv[3], "-i") == 0) {
		timeout = atoi(argv[4]);
		strcpy(input, argv[2]);
	} else {
		if (strcmp(argv[1], "-i") == 0 && strcmp(argv[3], "-t") == 0) {
			timeout = atoi(argv[2]);
			strcpy(input, argv[4]);
		} else {
			cout << "Invalid Arguments!!" << endl;
			return 0;
		}
	}

	std::ifstream infile(input);
	std::string line;
	std::vector<server_info> oServerList;
	std::vector<neighbour_info> oNeighbourList;

	std::getline(infile, line);
	int i;
	if (!checkInteger(line)) {
		return 0;
	}
	int server_count = atoi(line.c_str());
	std::getline(infile, line);
	if (!checkInteger(line)) {
		return 0;
	}
	int neighbr_count = atoi(line.c_str());

	/*
	 * getting the server list information from the topology file
	 * */
	for (i = 0; i < server_count; i++) {
		std::getline(infile, line);
		std::vector<string> splitVector;
		splitVector = splitString(line, " ");
		struct server_info oTemp;
		oTemp.id = atoi(splitVector[0].c_str());
		strcpy(oTemp.ip, splitVector[1].c_str());
		oTemp.port = atoi(splitVector[2].c_str());
		oServerList.push_back(oTemp);
	}
	std::sort(oServerList.begin(), oServerList.end(), serverCompare);

	int myid;

	/*
	 * getting the neighbor information from the topology file
	 * */
	for (i = 0; i < neighbr_count; i++) {
		std::getline(infile, line);

		std::vector<string> splitVector;
		splitVector = splitString(line, " ");
		struct neighbour_info oTemp;
		oTemp.source = atoi(splitVector[0].c_str());
		myid = oTemp.source;
		oTemp.dest = atoi(splitVector[1].c_str());
		oTemp.cost = atoi(splitVector[2].c_str());
		oTemp.status = 1;
		oNeighbourList.push_back(oTemp);
	}

	startServer(oServerList, oNeighbourList, myid, timeout);

	return 0;
}

