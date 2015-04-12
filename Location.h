#ifndef LOCATION_H_
#define LOCATION_H_

#include <cstdio>
#include <iostream>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Quoted.h"

struct Location
{
    private:
    std::string str;

    public:
    Location()
    {

    }

    Location(const std::string Str)
    {
        str = Str;
    }

    Location& operator=(const std::string Str)
    {
        str = Str;
        return *this;
    }

    std::string Str()
    {
        return str;
    }

    friend std::ostream& operator<<(std::ostream& os, const Location& loc);
    friend std::istream& operator>>(std::istream &is, Location &loc);

    friend Location operator+(const std::string &s, const Location &loc);
    friend Location operator+(const Location &loc, const std::string &s);

    friend std::string getComparable(const Location &loc);
    friend bool operator<(const Location &loc1, const Location &loc2);
    friend bool operator!=(const Location &loc1, const Location &loc2);
    friend bool operator==(const Location &loc1, const Location &loc2);

    friend bool modified_after(const Location &loc1, const Location &loc2);

    friend std::string STR(const Location &loc);
    friend Location getFilename(const Location &loc);
    friend Location stripExtension(const Location &file);
    friend Location getInfoLocation(const Location &contentLoc);
};

std::ostream& operator<<(std::ostream &os, const Location &loc)
{
    os << quote(loc.str);

    return os;
};

//reads a (possibly quoted) location from input stream
std::istream& operator>>(std::istream &is, Location &loc)
{
    read_quoted(is, loc.str);

    return is;
};

Location operator+(const std::string &s, const Location &loc)
{
    return Location(s + loc.str);
};

Location operator+(const Location &loc, const std::string &s)
{
    return Location(loc.str + s);
};

//note assumes locations don't have quotes
//removes ./ from the front if it's there
std::string getComparable(const Location &loc)
{
    if(loc.str.substr(0, 2) != "./")
        return loc.str;
    else
        return loc.str.substr(2, loc.str.length()-2);
};

bool operator==(const Location &loc1, const Location &loc2)
{
    return (getComparable(loc1) == getComparable(loc2));
}

bool operator!=(const Location &loc1, const Location &loc2)
{
    return (getComparable(loc1) != getComparable(loc2));
}

bool operator<(const Location &loc1, const Location &loc2)
{
    return (getComparable(loc1) < getComparable(loc2));
};

//returns whether first file was modified after second file
bool modified_after(const Location &loc1, const Location &loc2)
{
    struct stat sb1, sb2;
    stat(loc1.str.c_str(), &sb1);
    stat(loc2.str.c_str(), &sb2);

    if(difftime(sb1.st_mtime, sb2.st_mtime) > 0)
        return 1;
    else
        return 0;
};

std::string STR(const Location &loc)
{
    return loc.str;
};

Location getFilename(const Location &loc)
{
    int pos = loc.str.find_last_of('/');
    if(pos == std::string::npos)
        return loc;
    else
    {
        pos++;
        return Location(loc.str.substr(pos, loc.str.length()-pos));
    }
};

Location stripExtension(const Location &fil)
{
    int pos = fil.str.find_last_of('.');
    if(pos == std::string::npos)
        return fil;
    else
        return Location(fil.str.substr(0, pos));
};

Location getInfoLocation(const Location &contentLoc)
{
    return ".siteinfo/" + stripExtension(getFilename(contentLoc)) + ".info";
};

#endif //LOCATION_H_
