#include "bittorrent.h"

/* Initialization - Reading file and adding to fileMap */
void readFile(std::unordered_map<std::string, fileT> &fileMap,
                std::vector<std::string> &wantedFiles, int rank) {
    std::string fileName = "tests/input/in" + std::to_string(rank) + ".txt";
    std::ifstream infile(fileName);
    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << fileName << std::endl;
        exit(-1);
    }

    int num_files, num_segments, num_wanted_files;
    infile >> num_files;

    for (int i = 0; i < num_files; i++) {
        std::string file_name;
        infile >> file_name >> num_segments;
        fileT file{num_segments, {}, std::vector<bool>(num_segments, true)};
        for (int j = 0; j < num_segments; j++) {
            std::string segment;
            infile >> segment;
            file.segments.push_back(segment);
        }
        fileMap[file_name] = file;
    }

    infile >> num_wanted_files;
    wantedFiles.resize(num_wanted_files);
    for (int i = 0; i < num_wanted_files; i++) {
        infile >> wantedFiles[i];
    }
    infile.close();
}

/* Initialization - Sending data to tracker */
void sendInitialData(const std::unordered_map<std::string, fileT>& fileMap, int rank) {
    int num_files = fileMap.size();
    MPI_Send(&num_files, 1, MPI_INT, TRACKER_RANK, FILE_INIT, MPI_COMM_WORLD);

    for (const auto& entry : fileMap) {
        const std::string& filename = entry.first;
        const fileT& filedata = entry.second;

        int filename_length = filename.size();
        MPI_Send(&filename_length, 1, MPI_INT, TRACKER_RANK, FILE_INIT_LEN, MPI_COMM_WORLD);
        MPI_Send(filename.c_str(), filename_length, MPI_CHAR, TRACKER_RANK, FILE_INIT_NAME, MPI_COMM_WORLD);
        MPI_Send(&filedata.nr_segments, 1, MPI_INT, TRACKER_RANK, FILE_INIT_NR_SEG, MPI_COMM_WORLD);

        for (const auto& segment : filedata.segments) {
            MPI_Send(segment.c_str(), HASH_SIZE, MPI_CHAR, TRACKER_RANK, FILE_INIT_SEG, MPI_COMM_WORLD);
        }
    }

    /* Waiting for ACK */
    int ack;
    MPI_Recv(&ack, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

/* Download - Logic for requesting file swarm from tracker */
swarmT getFileSwarm(const std::string fileName) {
    /* Sending request and data to tracker */
    int msg_id = MSG_REQUEST_FILE;
    MPI_Send(&msg_id, 1, MPI_INT, TRACKER_RANK, TRACKER_REQUEST_TAG, MPI_COMM_WORLD);

    int fileName_length = fileName.length();
    MPI_Send(&fileName_length, 1, MPI_INT, TRACKER_RANK, TRACKER_REQUEST_TAG, MPI_COMM_WORLD);
    MPI_Send(fileName.c_str(), fileName_length + 1, MPI_CHAR, TRACKER_RANK, TRACKER_REQUEST_TAG, MPI_COMM_WORLD);
    
    /* Receiving file information from tracker */
    swarmT swarm;
    MPI_Recv(&swarm.nr_segments, 1, MPI_INT, TRACKER_RANK, TRACKER_REQUEST_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    swarm.segments.resize(swarm.nr_segments);
    for (int i = 0; i < swarm.nr_segments; ++i) {
        int segment_length;
        MPI_Recv(&segment_length, 1, MPI_INT, TRACKER_RANK, TRACKER_REQUEST_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        std::string segment(segment_length, ' ');
        MPI_Recv(&segment[0], segment_length, MPI_CHAR, TRACKER_RANK, TRACKER_REQUEST_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        swarm.segments[i] = segment;
    }

    int num_users;
    MPI_Recv(&num_users, 1, MPI_INT, TRACKER_RANK, TRACKER_REQUEST_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    swarm.users.resize(num_users);
    MPI_Recv(swarm.users.data(), num_users, MPI_INT, TRACKER_RANK, TRACKER_REQUEST_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    /* Writing data to file list */
    fileMap[fileName].segments = swarm.segments;
    fileMap[fileName].nr_segments = swarm.nr_segments;
    fileMap[fileName].haveSegment = std::vector<bool>(swarm.nr_segments, false);

    return swarm;
}

/* Writing segments to output file */
void writeOutputFile(const std::string& fileName, const swarmT& swarm, int rank) {
    std::ofstream outfile("tests/output/client" + std::to_string(rank) + "_" + fileName);
    if (!outfile.is_open()) {
        std::cerr << "Error opening file: " << fileName << std::endl;
        return;
    }

    for (const auto& segment : swarm.segments) {
        outfile << segment << std::endl;
    }

    outfile.close();
}