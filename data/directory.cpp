#include "directory.h"
#include <algorithm>

std::vector<std::string> entryNamesInDirectory(const std::string &directory) {
    std::vector<std::string> entryNames;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(directory.c_str())) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != NULL) {
            entryNames.push_back(ent->d_name);
        }
        closedir (dir);
    }
    return entryNames;
}

bool isEntryExistInDirectory(
        const std::string &entryName, const std::string &directory) {
    auto entryNames = entryNamesInDirectory(directory);
    return std::any_of(entryNames.begin(), entryNames.end(),
            [entryName](const std::string& element) {
        return entryName == element;
    });
}
