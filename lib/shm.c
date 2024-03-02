#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shm.h"

int shm_create(key_t key, size_t size){ //crea segmento mem cond con shmget e ritorna l'id
	int res;
	if ((res = shmget(key, size, 0660 | IPC_CREAT)) == -1) {
		dprintf(2, "shm.c - shm_create() : Failed to create SHM segment.\n");
		perror("shmget");
	}else{
		printf("memoria condivisa creata con id %d\n", res);
	}
	return res;
}

void shm_delete(int id_shm){ //cancella il segmento mem condivisa grazie al flag IPC_RMID
	printf("mem condivisa da cancellara id %d\n", id_shm);
	if (shmctl(id_shm, IPC_RMID, NULL) == -1) {
		dprintf(2, "shm.c - shm_delete() : Failed to delete SHM segment.\n");
		perror("shmctl");
	}else{
		printf("memoria condivisa cancellata\n");
	}
}

void *shm_attach(int id_shm){   //restituisce un puntatore generico che punta alla mem condivisa di id id_shm
	void *res;//il cast a puntatore a specifico tipo di mem condivisa puntata va fatto dal chiamante e non da questa funzione che appunto è in una libreria
	if ((res =shmat(id_shm, NULL, 0)) == ((void *) -1)){
		dprintf(2, "shm.c - shm_attach() : Failed to attach SHM segment.\n");
		perror("shmat");
	}else{
		printf("memoria condivisa attaccata\n");
	}
	return res;
}

void shm_detach(void *shm_ptr){ //contrario di attach
	if (shmdt(shm_ptr) == -1) {
		dprintf(2, "shm.c - shm_detach() : Failed to detach SHM segment.\n");
		perror("shmdt");
	}else{
		printf("memoria condivisa distaccata\n");
	}
}