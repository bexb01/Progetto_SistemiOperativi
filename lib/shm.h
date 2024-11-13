//Library that provides a more user friendly interface for System V shared memory.
#ifndef OS_PROJECT_SHM_H
#define OS_PROJECT_SHM_H

#include <sys/shm.h>
#include <sys/stat.h>

int shm_create(key_t key, size_t size);

void shm_delete(int id_shm);

void *shm_attach(int id_shm);

void shm_detach(void *shm_ptr);

#endif