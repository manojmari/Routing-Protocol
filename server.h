/*
 * server.h
 *
 *  Created on: Apr 19, 2014
 *      Author: manoj
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <vector>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include "common.h"

void startServer(std::vector<struct server_info>,
		std::vector<struct neighbour_info>, int myport, int timeout);

#endif /* SERVER_H_ */
