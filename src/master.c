//aggiungere tutte le #define  e #include non so nemmeno se servono tutte queste
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

//prototipazione/ dichiarazione anticipata delle funzioni

void init_atoms(void);

void init_activator(void);

void init_alimentation(void);

pid_t run_process(char *name, int index);

void exit_n_sec(int n_seconds);

void print_statistics(void);

void close_and_exit();

struct stats stats;//creo una variabile di tipo struct statistics chiamata stats

//avvia simulazione==avviare tutti i processi(penso)/sincronizzarli cioe farli partire dopo che tutti sono stati creati, magari con un semaforo

int main(int argc, char *argv[]){

	
	//shm_info_attach fa sia la get (cioè la creazione del  segmento di mem condivisa) che la attach
	shm_info_attach(&stats.info);//shm_info_attach richiede un puntatore a puntatore ma noi gli passiamo un indirizzo di un puntatore,
	                            // questo è possibile perche un punt a punt appunto contiene un indirizzo di un puntatore, quindi passare 
								//un punt a punt oppure l'indirizzo di un punt è disciamo la stessa cosa per il chiamanate, tanto poi 
								//la funzione chiamata salverà l'indirizzo che gli abbiamo passato nella sua istanza di puntatore a puntatore
	param_init("../config_param.txt", stats.info);
	msg_q_a_a_init(stats.info);	//gli passiamo il valore del puntatore *info che è l'indirizzo della structct shm_info_t che è dove salviamo l'id con la funzione				
	shm_sem_init(stats.info);
	init_atoms();
	init_alimentation();
	init_activator();
	printf("Attesa che tutti i processi figli vengano creati...\n");
	while(shm_sem_ready(stats.info)!= 0){// fino a quando semaforo non è ready aspettiamo
		printf("attendo creazione figli\n");
	}
		//se siamo qui il semaforo è in stato ready	
	printf("semaphore processi totali: %d\n", sem_getval(shm_sem_get_startid(stats.info), 0));
	printf("figli creati con successo\n");
	sem_execute_semop(shm_sem_get_startid(stats.info), 1, 1, 0); //allora semaforo simulazione a 1
	printf("semaphore start : %d\n simulazione avviata\n", sem_getval(shm_sem_get_startid(stats.info), 1));

	
	int n_sec = shm_info_get_sim_duration(stats.info);
	//close_and_exit();
	exit_n_sec(n_sec); // gli do 35 sec di vita

	/*alarm(1);

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
	n_atoms = shm_info_get_n_atoms_init(stats.info); //a scopo di debugging iniziamo con 2 atomi 
	//n_atoms=2;
	printf("voglio avviare  %d atomi ma ne sto avviando %d.\n", shm_info_get_n_atoms_init(stats.info), n_atoms);
	for(i = 0; i <  n_atoms; i++){
		pid= run_process("./atom", i);
		//shm_port_set_pid(state.ports, i, pid);//penso scriva in memoria condivisa i dati dei porti nel suo caso, degli atomi nel nostro, non so se ci serve
		printf("ho crato atomo con pid %d", pid);
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
	char *args[2], buf[10];
	process_pid = fork();
	if (process_pid == -1) {  // fork restituisce 0 se pid figlio, 1 se padre, -1 errore
		perror("master.c: Error in fork.\n");//stampa u filedescriptor 2 che stampa su sdterr
		return -1;
	} else if (process_pid == 0) { // se figlio
		printf("se sono qui sono nel figlio e process pid ha restituito %d e ho pid %d", process_pid, getpid());
		snprintf(buf, sizeof(buf), "%d", index);
        args[0] = name;
        args[1] = NULL; 
		int res_execve;
		if ((res_execve=execve(name, args, NULL)) == -1) { //runno processo name attraverso execve, in teoria adesso no è piu figlio master ma è un processo "a parte"
			perror("execve");//stampa l'ultimo messaggio di errore
			exit(EXIT_FAILURE);   //errori execve
		}
	}else{
		printf("Sono il processo padre con PID %d\n", getpid());
	}
	return process_pid;
	
}


void exit_n_sec(int n_seconds){
    printf("Il processo master verrà terminato dopo %d secondi.\n", n_seconds);

    // Aspetta n_secondi
    sleep(n_seconds);

    // Termina il processo
    printf("Terminazione del processo master dopo %d secondi.\n", n_seconds);
    close_and_exit();
    exit(0);

    // Questa parte del codice non verrà mai eseguita
    printf("Questa riga non verrà mai stampata.\n");
}

void close_and_exit(){
	msg_queue_remove(stats.info); //la creazione e la rimozione delle risorse ipc la lasciamo fare esclusivamente la master
	//shm_info_detach(stats.info);//funziona ma non bisogna fargliela fare al master perche la delete deve essere effettuata dal master e la delete funziona solo se fatta da un processo attaccato
	shm_info_delete(stats.info);

	exit(0);
}