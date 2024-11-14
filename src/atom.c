#define _GNU_SOURCE

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
	shm_info_t *info; //*inf = (shm_info_t *)shm_attach(shm_id); questo si trova nella funzione shm_info_attach, grazie a questo
	                  // adesso il puntatore di tipo shm_info_t punta ad un area di memoria condivisa allocata e vuota di granezza
					  //shm_info_t 
};

int split(int atomic_n, int if_waste);
void update_energy(struct  atom_n_parent_child);
int energy(struct atom_n_parent_child);
struct  atom_n_parent_child atomic_n_to_split(int atomic_n);
void close_and_exit();
int rcv_msg(void);
int get_max_user_processes(void);
void init_random(void);
int read_pids_max(void);
long get_free_memory(void);
int adaptive_probability(int user_limit, int cgroup_limit);
int random_atomic_n(int max, int min);
void update_waste(int waste);
int ctrl_sem_getval(int sem_id, int sem_n);
int ctrl_sem_execute_semop(id_t sem_id, int sem_index, int op_val, int flags);
double max3(double a, double b, double c);
void sigint_handler(int sig);

struct stats stats;

int main(int argc, char *argv[]){
	int atomic_number;
	int user_limit=get_max_user_processes();
	int cgroup_limit=read_pids_max();
	signal(SIGINT, sigint_handler);
	if(shm_info_attach(&stats.info)==-1){
		exit(EXIT_FAILURE);
	}
	ctrl_sem_execute_semop(shm_sem_get_startid(stats.info), 0, 1, 0);
	ctrl_sem_execute_semop(shm_sem_get_startid(stats.info), 2, 1, 0);
	if(ctrl_sem_getval(shm_sem_get_startid(stats.info), 7)==0){
		close_and_exit();
	}
	while(ctrl_sem_getval(shm_sem_get_startid(stats.info), 1) != 1){
		sleep(1);
	}
	int min_atomic_n = shm_info_get_min_n_atoms(stats.info);
	int max_atomic_n = shm_info_get_n_atom_max(stats.info);
	init_random();
    atomic_number=random_atomic_n(max_atomic_n,min_atomic_n);
	int split_prob;
	
	while (ctrl_sem_getval(shm_sem_get_startid(stats.info), 7)>0){
		if(rcv_msg()){
			while(ctrl_sem_getval(shm_sem_get_startid(stats.info), 8)==0){
				
			}
				ctrl_sem_execute_semop(shm_sem_get_startid(stats.info), 8, -1, 0);
				shm_info_set_n_activation_tot(stats.info, 1);
				ctrl_sem_execute_semop(shm_sem_get_startid(stats.info), 8, 1, 0);
			if(atomic_number > min_atomic_n){
				if(ctrl_sem_getval(shm_sem_get_startid(stats.info), 6)==1){ // se inibitore è attivo
					split_prob=adaptive_probability(user_limit, cgroup_limit);
					if(split_prob ==  -1){ //-1 blocco tutto, 1 splittowaste o blocco, 0 splitto
						//non faccio niente=blocco lo split
						while(sem_getval(shm_sem_get_startid(stats.info), 11)==0){
						}
						sem_execute_semop(shm_sem_get_startid(stats.info), 11, -1, 0);
						shm_info_set_n_split_blocked((stats.info), 1);
						sem_execute_semop(shm_sem_get_startid(stats.info), 11, 1, 0);
					}else if(split_prob ==  1){
						split_prob=adaptive_probability(user_limit, cgroup_limit);
						if((split_prob == 1) || (split_prob == -1)){ //split con waste o non split
							//blocchiamo split
							while(sem_getval(shm_sem_get_startid(stats.info), 11)==0){
							}
							sem_execute_semop(shm_sem_get_startid(stats.info), 11, -1, 0);
							shm_info_set_n_split_blocked((stats.info), 1);
							sem_execute_semop(shm_sem_get_startid(stats.info), 11, 1, 0);
						}else if(split_prob == 0){
							atomic_number=split(atomic_number, 1); //splittiamo con waste
						}
					}else if(split_prob ==  0){
						//split senza waste
						atomic_number=split(atomic_number, 0);
					}
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

int split(int atomic_n, int if_waste){
	int p_c_pipe[2];
	if (pipe(p_c_pipe) == -1) {
		perror("pipe open err.\n");
        	exit(EXIT_FAILURE);
	}
	pid_t process_pid;
	if ((process_pid = fork()) == -1) {
		dprintf(2, "atom.c: Error in fork MELTDOWN MELTDOWN MELTDOWN .\n");
		printf("MELTDOWN MELTDOWN MELTDOWN errore nella fork in atom\n");
		sem_setval(shm_sem_get_startid(stats.info), 7, 0);
		close_and_exit();
	} else if (process_pid == 0) {
		
		if(shm_info_attach(&stats.info)==-1){
		exit(EXIT_FAILURE);
		} 
		
		if (close(p_c_pipe[1]) == -1) {// Chiudo il lato di scrittura della pipe
			 perror("write pipe child err. current atom will be closed \n");
			 exit(EXIT_FAILURE);
    	}
		read(p_c_pipe[0], &atomic_n, sizeof(int));

			ctrl_sem_execute_semop(shm_sem_get_startid(stats.info), 2, 1, 0);

			while(ctrl_sem_getval(shm_sem_get_startid(stats.info), 9)==0){
			}
				ctrl_sem_execute_semop(shm_sem_get_startid(stats.info), 9, -1, 0);
				shm_info_set_n_split_tot(stats.info, 1);
				ctrl_sem_execute_semop(shm_sem_get_startid(stats.info), 9, 1, 0);

		if (close(p_c_pipe[0]) == -1) {  // Chiudo il lato di lettura della pipe
       		perror("read pipe child err. current atom will be closed\n");
    		exit(EXIT_FAILURE);
    	}
		if(if_waste==1){
			update_waste(1);
			while(sem_getval(shm_sem_get_startid(stats.info), 12)==0){
							}
							sem_execute_semop(shm_sem_get_startid(stats.info), 12, -1, 0);
							shm_info_set_n_waste_after_split((stats.info), 1);
							sem_execute_semop(shm_sem_get_startid(stats.info), 12, 1, 0);
			close_and_exit();
		}
		return atomic_n;
	} else {
        struct  atom_n_parent_child split_atom_n = atomic_n_to_split(atomic_n);
		atomic_n = split_atom_n.atom_n_parent;
		if (close(p_c_pipe[0]) == -1) {  // Chiudo il lato di lettura della pipe
        	perror("read pipe parent err. current atom will be closed\n");
            exit(EXIT_FAILURE);
    	}
		write(p_c_pipe[1], &split_atom_n.atom_n_child, sizeof(int));

		if (close(p_c_pipe[1]) == -1) {// Chiudo il lato di scrittura della pipe
       		perror("write pipe parent err. current atom will be closed\n");
        	exit(EXIT_FAILURE);
    	}
		update_energy(split_atom_n);
		return atomic_n;
	}
}

int get_max_user_processes(void) { // equivalente al numero di processi con comando ulimit -u
    struct rlimit limit;
    if (getrlimit(RLIMIT_NPROC, &limit) == 0) {
        return (int)limit.rlim_cur;
    } else {
        perror("getrlimit");
        return -1;
    }
}

// Costruisce il percorso al file cgroup del processo corrente grazie al pid
void get_cgroup_path(char *path_buffer, size_t buffer_size) {
    pid_t pid = getpid();
    snprintf(path_buffer, buffer_size, "/proc/%d/cgroup", pid);  // scrive nel buffer il path
}

// Apre il file cgroup per determinare il percorso del cgroup relativo al controller pids
int get_pids_max_path(char *cgroup_path, char *pids_max_path, size_t buffer_size) {
    FILE *file = fopen(cgroup_path, "r");  //apre il file in lettura
    if (file == NULL) { 
        return -1;
    }
    char line[256];  // Buffer per leggere ogni linea del file cgroup
    int found = 0;  // Flag per indicare se abbiamo trovato il controller pids
    char relative_path[PATH_MAX] = "";  // Buffer per il percorso relativo
    // Legge ogni riga del file cgroup
    while (fgets(line, sizeof(line), file)) {  // fgets legge una linea dal file e la memorizza in line, in teoria il file dovrebbe contenere solo una linea
        char *start_path = strchr(line, '/');// Cerca la linea contenente "/"
        if (start_path != NULL) {
            // Rimuove il carattere di nuova riga alla fine della stringa, se presente
            start_path[strcspn(start_path, "\n")] = '\0'; // sostituisce con il valore nullo il primo a capo
            strncpy(relative_path, start_path, sizeof(relative_path) - 1);
            relative_path[sizeof(relative_path) - 1] = '\0';  // termino la stringa
            found = 1;  // Imposta il flag su 1, indicando che abbiamo trovato il percorso
            break;  // Esce dal loop una volta trovato il percorso relativo
        }
    }
    fclose(file);  //Chiude il file cgroup
    if (!found) {
        return -1;
    }
    snprintf(pids_max_path, buffer_size, "/sys/fs/cgroup%s/pids.max", relative_path);// Costruisce il percorso al file pids.max
    // Rimuove il carattere di nuova riga alla fine della stringa, sostituendolo con il terminatore di stringa '\0'
    pids_max_path[strcspn(pids_max_path, "\n")] = '\0';  // strcspn calcola la lunghezza della parte iniziale della stringa che non contiene il carattere '\n'
    return 1;  //successo
}

int read_pids_max(void) {
    char cgroup_path[PATH_MAX];
    char pids_max_path[PATH_MAX];

    get_cgroup_path(cgroup_path, sizeof(cgroup_path));
    if (get_pids_max_path(cgroup_path, pids_max_path, sizeof(pids_max_path)) != 1) {
        return -1;
    }

    FILE *file = fopen(pids_max_path, "r");
    if (file == NULL) {
        return -1;
    }
    char buffer[128];
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        fclose(file);
        return -1;
    }
    fclose(file);
    if (buffer[0] == 'm') {//No limit on the number of processes
        return -1;
    } else {
        int max_processes = atoi(buffer);
        return max_processes;
    }
}


long get_free_memory(void) {
    FILE *file = fopen("/proc/meminfo", "r");// Apre il file /proc/meminfo in modalità lettura
    if (file == NULL) {
        perror("fopen");
        return -1; 
    }
    char line[256];
    long free_memory_kb = -1;  // Variabile per memorizzare la memoria libera in kilobytes
    while (fgets(line, sizeof(line), file)) {// Scansione del file /proc/meminfo per trovare la riga contenente "MemFree:"
        if (strncmp(line, "MemFree:", 8) == 0) {
            sscanf(line + 8, "%ld", &free_memory_kb);// Legge il valore di memoria libera dopo "MemFree:" e lo memorizza in free_memory_kb
            break;
        }
    }
    fclose(file);  // Chiude il file /proc/meminfo
    if (free_memory_kb == -1) {
        fprintf(stderr, "Failed to find MemFree in /proc/meminfo\n");
        return -1;
    }
    long free_memory_mb = free_memory_kb / 1024;// Converto in megabytes
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
void update_waste(int waste){// aggiorna le scorie in mem condivisa in mutua esclusione
	while(ctrl_sem_getval(shm_sem_get_startid(stats.info), 5)==0){
	}
	ctrl_sem_execute_semop(shm_sem_get_startid(stats.info), 5, -1, 0);
	waste = waste+ shm_info_get_waste_tot(stats.info);
	shm_info_set_waste_tot(stats.info, waste);
	ctrl_sem_execute_semop(shm_sem_get_startid(stats.info), 5, 1, 0);
}   

//aggiorna energia, teneno conto dell'attività dell'inibitore
void update_energy(struct  atom_n_parent_child p_c){
	int energy_val_tot, energy_val, energy_to_consume, inhibitor, energy_inhibited, temp;
	int explode=(shm_info_get_energy_explode_trashold(stats.info));
	energy_val=energy(p_c);
	energy_to_consume=shm_info_get_energy_demand(stats.info);
	inhibitor=ctrl_sem_getval(shm_sem_get_startid(stats.info), 6);
	
	while((ctrl_sem_getval(shm_sem_get_startid(stats.info), 3)==0) && ctrl_sem_getval(shm_sem_get_startid(stats.info), 4)==0){
	}
	ctrl_sem_execute_semop(shm_sem_get_startid(stats.info), 3, -1, 0);
	ctrl_sem_execute_semop(shm_sem_get_startid(stats.info), 4, -1, 0);
	energy_val_tot=shm_info_get_energy_prod_tot(stats.info);

	if(inhibitor==1){
		if(energy_val_tot<energy_to_consume){
			temp=energy_to_consume - energy_val_tot;
			if(energy_inhibited=(energy_val-temp)>=0){
				energy_val=energy_val-energy_inhibited;
			}else{
				energy_inhibited=0;
			}
		}else if(energy_val_tot>=energy_to_consume){
			energy_inhibited=energy_val;
			energy_val=0;
		}
	}
	energy_val_tot=energy_val+energy_val_tot;
	if(energy_val_tot > explode){
		printf("EXPLODE - EXPLODE - EXPLODE energia totale prodotta da atom al netto di quella consumata dal master %d è maggiore del parametro massimo %d\n",energy_val_tot , explode);
		ctrl_sem_execute_semop(shm_sem_get_startid(stats.info), 3, 1, 0);
		ctrl_sem_execute_semop(shm_sem_get_startid(stats.info), 4, 1, 0);
		sem_setval(shm_sem_get_startid(stats.info), 7, 0);
		close_and_exit();
	}else{
		shm_info_set_energy_prod(stats.info, energy_val);
		shm_info_set_energy_prod_tot(stats.info, energy_val_tot);
		ctrl_sem_execute_semop(shm_sem_get_startid(stats.info), 3, 1, 0);
		ctrl_sem_execute_semop(shm_sem_get_startid(stats.info), 4, 1, 0);
		while(sem_getval(shm_sem_get_startid(stats.info), 10)==0){
		}
		sem_execute_semop(shm_sem_get_startid(stats.info), 10, -1, 0);
		shm_info_set_energy_inhibitor((stats.info), energy_inhibited);
		sem_execute_semop(shm_sem_get_startid(stats.info), 10, 1, 0);
	}
}

//calcola energia prodotta
int energy(struct atom_n_parent_child p_c){
	int temp= p_c.atom_n_parent*p_c.atom_n_child;
	if(p_c.atom_n_parent >= p_c.atom_n_child){
		temp=temp-p_c.atom_n_parent;
	}else{
		temp=temp-p_c.atom_n_child;
	}
	return temp;
}

void init_random() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    unsigned int seed = ts.tv_nsec ^ getpid();
    srand(seed);
}

// Genera un numero casuale compreso tra max e min
int random_atomic_n(int max, int min){
    return rand() % (max - min+ 1) + min;
}

int rcv_msg(void){
	int msg_q_id = msg_q_a_a_id_get(stats.info);
	struct comunication_msg msg_rcv;
	msg_comunication_rcv(msg_q_id, &msg_rcv.bool_split);
	return msg_rcv.bool_split;
}

int ctrl_sem_getval(int sem_id, int sem_n){
	int res;
	if((res=sem_getval(sem_id, sem_n))<0){
		exit(-1); // se fallisce la getval allora la mem cond o i semafori sono stati cancellati, quindi inutile fare close_and_exit
	}
	return res;
}

int ctrl_sem_execute_semop(id_t sem_id, int sem_index, int op_val, int flags){
	int res;
	if((res=sem_execute_semop(sem_id, sem_index, op_val, flags))<0){
		exit(-1); // se fallisce la execute semop allora la mem cond o i semafori sono stati cancellati, quindi inutile fare close_and_exit che usa appunto semafori e mem condivisa
	}
	return res;
}

void sigint_handler(int sig) {
	close_and_exit();
}

void close_and_exit(){
	ctrl_sem_execute_semop(shm_sem_get_startid(stats.info), 2, -1, 0);
	shm_info_detach(stats.info);

	exit(0);
}