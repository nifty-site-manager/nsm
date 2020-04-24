#ifndef FILE_SYSTEM_H_
#define FILE_SYSTEM_H_

#include <dirent.h>
#include <mutex>
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

bool can_exec(const std::string& path);
bool can_write(const std::string& path);

bool remove_file(const Path& path);
bool remove_path(const Path& path);
bool remove_path(const Path& path,
                 const int& lineNo,
                 const Path& readPath,
                 std::ostream& eos,
                 const bool& consoleLocked,
                 std::mutex* os_mtx);
std::string ls(const char* path);
std::vector<std::string> lsVec(const char* path);
std::set<std::string> lsSet(const char* path);
void makeSearchable(Path& path);
std::set<std::string> lsSetStar(const Path& path, const int& incHidden);
void coutPaths(const std::string& dir,
               const std::set<std::string>& paths, 
               const std::string& separator, 
               const bool& highlight,
               const size_t& maxNoPaths);
int delDir(const std::string& dir);
int delDir(const std::string& dir,
	       const int& lineNo,
           const Path& readPath,
           std::ostream& eos,
           const bool& consoleLocked,
           std::mutex* os_mtx);
int cpDir(const std::string& sourceDir, 
          const std::string& targetDir);
int cpDir(const std::string& sourceDir, 
          const std::string& targetDir,
          const int& lineNo,
          const Path& readPath,
          std::ostream& eos,
          const bool& consoleLocked,
          std::mutex* os_mtx);
int cpFile(const std::string& sourceFile, const std::string& targetFile);
int cpFile(const std::string& sourceFile, 
           const std::string& targetFile,
           const int& lineNo,
           const Path& readPath,
           std::ostream& eos,
           const bool& consoleLocked,
           std::mutex* os_mtx);
std::string ifs_to_string(std::ifstream& ifs);
std::string string_from_file(const std::string& path);

#endif //FILE_SYSTEM_H_
