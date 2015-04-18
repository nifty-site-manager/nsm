#ifndef DIRECTORY_H_
#define DIRECTORY_H_

#include <deque>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Quoted.h"

typedef std::string Directory;

Directory comparable(const Directory &directory);

Directory leadingDir(const Directory &directory);
Directory stripLeadingDir(const Directory &directory);
std::deque<Directory> dirDeque(const Directory &directory);

std::string pathBetween(const Directory &sourceDir, const Directory &targetDir);


#endif //DIRECTORY_H_
