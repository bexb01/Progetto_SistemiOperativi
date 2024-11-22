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
	int n_activation_tot, n_activation_last_sec, n_split_tot, n_split_last_sec, energy_prod_tot, energy_prod_laste_sec, energy_prod,
		energy_cons_tot, energy_cons_last_sec, waste_tot, waste_last_sec, energy_inhibitor, n_split_blocked, n_waste_after_split;
	long step;
	int shm_info_id;
	int msg_q_atom_activator_id;
	int sem_start_id;
	pid_t inhibitor_pid;
};

void param_init(char * file_path, shm_info_t *inf){
	FILE * param_file;
	char buffer[100];
	if ((param_file = fopen(file_path, "r")) == NULL) {
		fprintf(stderr, " Errore in apertura param_file.\n");
	}
	printf("caricando parametri di configurazione.\n");
	while(fgets(buffer, sizeof(buffer), param_file) != NULL){

		const char delim[] = " : ";
		char *token = strtok(buffer, delim);
		
		if (strcmp(token, "ENERGY_DEMAND") == 0) {
			token = strtok(NULL, delim);
    		shm_info_set_energy_demand(inf, (int)strtol(token, NULL, 10));
		}
		else if (strcmp(token, "N_ATOMS_INIT") == 0) {
			token = strtok(NULL, delim);
			shm_info_set_n_atoms_init(inf, (int)strtol(token, NULL, 10));
		}
		else if (strcmp(token, "N_ATOM_MAX") == 0) {
			token = strtok(NULL, delim);
			shm_info_set_n_atom_max(inf, (int)strtol(token, NULL, 10));
		}
		else if (strcmp(token, "STEP_ALIMENTAZIONE") == 0) {
			token = strtok(NULL, delim);
			shm_info_set_step(inf, strtol(token, NULL, 10));
		}
		else if (strcmp(token, "MIN_N_ATOMICO") == 0) {
			token = strtok(NULL, delim);
			shm_info_set_min_n_atoms(inf, (int)strtol(token, NULL, 10));
		}
		else if (strcmp(token, "N_NEW_ATOMS") == 0) {
			token = strtok(NULL, delim);
			shm_info_set_n_new_atoms(inf, (int)strtol(token, NULL, 10));
		}
		else if (strcmp(token, "SIM_DURATION") == 0) {
			token = strtok(NULL, delim);
			shm_info_set_sim_duration(inf, (int)strtol(token, NULL, 10));
		}
		else if (strcmp(token, "ENERGY_EXPLODE_THRESHOLD") == 0) {
			token = strtok(NULL, delim);
			shm_info_set_energy_explode_trashold(inf, (int)strtol(token, NULL, 10));
		}	
		else if (strcmp(token, "STEP_ATTIVATORE") == 0) {
			token = strtok(NULL, delim);
			shm_info_set_step_attivatore(inf, (int)strtol(token, NULL, 10));
		}
	}
}

int shm_info_attach(shm_info_t **inf){
	int shm_id;

	shm_id = shm_create(SHM_INFO_KEY, sizeof(shm_info_t));
	if (shm_id == -1) {
		dprintf(1, "do something\n");
		
	}
	*inf = (shm_info_t *)shm_attach(shm_id);
	shm_info_set_id( *inf);
	return shm_id;
}
void msg_q_a_a_init(shm_info_t *inf){
	inf->msg_q_atom_activator_id = msg_comunication_atom_activator_init();
}

void shm_info_detach(shm_info_t *inf){
	shm_detach(inf);
}

void shm_info_delete(shm_info_t *inf){
	int shm_id = shm_id_get(inf);
	shm_delete(shm_id);
}

//crea semafori 
void shm_sem_init(shm_info_t *inf){
	inf->sem_start_id = sem_create(SEM_ID_READY, 15);
	sem_setval(inf->sem_start_id, 0, 0);	// process semaphore conta i processi per far iniziare l'esecuzione
	sem_setval(inf->sem_start_id, 1, 0);	// simulation semaphore :1 start; 0 stay
	sem_setval(inf->sem_start_id, 2, 0);    // contatore processi conta tutti i processi attualemnte in esecuzione, serve per la terminazione della simulazione

	sem_setval(inf->sem_start_id, 3, 1);    // controlla accesso a energy_prod_tot 
	sem_setval(inf->sem_start_id, 4, 1);    // controlla accesso a energy_prod_ energy_prod_last_sec
	sem_setval(inf->sem_start_id, 5, 1);    // controlla accesso a waste_tot waste_tot_last_sec
	sem_setval(inf->sem_start_id, 6, 2);    // inhibitor

	sem_setval(inf->sem_start_id, 7, 1);    // end simulation sem: 1 running 0 shutdown

	sem_setval(inf->sem_start_id, 8, 1);    // controlla accesso a n_activation_tot activation_last_sec
	sem_setval(inf->sem_start_id, 9, 1);    // controlla accesso a n_split_tot split_last_sec
	sem_setval(inf->sem_start_id, 10, 1);	// controlla accesso a energy_inhibitor
	sem_setval(inf->sem_start_id, 11, 1);	// controlla accesso a n_split_blocked
	sem_setval(inf->sem_start_id, 12, 1);	// controlla accesso a n_waste_after_split


}

// Setters
static void shm_info_set_id(shm_info_t *inf){inf->shm_info_id = shm_create(SHM_INFO_KEY, 0);}
void shm_info_set_energy_demand(shm_info_t *inf,int energ_demand){inf->energy_demand=energ_demand;}
void shm_info_set_n_atoms_init(shm_info_t *inf, int num_atoms_init){inf->n_atoms_init=num_atoms_init;}
void shm_info_set_n_atom_max(shm_info_t *inf, int num_atom_max){inf->n_atom_max=num_atom_max;}
void shm_info_set_min_n_atoms(shm_info_t *inf, int min_num_atoms){inf->min_n_atoms=min_num_atoms;}
void shm_info_set_n_new_atoms(shm_info_t *inf, int num_new_atoms){inf->n_new_atoms=num_new_atoms;}
void shm_info_set_sim_duration(shm_info_t *inf, int simulation_duration){inf->sim_duration=simulation_duration;}
void shm_info_set_energy_explode_trashold(shm_info_t *inf, int nrg_explode_trashold){inf->energy_explode_trashold=nrg_explode_trashold;}
void shm_info_set_step(shm_info_t *inf, long step_n_sec){inf->step=step_n_sec;}
void shm_info_set_step_attivatore(shm_info_t *inf, int step_sec){inf->step_attivatore=step_sec;}

void shm_info_set_energy_prod(shm_info_t *inf, int energy_product){inf->energy_prod=inf->energy_prod+energy_product;}
void shm_info_set_energy_prod_tot(shm_info_t *inf, int energy_prod){inf->energy_prod_tot=energy_prod;}
void shm_info_set_energy_prod_laste_sec(shm_info_t *inf, int energy_prod_sec){inf->energy_prod_laste_sec=energy_prod_sec;}
void shm_info_set_waste_tot(shm_info_t *inf, int waste){inf->waste_tot=waste;}
void shm_info_set_waste_last_sec(shm_info_t *inf, int waste_sec){inf->waste_last_sec=waste_sec;}
void shm_info_set_n_activation_tot(shm_info_t *inf, int n_activation){inf->n_activation_tot=inf->n_activation_tot+n_activation;}
void shm_info_set_n_activation_last_sec(shm_info_t *inf, int activation_last_sec){inf->n_activation_last_sec=activation_last_sec;}
void shm_info_set_n_split_tot(shm_info_t *inf, int n_split){inf->n_split_tot=inf->n_split_tot+n_split;}
void shm_info_set_n_split_last_sec(shm_info_t *inf, int split_last_sec){inf->n_split_last_sec=split_last_sec;}
void shm_info_set_energy_cons_tot(shm_info_t *inf, int energy_cons){inf->energy_cons_tot=inf->energy_cons_tot+energy_cons;}
void shm_info_set_energy_cons_last_sec(shm_info_t *inf, int energy_cons_sec){inf->energy_cons_last_sec=energy_cons_sec;}
void shm_info_set_n_waste_after_split(shm_info_t *inf, int waste){inf->n_waste_after_split=inf->n_waste_after_split+waste;}
void shm_info_set_n_split_blocked(shm_info_t *inf, int split_blocked){inf->n_split_blocked=inf->n_split_blocked+split_blocked;}
void shm_info_set_energy_inhibitor(shm_info_t *inf, int energy_absorbed){inf->energy_inhibitor=inf->energy_inhibitor+energy_absorbed;}

void shm_info_set_inhibitor_pid(shm_info_t *inf, pid_t inhib_pid){inf->inhibitor_pid=inhib_pid;}
//getters

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

int shm_info_get_energy_prod(shm_info_t *inf){return inf->energy_prod;}
int shm_info_get_energy_prod_tot(shm_info_t *inf){return inf->energy_prod_tot;}
int shm_info_get_energy_prod_laste_sec(shm_info_t *inf){return inf->energy_prod_laste_sec;}
int shm_info_get_waste_tot(shm_info_t *inf){return inf->waste_tot;}
int shm_info_get_waste_last_sec(shm_info_t *inf){return inf->waste_last_sec;}
int shm_info_get_n_activation_tot(shm_info_t *inf){return inf->n_activation_tot;}
int shm_info_get_n_activation_last_sec(shm_info_t *inf){return inf->n_activation_last_sec;}
int shm_info_get_n_split_tot(shm_info_t *inf){return inf->n_split_tot;}
int shm_info_get_n_split_last_sec(shm_info_t *inf){return inf->n_split_last_sec;}
int shm_info_get_energy_cons_tot(shm_info_t *inf){return inf->energy_cons_tot;}
int shm_info_get_energy_cons_last_sec(shm_info_t *inf){return inf->energy_cons_last_sec;}
int shm_info_get_n_waste_after_split(shm_info_t *inf){return inf->n_waste_after_split;}
int shm_info_get_n_split_blocked(shm_info_t *inf){return inf->n_split_blocked;}
int shm_info_get_energy_inhibitor(shm_info_t *inf){return inf->energy_inhibitor;}

pid_t shm_info_get_inhibitor_pid(shm_info_t *inf){return inf->inhibitor_pid;}