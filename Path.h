#ifndef PATH_H_
#define PATH_H_

#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#ifdef _WIN32
    #include <direct.h>
#endif

#include "Directory.h"
#include "Filename.h"

struct Path
{
    Directory dir;
    Filename file;
    std::string type;

    Path();
    Path(const Directory &Dir, const Filename &File);

    void set_file_path_from(const std::string &s);
    std::istream& read_file_path_from(std::istream &is);

    std::string str() const;
    std::string comparableStr() const;

    //returns whether first file was modified after second file
    bool modified_after(const Path &path2) const;

    bool removePath() const;
    bool ensurePathExists() const;

    Path getInfoPath() const;
};

//outputs path (quoted if it contains spaces)
std::ostream& operator<<(std::ostream &os, const Path &path);

//equality relation
bool operator==(const Path &path1, const Path &path2);
//inequality relation
bool operator!=(const Path &path1, const Path &path2);
//less than relation
bool operator<(const Path &path1, const Path &path2);

#endif //PATH_H_
