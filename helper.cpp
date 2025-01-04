#include "helper.h"
#include "tema2.h"
#include <fstream>
#include <vector>
#include <iostream>

void readFile(const std::string &fileName, std::unordered_map<std::string, fileT> &fileMap,
                std::vector<std::string> &wantedFiles, int rank) {
    std::unordered_map<std::string, fileT> fileData;

    std::ifstream infile(fileName);
    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << fileName << std::endl;
        return;
    }

    int num_files;
    infile >> num_files;
    std::cout << rank << ") num_files=" << num_files << "\n";

    for (int i = 0; i < num_files; i++) {
        std::string file_name;
        int num_segments;
        infile >> file_name >> num_segments;
        std::cout << rank << ")filename: " << file_name << "; numseg = " << num_segments << "\n";

        fileT file;
        file.nr_segments = num_segments;
        std::string currentSegments;
        for (int j = 0; j < num_segments; j++) {
            infile >> currentSegments;
            file.segments.push_back(currentSegments);
        }

        fileMap[file_name] = file;
    }

    int num_wanted_files;
    infile >> num_wanted_files;
    for (int i = 0; i < num_wanted_files; i++) {
        std::string wanted_file;
        infile >> wanted_file;
        wantedFiles.push_back(wanted_file);
    }

    infile.close();
}

// Function to print the contents of fileMap
void printFileMap(const std::unordered_map<std::string, fileT> &fileMap) {
    for (const auto &pair : fileMap) {
        std::cout << "File: " << pair.first << std::endl;
        for (int i = 0; i < pair.second.nr_segments; ++i) {
            std::cout << "  Segment: " << pair.second.segments[i] << std::endl;
        }
    }
}

// Function to print the contents of swarmMap
void printSwarmMap(const std::unordered_map<std::string, swarmT> &swarmMap) {
    for (const auto &pair : swarmMap) {
        std::cout << "File: " << pair.first << std::endl;
        std::cout << "  Peers: ";
        for (const int &peer : pair.second.users) {
            std::cout << peer << " ";
        }
        std::cout << std::endl;
    }
}

/* Send initial files data to tracker */
void sendInitialData(const std::unordered_map<std::string, fileT>& fileMap, int rank) {
    int num_files = fileMap.size();
    MPI_Send(&num_files, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);

    for (const auto& entry : fileMap) {
        const std::string& filename = entry.first;
        const fileT& filedata = entry.second;

        int filename_length = filename.size();
        MPI_Send(&filename_length, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);
        MPI_Send(filename.c_str(), filename_length, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
        MPI_Send(&filedata.nr_segments, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);

        for (const auto& segment : filedata.segments) {
            MPI_Send(segment.c_str(), HASH_SIZE, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
        }
    }
    // Wait for ACK from the tracker
    int ack;
    MPI_Recv(&ack, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

}

void receiveDataFromPeers(int numtasks, std::unordered_map<std::string, swarmT>& swarmMap) {
    for (int peer_rank = 1; peer_rank < numtasks; ++peer_rank) {
        int num_files;
        MPI_Recv(&num_files, 1, MPI_INT, peer_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (int i = 0; i < num_files; ++i) {
            int filename_length;
            MPI_Recv(&filename_length, 1, MPI_INT, peer_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            char* filename_cstr = new char[filename_length + 1];
            MPI_Recv(filename_cstr, filename_length, MPI_CHAR, peer_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            filename_cstr[filename_length] = '\0';
            std::string filename(filename_cstr);
            delete[] filename_cstr;

            // Check if the file has already been received
            if (swarmMap.find(filename) != swarmMap.end()) {
                swarmMap[filename].users.push_back(peer_rank);
                continue;
            }

            int nr_segments;
            MPI_Recv(&nr_segments, 1, MPI_INT, peer_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            std::cout << "nrseg=" << nr_segments << "\n";
            std::vector<std::string> segments(nr_segments);
            for (int j = 0; j < nr_segments; ++j) {
                char segment_cstr[HASH_SIZE + 1];
                MPI_Recv(segment_cstr, HASH_SIZE, MPI_CHAR, peer_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                segment_cstr[HASH_SIZE] = '\0';
                segments[j] = std::string(segment_cstr);
                std::cout << segment_cstr << "\n";
            }

            swarmMap[filename].nr_segments = nr_segments;
            swarmMap[filename].segments = segments;
            swarmMap[filename].users.push_back(peer_rank);
        }
    }

    
    for (int peer_rank = 1; peer_rank < numtasks; ++peer_rank) {
        int ack = 1;
        MPI_Send(&ack, 1, MPI_INT, peer_rank, 0, MPI_COMM_WORLD);
    }
}