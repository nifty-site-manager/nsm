#ifndef WATCH_LIST_H_
#define WATCH_LIST_H_

#include <map>
#include <set>

#include "FileSystem.h"

std::string replace_slashes(const std::string& source);

struct WatchDir
{
    Directory watchDir;
    std::set<std::string> contExts;
    std::map<std::string, Path> templatePaths;
    std::map<std::string, std::string> outputExts;

    WatchDir();
};

bool operator<(const WatchDir& wd1, const WatchDir& wd2);

struct WatchList
{
    std::set<WatchDir> dirs;

    WatchList();

    int open();
    int save();
};

std::ostream& operator<<(std::ostream &os, const WatchList &wl);

#endif //WATCH_LIST_H_
