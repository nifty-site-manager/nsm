/*
    Nift (aka nsm) is a cross-platform open source
    git-like and LaTeX-like command-line site manager.

    site: https://nift.cc

    Copyright (c) 2015-present
    https://n-ham.com
*/

#include <atomic>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "SiteInfo.h"
#include "Timer.h"

std::atomic<int> trash;

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
    std::string owd = get_pwd();
    trash = chdir(dir.c_str());

    std::vector<std::string> files = lsVec("./");
    for(size_t f=0; f<files.size(); f++)
    {
        struct stat s;

        if(stat(files[f].c_str(),&s) == 0 && s.st_mode & S_IFDIR)
            trash = delDir(files[f]);
        else
            Path("./", files[f]).removePath();
    }

    trash = chdir(owd.c_str());
    trash = rmdir(dir.c_str());

    return 0;
}

//get present git branch
std::string get_pb()
{
    std::string branch = "";

    trash = system("git status > .f242tgg43.txt");
    std::ifstream ifs(".f242tgg43.txt");

    while(ifs >> branch)
        if(branch == "branch")
            break;
    if(branch == "branch")
        ifs >> branch;

    ifs.close();
    Path("./", ".f242tgg43.txt").removePath();

    return branch;
}

//get set of git branches
std::set<std::string> get_git_branches()
{
    std::set<std::string> branches;
    std::string branch = "";

    trash = system("git branch > .f242tgg43.txt");
    std::ifstream ifs(".f242tgg43.txt");

    while(ifs >> branch)
        if(branch != "*")
            branches.insert(branch);

    ifs.close();

    Path("./", ".f242tgg43.txt").removePath();

    return branches;
}

std::atomic<bool> serving;

int read_serve_commands()
{
    std::string cmd;

    std::cout << "serving website locally, 'exit' or 'stop' to stop Nift serving" << std::endl;

    while(cmd != "exit" && cmd != "stop")
    {
        std::cout << "command: ";

        std::cin >> cmd;

        if(cmd != "exit" && cmd != "stop")
            std::cout << "unrecognised command, 'exit' or 'stop' to stop Nift serving" << std::endl;
    }

    serving = 0;

    return 0;
}

int serve()
{
    std::ofstream ofs;

    while(serving)
    {
        SiteInfo site;
        trash = site.open();

        ofs.open(".serve-build-log.txt");

        site.build_updated(ofs);

        ofs.close();

        usleep(500000);
    }

    Path("./", ".serve-build-log.txt").removePath();

    return 0;
}

void unrecognisedCommand(const std::string from, const std::string cmd)
{
    std::cout << "error: " << from << " does not recognise the command '" << cmd << "'" << std::endl;
}

bool parError(int noParams, char* argv[], const std::string &expectedNo)
{
    std::cout << "error: " << noParams << " parameters is not the " << expectedNo << " parameters expected" << std::endl;
    std::cout << "parameters given:";
    for(int p=1; p<=noParams; p++)
        std::cout << " " << argv[p];
    std::cout << std::endl;
    return 1;
}

int main(int argc, char* argv[])
{
    int noParams = argc-1;
    trash = trash + 1; //so trash is 'used'

    if(noParams == 0)
    {
        std::cout << "no commands given, nothing to do" << std::endl;
        return 0;
    }

    std::string cmd = argv[1];

    if(get_pwd() == "/")
    {
        std::cout << "error: don't run nsm from the root directory!" << std::endl;
        return 1;
    }

    if(cmd == "version" || cmd == "-version" || cmd == "--version")
    {
        std::cout << "Nift (aka nsm) v1.9" << std::endl;

        return 0;
    }
    else if(cmd == "commands")
    {
        std::cout << "+------ available commands ----------------------------------------+" << std::endl;
        std::cout << "| commands       | lists all nsm commands                          |" << std::endl;
        std::cout << "| config         | list config settings or set git email/username  |" << std::endl;
        std::cout << "| clone          | input: clone-url                                |" << std::endl;
        std::cout << "| init           | initialise managing a site - input: (site-name) |" << std::endl;
        std::cout << "| status         | lists updated and problem pages                 |" << std::endl;
        std::cout << "| info           | input: page-name-1 .. page-name-k               |" << std::endl;
        std::cout << "| info-all       | lists tracked pages                             |" << std::endl;
        std::cout << "| info-names     | lists tracked page names                        |" << std::endl;
        std::cout << "| track          | input: page-name (page-title) (template-path)   |" << std::endl;
        std::cout << "| untrack        | input: page-name                                |" << std::endl;
        std::cout << "| rm or del      | input: page-name                                |" << std::endl;
        std::cout << "| mv or move     | input: old-name new-name                        |" << std::endl;
        std::cout << "| cp or copy     | input: tracked-name new-name                    |" << std::endl;
        std::cout << "| build          | input: page-name-1 .. page-name-k               |" << std::endl;
        std::cout << "| build-updated  | builds updated pages                            |" << std::endl;
        std::cout << "| build-all      | builds all tracked pages                        |" << std::endl;
        std::cout << "| serve          | serves website locally                          |" << std::endl;
        std::cout << "| bcp            | input: commit-message                           |" << std::endl;
        std::cout << "| new-title      | input: page-name new-title                      |" << std::endl;
        std::cout << "| new-template   | input: page-name template-path                  |" << std::endl;
        std::cout << "+------------------------------------------------------------------+" << std::endl;

        return 0;
    }
    else if(cmd == "help" || cmd == "-help" || cmd == "--help")
    {
        std::cout << "nifty site manager (aka nift or nsm) is a cross-platform open source git-like and LaTeX-like site manager." << std::endl;
        std::cout << "official site: https://nift.cc/" << std::endl;
        std::cout << "enter `nift commands` or `nsm commands` for available commands" << std::endl;

        return 0;
    }

    if(cmd == "init")
    {
        //ensures correct number of parameters given
        if(noParams > 2)
            return parError(noParams, argv, "1-2");

        //ensures nsm is not managing a site from this directory or one of the ancestor directories
        std::string parentDir = "../",
            rootDir = "C:\\",
            owd = get_pwd(),
            pwd = get_pwd(),
            prevPwd;

        #ifdef _WIN32
            rootDir = "C:\\";
        #else  //unix
            rootDir = "/";
        #endif // _WIN32

        if(std::ifstream(".siteinfo/pages.list") || std::ifstream(".siteinfo/nsm.config"))
        {
            std::cout << "nsm is already managing a site in " << owd << " (or an accessible ancestor directory)" << std::endl;
            return 1;
        }

        while(pwd != rootDir || pwd == prevPwd)
        {
            //sets old pwd
            prevPwd = pwd;

            //changes to parent directory
            trash = chdir(parentDir.c_str());

            //gets new pwd
            pwd = get_pwd();

            if(std::ifstream(".siteinfo/pages.list") || std::ifstream(".siteinfo/nsm.config"))
            {
                std::cout << "nsm is already managing a site in " << owd << " (or an accessible ancestor directory)" << std::endl;
                return 1;
            }
        }
        trash = chdir(owd.c_str());

        //checks that directory is empty
        std::string str = ls("./");
        if(str != ".. . " &&
           str != ". .. " &&
           str != ".git .. . " &&
           str != ".git . .. " &&
           str != ". .git .. " &&
           str != ".. .git . " &&
           str != ". .. .git " &&
           str != ".. . .git ")
        {
            std::cout << "error: init must be run in an empty directory or empty git repository" << std::endl;
            return 1;
        }

        //sets up
        Path pagesListPath(".siteinfo/", "pages.list");
        //ensures pages list file exists
        pagesListPath.ensurePathExists();
        //adds read and write permissions to pages list file
        chmod(pagesListPath.str().c_str(), 0666);

        #ifdef _WIN32
            trash = system("attrib +h .siteinfo");
        #endif // _WIN32

        std::ofstream ofs(".siteinfo/nsm.config");
        ofs << "contentDir content/" << std::endl;
        ofs << "contentExt .content" << std::endl;
        ofs << "siteDir site/" << std::endl;
        ofs << "pageExt .html" << std::endl;
        ofs << "defaultTemplate template/page.template" << std::endl;
        ofs.close();

        pagesListPath = Path("template/", "page.template");
        //ensures pages list file exists
        pagesListPath.ensurePathExists();
        //adds read and write permissions to pages list file
        chmod(pagesListPath.str().c_str(), 0666);

        ofs.open("template/page.template");
        ofs << "<html>" << std::endl;
        ofs << "    <head>" << std::endl;
        ofs << "        @input(template/head.content)" << std::endl;
        ofs << "    </head>" << std::endl;
        ofs << std::endl;
        ofs << "    <body>" << std::endl;
        ofs << "        @inputcontent" << std::endl;
        ofs << "    </body>" << std::endl;
        ofs << "</html>" << std::endl;
        ofs << std::endl;
        ofs.close();

        ofs.open("template/head.content");
        if(noParams == 1)
            ofs << "<title>empty site - @pagetitle</title>" << std::endl;
        else if(noParams > 1)
            ofs << "<title>" << argv[2] << " - @pagetitle</title>" << std::endl;
        ofs.close();

        SiteInfo site;
        if(site.open() > 0)
            return 1;

        Name name = "index";
        Title title;
        title = get_title(name);
        site.track(name, title, site.defaultTemplate);
        site.build_updated(std::cout);

        //trash = system("nsm track index");
        //trash = system("nsm build-updated");

        std::cout << "nsm: initialised empty site in " << get_pwd() << "/.siteinfo/" << std::endl;

        return 0;
    }
    else if(cmd == "config" && noParams > 1)
    {
        //ensures correct number of parameters given
        if(noParams != 3 && noParams != 4)
        {
            std::cout << "you might have meant one of of:" << std::endl;
            std::cout << "  nsm config --global user.email" << std::endl;
            std::cout << "  nsm config --global user.name" << std::endl;
            std::cout << "  nsm config --global user.email \"you@example.com\"" << std::endl;
            std::cout << "  nsm config --global user.name \"Your Username\"" << std::endl;
            return parError(noParams, argv, "1, 3 or 4");
        }

        std::string str = argv[2];
        if(str == "--global")
        {
            str = argv[3];
            if(str == "user.email")
            {
                if(noParams == 3)
                    trash = system("git config --global user.email");
                else
                {
                    std::string cmdStr = "git config --global user.email \"" + std::string(argv[4]) + "\"";
                    trash = system(cmdStr.c_str());
                }
            }
            else if(str == "user.name")
            {
                if(noParams == 3)
                    trash = system("git config --global user.name");
                else
                {
                    std::string cmdStr = "git config --global user.name \"" + std::string(argv[4]) + "\" --replace-all";
                    trash = system(cmdStr.c_str());
                }
            }
            else
            {
                unrecognisedCommand("nsm", cmd + " " + argv[2] + " " + argv[3]);
                return 1;
            }
        }
        else
        {
            unrecognisedCommand("nsm", cmd + " " + argv[2]);
            return 1;
        }
    }
    else if(cmd == "clone")
    {
        //ensures correct number of parameters given
        if(noParams != 2)
            return parError(noParams, argv, "2");

        //checks that git is configured
        std::string str;
        trash = system("git config --global user.email > .1223fsf23.txt");
        std::ifstream ifs(".1223fsf23.txt");
        ifs >> str;
        ifs.close();
        Path("./", ".1223fsf23.txt").removePath();
        if(str == "")
        {
            std::cout << "*** Please tell me who you are." << std::endl;
            std::cout << std::endl;
            std::cout << "Run" << std::endl;
            std::cout << std::endl;
            std::cout << "  nsm config --global user.email \"you@example.com\"" << std::endl;
            std::cout << "  nsm config --global user.name \"Your Username\"" << std::endl;
            std::cout << std::endl;
            std::cout << "to set your account's default identity." << std::endl;

            return 0;
        }
        trash = system("git config --global user.name > .1223fsf23.txt");
        ifs.open(".1223fsf23.txt");
        ifs >> str;
        ifs.close();
        Path("./", ".1223fsf23.txt").removePath();
        if(str == "")
        {
            std::cout << "*** Please tell me who you are." << std::endl;
            std::cout << std::endl;
            std::cout << "Run" << std::endl;
            std::cout << std::endl;
            std::cout << "  nsm config --global user.email \"you@example.com\"" << std::endl;
            std::cout << "  nsm config --global user.name \"Your Username\"" << std::endl;
            std::cout << std::endl;
            std::cout << "to set your account's default identity." << std::endl;

            return 0;
        }

        std::string cloneURL = std::string(argv[2]);

        if(cloneURL.size() && cloneURL[0] == '?')
            cloneURL = cloneURL.substr(1, cloneURL.size()-1);

        std::string cloneCmnd = "git clone " + cloneURL;
        std::string dirName = "";
        std::string parDir = "../";

        if(cloneCmnd.find('/') == std::string::npos)
        {
            std::cout << "error: no / found in provided clone url" << std::endl;
            return 1;
        }
        else if(cloneCmnd.find_last_of('/') > cloneCmnd.find_last_of('.'))
        {
            std::cout << "error: clone url should contain .git after the last /" << std::endl;
            return 1;
        }

        for(auto i=cloneCmnd.find_last_of('/')+1; i < cloneCmnd.find_last_of('.'); ++i)
            dirName += cloneCmnd[i];

        if(file_exists("./", dirName))
        {
            std::cout << "fatal: destination path '" << dirName << "' already exists." << std::endl;
            return 1;
        }

        trash = system(cloneCmnd.c_str());

        trash = chdir(dirName.c_str());

        trash = system("git checkout stage > /dev/null 2>&1 >nul 2>&1");
        if(std::ifstream("./nul"))
            Path("./", "nul").removePath();

        std::set<std::string> branches = get_git_branches();

        if(branches.count("stage") && std::ifstream(".siteinfo/nsm.config"))
        {
            trash = system("git checkout master > /dev/null 2>&1 >nul 2>&1");
            if(std::ifstream("./nul"))
                Path("./", "nul").removePath();

            trash = chdir(parDir.c_str());

            trash = system(("cp -r " + dirName + " .abcd143d > /dev/null 2>&1 >nul 2>&1").c_str());
            trash = system(("echo d | xcopy " + dirName + " .abcd143d /E /H > /dev/null 2>&1 >nul 2>&1").c_str());
            if(std::ifstream("./nul"))
                Path("./", "nul").removePath();

            /*this is an alternative to the above paragraph
            #ifdef _WIN32 //note might be using cmd-prompt/git-bash/cygwin
                cloneCmnd += " > /dev/null 2>&1 >nul 2>&1";
                rename(dirName.c_str(), ".abcd143d");
                trash = system(cloneCmnd.c_str());
                if(std::ifstream("./nul"))
                    Path("./", "nul").removePath();
            #elif _WIN64 //note might be using cmd-prompt/git-bash/cygwin
                cloneCmnd += " > /dev/null 2>&1 >nul 2>&1";
                rename(dirName.c_str(), ".abcd143d");
                trash = system(cloneCmnd.c_str());
                if(std::ifstream("./nul"))
                    Path("./", "nul").removePath();
            #else  //osx/unix
                trash = system(("cp -r " + dirName + " .abcd143d").c_str());
            #endif*/

            trash = chdir(dirName.c_str());

            trash = system("git checkout stage > /dev/null 2>&1 >nul 2>&1");
            if(std::ifstream("./nul"))
                Path("./", "nul").removePath();

            SiteInfo site;
            if(site.open() > 0)
                return 1;

            //trash = rmdir(site.siteDir.c_str()); //doesn't work for non-empty directories
            trash = delDir(site.siteDir);

            trash = chdir(parDir.c_str());
            rename(".abcd143d", (dirName + "/" + site.siteDir).c_str());
        }

        return 0;
    }
    else
    {
        //checks that we have a valid command
        if(cmd != "config" &&
           cmd != "bcp" &&
           cmd != "status" &&
           cmd != "info" &&
           cmd != "info-all" &&
           cmd != "info-names" &&
           cmd != "track" &&
           cmd != "untrack" &&
           cmd != "rm" &&
           cmd != "del" &&
           cmd != "mv" &&
           cmd != "move" &&
           cmd != "cp" &&
           cmd != "copy" &&
           cmd != "new-title" &&
           cmd != "new-template" &&
           cmd != "build-updated" &&
           cmd != "build" &&
           cmd != "build-all" &&
           cmd != "serve")
        {
            unrecognisedCommand("nsm", cmd);
            return 1;
        }

        //ensures nsm is managing a site from this directory or one of the parent directories
        std::string parentDir = "../",
            rootDir = "/",
            siteRootDir = get_pwd(),
            owd = get_pwd(),
            pwd = get_pwd(),
            prevPwd;

        while(!std::ifstream(".siteinfo/pages.list") && !std::ifstream(".siteinfo/nsm.config"))
        {
            //sets old pwd
            prevPwd = pwd;

            //changes to parent directory
            trash = chdir(parentDir.c_str());

            //gets new pwd
            pwd = get_pwd();

            //checks we haven't made it to root directory or stuck at same dir
            if(pwd == rootDir || pwd == prevPwd)
            {
                std::cout << "nsm is not managing a site from " << owd << " (or any accessible parent directories)" << std::endl;
                return 1;
            }
        }

        //ensures both pages.list and nsm.config exist
        if(!std::ifstream(".siteinfo/pages.list"))
        {
            std::cout << "error: " << get_pwd() << "/.siteinfo/pages.list is missing" << std::endl;
            return 1;
        }

        if(!std::ifstream(".siteinfo/nsm.config"))
        {
            std::cout << "error: " << get_pwd() << "/.siteinfo/nsm.config is missing" << std::endl;
            return 1;
        }

        //opens up site information
        SiteInfo site;
        if(site.open() > 0)
            return 1;

        if(cmd == "config")
        {
            std::cout << "contentDir: " << site.contentDir << std::endl;
            std::cout << "contentExt: " << site.contentExt << std::endl;
            std::cout << "siteDir: " << site.siteDir << std::endl;
            std::cout << "pageExt: " << site.pageExt << std::endl;
            std::cout << "defaultTemplate: " << site.defaultTemplate << std::endl;

            return 0;
        }
        else if(cmd == "bcp") //build-updated commit push
        {
            //ensures correct number of parameters given
            if(noParams != 2)
            {
                std::cout << "error: correct usage 'bcp \"commit message\"'" << std::endl;
                return parError(noParams, argv, "2");
            }

            //checks that git is configured
            std::string str;
            trash = system("git config --global user.email > .1223fsf23.txt");
            std::ifstream ifs(".1223fsf23.txt");
            ifs >> str;
            ifs.close();
            Path("./", ".1223fsf23.txt").removePath();
            if(str == "")
            {
                std::cout << "*** Please tell me who you are." << std::endl;
                std::cout << std::endl;
                std::cout << "Run" << std::endl;
                std::cout << std::endl;
                std::cout << "  nsm config --global user.email \"you@example.com\"" << std::endl;
                std::cout << "  nsm config --global user.name \"Your Username\"" << std::endl;
                std::cout << std::endl;
                std::cout << "to set your account's default identity." << std::endl;

                return 0;
            }
            trash = system("git config --global user.name > .1223fsf23.txt");
            ifs.open(".1223fsf23.txt");
            ifs >> str;
            ifs.close();
            Path("./", ".1223fsf23.txt").removePath();
            if(str == "")
            {
                std::cout << "*** Please tell me who you are." << std::endl;
                std::cout << std::endl;
                std::cout << "Run" << std::endl;
                std::cout << std::endl;
                std::cout << "  nsm config --global user.email \"you@example.com\"" << std::endl;
                std::cout << "  nsm config --global user.name \"Your Username\"" << std::endl;
                std::cout << std::endl;
                std::cout << "to set your account's default identity." << std::endl;

                return 0;
            }

            //checks that remote git url is set
            trash = system("git config --get remote.origin.url > .txt65232g42f.txt");
            ifs.open(".txt65232g42f.txt");
            str="";
            ifs >> str;
            ifs.close();
            Path("./", ".txt65232g42f.txt").removePath();
            if(str == "")
            {
                std::cout << "error: no remote git url set" << std::endl;
                return 1;
            }

            if(site.build_updated(std::cout))
                return 1;

            std::string commitCmnd = "git commit -m \"" + std::string(argv[2]) + "\"",
                        pushCmnd,
                        siteDirBranch,
                        siteRootDirBranch = get_pb();
            std::cout << commitCmnd << std::endl;

            trash = chdir(site.siteDir.c_str());

            siteDirBranch = get_pb();

            if(siteDirBranch != siteRootDirBranch)
            {
                pushCmnd = "git push origin " + siteDirBranch;

                trash = system("git add -A .");
                trash = system(commitCmnd.c_str());
                trash = system(pushCmnd.c_str());
            }

            trash = chdir(siteRootDir.c_str());

            pushCmnd = "git push origin " + siteRootDirBranch;

            trash = system("git add -A .");
            trash = system(commitCmnd.c_str());
            trash = system(pushCmnd.c_str());

            return 0;
        }
        else if(cmd == "status")
        {
            //ensures correct number of parameters given
            if(noParams != 1)
                return parError(noParams, argv, "1");

            return site.status();
        }
        else if(cmd == "info")
        {
            //ensures correct number of parameters given
            if(noParams <= 1)
                return parError(noParams, argv, ">1");

            std::vector<Name> pageNames;
            Name pageName;
            for(int p=2; p<argc; p++)
                pageNames.push_back(argv[p]);
            return site.info(pageNames);
        }
        else if(cmd == "info-all")
        {
            //ensures correct number of parameters given
            if(noParams > 1)
                return parError(noParams, argv, "1");

            return site.info_all();
        }
        else if(cmd == "info-names")
        {
            //ensures correct number of parameters given
            if(noParams > 1)
                return parError(noParams, argv, "1");

            return site.info_names();
        }
        else if(cmd == "track")
        {
            //ensures correct number of parameters given
            if(noParams < 2 || noParams > 4)
                return parError(noParams, argv, "2-4");

            Name newPageName = quote(argv[2]);
            Title newPageTitle;
            if(noParams >= 3)
                newPageTitle = quote(argv[3]);
            else
                newPageTitle = get_title(newPageName);

            Path newTemplatePath;
            if(noParams == 4)
                newTemplatePath.set_file_path_from(argv[4]);
            else
                newTemplatePath = site.defaultTemplate;

            return site.track(newPageName, newPageTitle, newTemplatePath);
        }
        else if(cmd == "untrack")
        {
            //ensures correct number of parameters given
            if(noParams != 2)
                return parError(noParams, argv, "2");

            Name pageNameToUntrack = argv[2];

            return site.untrack(pageNameToUntrack);
        }
        else if(cmd == "rm" || cmd == "del")
        {
            //ensures correct number of parameters given
            if(noParams != 2)
                return parError(noParams, argv, "2");

            Name pageNameToRemove = argv[2];

            return site.rm(pageNameToRemove);
        }
        else if(cmd == "mv" || cmd == "move")
        {
            //ensures correct number of parameters given
            if(noParams != 3)
                return parError(noParams, argv, "3");

            Name oldPageName = argv[2],
                 newPageName = argv[3];

            return site.mv(oldPageName, newPageName);
        }
        else if(cmd == "cp" || cmd == "copy")
        {
            //ensures correct number of parameters given
            if(noParams != 3)
                return parError(noParams, argv, "3");

            Name trackedPageName = argv[2],
                 newPageName = argv[3];

            return site.cp(trackedPageName, newPageName);
        }
        else if(cmd == "new-title")
        {
            //ensures correct number of parameters given
            if(noParams != 3)
                return parError(noParams, argv, "3");

            Name pageName = argv[2];
            Title newTitle;
            newTitle.str = argv[3];

            return site.new_title(pageName, newTitle);
        }
        else if(cmd == "new-template")
        {
            //ensures correct number of parameters given
            if(noParams != 3)
                return parError(noParams, argv, "3");

            Name pageName = argv[2];
            Path newTemplatePath;
            newTemplatePath.set_file_path_from(argv[3]);

            return site.new_template(pageName, newTemplatePath);
        }
        else if(cmd == "build-updated")
        {
            //ensures correct number of parameters given
            if(noParams > 1)
                return parError(noParams, argv, "1");

            Timer timer;

            timer.start();

            int result = site.build_updated(std::cout);

            std::cout.precision(3);
            std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

            return result;
        }
        else if(cmd == "build")
        {
            //ensures correct number of parameters given
            if(noParams <= 1)
                return parError(noParams, argv, ">1");

            Timer timer;

            timer.start();

            std::vector<Name> pageNamesToBuild;
            for(int p=2; p<argc; p++)
            {
                pageNamesToBuild.push_back(argv[p]);
            }

            int result = site.build(pageNamesToBuild);

            std::cout.precision(3);
            std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

            return result;
        }
        else if(cmd == "build-all")
        {
            //ensures correct number of parameters given
            if(noParams != 1)
                return parError(noParams, argv, "1");

            Timer timer;

            timer.start();

            int result = site.build_all();

            std::cout.precision(3);
            std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

            return result;
        }
        else if(cmd == "serve")
        {
            //ensures correct number of parameters given
            if(noParams > 2)
                return parError(noParams, argv, "1 or 2");

            if(noParams == 2 && std::string(argv[2]) != "-s")
            {
                std::cout << "error: Nift does not recognise command serve " << argv[2] << std::endl;
                return 1;
            }

            serving = 1;

            std::thread serve_thread(serve);
            if(noParams == 1)
            {
                std::thread read_serve_commands_thread(read_serve_commands);
                read_serve_commands_thread.join();
            }
            serve_thread.join();
        }
        else
        {
            unrecognisedCommand("nsm", cmd);
            return 1;
        }
    }

    return 0;
}
