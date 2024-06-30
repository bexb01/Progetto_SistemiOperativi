#ifndef OS_PROJECT_SHM_GENERAL_H
#define OS_PROJECT_SHM_GENERAL_H


typedef struct shm_inf shm_info_t; //shm_info_t adesso è l'allias di shm_info

int shm_info_attach(shm_info_t **inf);

void param_init(char * file_path, shm_info_t *inf);

void msg_q_a_a_init(shm_info_t *inf);

void shm_info_detach(shm_info_t *inf);

void shm_info_delete(shm_info_t *inf);

void shm_sem_init(shm_info_t *inf);

//int shm_sem_ready(shm_info_t *inf);

static void shm_info_set_id(shm_info_t *inf);
void shm_info_set_energy_demand(shm_info_t *inf,int energ_demand);
void shm_info_set_n_atoms_init(shm_info_t *inf, int num_atoms_init);
void shm_info_set_n_atom_max(shm_info_t *inf, int num_atom_max);
void shm_info_set_min_n_atoms(shm_info_t *inf, int min_num_atoms);
void shm_info_set_n_new_atoms(shm_info_t *inf, int num_new_atoms);
void shm_info_set_sim_duration(shm_info_t *inf, int simulation_duration);
void shm_info_set_energy_explode_trashold(shm_info_t *inf, int nrg_explode_trashold);
void shm_info_set_step(shm_info_t *inf, long step_n_sec);
void shm_info_set_step_attivatore(shm_info_t *inf, int step_sec);

void shm_info_set_energy_prod_tot(shm_info_t *inf, int energy_prod);
void shm_info_set_energy_prod_laste_sec(shm_info_t *inf, int energy_prod_sec);
void shm_info_set_waste_tot(shm_info_t *inf, int waste);
void shm_info_set_waste_last_sec(shm_info_t *inf, int waste_sec);
void shm_info_set_n_activation_tot(shm_info_t *inf, int n_activation);
void shm_info_set_n_activation_last_sec(shm_info_t *inf, int activation_last_sec);
void shm_info_set_n_split_tot(shm_info_t *inf, int n_split);
void shm_info_set_n_split_last_sec(shm_info_t *inf, int split_last_sec);
int msg_q_a_a_id_get(shm_info_t *inf);
int shm_id_get(shm_info_t *inf);

int shm_info_get_energy_demand(shm_info_t *inf);
int shm_info_get_n_atoms_init(shm_info_t *inf);
int shm_info_get_n_atom_max(shm_info_t *inf);
int shm_info_get_min_n_atoms(shm_info_t *inf);
int shm_info_get_n_new_atoms(shm_info_t *inf);
int shm_info_get_sim_duration(shm_info_t *inf);
int shm_info_get_energy_explode_trashold(shm_info_t *inf);
long shm_info_get_step(shm_info_t *inf);
int shm_sem_get_startid(shm_info_t *inf);
int shm_info_get_step_attivatore(shm_info_t *inf);

int shm_info_get_energy_prod_tot(shm_info_t *inf);
int shm_info_get_energy_prod_laste_sec(shm_info_t *inf);
int shm_info_get_waste_tot(shm_info_t *inf);
int shm_info_get_waste_last_sec(shm_info_t *inf);
int shm_info_get_n_activation_tot(shm_info_t *inf);
int shm_info_get_n_activation_last_sec(shm_info_t *inf);
int shm_info_get_n_split_tot(shm_info_t *inf);
int shm_info_get_n_split_last_sec(shm_info_t *inf);

#endif