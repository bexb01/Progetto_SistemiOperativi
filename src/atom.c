//include e define
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
/*struct statistics {
	//shared mem tutto quello che decideremo di metterci
}*/

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

//void signal_handler(int signal);//da impl

//void signal_handler_init(void);//da impl

int split(int atomic_n); //metodo per la scissione atomica

void update_energy(struct  atom_n_parent_child);

int energy(struct atom_n_parent_child);
struct  atom_n_parent_child atomic_n_to_split(int atomic_n);
int exit_n_sec(int n_seconds);
void close_and_exit();
int loop_rcv_msg(int atomic_n);

struct stats stats;

int main(int argc, char *argv[]){
	int atomic_number;
	int min_atomic_n = 24;
	int received;
    printf("atomo creato.\n");
	shm_info_attach(&stats.info);
	printf("atomo %d ha effettuato attach alla mem condivisa", getpid());
	//atomic_number = (int)strtol(argv[1], NULL, 10);
    atomic_number=55;
	int i=0;
    printf("atomo ha ricevuto num atomico che è %d.\n", atomic_number);
	/*if*/ while (atomic_number >= min_atomic_n){
		printf("ancora vivo.\n");
		if((received=loop_rcv_msg(atomic_number))){//ricezzione di un messaggio gli dice di fare scissione
			received=0;
			printf("atomo si sta per scindere.\n");
			atomic_number=split(atomic_number); //gli passiamo n atomico padre
    		printf("scissione avvenuta tramite messaggio da activator. ho numero atomico %d\n", atomic_number);
		}
    	i=i+1;
    } //else { //se numero atomico troppo piccolo per split allora va nelle scorie e il processa va spento, da implementare 
		printf("Terminazione del processo atomo, numero atomico insufficiente.\n");
    	//exit(0);
		close_and_exit();
	//}//se c'è hwile in teoria non arriviamo mai qui, se c'è if arriviamo sempre qui
	int n_sec = 15;
	exit_n_sec(n_sec);
}

int split(int atomic_n){//crea atomo figlio + setta il numero atomico del padre e figlio come atomic_n/2, magari dovrebbe salvare anche i pid nella mem condivisa? lo faremo se serve
	//[da aggiungere mem cond] se il num atomico è abbastanza grande da essere splittato allora forkiamo
	int p_c_pipe[2];//mi servono per la comunicazione atomo parent-child, forse da mettere dentro split()? anche no
	if (pipe(p_c_pipe) == -1) {
		perror("pipe open err.\n");
        	exit(EXIT_FAILURE); //oppure lo gestiamo diversamente
	}
	pid_t process_pid;
	if ((process_pid = fork()) == -1) {  // se errore
		dprintf(2, "atom.c: Error in fork.\n");//gestione errore fork= meltdown
		//close_all();//da implemantare
	} else if (process_pid == 0) { // se figlio 
		//deve ricevere da padre il suo n atomico e aggiornare il num atom
		if (close(p_c_pipe[1]) == -1) {// Chiudo il lato di scrittura della pipe
			 perror("close write pipe child err\n");
			 exit(EXIT_FAILURE);
    	}
		read(p_c_pipe[0], &atomic_n, sizeof(int));

        //printf("figlio ha ricevuto natomico dal padre atomic_n = %d.\n", atomic_n);

		if (close(p_c_pipe[0]) == -1) {  // Chiudo il lato di lettura della pipe
       		perror("close read pipe child err\n");
        	exit(EXIT_FAILURE);
    	}
		return atomic_n;
	} else {//padre
		//invia al figlio il num atomico e aggiorna il proprio+calcolare energia liberata? oppure nel main
        struct  atom_n_parent_child split_atom_n = atomic_n_to_split(atomic_n);
		atomic_n = split_atom_n.atom_n_parent;
		if (close(p_c_pipe[0]) == -1) {  // Chiudo il lato di lettura della pipe
        	perror("close read pipe parent err\n");
            exit(EXIT_FAILURE);
    	}
		write(p_c_pipe[1], &split_atom_n.atom_n_child, sizeof(int));

        //printf("padre ha inviato natomico al figlio &split_atom_n.atom_n_child= %d .\n", split_atom_n.atom_n_child);

		if (close(p_c_pipe[1]) == -1) {// Chiudo il lato di scrittura della pipe
       		perror("close write pipe parent err\n");
        	exit(EXIT_FAILURE);
    	}
			//calcolo energia+aggiornamento  fa solo il padre
		update_energy(split_atom_n);
		return atomic_n;
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
	      	
void update_energy(struct  atom_n_parent_child p_c){
	int energy_val;
	energy_val=energy(p_c);
	//aggiornare memoria condivisa delle statistiche del master con l'energia (o magari bisogna inviare un messaggio al master con l'energia e lui l'aggiorna? bho)
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

int loop_rcv_msg(int atomic_n){
	//struct comunication_msg msg_rcv;
	int msg_q_id = msg_q_a_a_id_get(stats.info);
	printf("id coda messaggi atomo a cui attaccarsi = %d", msg_q_id);
	struct comunication_msg msg_rcv;
	msg_comunication_rcv(msg_q_id, atomic_n, &msg_rcv.sender, &msg_rcv.bool_split);
	return msg_rcv.bool_split;
}

void close_and_exit(){
	//msg_queue_remove(stats.info); //la creazione e la rimozione delle risorse ipc la lasciamo fare esclusivamente la master
	shm_info_detach(stats.info);

	exit(0);
}