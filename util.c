#include "main.h"
#include "util.h"
MPI_Datatype MPI_PAKIET_T;


state_t stan=InNothin; //stan początkowy

/* zamek wokół zmiennej współdzielonej między wątkami. 
 * Każdy proces ma osobą pamięć, ale w ramach jednego
 * procesu wątki współdzielą zmienne - więc dostęp do nich jest objety w mutexach
 */
pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;

struct tagNames_t{
    const char *name;
    int tag;
} tagNames[] = { { "number domu", NUM }, 
                { "potwierdzenie", ACK}, {"prośbę o sekcję krytyczną", REQ}, {"zwolnienie sekcji krytycznej", RET} };

const char *const tag2string( int tag )
{
    for (int i=0; i <sizeof(tagNames)/sizeof(struct tagNames_t);i++) {
	if ( tagNames[i].tag == tag )  return tagNames[i].name;
    }
    return "<unknown>";
}
/* tworzy typ MPI_PAKIET_T
*/
void inicjuj_typ_pakietu()
{
   
    int blocklengths[NITEMS] = {1,1,1};
    MPI_Datatype typy[NITEMS] = {MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint     offsets[NITEMS]; 
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, type);

    MPI_Type_create_struct(NITEMS, blocklengths, offsets, typy, &MPI_PAKIET_T);

    MPI_Type_commit(&MPI_PAKIET_T);
}

void sendPacket(packet_t *pkt, int destination, int tag)
{
    int freepkt=0;
    if (pkt==0) { pkt = malloc(sizeof(packet_t)); freepkt=1;}
    pkt->src = rank;
    MPI_Send( pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
    debug("Wysyłam %s do %d\n", tag2string( tag), destination);
    if (freepkt) free(pkt);
}

void changeState( state_t newState )
{
    pthread_mutex_lock( &stateMut );
    if (stan==InFinish) { 
	pthread_mutex_unlock( &stateMut );
        return;
    }
    stan = newState;
    pthread_mutex_unlock( &stateMut );
}

/*implementacja kolejki*/
queue_t houseQueue;
queue_t paserQueue;
queue_t numberQueue;
int total_threads;

static int comparePackets(const void *a, const void *b) {
    packet_t *p1 = (packet_t*)a;
    packet_t *p2 = (packet_t*)b;
    
    if (p1->ts != p2->ts)
        return p1->ts - p2->ts;
    return p1->src - p2->src;
}

void initQueue(int thread_count) {
    total_threads = thread_count;
    
    // Initialize houseQueue
    houseQueue.capacity = thread_count;
    houseQueue.packets = (packet_t*)malloc(sizeof(packet_t) * thread_count);
    houseQueue.size = 0;
    pthread_mutex_init(&houseQueue.mutex, NULL);
    
    // Initialize paserQueue
    paserQueue.capacity = thread_count;
    paserQueue.packets = (packet_t*)malloc(sizeof(packet_t) * thread_count);
    paserQueue.size = 0;
    pthread_mutex_init(&paserQueue.mutex, NULL);
    
    // Initialize numberQueue
    numberQueue.capacity = thread_count;
    numberQueue.packets = (packet_t*)malloc(sizeof(packet_t) * thread_count);
    numberQueue.size = 0;
    pthread_mutex_init(&numberQueue.mutex, NULL);
}

void destroyQueue() {
    pthread_mutex_lock(&houseQueue.mutex);
    free(houseQueue.packets);
    pthread_mutex_unlock(&houseQueue.mutex);
    pthread_mutex_destroy(&houseQueue.mutex);
    
    pthread_mutex_lock(&paserQueue.mutex);
    free(paserQueue.packets);
    pthread_mutex_unlock(&paserQueue.mutex);
    pthread_mutex_destroy(&paserQueue.mutex);
    
    pthread_mutex_lock(&numberQueue.mutex);
    free(numberQueue.packets);
    pthread_mutex_unlock(&numberQueue.mutex);
    pthread_mutex_destroy(&numberQueue.mutex);
}

int enqueueWithPriority(queue_t *queue, packet_t packet) {
    pthread_mutex_lock(&queue->mutex);
    
    for (int i = 0; i < queue->size; i++) {
        if (queue->packets[i].src == packet.src) {
            if (packet.ts < queue->packets[i].ts) {
                queue->packets[i] = packet;
                qsort(queue->packets, queue->size, sizeof(packet_t), comparePackets);
            }
            pthread_mutex_unlock(&queue->mutex);
            return 1;
        }
    }
    if (queue->size >= queue->capacity) {
        pthread_mutex_unlock(&queue->mutex);
        return 0;
    }
    
    queue->packets[queue->size] = packet;
    queue->size++;
    qsort(queue->packets, queue->size, sizeof(packet_t), comparePackets);
    
    pthread_mutex_unlock(&queue->mutex);
    return 1;
}


void clearQueueBySrc(queue_t *queue, int src) {
    pthread_mutex_lock(&queue->mutex);
    
    int write = 0;
    for (int read = 0; read < queue->size; read++) {
        if (queue->packets[read].src != src) {
            if (write != read) {
                queue->packets[write] = queue->packets[read];
            }
            write++;
        }
    }
    queue->size = write;
    
    pthread_mutex_unlock(&queue->mutex);
}

void clearQueue(queue_t *queue) {
    pthread_mutex_lock(&queue->mutex);
    queue->size = 0;
    pthread_mutex_unlock(&queue->mutex);
}

int lamport_clock = 0;
pthread_mutex_t lamport_mutex = PTHREAD_MUTEX_INITIALIZER;

void increment_lamport() {
    pthread_mutex_lock(&lamport_mutex);
    lamport_clock++;
    pthread_mutex_unlock(&lamport_mutex);
}

void update_lamport(int received_timestamp) {
    pthread_mutex_lock(&lamport_mutex);
    lamport_clock = (received_timestamp > lamport_clock ? received_timestamp : lamport_clock) + 1;
    pthread_mutex_unlock(&lamport_mutex);
}

int my_house_number = -1;
pthread_mutex_t house_number_mutex = PTHREAD_MUTEX_INITIALIZER;

void set_house_number(int number) {
    pthread_mutex_lock(&house_number_mutex);
    my_house_number = number;
    pthread_mutex_unlock(&house_number_mutex);
}

int get_house_number() {
    pthread_mutex_lock(&house_number_mutex);
    int number = my_house_number;
    pthread_mutex_unlock(&house_number_mutex);
    return number;
}

int ACK_number = 0;
pthread_mutex_t ACK_number_mutex = PTHREAD_MUTEX_INITIALIZER;

void set_ACK_number(int number) {
    pthread_mutex_lock(&ACK_number_mutex);
    ACK_number = number;
    pthread_mutex_unlock(&ACK_number_mutex);
}

int get_ACK_number() {
    pthread_mutex_lock(&ACK_number_mutex);
    int number = ACK_number;
    pthread_mutex_unlock(&ACK_number_mutex);
    return number;
}