#ifndef BITTORRENT_H
#define BITTORRENT_H

#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <iostream>

#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100

/* Message types */
#define MSG_REQUEST_FILE 1
#define MSG_FILE_COMPLETE 2
#define MSG_FILE_COMPLETE_ALL 3
#define MSG_DOWNLOAD_END 4

/* Tags for message synchronization */
#define TRACKER_REQUEST_TAG 32
#define FILE_INIT 38
#define FILE_INIT_LEN 39
#define FILE_INIT_NAME 40
#define FILE_INIT_NR_SEG 41
#define FILE_INIT_SEG 42
#define FILE_TRANSFER_TAG 30
#define FILE_TRANSFER_TAG_MSG 33
#define FILE_TRANSFER_TAG_LEN 34
#define FILE_TRANSFER_TAG_TEXT 35
#define FILE_TRANSFER_TAG_RESP 36
#define FILE_INFO_TAG 31
#define END_TAG 37

/* Define ACK and NAK for file transfer */
#define ACK 6
#define NAK 15

/* File information (for clients) */
typedef struct {
    int nr_segments;
    std::vector<std::string> segments;
    std::vector<bool> haveSegment;
} fileT;

/* File information / swarm */
typedef struct {
    int nr_segments;
    std::vector<std::string> segments;
    std::vector<int> users;
} swarmT;

/* Global variables */
std::unordered_map<std::string, fileT> fileMap;
std::vector<std::string> wantedFiles;
std::unordered_map<std::string, swarmT> swarmMap;


#endif // BITTORRENT_H