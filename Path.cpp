#include "Path.h"

Path::Path()
{
    type = "none";
}

Path::Path(const Directory &Dir, const Filename &File)
{
    dir = Dir;
    file = File;
    type = "file";
}

std::string Path::str() const
{
    return dir + file;
    //return quote(dir + file);
    /*
        consider:  <a href=@pathtofile('site/pdfs/Example name.pdf')>pdf</a>
            vs     <a href="@pathtofile('site/pdfs/Example name.pdf')">pdf</a>
    */
}

//removes ./ from the front if it's there
std::string Path::comparableStr() const
{
    return comparable(dir) + file;
}

//outputs path (quoted if it contains spaces)
std::ostream& operator<<(std::ostream &os, const Path &path)
{
    os << quote(path.str());

    return os;
}

void Path::set_file_path_from(const std::string &s)
{
    size_t pos = s.find_last_of('/');
    if(pos == std::string::npos)
        *this = Path("", s);
    else
        *this = Path(s.substr(0, (pos+1)),
                     s.substr(pos+1, s.size()-(pos+1)) );
}

std::istream& Path::read_file_path_from(std::istream &is)
{
    std::string s;
    read_quoted(is, s);
    set_file_path_from(s);

    return is;
}

//returns whether first file was modified after second file
bool Path::modified_after(const Path &path2) const
{
    struct stat sb1, sb2;
    stat(str().c_str(), &sb1);
    stat(path2.str().c_str(), &sb2);

    if(difftime(sb1.st_mtime, sb2.st_mtime) > 0)
        return 1;
    else
        return 0;
}

Path Path::getInfoPath() const
{
    return Path(".siteinfo/" + dir,  strippedExtension(file) + ".info");
}

bool Path::removePath() const
{
    std::remove(str().c_str());

    return 0;
}

bool Path::ensurePathExists() const
{
    std::deque<Directory> dDeque = dirDeque(dir);
    std::string cDir="";

    for(size_t d=0; d<dDeque.size(); d++)
    {
        cDir += dDeque[d];
        //std::cout << "making sure " << cDir << " exists " << std::endl;
        mkdir(cDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    if(file.length())
    {
        //std::cout << "making sure " << str() << " exists " << std::endl;
        creat(str().c_str(), O_CREAT);
    }

    return 0;
}

//equality relation
bool operator==(const Path &path1, const Path &path2)
{
    return (path1.comparableStr() == path2.comparableStr());
}

//inequality relation
bool operator!=(const Path &path1, const Path &path2)
{
    return (path1.comparableStr() != path2.comparableStr());
}

//less than relation
bool operator<(const Path &path1, const Path &path2)
{
    return (path1.comparableStr() < path2.comparableStr());
}

