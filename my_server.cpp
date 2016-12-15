//============================================================================
// Name        : my_server.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Server part of mycron
//============================================================================
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <crypt.h>
#define BUF_SIZE 8128

using namespace std;

int stop_server();
int execute_command_localhost(char *);
char *make_normal_current_time(char *, int);
int last_log_send(int);
int verify_client(int, char *);

int main(int argc, char *argv[]) {



	if (argc != 2) {
		printf("Usage: %s port OR %s cmd\n", argv[0], argv[0]);
	}

	if ((strcmp(argv[1], "stop\0") == 0)) {
			if (-1 == stop_server()) {
				printf("Not stopped: error\n");
				return -1;
			}
			else printf("Stopped\n");
			return 0;
	}

	FILE* hash_f = fopen("hash.txt", "r");
	if (hash_f == NULL) {
		printf("NO PASSWD!\n");
		return -1;
	}
	char hash[100];
	fgets(hash, 100, hash_f);

	int list_sock = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server;

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(atoi(argv[1]));

	if (-1 == bind(list_sock, (struct sockaddr *)&server, sizeof (server))) {
		perror("Error binding");
		return -1;
	}

	if (-1 == listen(list_sock, 10)) {
		perror("Error listening");
		return -1;
	}
	//making daemon
	/*close(0);
	close(1);
	close(2);
	if (0 != fork()) return 0;*/

	//to know pid for stopping
	FILE *pid_f = fopen ("pid.txt", "w");
	fprintf(pid_f,"%d\n", getpid());
	fclose(pid_f);
	unlink("localhost.log");
	while (1) {
		//collecting childs
		while (waitpid(-1, 0, WNOHANG) > 0);
		int cl_sock;
		struct sockaddr_in client;
		socklen_t addrlen= sizeof(client);
		if (-1 == (cl_sock = accept(list_sock, (struct sockaddr *) &client, &addrlen))) {
			return -1;
		}
		pid_t child_pid = fork();
		if (child_pid != 0) {
			close(cl_sock);
			continue;
		}
		close(list_sock);
		if (-1 == verify_client(cl_sock, hash)) {
			close(cl_sock);
			exit(EXIT_FAILURE);
		}
		char buf[BUF_SIZE];
		while (1) {
			if (-1 == recv(cl_sock, buf, BUF_SIZE, MSG_NOSIGNAL)) {
				//log
				close(cl_sock);
				exit(EXIT_FAILURE);
			}
			//printf("GOT: %s\n", buf);
			if (strcmp(buf, "GET\0") == 0) {
				last_log_send(cl_sock);
				continue;
			}

			if (strncmp(buf, "COMM\0", 4) == 0) {
				strcpy(buf, "OK\0");
				send(cl_sock, buf, strlen(buf)+1, MSG_NOSIGNAL);
				//printf("SENT: %s\n", buf);
				execute_command_localhost(buf + 5);
				continue;
			}

			if (strcmp(buf, "DISCONNECT\0") == 0) {
				strcpy(buf, "OK\0");
				send(cl_sock, buf, strlen(buf)+1, MSG_NOSIGNAL);
				//printf("SENT: %s\n", buf);
				shutdown(cl_sock, SHUT_RDWR);
				close(cl_sock);
				break;
			}

			close(cl_sock);
			exit(EXIT_FAILURE);
		}

		exit(EXIT_SUCCESS);
	}
	return 0;
}

/*
 * executed command locally, output is in localhost.log
 */
int execute_command_localhost(char *comm) {
	pid_t pid_child = fork();
	if (pid_child != 0) return 0;
	char s[30];
	int fd = open("localhost_full.log", O_WRONLY | O_APPEND | O_CREAT, 0700);
	FILE *short_log = fopen("localhost.log", "w");
	dup2(fd, 1);
	dup2(fd, 2);
	struct flock lock_s = {F_WRLCK, SEEK_SET, 0, 0, 0};
	fcntl(fd, F_SETLKW, &lock_s);
	fprintf(short_log, "[%s]: Executing command '%s'...\n", make_normal_current_time(s, 30), comm);
	int exec_stat;
	exec_stat = system(comm);
	fprintf(short_log, "[%s]: Execution ended with code %d\n", make_normal_current_time(s, 30), exec_stat);
	lock_s.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock_s);
	close(fd);
	if (exec_stat != 0) {
		FILE *err_log = fopen("error.log", "a");
		fprintf(err_log, "[%s]: Command '%s' at localhost returned %d\n",
				make_normal_current_time(s, 30), comm, exec_stat);
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
 * to stop server
 */
int stop_server() {

	FILE* pid_f = fopen("pid.txt", "r");
	pid_t pid;
	fscanf(pid_f, "%d", &pid);
	kill(pid, SIGTERM);

	return 0;
}

/*
 * last log sending
 */
int last_log_send(int fd) {
	char buf[BUF_SIZE];
	int file;
	if (-1 == (file = open("localhost.log", O_RDONLY))) {
		strcpy(buf, "NONE\0");
		send(fd, buf, strlen(buf)+1, MSG_NOSIGNAL);
		//printf("SENT: %s\n", buf);
		return -1;
	}
	int count = read(file, buf, 100);
	int count_2 = read(file, buf+count, 100);
	send(fd, buf, count+count_2, MSG_NOSIGNAL);
	unlink("localhost.log");
	return 0;
}

/*
 * checking passwd
 */
int verify_client(int fd, char *hash) {
	char buf[BUF_SIZE];
	int count;
	if (-1 == (count = recv(fd, buf, BUF_SIZE, MSG_NOSIGNAL))) {
		return -1;
	}
	buf[count] = '\0';
	char *hash_2 = crypt(buf, hash);
	if (strcmp(hash_2, hash) == 0) {
		strcpy(buf, "OK\0");
		send (fd, buf, strlen(buf)+1, MSG_NOSIGNAL);
		return 0;
	}
	return -1;
}
