#include "tema2.h"
#include "helper.cpp"

void *download_thread_func(void *arg)
{
    DownloadThreadArgs* args = (DownloadThreadArgs*) arg;
    int rank = args->rank;
    std::vector<std::string> wantedFiles = args->wantedFiles;

    for (const std::string& wantedFile : wantedFiles) {
        int totalSegments = 100; // Assuming a fixed number of segments for simplicity
        for (int segmentIndex = 0; segmentIndex < totalSegments; ++segmentIndex) {
            
            // Every 10 segments, request the swarmT from the tracker
            if (segmentIndex % 10 == 0) {
                int message_type = MSG_REQUEST_FILE;
                MPI_Send(&message_type, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);
                MPI_Send(wantedFile.c_str(), MAX_FILENAME, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);

                // Receive the list of peers that have the file
                int num_users;
                MPI_Recv(&num_users, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                std::vector<int> users(num_users);
                MPI_Recv(users.data(), num_users, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                // Process the received swarmT information (e.g., update peer list)
                std::cout << "Received swarmT for file " << wantedFile << " from tracker:\n";
                for (int user : users) {
                    std::cout << "  Peer: " << user << "\n";
                }
            }
        }

        // After downloading all segments, notify the tracker
        int message_type = MSG_FILE_COMPLETE;
        MPI_Send(&message_type, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);
        MPI_Send(wantedFile.c_str(), MAX_FILENAME, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
    }

    // Notify the tracker that the peer has finished downloading all files
    int message_type = MSG_FILE_COMPLETE_ALL;
    MPI_Send(&message_type, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);

    return NULL;
}

void *upload_thread_func(void *arg)
{
    int rank = *(int*) arg;

    return NULL;
}

void tracker(int numtasks, int rank) {
    std::unordered_map<std::string, swarmT> swarmMap;
    receiveDataFromPeers(numtasks, swarmMap);
    printSwarmMap(swarmMap);

    bool loopHold = true;

    while (loopHold) {
        MPI_Status status;
        int message_type;
        // Probe for an incoming message
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        // Receive the message type
        MPI_Recv(&message_type, 1, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        switch (message_type) {
            case MSG_REQUEST_FILE:
                std::cout << "REQUEST_FILE\n";
                break;
            case MSG_FILE_COMPLETE:
                std::cout << "MSG_FILE_COMPLETE\n";
                break;
            case MSG_FILE_COMPLETE_ALL:
                std::cout << "MSG_FILE_COMPLETE_ALL\n";
                break;
            case MSG_DOWNLOAD_END:
                std::cout << "MSG_DOWNLOAD_END\n";
                loopHold = false;
                break;
            default:
                std::cout << "Unknown message from peer " << status.MPI_SOURCE << "\n";
                break;
        }
    }
    return;
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

    /* Send data to TRACKER and wait for ACK */
    sendInitialData(fileMap, rank);

    DownloadThreadArgs downloadArgs;
    downloadArgs.rank = rank;
    downloadArgs.wantedFiles = wantedFiles;

    r = pthread_create(&download_thread, NULL, download_thread_func, (void *) &downloadArgs);
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
