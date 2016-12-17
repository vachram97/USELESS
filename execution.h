/*
 * execution.h
 *
 *  Created on: 12.12.2016
 *      Author: vachram
 */

#ifndef EXECUTION_H_
#define EXECUTION_H_

#include "includes.h"

int execute_command(command *, host *);
int execute_command_localhost(command *);
char *make_normal_current_time(char *, int);
int logerr(string, string);
int verify(int, string);

#endif /* EXECUTION_H_ */
