#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include "semaphore.h"


static struct sembuf create_sembuf(int index, int semop_value, int flags);

int sem_create(key_t sem_key, int nsems) //crea gruppo di semafori
{
	int res;
	if (nsems <= 0) {
		dprintf(2, "semaphore.c - sem_create: nsems must be greater than 0.\n");
		return -1;
	}

	if (( res = semget(sem_key, nsems, 0660 | IPC_CREAT | IPC_EXCL)) < 0) { //crea nsem semafori, sem_key è la chiave univoca che si trova in constants.h, ipc_creat crea l'insieme di semafori associato alla chiave solo se non esiste gia, restituisce l'id se esiste gia, ipc_excl combinato con creat se esiste gia un insieme di sem associato alla chiave semget fallisce restituisce -1 con errore eexist 
		dprintf(2, "semaphore.c - sem_create: Failed to create semaphore array.\n");
	}
	return res; //restituisce id
}

int sem_get_id(key_t key)
{
	int res;
	if ((res = semget(key, 0, 0)) == -1) { //mettendo il secondo e terzo parametro a 0 restituisce l'id dell'insieme di semafori
		dprintf(2, "semaphore.c - sem_get_id: Failed to obtain semaphore array id.\n");
	}
	return res;
}

void sem_setval(id_t sem_id, int sem_index, int value)
{
	if (semctl(sem_id, sem_index, SETVAL, value) < 0) {//usa  semctl per settare il valore iniziale del semaforo di indice em_index
		dprintf(2, "semaphore.c - sem_setval: Failed to set semaphore value\n");
	}
}

int sem_getval(id_t sem_id, int sem_index)
{
	int val;
	if ((val = semctl(sem_id, sem_index, GETVAL)) < 0) {//retituisce il valore del semaforo di indice getval dell' insieme di semafori di id sem_id
	}
	return val;
}

int sem_execute_semop(id_t sem_id, int sem_index, int op_val, int flags)
{
	struct sembuf operation; //sembuf è una struct che specifica i parametri per le operazioni che svolge semop
	operation = create_sembuf(sem_index, op_val, flags); //indica l'indice del semaforo su cui lavorare, il valore da assegnargli e i flag tipo ipc_nowait

	while (semop(sem_id, &operation, 1) == -1) {
        if (errno == EINTR || errno == EAGAIN) {// EINTR: L'operazione è stata interrotta da un segnale. Riprovare. EAGAIN: Il semaforo è occupato, ma può essere ritentato. Riprovare.

        } else if (errno == EIDRM || errno == EINVAL) {// EIDRM: Il set di semafori è stato rimosso. Uscire dal ciclo. EINVAL: ID di semaforo non valido o altre condizioni non riparabili.
            return -1;
        } else {// Qualsiasi altro errore. esce.
			//printf("semop failed\n");
            return -1;
        }
    }
	return 1;
}

void sem_delete(id_t sem_id)
{
	if (semctl(sem_id, 0, IPC_RMID) < 0) { //cancella la lista di semafori specificando l'id
		dprintf(2, "semaphore.c - sem_delete: Failed to delete semaphore set.\n");
	}
}

static struct sembuf create_sembuf(int index, int semop_value, int flags)//popola la struct sembuf utilizzata da sem_ececute_semop
{
	struct sembuf res;
	res.sem_num = index;
	res.sem_op = semop_value;
	res.sem_flg = flags;

	return res;
}