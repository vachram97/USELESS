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

int main (int argc, char *argv[]) {

	int curr_arg = 1; //number of current argument
	char mycron_path[PATH_MAX]; //path of mycrontabfile
	char server_path[PATH_MAX]; //path of hosttab file
	vector <command> command_list; //command list
	vector <task> task_queue;
	vector <host> hosts; //server list


	if (argc == 1) {
		printf ("Usage: %s mycronfile hostfile\n", argv[0]);
		return 0;
	}

	//keys
	if (strcmp(argv[1], "stop\0") == 0) {
		stop_serv();
		printf("Signal sent\n");
		return 0;
	}
	if (argc < curr_arg + 2) {
		printf("Usage:\n");
		return 0;
	}
	strcpy(mycron_path, argv[curr_arg]); //saving mycron file path to use it further
	strcpy(server_path, argv[curr_arg+1]); //saving hosttab file path to use it further

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
	time_s curr_time_s = {next_exec_time.tm_mon, next_exec_time.tm_mday, next_exec_time.tm_hour, next_exec_time.tm_min};
	while (time_s_cmp((*next_task).time, curr_time_s) > 0) {
		next_task++;
		if (next_task == (*task_list).end()) {
			next_task = (*task_list).begin();
			break;
		}
	}
	next_exec_time.tm_sec = 1;

	while (1) {
		//collecting childs
		while (waitpid(-1, 0, WNOHANG) > 0);
		fill_tm_using_time_s(&next_exec_time, &((*next_task).time));
		next_exec_time_t = mktime(&next_exec_time);
		if (next_exec_time_t - time(NULL) < -1) {
			next_exec_time.tm_year++;
			next_exec_time_t = mktime(&next_exec_time);
		}
		long int wait_sec = next_exec_time_t - time(NULL);
		sleep(wait_sec);
		//we have a task to do
		curr_time_s = (*next_task).time;
		while (is_equal_time_s(curr_time_s,(*next_task).time)) {
			//at localhost
			if ((*comm_list).at((*next_task).number).host == "localhost") {
				//puts("exe loc");
				execute_command_localhost(&(*comm_list).at((*next_task).number));
				next_task++;
				if (next_task == (*task_list).end())next_task = (*task_list).begin();
				continue;
			}
			//at all hosts
			if ((*comm_list).at((*next_task).number).host == "all") {
				execute_command_localhost(&(*comm_list).at((*next_task).number));
				for (unsigned int i = 0; i < (*host_list).size(); i++) {
					execute_command(&(*comm_list).at((*next_task).number), &(*host_list).at(i));
				}
				next_task++;
				if (next_task == (*task_list).end())next_task = (*task_list).begin();
				continue;
			}

			for (unsigned int i = 0; i < (*host_list).size(); i++) {
				if ((*host_list).at(i).name == (*comm_list).at((*next_task).number).host) {
					//puts("Executing nonlocal\n");
					execute_command(&(*comm_list).at((*next_task).number), &(*host_list).at(i));
					break;
				}
				if (i == (*host_list).size()) {
					logerr((*comm_list).at((*next_task).number).host, "Not found in hosts\n");
				}
			}
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

	return 0;
}
