#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <string>
#include <vector>
#include <dirent.h>

std::vector<std::string> entryNamesInDirectory(const std::string& directory);

bool isEntryExistInDirectory(
        const std::string& entryName, const std::string& directory);

#endif // DIRECTORY_H
