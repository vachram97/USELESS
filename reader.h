/*
 * reader.h
 *
 *  Created on: 11.12.2016
 *      Author: vachram
 */

#ifndef READER_H_
#define READER_H_

#include "includes.h"

int read_tasks(char *, vector <command> *, vector <task> *);
int check_date(time_s *);
int parse_time(string, time_s *);
int parse_string(string, command *);
int taskscmp(task, task);
int read_server_list(char *, vector <host> *);
int host_cmp_by_name(host, host);


#endif /* READER_H_ */
