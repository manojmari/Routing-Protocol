/*
 * common.cpp
 *
 *  Created on: Feb 17, 2014
 *      Author: manoj
 */

#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <cstdio>
#include <sstream>
#include "common.h"

using namespace std;

std::vector<std::string>  splitString(std::string s, std::string delimiters) {

	std::vector<std::string> arrString;
	size_t current;
	size_t next = -1;
	int i = 0;

	do {
		current = next + 1;
		next = s.find_first_of(delimiters, current);
		string strTemp = s.substr(current, next - current);

		if (strTemp.compare("") != 0) {
			arrString.push_back(strTemp);
		}
	} while (next != std::string::npos && i < 5);
	return arrString;
}

bool checkInteger(string input) {
	char * p;
	strtol(input.c_str(), &p, 10);
	return (*p == 0);
}

string intToString(int number) {
	std::stringstream ss;
	ss << number;
	return ss.str();
}

/*
 * get index of server information based on id
 * */
int getIndexById(vector<server_info> oServerList, int id) {
	int index = -1;
	for (int i = 0; i < oServerList.size(); i++) {
		if (oServerList[i].id == id) {
			index = i;
			break;
		}
	}
	return index;
}

/*
 * get index of server information based on IP and Port number
 * */
int getIndexByIpPort(vector<server_info> oServerList, string strIp,
		short int port) {
	int index = -1;
	for (int i = 0; i < oServerList.size(); i++) {
		if (oServerList[i].port == port
				&& strIp.compare(oServerList[i].ip) == 0) {
			index = i;
			break;
		}
	}
	return index;
}

/*
 * get index of neighbor information based on id
 * */
int   getNeighbourIndex(vector<neighbour_info> oNeighList, int dest) {
	int index = -1;
	for (int i = 0; i < oNeighList.size(); i++) {
		if (oNeighList[i].dest == dest) {
			index = i;
		}
	}
	return index;
}
