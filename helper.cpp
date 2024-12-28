#include "helper.h"
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