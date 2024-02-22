//aggiungere tutte le #define  e #include non so nemmeno se servono tutte queste
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

//#include "../path/to/your/shm_n_atoms.h" // Assicurati di sostituire il percorso corretto
//#include "../path/to/your/shm_energy.h" // Assicurati di sostituire il percorso corretto


//implementare una struct per la memoria condiviasa per le statistiche da stampare qualcosa del tipo

/*struct statistics{ mi sa che struct è formata da puntatori a memoria condivisa
	shm_n_atoms *atoms;
	shm_energy *energy; //e  molte altre da implementare
}*/

//prototipazione/ dichiarazione anticipata delle funzioni

//void signal_handler(int signal);//da imlementare(lo copio pari dal prog)

//void signal_handler_init(void); //da implementare(lo copio pari dal prog)

void init_atoms(void);

void init_activator(void);

void init_alimentation(void);

pid_t run_process(char *name, int index);

void exit_n_sec(int n_seconds);

void print_statistics(void);

void close_all(void);  // da implementare, per chiudere tutte le risorse utilizzate e i processi?

//struct statistics stats;//creo una variabile di tipo struct statistics chiamata stats

//avvia simulazione==avviare tutti i processi(penso)

int main(int argc, char *argv[]){

	init_atoms();
	//init_activator();
	init_alimentation();
	
	int n_sec = 20;
	exit_n_sec(n_sec); // gli do 35 sec di vita

	/*sem_execute_semop(sem_port_init_get_id(state.general), 0, 0, 0);
	sem_execute_semop(sem_start_get_id(state.general), 0, -1, 0);
	//due operazioni con semafori che servono a sincronizzare qualcosa

	alarm(1);

	while (1) {
		pause(); //questo ciclo aspetta indefinitamente fino a quando non riceve un segnale.
	}
	*/
//bho
}

//crea  atomi
void init_atoms(void){
	int i, n_atoms;
	pid_t pid;
	//n_atoms= get_atoms(state.general); //legge da struct state campo general in cui ci ha salvato i dati 
	n_atoms = 2; //a scopo di debugging iniziamo con 2 atomi 
	for(i = 0; i <  n_atoms; i++){
		pid= run_process("./atom", i);
		//shm_port_set_pid(state.ports, i, pid);//penso scriva in memoria condivisa i dati dei porti nel suo caso, degli atomi nel nostro, non so se ci serve
	}
}


//crea proc attivatore
void init_activator(void){
	pid_t pid;
	pid=run_process("./activator", 1); //  ./ prima del programma indica che si trova nella directory corrente quindi master e il codice dei processi attivati da master stanno nella stessa cartella
}

//crea alimentatore
void init_alimentation(void){
	pid_t pid;
	pid=run_process("./alimentation", 1); //dice a run process di runnare proc alimentatore
}


//stampe periodiche delle statistiche
void print_statistics(void){
// da implementare dopo aver implementato la memoria condivisa per la condivisione delle statistiche tra i processi
}

//runna il processo specificato da name
pid_t run_process(char *name, int index){ // cre il figlio con fork() e lo trasforma in un altro processo, specificato da name, attraverso execve
	pid_t process_pid;
	int atomic_n = 55; //[DA CORREGGERE]per adesso metto num atomico 55 per ogni atomo creato da master
	char *args[3], buf[10];   
	if ((process_pid = fork()) == -1) {  // fork restituisce 0 se pid figlio, 1 se padre, -1 errore
		dprintf(2, "master.c: Error in fork.\n");//stampa u filedescriptor 2 che stampa su sdterr
		//close_all();
	} else if (process_pid == 0) { // se figlio 
	      	//implementare funzione per il num atomico (casuale o distribuzione di prob a piacere), magari si puo mettere in una libreria? visto che deve usarlo si master che alimentatore
		//sprintf(buf, "%d", index);//scrive in buf l'id noi lo possiamo usare per inviare il numero atomico agli atomi appena creati
		
        /*sprintf(buf, "%d", atomic_n);	//[DA CORREGGERE]
		args[0] = name;  //nome del processo da inizializzare (atom, activator, alimentatore)
		args[1] = buf;   //DOBBIAMO METTERE IL NUMERO ATOMICO
		args[2] = NULL;  //fine args*/

		if (execve(name, args, NULL) == -1) { //runno processo name attraverso execve, in teoria adesso no è piu figlio master ma è un processo "a parte"
			perror("execve");//stampa l'ultimo messaggio di errore
			exit(EXIT_FAILURE);   //errori execve
		}
	}

	return process_pid;
}

/*void signal_handler_init(void) //copiato paripari dal prog e non mi va di vedere che fa
{
	static struct sigaction sa;
	sigset_t mask;

	bzero(&sa, sizeof(sa));
	sa.sa_handler = signal_handler;

	sigfillset(&mask);
	sa.sa_mask = mask;
	sigaction(SIGALRM, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
}*/

/*void signal_handler(int signal)//copiato pari pari dal progetto non mi va di cedere che fa
{
	switch (signal) {
	case SIGSEGV:
		dprintf(2, "master.c: Segmentation fault. Closing all.\n");
	case SIGTERM:
	case SIGINT:
		close_all();
	case SIGALRM:
		print_daily_report();
		if (check_ships_all_dead()) {
			dprintf(1, "All ships are dead. Terminating...\n");
			close_all();
		}
		if (get_current_day(state.general) + 1 == get_days(state.general) + 1) {
			dprintf(1,
				"Reached last day of simulation. Terminating...\n");
			close_all();
		}

		increase_day(state.general);
		shm_port_send_signal_to_all_ports(state.ports, state.general, SIGDAY);
		kill(state.weather, SIGDAY);
		alarm(1);
		break;
	default:
		break;
	}
}*/

// Function to close all resources
void close_all(void) {
    // Your cleanup code...
    // Close semaphores, shared memory, etc.
}

void exit_n_sec(int n_seconds){
    printf("Il processo master verrà terminato dopo %d secondi.\n", n_seconds);

    // Aspetta n_secondi
    sleep(n_seconds);

    // Termina il processo
    printf("Terminazione del processo master dopo %d secondi.\n", n_seconds);
    exit(0);

    // Questa parte del codice non verrà mai eseguita
    printf("Questa riga non verrà mai stampata.\n");
}