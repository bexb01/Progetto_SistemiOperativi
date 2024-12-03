#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "include/shm_info.h"
#include "../lib/semaphore.h"

struct stats { 
	shm_info_t *info;
};

void init_atoms(void);

void sigint_handler(int sig);

void run_process(char *name);

void nsleep(long step_nsec);

void close_and_exit();

struct stats stats;

int main(int argc, char *argv[]){
	long step_nsec;
	signal(SIGCHLD, SIG_IGN);
	signal(SIGINT, sigint_handler);
	shm_info_attach(&stats.info);
	sem_execute_semop(shm_sem_get_startid(stats.info), 0, 1, 0);
	while(sem_getval(shm_sem_get_startid(stats.info), 1) != (long)1){
		sleep(1);
	}
	step_nsec = shm_info_get_step(stats.info);	//ogni quanti nano secondi creare n atomi
	sem_execute_semop(shm_sem_get_startid(stats.info), 2, 1, 0);
	while (sem_getval(shm_sem_get_startid(stats.info), 7)>0) { 
		init_atoms();
		nsleep(step_nsec);
	}
	close_and_exit();
}

void init_atoms(void){
	int i, n_new_atoms;
	n_new_atoms=shm_info_get_n_new_atoms(stats.info);
	for(i=0; (i< n_new_atoms) && (sem_getval(shm_sem_get_startid(stats.info), 7)>0); i++){
		run_process("./atom");
	}
}

void run_process(char *name){
	pid_t process_pid;
	char *args[2];   
	if ((process_pid = fork()) == -1) {
		printf("------ MELTDOWN: errore nella fork in alimentation ------\n");
		sem_setval(shm_sem_get_startid(stats.info), 7, 0);
		close_and_exit();
	} else if (process_pid == 0) {
        args[0] = name;
        args[1] = NULL; 
		if (execve(name, args, NULL) == -1) {
			perror("execve");
			exit(EXIT_FAILURE);
		}
	}
}

void nsleep(long step_nsec){
	struct timespec nsec, rem_nsec;
	nsec.tv_sec= 0;
	nsec.tv_nsec= step_nsec;
	nanosleep(&nsec, NULL); 
}

void sigint_handler(int sig) {
	close_and_exit();
}

void close_and_exit(){
	sem_execute_semop(shm_sem_get_startid(stats.info), 2, -1, 0);
	shm_info_detach(stats.info);
	exit(0);
}
