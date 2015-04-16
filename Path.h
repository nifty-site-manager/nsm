#ifndef PATH_H_
#define PATH_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Quoted.h"

struct Path
{
    std::string str;

    Path();
    Path(const std::string Str);

    Path& operator=(const std::string Str);

    //note assumes paths don't have quotes
    //removes ./ from the front if it's there
    std::string getComparable() const;

    //returns whether first file was modified after second file
    bool modified_after(const Path &path2) const;

    Path getFilename() const;
    //currently will not work on say ./dir/filewithoutextension
    Path stripExtension() const;
    Path getInfoPath() const;

};

//outputs path (quoted if it contains spaces)
std::ostream& operator<<(std::ostream &os, const Path &path);
//reads a (possibly quoted) path from input stream
std::istream& operator>>(std::istream &is, Path &path);

//concatenation fns
Path operator+(const std::string &s, const Path &path);
Path operator+(const Path &path, const std::string &s);

//equality relation
bool operator==(const Path &path1, const Path &path2);
//inequality relation
bool operator!=(const Path &path1, const Path &path2);
//less than relation
bool operator<(const Path &path1, const Path &path2);

#endif //PATH_H_
