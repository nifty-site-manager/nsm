#ifndef GIT_INFO_H_
#define GIT_INFO_H_

#include <set>

#include "FileSystem.h"
#include "Path.h"

bool is_git_configured(); //checks whether git is configured
bool is_git_remote_set(); //checks whether git remote is set
std::string get_pb(); //returns the current branch
std::string get_remote(); //returns the current remote
std::set<std::string> get_git_branches(); //returns the set of branches

#endif //FILE_SYSTEM_H_
