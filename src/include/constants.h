#ifndef CONSTANTS_H
#define CONSTANTS_H
//file di intestazione (header file) in C che definisce diverse costanti utilizzate nel progetto. 
//Queste costanti sono utilizzate per rappresentare chiavi di memoria condivisa (shared memory keys), 
//chiavi di semaforo (semaphore keys), chiavi di coda di messaggi (message queue keys),
//e segnali (signals) nel progetto. le chiavi sono usate dal so come identificatore univoco  delle risorse
//ipc, ad ogni esecuzione assegna alla key un id che viene usato poi dai processi, 
//la key è univoca, id cambia per ogni esecuzione

#define SHM_INFO_KEY 0x1fffffff
#define SEM_ID_READY 0x00ffffff
/*#define SHM_DATA_PORTS_KEY 0x2fffffff
#define SHM_DATA_SHIPS_KEY 0x3fffffff
#define SHM_DATA_CARGO_KEY 0x4fffffff
#define SHM_DATA_PORT_OFFER_KEY 0x5fffffff
#define SHM_DATA_DEMAND_KEY 0x6fffffff

#define SEM_PORTS_INITIALIZED_KEY 0x00ffffff
#define SEM_START_KEY 0x10ffffff
#define SEM_DOCK_KEY 0x11ffffff
#define SEM_CARGO_KEY 0x12ffffff*/

#define MSG_ACTIV_ATOM 0x100fffff
//#define MSG_OUT_PORT_KEY 0x110fffff

/*#define SIGDAY SIGUSR1
#define SIGSWELL SIGUSR2
#define SIGSTORM SIGUSR2
#define SIGMAELSTROM SIGTERM

#define NUM_CONST 16*/

#endif