#define _GNU_SOURCE

#include "../lib/shm.h"
#include "../lib/semaphore.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "include/constants.h" 
#include "include/shm_info.h"
#include "include/msg_comunication.h"

struct shm_inf{
	int energy_demand, n_atoms_init, n_atom_max, min_n_atoms, n_new_atoms, sim_duration, energy_explode_trashold, step_attivatore;
	int n_activation_tot, n_activation_last_sec, n_split_tot, n_split_last_sec, energy_prod_tot, energy_prod_laste_sec,
		energy_cons_tot, energy_cons_last_sec, waste_tot, waste_last_sec;
	long step;
	int shm_info_id;
	int msg_q_atom_activator_id;
	int sem_start_id;
};

void param_init(char * file_path, shm_info_t *inf){
	FILE * param_file;
	char buffer[100];
	if ((param_file = fopen(file_path, "r")) == NULL) {
		fprintf(stderr, " Errore in apertura param_file.\n");
	}
	while(fgets(buffer, sizeof(buffer), param_file) != NULL){

		const char delim[] = " : ";
		char *token = strtok(buffer, delim);
		
		if (strcmp(token, "ENERGY_DEMAND") == 0) {
			token = strtok(NULL, delim);
    		shm_info_set_energy_demand(inf, (int)strtol(token, NULL, 10));
			printf("appena salvato enrgy demand %d.\n", shm_info_get_energy_demand(inf));
		}
		else if (strcmp(token, "N_ATOMS_INIT") == 0) {
			token = strtok(NULL, delim);
			shm_info_set_n_atoms_init(inf, (int)strtol(token, NULL, 10));
			printf("appena salvato n atoms init %d.\n", shm_info_get_n_atoms_init(inf));
		}
		else if (strcmp(token, "N_ATOM_MAX") == 0) {
			token = strtok(NULL, delim);
			shm_info_set_n_atom_max(inf, (int)strtol(token, NULL, 10));
			printf("appena salvato n atom max %d .\n", shm_info_get_n_atom_max(inf));
		}
		else if (strcmp(token, "STEP_ALIMENTAZIONE") == 0) {
			token = strtok(NULL, delim);
			shm_info_set_step(inf, strtol(token, NULL, 10));
			printf("appena salvato  step %ld.\n", shm_info_get_step(inf));
		}
		else if (strcmp(token, "MIN_N_ATOMICO") == 0) {
			token = strtok(NULL, delim);
			shm_info_set_min_n_atoms(inf, (int)strtol(token, NULL, 10));
			printf("appena salvato min n atomico %d.\n", shm_info_get_min_n_atoms(inf));
		}
		else if (strcmp(token, "N_NEW_ATOMS") == 0) {
			token = strtok(NULL, delim);
			shm_info_set_n_new_atoms(inf, (int)strtol(token, NULL, 10));
			printf("appena salvato n new atoms %d.\n", shm_info_get_n_new_atoms(inf));
		}
		else if (strcmp(token, "SIM_DURATION") == 0) {
			token = strtok(NULL, delim);
			shm_info_set_sim_duration(inf, (int)strtol(token, NULL, 10));
			printf("appena salvato sim duration %d.\n", shm_info_get_sim_duration(inf));
		}
		else if (strcmp(token, "ENERGY_EXPLODE_THRESHOLD") == 0) {
			token = strtok(NULL, delim);
			shm_info_set_energy_explode_trashold(inf, (int)strtol(token, NULL, 10));
			printf("appena salvato energy explode threshold %d.\n", shm_info_get_energy_explode_trashold(inf));
	
		}	
		else if (strcmp(token, "STEP_ATTIVATORE") == 0) {
			token = strtok(NULL, delim);
			shm_info_set_step_attivatore(inf, (int)strtol(token, NULL, 10));
			printf("appena salvato step attivatore %d.\n", shm_info_get_step_attivatore(inf));
		}
	}
}

//inf è l'istanza di tipo shm_info_t puntata, *inf è il puntatore che la punta e **inf e il puntatore di puntatore che punta a *inf
int shm_info_attach(shm_info_t **inf){//questo metodo fai sia create che attach alla mem condivisa con la kay shm_info_key e un segmento di grandezzashm_info_t
	int shm_id;

	shm_id = shm_create(SHM_INFO_KEY, sizeof(shm_info_t));
	if (shm_id == -1) {
		dprintf(1, "do something\n");
		
	}
	//*inf non è nient'altro che un puntatore di tipo shm_info_t salvato nella struct stats della funzione chiamante, la struct che contiene puntatori a tutti i segmenti
	//di mem condivisa che servono al processo chiamante 
	*inf = (shm_info_t *)shm_attach(shm_id);//non bisogna fare return di *inf perche è puntato da **inf che è gia visibile al chaiamnte? e quindi puo accedere a *inf
	//+ castato in shm_info_t * perche mi restituisce un puntatore a void void * (cioè di tipo generico)quindi bisogna indicargli che tipo di puntatore deve dientare
	shm_info_set_id( *inf); //settiamo l'id nella mem cond, bisognerà toglierlo da qui e metterlo nel mrtodo che legge da path che userà solo il master all'avvio
	return shm_id;
}
//se al posto del puntatore avessimo messo direttament la struct inf allora avrebbe creato una copia della struct e avrebbe modificato la copia ma non la struct originaria, invece inviando il puntatore o l'indirizzo della struct noi la modifichiamo direttamente, evitando copie e quindi risparmiando tempo e memoria
void msg_q_a_a_init(shm_info_t *inf){//quando inviamo un puntatore ad una struttura, per accedere direttamente ai campi della struttua si usa la notazione inf->msg_q_atom_activator_id vhe indiche che msg_q_atom_activator_è l'elemento della struttura da accesdere
	inf->msg_q_atom_activator_id = msg_comunication_atom_activator_init();//questa funz restituisce l'id della msg q appena creata e la salva in mem condivisa rendendola visibile a tutti quelli che sono attaccati
}

void shm_info_detach(shm_info_t *inf){
	shm_detach(inf);
}

void shm_info_delete(shm_info_t *inf){
	int shm_id = shm_id_get(inf);
	//printf("shm_id da cancellare %d", shm_id);
	shm_delete(shm_id);
}

void shm_sem_init(shm_info_t *inf){// cera i semafori 
	/* Semaphores */
	inf->sem_start_id = sem_create(SEM_ID_READY, 10);
	sem_setval(inf->sem_start_id, 0, 0);	// process semaphore 
	sem_setval(inf->sem_start_id, 1, 0);	// simulation semaphore
	sem_setval(inf->sem_start_id, 2, 0);    // contatore processi semaphore

	sem_setval(inf->sem_start_id, 3, 1);    // energy_prod_tot sem 
	sem_setval(inf->sem_start_id, 4, 1);    // energy_prod_laste_sec sem
	sem_setval(inf->sem_start_id, 5, 1);    // waste_tot
	sem_setval(inf->sem_start_id, 6, 1);    // waste_last_sec

	sem_setval(inf->sem_start_id, 7, 1);    // end simulation sem

	sem_setval(inf->sem_start_id, 8, 1);    // n_activation_tot
	sem_setval(inf->sem_start_id, 9, 1);    // n_split_tot


}

/*int shm_sem_ready(shm_info_t *inf){// controlla che il semaforo process sia pronto= abbia valore di numatomsinit+2
	// Semaphores
	int num_process = shm_info_get_n_atoms_init(inf)+2;
	while (sem_getval(inf->sem_start_id, 0) < num_process) {
		printf("semaphore processi atom stampati da shm ogni secondo: %d\n", sem_getval(inf->sem_start_id, 0));
		sleep(1);
    }
	return 0;
}*/

// Setters
static void shm_info_set_id(shm_info_t *inf){inf->shm_info_id = shm_create(SHM_INFO_KEY, 0);}//shm_create usa shmget che ci restituirà l'id della mem condivisa se è 
                                            //gia stata creata, noi sfruttiamo questo meccanismo per farci restituire l'id e salvarlo in memoria condivisa così che 
											//altri processi che devono usarla ci si possono attaccare con funzione attach e l'id
void shm_info_set_energy_demand(shm_info_t *inf,int energ_demand){inf->energy_demand=energ_demand;}
void shm_info_set_n_atoms_init(shm_info_t *inf, int num_atoms_init){inf->n_atoms_init=num_atoms_init;}
void shm_info_set_n_atom_max(shm_info_t *inf, int num_atom_max){inf->n_atom_max=num_atom_max;}
void shm_info_set_min_n_atoms(shm_info_t *inf, int min_num_atoms){inf->min_n_atoms=min_num_atoms;}
void shm_info_set_n_new_atoms(shm_info_t *inf, int num_new_atoms){inf->n_new_atoms=num_new_atoms;}
void shm_info_set_sim_duration(shm_info_t *inf, int simulation_duration){inf->sim_duration=simulation_duration;}
void shm_info_set_energy_explode_trashold(shm_info_t *inf, int nrg_explode_trashold){inf->energy_explode_trashold=nrg_explode_trashold;}
void shm_info_set_step(shm_info_t *inf, long step_n_sec){inf->step=step_n_sec;}
void shm_info_set_step_attivatore(shm_info_t *inf, int step_sec){inf->step_attivatore=step_sec;}

void shm_info_set_energy_prod_tot(shm_info_t *inf, int energy_prod){inf->energy_prod_tot=energy_prod;}
void shm_info_set_energy_prod_laste_sec(shm_info_t *inf, int energy_prod_sec){inf->energy_prod_laste_sec=energy_prod_sec;}
void shm_info_set_waste_tot(shm_info_t *inf, int waste){inf->waste_tot=waste;}
void shm_info_set_waste_last_sec(shm_info_t *inf, int waste_sec){inf->waste_last_sec=waste_sec;}
void shm_info_set_n_activation_tot(shm_info_t *inf, int n_activation){inf->n_activation_tot=inf->n_activation_tot+n_activation;}
void shm_info_set_n_activation_last_sec(shm_info_t *inf, int activation_last_sec){inf->n_activation_last_sec=activation_last_sec;}
void shm_info_set_n_split_tot(shm_info_t *inf, int n_split){inf->n_split_tot=inf->n_split_tot+n_split;}
void shm_info_set_n_split_last_sec(shm_info_t *inf, int split_last_sec){inf->n_split_last_sec=split_last_sec;}

//getters
//void shm_info_get_id(shm_info_t +inf)
int msg_q_a_a_id_get(shm_info_t *inf){return inf->msg_q_atom_activator_id;}
int shm_id_get(shm_info_t *inf){return inf->shm_info_id;}

int shm_info_get_energy_demand(shm_info_t *inf){return inf->energy_demand;}
int shm_info_get_n_atoms_init(shm_info_t *inf){return inf->n_atoms_init;}
int shm_info_get_n_atom_max(shm_info_t *inf){return inf->n_atom_max;}
int shm_info_get_min_n_atoms(shm_info_t *inf){return inf->min_n_atoms;}
int shm_info_get_n_new_atoms(shm_info_t *inf){return inf->n_new_atoms;}
int shm_info_get_sim_duration(shm_info_t *inf){return inf->sim_duration;}
int shm_info_get_energy_explode_trashold(shm_info_t *inf){return inf->energy_explode_trashold;}
long shm_info_get_step(shm_info_t *inf){return inf->step;}
int shm_info_get_step_attivatore(shm_info_t *inf){return inf->step_attivatore;}

int shm_sem_get_startid(shm_info_t *inf){return inf->sem_start_id;}

int shm_info_get_energy_prod_tot(shm_info_t *inf){return inf->energy_prod_tot;}
int shm_info_get_energy_prod_laste_sec(shm_info_t *inf){return inf->energy_prod_laste_sec;}
int shm_info_get_waste_tot(shm_info_t *inf){return inf->waste_tot;}
int shm_info_get_waste_last_sec(shm_info_t *inf){return inf->waste_last_sec;}
int shm_info_get_n_activation_tot(shm_info_t *inf){return inf->n_activation_tot;}
int shm_info_get_n_activation_last_sec(shm_info_t *inf){return inf->n_activation_last_sec;}
int shm_info_get_n_split_tot(shm_info_t *inf){return inf->n_split_tot;}
int shm_info_get_n_split_last_sec(shm_info_t *inf){return inf->n_split_last_sec;}