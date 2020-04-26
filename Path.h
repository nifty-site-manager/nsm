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

#include "ConsoleColor.h"
#include "Directory.h"
#include "Filename.h"
#include "SystemInfo.h"

struct Path
{
    Directory dir;
    Filename file;
    std::string type;

    Path();
    Path(const Directory& Dir, const Filename& File);
    Path(const std::string& path);

    void set_file_path_from(const std::string& s);
    std::istream& read_file_path_from(std::istream& is);

    std::string str() const;
    std::string comparableStr() const;

    //returns whether first file was modified after second file
    bool modified_after(const Path& path2) const;

    bool ensureDirExists() const;
	bool ensureFileExists() const;

    Path getInfoPath() const;
    Path getHashPath() const;
    Path getPaginationPath() const;
};

//outputs path (quoted if it contains spaces)
std::ostream& operator<<(std::ostream& os, const Path& path);

//equality relation
bool operator==(const Path& path1, const Path& path2);
//inequality relation
bool operator!=(const Path& path1, const Path& path2);
//less than relation
bool operator<(const Path& path1, const Path& path2);

void clear_console_line();

std::ostream& start_err(std::ostream& eos);
std::ostream& start_err(std::ostream& eos, const Path& readPath);
std::ostream& start_err(std::ostream& eos, const Path& readPath, const int& lineNo);
std::ostream& start_err_ml(std::ostream& eos, const Path& readPath, const int& sLineNo, const int& eLineNo);

std::ostream& start_warn(std::ostream& eos);
std::ostream& start_warn(std::ostream& eos, const Path& readPath);
std::ostream& start_warn(std::ostream& eos, const Path& readPath, const int& lineNo);


#endif //PATH_H_
