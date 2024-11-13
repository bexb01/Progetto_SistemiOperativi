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


#include "include/shm_info.h"
#include "include/msg_comunication.h"
#include "../lib/semaphore.h"
struct stats { //struct stats è formata da puntatori a memoria condivisa
	shm_info_t *info; //*inf = (shm_info_t *)shm_attach(shm_id); questo si trova nella funzione shm_info_attach, grazie a questo
	                  // adesso il puntatore di tipo shm_info_t punta ad un area di memoria condivisa allocata e vuota di granezza
					  //shm_info_t 
};

int send_split_msg(int cargo_type);

void sigint_handler(int sig);

void close_and_exit();

void nsleep(long step_nsec);

struct stats stats;

int main(int argc, char *argv[]){
    int atomic_n_to_split;
	signal(SIGINT, sigint_handler);
	
	shm_info_attach(&stats.info);
	long step_nsec=shm_info_get_step_attivatore(stats.info);
	sem_execute_semop(shm_sem_get_startid(stats.info), 0, 1, 0);
	sem_execute_semop(shm_sem_get_startid(stats.info), 2, 1, 0);
	while(sem_getval(shm_sem_get_startid(stats.info), 1) != 1){

	}
	while(sem_getval(shm_sem_get_startid(stats.info), 7)>0){
		if(sem_getval(shm_sem_get_startid(stats.info), 2)>0 ){
			atomic_n_to_split=1;
			send_split_msg(atomic_n_to_split);
			nsleep(step_nsec);
		}
	}
	close_and_exit();
}

int send_split_msg(int atomic_n_rec){
	struct comunication_msg msg;
	pid_t activator_id = getpid();
	int bool_split = 1;
	msg = create_msg_comunication(atomic_n_rec,  activator_id, bool_split);
	msg_comunication_snd(msg_q_a_a_id_get(stats.info), &msg);
	return 0;
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