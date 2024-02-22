//include e define
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

/*struct statistics {
	//shared mem tutto quello che decideremo di metterci
}*/

struct atom_n_parent_child{
	int atom_n_parent;
	int atom_n_child;
};

//void signal_handler(int signal);//da impl

//void signal_handler_init(void);//da impl

int split(int); //metodo per la scissione atomica

void update_energy(struct  atom_n_parent_child);

int energy(struct atom_n_parent_child);
struct  atom_n_parent_child atom_n_bef_split(int);
struct  atom_n_parent_child atomic_n_to_split(int atomic_n);
int exit_n_sec(int n_seconds);

//struct statistics stats;

int main(int argc, char *argv[]){
	int atomic_number;
    printf("atomo creato.\n");
    
	/*bzero(&state, sizeof(struct state));
	signal_handler_init();*/

	// riceviamo il numero atomico da parte di master oppure alimentation (lui lo mette in memoria condivisa noi per adesso in una var) state.id = (int)strtol(argv[1], NULL, 10);
	//atomic_number = (int)strtol(argv[1], NULL, 10);
    atomic_number=55;
    printf("atomo ha ricevuto num atomico da master che è %d.\n", atomic_number);
	//ricezzione di un segnale/un messaggio gli dice di fare scissione
	
	      //SCOMMENTARE PER SCINDERE
    int min_atomic_n = 20;
	/*if*/ while (atomic_number >= min_atomic_n){
    	printf("atomo si sta per scindere.\n");
		atomic_number=split(atomic_number); //gli passiamo n atomico padre
    	printf("scissione avvenuta.\n");
    } //else { //se numero atomico troppo piccolo per split allora va nelle scorie e il processa va spento, da implementare 
		printf("Terminazione del processo atomo, numero atomico insufficiente.\n");
    	exit(0);
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
			 perror("close write pipe child err");
			 exit(EXIT_FAILURE);
    	}
		read(p_c_pipe[0], &atomic_n, sizeof(int));	//

        printf("figlio ha ricevuto natomico dal padre atomic_n = %d.\n", atomic_n);

		if (close(p_c_pipe[0]) == -1) {  // Chiudo il lato di lettura della pipe
       		perror("close read pipe child err");
        	exit(EXIT_FAILURE);
    	}
        return atomic_n;
	} else {//padre
		//invia al figlio il num atomico e aggiorna il proprio+calcolare energia liberata? oppure nel main
        struct  atom_n_parent_child split_atom_n = atomic_n_to_split(atomic_n);
		atomic_n= split_atom_n.atom_n_parent;
		if (close(p_c_pipe[0]) == -1) {  // Chiudo il lato di lettura della pipe
        	perror("close read pipe parent err");
            exit(EXIT_FAILURE);
    	}
		write(p_c_pipe[1], &split_atom_n.atom_n_child, sizeof(int));

        printf("padre ha inviato natomico al figlio &split_atom_n.atom_n_child= %d .\n", split_atom_n.atom_n_child);

		if (close(p_c_pipe[1]) == -1) {// Chiudo il lato di scrittura della pipe
       		perror("close write pipe parent err");
        	exit(EXIT_FAILURE);
    	}
			//calcolo energia+aggiornamento  fa solo il padre
		update_energy(split_atom_n);
        return split_atom_n.atom_n_parent;
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
    exit(0);

    // Questa parte del codice non verrà mai eseguita
    printf("Questa riga non verrà mai stampata.\n");
}
