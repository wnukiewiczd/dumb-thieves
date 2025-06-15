/* w main.h także makra println oraz debug -  z kolorkami! */
#include "main.h"
#include "watek_glowny.h"
#include "watek_komunikacyjny.h"


int rank, size;
int ackCount = 0;
/* 
 * Każdy proces ma dwa wątki - główny i komunikacyjny
 * w plikach, odpowiednio, watek_glowny.c oraz watek_komunikacyjny.c
 */

pthread_t threadKom;

void finalizuj()
{
    pthread_mutex_destroy( &stateMut);
    /* Czekamy, aż wątek potomny się zakończy */
    println("czekam na wątek \"komunikacyjny\"\n" );
    pthread_join(threadKom,NULL);
    MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
}

void check_thread_support(int provided)
{
    printf("THREAD SUPPORT: chcemy %d.\n", provided);

    if(provided == MPI_THREAD_MULTIPLE){
        printf("Pełne wsparcie dla wątków\n");   
    } else {
        MPI_Finalize();
	    exit(-1);
    }
}


int main(int argc, char **argv)
{
    MPI_Status status;
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);
    srand(rank);
    inicjuj_typ_pakietu(); // tworzy typ pakietu
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    initQueue(size); //tworzenie kolejki
    pthread_create( &threadKom, NULL, startKomWatek , 0);

    // mainLoop w watek_glowny.c 
    
    mainLoop(); 

    finalizuj();
    
    return 0;
}

