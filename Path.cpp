#include "Path.h"

struct Path;
Path operator+(const std::string &s, const Path &path);
Path operator+(const Path &path, const std::string &s);

Path::Path()
{

}

Path::Path(const std::string Str)
{
    str = Str;
}

Path& Path::operator=(const std::string Str)
{
    str = Str;
    return *this;
}

//note assumes paths don't have quotes
//removes ./ from the front if it's there
std::string Path::getComparable() const
{
    if(str.substr(0, 2) != "./")
        return str;
    else
        return str.substr(2, str.length()-2);
};

//returns whether first file was modified after second file
bool Path::modified_after(const Path &path2) const
{
    struct stat sb1, sb2;
    stat(str.c_str(), &sb1);
    stat(path2.str.c_str(), &sb2);

    if(difftime(sb1.st_mtime, sb2.st_mtime) > 0)
        return 1;
    else
        return 0;
};

Path Path::getFilename() const
{
    int pos = str.find_last_of('/');
    if(pos == std::string::npos)
        return *this;
    else
    {
        pos++;
        return Path(str.substr(pos, str.length()-pos));
    }
};

//currently will not work on say ./dir/filewithoutextension
Path Path::stripExtension() const
{
    int pos = str.find_last_of('.');
    if(pos == std::string::npos)
        return *this;
    else
        return Path(str.substr(0, pos));
};

Path Path::getInfoPath() const
{
    return ".siteinfo/" + this->getFilename().stripExtension() + ".info";
};



//outputs path (quoted if it contains spaces)
std::ostream& operator<<(std::ostream &os, const Path &path)
{
    os << quote(path.str);

    return os;
};

//reads a (possibly quoted) path from input stream
std::istream& operator>>(std::istream &is, Path &path)
{
    read_quoted(is, path.str);

    return is;
};

//concat fns
Path operator+(const std::string &s, const Path &path)
{
    return Path(s + path.str);
};

Path operator+(const Path &path, const std::string &s)
{
    return Path(path.str + s);
};


//equality relation
bool operator==(const Path &path1, const Path &path2)
{
    return (path1.getComparable() == path2.getComparable());
}

//inequality relation
bool operator!=(const Path &path1, const Path &path2)
{
    return (path1.getComparable() != path2.getComparable());
}

//less than relation
bool operator<(const Path &path1, const Path &path2)
{
    return (path1.getComparable() < path2.getComparable());
};

