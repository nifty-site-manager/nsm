#ifndef FILENAME_H_
#define FILENAME_H_

#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Quoted.h"

typedef std::string Filename;

Filename strippedExtension(const Filename &filename);


#endif //FILENAME_H_
