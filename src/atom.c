#define _GNU_SOURCE
#define MAX_ATOMS 5200

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
#include <fcntl.h>


#include "../lib/semaphore.h"
#include "include/msg_comunication.h"
#include "include/shm_info.h"
struct atom_n_parent_child{
	int atom_n_parent;
	int atom_n_child;
};

struct stats { //struct stats è formata da puntatori a memoria condivisa
	//shm_n_atoms *atoms;
	//shm_energy *energy; //e  molte altre da implementare
	shm_info_t *info; //*inf = (shm_info_t *)shm_attach(shm_id); questo si trova nella funzione shm_info_attach, grazie a questo
	                  // adesso il puntatore di tipo shm_info_t punta ad un area di memoria condivisa allocata e vuota di granezza
					  //shm_info_t 
};

int split(int atomic_n, int if_waste); //metodo per la scissione atomica

void update_energy(struct  atom_n_parent_child);

int energy(struct atom_n_parent_child);
struct  atom_n_parent_child atomic_n_to_split(int atomic_n);
int exit_n_sec(int n_seconds);
void close_and_exit();
int rcv_msg(int atomic_n);
void init_random();
int random_atomic_n(int max, int min);
void update_waste(int waste);
int ctrl_sem_getval(int sem_id, int sem_n);

int adaptive_probability(void);

struct stats stats;

int main(int argc, char *argv[]){
	int atomic_number;
	//int min_atomic_n = 24
    //printf("atomo creato.\n");
	if(shm_info_attach(&stats.info)==-1){
		exit(EXIT_FAILURE);
	}
	//printf("atomo %d ha effettuato attach alla mem condivisa \n", getpid());
	sem_execute_semop(shm_sem_get_startid(stats.info), 0, 1, 0);
	//printf("semaphore processi atom: %d\n", ctrl_sem_getval(shm_sem_get_startid(stats.info), 0));
	while(ctrl_sem_getval(shm_sem_get_startid(stats.info), 1) != 1){
		sleep(1);
	}
	int min_atomic_n = shm_info_get_min_n_atoms(stats.info);
	int max_atomic_n = shm_info_get_n_atom_max(stats.info);
	init_random();
    atomic_number=random_atomic_n(max_atomic_n,min_atomic_n);//dovrebbe essere random_atomic_n(max_atomic_n,min_atomic_n); ma non abbiamo semafori quindi non possiamo scrivere quanti atomi ci sono o muoiono in mem cond sesnza rischiare problemi di sincronizzaz
	//printf("numero atomico = %d \n", atomic_number);
	int i=0;
		sem_execute_semop(shm_sem_get_startid(stats.info), 2, 1, 0);
		//printf("NUMERO ATOMI RIMANENTI ORA = %d.\n", sem_getval(shm_sem_get_startid(stats.info), 2));
	int split_prob;
	
    //printf("atomo ha ricevuto num atomico che è %d.\n", atomic_number);
	while (ctrl_sem_getval(shm_sem_get_startid(stats.info), 7)>0){
		//printf("ancora vivo.\n");
		if(rcv_msg(atomic_number)){//ricezzione di un messaggio gli dice di fare scissione
			while(ctrl_sem_getval(shm_sem_get_startid(stats.info), 8)==0){
			}
				sem_execute_semop(shm_sem_get_startid(stats.info), 8, -1, 0);
				shm_info_set_n_activation_tot(stats.info, 1);//aumento attivazioni in MUTUA ESCLUSIONE
				sem_execute_semop(shm_sem_get_startid(stats.info), 8, 1, 0);
			
			if(atomic_number > min_atomic_n){
				//printf("%d \n", ctrl_sem_getval(shm_sem_get_startid(stats.info), 6));
				//srand(time(NULL));
				if(ctrl_sem_getval(shm_sem_get_startid(stats.info), 6)==1){
					split_prob=adaptive_probability();
					if(split_prob ==  -1){ //-1 blocco tutto, 1 splittowaste o blocco, 0 splitto
						//non faccio niente=blocco lo split
					}else if(split_prob ==  1){
						split_prob=adaptive_probability();
						if((split_prob == 1) || (split_prob == -1)){ //probabilità del secondo caso//split con waste o non split
							//blocchiamo split
						}else if(split_prob == 0){
							atomic_number=split(atomic_number, 1); //splittiamo con waste
						}
					}else if(split_prob ==  0){ //plit_prob==0
						//split senza waste
						atomic_number=split(atomic_number, 0); //gli passiamo n atomico padre
					}
					//printf("scissione avvenuta tramite messaggio da activator. ho numero atomico %d\n", atomic_number);
				}else{
					atomic_number=split(atomic_number, 0);
				}
			}else{
				update_waste(1);
				close_and_exit();
			}
		}else{
			sleep(1);
		}
	}
	close_and_exit();
}

int split(int atomic_n, int if_waste){//crea atomo figlio + setta il numero atomico del padre e figlio come atomic_n/2, magari dovrebbe salvare anche i pid nella mem condivisa? lo faremo se serve
	//[da aggiungere mem cond] se il num atomico è abbastanza grande da essere splittato allora forkiamo
	int p_c_pipe[2];//mi servono per la comunicazione atomo parent-child, forse da mettere dentro split()? anche no
	if (pipe(p_c_pipe) == -1) {
		perror("pipe open err.\n");
        	exit(EXIT_FAILURE); //oppure lo gestiamo diversamente
	}
	pid_t process_pid;
	if ((process_pid = fork()) == -1) {  // se errore
		dprintf(2, "atom.c: Error in fork MELTDOWN MELTDOWN MELTDOWN .\n");//gestione errore fork= meltdown
		printf("MELTDOWN MELTDOWN MELTDOWN errore nella fork in atom\n");
		//blocco esecuzione
		sem_setval(shm_sem_get_startid(stats.info), 7, 0);
		close_and_exit();
	} else if (process_pid == 0) { // se figlio
		
		if(shm_info_attach(&stats.info)==-1){
		exit(EXIT_FAILURE);
		} 
		
		//deve ricevere da padre il suo n atomico e aggiornare il num atom
		if (close(p_c_pipe[1]) == -1) {// Chiudo il lato di scrittura della pipe
			 perror("write pipe child err. current atom will be closed \n");
			 exit(EXIT_FAILURE);
    	}
		read(p_c_pipe[0], &atomic_n, sizeof(int));

			sem_execute_semop(shm_sem_get_startid(stats.info), 2, 1, 0);

			while(ctrl_sem_getval(shm_sem_get_startid(stats.info), 9)==0){
			}
				sem_execute_semop(shm_sem_get_startid(stats.info), 9, -1, 0);
				shm_info_set_n_split_tot(stats.info, 1);//aumento attivazioni in MUTUA ESCLUSIONE
				sem_execute_semop(shm_sem_get_startid(stats.info), 9, 1, 0);

			//printf("NUMERO ATOMI RIMANENTI ORA = %d.\n", sem_getval(shm_sem_get_startid(stats.info), 2));

        //printf("figlio ha ricevuto natomico dal padre atomic_n = %d.\n", atomic_n);

		if (close(p_c_pipe[0]) == -1) {  // Chiudo il lato di lettura della pipe
       		perror("read pipe child err. current atom will be closed\n");
    		exit(EXIT_FAILURE);
    	}
		if(if_waste==1){
			update_waste(1);
			close_and_exit();
		}
		return atomic_n;
	} else {//padre
		//invia al figlio il num atomico e aggiorna il proprio+calcolare energia liberata? oppure nel main
        struct  atom_n_parent_child split_atom_n = atomic_n_to_split(atomic_n);
		atomic_n = split_atom_n.atom_n_parent;
		if (close(p_c_pipe[0]) == -1) {  // Chiudo il lato di lettura della pipe
        	perror("read pipe parent err. current atom will be closed\n");
            exit(EXIT_FAILURE);
    	}
		write(p_c_pipe[1], &split_atom_n.atom_n_child, sizeof(int));

        //printf("padre ha inviato natomico al figlio &split_atom_n.atom_n_child= %d .\n", split_atom_n.atom_n_child);

		if (close(p_c_pipe[1]) == -1) {// Chiudo il lato di scrittura della pipe
       		perror("write pipe parent err. current atom will be closed\n");
        	exit(EXIT_FAILURE);
    	}
			//calcolo energia+aggiornamento  fa solo il padre
		update_energy(split_atom_n);
		return atomic_n;
	}
}

// Funzione per calcolare la probabilità adattiva
int adaptive_probability() {
	int active_process=ctrl_sem_getval(shm_sem_get_startid(stats.info), 2);
    if (active_process >= MAX_ATOMS) {
        return -1; // Blocco totale se abbiamo raggiunto il limite
    }
    double probability = (double)active_process / MAX_ATOMS; // Probabilità adattiva
	double random_value = (((double)rand() / RAND_MAX) * 0.9); // Numero casuale tra 0 e 0.
	//printf("%lf \n", probability-random_value);
    if (random_value < probability) {
        return 1; // indica che o scissione+waste oppure blocca scissione/ nel secondo caso significa che blocca scissione
    }else{
    return 0; // Indica che la scissione può avvenire/ nel secondo caso indica che scinde+waste
	}
}


struct  atom_n_parent_child atomic_n_to_split(int atomic_n){//splitto sempre per 2, se dispari padre difetto e figlio eccesso
	if((atomic_n % 2)==0){
	      	struct atom_n_parent_child parent_child;
	      	parent_child.atom_n_parent = atomic_n/2;
	      	parent_child.atom_n_child = atomic_n/2;
	      	return parent_child;
	}else {
	   	struct atom_n_parent_child parent_child;
	      	parent_child.atom_n_parent = (atomic_n-1)/2;
	      	parent_child.atom_n_child = (atomic_n+1)/2;
	      	return parent_child;
	}
}
void update_waste(int waste){// aggiorna le scorie in mem condivisa
	while(ctrl_sem_getval(shm_sem_get_startid(stats.info), 5)==0){
	}
	sem_execute_semop(shm_sem_get_startid(stats.info), 5, -1, 0);
	waste = waste+ shm_info_get_waste_tot(stats.info);
	shm_info_set_waste_tot(stats.info, waste);         //aggiorna mem condivisa in mutua escl
	sem_execute_semop(shm_sem_get_startid(stats.info), 5, 1, 0);
}   

void update_energy(struct  atom_n_parent_child p_c){
	int energy_val;
	energy_val=energy(p_c);
	while((ctrl_sem_getval(shm_sem_get_startid(stats.info), 3)==0) && sem_getval(shm_sem_get_startid(stats.info), 4)==0){
	}
	sem_execute_semop(shm_sem_get_startid(stats.info), 3, -1, 0);
	sem_execute_semop(shm_sem_get_startid(stats.info), 4, -1, 0);
	energy_val=energy_val+shm_info_get_energy_prod_tot(stats.info);
	int explode;
	if(energy_val > (explode=(shm_info_get_energy_explode_trashold(stats.info)))){
		printf("EXPLODE - EXPLODE - EXPLODE l'energia totale al netto di quella consumata dal master: %d è maggiore del parametro massimo %d\n",energy_val , explode);
		//bloccare l'esecuzione
		sem_setval(shm_sem_get_startid(stats.info), 7, 0);
		close_and_exit();
	}else{
		shm_info_set_energy_prod(stats.info, energy(p_c));
		shm_info_set_energy_prod_tot(stats.info, energy_val);         //aggiorna mem condivisa in mutua escl
		sem_execute_semop(shm_sem_get_startid(stats.info), 3, 1, 0);
		sem_execute_semop(shm_sem_get_startid(stats.info), 4, 1, 0);
	}
}

int energy(struct atom_n_parent_child p_c){
	int temp= p_c.atom_n_parent*p_c.atom_n_child;
	if(p_c.atom_n_parent >= p_c.atom_n_child){
		temp=temp-p_c.atom_n_parent;
	}else{
		temp=temp-p_c.atom_n_child;
	}
	return temp;
}


int exit_n_sec(int n_seconds){
    printf("Il processo atomo verrà terminato dopo %d secondi.\n", n_seconds);

    // Aspetta n_secondi
    sleep(n_seconds);

    // Termina il processo
    printf("Terminazione del processo atomo dopo %d secondi.\n", n_seconds);
    //exit(0);
	return 0;

    // Questa parte del codice non verrà mai eseguita
    printf("Questa riga non verrà mai stampata.\n");
}

void init_random() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    unsigned int seed = ts.tv_nsec ^ getpid();
    srand(seed);
}

int random_atomic_n(int max, int min){
    // Genera un numero casuale compreso tra max e min
    return rand() % (max - min+ 1) + min;
}

int rcv_msg(int atomic_n){
	//struct comunication_msg msg_rcv;
	int msg_q_id = msg_q_a_a_id_get(stats.info);
	//printf("id coda messaggi atomo a cui attaccarsi = %d", msg_q_id);
	struct comunication_msg msg_rcv;
	msg_comunication_rcv(msg_q_id, atomic_n, &msg_rcv.sender, &msg_rcv.bool_split);
	return msg_rcv.bool_split;
}

int ctrl_sem_getval(int sem_id, int sem_n){
	int res;
	if((res=sem_getval(sem_id, sem_n))<0){
		close_and_exit();
	}
	return res;
}

void close_and_exit(){
	sem_execute_semop(shm_sem_get_startid(stats.info), 2, -1, 0);
	//msg_queue_remove(stats.info); //la creazione eS la rimozione delle risorse ipc la lasciamo fare esclusivamente la master
	shm_info_detach(stats.info);

	//printf("terminazione del processo ATOM.\n");
	exit(0);
}