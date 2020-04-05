#include "FileSystem.h"

std::string get_pwd()
{
    char * pwd_char = getcwd(NULL, 0);
    std::string pwd = pwd_char;
    free(pwd_char);
    return pwd;
}

bool path_exists(const std::string& path)
{
    struct stat info;

    if(stat( path.c_str(), &info ) != 0) //no file
        return 0;
    else
        return 1;
}

bool dir_exists(const std::string& path)
{
    struct stat info;

    if(stat( path.c_str(), &info ) != 0) //no file
        return 0;
    else if(info.st_mode & S_IFDIR) //dir
        return 1;
    else //file
        return 0;
}

bool file_exists(const std::string& path)
{
    struct stat info;

    if(stat( path.c_str(), &info ) != 0) //no file
        return 0;
    else if(info.st_mode & S_IFDIR) //dir
        return 0;
    else //file
        return 1;
}

bool exec_exists(const std::string& path)
{
    struct stat info;

    if(stat( path.c_str(), &info ) != 0) //no file
        return 0;
    else if(info.st_mode & S_IEXEC) //executable
        return 1;
    else //file
        return 0;
}

bool can_exec(const char *file)
{
    struct stat  st;

    if (stat(file, &st) < 0)
        return false;
    if ((st.st_mode & S_IEXEC) != 0)
        return true;
    return false;
}

bool remove_file(const Path& path)
{
	if(path.file != "" && path_exists(path.str())) //should this just be file_exists?
        if(std::remove(path.str().c_str()))
			return 1;

	return 0;
}

//don't use this anywhere with multithreading!
bool remove_path(const Path& path)
{
	if(remove_file(path))
		return 1;

	/*
		if(!dir_exists(path.dir))
			return 0;
	*/

	std::string owd = get_pwd(),
	            pwd = owd,
	            delDir,
				parDir = "../";

	if(chdir(path.dir.c_str()))
	{
		std::cout << "error: remove_path(" << path.str() << "): failed to change directory to " << quote(path.dir) << " from " << quote(get_pwd()) << std::endl;
		return 1;
	}
	delDir = get_pwd();

	if(chdir(parDir.c_str()))
	{
		std::cout << "error: remove_path(" << path.str() << "): failed to change directory to parent " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
		return 1;
	}
	pwd = get_pwd();

	while(!rmdir(delDir.c_str()))
	{
		if(pwd == "/" || pwd == "C:\\" || pwd == owd)
			break;
		delDir = pwd;
		if(chdir(parDir.c_str()))
		{
			std::cout << "error: remove_path(" << path.str() << "): failed to change directory to parent " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
			return 1;
		}
		pwd = get_pwd();
	}

	if(chdir(owd.c_str()))
	{
		std::cout << "error: remove_path(" << path.str() << "): failed to change directory to " << quote(owd) << " from " << quote(get_pwd()) << std::endl;
		return 1;
	}

	return 0;
}

std::string ls(const char* path)
{
    std::string lsStr;
    struct dirent *entry;
    DIR *dir = opendir(path);
    if(dir == NULL)
        return "";

    while((entry = readdir(dir)) != NULL)
    {
        lsStr += entry->d_name;
        lsStr += " ";
    }

    closedir(dir);

    return lsStr;
}

std::vector<std::string> lsVec(const char* path)
{
    std::vector<std::string> ans;
    struct dirent *entry;
    DIR *dir = opendir(path);
    if(dir == NULL)
        return ans;

    while((entry = readdir(dir)) != NULL)
        if(std::string(entry->d_name) != "." && std::string(entry->d_name) != "..")
            ans.push_back(entry->d_name);

    closedir(dir);

    return ans;
}

std::set<std::string> lsSet(const char* path)
{
    std::set<std::string> ans;
    struct dirent *entry;
    DIR *dir = opendir(path);
    if(dir == NULL)
        return ans;

    while((entry = readdir(dir)) != NULL)
        if(std::string(entry->d_name) != "." && std::string(entry->d_name) != "..")
            ans.insert(entry->d_name);

    closedir(dir);

    return ans;
}

void makeSearchable(Path& path)
{
    if(path.dir.substr(0, 3) == "C:\\")
        return;
    else if(path.dir.substr(0, 2) == "./")
        return;
    else if(path.dir.substr(0, 2) == "~/")
    {
        path.dir = replace_home_dir(path.dir);
        return;
    }
    else if(path.dir.size() && path.dir[0] == '/')
        return;

    path.dir = "./" + path.dir;
}

std::set<std::string> lsSetStar(const Path& path, const int& incHidden)
{
    std::set<std::string> ans;
    struct dirent *entry;
    DIR *dir = opendir(path.dir.c_str());
    if(dir == NULL)
        return ans;
    std::string file;

    while((entry = readdir(dir)) != NULL)
    {
        file = std::string(entry->d_name);
        if(!incHidden && file.size() && file[0] == '.')
            continue;
        else if(incHidden == -1 && (file == "." || file == ".."))
            continue;
        else if(file.substr(0, path.file.size()) == path.file)
        {
            if(dir_exists(path.dir + file) && file != "." && file != "..")
                ans.insert(file + "/");
            else
                ans.insert(file);
        }
    }

    closedir(dir);

    return ans;
}

void coutPaths(const std::string& dir,
               const std::set<std::string>& paths, 
               const std::string& separator,
               const bool& highlight,
               const size_t& maxNoPaths)
{
    bool first = 1;
    size_t p=0;
    for(auto path=paths.begin(); p < maxNoPaths && path!=paths.end(); ++p, ++path)
    {
        if(first)
            first = 0;
        else
            std::cout << separator; 

        if(highlight)
        {
            if(dir_exists(dir + *path))
                std::cout << c_light_blue << *path << c_white;
            else if(exec_exists(dir + *path))
                std::cout << c_green << *path << c_white;
            else if(file_exists(dir + *path))
                std::cout << *path;
            else
                std::cout << c_red << *path << c_white;
        }
        else
            std::cout << *path;
    }
    if(maxNoPaths < paths.size())
        std::cout << c_white << " (and " << paths.size() - maxNoPaths << " more)";
    std::cout << std::flush;
}

//don't use this anywhere with multithreading!
int delDir(const std::string& dir)
{
	if(!dir_exists(dir))
		return 0;
    std::string owd = get_pwd();
    int ret_val = chdir(dir.c_str());

    std::vector<std::string> files = lsVec("./");
    for(size_t f=0; f<files.size(); f++)
    {
        struct stat s;

        if(stat(files[f].c_str(),&s) == 0 && s.st_mode & S_IFDIR)
        {
            ret_val = delDir(files[f]);

            if(ret_val)
            {
                std::cout << "error: FileSystem.cpp: delDir(" << quote(dir) << "): failed to remove (empty) directory " << files[f] << std::endl;
                return ret_val;
            }
        }
        else
            remove_file(Path("./", files[f]));
    }

    ret_val = chdir(owd.c_str());
    if(ret_val)
    {
        std::cout << "error: FileSystem.cpp: delDir(" << quote(dir) << "): failed to change directory to " << quote(owd) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    ret_val = rmdir(dir.c_str());
    if(ret_val)
    {
        std::cout << "error: FileSystem.cpp: delDir(" << quote(dir) << "): failed to remove (empty) directory " << quote(dir) << std::endl;
        return ret_val;
    }

	//this causes errors! eg. with clone (though may work after not allowing remove_path to go past the owd)
	//plus would be quite inefficient as it's a recursive function!
    /*ret_val = remove_path(Path(dir, ""));
    if(ret_val)
    {
        std::cout << "error: FileSystem.cpp: delDir(" << quote(dir) << "): failed to remove (empty) directory " << quote(dir) << std::endl;
        return ret_val;
    }*/

    return 0;
}

int cpDir(const std::string& sourceDir, const std::string& targetDir)
{
    int ret_val = system(("cp -r " + sourceDir + " " + targetDir + " > /dev/null 2>&1 >nul 2>&1").c_str());
    if(ret_val)
        ret_val = system(("echo d | xcopy " + sourceDir + " " + targetDir + " /E /H > /dev/null 2>&1 >nul 2>&1").c_str());
    if(file_exists("./nul"))
        remove_file(Path("./", "nul"));
    if(ret_val)
        std::cout << "error: FileSystem.cpp: cpDir(" << quote(sourceDir) << ", " << quote(targetDir) << "): copy failed" << std::endl;
    return ret_val;
}

int cpFile(const std::string& sourceFile, const std::string& targetFile)
{
	if(!file_exists(sourceFile))
	{
		std::cout << "error: cannot copy '" << sourceFile << "' as file does not exist" << std::endl;
		return 1;
	}
	else if(file_exists(targetFile))
	{
		std::cout << "error: will not copy '" << sourceFile << "' to '" << targetFile << "' as target file already exists" << std::endl;
		return 1;
	}

	Path targetPath;
	targetPath.set_file_path_from(targetFile);
	targetPath.ensureDirExists();

	std::ifstream ifs(sourceFile);
	std::ofstream ofs(targetFile);

	ofs << ifs.rdbuf();

	ofs.close();
	ifs.close();

	return 0;
}

std::string ifs_to_string(std::ifstream& ifs)
{
	std::string s;
	getline(ifs, s, (char) ifs.eof());
	return s;
}

std::string string_from_file(const std::string& path)
{
	std::string s;
	std::ifstream ifs(path);
	getline(ifs, s, (char) ifs.eof());
	ifs.close();
	return s;	
}
