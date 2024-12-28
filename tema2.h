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
    int nr_users;
    std::vector<std::string> users;
} swarmT;

#endif // TEMA2_H