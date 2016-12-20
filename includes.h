/*
 * includes.h
 *
 *  Created on: 11.12.2016
 *      Author: vachram
 */

#ifndef INCLUDES_H_
#define INCLUDES_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/limits.h>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <time.h>
#include <poll.h>
#include <mqueue.h>
#include <crypt.h>

#define BUF_SIZE 8128

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

using namespace std;

struct command {
	string command_line;
	string host;
	struct tm execution_time;
};

struct host {
	string name;
	struct in_addr addr;
	int port;
	string passwd;
};

struct task {
	struct tm time;
	int number;
};



#endif /* INCLUDES_H_ */
