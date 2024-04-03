#define _GNU_SOURCE

#include <stdio.h>

#include "include/shm_info.h"
#include "include/msg_comunication.h"
#include "include/constants.h" //file delle costanti per il so


#define MSG_SIZE (sizeof(struct comunication_msg) - sizeof(long))//grandezza della struct - grandezza tipo messaggio che viene inviato insiem al mex sesso come un long, stava nel codice di una lezione
#define MSG_TYPE(type) (type)//creiam una macro che specifica il tipo di messaggio, combacerà al numero atomi nel caso di comunicazione atomo-attivatore

int msg_comunication_atom_activator_init(void){
	int id;
	if ((id = msgget(MSG_ACTIV_ATOM, 0660 | IPC_CREAT | IPC_EXCL)) < 0){//    MSG_ACTIV_ATOM: È la chiave univoca che identifica la coda dei messaggi. Può essere un valore specifico o generato in base a qualche regola nel tuo programma. In questo caso, sembra essere un identificatore specifico, presumibilmente definito altrove nel tuo codice. 0660: Sono i permessi della coda dei messaggi. In questo caso, stai dando al proprietario e al gruppo la possibilità di leggere e scrivere sulla coda dei messaggi, mentre gli altri possono solo leggere. I permessi sono espressi in notazione ottale. IPC_CREAT: Indica che la coda dei messaggi deve essere creata se non esiste già. Se la coda dei messaggi esiste già con la stessa chiave, questa opzione viene ignorata. IPC_EXCL: Garantisce che la creazione della coda dei messaggi fallirà se la coda esiste già con la stessa chiave. Questa opzione è spesso utilizzata insieme a IPC_CREAT per evitare conflitti con code già esistenti.
		dprintf(2, "msg_comunication.c - msg_comunication_atom_activator_init: Failed to create message queue.\n");
	}else{
		printf("coda di messaggi creata con id %d\n", id);
	}
	return id;
}

void msg_queue_remove(shm_info_t *inf){
	int msgq_id=msg_q_a_a_id_get( inf);
	if (msgctl(msgq_id, IPC_RMID, NULL)<0){
		dprintf(2, "msg_comunication.c - msg_comunication_remove: Failed to remove message queue.\n");
	}else{
		printf("coda di messaggi eliminata \n");
	}
}

//struct che compone il messaggio
struct comunication_msg create_msg_comunication(int  receiver_atomic_n, int sender_id, int boolean_split){
	struct comunication_msg ret;
	ret.receiver = MSG_TYPE(receiver_atomic_n);
	ret.sender = sender_id;
    ret.bool_split = boolean_split;
	printf("messaggio composto \n");
	return ret;
}

void msg_comunication_snd(int msg_q_id, struct comunication_msg *msg_snd){
	int ret;
	do {
		ret = msgsnd(msg_q_id, msg_snd, MSG_SIZE, 0);
	} while (ret < 0);//send del messaggio che continua a fare send fino a quando il messaggio non si è inviato
	//printf("messaggio inviato sulla coda di id %d\n", msg_q_id);
}

int msg_comunication_rcv(int msg_q_id, int type, int *sender_id, int *boolean_split){
	long ret;
	struct comunication_msg msg_rcv;
	int i=0;
	printf("sto per cercare di ricevere su coda id %d e messaggio tipo %d\n",msg_q_id,type);
	//do {
		//printf("msg_comunication_rcv dopo questa sta in rcv %d %d\n", msg_q_id, type);
		ret = msgrcv(msg_q_id, &msg_rcv, MSG_SIZE, 0, 0);//il quarto  parametro è il tipo di messaggio, mettendolo a 0 non verrà piu effettuato il controllo sul tipo ricevuto=ora non c'è bisogno di specificare il tipo di messaggio quindi il primo atomo a caso che riceve il messaggio lo cancella dalla coda messaggi
		//printf("msg_comunication_rcv aspetta messaggio su coda %d di tipo %d\n", msg_q_id, type);
		/*if (!restarting && ret < 0)
			return FALSE;*/
		//i=i+1;
	//} while(ret < 0 /*&& i<=1000*/);
	if(ret>0){
	printf("MESSAGGIO RICEVUTO00000000000000\n");
	  	*sender_id = msg_rcv.sender;
		*boolean_split= msg_rcv.bool_split;
	}else if (ret == -1) {
        perror("msgrcv");
	//if(i!=1000){
		*sender_id = 0;
		*boolean_split= 0;
	}

	return ret;
}