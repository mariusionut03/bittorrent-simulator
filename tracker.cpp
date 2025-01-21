#include "bittorrent.h"

/* Initialization - Receiving file data from clients */
void receiveInitialData(int numtasks, std::unordered_map<std::string, swarmT>& swarmMap) {
    for (int peer_rank = 1; peer_rank < numtasks; peer_rank++) {
        int num_files, filename_length, nr_segments;
        MPI_Recv(&num_files, 1, MPI_INT, peer_rank, FILE_INIT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int i = 0; i < num_files; ++i) {
            /* Receiving file name */
            MPI_Recv(&filename_length, 1, MPI_INT, peer_rank, FILE_INIT_LEN, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            std::string filename(filename_length, ' ');
            MPI_Recv(&filename[0], filename_length, MPI_CHAR, peer_rank, FILE_INIT_NAME, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            /* If we already have the data, skip it */
            if (swarmMap.find(filename) != swarmMap.end()) {
                swarmMap[filename].users.push_back(peer_rank);
                continue;
            }

            /* Receiving segments */
            MPI_Recv(&nr_segments, 1, MPI_INT, peer_rank, FILE_INIT_NR_SEG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            std::vector<std::string> segments(nr_segments);
            for (int j = 0; j < nr_segments; ++j) {
                std::string segment(HASH_SIZE, ' ');
                MPI_Recv(&segment[0], HASH_SIZE, MPI_CHAR, peer_rank, FILE_INIT_SEG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                segments[j] = segment;
            }

            /* Storing data in the swarm of all files */
            swarmMap[filename] = {nr_segments, segments, {peer_rank}};
        }
    }
    /* After initialization, send ACK to all Clients */
    for (int peer_rank = 1; peer_rank < numtasks; ++peer_rank) {
        int ack = 1;
        MPI_Send(&ack, 1, MPI_INT, peer_rank, 0, MPI_COMM_WORLD);
    }
}

/* Sending file swarm to client */
void sendFileSwarm(std::unordered_map<std::string, swarmT> &swarmMap, int rank) {
    /* Receiving requested file name */
    int fileName_length;
    MPI_Recv(&fileName_length, 1, MPI_INT, rank, TRACKER_REQUEST_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    std::string fileName(fileName_length, ' ');
    MPI_Recv(&fileName[0], fileName_length + 1, MPI_CHAR, rank, TRACKER_REQUEST_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    fileName[fileName_length] = '\0';

    /* Sending file */    
    if (swarmMap.find(fileName) != swarmMap.end()) {
        const swarmT& swarm = swarmMap.at(fileName);
        MPI_Send(&swarm.nr_segments, 1, MPI_INT, rank, TRACKER_REQUEST_TAG, MPI_COMM_WORLD);

        for (const auto& segment : swarm.segments) {
            int segment_length = segment.size();
            MPI_Send(&segment_length, 1, MPI_INT, rank, TRACKER_REQUEST_TAG, MPI_COMM_WORLD);
            MPI_Send(segment.c_str(), segment_length, MPI_CHAR, rank, TRACKER_REQUEST_TAG, MPI_COMM_WORLD);
        }

        int num_users = swarm.users.size();
        MPI_Send(&num_users, 1, MPI_INT, rank, TRACKER_REQUEST_TAG, MPI_COMM_WORLD);
        MPI_Send(swarm.users.data(), num_users, MPI_INT, rank, TRACKER_REQUEST_TAG, MPI_COMM_WORLD);
    }

    /* Adding user to the file's swarm */
    swarmMap[fileName].users.push_back(rank);
}