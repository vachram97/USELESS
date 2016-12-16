/*
 * execution.cpp
 *
 *  Created on: 12.12.2016
 *      Author: vachram
 */

#include "execution.h"

/*
 * compares time_s
 */
int time_s_cmp(time_s first, time_s second) {
	int k = (second.min - first.min)
			+ (second.hour - first.hour) * 100
			+ (second.day - first.day) * 10000
			+ (second.month - first.month) * 1000000;
	return (k >= 0) ? 1 : 0;
}

/*
 * compares two time_structures
 */
int is_equal_time_s(time_s first, time_s second) {
	int k = (second.min - first.min)
			+ (second.hour - first.hour) * 100
			+ (second.day - first.day) * 10000
			+ (second.month - first.month) * 1000000;
	return (k == 0) ? 1 : 0;
}

/*
 * fill fields of tm structure using time_s structure
 */
int fill_tm_using_time_s(struct tm *dest, time_s* source) {
	dest->tm_hour = source->hour;
	dest->tm_min = source->min;
	dest->tm_mon = source->month;
	dest->tm_mday = source->day;
	return 0;
}

/*
 * executes command on host
 */
int execute_command (command *comm, host *ex_host){
	pid_t pid_child = fork();
	if (pid_child != 0) return 0;

	char buf[BUF_SIZE];
	int cl_sock = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server;

	server.sin_family = AF_INET;
	server.sin_addr = ex_host->addr;
	server.sin_port = ex_host->port;

	if (-1 == connect(cl_sock, (struct sockaddr *) &server, sizeof(server))) {
		logerr((*ex_host).name, "Unable to establish connection\n");
		close(cl_sock);
		exit(EXIT_FAILURE);
	}

	if (-1 == verify(cl_sock, (*ex_host).passwd)) {
		logerr((*ex_host).name, "NO VERIFICATION: error\n");
		exit(EXIT_FAILURE);
	}
	//checking for new information
	strcpy(buf, "GET\0");
	if (-1 == send(cl_sock, buf, strlen(buf)+1, MSG_NOSIGNAL)) {
		logerr((*ex_host).name, "Command isn't sent: error\n");
		exit(EXIT_FAILURE);
	}
	//printf("SENT: %s\n", buf);
	ssize_t size;
	if (-1 == (size = recv(cl_sock, buf, BUF_SIZE-1, MSG_NOSIGNAL))) {
		logerr((*ex_host).name, "Command isn't sent: error\n");
		exit(EXIT_FAILURE);
	}
	//printf("GOT: %s\n", buf);
	buf[size] = '\n';
	if (strcmp(buf, "NONE\0") != 0) {
		string file = (*ex_host).name + ".log";
		int fd = open(file.c_str(), O_WRONLY|O_APPEND|O_CREAT, 0700);
		write(fd, buf, size);
		close(fd);
	}

	strcpy(buf, "COMM \0");
	strcat(buf, (*comm).command_line.c_str());

	if (-1 == send(cl_sock, buf, strlen(buf)+1, MSG_NOSIGNAL)) {
		logerr((*ex_host).name, "Command isn't sent: error\n");
		close(cl_sock);
		exit(EXIT_FAILURE);
	}
	//printf ("SENT: %s\n", buf);
	if (-1 == recv(cl_sock, buf, BUF_SIZE, MSG_NOSIGNAL)) {
		logerr((*ex_host).name, "Command sent, but connection was broken: error\n");
		close(cl_sock);
		exit(EXIT_FAILURE);
	}
	///printf ("GOT: %s\n", buf);
	if (strcmp(buf, "OK\0") != 0) {
		logerr((*ex_host).name, "Command rejected\n");
	}
	//send command DISCONNECT, with no error checking
	strcpy(buf, "DISCONNECT\0");
	send(cl_sock, buf, BUF_SIZE, MSG_NOSIGNAL);
	//printf("SENT: %s\n", buf);
	recv(cl_sock, buf, BUF_SIZE, MSG_NOSIGNAL);
	//printf("GOT: %s\n", buf);
	shutdown(cl_sock, SHUT_RDWR);
	close(cl_sock);
	if (strcmp(buf, "OK\0") == 0) exit(EXIT_SUCCESS);
	else exit(EXIT_FAILURE);
}

/*
 * executed command locally, writing output in localhost.log
 */
int execute_command_localhost(command *comm) {
	pid_t pid_child = fork();
	if (pid_child != 0) return 0;
	char s[30];
	int fd = open("localhost.log", O_WRONLY | O_APPEND | O_CREAT, 0700);
	dup2(fd, 1);
	dup2(fd, 2);
	struct flock lock_s = {F_WRLCK, SEEK_SET, 0, 0};
	fcntl(fd, F_SETLKW, &lock_s);
	char log[400];
	sprintf(log, "[%s]: Executing command '%s'...\n", make_normal_current_time(s, 30), comm->command_line.c_str());
	write(fd, log, strlen(log));
	int exec_stat;
	exec_stat = system(comm->command_line.c_str());
	sprintf(log, "[%s]: Execution ended with code %d\n", make_normal_current_time(s, 30), exec_stat);
	write(fd, log, strlen(log));
	lock_s.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock_s);
	close(fd);
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
 * passwd sending
 */
int verify(int fd, string passwd) {
	char buf[BUF_SIZE];

	if (-1 == send(fd, passwd.c_str(), passwd.length(), MSG_NOSIGNAL)) {
		return -1;
	}
	if (-1 == recv(fd, buf, BUF_SIZE, MSG_NOSIGNAL)) {
		return -1;
	}
	if ((strcmp(buf, "OK\0") == 0)) return 0;
	else return -1;
}

