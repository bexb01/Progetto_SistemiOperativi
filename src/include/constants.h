#ifndef CONSTANTS_H
#define CONSTANTS_H 
//costanti utilizzate per rappresentare chiavi di memoria condivisa (shared memory keys), 
//chiavi di semaforo (semaphore keys), chiavi di coda di messaggi (message queue keys), 
//le chiavi sono usate dal so come identificatore univoco  delle risorseipc, ad ogni esecuzione
//assegna alla key un id che viene usato poi dai processi, la key è univoca, id cambia per ogni esecuzione

#define SHM_INFO_KEY 0x1fffffff
#define SEM_ID_READY 0x00ffffff

#define MSG_ACTIV_ATOM 0x100fffff

#endif