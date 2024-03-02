#ifndef OS_PROJECT_SHM_GENERAL_H
#define OS_PROJECT_SHM_GENERAL_H


typedef struct shm_inf shm_info_t; //shm_info_t adesso è l'allias di shm_info

void shm_info_attach(shm_info_t **inf);

void msg_q_a_a_init(shm_info_t *inf);

void shm_info_detach(shm_info_t *inf);

void shm_info_delete(shm_info_t *inf);

static void shm_info_set_id(shm_info_t *inf);

int msg_q_a_a_id_get(shm_info_t *inf);

int shm_id_get(shm_info_t *inf);

#endif