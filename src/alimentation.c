//alimentazione : ogni step nanosecondi immette combustibile ovvero crea n nuovi atomi
//tutti include e define
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

/*struct data{//è una struttura dati contipi che sono a loro volta strutture dati in mem condivisa o qualcosa del genere
	shm_data_t *data
}*/
void init_atoms(void);
pid_t run_process(char *name, int index);
void nsleep(long step_nsec);
//struct data data; 

int main(int argc, char *argv[]){
	long step_nsec;
	step_nsec = 5;//dovremmo leggerli da mem condivisa  ho messo 5 secondo
	/*while (1) {  //rimane in esecuzione
		nsleep(step_nsec); //dorme per strap nanosecondi 
		init_atoms();//ed avvia n_new_atoms , poi torna a dormire così all'infinito
	}*/
	printf("alimentazione dice:sono vivooo.\n");
	int i=0;
	while (i<=3) {  //rimane in esecuzione
		nsleep(step_nsec); //dorme per strap nanosecondi 
		init_atoms();//ed avvia n_new_atoms , poi torna a dormire così all'infinito
		printf("alimetazione ha creato atomo con  i= %d.\n", i);
		i=i+1;
	}
	printf("terminazione del processo alimentation.\n");
	exit(0);

    // Questa parte del codice non verrà mai eseguita
    printf("Questa riga non verrà mai stampata.\n");
	
}

void init_atoms(void){
	int i, n_new_atoms;//n new atoms da leggerli in struct data data
	pid_t pid;
	//n_atoms= get_atoms(state.general); //legge da struct state campo general in cui ci ha salvato i dati  a noi deve leggere campo N_NUOVI_ATOMI
	n_new_atoms=1; //a scopo di debugging iniziamo con 1 nuovi atomi 
	for(i=0; i< n_new_atoms; i++){
		pid= run_process("./atom", i);
		//shm_port_set_pid(state.ports, i, pid);//penso scriva in memoria condivisa i dati dei porti nel suo caso, degli atomi nel nostro, non so se ci serve
	}
}

pid_t run_process(char *name, int index){ // cre il figlio con fork() e lo trasforma in un altro processo, specificato da name, attraverso execve
	pid_t process_pid;
	//int atomic_n =35; //[DA CORREGGERE] 35 num atomico atomi creati da aimentatore 
	char *args[2], buf[10];   
	if ((process_pid = fork()) == -1) {  // fork restituisce 0 se pid figlio, 1 se padre, -1 errore
		dprintf(2, "alimentation.c: Error in fork.\n");//stampa u filedescriptor 2 che stampa su sdterr
		//close_all();
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
