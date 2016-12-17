/*
 ============================================================================
 Name        : mycron.c
 Author      : vachram
 Version     : 1.0
 Copyright   : No copyright
 Description : Client part of mycron
 ============================================================================
 */

#include "includes.h"
#include "reader.h"
#include "execution.h"

void run_cron(vector<command> *, vector<task> *, vector<host> *);
int stop_serv();
int get_logs();
int get_log_host(host *);
int time_cmp(struct tm*, struct tm *);

int main (int argc, char *argv[]) {

	char mycron_path[PATH_MAX]; //path of mycrontabfile
	char server_path[PATH_MAX]; //path of hosttab file
	vector <command> command_list; //command list
	vector <task> task_queue;
	vector <host> hosts; //server list

	if (argc == 1) {
		printf ("Usage: %s mycronfile hostfile\n", argv[0]);
		return 0;
	}

	int opt;
	while (-1 != (opt = getopt(argc, argv, "sr"))) {
		if (opt == '?') {
			printf ("Usage: %s mycronfile hostfile\n", argv[0]);
			return -1;
		}
		if ((opt == 's') && (argc == 2)) {
			if (-1 == stop_serv()) {
				printf("Not stopped\n");
				return -1;
			}
			printf("Stopped\n");
			return 0;
		}
		if ((opt == 'r') && (argc == 2)) {
			get_logs();
			return 0;
		}
		printf ("Usage: %s mycronfile hostfile\n", argv[0]);
		return -1;
	}
	if (optind + 2 > argc) {
		printf ("Usage: %s mycronfile hostfile\n", argv[0]);
		return -1;
	}
	strcpy(mycron_path, argv[optind]);
	strcpy(server_path, argv[optind+1]);
	//reading mycrontab
	if (-1 == read_tasks(mycron_path, &command_list, &task_queue)) {
		printf("Reading mycrontab failed\n");
		return -1;
	}

	//reading hosttab
	if (-1 == read_server_list(server_path, &hosts)) {
		printf("Reading hosttab failed\n");
		return -1;
	}
	//daemon
	/*close (0);
	close (1);
	close (2);
	if (0 != fork()) return 0;*/

	//to know pid for stopping
	FILE *pid_f = fopen ("pid.txt", "w");
	fprintf(pid_f,"%d\n", getpid());
	fclose(pid_f);

	FILE *host_f = fopen ("host_path.txt", "w");
	fprintf(host_f, "%s", server_path);
	fclose(host_f);

	run_cron(&command_list, &task_queue, &hosts);
	return 0;
}


void run_cron (vector<command> *comm_list, vector<task> *task_list, vector<host> *host_list){

	struct tm curr_time, next_exec_time;
	time_t curr_time_t = time(NULL);
	time_t next_exec_time_t;

	localtime_r(&curr_time_t, &curr_time);
	localtime_r(&curr_time_t, &next_exec_time);

	vector<task>::iterator next_task = (*task_list).begin();

	while (time_cmp(&curr_time, &next_task->time) == 0) {
		//putting off launch at next year
		next_task->time.tm_year++;
		next_task++;

		if (next_task == (*task_list).end()) {
			next_task = (*task_list).begin();
			break;
		}
	}


	while (1) {
		//collecting childs
		while (waitpid(-1, 0, WNOHANG) > 0);

		//sleeping
		next_exec_time_t = mktime(&(next_task->time));
		long int wait_sec = next_exec_time_t - time(NULL);

		sleep(wait_sec);

		//we have a task to do
		while (mktime(&next_task->time) - next_exec_time_t == 0) {
			//at localhost
			if ((*comm_list).at(next_task->number).host == "localhost") {
				//puts("exe loc");
				execute_command_localhost(&(*comm_list).at(next_task->number));
				next_task->time.tm_year++;
				next_task++;
				if (next_task == (*task_list).end())next_task = (*task_list).begin();
				continue;
			}
			//at all hosts
			if ((*comm_list).at(next_task->number).host == "all") {
				execute_command_localhost(&(*comm_list).at(next_task->number));
				for (unsigned int i = 0; i < (*host_list).size(); i++) {
					execute_command(&(*comm_list).at((*next_task).number), &(*host_list).at(i));
				}
				next_task->time.tm_year++;
				next_task++;
				if (next_task == (*task_list).end())next_task = (*task_list).begin();
				continue;
			}

			for (unsigned int i = 0; i < (*host_list).size(); i++) {
				if ((*host_list).at(i).name == (*comm_list).at(next_task->number).host) {
					//puts("Executing nonlocal\n");
					execute_command(&(*comm_list).at(next_task->number), &(*host_list).at(i));
					break;
				}
				if (i == (*host_list).size()) {
					logerr((*comm_list).at(next_task->number).host, "Not found in hosts\n");
				}
			}
			next_task->time.tm_year++;
			next_task++;
			if (next_task == (*task_list).end())next_task = (*task_list).begin();
		}
	}

	return;
}
 /*
  * to stop daemon
  */
int stop_serv() {
	FILE* pid_f = fopen("pid.txt", "r");
	pid_t pid;
	fscanf(pid_f, "%d", &pid);
	kill(pid, SIGTERM);
	sleep(1);
	if (-1 == kill(pid, 0)) {
		if (errno == ESRCH) return 0;
	}
	return -1;
}

int get_logs() {
	FILE *host_path_file = fopen("host_path.txt", "r");
	char host_path[PATH_MAX];
	fgets(host_path, PATH_MAX, host_path_file);
	fclose(host_path_file);
	vector <host> hosts;

	if (-1 == read_server_list(host_path, &hosts)) {
		printf("Reading hosttab failed\n");
		return -1;
	}

	for (unsigned int i = 0; i < hosts.size(); i++) {
		get_log_host(&hosts.at(i));
	}
	return 0;
}

int get_log_host(host *host) {
	char buf[BUF_SIZE];
	int cl_sock = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server;

	server.sin_family = AF_INET;
	server.sin_addr = host->addr;
	server.sin_port = host->port;

	ssize_t count;

	if (-1 == connect(cl_sock, (struct sockaddr *) &server, sizeof(server))) {
		logerr(host->name, "Unable to establish connection\n");
		close(cl_sock);
		exit(EXIT_FAILURE);
	}

	if (-1 == verify(cl_sock, host->passwd)) {
		printf("'%s': Connection rejected\n", host->name.c_str());
		return -1;
	}

	strcpy(buf, "GETLOG");
	if (-1 == send(cl_sock, buf, strlen(buf)+1, MSG_NOSIGNAL)) {
		printf("Error getting logs from %s: %s\n", host->name.c_str(), strerror(errno));
		return -1;
	}

	if (-1 == (count = recv(cl_sock, buf, BUF_SIZE, MSG_NOSIGNAL))) {
		printf("Error getting logs from %s: %s\n", host->name.c_str(), strerror(errno));
		return -1;
	}

	if (strcmp(buf, "NONE") == 0) {
		printf("NO LOGS AVAIABLE at %s\n", host->name.c_str());
		strcpy(buf, "DISCONNECT");
		send(cl_sock, buf, strlen(buf)+1, MSG_NOSIGNAL);
		shutdown(cl_sock, SHUT_RDWR);
		close(cl_sock);
		return 0;
	}
	string filename = host->name + ".log";
	int filelog = open (filename.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0700);
	write(filelog, buf, count);
	while (0 < (count = recv(cl_sock, buf, BUF_SIZE, MSG_NOSIGNAL))) {
		write(filelog, buf, count);
	}
	shutdown(cl_sock, SHUT_RDWR);
	close(cl_sock);
	return 0;

}

/*
 * returns 1 if second time is later than first
 */
int time_cmp(struct tm *first, struct tm *second) {
	int k = (second->tm_min - first->tm_min) +
			(second->tm_hour - first->tm_hour) * 100 +
			(second->tm_mday - first->tm_mday) * 10000 +
			(second->tm_mon - first->tm_mon) * 1000000;
	return (k > 0) ? 1 : 0;
}
