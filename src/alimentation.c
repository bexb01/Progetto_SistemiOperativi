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

struct stats { //struct stats è formata da puntatori a memoria condivisa
	//shm_n_atoms *atoms;
	//shm_energy *energy; //e  molte altre da implementare
	shm_info_t *info; //*inf = (shm_info_t *)shm_attach(shm_id); questo si trova nella funzione shm_info_attach, grazie a questo
	                  // adesso il puntatore di tipo shm_info_t punta ad un area di memoria condivisa allocata e vuota di granezza
					  //shm_info_t 
};

void init_atoms(void);

void sigint_handler(int sig);

pid_t run_process(char *name, int index);

void nsleep(long step_nsec);

void close_and_exit();
//struct data data; 

struct stats stats;

int main(int argc, char *argv[]){
	long step_nsec;
	signal(SIGCHLD, SIG_IGN);
	signal(SIGINT, sigint_handler); //settiamo sigint_handler come handler del segnale SIGINT
	shm_info_attach(&stats.info);
	sem_execute_semop(shm_sem_get_startid(stats.info), 0, 1, 0);
	//printf("semaphore processi alimentation: %d \n", sem_getval(shm_sem_get_startid(stats.info), 0));
	while(sem_getval(shm_sem_get_startid(stats.info), 1) != (long)1){

	}
	step_nsec = shm_info_get_step(stats.info);
	int n_new_atoms=shm_info_get_n_new_atoms(stats.info);
	sem_execute_semop(shm_sem_get_startid(stats.info), 2, 1, 0);
	while (sem_getval(shm_sem_get_startid(stats.info), 7)>0) {  //rimane in esecuzione
		//crea n new atoms
		//printf("alimentazione dice:sono vivooo.\n");
		init_atoms();
		nsleep(step_nsec);
	}
	//printf("terminazione del processo alimentation.\n");
	close_and_exit();

    // Questa parte del codice non verrà mai eseguita
    printf("Questa riga non verrà mai stampata.\n");
	
}

void init_atoms(void){
	int i, n_new_atoms;//n new atoms da leggerli in struct data data
	pid_t pid;
	//n_atoms= get_atoms(state.general); //legge da struct state campo general in cui ci ha salvato i dati  a noi deve leggere campo N_NUOVI_ATOMI
	n_new_atoms=shm_info_get_n_new_atoms(stats.info); //a scopo di debugging iniziamo con 1 nuovi atomi 
	for(i=0; (i< n_new_atoms) && (sem_getval(shm_sem_get_startid(stats.info), 7)>0); i++){
		pid= run_process("./atom", i);
		//shm_port_set_pid(state.ports, i, pid);//penso scriva in memoria condivisa i dati dei porti nel suo caso, degli atomi nel nostro, non so se ci serve
	}
}

pid_t run_process(char *name, int index){ // cre il figlio con fork() e lo trasforma in un altro processo, specificato da name, attraverso execve
	pid_t process_pid;
	//int atomic_n =35; //[DA CORREGGERE] 35 num atomico atomi creati da aimentatore 
	char *args[2], buf[10];   
	if ((process_pid = fork()) == -1) {  // fork restituisce 0 se pid figlio, 1 se padre, -1 errore
		//dprintf(2, "alimentation.c: Error in fork. MELTDOWN MELTDOWN MELTDOWN\n");//stampa u filedescriptor 2 che stampa su sdterr
		printf("MELTDOWN MELTDOWN MELTDOWN errore nella fork in alimentation\n");
		//blocco esecuzione
		sem_setval(shm_sem_get_startid(stats.info), 7, 0);
		close_and_exit();
	} else if (process_pid == 0) { // se figlio 
	    snprintf(buf, sizeof(buf), "%d", index);
        args[0] = name;
        args[1] = NULL; 
		if (execve(name, args, NULL) == -1) { //runno processo name attraverso execve, in teoria adesso no è piu figlio master ma è un processo "a parte"
			perror("execve");//stampa l'ultimo messaggio di errore
			exit(EXIT_FAILURE);   //errori execve
		}
	}

	return process_pid;
}

void nsleep(long step_nsec){
	struct timespec nsec, rem_nsec;
	nsec.tv_sec= 0;
	nsec.tv_nsec= step_nsec;
	nanosleep(&nsec, NULL); //senza gestione degli errori
	/*do{ //con gestione erroriò
		errno = EXIT_SUCCESS;
		nanosleep(&nsec, &rem_nsec);
		nsec=rem_nsec;
	}while (errno== EINTR)*/
}

void sigint_handler(int sig) {
	close_and_exit();
}

void close_and_exit(){
	sem_execute_semop(shm_sem_get_startid(stats.info), 2, -1, 0);
	//msg_queue_remove(stats.info); //la creazione e la rimozione delle risorse ipc la lasciamo fare esclusivamente la master
	shm_info_detach(stats.info);
 	//printf("terminazione del processo alimentation.\n");
	exit(0);
}
