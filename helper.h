#ifndef HELPER_H
#define HELPER_H

// Include necessary headers
#include <iostream>
#include <unordered_set>
#include <string>
#include "tema2.h"

// Namespace declaration
namespace helper {

// Function declarations
void readFile(const std::string &fileName, std::unordered_map<std::string, fileT> &fileMap,
                std::vector<std::string> &wantedFiles, int rank);
void printFileMap(const std::unordered_map<std::string, fileT> &fileMap);

} // namespace helper

#endif // HELPER_H