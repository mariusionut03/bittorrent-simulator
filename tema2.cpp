#include "tema2.h"
#include "helper.cpp"

void *download_thread_func(void *arg)
{
    int rank = *(int*) arg;

    /*  TODO: Ask TRACKER for info about the files peer wants
        TODO: Ask again every 10 recieved segments */

    /*  TODO: Find Seed/peer for the current segment */

    /*  TODO: Send request to that seed/peed */
    
    /*  TODO: Recieve ACK from seed/peer */
    
    /*  TODO: Mark segment as recieved */

    /*  TODO: When file is completed, save all the hashes
        in order in a file called client<RANK>_<FILENAME>*/

    /*  TODO: When all files are recieved, announce the TRACKER
        and close download thread */

    return NULL;
}

void *upload_thread_func(void *arg)
{
    int rank = *(int*) arg;

    /*  TODO: Wait for segment request, send ACK if found in list */

    /*  TODO: Stop the thread if Tracker anounches everyone is finished */

    return NULL;
}

void tracker(int numtasks, int rank) {
    /* TODO: Wait for initial lists from every user */
    /* TODO: Mark the client as seeder in the file swarm */
    /* TODO: After recieving from all the clients, send ACK to every one */
    /* TODO: Recieve message and do something after checking the type: 
        a. Cerere fisier / actualizare: Trimite datele acelui fisier
        b. Finalizare descarcare: Trackerul marcheaza clientul ca seeder
        c. Finalizare tot descarcare: Marcheaza client ca terminat
        d. Toti clientii au terminat: Trimite mesaj de finalizare
            la fiecare client si se opreste.
    */
}

void peer(int numtasks, int rank) {
    pthread_t download_thread;
    pthread_t upload_thread;
    void *status;
    int r;

    /* Declarare variabile */
    std::unordered_map<std::string, fileT> fileMap;
    std::vector<std::string> wantedFiles;

    /* Citire fisiere de intrare (PEERs) */
    std::string filename = "tests/in" + std::to_string(rank) + ".txt";
    readFile(filename, fileMap, wantedFiles, rank);
    printFileMap(fileMap);

    /* TODO: Send data to TRACKER */

    /* TODO: WAIT FOR ACK FROM TRACKER TO CONTINUE */

    r = pthread_create(&download_thread, NULL, download_thread_func, (void *) &rank);
    if (r) {
        printf("Eroare la crearea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_create(&upload_thread, NULL, upload_thread_func, (void *) &rank);
    if (r) {
        printf("Eroare la crearea thread-ului de upload\n");
        exit(-1);
    }

    r = pthread_join(download_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_join(upload_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de upload\n");
        exit(-1);
    }
}
 
int main (int argc, char *argv[]) {
    int numtasks, rank;
 
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI nu are suport pentru multi-threading\n");
        exit(-1);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == TRACKER_RANK) {
        tracker(numtasks, rank);
    } else {
        peer(numtasks, rank);
    }

    MPI_Finalize();
}
