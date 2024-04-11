#ifndef OS_PROJECT_SEMAPHORE_H
#define OS_PROJECT_SEMAPHORE_H

#include <sys/sem.h>

int sem_create(key_t sem_key, int nsems);

int sem_get_id(key_t key);

void sem_setval(id_t sem_id, int sem_index, int value);

int sem_getval(id_t sem_id, int sem_index);

void sem_execute_semop(id_t sem_id, int sem_index, int op_val, int flags);

void sem_delete(id_t sem_id);


#endif