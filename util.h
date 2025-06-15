#ifndef UTILH
#define UTILH
#include "main.h"

/* typ pakietu */
typedef struct {
    int ts;       /* timestamp (zegar lamporta */
    int src;  
    int type;      /*resource type*/
} packet_t;
/* packet_t ma trzy pola, więc NITEMS=3. Wykorzystane w inicjuj_typ_pakietu */
#define NITEMS 3

/*definicja kolejki*/
typedef struct {
    packet_t *packets;
    int capacity; //by ustalić rozmiar *packets
    int size; //ilość elementów
    pthread_mutex_t mutex;
} queue_t;

extern queue_t houseQueue;
extern queue_t paserQueue;
extern queue_t numberQueue;
extern int total_threads;

void initQueue(int thread_count);
void destroyQueue();
int enqueueWithPriority(queue_t *queue, packet_t packet);
void clearQueueBySrc(queue_t *queue, int src);
void clearQueue(queue_t * queue);

/* Typy wiadomości */
/* TYPY PAKIETÓW */
#define ACK 1
#define REQ 2
#define RET 3
#define NUM 4

//Resource type
#define r_ask 1
#define r_ans 2
#define house 3
#define paser 4

extern MPI_Datatype MPI_PAKIET_T;
void inicjuj_typ_pakietu();

/* wysyłanie pakietu, skrót: wskaźnik do pakietu (0 oznacza stwórz pusty pakiet), do kogo, z jakim typem */
void sendPacket(packet_t *pkt, int destination, int tag);

typedef enum {InPaser, InHouse, InNothin, InFinish} state_t;
extern state_t stan;
extern pthread_mutex_t stateMut;
/* zmiana stanu, obwarowana muteksem */
void changeState( state_t );

/* zegar lamporta*/
extern int lamport_clock;
extern pthread_mutex_t lamport_mutex;

void increment_lamport();
void update_lamport(int received_timestamp);

/*numer domu*/
extern int my_house_number;
extern pthread_mutex_t house_number_mutex;

void set_house_number(int number);
int get_house_number();

extern int ACK_number;
extern pthread_mutex_t ACK_number_mutex;

void set_ACK_number(int number);
int get_ACK_number();
#endif
