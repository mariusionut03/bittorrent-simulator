#ifndef TEMA2_H
#define TEMA2_H

#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include <string>
#include <vector>

#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100

#define MSG_REQUEST_FILE 1
#define MSG_FILE_COMPLETE 2
#define MSG_FILE_COMPLETE_ALL 3
#define MSG_DOWNLOAD_END 4

/* File informations in user */
typedef struct {
    int nr_segments;
    std::vector<std::string> segments;
    std::vector<bool> recievedSegments;
} fileT;

/* File informations in tracker */
typedef struct {
    int nr_segments;
    std::vector<std::string> segments;
    std::vector<int> users;
} swarmT;

struct DownloadThreadArgs {
    int rank;
    std::vector<std::string> wantedFiles;
};

#endif // TEMA2_H