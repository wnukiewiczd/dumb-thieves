#include "main.h"
#include "watek_komunikacyjny.h"

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    packet_t pakiet;
    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while ( stan!=InFinish ) {
	debug("czekam na recv");
        MPI_Recv( &pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        switch ( status.MPI_TAG ) {
	    case REQ:
                update_lamport(pakiet.ts);
                packet_t ack_pkt;
                ack_pkt.ts = lamport_clock;
                ack_pkt.src = rank;
                if (stan == InHouse) {
                    ack_pkt.type = house;
                } else if (stan == InPaser) {
                    ack_pkt.type = paser;
                }
                sendPacket(&ack_pkt, status.MPI_SOURCE, ACK);
                if (pakiet.type == house) {
                    enqueueWithPriority(&houseQueue, pakiet);
                } else if (pakiet.type == paser) {
                    enqueueWithPriority(&paserQueue, pakiet);
                }
	            break;
	    case ACK:
                debug("Otrzymano ACK od %d", status.MPI_SOURCE);
                int number = get_ACK_number();
                set_ACK_number(number + 1);
                update_lamport(pakiet.ts);
                if (stan == InHouse) {
                    if (pakiet.type == house || pakiet.type == paser) {
                        enqueueWithPriority(&houseQueue, pakiet);
                    }
                } 
                else if (stan == InPaser) {
                    if (pakiet.type == paser) {
                        enqueueWithPriority(&paserQueue, pakiet);
                    }
                }
                break;
        case RET:
                debug("Otrzymano RET od %d", status.MPI_SOURCE);
                update_lamport(pakiet.ts);
                clearQueueBySrc(&houseQueue, status.MPI_SOURCE);
                clearQueueBySrc(&paserQueue, status.MPI_SOURCE);
                break;
        case NUM:
                debug("Otrzymano NUM od %d", status.MPI_SOURCE);
                if (pakiet.type == r_ask) {
                    packet_t response;
                    response.ts = get_house_number();
                    response.src = rank;
                    response.type = r_ans;
                    sendPacket(&response, status.MPI_SOURCE, NUM);
                } 
                else if (pakiet.type == r_ans) {
                    enqueueWithPriority(&numberQueue, pakiet);
                }
                break;
	    default:
	    break;
        }
    }
}
