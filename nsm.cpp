/*
    Nift (aka nsm) is a cross-platform open source
    git-like and LaTeX-like command-line site manager.

    official website: https://nift.cc
    github: https://github.com/nifty-site-manager/nsm

    Copyright (c) 2015-present
    Creator: Nicholas Ham
    https://n-ham.com
*/

#include "GitInfo.h"
#include "SiteInfo.h"
#include "Timer.h"

std::atomic<bool> serving;

int read_serve_commands()
{
    std::string cmd;

    std::cout << "serving website locally - 'exit', 'stop' or 'ctrl c' to stop Nift serving" << std::endl;

    while(cmd != "exit" && cmd != "stop")
    {
        std::cout << "command: ";

        std::cin >> cmd;

        if(cmd != "exit" && cmd != "stop")
            std::cout << "unrecognised command - 'exit', 'stop' or 'ctrl c' to stop Nift serving" << std::endl;
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
        if(site.open())
        {
            std::cout << "error: nsm.cpp: serve(): failed to open site" << std::endl;
            std::cout << "Nift is no longer serving!" << std::endl;
            return 1;
        }

        ofs.open(".serve-build-log.txt");

        if(!run_scripts(ofs, "pre-build.scripts"))
            site.build_updated(ofs);

        run_scripts(ofs, "post-build.scripts");

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
    Timer timer;
    timer.start();

    int noParams = argc-1;
    int ret_val;

    if(noParams == 0)
    {
        std::cout << "no commands given, nothing to do" << std::endl;
        return 0;
    }

    std::string cmd = argv[1];

    if(get_pwd() == "/")
    {
        std::cout << "error: don't run Nift from the root directory!" << std::endl;
        return 1;
    }

    if(cmd == "version" || cmd == "-version" || cmd == "--version")
    {
        std::cout << "Nift (aka nsm) v1.19" << std::endl;

        return 0;
    }
    else if(cmd == "commands")
    {
        std::cout << "+------ available commands ----------------------------------------+" << std::endl;
        std::cout << "| commands       | lists all Nift commands                         |" << std::endl;
        std::cout << "| config         | list config settings or set git email/username  |" << std::endl;
        std::cout << "| clone          | input: clone-url                                |" << std::endl;
        std::cout << "| diff           | input: file-path                                |" << std::endl;
        std::cout << "| pull           | pull remote changes locally                     |" << std::endl;
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
        std::cout << "| new-template   | input: (page-name) template-path                |" << std::endl;
        std::cout << "| new-site-dir   | input: dir-path                                 |" << std::endl;
        std::cout << "| new-cont-dir   | input: dir-path                                 |" << std::endl;
        std::cout << "| new-cont-ext   | input: (page-name) content-extension            |" << std::endl;
        std::cout << "| new-page-ext   | input: (page-name) page-extension               |" << std::endl;
        std::cout << "+------------------------------------------------------------------+" << std::endl;

        return 0;
    }
    else if(cmd == "help" || cmd == "-help" || cmd == "--help")
    {
        std::cout << "Nift (aka nifty-site-manager or nsm) is a cross-platform open source git-like and LaTeX-like site manager." << std::endl;
        std::cout << "official site: https://nift.cc/" << std::endl;
        std::cout << "enter `nift commands` or `nsm commands` for available commands" << std::endl;

        return 0;
    }

    if(cmd == "init")
    {
        //ensures correct number of parameters given
        if(noParams > 2)
            return parError(noParams, argv, "1-2");

        //ensures Nift is not managing a site from this directory or one of the ancestor directories
        std::string parentDir = "../",
            rootDir = "C:\\",
            owd = get_pwd(),
            pwd = get_pwd(),
            prevPwd;

        #if defined _WIN32 || defined _WIN64
            rootDir = "C:\\";
        #else  //unix
            rootDir = "/";
        #endif

        if(std::ifstream(".siteinfo/pages.list") || std::ifstream(".siteinfo/nsm.config"))
        {
            std::cout << "Nift is already managing a site in " << owd << " (or an accessible ancestor directory)" << std::endl;
            return 1;
        }

        while(pwd != rootDir || pwd == prevPwd)
        {
            //sets old pwd
            prevPwd = pwd;

            //changes to parent directory
            ret_val = chdir(parentDir.c_str());
            if(ret_val)
            {
                std::cout << "error: nsm.cpp: init: failed to change directory to " << quote(parentDir) << " from " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

            //gets new pwd
            pwd = get_pwd();

            if(std::ifstream(".siteinfo/pages.list") || std::ifstream(".siteinfo/nsm.config"))
            {
                std::cout << "Nift is already managing a site in " << owd << " (or an accessible ancestor directory)" << std::endl;
                return 1;
            }
        }
        ret_val = chdir(owd.c_str());
        if(ret_val)
        {
            std::cout << "error: nsm.cpp: init: failed to change directory to " << quote(owd) << " from " << quote(get_pwd()) << std::endl;
            return ret_val;
        }

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

        #if defined _WIN32 || defined _WIN64
            ret_val = system("attrib +h .siteinfo");

            //if handling error here need to check what happens in all of cmd prompt/power shell/git bash/cygwin/etc.
        #endif

        std::ofstream ofs(".siteinfo/nsm.config");
        ofs << "contentDir content/" << std::endl;
        ofs << "contentExt .content" << std::endl;
        ofs << "siteDir site/" << std::endl;
        ofs << "pageExt .html" << std::endl;
        ofs << "defaultTemplate template/page.template" << std::endl << std::endl;
        ofs << "rootBranch ##unset##" << std::endl;
        ofs << "siteBranch ##unset##" << std::endl;
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

        std::cout << "Nift: initialised empty site in " << get_pwd() << "/.siteinfo/" << std::endl;

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
                {
                    ret_val = system("git config --global user.email");
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: config: system('git config --global user.email') failed in " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }
                }
                else
                {
                    std::string cmdStr = "git config --global user.email \"" + std::string(argv[4]) + "\"";
                    ret_val = system(cmdStr.c_str());
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: config: system('" << cmdStr << "') failed in " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }
                }
            }
            else if(str == "user.name")
            {
                if(noParams == 3)
                {
                    ret_val = system("git config --global user.name");
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: config: system('git config --global user.name') failed in " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }
                }
                else
                {
                    std::string cmdStr = "git config --global user.name \"" + std::string(argv[4]) + "\" --replace-all";
                    ret_val = system(cmdStr.c_str());
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: config: system('" << cmdStr << "') failed in " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }
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

        if(!is_git_configured())
            return 1;

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

        ret_val = system(cloneCmnd.c_str());
        if(ret_val)
        {
            std::cout << "error: nsm.cpp: clone: system('" << cloneCmnd << "') failed in " << quote(get_pwd()) << std::endl;
            return ret_val;
        }

        ret_val = chdir(dirName.c_str());
        if(ret_val)
        {
            std::cout << "error: nsm.cpp: clone: failed to change directory to " << quote(dirName) << " from " << quote(get_pwd()) << std::endl;
            return ret_val;
        }

        std::string obranch = get_pb();
        std::set<std::string> branches = get_git_branches();

        if(branches.size() == 0)
        {
            //std::cout << "error: nsm.cpp: clone: get_git_branches() failed in " << quote(get_pwd()) << std::endl;
            std::cout << "no branches found, cloning finished" << std::endl;
            return 0;
        }

        //looks for root branch
        std::string checkoutCmnd;
        for(auto branch=branches.begin(); branch!=branches.end(); branch++)
        {
            checkoutCmnd = "git checkout " + *branch + " > /dev/null 2>&1 >nul 2>&1";
            ret_val = system(checkoutCmnd.c_str());
            if(std::ifstream("./nul"))
                Path("./", "nul").removePath();
            if(ret_val)
            {
                std::cout << "error: nsm.cpp: clone: system('" << checkoutCmnd << "') failed in " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

            if(std::ifstream(".siteinfo/nsm.config")) //found root branch
            {
                SiteInfo site;
                if(site.open())
                    return 1;

                if(!branches.count(site.rootBranch))
                {
                    std::cout << "error: nsm.cpp: clone: rootBranch " << quote(site.rootBranch) << " is not present in the git repository" << std::endl;
                    return 1;
                }
                else if(!branches.count(site.siteBranch))
                {
                    std::cout << "error: nsm.cpp: clone: siteBranch " << quote(site.siteBranch) << " is not present in the git repository" << std::endl;
                    return 1;
                }

                if(site.siteBranch != site.rootBranch)
                {
                    ret_val = chdir(parDir.c_str());
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    ret_val = cpDir(dirName, ".abcd143d");
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: failed to copy directory " << quote(dirName) << " to '.abcd143d/' from " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    ret_val = chdir(".abcd143d");
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: failed to change directory to " << quote(dirName) << " from " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    ret_val = system("git checkout -- . > /dev/null 2>&1 >nul 2>&1");
                    if(std::ifstream("./nul"))
                        Path("./", "nul").removePath();
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: system('git checkout -- .') failed in " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    checkoutCmnd = "git checkout " + site.siteBranch + " > /dev/null 2>&1 >nul 2>&1";
                    ret_val = system(checkoutCmnd.c_str());
                    if(std::ifstream("./nul"))
                        Path("./", "nul").removePath();
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: system('" << checkoutCmnd << "') failed in " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    ret_val = chdir((parDir + dirName).c_str());
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: failed to change directory to " << quote(parDir + dirName) << " from " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    ret_val = delDir(site.siteDir);
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: failed to delete directory " << quote(site.siteDir) << " from " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    ret_val = chdir(parDir.c_str());
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    //moves site dir/branch in two steps here in case site dir/branch isn't located inside stage/root dir/branch
                    rename(".abcd143d", (dirName + "/.abcd143d").c_str());

                    ret_val = chdir(dirName.c_str());
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: failed to change directory to " << quote(parDir + dirName) << " from " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    rename(".abcd143d", site.siteDir.c_str());

                    ret_val = chdir(parDir.c_str());
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    return 0;
                }
            }
        }
        //switches back to original branch
        checkoutCmnd = "git checkout " + obranch + " > /dev/null 2>&1 >nul 2>&1";
        ret_val = system(checkoutCmnd.c_str());
        if(std::ifstream("./nul"))
            Path("./", "nul").removePath();
        if(ret_val)
        {
            std::cout << "error: nsm.cpp: clone: system('" << checkoutCmnd << "') failed in " << quote(get_pwd()) << std::endl;
            return ret_val;
        }

        return 0;
    }
    else
    {
        //checks that we have a valid command
        if(cmd != "config" &&
           cmd != "bcp" &&
           cmd != "diff" &&
           cmd != "pull" &&
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
           cmd != "new-site-dir" &&
           cmd != "new-cont-dir" &&
           cmd != "new-cont-ext" &&
           cmd != "new-page-ext" &&
           cmd != "build-updated" &&
           cmd != "build" &&
           cmd != "build-all" &&
           cmd != "serve")
        {
            unrecognisedCommand("nsm", cmd);
            return 1;
        }

        //ensures Nift is managing a site from this directory or one of the parent directories
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
            ret_val = chdir(parentDir.c_str());
            if(ret_val)
            {
                std::cout << "error: nsm.cpp: A: failed to change directory to " << quote(parentDir) << " from " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

            //gets new pwd
            pwd = get_pwd();

            //checks we haven't made it to root directory or stuck at same dir
            if(pwd == rootDir || pwd == prevPwd)
            {
                std::cout << "Nift is not managing a site from " << owd << " (or any accessible parent directories)" << std::endl;
                return 1;
            }
        }

        //ensures both pages.list and nsm.config exist
        if(!std::ifstream(".siteinfo/pages.list"))
        {
            std::cout << "error: '" << get_pwd() << "/.siteinfo/pages.list' is missing" << std::endl;
            return 1;
        }

        if(!std::ifstream(".siteinfo/nsm.config"))
        {
            std::cout << "error: '" << get_pwd() << "/.siteinfo/nsm.config' is missing" << std::endl;
            return 1;
        }

        SiteInfo site;
        if(cmd == "config")
        {
            if(site.open_config())
                return 1;

            std::cout << "contentDir: " << site.contentDir << std::endl;
            std::cout << "contentExt: " << site.contentExt << std::endl;
            std::cout << "siteDir: " << site.siteDir << std::endl;
            std::cout << "pageExt: " << site.pageExt << std::endl;
            std::cout << "defaultTemplate: " << site.defaultTemplate << std::endl << std::endl;
            std::cout << "rootBranch: " << site.rootBranch << std::endl;
            std::cout << "siteBranch: " << site.siteBranch << std::endl;

            return 0;
        }
        else if(cmd == "diff") //diff of file
        {
            //ensures correct number of parameters given
            if(noParams != 2)
            {
                std::cout << "error: correct usage 'diff file-path'" << std::endl;
                return parError(noParams, argv, "2");
            }

            if(!std::ifstream(".git"))
            {
                std::cout << "error: project directory not a git repository" << std::endl;
                return 1;
            }

            if(!std::ifstream(argv[2]))
            {
                std::cout << "error: path " << quote(argv[2]) << " not in working tree" << std::endl;
                return 1;
            }
            else
            {
                ret_val = system(("git diff " + std::string(argv[2])).c_str());
                if(ret_val)
                {
                    std::cout << "error: nsm.cpp: diff: system('git diff " << std::string(argv[2]) << "') failed in " << quote(get_pwd()) << std::endl;
                    return ret_val;
                }
            }

            return 0;
        }
        else if(cmd =="pull")
        {
            if(site.open_config())
                return 1;

            //ensures correct number of parameters given
            if(noParams != 1)
                return parError(noParams, argv, "1");

            //checks that git is configured
            if(!is_git_configured())
                return 1;

            //checks that remote git url is set
            if(!is_git_remote_set())
                return 1;

            std::string pullCmnd,
                        siteDirRemote,
                        siteRootDirRemote = get_remote(),
                        siteDirBranch,
                        siteRootDirBranch = get_pb();

            if(siteRootDirRemote == "##error##")
            {
                std::cout << "error: nsm.cpp: pull: get_remote() failed in " << quote(get_pwd()) << std::endl;
                return 1;
            }

            if(siteRootDirBranch == "##error##")
            {
                std::cout << "error: nsm.cpp: pull: get_pb() failed in " << quote(get_pwd()) << std::endl;
                return 1;
            }

            if(siteRootDirBranch != site.rootBranch)
            {
                std::cout << "error: nsm.cpp: pull: root dir branch " << quote(siteRootDirBranch) << " is not the same as rootBranch " << quote(site.rootBranch) << " in .siteinfo/nsm.config" << std::endl;
                return 1;
            }

            ret_val = chdir(site.siteDir.c_str());
            if(ret_val)
            {
                std::cout << "error: nsm.cpp: pull: failed to change directory to " << quote(site.siteDir) << " from " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

            siteDirRemote = get_remote();

            if(siteDirRemote == "##error##")
            {
                std::cout << "error: nsm.cpp: pull: get_remote() failed in " << quote(get_pwd()) << std::endl;
                return 0;
            }

            siteDirBranch = get_pb();

            if(siteDirBranch == "##error##")
            {
                std::cout << "error: nsm.cpp: pull: get_pb() failed in " << quote(get_pwd()) << std::endl;
                return 0;
            }

            if(siteDirBranch != site.siteBranch)
            {
                std::cout << "error: nsm.cpp: pull: site dir branch " << quote(siteDirBranch) << " is not the same as siteBranch " << quote(site.siteBranch) << " in .siteinfo/nsm.config" << std::endl;
                return 1;
            }

            if(siteDirBranch != siteRootDirBranch)
            {
                pullCmnd = "git pull " + siteDirRemote + " " + siteDirBranch;

                ret_val = system(pullCmnd.c_str());
                if(ret_val)
                {
                    std::cout << "error: nsm.cpp: pull: system('" << pullCmnd << "') failed in " << quote(get_pwd()) << std::endl;
                    return ret_val;
                }
            }

            ret_val = chdir(siteRootDir.c_str());
            if(ret_val)
            {
                std::cout << "error: nsm.cpp: pull: failed to change directory to " << quote(siteRootDir) << " from " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

            pullCmnd = "git pull " + siteRootDirRemote + " " + siteRootDirBranch;

            ret_val = system(pullCmnd.c_str());
            if(ret_val)
            {
                std::cout << "error: nsm.cpp: pull: system('" << pullCmnd << "') failed in " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

            return 0;
        }

        //opens up nsm.config and pages.list files
        if(site.open())
            return 1;

        if(cmd == "bcp") //build-updated commit push
        {
            //ensures correct number of parameters given
            if(noParams != 2)
            {
                std::cout << "error: correct usage 'bcp \"commit message\"'" << std::endl;
                return parError(noParams, argv, "2");
            }

            if(!is_git_configured())
                return 1;

            if(!is_git_remote_set())
                return 1;

            if(site.build_updated(std::cout))
                return 1;

            std::string commitCmnd = "git commit -m \"" + std::string(argv[2]) + "\"",
                        pushCmnd,
                        siteDirBranch,
                        siteRootDirBranch = get_pb();

            if(siteRootDirBranch == "##error##")
            {
                std::cout << "error: nsm.cpp: bcp: get_pb() failed in " << quote(get_pwd()) << std::endl;
                return 0;
            }

            if(siteRootDirBranch != site.rootBranch)
            {
                std::cout << "error: nsm.cpp: bcp: root dir branch " << quote(siteRootDirBranch) << " is not the same as rootBranch " << quote(site.rootBranch) << " in .siteinfo/nsm.config" << std::endl;
                return 1;
            }

            std::cout << commitCmnd << std::endl;

            ret_val = chdir(site.siteDir.c_str());
            if(ret_val)
            {
                std::cout << "error: nsm.cpp: bcp: failed to change directory to " << quote(site.siteDir) << " from " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

            siteDirBranch = get_pb();

            if(siteDirBranch == "##error##")
            {
                std::cout << "error: nsm.cpp: bcp: get_pb() failed in " << quote(get_pwd()) << std::endl;
                return 0;
            }

            if(siteDirBranch != site.siteBranch)
            {
                std::cout << "error: nsm.cpp: pull: site dir branch " << quote(siteDirBranch) << " is not the same as siteBranch " << quote(site.siteBranch) << " in .siteinfo/nsm.config" << std::endl;
                return 1;
            }

            if(siteDirBranch != siteRootDirBranch)
            {
                pushCmnd = "git push origin " + siteDirBranch;

                ret_val = system("git add -A .");
                if(ret_val)
                {
                    std::cout << "error: nsm.cpp: bcp: system('git add -A .') failed in " << quote(get_pwd()) << std::endl;
                    return ret_val;
                }

                ret_val = system(commitCmnd.c_str());
                //can't handle error here incase we had to run bcp multiple times

                ret_val = system(pushCmnd.c_str());
                if(ret_val)
                {
                    std::cout << "error: nsm.cpp: bcp: system('" << pushCmnd << "') failed in " << quote(get_pwd()) << std::endl;
                    return ret_val;
                }
            }

            ret_val = chdir(siteRootDir.c_str());
            if(ret_val)
            {
                std::cout << "error: nsm.cpp: bcp: failed to change directory to " << quote(siteRootDir) << " from " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

            pushCmnd = "git push origin " + siteRootDirBranch;

            ret_val = system("git add -A .");
            if(ret_val)
            {
                std::cout << "error: nsm.cpp: bcp: system('git add -A .') failed in " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

            ret_val = system(commitCmnd.c_str());
            //can't handle error here incase we had to run bcp multiple times

            ret_val = system(pushCmnd.c_str());
            if(ret_val)
            {
                std::cout << "error: nsm.cpp: bcp: system('" << pushCmnd << "') failed in " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

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
            if(noParams != 2 && noParams != 3)
                return parError(noParams, argv, "2 or 3");

            if(noParams == 2)
            {
                Path newTemplatePath;
                newTemplatePath.set_file_path_from(argv[2]);

                return site.new_template(newTemplatePath);
            }
            else
            {
                Name pageName = argv[2];
                Path newTemplatePath;
                newTemplatePath.set_file_path_from(argv[3]);

                int return_val =  site.new_template(pageName, newTemplatePath);

                if(!return_val)
                    std::cout << "successfully changed template path to " << newTemplatePath.str() << std::endl;

                return return_val;
            }
        }
        else if(cmd == "new-site-dir")
        {
            //ensures correct number of parameters given
            if(noParams != 2)
                return parError(noParams, argv, "2");

            Directory newSiteDir = argv[2];

            if(newSiteDir != "" && newSiteDir[newSiteDir.size()-1] != '/' && newSiteDir[newSiteDir.size()-1] != '\\')
                newSiteDir += "/";

            return site.new_site_dir(newSiteDir);
        }
        else if(cmd == "new-cont-dir")
        {
            //ensures correct number of parameters given
            if(noParams != 2)
                return parError(noParams, argv, "2");

            Directory newContDir = argv[2];

            if(newContDir != "" && newContDir[newContDir.size()-1] != '/' && newContDir[newContDir.size()-1] != '\\')
                newContDir += "/";

            return site.new_content_dir(newContDir);
        }
        else if(cmd == "new-cont-ext")
        {
            //ensures correct number of parameters given
            if(noParams != 2 && noParams != 3)
                return parError(noParams, argv, "2 or 3");

            if(noParams == 2)
                return site.new_content_ext(argv[2]);
            else
            {
                Name pageName = argv[2];

                int return_val = site.new_content_ext(pageName, argv[3]);

                if(!return_val) //informs user that page extension was successfully changed
                    std::cout << "successfully changed page extention for " << pageName << " to " << argv[3] << std::endl;

                return return_val;
            }
        }
        else if(cmd == "new-page-ext")
        {
            //ensures correct number of parameters given
            if(noParams != 2 && noParams != 3)
                return parError(noParams, argv, "2 or 3");

            if(noParams == 2)
                return site.new_page_ext(argv[2]);
            else
            {
                Name pageName = argv[2];

                int return_val = site.new_page_ext(pageName, argv[3]);

                if(!return_val) //informs user that page extension was successfully changed
                    std::cout << "successfully changed page extention for " << pageName << " to " << argv[3] << std::endl;

                return return_val;
            }
        }
        else if(cmd == "build-updated")
        {
            //ensures correct number of parameters given
            if(noParams > 1)
                return parError(noParams, argv, "1");

            //checks for pre-build scripts
            if(run_scripts(std::cout, "pre-build.scripts"))
                return 1;

            //checks for pre-build-updated scripts
            if(run_scripts(std::cout, "pre-build-updated.scripts"))
                return 1;

            int result = site.build_updated(std::cout);

            //checks for post-build scripts
            if(run_scripts(std::cout, "post-build.scripts"))
                return 1;

            //checks for post-build-updated scripts
            if(run_scripts(std::cout, "post-build-updated.scripts"))
                return 1;

            std::cout.precision(4);
            std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

            return result;
        }
        else if(cmd == "build")
        {
            //ensures correct number of parameters given
            if(noParams <= 1)
                return parError(noParams, argv, ">1");

            //checks for pre-build scripts
            if(run_scripts(std::cout, "pre-build.scripts"))
                return 1;

            std::vector<Name> pageNamesToBuild;
            for(int p=2; p<argc; p++)
            {
                pageNamesToBuild.push_back(argv[p]);
            }

            int result = site.build(pageNamesToBuild);

            //checks for post-build scripts
            if(run_scripts(std::cout, "post-build.scripts"))
                return 1;

            std::cout.precision(4);
            std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

            return result;
        }
        else if(cmd == "build-all")
        {
            //ensures correct number of parameters given
            if(noParams != 1)
                return parError(noParams, argv, "1");

            //checks for pre-build scripts
            if(run_scripts(std::cout, "pre-build.scripts"))
                return 1;

            //checks for pre-build-all scripts
            if(run_scripts(std::cout, "pre-build-all.scripts"))
                return 1;

            int result = site.build_all();

            //checks for post-build scripts
            if(run_scripts(std::cout, "post-build.scripts"))
                return 1;

            //checks for post-build-all scripts
            if(run_scripts(std::cout, "post-build-all.scripts"))
                return 1;

            std::cout.precision(4);
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

            //checks for pre-serve scripts
            if(run_scripts(std::cout, "pre-serve.scripts"))
                return 1;

            serving = 1;

            std::thread serve_thread(serve);
            if(noParams == 1)
            {
                std::thread read_serve_commands_thread(read_serve_commands);
                read_serve_commands_thread.join();
            }
            serve_thread.join();

            //checks for post-serve scripts
            if(run_scripts(std::cout, "post-serve.scripts"))
                return 1;
        }
        else
        {
            unrecognisedCommand("nsm", cmd);
            return 1;
        }
    }

    return 0;
}
