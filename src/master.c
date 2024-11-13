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
	shm_info_t *info; //*inf = (shm_info_t *)shm_attach(shm_id); questo si trova nella funzione shm_info_attach, grazie a questo
	                  // adesso il puntatore di tipo shm_info_t punta ad un area di memoria condivisa allocata e vuota di granezza
					  //shm_info_t 
};

int inhibitor_created=0;

void init_atoms(void);

void init_activator(void);

void init_alimentation(void);

void init_inhibitor(void);

void terminal_inhibitor(void);

pid_t run_process(char *name, int index);

void sigint_handler(int sig);

void take_energy(void);

void close_and_exit(void);

void periodic_print(void);

int shm_sem_ready();

struct stats stats;//creo una variabile di tipo struct statistics chiamata stats

int main(int argc, char *argv[]){
	signal(SIGINT, sigint_handler); //settiamo sigint_handler come handler del segnale SIGINT
	signal(SIGCHLD, SIG_IGN); // ignoriamo i segnali sigchld della terminazione dei procesi figli
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
	printf("Attesa che tutti i processi figli vengano creati...\n");
	if(shm_sem_ready(stats.info)!= 0){

	}
		
	int n_sec = shm_info_get_sim_duration(stats.info);
	printf("processi totali: %d\n", sem_getval(shm_sem_get_startid(stats.info), 0));

	shm_info_set_n_activation_last_sec(stats.info, 0);
	shm_info_set_n_split_last_sec(stats.info, 0);
	shm_info_set_energy_prod_laste_sec(stats.info, 0);
	shm_info_set_energy_cons_last_sec(stats.info, 0);
	shm_info_set_waste_last_sec(stats.info, 0);  
	shm_info_set_n_waste_after_split(stats.info, 0);
	shm_info_set_n_split_blocked(stats.info, 0);
	shm_info_set_energy_inhibitor(stats.info, 0);

	sem_execute_semop(shm_sem_get_startid(stats.info), 1, 1, 0);
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
}

//crea  atomi
void init_atoms(void){
	int i, n_atoms;
	pid_t pid; 
	n_atoms = shm_info_get_n_atoms_init(stats.info);
	for(i = 0; i <  n_atoms; i++){
		pid= run_process("./atom", i);
	}
}


//crea proc attivatore
void init_activator(void){
	pid_t pid;
	pid=run_process("./activator", 1);
}

//crea alimentatore
void init_alimentation(void){
	pid_t pid;
	pid=run_process("./alimentation", 1);
}

//crea proc inibitore
void init_inhibitor(void){
	pid_t pid;
	pid=run_process("./inhibitor", 1);
}

//chiedo da terminale se avviare inibitore
void terminal_inhibitor(void){
	char response[4];

    printf("inizializzare esecuzione con Inibitore? (s/n): ");
    if (fgets(response, sizeof(response), stdin) != NULL) {
        response[strcspn(response, "\n")] = 0; // Rimuovi il carattere di nuova linea (newline) alla fine dell'input
        if (strcmp(response, "s") == 0 || strcmp(response, "S") == 0) {
			sem_setval(shm_sem_get_startid(stats.info), 6, 1);
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
pid_t run_process(char *name, int index){
	pid_t process_pid;
	char *args[2], buf[10];
	process_pid = fork();
	if (process_pid == -1) {  // fork restituisce 0 se pid figlio, 1 se padre, -1 errore
		printf("MELTDOWN MELTDOWN MELTDOWN errore nella fork in master: prova a inizializzare l'esecuzione con meno atomi modificando il file confic_param.txt\n");
		sem_setval(shm_sem_get_startid(stats.info), 7, 0);
		sem_execute_semop(shm_sem_get_startid(stats.info), 1, 1, 0);
		close_and_exit();
		return -1;
	} else if (process_pid == 0) { 
		snprintf(buf, sizeof(buf), "%d", index);
        args[0] = name;
        args[1] = NULL; 
		int res_execve;
		if ((res_execve=execve(name, args, NULL)) == -1) {
			perror("execve");
			exit(EXIT_FAILURE);
		}
	}
	return process_pid;
}

// preleva energy demand
void take_energy(){ 	
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
		sem_execute_semop(shm_sem_get_startid(stats.info), 3, 1, 0);
		sem_setval(shm_sem_get_startid(stats.info), 7, 0);
		close_and_exit();
	}else if(energy_demand > energy_now){
		printf("BLACKOUT - BLACKOUT - BLACKOUT l'energia consumata dal master: %d è maggiore dell'energia totale %d\n",energy_demand , energy_now);
		sem_execute_semop(shm_sem_get_startid(stats.info), 3, 1, 0);
		sem_setval(shm_sem_get_startid(stats.info), 7, 0);
		close_and_exit();
	}else{
		shm_info_set_energy_prod_tot(stats.info, energ);   //aggiorna mem condivisa in mutua escl
		shm_info_set_energy_cons_tot(stats.info, energy_demand);
		sem_execute_semop(shm_sem_get_startid(stats.info), 3, 1, 0);
	}
	
}


void periodic_print(void){
	int print;
	int temp;
	printf("inibitore attivo: %d\n", sem_getval(shm_sem_get_startid(stats.info), 6) );
	while(sem_getval(shm_sem_get_startid(stats.info), 9)==0){
	}
	sem_execute_semop(shm_sem_get_startid(stats.info), 9, -1, 0);
	print=shm_info_get_n_split_tot(stats.info); 
	printf("scissioni totali = %d \n" , print );
	temp=print;
	print=print-shm_info_get_n_split_last_sec(stats.info); //calcolo le scissioni ultimo sec come quelle totali di questo secondo meno le totali dello scorso secondo
	printf("scissioni ultimo secondo  = %d\n", print );
	shm_info_set_n_split_last_sec(stats.info, temp);
	sem_execute_semop(shm_sem_get_startid(stats.info), 9, 1, 0);

	while(sem_getval(shm_sem_get_startid(stats.info), 8)==0){
	}
	sem_execute_semop(shm_sem_get_startid(stats.info), 8, -1, 0);
	print=shm_info_get_n_activation_tot(stats.info);
	printf("attivazioni totali = %d\n" , print);
	temp=print;
	print=print-shm_info_get_n_activation_last_sec(stats.info); //calcolo le attivazioni ultimo sec come quelle totali di questo secondo meno le totali dello scorso secondo
	printf("attivazioni ultimo secondo  = %d\n", print );
	shm_info_set_n_activation_last_sec(stats.info, temp);
	sem_execute_semop(shm_sem_get_startid(stats.info), 8, 1, 0);

	while(sem_getval(shm_sem_get_startid(stats.info), 4)==0){
	}
	sem_execute_semop(shm_sem_get_startid(stats.info), 4, -1, 0);
	print=shm_info_get_energy_prod(stats.info);
	printf("energia prodotta totale = %d\n" , print);
	temp=print;
	print=print-shm_info_get_energy_prod_laste_sec(stats.info); //calcolo le attivazioni ultimo sec come quelle totali di questo secondo meno le totali dello scorso secondo
	printf("energia prodotta ultimo secondo  = %d\n", print );
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
	shm_info_set_waste_last_sec(stats.info, temp);
	sem_execute_semop(shm_sem_get_startid(stats.info), 5, 1, 0);

	print=shm_info_get_energy_cons_tot(stats.info);
	printf("energia consumata totale  = %d\n", print );
	temp=print;
	print=print-shm_info_get_energy_cons_last_sec(stats.info);
	printf("energia consumata ultimo secondo  = %d\n", print );
	shm_info_set_energy_cons_last_sec(stats.info, temp);

	printf("processi rimanenti %d\n", sem_getval(shm_sem_get_startid(stats.info), 2));

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


int shm_sem_ready(){// controlla che il semaforo process sia pronto
	int num_process = shm_info_get_n_atoms_init(stats.info)+2;
	//se inibitore attivato allora 
	if(sem_getval(shm_sem_get_startid(stats.info), 6)==1){
		num_process= num_process+1;
	}
	while (sem_getval(shm_sem_get_startid(stats.info), 0) < num_process) {
		printf("semaphore processi stampati da shm ogni secondo: %d\n", sem_getval(shm_sem_get_startid(stats.info), 0));
		sleep(1);
    }
	return 0;
}


void sigint_handler(int sig) {
    printf("Terminazione controllata della simulazione per ricezione segnale SIGINT\n");
	sem_setval(shm_sem_get_startid(stats.info), 7, 0);
	close_and_exit();
}

void close_and_exit(){
	int process_remaining, new_process_remaining, count_exit;
	process_remaining=0;
	count_exit=0;
	pid_t inhib_pid;
	const char *process_name = "atom activator alimentation inhibitor";  // Nome/i del processo a cui eventualmente inviare il segnale
	while(sem_getval(shm_sem_get_startid(stats.info), 2)>0)
	{
		if(sem_getval(shm_sem_get_startid(stats.info),6)==0){
			inhib_pid=shm_info_get_inhibitor_pid(stats.info);
			kill(inhib_pid, SIGUSR2);
		}
		new_process_remaining=sem_getval(shm_sem_get_startid(stats.info),2);
		printf("processi rimanenti sem 2 %d\n", sem_getval(shm_sem_get_startid(stats.info), 2));
		if (process_remaining==new_process_remaining){
			printf("%d \n", count_exit);
			count_exit=count_exit+1;
		}
		process_remaining=new_process_remaining;
		if (count_exit ==5){
			const char *signal = "SIGINT"; // Il segnale da inviare

			char command[256];// Costruzione del comando killall
			snprintf(command, sizeof(command), "killall %s %s", signal, process_name);

			int ret = system(command);// Esecuzione del comando
			if (ret == -1) {
				perror("Errore nell'esecuzione del comando killall");
			} else {
				printf("Segnale %s inviato a tutti i processi con nome %s\n", signal, process_name);
			}
		}else if(count_exit>=8){
			printf("%d processi verranno terminati in modo non controllato\n" , sem_getval(shm_sem_get_startid(stats.info), 2));
			const char *signal = "SIGKILL";
			char command[256];// Costruzione del comando killall
			snprintf(command, sizeof(command), "killall %s %s", signal, process_name);

			int ret = system(command);// Esecuzione del comando
			if (ret == -1) {
				perror("Errore nell'esecuzione del comando killall");
			} else {
				printf("Segnale %s inviato a tutti i processi con nome %s\n", signal, process_name);
			}
			break;
		}
		sleep(1);
		 
		
	}
	msg_queue_remove(stats.info);
	sem_delete(shm_sem_get_startid(stats.info));
	shm_info_delete(stats.info);

	printf("terminazione del processo MASTER.\n");
	exit(0);
}