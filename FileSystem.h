#ifndef FILE_SYSTEM_H_
#define FILE_SYSTEM_H_

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <set>

#include "Path.h"
#include "SystemInfo.h"

std::string get_pwd();
bool path_exists(const std::string& path);
bool dir_exists(const std::string& path);
bool file_exists(const std::string& path);
bool exec_exists(const std::string& path);
bool remove_file(const Path& path);
bool remove_path(const Path& path); //don't use this anywhere with multithreading!
std::string ls(const char* path);
std::vector<std::string> lsVec(const char* path);
std::set<std::string> lsSet(const char* path);
void makeSearchable(Path& path);
std::set<std::string> lsSetStar(const Path& path, const int& incHidden);
void coutPaths(const std::string& dir, const std::set<std::string>& paths, const std::string& separator);
int delDir(const std::string& dir); //don't use this anywhere with multithreading!
int cpDir(const std::string& sourceDir, const std::string& targetDir);
int cpFile(const std::string& sourceFile, const std::string& targetFile);
std::string ifs_to_string(std::ifstream& ifs);
std::string string_from_file(const std::string& path);

#endif //FILE_SYSTEM_H_
