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

struct stats { //struct stats è formata da puntatori a memoria condivisa
	//shm_n_atoms *atoms;
	//shm_energy *energy; //e  molte altre da implementare
	shm_info_t *info; //*inf = (shm_info_t *)shm_attach(shm_id); questo si trova nella funzione shm_info_attach, grazie a questo
	                  // adesso il puntatore di tipo shm_info_t punta ad un area di memoria condivisa allocata e vuota di granezza
					  //shm_info_t 
};

struct stats stats;

int main(int argc, char *argv[]){
    printf("CIAO SONO L'INIBITOREEEEEEEEEEEEEEEEEEEEEE\n");

	if(shm_info_attach(&stats.info)==-1){
		exit(EXIT_FAILURE);
	}

	while(sem_getval(shm_sem_get_startid(stats.info), 1) != 1){
		sleep(1);
	}

	sem_execute_semop(shm_sem_get_startid(stats.info), 6, 1, 0);
}