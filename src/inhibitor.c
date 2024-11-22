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

#include "../lib/semaphore.h"
#include "include/msg_comunication.h"
#include "include/shm_info.h"

struct stats {
	shm_info_t *info;
};

void close_and_exit();
void handle_sigusr1(int sig);
void handle_sigusr2(int sig);
void sigint_handler(int sig);

struct stats stats;

int main(int argc, char *argv[]){
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);
	signal(SIGINT, sigint_handler);

	if(shm_info_attach(&stats.info)==-1){
		exit(EXIT_FAILURE);
	}
	shm_info_set_inhibitor_pid(stats.info, getpid());
	sem_setval(shm_sem_get_startid(stats.info), 6, 1);

	sem_execute_semop(shm_sem_get_startid(stats.info), 0, 1, 0);
	sem_execute_semop(shm_sem_get_startid(stats.info), 2, 1, 0);

	while(sem_getval(shm_sem_get_startid(stats.info), 1) != 1){
		if(sem_getval(shm_sem_get_startid(stats.info), 7)==0){
			close_and_exit();
		}
		sleep(1);
	}

	while(sem_getval(shm_sem_get_startid(stats.info), 7)!=0){
        while (sem_getval(shm_sem_get_startid(stats.info), 6)==0) {
            pause();
        }
        sleep(1);
	}
	close_and_exit();
}

void handle_sigusr1(int sig) {
    printf("Processo inibitore fermato (SIGUSR1 ricevuto)...\n");
    sem_execute_semop(shm_sem_get_startid(stats.info), 6, -1, 0);
}

void handle_sigusr2(int sig) {
    printf("Processo inibitore ripreso (SIGUSR2 ricevuto)...\n");
    sem_execute_semop(shm_sem_get_startid(stats.info), 6, 1, 0) ;
}

void sigint_handler(int sig) {
	close_and_exit();
}

void close_and_exit(){
	sem_execute_semop(shm_sem_get_startid(stats.info), 2, -1, 0);
	shm_info_detach(stats.info);
	exit(0);

}