/*
 * reader.cpp
 *
 *  Created on: 11.12.2016
 *      Author: vachram
 *      Functions for reading
 */

#include "includes.h"
#include "reader.h"

int mdays[12] = {31, 28, 31, 30, 31, 31, 30, 31, 30, 31, 30, 31};//days in month

/*
 * function gets file and pointer at tasks structure, fills it with commands
 * return 0 on success, -1 otherwise
 */
int read_tasks(char * file, vector <command> *tasks, vector<task> *task_queue) {

	string line;
	command comm;
	int line_number = 0; //current number line
	ifstream input;

	time_t curr_time_t = time(NULL);
	struct tm curr_time;
	localtime_r(&curr_time_t, &curr_time);

	input.open(file, ios::in);
	if (!input.is_open()) {
		printf("Error opening mycrontab file '%s': %s\n", file, strerror(errno));
		return -1;
	}

	while (!input.eof()) {
		line_number++;
		getline(input, line);
		if (input.fail()) {
			printf("Error while reading '%s': %s\n", file, strerror(errno));
			input.close();
			return -1;
		}
		if (line[0] == '#') continue; //comments shouldn't be read
		if (-1 == parse_string(line, &comm)) {
			printf("Error in %d string of '%s': ignored\n", line_number, file);
			continue;
		}

		comm.execution_time.tm_year = curr_time.tm_year;

		comm.execution_time.tm_sec = 0;
		task tmp;
		(*tasks).push_back(comm);
		tmp.time = comm.execution_time;
		tmp.number = (*tasks).size()-1;
		if ((comm.execution_time.tm_min != -1) && (comm.execution_time.tm_hour != -1)
				&& (comm.execution_time.tm_mday != -1) && (comm.execution_time.tm_mon != -1)) {
			(*task_queue).push_back(tmp);
			continue;
		}

		if ((comm.execution_time.tm_min != -1) && (comm.execution_time.tm_hour != -1)
				&& (comm.execution_time.tm_mday != -1) && (comm.execution_time.tm_mon == -1)) {
			for (int i = 0; i < 12; i++) {
				tmp.time.tm_mon = i;
				(*task_queue).push_back(tmp);
			}
			continue;
		}

		if ((comm.execution_time.tm_min != -1) && (comm.execution_time.tm_hour != -1)
				&& (comm.execution_time.tm_mday == -1) && (comm.execution_time.tm_mon == -1)) {
			for (int i = 0; i < 12; i++) {
				tmp.time.tm_mon = i;
				for (int j = 1; j <= mdays[i]; j++) {
					tmp.time.tm_mday = j;
					(*task_queue).push_back(tmp);
				}
			}
			continue;
		}

		if ((comm.execution_time.tm_min != -1) && (comm.execution_time.tm_hour == -1)
				&& (comm.execution_time.tm_mday == -1) && (comm.execution_time.tm_mon == -1)) {
			for (int i = 0; i < 12; i++) {
				tmp.time.tm_mon = i;
				for (int j = 1; j <= mdays[i]; j++) {
					tmp.time.tm_mday = j;
					for (int k = 0; k < 24; k++) {
						tmp.time.tm_hour = k;
						(*task_queue).push_back(tmp);
					}
				}
			}
			continue;
		}

		if ((comm.execution_time.tm_min == -1) && (comm.execution_time.tm_hour == -1)
				&& (comm.execution_time.tm_mday == -1) && (comm.execution_time.tm_mon == -1)) {
			for (int i = 0; i < 12; i++) {
				tmp.time.tm_mon = i;
				for (int j = 1; j <= mdays[i]; j++) {
					tmp.time.tm_mday = j;
					for (int k = 0; k < 24; k++) {
						tmp.time.tm_hour = k;
						for (int l = 0; l < 60; l++) {
							tmp.time.tm_min = l;
							(*task_queue).push_back(tmp);
						}
					}
				}
			}
			continue;
		}

		(*tasks).pop_back();
		printf("Error in %d string in '%s':ignored\n", line_number, file);
	}
	input.close();
	sort((*task_queue).begin(), (*task_queue).end(), taskscmp);
	return 0;
}

/*
 * function gets string and pointer at command structure, it fills the structure
 * returns 0 on success, -1 otherwise
 */
int parse_string(string line, command *comm) {
	size_t space = 0;
	for (int i = 0; i < 4; i++) {
		space = line.find(' ', space+1);
		if (space == string::npos) {
			return -1;
		}
	}
	if (-1 == parse_time(line.substr(0, space), &(comm->execution_time))) {
		return -1;
	}

	//parsing to host and command
	size_t space_2 = line.find(' ', space + 1);
	if (space_2 == string::npos) return -1;
	comm->host = line.substr(space + 1, space_2 - space - 1);
	comm->command_line = line.substr(space_2 + 1);
	return 0;
}

/*
 * function get string and pointer at time structure and fill this structure
 * return 0 on success, -1 otherwise
 */
int parse_time(string line, struct tm *tim) {
	size_t space;
	//replacing * -> -1
	while (string::npos != (space = line.find('*'))) {
		line.replace(space, 1, "-1");
	}
	if (4 != sscanf(line.c_str(), "%d %d %d %d", &(tim->tm_min), &(tim->tm_hour), &(tim->tm_mday), &(tim->tm_mon))) {
		return -1;
	}
	if (-1 == check_date(tim)) {
		return -1;
	}
	tim->tm_isdst = 0; //to determine DST
	return 0;
}

/*
 * function checks if the date is correct, return 0 if correct, -1 otherwise
 */
//TODO: think about more correct checking, now
//we are assuming that people couldn't make mistakes
int check_date(struct tm *tim) {
	if ((tim->tm_min > 59) || (tim->tm_min < -1) || (tim->tm_hour > 23)
			|| (tim->tm_hour < -1) || (tim->tm_mday > 31) || (tim->tm_mday < -1)
			|| (tim->tm_mon > 12) || (tim->tm_mon < -1)) return -1;
	return 0;
}

/*
 * function compares the times of command execution and returns 1 if the first is earlier
 * than the second and 0 otherwise
 */
int taskscmp(task first, task second) {
	int k = (second.time.tm_min - first.time.tm_min)
			+ (second.time.tm_hour - first.time.tm_hour) * 100
			+ (second.time.tm_mday - first.time.tm_mday) * 10000
			+ (second.time.tm_mon - first.time.tm_mon) * 1000000;
	return (k > 0) ? 1 : 0;
}

/*
 * function read file with servers and returns vector with them
 */
int read_server_list(char * file, vector <host> *host_list) {
	string line;
	host curr_host;
	int line_number = 0; //current number line
	ifstream input;

	input.open(file, ios::in);
	if (!input.is_open()) {
		printf("Error opening hosttab file '%s': %s\n", file, strerror(errno));
		return -1;
	}

	while (!input.eof()) {
		line_number++;
		getline(input, line);
		if (input.bad()) {
			printf("Error while reading '%s': %s\n", file, strerror(errno));
			input.close();
			return -1;
		}
		if (line[0] == '#') continue; //comments shouldn't be read

		//get name
		size_t space = line.find(' ');
		if (space == string::npos) {
			printf("Error in %d string of '%s': ignored\n", line_number, file);
			continue;
		}
		curr_host.name = line.substr(0, space);
		if ((curr_host.name == "all") || (curr_host.name == "localhost")) continue;
		//get ip and make an address structure
		size_t n_space = line.find(' ', space + 1);
		string tmp = line.substr(space + 1, n_space - space - 1);
		if (1 != inet_pton(AF_INET, tmp.c_str(), (void *)&(curr_host.addr))) {
			printf("Error in %d string of '%s': ignored\n", line_number, file);
			continue;
		}

		//get port and reverse byte order
		space = line.find(' ', n_space + 1);
		if (space == string::npos) {
			printf("Error in %d string of '%s': ignored\n", line_number, file);
			continue;
		}
		tmp = line.substr(n_space+1, space - n_space - 1);
		int port_host = atoi(tmp.c_str());
		if ((port_host < 0) || (port_host > 65535)) {
			printf("Error in %d string of '%s': ignored\n", line_number, file);
			continue;
		}
		curr_host.port = htons(port_host);
		//passwd
		n_space = line.find(' ', space + 1);
		if (n_space != string::npos) {
			printf("Error in %d string of '%s': ignored\n", line_number, file);
			continue;
		}
		curr_host.passwd = line.substr(space+1);

		(*host_list).push_back(curr_host);
	}
	sort((*host_list).begin(), (*host_list).end(), host_cmp_by_name);
	return 0;
}

/*
 * compares hosts by name
 */
int host_cmp_by_name(host first, host second) {
	return (first.name.compare(second.name) > 0)? 0 : 1;
}
