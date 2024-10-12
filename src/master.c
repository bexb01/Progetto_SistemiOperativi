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

int inhibitor_created=0;

//prototipazione/ dichiarazione anticipata delle funzioni

void init_atoms(void);

void init_activator(void);

void init_alimentation(void);

void init_inhibitor(void);

void terminal_inhibitor(void);

pid_t run_process(char *name, int index);

void exit_n_sec(int n_seconds);

void take_energy(void);

void signal_handler(int signal);

void set_signal_handler(void);

void term_childs(const char *process_name);

void close_and_exit(void);

void periodic_print(void);

int shm_sem_ready();

struct stats stats;//creo una variabile di tipo struct statistics chiamata stats

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
	terminal_inhibitor();
	printf("Attesa c90he tutti i processi figli vengano creati...\n");
	if(shm_sem_ready(stats.info)!= 0){// fino a quando semaforo non è ready aspettiamo
		//printf("attendo creazione figli\n");
		
	}
		//se siamo qui il semaforo è in stato ready	
	int n_sec = shm_info_get_sim_duration(stats.info); //durata simulazione
	printf("processi totali: %d\n", sem_getval(shm_sem_get_startid(stats.info), 0));
	//printf("figli creati con successo\n");

	shm_info_set_n_activation_last_sec(stats.info, 0);
	shm_info_set_n_split_last_sec(stats.info, 0);
	shm_info_set_energy_prod_laste_sec(stats.info, 0);
	shm_info_set_energy_cons_last_sec(stats.info, 0);
	shm_info_set_waste_last_sec(stats.info, 0);  
	shm_info_set_n_waste_after_split(stats.info, 0);
	shm_info_set_n_split_blocked(stats.info, 0);
	shm_info_set_energy_inhibitor(stats.info, 0);

	sem_execute_semop(shm_sem_get_startid(stats.info), 1, 1, 0); //allora semaforo simulazione a 1
	printf("simulazione avviata\n");

	while (sem_getval(shm_sem_get_startid(stats.info), 7)>0) {

		if(shm_info_get_sim_duration(stats.info)<=0){
			printf("TIME OUT TIME OUT TIMEOUT TIME OUT TIMEOUT\n");
			printf("stampa finale \n");
			periodic_print();
			sem_setval(shm_sem_get_startid(stats.info), 7, 0);
			close_and_exit();
		}else if(shm_info_get_sim_duration(stats.info)>0){
			//alarm(1);
			shm_info_set_sim_duration(stats.info, shm_info_get_sim_duration(stats.info)-1);
			printf("TEMPO RIMANENTE TEMPO RIMANENTE TEMPO RIMANENTE TEMPO RIMANENTE %d \n", shm_info_get_sim_duration(stats.info));
			periodic_print();
		}
		sleep(1);
		take_energy();
	}
	close_and_exit();
//bho
}

//crea  atomi
void init_atoms(void){
	int i, n_atoms;
	pid_t pid; 
	n_atoms = shm_info_get_n_atoms_init(stats.info); //a scopo di debugging iniziamo con 2 atomi 
	//n_atoms=2;
	//printf("voglio avviare  %d atomi ma ne sto avviando %d.\n", shm_info_get_n_atoms_init(stats.info), n_atoms);
	for(i = 0; i <  n_atoms; i++){
		pid= run_process("./atom", i);
		//shm_port_set_pid(state.ports, i, pid);//penso scriva in memoria condivisa i dati dei porti nel suo caso, degli atomi nel nostro, non so se ci serve
		//printf("ho crato atomo con pid %d", pid);
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

//crea proc inibitore
void init_inhibitor(void){
	pid_t pid;
	pid=run_process("./inhibitor", 1); //  ./ prima del programma indica che si trova nella directory corrente quindi master e il codice dei processi attivati da master stanno nella stessa cartella
}


void terminal_inhibitor(void){
	char response[4];

    printf("inizializzare esecuzione con Inibitore? (s/n): ");
    if (fgets(response, sizeof(response), stdin) != NULL) {
        response[strcspn(response, "\n")] = 0; // Rimuovi il carattere di nuova linea (newline) alla fine dell'input
        if (strcmp(response, "s") == 0 || strcmp(response, "S") == 0) {
            init_inhibitor();
			inhibitor_created=1;
			printf("Inibitore avviato.\n");
        } else if (strcmp(response, "n") == 0 || strcmp(response, "N") == 0) {
            printf("Inibitore non avviato.\n");
        } else {
            printf("Risposta non valida. Inibitore non avviato.\n");
        }
	}
}

//runna il processo specificato da name
pid_t run_process(char *name, int index){ // cre il figlio con fork() e lo trasforma in un altro processo, specificato da name, attraverso execve
	pid_t process_pid;
	char *args[2], buf[10];
	process_pid = fork();
	if (process_pid == -1) {  // fork restituisce 0 se pid figlio, 1 se padre, -1 errore
		perror("master.c: Error in fork.\n");//stampa u filedescriptor 2 che stampa su sdterr
		fprintf(stderr, "Errore nella fork: %s\n", strerror(errno));
        perror("Errore nella fork");
        printf("Codice di errore errno: %d\n", errno);
		printf("MELTDOWN MELTDOWN MELTDOWN errore nella fork in master: prova a inizializzare l'esecuzione con meno atomi modificando il file confic_param.txt\n");
		//blocco esecuzione
		sem_setval(shm_sem_get_startid(stats.info), 7, 0);
		sem_execute_semop(shm_sem_get_startid(stats.info), 1, 1, 0); //allora semaforo simulazione a 1 cosi i processi che si sono creati almeno hanno la possibilità 
		close_and_exit();
		return -1;
	} else if (process_pid == 0) { // se figlio
		//printf("se sono qui sono nel figlio e process pid ha restituito %d e ho pid %d", process_pid, getpid());
		snprintf(buf, sizeof(buf), "%d", index);
        args[0] = name;
        args[1] = NULL; 
		int res_execve;
		if ((res_execve=execve(name, args, NULL)) == -1) { //runno processo name attraverso execve, in teoria adesso no è piu figlio master ma è un processo "a parte"
			perror("execve");//stampa l'ultimo messaggio di errore
			exit(EXIT_FAILURE);   //errori execve
		}
	}else{
		//printf("Sono il processo padre con PID %d\n", getpid());
	}
	return process_pid;
	
}

void take_energy(){ 	// preleva energy demand
	int energy_demand=shm_info_get_energy_demand(stats.info);
	int energy_now=0;
	int energ =0;
	int explode;
	while(sem_getval(shm_sem_get_startid(stats.info), 3)==0){
	}
	sem_execute_semop(shm_sem_get_startid(stats.info), 3, -1, 0);
	energy_now=shm_info_get_energy_prod_tot(stats.info);
	energ = energy_now - energy_demand;
	if(energ > (explode=(shm_info_get_energy_explode_trashold(stats.info)))){
		printf("EXPLODE - EXPLODE - EXPLODE l'energia totale al netto di quella consumata dal master. %d è maggiore del parametro massimo %d\n",energ , explode);
		//bloccare l'esecuzione
		sem_execute_semop(shm_sem_get_startid(stats.info), 3, 1, 0);
		sem_setval(shm_sem_get_startid(stats.info), 7, 0);
		close_and_exit();
	}else if(energy_demand > energy_now){
		printf("BLACKOUT - BLACKOUT - BLACKOUT l'energia consumata dal master: %d è maggiore dell'energia totale %d\n",energy_demand , energy_now);
		//bloccare l'esecuzione
		sem_execute_semop(shm_sem_get_startid(stats.info), 3, 1, 0);
		sem_setval(shm_sem_get_startid(stats.info), 7, 0);
		close_and_exit();
	}else{
		shm_info_set_energy_prod_tot(stats.info, energ);         //aggiorna mem condivisa in mutua escl
		shm_info_set_energy_cons_tot(stats.info, energy_demand);
		sem_execute_semop(shm_sem_get_startid(stats.info), 3, 1, 0);
	}
	
}

void signal_handler(int signal){
	switch (signal) {
	/*case SIGSEGV:
		dprintf(2, "master.c: Segmentation fault. Closing all.\n");
	case SIGTERM:
	case SIGINT:
		close_all();*/
	case SIGALRM:
		if(sem_getval(shm_sem_get_startid(stats.info), 7)==0){
			close_and_exit();
		}else if(shm_info_get_sim_duration(stats.info)==0){
			printf("STAMPA FINALE, secondi rimanenti: 0\n");
			periodic_print();
			sem_setval(shm_sem_get_startid(stats.info), 7, 0);
			close_and_exit();
		}else if(shm_info_get_sim_duration(stats.info)>0){
			alarm(1);
			shm_info_set_sim_duration(stats.info, shm_info_get_sim_duration(stats.info)-1);
			printf("TEMPO RIMANENTE TEMPO RIMANENTE TEMPO RIMANENTE TEMPO RIMANENTE %d\n", shm_info_get_sim_duration(stats.info));
			periodic_print();
		}
		break;
	default:
		break;
	}
}

void periodic_print(void){
	int print;
	int temp;
	printf("inibitore attivo: %d\n", sem_getval(shm_sem_get_startid(stats.info), 6) );
	while(sem_getval(shm_sem_get_startid(stats.info), 9)==0){
	}
	sem_execute_semop(shm_sem_get_startid(stats.info), 9, -1, 0);
	print=shm_info_get_n_split_tot(stats.info);//leggo 
	printf("scissioni totali = %d \n" , print );//stampo scissioni totali
	temp=print;
	print=print-shm_info_get_n_split_last_sec(stats.info); //calcolo le scissioni ultimo sec come quelle totali di questo secondo meno le totali dello scorso secondo
	printf("scissioni ultimo secondo  = %d\n", print );//stampo scissioni ultimo secondo
	shm_info_set_n_split_last_sec(stats.info, temp);
	sem_execute_semop(shm_sem_get_startid(stats.info), 9, 1, 0);

	while(sem_getval(shm_sem_get_startid(stats.info), 8)==0){
	}
	sem_execute_semop(shm_sem_get_startid(stats.info), 8, -1, 0);
	print=shm_info_get_n_activation_tot(stats.info);//leggo
	printf("attivazioni totali = %d\n" , print);//stampo le attivazioni totali
	temp=print;
	print=print-shm_info_get_n_activation_last_sec(stats.info); //calcolo le attivazioni ultimo sec come quelle totali di questo secondo meno le totali dello scorso secondo
	printf("attivazioni ultimo secondo  = %d\n", print );//stampo attivazioni ultimo secondo
	shm_info_set_n_activation_last_sec(stats.info, temp);
	sem_execute_semop(shm_sem_get_startid(stats.info), 8, 1, 0);

	while(sem_getval(shm_sem_get_startid(stats.info), 4)==0){
	}
	sem_execute_semop(shm_sem_get_startid(stats.info), 4, -1, 0);
	print=shm_info_get_energy_prod(stats.info);//leggo
	printf("energia prodotta totale = %d\n" , print);//stampo le attivazioni totali
	temp=print;
	print=print-shm_info_get_energy_prod_laste_sec(stats.info); //calcolo le attivazioni ultimo sec come quelle totali di questo secondo meno le totali dello scorso secondo
	printf("energia prodotta ultimo secondo  = %d\n", print );//stampo attivazioni ultimo secondo
	shm_info_set_energy_prod_laste_sec(stats.info, temp);
	sem_execute_semop(shm_sem_get_startid(stats.info), 4, 1, 0);
	
	while(sem_getval(shm_sem_get_startid(stats.info), 5)==0){
	}
	sem_execute_semop(shm_sem_get_startid(stats.info), 5, -1, 0);
	print=shm_info_get_waste_tot(stats.info);
	printf("waste totale = %d\n" , print);
	temp=print;
	print=print-shm_info_get_waste_last_sec(stats.info);
	printf("waste ultimo secondo  = %d\n", print );
	shm_info_set_waste_last_sec(stats.info, temp);      //aggiorna mem condivisa in mutua escl
	sem_execute_semop(shm_sem_get_startid(stats.info), 5, 1, 0);

	print=shm_info_get_energy_cons_tot(stats.info);
	printf("energia consumata totale  = %d\n", print );
	temp=print;
	print=print-shm_info_get_energy_cons_last_sec(stats.info);
	printf("energia consumata ultimo secondo  = %d\n", print );
	shm_info_set_energy_cons_last_sec(stats.info, temp);

	if(inhibitor_created==1){
		while(sem_getval(shm_sem_get_startid(stats.info), 10)==0){
		}
		sem_execute_semop(shm_sem_get_startid(stats.info), 10, -1, 0);
		print=shm_info_get_energy_inhibitor(stats.info);
		printf("quantità di energia assorbita dal processo inhibitor: %d \n" , print);
		sem_execute_semop(shm_sem_get_startid(stats.info), 10, 1, 0);

		while(sem_getval(shm_sem_get_startid(stats.info), 11)==0){
		}
		sem_execute_semop(shm_sem_get_startid(stats.info), 11, -1, 0);
		print=shm_info_get_n_split_blocked(stats.info);
		printf("numero di scissioni bloccate dal processo inhibitor: %d \n" , print);
		sem_execute_semop(shm_sem_get_startid(stats.info), 11, 1, 0);

		while(sem_getval(shm_sem_get_startid(stats.info), 12)==0){
		}
		sem_execute_semop(shm_sem_get_startid(stats.info), 12, -1, 0);
		print=shm_info_get_n_waste_after_split(stats.info);
		printf("waste creata dal processo inhibitor dopo le scissioni: %d \n" , print);
		sem_execute_semop(shm_sem_get_startid(stats.info), 12, 1, 0);
	}
}

void set_signal_handler(void){
	static struct sigaction sa;
	sigset_t mask;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;

	sigfillset(&mask);
	sa.sa_mask = mask;
	sigaction(SIGALRM, &sa, NULL);
	/*sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	*/
}

int shm_sem_ready(){// controlla che il semaforo process sia pronto= abbia valore di numatomsinit+2
	int num_process = shm_info_get_n_atoms_init(stats.info)+2;
	while (sem_getval(shm_sem_get_startid(stats.info), 0) < num_process) {
		printf("semaphore processi stampati da shm ogni secondo: %d\n", sem_getval(shm_sem_get_startid(stats.info), 0));
		sleep(1);
    }
	return 0;
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

void term_childs(const char *process_name) {
    char command[256];
    snprintf(command, sizeof(command), "killall %s", process_name);
    system(command);  // Esegue il comando killall per terminare i processi con nome process_name
}

void close_and_exit(){
	int process_remaining, new_process_remaining, count_exit;
	process_remaining=0;
	count_exit=0;
	while(sem_getval(shm_sem_get_startid(stats.info), 0)>0)
	{
		new_process_remaining=sem_getval(shm_sem_get_startid(stats.info),0);
		printf("processi rimanenti %d\n", sem_getval(shm_sem_get_startid(stats.info), 0));
		if (process_remaining==new_process_remaining){
			printf("%d \n", count_exit);
			count_exit=count_exit+1;
		}
		process_remaining=new_process_remaining;
		if (count_exit >=5){
			//term_childs("atom"); // lo commento tanto non ha fatto nessun effetto

			break;// l'unica cosa che funziona è uscire dal ciclo, pulire le risorse e i figli rimanenti, se ce ne sono,  si eliminano da soli se non ci sono le risorse 
		}
		sleep(1);
		 
		
	}
	msg_queue_remove(stats.info); //la creazione e la rimozione delle risorse ipc la lasciamo fare esclusivamente la master
	//shm_info_detach(stats.info);//funziona ma non bisogna fargliela fare al master perche la delete deve essere effettuata dal master e la delete funziona solo se fatta da un processo attaccato
	sem_delete(shm_sem_get_startid(stats.info));
	shm_info_delete(stats.info);

	printf("terminazione del processo MASTER.\n");
	exit(0);
}