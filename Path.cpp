#include "Path.h"

Path::Path()
{
    type = "none";
}

Path::Path(const Directory& Dir, const Filename& File)
{
    dir = Dir;
    file = File;
    type = "file";
}

Path::Path(const std::string& path)
{
	set_file_path_from(path);
}

std::string Path::str() const
{
    return dir + file;
    //return quote(dir + file);
    /*
        consider:  <a href=@pathtofile('site/pdfs/Example name.pdf')>pdf</a>
            vs     <a href="@pathtofile('site/pdfs/Example name.pdf')">pdf</a>
        I prefer the latter! Plus first wont even work for paths
        that don't need to be quoted (though quite rare)
    */
}

//removes ./ from the front if it's there
std::string Path::comparableStr() const
{
    return comparable(dir) + file;
}

//outputs path quoted (coloured purple if to console)
std::ostream& operator<<(std::ostream& os, const Path& path)
{
	os << c_purple << quote(path.str()) << c_white;

    return os;
}

void Path::set_file_path_from(const std::string& s)
{
    size_t pos = s.find_last_of('/');
    if(pos == std::string::npos)
        *this = Path("", s);
    else
        *this = Path(s.substr(0, (pos+1)),
                     s.substr(pos+1, s.size()-(pos+1)) );
}

std::istream& Path::read_file_path_from(std::istream& is)
{
    std::string s;
    read_quoted(is, s);
    set_file_path_from(s);

    return is;
}

//returns whether first file was modified after second file
bool Path::modified_after(const Path& path2) const
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
    return Path(".nsm/" + dir,  strippedExtension(file) + ".info");
}

Path Path::getPaginationPath() const
{
    return Path(".nsm/" + dir,  strippedExtension(file) + ".pagination");
}

bool Path::ensureDirExists() const
{
    std::deque<Directory> dDeque = dirDeque(dir);
    std::string cDir="";

    for(size_t d=0; d<dDeque.size(); d++)
    {
        cDir += dDeque[d];
        #if defined _WIN32 || defined _WIN64
            _mkdir(cDir.c_str()); //windows specific
        #else //osx/unix
            mkdir(cDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); //unix/linux/osx specific
        #endif
    }

    return 0;
}

bool Path::ensureFileExists() const
{
    ensureDirExists();

    if(file.length())
    {
        close(creat(str().c_str(), O_CREAT));
		chmod(str().c_str(), 0644);
    }

    return 0;
}

//equality relation
bool operator==(const Path& path1, const Path& path2)
{
    return (path1.comparableStr() == path2.comparableStr());
}

//inequality relation
bool operator!=(const Path& path1, const Path& path2)
{
    return (path1.comparableStr() != path2.comparableStr());
}

//less than relation
bool operator<(const Path& path1, const Path& path2)
{
    return (path1.comparableStr() < path2.comparableStr());
}

#if defined _WIN32 || defined _WIN64
	void clear_console_line()
	{
		std::string bLine = std::string(console_width()-1, '\b');
		std::cout << bLine << std::string(bLine.size(), ' ') << bLine;

		//std::cout << std::flush;
		std::fflush(stdout);
	}
#else
    void clear_console_line()
	{
        std::cout << "\33[2K\r";
		//std::cout << std::flush;
		std::fflush(stdout);
	}
#endif

const std::string errStr = "error";
const std::string warnStr = "warning";

std::ostream& start_err(std::ostream& eos)
{
	clear_console_line();

    if(&eos == &std::cout)
        eos << c_red << "\a" << errStr << c_white << ": ";
    else
        eos << errStr << ": ";

    return eos;
}

std::ostream& start_err(std::ostream& eos, const Path& readPath)
{
	clear_console_line();

    if(&eos == &std::cout)
    {
        eos << c_red << "\a" << errStr << c_white << ": ";
		if(readPath.str() != "")
			eos << readPath << ": ";
    }
    else
        eos << errStr << ": " << readPath << ": ";

    return eos;
}

std::ostream& start_err(std::ostream& eos, const Path& readPath, const int& lineNo)
{
	clear_console_line();

    if(&eos == &std::cout)
    {
        eos << c_red << "\a" << errStr << c_white << ": ";
		if(readPath.str() != "")
			eos << readPath;
		eos << "[" << c_gold << lineNo << c_white << "]: ";
    }
    else
        eos << errStr << ": " << readPath << ": line " << lineNo << ": ";

    return eos;
}

std::ostream& start_err_ml(std::ostream& eos, const Path& readPath, const int& sLineNo, const int& eLineNo)
{
    if(sLineNo == eLineNo)
        return start_err(eos, readPath, sLineNo);

    clear_console_line();
    if(&eos == &std::cout)
    {
        eos << c_red << "\a" << errStr << c_white << ": ";
		if(readPath.str() != "")
			eos << readPath;
		eos << "[" << c_gold << sLineNo << "-" << eLineNo << c_white << "]: ";
    }
    else
        eos << errStr << ": " << readPath << ": lines " << sLineNo << "-" << eLineNo << ": ";

    return eos;
}

std::ostream& start_warn(std::ostream& eos)
{
	clear_console_line();

    if(&eos == &std::cout)
        eos << c_aqua << "\a" << warnStr << c_white << ": ";
    else
        eos << warnStr << ": ";

    return eos;
}

std::ostream& start_warn(std::ostream& eos, const Path& readPath)
{
	clear_console_line();

    if(&eos == &std::cout)
    {
        eos << c_aqua << "\a" << warnStr << c_white << ": ";
		if(readPath.str() != "")
			eos << readPath << ": ";
    }
    else
        eos << warnStr << ": " << readPath << ": ";

    return eos;
}

std::ostream& start_warn(std::ostream& eos, const Path& readPath, const int& lineNo)
{
	clear_console_line();

    if(&eos == &std::cout)
    {
        eos << c_aqua << "\a" << warnStr << c_white << ": ";
		if(readPath.str() != "")
			eos << readPath;
		eos << "[" << c_gold << lineNo << c_white << "]: ";
    }
    else
        eos << warnStr << ": " << readPath << ": line " << lineNo << ": ";

    return eos;
}

