#define _GNU_SOURCE
#define MAX_ATOMS 5200

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>


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
int get_max_user_processes(void);
void init_random(void);
int read_pids_max(void);
long get_free_memory(void);
int adaptive_probability(int user_limit, int cgroup_limit);
int random_atomic_n(int max, int min);
void update_waste(int waste);
int ctrl_sem_getval(int sem_id, int sem_n);
double max3(double a, double b, double c);

struct stats stats;

int main(int argc, char *argv[]){
	int atomic_number;
	int user_limit=get_max_user_processes();
	int cgroup_limit=read_pids_max();
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
				if(ctrl_sem_getval(shm_sem_get_startid(stats.info), 6)==1){ // se inibitore è attivo
					split_prob=adaptive_probability(user_limit, cgroup_limit);
					if(split_prob ==  -1){ //-1 blocco tutto, 1 splittowaste o blocco, 0 splitto
						//non faccio niente=blocco lo split
					}else if(split_prob ==  1){
						split_prob=adaptive_probability(user_limit, cgroup_limit); //riuso la funzione di probabilità 
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
				}else{ // se inibitore non è attivo splitto
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

int get_max_user_processes(void) { // equivalente al numero di processi con comando ulimit -u
    struct rlimit limit;
    if (getrlimit(RLIMIT_NPROC, &limit) == 0) {
		//printf("%d \n",(int)limit.rlim_cur );
        return (int)limit.rlim_cur;
    } else {
        perror("getrlimit");
        return -1;
    }
}



void get_cgroup_path(char *path_buffer, size_t buffer_size) {
    pid_t pid = getpid();
    // Costruisce il percorso al file cgroup del processo corrente grazie al pid
    snprintf(path_buffer, buffer_size, "/proc/%d/cgroup", pid);  // scrive nel buffer il path
    //printf("Variabile 'path_buffer' modificata: %s\n", path_buffer);  // Stampa il percorso del file cgroup
}

int get_pids_max_path(char *cgroup_path, char *pids_max_path, size_t buffer_size) {
    // Apre il file cgroup per determinare il percorso del cgroup relativo al controller pids
    FILE *file = fopen(cgroup_path, "r");  //apre il file in lettura
    if (file == NULL) { 
        //perror("Failed to open cgroup file"); 
        return -1;
    }
    char line[256];  // Buffer per leggere ogni linea del file cgroup
    int found = 0;  // Flag per indicare se abbiamo trovato il controller pids
    char relative_path[PATH_MAX] = "";  // Buffer per il percorso relativo
    // Legge ogni riga del file cgroup
    while (fgets(line, sizeof(line), file)) {  // fgets legge una linea dal file e la memorizza in line, in teoria il file dovrebbe contenere solo una linea
        //printf("Variabile 'line' letta: %s", line);  // Stampa la linea letta che dovrebbe essre l'unica letta
        
        char *start_path = strchr(line, '/');// Cerca la linea contenente "/"
        if (start_path != NULL) {
            // Rimuove il carattere di nuova riga alla fine della stringa, se presente
            start_path[strcspn(start_path, "\n")] = '\0'; // sostituisce con il valore nullo il primo a capo
            strncpy(relative_path, start_path, sizeof(relative_path) - 1);
            relative_path[sizeof(relative_path) - 1] = '\0';  // termino la stringa
            //printf("Variabile 'relative_path' modificata: %s\n", relative_path);  // Stampa il percorso relativo trovato
            found = 1;  // Imposta il flag su 1, indicando che abbiamo trovato il percorso
            break;  // Esce dal loop una volta trovato il percorso relativo
        }
    }

    fclose(file);  // Chiude il file cgroup
    //printf("File cgroup chiuso.\n");  // Stampa un messaggio di chiusura del file

    if (!found) {
        //fprintf(stderr, "No valid path found in cgroup file.\n");
        return -1;
    }

    snprintf(pids_max_path, buffer_size, "/sys/fs/cgroup%s/pids.max", relative_path);// Costruisce il percorso al file pids.max
    //printf("Variabile 'pids_max_path' modificata: %s\n", pids_max_path);  // Stampa il percorso del file pids.max
    // Rimuove il carattere di nuova riga alla fine della stringa, sostituendolo con il terminatore di stringa '\0'
    pids_max_path[strcspn(pids_max_path, "\n")] = '\0';  // strcspn calcola la lunghezza della parte iniziale della stringa che non contiene il carattere '\n'
    //printf("Variabile 'pids_max_path' corretta: %s\n", pids_max_path);  // Stampa il percorso corretto del file pids.max

    return 1;  // Ritorna 1 per indicare successo
}

int read_pids_max(void) {
    char cgroup_path[PATH_MAX];
    char pids_max_path[PATH_MAX];

    get_cgroup_path(cgroup_path, sizeof(cgroup_path));
    if (get_pids_max_path(cgroup_path, pids_max_path, sizeof(pids_max_path)) != 1) {
        //fprintf(stderr, "Failed to determine pids.max path.\n");
        return -1;
    }
    //printf("Variabile 'cgroup_path': %s\n", cgroup_path);  // Stampa il percorso del file cgroup
    //printf("Variabile 'pids_max_path': %s\n", pids_max_path);  // Stampa il percorso del file pids.max 
    FILE *file = fopen(pids_max_path, "r");
    if (file == NULL) {
        //perror("Failed to open pids.max file");
        return -1;
    }
    // printf("Variabile 'file' aperta con successo per pids.max: %p\n", (void *)file);  // Stampa il puntatore al file aperto
    char buffer[128];
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        //perror("Failed to read pids.max file");
        fclose(file);
        return -1;
    }
    //printf("Variabile 'buffer' letta: %s\n", buffer);  // Stampa il contenuto del buffer letto
    fclose(file);
    //printf("File pids.max chiuso.\n");  // Stampa un messaggio di chiusura del file
    if (buffer[0] == 'm') {
        //printf("No limit on the number of processes\n");
        return -1;
    } else {
        int max_processes = atoi(buffer);
        //printf("Variabile 'max_processes': %d\n", max_processes);  // Stampa il numero massimo di processi
        return max_processes;
    }
}


long get_free_memory(void) {
    // Apre il file /proc/meminfo in modalità lettura
    FILE *file = fopen("/proc/meminfo", "r");
    if (file == NULL) {
        perror("fopen");
        return -1; 
    }
    char line[256];  // nuffer
    long free_memory_kb = -1;  // Variabile per memorizzare la memoria libera in kilobytes
    // Scansione del file /proc/meminfo per trovare la riga contenente "MemFree:"
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "MemFree:", 8) == 0) {
            // Legge il valore di memoria libera dopo "MemFree:" e lo memorizza in free_memory_kb
            sscanf(line + 8, "%ld", &free_memory_kb);
            break;  // Esce dal loop una volta trovata la linea desiderata
        }
    }
    fclose(file);  // Chiude il file /proc/meminfo
    if (free_memory_kb == -1) {
        fprintf(stderr, "Failed to find MemFree in /proc/meminfo\n");
        return -1;
    }
    long free_memory_mb = free_memory_kb / 1024;// Converto in megabytes
    //printf("Mem free %ld MB\n", free_memory_mb);
    return free_memory_mb;
}

// Funzione per calcolare la probabilità adattiva, si basa sulla regola piu restringente tra pid massimi di tipo del processo o pid massimi del utente oppure sulla memoria libera
int adaptive_probability(int user_limit, int cgroup_limit) {
	long free_mem = get_free_memory();
	double prob_u_l, prob_cg, param;
	double prob_mem_free= (double)100/free_mem;
	int active_process=ctrl_sem_getval(shm_sem_get_startid(stats.info), 2);
	prob_cg=(double)active_process/cgroup_limit;
	prob_u_l=(double)active_process/user_limit;
    if ((active_process >= user_limit) || (active_process >=cgroup_limit) || (free_mem <= 100)) {
        return -1; // Blocco totale se abbiamo raggiunto il limite
    }
    double probability = max3(prob_cg, prob_u_l, prob_mem_free);
	double random_value = (((double)rand() / RAND_MAX) * 0.9); // Numero casuale tra 0 e 0.9
	//printf("%lf \n", probability-random_value);
    if (random_value < probability) {
        return 1; // indica che o scissione+waste oppure blocca scissione/ nel secondo caso significa che blocca scissione
    }else{
    return 0; // Indica che la scissione può avvenire/ nel secondo caso indica che scinde+waste
	}
}

double max3(double a, double b, double c) {
    if (a >= b && a >= c) {
        return a;
    } else if (b >= a && b >= c) {
        return b;
    } else {
        return c;
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