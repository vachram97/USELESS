/*
 * execution.cpp
 *
 *  Created on: 12.12.2016
 *      Author: vachram
 */

#include "execution.h"

/*
 * executes command on host
 */
int execute_command (command *comm, host *ex_host){
	//creates special process for execution
	pid_t pid_child = fork();
	if (pid_child != 0) return 0;

	char buf[BUF_SIZE];
	int cl_sock = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server;

	server.sin_family = AF_INET;
	server.sin_addr = ex_host->addr;
	server.sin_port = ex_host->port;

	if (-1 == connect(cl_sock, (struct sockaddr *) &server, sizeof(server))) {
		logerr(ex_host->name, "Unable to establish connection\n");
		close(cl_sock);
		exit(EXIT_FAILURE);
	}
	//verification procedure
	if (-1 == verify(cl_sock, (*ex_host).passwd)) {
		logerr(ex_host->name, "NO VERIFICATION: error\n");
		exit(EXIT_FAILURE);
	}
	//sending command
	strcpy(buf, "COMM \0");
	strcat(buf, (*comm).command_line.c_str());

	if (-1 == send(cl_sock, buf, strlen(buf)+1, MSG_NOSIGNAL)) {
		logerr(ex_host->name, "Command isn't sent: error\n");
		close(cl_sock);
		exit(EXIT_FAILURE);
	}
	
	if (-1 == recv(cl_sock, buf, BUF_SIZE, MSG_NOSIGNAL)) {
		logerr(ex_host->name, "Command sent, but connection was broken: error\n");
		close(cl_sock);
		exit(EXIT_FAILURE);
	}
	
	if (strcmp(buf, "OK\0") != 0) {
		logerr(ex_host->name, "Command rejected by server\n");
	}
	//send command DISCONNECT, with no error checking
	strcpy(buf, "DISCONNECT\0");
	send(cl_sock, buf, BUF_SIZE, MSG_NOSIGNAL);
	recv(cl_sock, buf, BUF_SIZE, MSG_NOSIGNAL);
	shutdown(cl_sock, SHUT_RDWR);
	close(cl_sock);
	if (strcmp(buf, "OK\0") == 0) exit(EXIT_SUCCESS);
	else exit(EXIT_FAILURE);
}

/*
 * executed command locally, writing output in localhost.log
 */
int execute_command_localhost(command *comm) {
	//create process for execution
	pid_t pid_child = fork();
	if (pid_child != 0) return 0;
	char s[30];
	//opening filelog
	int fd = open("localhost.log", O_WRONLY | O_APPEND | O_CREAT, 0700);
	char log[400];
	sprintf(log, "[%s]: Executing command '%s'...\n", make_normal_current_time(s, 30), comm->command_line.c_str());
	write(fd, log, strlen(log));
	//execution command
	int exec_stat;
	exec_stat = system(comm->command_line.c_str());
	sprintf(log, "[%s]: Execution ended with code %d\n", make_normal_current_time(s, 30), exec_stat);
	write(fd, log, strlen(log));
	close(fd);
	//if smth goes wrong
	if (exec_stat != 0) {
		FILE *err_log = fopen("error.log", "a");
		fprintf(err_log, "[%s]: Command '%s' at localhost returned %d\n",
				make_normal_current_time(s, 30), comm->command_line.c_str(), exec_stat);
	}
	exit (EXIT_SUCCESS);
}

/*
 * function deletes '\n' at ctime output to bring beauty in our log files
 * returns pointer to array of chars
 */
char *make_normal_current_time(char * s, int size) {
	time_t tim = time(NULL);
	struct tm time_str;
	localtime_r(&tim, &time_str);
	strftime(s, size, "%H:%M:%S %d %b %Y", &time_str);
	return s;
}

/*
 * writes error msg to log
 */
int logerr(string hostname, string msg){
	char s[30];
	FILE *logfile = fopen("error.log", "a");
	fprintf(logfile, "[%s]: '%s' : %s\n", make_normal_current_time(s, 30), hostname.c_str(), msg.c_str());
	fclose(logfile);
	return 0;
}

/*
 * CHAP verification
 */
int verify(int fd, string passwd) {
	char buf[BUF_SIZE];
	
	strcpy(buf, "CONNECT");
	if (-1 == send(fd, buf, strlen(buf)+1, MSG_NOSIGNAL)) {
		return -1;
	}
	
	if (-1 == recv(fd, buf, BUF_SIZE, MSG_NOSIGNAL)) {
		return -1;
	}
	char salt[20];
	//making salt and calculating hash
	sprintf(salt,"$6$%s$", buf);
	
	char *hash = crypt(passwd.c_str(), salt);
	
	if (-1 == send(fd, hash, strlen(hash), MSG_NOSIGNAL)) {
		return -1;
	}
	if (-1 == recv(fd, buf, BUF_SIZE, MSG_NOSIGNAL)) {
		return -1;
	}
	if ((strcmp(buf, "OK\0") == 0)) return 0;
	else return -1;
}

