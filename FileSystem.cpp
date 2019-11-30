#include "FileSystem.h"

std::string get_pwd()
{
    char * pwd_char = getcwd(NULL, 0);
    std::string pwd = pwd_char;
    free(pwd_char);
    return pwd;
}

bool file_exists(const char *path, const std::string& file)
{
    bool exists = 0;
    std::string cFile;
    struct dirent *entry;
    DIR *dir = opendir(path);
    if(dir == NULL)
        return "";

    while((entry = readdir(dir)) != NULL)
    {
        cFile = entry->d_name;
        if(cFile == file)
        {
            exists = 1;
            break;
        }
    }

    closedir(dir);

    return exists;
}

std::string ls(const char *path)
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

std::vector<std::string> lsVec(const char *path)
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

int delDir(std::string dir)
{
	if(!std::ifstream(dir))
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
            Path("./", files[f]).removePath();
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

    return 0;
}

int cpDir(const std::string& sourceDir, const std::string& targetDir)
{
    int ret_val = system(("cp -r " + sourceDir + " " + targetDir + " > /dev/null 2>&1 >nul 2>&1").c_str());
    if(ret_val)
        ret_val = system(("echo d | xcopy " + sourceDir + " " + targetDir + " /E /H > /dev/null 2>&1 >nul 2>&1").c_str());
    if(std::ifstream("./nul"))
        Path("./", "nul").removePath();
    if(ret_val)
        std::cout << "error: FileSystem.cpp: cpDir(" << quote(sourceDir) << ", " << quote(targetDir) << "): copy failed" << std::endl;
    return ret_val;
}

int cpFile(const std::string& sourceFile, const std::string& targetFile)
{
	if(!std::ifstream(sourceFile))
	{
		std::cout << "error: cannot copy '" << sourceFile << "' as file does not exist" << std::endl;
		return 1;
	}
	else if(std::ifstream(targetFile))
	{
		std::cout << "error: will not copy '" << sourceFile << "' to '" << targetFile << "' as target file already exists" << std::endl;
		return 1;
	}

	std::ifstream ifs(sourceFile);
	std::ofstream ofs(targetFile);

	ofs << ifs.rdbuf();

	ofs.close();
	ifs.close();

	return 0;
}
