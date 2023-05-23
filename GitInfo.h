#ifndef GIT_INFO_H_
#define GIT_INFO_H_

#include <set>

#include "ConsoleColor.h"
#include "FileSystem.h"

bool is_git_configured();                                // checks whether git is configured
bool is_git_remote_set();                                // checks whether git remote is set
int get_pb(std::string& branch);                         // returns the current branch
int get_remote(std::string& remote);                     // returns the current remote
int get_git_branches(std::set<std::string>& branches);   // returns the set of branches

#endif //FILE_SYSTEM_H_
