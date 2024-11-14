#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>

#include "include/shm_info.h"
#include "include/msg_comunication.h"
#include "include/constants.h"

//grandezza della struct - grandezza tipo messaggio che viene inviato insiem al mex stesso come un long, stava nel codice di una lezione
#define MSG_SIZE (sizeof(struct comunication_msg) - sizeof(long))
#define MSG_TYPE(type) (type)//creiam una macro che specifica il tipo di messaggio, combacerà al numero atomi nel caso di comunicazione atomo-attivatore

int msg_comunication_atom_activator_init(void){
	int id;
	if ((id = msgget(MSG_ACTIV_ATOM, 0660 | IPC_CREAT | IPC_EXCL)) < 0){//    MSG_ACTIV_ATOM: È la chiave univoca che identifica la coda dei messaggi.
		dprintf(2, "msg_comunication.c - msg_comunication_atom_activator_init: Failed to create message queue.\n");
	}
	return id;
}

void msg_queue_remove(shm_info_t *inf){
	int msgq_id=msg_q_a_a_id_get( inf);
	if (msgctl(msgq_id, IPC_RMID, NULL)<0){
		dprintf(2, "msg_comunication.c - msg_comunication_remove: Failed to remove message queue.\n");
	}
}

//struct che compone il messaggio
struct comunication_msg create_msg_comunication(int  rec, int boolean_split){
	struct comunication_msg ret;
	ret.receiver = MSG_TYPE(rec);
    ret.bool_split = boolean_split;
	return ret;
}

void msg_comunication_snd(int msg_q_id, struct comunication_msg *msg_snd){
	int ret;
	do {
		ret = msgsnd(msg_q_id, msg_snd, MSG_SIZE, 0);
	} while (ret < 0);//send del messaggio che continua a fare send fino a quando il messaggio non si è inviato
}

int msg_comunication_rcv(int msg_q_id, int *boolean_split){
	long ret;
	struct comunication_msg msg_rcv;
	int i=0;
		ret = msgrcv(msg_q_id, &msg_rcv, MSG_SIZE, 0, IPC_NOWAIT);//con ipc_nowait non è bloccante, se no gli atomi in rcv non muoiono perche rimangono in receive
	if(ret>0){

		*boolean_split= msg_rcv.bool_split;
	}else if (ret == -1) {
		*boolean_split= 0;
        if (errno == ENOMSG) {
			//coda messaggi vuota, riprovo
    	} else {
        	perror("Errore durante la lettura dalla coda dei messaggi");
		}
	}

	return ret;
}