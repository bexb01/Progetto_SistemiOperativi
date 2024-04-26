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
	//shm_n_atoms *atoms;
	//shm_energy *energy; //e  molte altre da implementare
	shm_info_t *info; //*inf = (shm_info_t *)shm_attach(shm_id); questo si trova nella funzione shm_info_attach, grazie a questo
	                  // adesso il puntatore di tipo shm_info_t punta ad un area di memoria condivisa allocata e vuota di granezza
					  //shm_info_t 
};

int send_split_msg(int cargo_type);

void close_and_exit();

int sleep_n_sec(int n_seconds);

struct stats stats;

int main(int argc, char *argv[]){
    int atomic_n_to_split;
	
	shm_info_attach(&stats.info);//dobbiamo crearla in master, questo serve solo per fare attach, nel master fa create+ attach
	printf("shm attaccata activator.\n");
	int n_seconds=shm_info_get_step_attivatore(stats.info);
	sem_execute_semop(shm_sem_get_startid(stats.info), 0, 1, 0);
	printf("semaphore processi activator: %d\n", sem_getval(shm_sem_get_startid(stats.info), 0));
	while(sem_getval(shm_sem_get_startid(stats.info), 1) != 1){

	}
	//sleep_n_sec(10);
	sem_execute_semop(shm_sem_get_startid(stats.info), 2, 1, 0);
	while(sem_getval(shm_sem_get_startid(stats.info), 7)>0){
		if(sem_getval(shm_sem_get_startid(stats.info), 2)>0 ){
			atomic_n_to_split=1;//adesso non c'è piu bisogno di specificare il numero atomico= tipo di messaggio
			sleep_n_sec(n_seconds);
			send_split_msg(atomic_n_to_split);
			send_split_msg(atomic_n_to_split);
			send_split_msg(atomic_n_to_split);
			send_split_msg(atomic_n_to_split);
		//printf("inviato messaggio split 55.\n");
		}
	}
	close_and_exit();
}

int send_split_msg(int atomic_n_rec){
	struct comunication_msg msg;
	pid_t activator_id = getpid();
	int bool_split = 1;
	msg = create_msg_comunication(atomic_n_rec,  activator_id, bool_split);
	msg_comunication_snd(msg_q_a_a_id_get(stats.info), &msg); // do indirizzo della struct msg da inviare e col getter della coda messaggi ricevo id passandogli il valore del puntatore inf che è l'indirizzo della struttura shm, di cui ho fatto l'attach nel main, poi la funz chiamata lo mette in un puntatore per accedere all'id
	//msg_commerce_receive(msg_out_get_id(state.general), state.id, NULL, NULL, &quantity, NULL, &status, TRUE);

	/*if (status == STATUS_ACCEPTED && quantity > 0) {
		return ship_sell(quantity, cargo_type);
	}*/
	return 0;
}

int sleep_n_sec(int n_seconds){//solo per rallentare il processo e vedere se funziona tutto 
    //printf("Il processo activator dormirà %d secondi.\n", n_seconds);

    // Aspetta n_secondi
    sleep(n_seconds);

    // Termina il processo
    printf("activator si è svegliato.\n");
}

void close_and_exit(){
	sem_execute_semop(shm_sem_get_startid(stats.info), 2, -1, 0);
	//msg_queue_remove(stats.info);
	shm_info_detach(stats.info);

	printf("terminazione del processo ACTIVATOR.\n");
	exit(0);
}