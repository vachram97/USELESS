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
#include <ctype.h>
#include <crypt.h>
#include <errno.h>
#include <string>
#define BUF_SIZE 8128

using namespace std;

int stop_serv();
int execute_command_localhost(char *);
char *make_normal_current_time(char *, int);
int last_log_send(int);
int verify_client(int, char *);

int main(int argc, char *argv[]) {

	if (argc != 2) {
		printf("Usage: %s port OR %s -s\n", argv[0], argv[0]);
		return 0;
	}

	int opt;
	while (-1 != (opt = getopt(argc, argv, "s"))) {
		if (opt == '?') {
			printf ("Usage: %s mycronfile hostfile\n", argv[0]);
			return -1;
		}
		//-s stop option
		if ((opt == 's') && (argc == 2)) {
			if (-1 == stop_serv()) {
				printf("Not stopped\n");
				return -1;
			}
			printf("Stopped\n");
			return 0;
		}
		printf ("Usage: %s mycronfile hostfile\n", argv[0]);
		return -1;
	}
	
	char passwd[20];
	
	FILE *passwd_f = fopen("passwd.txt", "r+");
	if (passwd_f == NULL){
		perror("NO PASSWD FILE");
		return -1;
	}
	char c;
	//reading passwd while non_alpha and non_number char had read
	for (int i = 0; i < 20; i++) {
		c = fgetc(passwd_f);
		if (!isalnum(c)) {
			break;
		}
		passwd[i] = c;
	}
	fclose(passwd_f);
	
	//creating listening socket
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
	close(0);
	close(1);
	close(2);
	if (0 != fork()) return 0;

	//to know pid for stopping
	FILE *pid_f = fopen ("pid.txt", "w");
	fprintf(pid_f,"%d\n", getpid());
	fclose(pid_f);
	
	unlink("localhost.log");
	
	while (1) {
		//collecting childs
		while (waitpid(-1, 0, WNOHANG) > 0);
		//waiting for connection
		int cl_sock;
		struct sockaddr_in client;
		socklen_t addrlen= sizeof(client);
		if (-1 == (cl_sock = accept(list_sock, (struct sockaddr *) &client, &addrlen))) {
			return -1;
		}
		//creating process to work with client, the parent waits for another
		pid_t child_pid = fork();
		if (child_pid != 0) {
			close(cl_sock);
			continue;
		}
		close(list_sock);
		//checking passwd
		if (-1 == verify_client(cl_sock, passwd)) {
			close(cl_sock);
			exit(EXIT_FAILURE);
		}
		char buf[BUF_SIZE];
		while (1) {
			//receiving command from client
			if (-1 == recv(cl_sock, buf, BUF_SIZE, MSG_NOSIGNAL)) {
				close(cl_sock);
				exit(EXIT_FAILURE);
			}
			//getting logfile
			if (strcmp(buf, "GETLOG") == 0) {
				last_log_send(cl_sock);
				continue;
			}
			//command execution
			if (strncmp(buf, "COMM", 4) == 0) {
				puts("lalal");
				strcpy(buf, "OK");
				send(cl_sock, buf, strlen(buf)+1, MSG_NOSIGNAL);
				execute_command_localhost(buf + 5);
				continue;
			}
			//to disconnect
			if (strcmp(buf, "DISCONNECT") == 0) {
				strcpy(buf, "OK");
				send(cl_sock, buf, strlen(buf)+1, MSG_NOSIGNAL);
				shutdown(cl_sock, SHUT_RDWR);
				close(cl_sock);
				break;
			}
			//if no match for command found, the connection would be closed
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

	//creating process for executing
	pid_t pid_child = fork();
	if (pid_child != 0) return 0;
	char s[30];
	FILE *short_log = fopen("localhost.log", "a");//logfile
	fprintf(short_log, "[%s]: Executing command '%s'...\n", make_normal_current_time(s, 30), comm);
	int exec_stat;
	fclose(short_log);//to avoid spamming from system() in fd = 1,2
	exec_stat = system(comm);
	//opening log one more time for logging
	short_log = fopen("localhost.log", "a");
	fprintf(short_log, "[%s]: Execution of '%s' ended with code %d\n", make_normal_current_time(s, 30), comm, exec_stat);
	fclose(short_log);
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
 * last log sending
 */
int last_log_send(int fd) {
	char buf[BUF_SIZE];
	int file;
	//if no logfile found
	if (-1 == (file = open("localhost.log", O_RDONLY))) {
		strcpy(buf, "NONE\0");
		send(fd, buf, strlen(buf)+1, MSG_NOSIGNAL);
		return -1;
	}
	ssize_t count;
	//sending file
	while (0 < (count = read(file, buf, BUF_SIZE))) {
		if (-1 == send(fd, buf, count, MSG_NOSIGNAL)) break;
	}
	shutdown(fd, SHUT_RDWR);
	close(file);
	close(fd);
	exit(0);
}

/*
 * checking passwd (CHAP)
 */
int verify_client(int fd, char *passwd) {
	char buf[BUF_SIZE];
	int count;
	int chall_length = 8;
	
	//receiving request
	if (-1 == recv(fd, buf, BUF_SIZE, MSG_NOSIGNAL)) {
		return -1;
	}
	
	if (strcmp(buf, "CONNECT") != 0) {
		return -1;
	};
	
	//generating challenge
	string challenge;
	srand(time(NULL));
	for (int i = 0; i < chall_length; i++) {
		challenge += ((char)rand()%26 + 'a');
	}
	
	if (-1 == send(fd, challenge.c_str(), challenge.length() + 1, MSG_NOSIGNAL)) {
		return -1;
	}
	//making salt and calc hash
	string salt = "$6$" + challenge + "$";
	char *hash_local = crypt(passwd, salt.c_str());
	
	if (-1 == (count = recv(fd, buf, BUF_SIZE, MSG_NOSIGNAL))) {
		return -1;
	}
	buf[count] = '\0';
	//comparing it with saved

	if (strcmp(hash_local, buf) == 0) {
		strcpy(buf, "OK\0");
		send (fd, buf, strlen(buf)+1, MSG_NOSIGNAL);
		return 0;
	}
	return -1;
}

 /*
  * to stop daemon
  */
int stop_serv() {
	FILE* pid_f = fopen("pid.txt", "r");
	pid_t pid;
	fscanf(pid_f, "%d", &pid);
	//send signal
	kill(pid, SIGTERM);
	sleep(1);
	//checking if process was killed
	if (-1 == kill(pid, 0)) {
		if (errno == ESRCH) return 0;
	}
	return -1;
}
