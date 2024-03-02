#ifndef OS_PROJECT_MSG_COMMERCE_H 
#define OS_PROJECT_MSG_COMMERCE_H

#include <sys/msg.h>
#include "shm_info.h"

/*enum status { //non so se ci servirà
	// For port to ship messages 
	STATUS_ACCEPTED,
	STATUS_PARTIAL,
	STATUS_REFUSED,
	// For ship to port messages 
	STATUS_SELL,
	STATUS_BUY
};*/

struct comunication_msg {
	long receiver; //potrebbe essere il num atomico di chi vogliamo splittare, cosi splittiamo tutti coloro che hanno un certo num atomico
	int sender;  //pid del proc?
    int bool_split;  //1 o 0 in base  a se gli diciamo di splittare? bho
};


// Initialize an incoming message queue. return The identifier of the initialized message queue.
 
int msg_comunication_atom_activator_init(void);

void msg_queue_remove(shm_info_t *inf);

struct comunication_msg create_msg_comunication(int receiver_atomic_n, int sender_id, int boolean_split);
 // @brief Initialize an outgoing message queue@return The identifier of the initialized message queue.
//int msg_comunication_activator(void);
void msg_comunication_snd(int msg_q_id, struct comunication_msg *msg_snd);


int msg_comunication_rcv(int msg_q_id, int type, int *sender_id, int *boolean_split);

#endif