#ifndef FILE_SYSTEM_H_
#define FILE_SYSTEM_H_

#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>

#include "Path.h"

std::string get_pwd();
bool file_exists(const char *path, const std::string& file);
bool remove_file(const Path& path);
bool remove_path(const Path& path); //don't use this anywhere with multithreading!
std::string ls(const char *path);
std::vector<std::string> lsVec(const char *path);
int delDir(std::string dir); //don't use this anywhere with multithreading!
int cpDir(const std::string& sourceDir, const std::string& targetDir);
int cpFile(const std::string& sourceFile, const std::string& targetFile);

#endif //FILE_SYSTEM_H_
