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

bool remove_path(const Path& path)
{
    std::mutex os_mtx;
    return remove_path(path, -1, Path("", ""), std::cout, 0, &os_mtx);
}

bool remove_path(const Path& path,
          const int& lineNo,
          const Path& readPath,
          std::ostream& eos,
          const bool& consoleLocked,
          std::mutex* os_mtx)
{
	if(remove_file(path))
    {
        if(!consoleLocked)
            os_mtx->lock();
        if(lineNo > 0)
            start_err(eos, readPath, lineNo) << "remove_path: failed to remove " << path << std::endl;
        else
            start_err(eos) << "remove_path: failed to remove " << path << std::endl;
        os_mtx->unlock();
        return 1;
    }

    std::string delDir = path.dir;
    size_t pos;

    int delDirSize = delDir.size();
    if(delDirSize && (delDir[delDirSize-1] == '/' || delDir[delDirSize-1] == '\\'))
        delDir = delDir.substr(0, delDirSize-1);

    while(delDir.size() &&
          delDir != "/" && 
          delDir != "C:" &&
          delDir != "C:\\" &&
          delDir != "C:/" &&
          delDir != "./" &&
          !rmdir(delDir.c_str()))
    {
        pos = delDir.find_last_of("/\\");
        if(pos == std::string::npos)
            break;
        else
            delDir = delDir.substr(0, pos);
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
    else if(path.dir.substr(0, 1) == "~" || path.dir.substr(0, 13) == "%userprofile%")
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
    std::string toPrint;
    size_t p=0;
    for(auto path=paths.begin(); (maxNoPaths==0 || p < maxNoPaths) && path!=paths.end(); ++p, ++path)
    {
        if(first)
            first = 0;
        else
            std::cout << separator; 

        if(path->find_first_of(' ') == std::string::npos)
            toPrint = *path;
        else
            toPrint = quote(*path);

        if(highlight)
        {
            if(dir_exists(dir + *path))
                std::cout << c_light_blue << toPrint << c_white;
            else if(exec_exists(dir + *path))
                std::cout << c_green << toPrint << c_white;
            else if(file_exists(dir + *path))
                std::cout << toPrint;
            else
                std::cout << c_red << toPrint << c_white;
        }
        else
            std::cout << toPrint;
    }
    if(maxNoPaths &&  maxNoPaths < paths.size())
        std::cout << c_white << " (and " << paths.size() - maxNoPaths << " more)";
    std::cout << std::flush;
}

//don't use this anywhere with multithreading!
int delDirOld(const std::string& dir)
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

int delDir(const std::string& dir)
{
    std::mutex os_mtx;
    return delDir(dir, -1, Path("", ""), std::cout, 0, &os_mtx);
}

int delDir(const std::string& dir,
           const int& lineNo,
           const Path& readPath,
           std::ostream& eos,
           const bool& consoleLocked,
           std::mutex* os_mtx)
{
    if(!dir_exists(dir))
        return 0;
    int ret_val;

    std::vector<std::string> files = lsVec(dir.c_str());
    Path filePath;
    if(dir.size())
    {
        if(dir[dir.size()-1] == '/' || dir[dir.size()-1] == '\\')
            filePath.dir = dir;
        else
            filePath.dir = dir + "/";
    }
    else
        filePath.dir = "./";
    for(size_t f=0; f<files.size(); f++)
    {
        struct stat s;

        filePath.file = files[f];
        files[f] = filePath.str();

        //std::cout << "wtf " << quote(files[f]) << std::endl;

        if(stat(files[f].c_str(),&s) == 0 && s.st_mode & S_IFDIR)
        {
            ret_val = delDir(files[f]);

            if(ret_val)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                if(lineNo > 0)
                    start_err(eos, readPath, lineNo) << "delDir(" << quote(dir) << "): failed to remove (empty) directory " << files[f] << std::endl;
                else
                    start_err(eos) << "delDir(" << quote(dir) << "): failed to remove (empty) directory " << files[f] << std::endl;
                os_mtx->unlock();
                return ret_val;
            }
        }
        else
            remove_file(filePath);
    }

    ret_val = rmdir(dir.c_str());
    if(ret_val)
    {
        if(!consoleLocked)
            os_mtx->lock();
        if(lineNo > 0)
            start_err(eos, readPath, lineNo) << "delDir(" << quote(dir) << "): failed to remove (empty) directory " << quote(dir) << std::endl;
        else
            start_err(eos) << "delDir(" << quote(dir) << "): failed to remove (empty) directory " << quote(dir) << std::endl;
        os_mtx->unlock();
        return ret_val;
    }

    return 0;
}

int cpDir(const std::string& sourceDir, 
          const std::string& targetDir)
{
    std::mutex os_mtx;
    return cpDir(sourceDir, targetDir, -1, Path("", ""), std::cout, 0, &os_mtx);
}

int cpDir(const std::string& sourceDir, 
          const std::string& targetDir,
          const int& lineNo,
          const Path& readPath,
          std::ostream& eos,
          const bool& consoleLocked,
          std::mutex* os_mtx)
{
    if(!dir_exists(sourceDir))
        return 0;
    else if(!targetDir.size() || targetDir == "/" || targetDir == "C:\\")
    {
        if(!consoleLocked)
            os_mtx->lock();
        if(lineNo > 0)
            start_err(eos, readPath, lineNo) << "will not replace " << Path(targetDir, "") << " with " << Path(sourceDir, "") << std::endl;
        else
            start_err(eos) << "will not replace " << Path(targetDir, "") << " with " << Path(sourceDir, "") << std::endl;
        os_mtx->unlock();
        return 1;
    }
    int ret_val;

    std::vector<std::string> files = lsVec(sourceDir.c_str());
    Path sourcePath, targetPath;
    std::string targetStr;
    if(sourceDir.size())
    {
        if(sourceDir[sourceDir.size()-1] == '/' || sourceDir[sourceDir.size()-1] == '\\')
            sourcePath.dir = sourceDir;
        else
            sourcePath.dir = sourceDir + "/";
    }
    else
        sourcePath.dir = "./";
    if(targetDir.size())
    {
        if(targetDir[sourceDir.size()-1] == '/' || targetDir[targetDir.size()-1] == '\\')
            targetPath.dir = targetDir;
        else
            targetPath.dir = targetDir + "/";
    }
    else
        targetPath.dir = "./";

    if(dir_exists(targetDir))
    {
        ret_val = delDir(targetDir);

        if(ret_val)
        {
            if(!consoleLocked)
                os_mtx->lock();
            if(lineNo > 0)
                start_err(eos, readPath, lineNo) << "cpDir(" << quote(sourceDir) << ", " << quote(targetDir) << "): failed to remove existing directory " << quote(targetDir) << std::endl;
            else
                start_err(eos) << "cpDir(" << quote(sourceDir) << ", " << quote(targetDir) << "): failed to remove existing directory " << quote(targetDir) << std::endl;
            os_mtx->unlock();
            return ret_val;
        }
    }
    else if(file_exists(targetDir))
    {
        ret_val = remove_file(targetPath);

        if(ret_val)
        {
            if(!consoleLocked)
                os_mtx->lock();
            if(lineNo > 0)
                start_err(eos, readPath, lineNo) << "cpDir(" << quote(sourceDir) << ", " << quote(targetDir) << "): failed to remove existing file " << quote(targetDir) << std::endl;
            else
                start_err(eos) << "cpDir(" << quote(sourceDir) << ", " << quote(targetDir) << "): failed to remove existing file " << quote(targetDir) << std::endl;
            os_mtx->unlock();
            return ret_val;
        }
    }

    ret_val = targetPath.ensureDirExists();
    if(ret_val)
    {
        if(!consoleLocked)
            os_mtx->lock();
        if(lineNo > 0)
            start_err(eos, readPath, lineNo) << "cpDir(" << quote(sourceDir) << ", " << quote(targetDir) << "): failed to create directory " << targetPath << std::endl;
        else
            start_err(eos) << "cpDir(" << quote(sourceDir) << ", " << quote(targetDir) << "): failed to create directory " << targetPath << std::endl;
        os_mtx->unlock();
        return ret_val;
    }

    for(size_t f=0; f<files.size(); f++)
    {
        struct stat s;

        targetPath.file = files[f];
        targetStr = targetPath.str();
        sourcePath.file = files[f];
        files[f] = sourcePath.str();

        //std::cout << "wtf " << quote(files[f]) << " " << quote(targetStr) << std::endl;

        if(stat(files[f].c_str(),&s) == 0 && s.st_mode & S_IFDIR)
        {
            ret_val = cpDir(files[f], targetStr, lineNo, readPath, eos, consoleLocked, os_mtx);

            if(ret_val)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                if(lineNo > 0)
                    start_err(eos, readPath, lineNo) << "cpDir(" << quote(sourceDir) << ", " << quote(targetDir) << "): failed to copy directory " << sourcePath << " to " << targetPath << std::endl;
                else
                    start_err(eos) << "cpDir(" << quote(sourceDir) << ", " << quote(targetDir) << "): failed to copy directory " << sourcePath << " to " << targetPath << std::endl;
                os_mtx->unlock();
                return ret_val;
            }
        }
        else
        {
            ret_val = cpFile(files[f], targetStr, lineNo, readPath, eos, consoleLocked, os_mtx);

            if(ret_val)
            {
                if(!consoleLocked)
                    os_mtx->lock();
                if(lineNo > 0)
                    start_err(eos, readPath, lineNo) << "cpDir(" << quote(sourceDir) << ", " << quote(targetDir) << "): failed to copy file " << sourcePath << " to " << targetPath << std::endl;
                else
                    start_err(eos) << "cpDir(" << quote(sourceDir) << ", " << quote(targetDir) << "): failed to copy file " << sourcePath << " to " << targetPath << std::endl;
                os_mtx->unlock();
                return ret_val;
            }
        }
    }

    return 0;
}

int cpDirOld(const std::string& sourceDir, const std::string& targetDir)
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

int cpFile(const std::string& sourceFile, 
           const std::string& targetFile)
{
    std::mutex os_mtx;
    return cpFile(sourceFile, targetFile, -1, Path("", ""), std::cout, 0, &os_mtx);
}

int cpFile(const std::string& sourceFile, 
           const std::string& targetFile,
           const int& lineNo,
           const Path& readPath,
           std::ostream& eos,
           const bool& consoleLocked,
           std::mutex* os_mtx)
{
    Path targetPath;
    targetPath.set_file_path_from(targetFile);

	if(!file_exists(sourceFile))
	{
        Path sourcePath;
        sourcePath.set_file_path_from(sourceFile);
		if(!consoleLocked)
            os_mtx->lock();
        if(lineNo > 0)
            start_err(eos, readPath, lineNo) << "cannot copy " << sourcePath << " as file does not exist" << std::endl;
        else
            start_err(eos) << "cannot copy " << sourcePath << " as file does not exist" << std::endl;
        os_mtx->unlock();
        return 1;
	}
	else if(targetFile == "/" || targetFile == "C:\\")
	{
        Path sourcePath;
        sourcePath.set_file_path_from(sourceFile);
		if(!consoleLocked)
            os_mtx->lock();
        if(lineNo > 0)
            start_err(eos, readPath, lineNo) << "will not copy " << sourcePath << " to " << targetPath << std::endl;
        else
            start_err(eos) << "will not copy " << sourcePath << " to " << targetPath << std::endl;
        os_mtx->unlock();
        return 1;
	}

    targetPath.ensureDirExists();

	std::ifstream ifs(sourceFile);
	std::ofstream ofs(targetFile);

    if(!ifs.is_open())
    {
        Path sourcePath;
        sourcePath.set_file_path_from(sourceFile);
        if(!consoleLocked)
            os_mtx->lock();
        if(lineNo > 0)
            start_err(eos, readPath, lineNo) << "could not open " << sourcePath << " for reading" << std::endl;
        else
            start_err(eos) << "could not open " << sourcePath << " for reading" << std::endl;
        os_mtx->unlock();
        return 1;
    }
    else if(!ofs.is_open())
    {
        if(!consoleLocked)
            os_mtx->lock();
        if(lineNo > 0)
            start_err(eos, readPath, lineNo) << "could not open " << targetPath << " for writing" << std::endl;
        else
            start_err(eos) << "could not open " << targetPath << " for writing" << std::endl;
        os_mtx->unlock();
        return 1;
    }

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
