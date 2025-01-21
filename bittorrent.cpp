#include "bittorrent.h"
#include "tracker.cpp"
#include "client.cpp"

void *download_thread_func(void *arg)
{
    int rank = *((int*) arg);
    int message_id;

    /* Download each missing file */
    for (const std::string& wantedFile : wantedFiles) {
        /* Request the file's swarm from the tracker */
        swarmT fileSwarm = getFileSwarm(wantedFile);

        int iterator = 0, nr_of_segments = fileSwarm.nr_segments;
        int users_iter = 0, users_count = fileSwarm.users.size();

        /* Add the missing file to the list
            of files to be constructed */
        fileT file;
        file.nr_segments = fileSwarm.nr_segments;
        file.segments = fileSwarm.segments;
        file.haveSegment = std::vector<bool>(file.nr_segments, false);
        fileMap[wantedFile] = file;

        /* Download each segment in order */
        while (iterator < nr_of_segments) {
            bool receivedSegement = false;
            /* Update the swarm every 10 segments */
            if (iterator % 10 == 0) {
                fileSwarm = getFileSwarm(wantedFile);
                nr_of_segments = fileSwarm.nr_segments;
            }
            while (!receivedSegement) {
                /* Vary the peer for each segment */
                users_iter = (users_iter + 1) % users_count;
                while (fileSwarm.users[users_iter] == TRACKER_RANK || fileSwarm.users[users_iter] == rank) {
                    users_iter = (users_iter + 1) % users_count;
                }

                int message_id = MSG_REQUEST_FILE;
                int fileName_len = wantedFile.length();

                /* Send request to peer */
                MPI_Send(&message_id, 1, MPI_INT, fileSwarm.users[users_iter], FILE_TRANSFER_TAG_MSG, MPI_COMM_WORLD);
                MPI_Send(&fileName_len, 1, MPI_INT, fileSwarm.users[users_iter], FILE_TRANSFER_TAG_LEN, MPI_COMM_WORLD);
                MPI_Send(wantedFile.c_str(), fileName_len, MPI_CHAR, fileSwarm.users[users_iter], FILE_TRANSFER_TAG_TEXT, MPI_COMM_WORLD);
                MPI_Send(&iterator, 1, MPI_INT, fileSwarm.users[users_iter], FILE_TRANSFER_TAG, MPI_COMM_WORLD);

                /* Receive response from peer */
                int response;
                MPI_Recv(&response, 1, MPI_INT, fileSwarm.users[users_iter], FILE_TRANSFER_TAG_RESP, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (response == ACK) {
                    receivedSegement = true;
                    /* If we received ACK, mark the segment as owned */
                    fileMap[wantedFile].haveSegment[iterator] = true;
                }
            }
            iterator++;
        }
        /* Once all segments are downloaded, write the file */
        writeOutputFile(wantedFile, fileSwarm, rank);

        /* Notify the tracker that a file is complete */
        message_id = MSG_FILE_COMPLETE;
        MPI_Send(&message_id, 1, MPI_INT, TRACKER_RANK, TRACKER_REQUEST_TAG, MPI_COMM_WORLD);
    }
    /* Notify the tracker that all files are complete */
    message_id = MSG_FILE_COMPLETE_ALL;
    MPI_Send(&message_id, 1, MPI_INT, TRACKER_RANK, TRACKER_REQUEST_TAG, MPI_COMM_WORLD);

    return NULL;
}

void *upload_thread_func(void *arg)
{
    bool loopHolder = true;
    while (loopHolder) {
        int message_id;
        MPI_Status status;

        /* Wait for message */
        MPI_Recv(&message_id, 1, MPI_INT, MPI_ANY_SOURCE, FILE_TRANSFER_TAG_MSG, MPI_COMM_WORLD, &status);
        
        /* If the message is from the tracker, stop the upload process */
        if (status.MPI_SOURCE == TRACKER_RANK) {
            loopHolder = false;
            continue;
        }

        /* If the message is from a client, retrieve the rest of the information */
        int fileName_len;
        int segment_number;
        MPI_Recv(&fileName_len, 1, MPI_INT, status.MPI_SOURCE, FILE_TRANSFER_TAG_LEN, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        std::string filename(fileName_len, ' ');
        MPI_Recv(&filename[0], fileName_len, MPI_CHAR, status.MPI_SOURCE, FILE_TRANSFER_TAG_TEXT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&segment_number, 1, MPI_INT, status.MPI_SOURCE, FILE_TRANSFER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        /* If we own the file, send ACK, otherwise send NAK */
        int response = NAK;
        if (fileMap[filename].haveSegment[segment_number] == true) {
            response = ACK;
        }
        MPI_Send(&response, 1, MPI_INT, status.MPI_SOURCE, FILE_TRANSFER_TAG_RESP, MPI_COMM_WORLD);
    }
    return NULL;
}

void tracker(int numtasks, int rank) {
    /* Receive file information from clients */
    std::unordered_map<std::string, swarmT> swarmMap;
    receiveInitialData(numtasks, swarmMap);

    bool loopHold = true;
    int clientsLeft = numtasks - 1;

    while (loopHold) {
        MPI_Status status;
        int message_type;
        /* Receive message from any client */
        MPI_Recv(&message_type, 1, MPI_INT, MPI_ANY_SOURCE, TRACKER_REQUEST_TAG, MPI_COMM_WORLD, &status);
        int clientRank = status.MPI_SOURCE;
        switch (message_type) {
            case MSG_REQUEST_FILE: {
                /* Request for file information */
                sendFileSwarm(swarmMap, clientRank);
                break;
            }
            case MSG_FILE_COMPLETE: {
                /* Signal file download complete */
                std::cout << "Client " << status.MPI_SOURCE << " completed a file.\n";
                break;
            }
            case MSG_FILE_COMPLETE_ALL: {
                /* Signal all files download complete */
                std::cout << "Client " << status.MPI_SOURCE << " completed ALL files.\n";
                clientsLeft--;
                break;
            }
            default: {
                /* Error */
                std::cout << "Unknown message from peer " << status.MPI_SOURCE << "\n";
                break;
            }
        }

        /* Final message to close upload threads */
        if (clientsLeft <= 0) {
            for (int i = 1; i < numtasks; i++) {
                message_type = MSG_DOWNLOAD_END;
                MPI_Send(&message_type, 1, MPI_INT, i, FILE_TRANSFER_TAG_MSG, MPI_COMM_WORLD);
            }
            loopHold = false;
        }
    }

    return;
}

void peer(int numtasks, int rank) {
    pthread_t download_thread;
    pthread_t upload_thread;
    void *status;
    int r;

    /* Initialization - Read input files */
    readFile(fileMap, wantedFiles, rank);

    /* Initialization - Send data to tracker and wait for ACK */
    sendInitialData(fileMap, rank);

    /* Create upload and download threads */
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
