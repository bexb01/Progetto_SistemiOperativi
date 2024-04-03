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
	int n_seconds=2;
	shm_info_attach(&stats.info);//dobbiamo crearla in master, questo serve solo per fare attach, nel master fa create+ attach
	printf("shm attaccata activator.");
	int i =0;
	sleep_n_sec(10);
	//while(1){
		
		atomic_n_to_split=1;//adesso non c'è piu bisogno di specificare il numero atomico= tipo di messaggio
		send_split_msg(atomic_n_to_split);
		send_split_msg(atomic_n_to_split);
		//printf("inviato messaggio split 55.\n");
		sleep_n_sec(n_seconds);
		//atomic_n_to_split=27;
		send_split_msg(atomic_n_to_split);
		send_split_msg(atomic_n_to_split);
		//printf("inviato messaggio split 27.\n");
		sleep_n_sec(n_seconds);
		//atomic_n_to_split=55;
		send_split_msg(atomic_n_to_split);
		send_split_msg(atomic_n_to_split);
		send_split_msg(atomic_n_to_split);
		sleep_n_sec(n_seconds);
		//atomic_n_to_split=28;
		send_split_msg(atomic_n_to_split);
		send_split_msg(atomic_n_to_split);
		//printf("inviato messaggio split 28.\n");	
		sleep_n_sec(n_seconds);
		//atomic_n_to_split=27;
		send_split_msg(atomic_n_to_split);
		send_split_msg(atomic_n_to_split);
		send_split_msg(atomic_n_to_split);
		sleep_n_sec(n_seconds);
		//atomic_n_to_split=28;
		send_split_msg(atomic_n_to_split);
		send_split_msg(atomic_n_to_split);
		send_split_msg(atomic_n_to_split);
		sleep_n_sec(n_seconds);
		//atomic_n_to_split=55;
		send_split_msg(atomic_n_to_split);
		//atomic_n_to_split=28;
		send_split_msg(atomic_n_to_split);
		//atomic_n_to_split=27;
		send_split_msg(atomic_n_to_split);
		i=i+1;
	//}
	void close_and_exit();
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
    printf("Il processo activator dormirà %d secondi.\n", n_seconds);

    // Aspetta n_secondi
    sleep(n_seconds);

    // Termina il processo
    printf("activator si è svegliato.\n");
}

void close_and_exit(){
	//msg_queue_remove(stats.info);
	shm_info_detach(stats.info);

	exit(0);
}