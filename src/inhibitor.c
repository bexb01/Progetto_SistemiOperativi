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

void close_and_exit();
void handle_sigusr1(int sig);
void handle_sigusr2(int sig);
void sigint_handler(int sig);

struct stats stats;

int main(int argc, char *argv[]){
    //printf("CIAO SONO L'INIBITOREEEEEEEEEEEEEEEEEEEEEE\n");

	// Associa i gestori dei segnali per fermare e riprendere
    signal(SIGUSR1, handle_sigusr1);  // SIGUSR1 per fermare
    signal(SIGUSR2, handle_sigusr2);  // SIGUSR2 per riprendere
	signal(SIGINT, sigint_handler); //settiamo sigint_handler come handler del segnale SIGINT


	if(shm_info_attach(&stats.info)==-1){
		exit(EXIT_FAILURE);
	}
	shm_info_set_inhibitor_pid(stats.info, getpid());
	sem_setval(shm_sem_get_startid(stats.info), 6, 1); // questo indica che l'inibitore è attivo, quindi bisogn cmbiare il suo valore quando lo disaattivimo

	sem_execute_semop(shm_sem_get_startid(stats.info), 0, 1, 0);
	sem_execute_semop(shm_sem_get_startid(stats.info), 2, 1, 0);
	//sem_execute_semop(shm_sem_get_startid(stats.info), 10, -1, 0);

	while(sem_getval(shm_sem_get_startid(stats.info), 1) != 1){
		if(sem_getval(shm_sem_get_startid(stats.info), 7)==0){
			close_and_exit();
		}
		sleep(1);
	}


	while(sem_getval(shm_sem_get_startid(stats.info), 7)!=0){
		// Verifica se il processo è in esecuzione o fermo
        while (sem_getval(shm_sem_get_startid(stats.info), 6)==0) {  // Se fermato, aspetta fino a nuovo segnale
            pause();  // Sospende l'esecuzione finché non arriva un segnale
        }
        // Codice che simula il lavoro dell'inibitore (ciclo attivo)
        sleep(1);
	}
	close_and_exit();
}

void handle_sigusr1(int sig) {
    printf("Processo inibitore fermato (SIGUSR1 ricevuto)...\n");
    sem_execute_semop(shm_sem_get_startid(stats.info), 6, -1, 0);   // Imposta su -1 per fermare il ciclo
}

void handle_sigusr2(int sig) {
    printf("Processo inibitore ripreso (SIGUSR2 ricevuto)...\n");
    sem_execute_semop(shm_sem_get_startid(stats.info), 6, 1, 0) ;  // Imposta  su 1 per riprendere il ciclo
}

void sigint_handler(int sig) {
	close_and_exit();
}

void close_and_exit(){
	sem_execute_semop(shm_sem_get_startid(stats.info), 2, -1, 0);
	shm_info_detach(stats.info);

	printf("terminazione del processo inhibitor.\n");
	exit(0);

}