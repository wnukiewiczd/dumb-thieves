#include "main.h"
#include "watek_glowny.h"

//println("Ubiegam się o sekcję krytyczną")
void mainLoop()
{
    srandom(rank);
	sleep(5);
    int tag;
    int perc;
	packet_t pkt;
	int can_enter = 0;
	pthread_mutex_lock(&lamport_mutex);
    lamport_clock = rank;
    pthread_mutex_unlock(&lamport_mutex);
	set_ACK_number(0);
	set_house_number(-1);
	changeState(InNothin);
		while (stan != InFinish) {
		switch (stan) {
			case InNothin:
				changeState(InHouse);
				increment_lamport();
    			set_ACK_number(0);
				pkt.ts = lamport_clock;
				pkt.src = rank;
				pkt.type = house;
				println("Ubiegam się o sekcję krytyczną house");
				enqueueWithPriority(&houseQueue, pkt);
				for (int i = 0; i < size; i++) {
					if (i != rank) {
						sendPacket(&pkt, i, REQ);
					}
				}
				while (get_ACK_number() < size - 1) {
					sleep(1);
				}
				can_enter = 0;
				while (!can_enter) {
					pthread_mutex_lock(&houseQueue.mutex);
					for (int i = 0; i < NUM_HOUSES && i < houseQueue.size; i++) {
						if (houseQueue.packets[i].src == rank) {
							can_enter = 1;
							break;
						}
					}
					pthread_mutex_unlock(&houseQueue.mutex);
					if (!can_enter) sleep(1);
				}


				println("Mogę wziąć dom, wybieram numer");
				// get house number logic

				MPI_Barrier(MPI_COMM_WORLD);

				clearQueue(&numberQueue);
				pkt.ts = lamport_clock;
				pkt.src = rank;
				pkt.type = r_ask;
				for (int i = 0; i < size; i++) {
					if (i != rank) {
						sendPacket(&pkt, i, NUM);
					}
				}
				while (numberQueue.size < size - 1) {
					sleep(1);
				}
				int taken_numbers[NUM_HOUSES] = {-1};
				for(int i = 0; i < NUM_HOUSES; i++) {
					taken_numbers[i] = -1;
				}
				pthread_mutex_lock(&numberQueue.mutex);

				for (int i = 0; i < numberQueue.size; i++) {
					if (numberQueue.packets[i].ts != -1) { 
						taken_numbers[numberQueue.packets[i].ts] =  numberQueue.packets[i].src;
					}
				}
				pthread_mutex_unlock(&numberQueue.mutex);

				int eligible_threads[NUM_HOUSES] = {0};
				int eligible_count = 0;

				pthread_mutex_lock(&houseQueue.mutex);
				for (int i = 0; i < NUM_HOUSES && i < houseQueue.size; i++) {
					eligible_threads[eligible_count++] = houseQueue.packets[i].src;
				}
				pthread_mutex_unlock(&houseQueue.mutex);
				int my_position = -1;
				int position_count = 0;

				for (int i = 0; i < eligible_count; i++) {
					int thread = eligible_threads[i];
					int has_house = 0;
					for (int j = 0; j < NUM_HOUSES; j++) {
						if (taken_numbers[j] == thread) {
							has_house = 1;
							break;
						}
					}
					if (!has_house) {
						if (thread == rank) {
							my_position = position_count;
						}
						position_count++;
					}
				}
				if (my_position != -1) {
					int available_count = 0;
					for (int i = 0; i < NUM_HOUSES; i++) {
						if (taken_numbers[i] == -1) {
							if (available_count == my_position) {
								set_house_number(i);
								println("Otrzymałem dom numer %d", i);

								printf("[%d] ", rank);
								for(int d = 0; d <size; d++){
									printf("(%d, %d) ", houseQueue.packets[d].src, houseQueue.packets[d].ts);
								}
								printf("\n");

								break;
							}
							available_count++;
						}
					}
				}

				break;
			case InHouse:
				changeState(InPaser);
				increment_lamport();
    			set_ACK_number(0);
				pkt.ts = lamport_clock;
				pkt.src = rank;
				pkt.type = paser;
				println("Ubiegam się o sekcję krytyczną paser");
				enqueueWithPriority(&paserQueue, pkt);
				for (int i = 0; i < size; i++) {
					if (i != rank) {
						sendPacket(&pkt, i, REQ);
					}
				}
				while (get_ACK_number() < size - 1) {
					sleep(1);
				}
				can_enter = 0;
				while (!can_enter) {
					pthread_mutex_lock(&paserQueue.mutex);
					for (int i = 0; i < NUM_PASERS && i < paserQueue.size; i++) {
						if (paserQueue.packets[i].src == rank) {
							can_enter = 1;
							break;
						}
					}
					pthread_mutex_unlock(&paserQueue.mutex);
					if (!can_enter) sleep(1);
				}
				println("Wchodzę do pasera");

				break;
			case InPaser:
				set_house_number(-1);
				increment_lamport();
				pkt.ts = lamport_clock;
				pkt.src = rank;
				pkt.type = paser;
				
				for (int i = 0; i < size; i++) {
					if (i != rank) {
						sendPacket(&pkt, i, RET);
					}
				}
				clearQueueBySrc(&houseQueue, rank);
				clearQueueBySrc(&paserQueue, rank);
				println("Wychodzę z wszystkiego");
				changeState(InNothin);
				break;
		}
	}
}
