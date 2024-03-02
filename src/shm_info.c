#include "../lib/shm.h"
#include <stdio.h>
#include "include/constants.h"
#include "include/shm_info.h"
#include "include/msg_comunication.h"

struct shm_inf{

	int shm_info_id;
	int msg_q_atom_activator_id;
};

//inf è l'istanza di tipo shm_info_t puntata, *inf è il puntatore che la punta e **inf e il puntatore di puntatore che punta a *inf
void shm_info_attach(shm_info_t **inf){//questo metodo fai sia create che attach alla mem condivisa con la kay shm_info_key e un segmento di grandezzashm_info_t
	int shm_id;

	shm_id = shm_create(SHM_INFO_KEY, sizeof(shm_info_t));
	if (shm_id == -1) {
		dprintf(1, "do something\n");
	}
	//*inf non è nient'altro che un puntatore di tipo shm_info_t salvato nella struct stats della funzione chiamante, la struct che contiene puntatori a tutti i segmenti
	//di mem condivisa che servono al processo chiamante 
	*inf = (shm_info_t *)shm_attach(shm_id);//non bisogna fare return di *inf perche è puntato da **inf che è gia visibile al chaiamnte? e quindi puo accedere a *inf
	//+ castato in shm_info_t * perche mi restituisce un puntatore a void void * (cioè di tipo generico)quindi bisogna indicargli che tipo di puntatore deve dientare
	shm_info_set_id( *inf); //settiamo l'id nella mem cond, bisognerà toglierlo da qui e metterlo nel mrtodo che legge da path che userà solo il master all'avvio
}
//se al posto del puntatore avessimo messo direttament la struct inf allora avrebbe creato una copia della struct e avrebbe modificato la copia ma non la struct originaria, invece inviando il puntatore o l'indirizzo della struct noi la modifichiamo direttamente, evitando copie e quindi risparmiando tempo e memoria
void msg_q_a_a_init(shm_info_t *inf){//quando inviamo un puntatore ad una struttura, per accedere direttamente ai campi della struttua si usa la notazione inf->msg_q_atom_activator_id vhe indiche che msg_q_atom_activator_è l'elemento della struttura da accesdere
	inf->msg_q_atom_activator_id = msg_comunication_atom_activator_init();//questa funz restituisce l'id della msg q appena creata e la salva in mem condivisa rendendola visibile a tutti quelli che sono attaccati
}

void shm_info_detach(shm_info_t *inf){
	shm_detach(inf);
}

void shm_info_delete(shm_info_t *inf){
	int shm_id = shm_id_get(inf);
	printf("shm_id da cancellare %d", shm_id);
	shm_delete(shm_id);
}

// Setters
static void shm_info_set_id(shm_info_t *inf){inf->shm_info_id = shm_create(SHM_INFO_KEY, 0);}//shm_create usa shmget che ci restituirà l'id della mem condivisa se è 
                                            //gia stata creata, noi sfruttiamo questo meccanismo per farci restituire l'id e salvarlo in memoria condivisa così che 
											//altri processi che devono usarla ci si possono attaccare con funzione attach e l'id

//getters
//void shm_info_get_id(shm_info_t +inf)
int msg_q_a_a_id_get(shm_info_t *inf){return inf->msg_q_atom_activator_id;}
int shm_id_get(shm_info_t *inf){return inf->shm_info_id;}