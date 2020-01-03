/*
    Nift (aka nsm) is a cross-platform open source
    command-line project and website manager.

    Official Website: https://nift.cc
    GitHub: https://github.com/nifty-site-manager/nsm

    Copyright (c) 2015-present
    Creator: Nicholas Ham
    https://n-ham.com
*/

#include "GitInfo.h"
#include "ProjectInfo.h"
#include "Timer.h"

bool upgradeProject()
{
    std::cout << "warning: attempting to upgrade project for newer version of Nift" << std::endl;

    if(rename(".siteinfo/", ".nsm/"))
    {
        std::cout << "error: upgrade: failed to move config directory '.siteinfo/' to '.nsm/'" << std::endl;
        return 1;
    }

    if(rename(".nsm/nsm.config", ".nsm/nift.config"))
    {
        std::cout << "error: upgrade: failed to move config file '.nsm/nsm.config' to '.nsm/nift.config'" << std::endl;
        return 1;
    }

    if(rename(".nsm/pages.list", ".nsm/tracking.list"))
    {
        std::cout << "error: upgrade: failed to move pages list file '.nsm/pages.list' to tracking list file '.nsm/tracking.list'" << std::endl;
        return 1;
    }

    ProjectInfo project;
    if(project.open_config())
    {
        std::cout << "error: upgrade: failed, was unable to open configuration file" << std::endl;
        return 1;
    }
    else if(project.open_tracking()) //this needs to change
    {
        std::cout << "error: upgrade: failed, was unable to open tracking list" << std::endl;
        return 1;
    }

    //need to update .nsm/nift.config
    //project.save_config();

    //need to update output extension files
    Path oldExtPath, newExtPath;
    for(auto tInfo=project.trackedAll.begin(); tInfo!=project.trackedAll.end(); tInfo++)
    {
        oldExtPath = newExtPath = tInfo->outputPath.getInfoPath();
        oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".pageExt";
        newExtPath.file = newExtPath.file.substr(0, newExtPath.file.find_first_of('.')) + ".outputExt";

        if(std::ifstream(oldExtPath.str()))
        {
            if(rename(oldExtPath.str().c_str(), newExtPath.str().c_str()))
            {
                std::cout << "error: upgrade: failed to move output extension file " << quote(oldExtPath.str()) << " to new extension file " <<  quote(newExtPath.str()) << std::endl;
                return 1;
            }
        }
    }

    std::cout << "Successfully upgraded project configuration for newer version of Nift" << std::endl;
    std::cout << "Note: the syntax to input page and project/site information has also changed, please check the content files page of the docs" << std::endl;

    return 0;
}

std::atomic<bool> serving;
std::mutex os_mtx2;

int read_serve_commands()
{
    std::string cmd;

    std::cout << "serving project locally - 'exit', 'stop' or 'ctrl c' to stop Nift serving" << std::endl;

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

int sleepTime = 500000; //default value: 0.5 seconds

bool isNum(const std::string& str)
{
    if(str.size() && str[0] != '-' && !std::isdigit(str[0]))
        return 0;

    for(size_t i=1; i<str.size(); i++)
        if(!std::isdigit(str[i]))
            return 0;

    return 1;
}

int serve()
{
    std::ofstream ofs;

    while(serving)
    {
        ProjectInfo project;
        if(project.open())
        {
            std::cout << "error: nsm.cpp: serve(): failed to open project" << std::endl;
            std::cout << "Nift is no longer serving!" << std::endl;
            return 1;
        }

        ofs.open(".serve-build-log.txt");

        if(!run_script(ofs, "pre-build" + project.scriptExt, project.backupScripts, &os_mtx2))
            if(!project.build_updated(ofs))
                run_script(ofs, "post-build" + project.scriptExt, project.backupScripts, &os_mtx2);

        ofs.close();

        usleep(sleepTime);
    }

    remove_path(Path("./", ".serve-build-log.txt"));

    return 0;
}

void unrecognisedCommand(const std::string& from, const std::string& cmd)
{
    std::cout << "error: " << from << " does not recognise the command '" << cmd << "'" << std::endl;
}

bool parError(int noParams, char* argv[], const std::string& expectedNo)
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

    //Nift commands that can run from anywhere
    if(cmd == "version" || cmd == "-version" || cmd == "--version")
    {
        std::cout << "Nift (aka nsm) v2.0" << std::endl;

        return 0;
    }
    else if(cmd == "commands")
    {
        std::cout << "+--------- available commands ------------------------------------------+" << std::endl;
        std::cout << "| commands          | lists all Nift commands                           |" << std::endl;
        std::cout << "| config            | list or set git email/username                    |" << std::endl;
        std::cout << "| clone             | input: clone-url                                  |" << std::endl;
        std::cout << "| diff              | input: file-path                                  |" << std::endl;
        std::cout << "| pull              | pull remote changes locally                       |" << std::endl;
        std::cout << "| bcp               | input: commit-message                             |" << std::endl;
        std::cout << "| init              | start managing a project - input: output-ext      |" << std::endl;
        std::cout << "| init-html         | start managing html website - input: (site-name)  |" << std::endl;
        std::cout << "| status            | lists updated and problem files                   |" << std::endl;
        std::cout << "| info              | input: name-1 .. name-k                           |" << std::endl;
        std::cout << "| info-all          | lists watched directories and tracked files       |" << std::endl;
        std::cout << "| info-config       | lists config settings                             |" << std::endl;
        std::cout << "| info-names        | lists tracked names                               |" << std::endl;
        std::cout << "| info-tracking     | lists tracked files                               |" << std::endl;
        std::cout << "| info-watching     | lists watched directories                         |" << std::endl;
        std::cout << "| track             | input: name (title) (template-path)               |" << std::endl;
        std::cout << "| track-from-file   | input: file-path                                  |" << std::endl;
        std::cout << "| track-dir         | input: dir-path (cont-ext) (temp-path) (out-ext)  |" << std::endl;
        std::cout << "| untrack           | input: name                                       |" << std::endl;
        std::cout << "| untrack-from-file | input: file-path                                  |" << std::endl;
        std::cout << "| untrack-dir       | input: dir-path (content-ext)                     |" << std::endl;
        std::cout << "| rm or del         | input: name                                       |" << std::endl;
        std::cout << "| rm-from-file      | input: file-path                                  |" << std::endl;
        std::cout << "| rm-dir            | input: dir-path (content-ext)                     |" << std::endl;
        std::cout << "| mv or move        | input: old-name new-name                          |" << std::endl;
        std::cout << "| cp or copy        | input: tracked-name new-name                      |" << std::endl;
        std::cout << "| build-names       | input: name-1 .. name-k                           |" << std::endl;
        std::cout << "| build-updated     | builds updated output files                       |" << std::endl;
        std::cout << "| build-all         | builds all tracked output files                   |" << std::endl;
        std::cout << "| serve             | serves project locally                            |" << std::endl;
        std::cout << "| new-title         | input: name new-title                             |" << std::endl;
        std::cout << "| new-template      | input: (name) template-path                       |" << std::endl;
        std::cout << "| new-output-dir    | input: dir-path                                   |" << std::endl;
        std::cout << "| new-cont-dir      | input: dir-path                                   |" << std::endl;
        std::cout << "| new-cont-ext      | input: (name) content-extension                   |" << std::endl;
        std::cout << "| new-output-ext    | input: (name) output-extension                    |" << std::endl;
        std::cout << "| new-script-ext    | input: (name) script-extension                    |" << std::endl;
        std::cout << "| no-build-thrds    | input: (no-threads) [-n == n*cores]               |" << std::endl;
        std::cout << "| backup-scripts    | input: (option)                                   |" << std::endl;
        std::cout << "| watch             | input: dir-path (cont-ext) (temp-path) (out-ext)  |" << std::endl;
        std::cout << "| unwatch           | input: dir-path (cont-ext)                        |" << std::endl;
        std::cout << "+-----------------------------------------------------------------------+" << std::endl;

        return 0;
    }
    else if(cmd == "help" || cmd == "-help" || cmd == "--help")
    {
        std::cout << "Nift (aka nifty-site-manager or nsm) is a cross-platform open source project and website manager." << std::endl;
        std::cout << "Official Website: https://nift.cc/" << std::endl;
        std::cout << "GitHub: https://github.com/nifty-site-manager/nsm" << std::endl;
        std::cout << "enter `nift commands` or `nsm commands` for available commands" << std::endl;

        return 0;
    }

    if(cmd == "init")
    {
        //ensures correct number of parameters given
        if(noParams != 2)
            return parError(noParams, argv, "2");

        //ensures Nift is not managing a project from this directory or one of the ancestor directories
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

        if(std::ifstream(".nsm/nift.config") || std::ifstream(".siteinfo/nsm.config"))
        {
            std::cout << "Nift is already managing a project in " << owd << " (or an accessible ancestor directory)" << std::endl;
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

            if(std::ifstream(".nsm/nift.config") || std::ifstream(".siteinfo/nsm.config"))
            {
                std::cout << "Nift is already managing a project in " << owd << " (or an accessible ancestor directory)" << std::endl;
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
        Path trackingListPath(".nsm/", "tracking.list");
        //ensures tracking list file exists
        trackingListPath.ensureFileExists();

        #if defined _WIN32 || defined _WIN64
            ret_val = system("attrib +h .nsm");

            //if handling error here need to check what happens in all of cmd prompt/power shell/git bash/cygwin/etc.
        #endif

        if(std::string(argv[2]) == "" || argv[2][0] != '.')
        {
            std::cout << "error: project output extension should start with a fullstop" << std::endl;
            return 1;
        }

        std::ofstream ofs(".nsm/nift.config");
        ofs << "contentDir 'content/'" << std::endl;
        ofs << "contentExt '.content'" << std::endl;
        ofs << "outputDir 'output/'" << std::endl;
        ofs << "outputExt '" << argv[2] << "'" << std::endl;
        ofs << "scriptExt '.py'" << std::endl;
        ofs << "defaultTemplate 'template/project.template'" << std::endl << std::endl;
        ofs << "backupScripts 1" << std::endl << std::endl;
        ofs << "buildThreads -1" << std::endl << std::endl;
        ofs << "unixTextEditor nano" << std::endl;
        ofs << "winTextEditor notepad" << std::endl << std::endl;
        if(std::ifstream(".git/"))
        {
            ofs << "rootBranch '" << get_pb() << "'" << std::endl;
            ofs << "outputBranch '" << get_pb() << "'" << std::endl;
        }
        else
        {
            ofs << "rootBranch '##unset##'" << std::endl;
            ofs << "outputBranch '##unset##'" << std::endl;
        }
        ofs.close();

        trackingListPath = Path("template/", "project.template");
        //ensures tracking list file exists
        trackingListPath.ensureDirExists();

        ofs.open("template/project.template");
        ofs << "@inputcontent" << std::endl;
        ofs.close();

        std::cout << "Nift: initialised empty project in " << get_pwd() << std::endl;

        return 0;
    }
    else if(cmd == "init-html")
    {
        //ensures correct number of parameters given
        if(noParams > 2)
            return parError(noParams, argv, "1-2");

        //ensures Nift is not managing a project from this directory or one of the ancestor directories
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

        if(std::ifstream(".nsm/nift.config") || std::ifstream(".siteinfo/nsm.config"))
        {
            std::cout << "Nift is already managing a project in " << owd << " (or an accessible ancestor directory)" << std::endl;
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

            if(std::ifstream(".nsm/nift.config") || std::ifstream(".siteinfo/nsm.config"))
            {
                std::cout << "Nift is already managing a project in " << owd << " (or an accessible ancestor directory)" << std::endl;
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
        Path trackingListPath(".nsm/", "tracking.list");
        //ensures tracking list file exists
        trackingListPath.ensureFileExists();

        #if defined _WIN32 || defined _WIN64
            ret_val = system("attrib +h .nsm");

            //if handling error here need to check what happens in all of cmd prompt/power shell/git bash/cygwin/etc.
        #endif

        std::ofstream ofs(".nsm/nift.config");
        ofs << "contentDir 'content/'" << std::endl;
        ofs << "contentExt '.content'" << std::endl;
        ofs << "outputDir 'site/'" << std::endl;
        ofs << "outputExt '.html'" << std::endl;
        ofs << "scriptExt '.py'" << std::endl;
        ofs << "defaultTemplate 'template/page.template'" << std::endl << std::endl;
        ofs << "backupScripts 1" << std::endl << std::endl;
        ofs << "buildThreads -1" << std::endl << std::endl;
        ofs << "unixTextEditor nano" << std::endl;
        ofs << "winTextEditor notepad" << std::endl << std::endl;
        if(std::ifstream(".git/"))
        {
            ofs << "rootBranch '" << get_pb() << "'" << std::endl;
            ofs << "outputBranch '" << get_pb() << "'" << std::endl;
        }
        else
        {
            ofs << "rootBranch '##unset##'" << std::endl;
            ofs << "outputBranch '##unset##'" << std::endl;
        }
        ofs.close();

        trackingListPath = Path("template/", "page.template");
        //ensures tracking list file exists
        trackingListPath.ensureDirExists();

        ofs.open("template/page.template");
        ofs << "<html>" << std::endl;
        ofs << "    <head>" << std::endl;
        ofs << "        @input{!p}(template/head.content)" << std::endl;
        ofs << "    </head>" << std::endl;
        ofs << std::endl;
        ofs << "    <body>" << std::endl;
        ofs << "        @content{!p}()" << std::endl;
        ofs << "    </body>" << std::endl;
        ofs << "</html>" << std::endl;
        ofs << std::endl;
        ofs.close();

        ofs.open("template/head.content");
        if(noParams == 1)
            ofs << "<title>empty site - @[title]</title>" << std::endl;
        else if(noParams > 1)
            ofs << "<title>" << argv[2] << " - @[title]</title>" << std::endl;
        ofs.close();

        ProjectInfo project;
        if(project.open() > 0)
            return 1;

        Name name = "index";
        Title title;
        title = get_title(name);
        project.track(name, title, project.defaultTemplate);
        project.build_all();

        std::cout << "Nift: initialised empty html project in " << get_pwd() << std::endl;

        return 0;
    }
    else if(cmd == "config")
    {
        //ensures correct number of parameters given
        if(noParams != 3 && noParams != 4)
        {
            std::cout << "you might have meant one of:" << std::endl;
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
                        std::cout << "error: nsm.cpp: config: system(" << quote(cmdStr) << ") failed in " << quote(get_pwd()) << std::endl;
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
                        std::cout << "error: nsm.cpp: config: system(" << quote(cmdStr) << ") failed in " << quote(get_pwd()) << std::endl;
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
            std::cout << "error: nsm.cpp: clone: system(" << quote(cloneCmnd) << ") failed in " << quote(get_pwd()) << std::endl;
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

        if(obranch == "##error##")
        {
            std::cout << "error: nsm.cpp: pull: get_pb() failed in repository root directory " << quote(get_pwd()) << std::endl;
            return 1;
        }

        if(obranch == "##not-found##")
        {
            std::cout << "error: nsm.cpp: clone: no branch found in repository root directory " << quote(get_pwd()) << std::endl;
            return 1;
        }

        if(branches.size() == 0) //don't we already have an error just above?
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
                remove_path(Path("./", "nul"));
            if(ret_val)
            {
                std::cout << "error: nsm.cpp: clone: system(" << quote(checkoutCmnd) << ") failed in " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

            if(std::ifstream(".nsm/nift.config") || std::ifstream(".siteinfo/nsm.config")) //found root branch
            {
                if(std::ifstream(".siteinfo/nsm.config")) //can delete this later (and half of above)
                    if(upgradeProject())
                        return 1;

                ProjectInfo project;
                if(project.open())
                    return 1;

                if(!branches.count(project.rootBranch))
                {
                    std::cout << "error: nsm.cpp: clone: rootBranch " << quote(project.rootBranch) << " is not present in the git repository" << std::endl;
                    return 1;
                }
                else if(!branches.count(project.outputBranch))
                {
                    std::cout << "error: nsm.cpp: clone: outputBranch " << quote(project.outputBranch) << " is not present in the git repository" << std::endl;
                    return 1;
                }

                if(project.outputBranch != project.rootBranch)
                {
                    ret_val = chdir(parDir.c_str());
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    ret_val = cpDir(dirName, ".temp-output-dir");
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: failed to copy directory " << quote(dirName) << " to '.temp-output-dir/' from " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    ret_val = chdir(".temp-output-dir");
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: failed to change directory to " << quote(".temp-output-dir") << " from " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    ret_val = delDir(".nsm/"); //can delete this later
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: failed to delete directory " << get_pwd() << "/.nsm/" << std::endl;
                        return 1;
                    }

                    ret_val = system("git checkout -- . > /dev/null 2>&1 >nul 2>&1");
                    if(std::ifstream("./nul"))
                        remove_path(Path("./", "nul"));
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: system('git checkout -- .') failed in " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    checkoutCmnd = "git checkout " + project.outputBranch + " > /dev/null 2>&1 >nul 2>&1";
                    ret_val = system(checkoutCmnd.c_str());
                    if(std::ifstream("./nul"))
                        remove_path(Path("./", "nul"));
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: system(" << quote(checkoutCmnd) << ") failed in " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    ret_val = chdir((parDir + dirName).c_str());
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: failed to change directory to " << quote(parDir + dirName) << " from " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    ret_val = delDir(project.outputDir);
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: failed to delete directory " << quote(project.outputDir) << " from " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    ret_val = chdir(parDir.c_str());
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    //moves project dir/branch in two steps here in case project dir/branch isn't located inside stage/root dir/branch
                    rename(".temp-output-dir", (dirName + "/.temp-output-dir").c_str());

                    ret_val = chdir(dirName.c_str());
                    if(ret_val)
                    {
                        std::cout << "error: nsm.cpp: clone: failed to change directory to " << quote(parDir + dirName) << " from " << quote(get_pwd()) << std::endl;
                        return ret_val;
                    }

                    rename(".temp-output-dir", project.outputDir.c_str());

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
            remove_path(Path("./", "nul"));
        if(ret_val)
        {
            std::cout << "error: nsm.cpp: clone: system(" << quote(checkoutCmnd) << ") failed in " << quote(get_pwd()) << std::endl;
            return ret_val;
        }

        return 0;
    }
    else
    {
        //checks that we have a valid command
        if(cmd != "bcp" &&
           cmd != "diff" &&
           cmd != "pull" &&
           cmd != "status" &&
           cmd != "info" &&
           cmd != "info-all" &&
           cmd != "info-config" &&
           cmd != "info-names" &&
           cmd != "info-tracking" &&
           cmd != "info-watching" &&
           cmd != "track" &&
           cmd != "track-from-file" &&
           cmd != "track-dir" &&
           cmd != "untrack" &&
           cmd != "untrack-from-file" &&
           cmd != "untrack-dir" &&
           cmd != "rm-from-file" && cmd != "del-from-file" &&
           cmd != "rm-dir"       && cmd != "del-dir" &&
           cmd != "rm"           && cmd != "del" &&
           cmd != "mv" &&
           cmd != "move" &&
           cmd != "cp" &&
           cmd != "copy" &&
           cmd != "new-title" &&
           cmd != "new-template" &&
           cmd != "new-output-dir" &&
           cmd != "new-cont-dir" &&
           cmd != "new-cont-ext" &&
           cmd != "new-output-ext" &&
           cmd != "new-script-ext" &&
           cmd != "no-build-thrds" &&
           cmd != "backup-scripts" &&
           cmd != "watch" &&
           cmd != "unwatch" &&
           cmd != "build-names" &&
           cmd != "build-updated" &&
           cmd != "build-all" &&
           cmd != "serve")
        {
            unrecognisedCommand("nsm", cmd);
            return 1;
        }

        //ensures Nift is managing a project from this directory or one of the parent directories
        std::string parentDir = "../",
            rootDir = "/",
            projectRootDir = get_pwd(),
            owd = get_pwd(),
            pwd = get_pwd(),
            prevPwd;

        while(!std::ifstream(".nsm/nift.config") && !std::ifstream(".siteinfo/nsm.config"))
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
                std::cout << "Nift is not managing a project from " << owd << " (or any accessible parent directories)" << std::endl;
                return 1;
            }
        }

        if(std::ifstream(".siteinfo/nsm.config"))
        {
            if(upgradeProject())
                return 1;
            std::cout << std::endl;
        }

        //ensures both tracking.list and nift.config exist
        if(!std::ifstream(".nsm/tracking.list"))
        {
            std::cout << "error: '" << get_pwd() << "/.nsm/tracking.list' is missing" << std::endl;
            return 1;
        }

        if(!std::ifstream(".nsm/nift.config"))
        {
            std::cout << "error: '" << get_pwd() << "/.nsm/nift.config' is missing" << std::endl;
            return 1;
        }

        //Nift commands that just need to know it is a project directory
        if(cmd == "diff") //diff of file
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

        ProjectInfo project;
        if(project.open_config())
            return 1;

        //Nift commands that need project information file open
        if(cmd == "info-config")
        {
            if(project.buildThreads < 0)
                project.buildThreads = -project.buildThreads*std::thread::hardware_concurrency();

            std::cout << "contentDir: " << quote(project.contentDir) << std::endl;
            std::cout << "contentExt: " << quote(project.contentExt) << std::endl;
            std::cout << "outputDir: " << quote(project.outputDir) << std::endl;
            std::cout << "outputExt: " << quote(project.outputExt) << std::endl;
            std::cout << "scriptExt: " << quote(project.scriptExt) << std::endl;
            std::cout << "defaultTemplate: " << project.defaultTemplate << std::endl << std::endl;
            std::cout << "buildThreads: " << project.buildThreads << std::endl << std::endl;
            std::cout << "backupScripts: " << project.backupScripts << std::endl << std::endl;
            std::cout << "unixTextEditor: " << quote(project.unixTextEditor) << std::endl;
            std::cout << "winTextEditor: " << quote(project.winTextEditor) << std::endl << std::endl;
            std::cout << "rootBranch: " << quote(project.rootBranch) << std::endl;
            std::cout << "outputBranch: " << quote(project.outputBranch) << std::endl;

            return 0;
        }
        else if(cmd =="pull")
        {
            //ensures correct number of parameters given
            if(noParams != 1)
                return parError(noParams, argv, "1");

            //checks that git is configured
            if(!is_git_configured())
            {
                std::cout << "error: nsm.cpp: pull: is git configured?" << std::endl;
                return 1;
            }

            //checks that remote git url is set
            if(!is_git_remote_set())
            {
                std::cout << "error: nsm.cpp: pull: is the git remote url set?" << std::endl;
                return 1;
            }

            std::string pullCmnd,
                        projectDirRemote,
                        projectRootDirRemote = get_remote(),
                        projectDirBranch,
                        projectRootDirBranch = get_pb();

            if(projectRootDirRemote == "##error##")
            {
                std::cout << "error: nsm.cpp: pull: get_remote() failed in project root directory " << quote(get_pwd()) << std::endl;
                return 1;
            }

            if(projectRootDirRemote == "##not-found##")
            {
                std::cout << "error: nsm.cpp: pull: get_remote() did not find any git remote in project root directory " << quote(get_pwd()) << std::endl;
            }

            if(projectRootDirBranch == "##error##")
            {
                std::cout << "error: nsm.cpp: pull: get_pb() failed in project root directory " << quote(get_pwd()) << std::endl;
                return 1;
            }

            if(projectRootDirBranch == "##not-found##")
            {
                std::cout << "error: nsm.cpp: pull: no branch found in project root directory " << quote(get_pwd()) << std::endl;
                return 1;
            }

            if(projectRootDirBranch != project.rootBranch)
            {
                std::cout << "error: nsm.cpp: pull: project root dir branch " << quote(projectRootDirBranch) << " is not the same as rootBranch " << quote(project.rootBranch) << " in .nsm/nift.config" << std::endl;
                return 1;
            }

            ret_val = chdir(project.outputDir.c_str());
            if(ret_val)
            {
                std::cout << "error: nsm.cpp: pull: failed to change directory to " << quote(project.outputDir) << " from " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

            projectDirRemote = get_remote();

            if(projectDirRemote == "##error##")
            {
                std::cout << "error: nsm.cpp: pull: get_remote() failed in project directory " << quote(get_pwd()) << std::endl;
                return 1;
            }

            projectDirBranch = get_pb();

            if(projectDirBranch == "##error##")
            {
                std::cout << "error: nsm.cpp: pull: get_pb() failed in project directory " << quote(get_pwd()) << std::endl;
                return 1;
            }

            if(projectDirBranch == "##not-found##")
            {
                std::cout << "error: nsm.cpp: pull: no branch found in project directory " << quote(get_pwd()) << std::endl;
                return 1;
            }

            if(projectDirBranch != project.outputBranch)
            {
                std::cout << "error: nsm.cpp: pull: output dir branch " << quote(projectDirBranch) << " is not the same as outputBranch " << quote(project.outputBranch) << " in .nsm/nift.config" << std::endl;
                return 1;
            }

            if(projectDirBranch != projectRootDirBranch)
            {
                pullCmnd = "git pull " + projectDirRemote + " " + projectDirBranch;

                ret_val = system(pullCmnd.c_str());
                if(ret_val)
                {
                    std::cout << "error: nsm.cpp: pull: system(" << quote(pullCmnd) << ") failed in project directory " << quote(get_pwd()) << std::endl;
                    return ret_val;
                }
            }

            ret_val = chdir(projectRootDir.c_str());
            if(ret_val)
            {
                std::cout << "error: nsm.cpp: pull: failed to change directory to " << quote(projectRootDir) << " from " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

            pullCmnd = "git pull " + projectRootDirRemote + " " + projectRootDirBranch;

            ret_val = system(pullCmnd.c_str());
            if(ret_val)
            {
                std::cout << "error: nsm.cpp: pull: system(" << quote(pullCmnd) << ") failed in project root directory " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

            return 0;
        }
        if(cmd == "no-build-thrds")
        {
            //ensures correct number of parameters given
            if(noParams != 1 && noParams != 2)
                return parError(noParams, argv, "1 or 2");

            if(noParams == 1)
                std::cout << project.buildThreads << std::endl;
            else
            {
                if(!isNum(std::string(argv[2])))
                {
                    std::cout << "error: number of build threads should be a non-zero integer (use negative numbers for a multiple of the number of cores on the machine)" << std::endl;
                    return 1;
                }
                return project.no_build_threads(std::stoi(std::string(argv[2])));
            }

            return 0;
        }
        if(cmd == "backup-scripts")
        {
            //ensures correct number of parameters given
            if(noParams != 1 && noParams != 2)
                return parError(noParams, argv, "1 or 2");

            if(noParams == 1)
                std::cout << project.backupScripts << std::endl;
            else
            {
                if(std::string(argv[2]) == "")
                {
                    std::cout << "error: do not recognise the option " << argv[2] << std::endl;
                    return 1;
                }
                if(std::string(argv[2])[0] == 'y' || std::string(argv[2]) == "1")
                {
                    if(project.backupScripts)
                    {
                        std::cout << "error: project is already set to backup scripts" << std::endl;
                        return 1;
                    }
                    project.backupScripts = 1;
                    std::cout << "scripts will now be backed up when moved to run" << std::endl;
                }
                else if(std::string(argv[2])[0] == 'n' || std::string(argv[2]) == "0")
                {
                    if(!project.backupScripts)
                    {
                        std::cout << "error: project is already set to not backup scripts" << std::endl;
                        return 1;
                    }
                    project.backupScripts = 0;
                    std::cout << "scripts will no longer be backed up when moved to run" << std::endl;
                }
                else
                {
                    std::cout << "error: do not recognise the option " << argv[2] << std::endl;
                    return 1;
                }

                project.save_config();

                return 0;
            }

            return 0;
        }
        else if(cmd == "watch")
        {
            //ensures correct number of parameters given
            if(noParams < 2 && noParams > 5)
                return parError(noParams, argv, "2-5");

            WatchList wl;
            if(wl.open())
            {
                std::cout << "error: watch: failed to open watch list '.nsm/.watchinfo/watching.list'" << std::endl;
                return 1;
            }

            WatchDir wd;
            wd.watchDir = comparable(argv[2]);
            if(!std::ifstream(std::string(argv[2])))
            {
                std::cout << "error: cannot watch directory " << quote(wd.watchDir) << " as it does not exist" << std::endl;
                return 1;
            }
            else if(wd.watchDir.substr(0, project.contentDir.size()) != project.contentDir)
            {
                std::cout << "error: cannot watch directory " << quote(wd.watchDir) << " as it is not a subdirectory of the project-wide content directory " << quote(project.contentDir) << std::endl;
                return 1;
            }
            std::string contExt, outputExt;
            Path templatePath;
            if(wd.watchDir[wd.watchDir.size()-1] != '/' && wd.watchDir[wd.watchDir.size()-1] != '\\')
            {
                std::cout << "error: watch directory " << quote(wd.watchDir) << " must end with '/' or '\\'" << std::endl;
                return 1;
            }
            if(noParams >= 3)
            {
                contExt = argv[3];
                if(contExt[0] != '.')
                {
                    std::cout << "error: content extension " << quote(contExt) << " must start with a '.'" << std::endl;
                    return 1;
                }
            }
            else
                contExt = project.contentExt;
            if(noParams >= 4)
            {
                templatePath.set_file_path_from(argv[4]);

                if(!std::ifstream(templatePath.str()))
                {
                    std::cout << "error: template path " << templatePath << " does not exist" << std::endl;
                    return 1;
                }
            }
            else
                templatePath = project.defaultTemplate;
            if(noParams == 5)
            {
                outputExt = argv[5];
                if(outputExt[0] != '.')
                {
                    std::cout << "error: output extension " << quote(outputExt) << " must start with a '.'" << std::endl;
                    return 1;
                }
            }
            else
                outputExt = project.outputExt;

            auto found = wl.dirs.find(wd);

            if(found != wl.dirs.end())
            {
                if(found->contExts.count(contExt))
                {
                    std::cout << "error: already watching directory " << quote(wd.watchDir) << " with content extension " << quote(contExt) << std::endl;
                    return 1;
                }
                else
                {
                    wd.contExts = found->contExts;
                    wd.templatePaths = found->templatePaths;
                    wd.outputExts = found->outputExts;

                    wd.contExts.insert(contExt);
                    wd.templatePaths[contExt] = templatePath;
                    wd.outputExts[contExt] = outputExt;

                    wl.dirs.erase(found);

                    wl.dirs.insert(wd);
                    wl.save();

                    std::cout << "successfully watching directory " << quote(wd.watchDir) << " with:" << std::endl;
                    std::cout << "  content extension: " << quote(contExt) << std::endl;
                    std::cout << "      template path: " << templatePath << std::endl;
                    std::cout << "   output extension: " << quote(outputExt) << std::endl;
                }
            }
            else
            {
                wd.contExts.insert(contExt);
                wd.templatePaths[contExt] = templatePath;
                wd.outputExts[contExt] = outputExt;
                wl.dirs.insert(wd);
                wl.save();

                std::cout << "successfully watching directory " << quote(wd.watchDir) << " with:" << std::endl;
                std::cout << "  content extension: " << quote(contExt) << std::endl;
                std::cout << "      template path: " << templatePath << std::endl;
                std::cout << "   output extension: " << quote(outputExt) << std::endl;
            }

            return 0;
        }
        else if(cmd == "unwatch")
        {
            //ensures correct number of parameters given
            if(noParams < 2 || noParams > 3)
                return parError(noParams, argv, "2 or 3");

            if(std::ifstream(".nsm/.watchinfo/watching.list"))
            {
                std::string contExt;
                WatchList wl;
                if(wl.open())
                {
                    std::cout << "error: unwatch: failed to open watch list '.nsm/.watchinfo/watching.list'" << std::endl;
                    return 1;
                }

                WatchDir wd;
                wd.watchDir = comparable(argv[2]);

                if(noParams == 3)
                    contExt = argv[3];
                else
                    contExt = project.contentExt;

                auto found = wl.dirs.find(wd);

                if(found != wl.dirs.end())
                {
                    if(found->contExts.count(contExt))
                    {
                        if(found->contExts.size() == 1)
                        {
                            wl.dirs.erase(found);
                            remove_path(Path(".nsm/.watchinfo/", replace_slashes(wd.watchDir) + ".exts"));
                        }
                        else
                        {
                            wd.contExts = found->contExts;
                            wd.templatePaths = found->templatePaths;
                            wd.outputExts = found->outputExts;

                            wd.contExts.erase(contExt);

                            wl.dirs.erase(found);
                            wl.dirs.insert(wd);
                        }

                        wl.save();
                        std::cout << "successfully unwatched directory " << quote(argv[2]) << " with content extension " << quote(contExt) << std::endl;
                    }
                    else
                    {
                        std::cout << "error: not watching directory " << quote(argv[2]) << " with content extension " << quote(contExt) << std::endl;
                        return 1;
                    }
                }
                else
                {
                    std::cout << "error: not watching directory " << quote(argv[2]) << std::endl;
                    return 1;
                }
            }
            else
            {
                std::cout << "error: not watching directory " << quote(argv[2]) << std::endl;
                return 1;
            }

            return 0;
        }

        //opens up tracking.list file
        if(project.open_tracking())
            return 1;

        //Nift commands that need tracking list file open
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

            if(project.build_updated(std::cout))
                return 1;

            std::string commitCmnd = "git commit -m \"" + std::string(argv[2]) + "\"",
                        pushCmnd,
                        outputDirBranch,
                        projectRootDirBranch = get_pb();

            if(projectRootDirBranch == "##error##")
            {
                std::cout << "error: nsm.cpp: bcp: get_pb() failed in " << quote(get_pwd()) << std::endl;
                return 0;
            }

            if(projectRootDirBranch == "##not-found##")
            {
                std::cout << "error: nsm.cpp: bcp: no branch found in project root directory " << quote(get_pwd()) << std::endl;
                return 1;
            }

            if(projectRootDirBranch != project.rootBranch)
            {
                std::cout << "error: nsm.cpp: bcp: root dir branch " << quote(projectRootDirBranch) << " is not the same as rootBranch " << quote(project.rootBranch) << " in .nsm/nift.config" << std::endl;
                return 1;
            }

            std::cout << commitCmnd << std::endl;

            ret_val = chdir(project.outputDir.c_str());
            if(ret_val)
            {
                std::cout << "error: nsm.cpp: bcp: failed to change directory to " << quote(project.outputDir) << " from " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

            outputDirBranch = get_pb();

            if(outputDirBranch == "##error##")
            {
                std::cout << "error: nsm.cpp: bcp: get_pb() failed in " << quote(get_pwd()) << std::endl;
                return 0;
            }

            if(outputDirBranch == "##not-found##")
            {
                std::cout << "error: nsm.cpp: bcp: no branch found in project directory " << quote(get_pwd()) << std::endl;
                return 1;
            }

            if(outputDirBranch != project.outputBranch)
            {
                std::cout << "error: nsm.cpp: pull: output dir branch " << quote(outputDirBranch) << " is not the same as outputBranch " << quote(project.outputBranch) << " in .nsm/nift.config" << std::endl;
                return 1;
            }

            if(outputDirBranch != projectRootDirBranch)
            {
                pushCmnd = "git push origin " + outputDirBranch;

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
                    std::cout << "error: nsm.cpp: bcp: system(" << quote(pushCmnd) << ") failed in " << quote(get_pwd()) << std::endl;
                    return ret_val;
                }
            }

            ret_val = chdir(projectRootDir.c_str());
            if(ret_val)
            {
                std::cout << "error: nsm.cpp: bcp: failed to change directory to " << quote(projectRootDir) << " from " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

            pushCmnd = "git push origin " + projectRootDirBranch;

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
                std::cout << "error: nsm.cpp: bcp: system(" << quote(pushCmnd) << ") failed in " << quote(get_pwd()) << std::endl;
                return ret_val;
            }

            return 0;
        }
        else if(cmd == "status")
        {
            //ensures correct number of parameters given
            if(noParams != 1)
                return parError(noParams, argv, "1");

            return project.status();
        }
        else if(cmd == "info")
        {
            //ensures correct number of parameters given
            if(noParams <= 1)
                return parError(noParams, argv, ">1");

            std::vector<Name> names;
            for(int p=2; p<argc; p++)
                names.push_back(argv[p]);
            return project.info(names);
        }
        else if(cmd == "info-all")
        {
            //ensures correct number of parameters given
            if(noParams > 1)
                return parError(noParams, argv, "1");

            return project.info_all();
        }
        else if(cmd == "info-names")
        {
            //ensures correct number of parameters given
            if(noParams > 1)
                return parError(noParams, argv, "1");

            return project.info_names();
        }
        else if(cmd == "info-tracking")
        {
            //ensures correct number of parameters given
            if(noParams > 1)
                return parError(noParams, argv, "1");

            return project.info_tracking();
        }
        else if(cmd == "info-watching")
        {
            //ensures correct number of parameters given
            if(noParams > 1)
                return parError(noParams, argv, "1");

            return project.info_watching();
        }
        else if(cmd == "track")
        {
            //ensures correct number of parameters given
            if(noParams < 2 || noParams > 4)
                return parError(noParams, argv, "2-4");

            Name newName = quote(argv[2]); //surely this doesn't need to be quoted?
            Title newTitle;
            if(noParams >= 3)
                newTitle = quote(argv[3]); //surely this doesn't need to be quoted?
            else
                newTitle = get_title(newName);

            Path newTemplatePath;
            if(noParams == 4)
                newTemplatePath.set_file_path_from(argv[4]);
            else
                newTemplatePath = project.defaultTemplate;

            return project.track(newName, newTitle, newTemplatePath);
        }
        else if(cmd == "track-from-file")
        {
            //ensures correct number of parameters given
            if(noParams != 2)
                return parError(noParams, argv, "2");

            int result = project.track_from_file(argv[2]);

            std::cout.precision(4);
            std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

            return result;
        }
        else if(cmd == "track-dir")
        {
            //ensures correct number of parameters given
            if(noParams < 2 || noParams > 5)
                return parError(noParams, argv, "2-5");

            Path dirPath,
                 templatePath = project.defaultTemplate;
            std::string contExt = project.contentExt,
                        outputExt = project.outputExt;

            dirPath.set_file_path_from(argv[2]);

            if(noParams >= 3)
                contExt = argv[3];
            if(noParams >= 4)
                templatePath.set_file_path_from(argv[4]);
            if(noParams == 5)
                outputExt = argv[5];

            int result = project.track_dir(dirPath, contExt, templatePath, outputExt);

            std::cout.precision(4);
            std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

            return result;
        }
        else if(cmd == "untrack")
        {
            //ensures correct number of parameters given
            if(noParams != 2)
                return parError(noParams, argv, "2");

            Name nameToUntrack = argv[2];

            return project.untrack(nameToUntrack);
        }
        else if(cmd == "untrack-from-file")
        {
            //ensures correct number of parameters given
            if(noParams != 2)
                return parError(noParams, argv, "2");

            int result = project.untrack_from_file(argv[2]);

            std::cout.precision(4);
            std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

            return result;
        }
        else if(cmd == "untrack-dir")
        {
            //ensures correct number of parameters given
            if(noParams < 2 || noParams > 3)
                return parError(noParams, argv, "2-3");

            Path dirPath;
            std::string contExt = project.contentExt;

            dirPath.set_file_path_from(argv[2]);

            if(noParams == 3)
                contExt = argv[3];

            int result = project.untrack_dir(dirPath, contExt);

            std::cout.precision(4);
            std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

            return result;
        }
        else if(cmd == "rm-from-file" || cmd == "del-from-file")
        {
            //ensures correct number of parameters given
            if(noParams != 2)
                return parError(noParams, argv, "2");

            int result = project.rm_from_file(argv[2]);

            std::cout.precision(4);
            std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

            return result;
        }
        else if(cmd == "rm-dir" || cmd == "del-dir")
        {
            //ensures correct number of parameters given
            if(noParams < 2 || noParams > 3)
                return parError(noParams, argv, "2-3");

            Path dirPath;
            std::string contExt = project.contentExt;

            dirPath.set_file_path_from(argv[2]);

            if(noParams == 3)
                contExt = argv[3];

            int result = project.rm_dir(dirPath, contExt);

            std::cout.precision(4);
            std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

            return result;
        }
        else if(cmd == "rm" || cmd == "del")
        {
            //ensures correct number of parameters given
            if(noParams != 2)
                return parError(noParams, argv, "2");

            Name nameToRemove = argv[2];

            return project.rm(nameToRemove);
        }
        else if(cmd == "mv" || cmd == "move")
        {
            //ensures correct number of parameters given
            if(noParams != 3)
                return parError(noParams, argv, "3");

            Name oldName = argv[2],
                 newName = argv[3];

            return project.mv(oldName, newName);
        }
        else if(cmd == "cp" || cmd == "copy")
        {
            //ensures correct number of parameters given
            if(noParams != 3)
                return parError(noParams, argv, "3");

            Name trackedName = argv[2],
                 newName = argv[3];

            return project.cp(trackedName, newName);
        }
        else if(cmd == "new-title")
        {
            //ensures correct number of parameters given
            if(noParams != 3)
                return parError(noParams, argv, "3");

            Name name = argv[2];
            Title newTitle;
            newTitle.str = argv[3];

            return project.new_title(name, newTitle);
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

                return project.new_template(newTemplatePath);
            }
            else
            {
                Name name = argv[2];
                Path newTemplatePath;
                newTemplatePath.set_file_path_from(argv[3]);

                int return_val =  project.new_template(name, newTemplatePath);

                if(!return_val)
                    std::cout << "successfully changed template path to " << newTemplatePath.str() << std::endl;

                return return_val;
            }
        }
        else if(cmd == "new-output-dir")
        {
            //ensures correct number of parameters given
            if(noParams != 2)
                return parError(noParams, argv, "2");

            Directory newOutputDir = argv[2];

            if(newOutputDir != "" && newOutputDir[newOutputDir.size()-1] != '/' && newOutputDir[newOutputDir.size()-1] != '\\')
                newOutputDir += "/";

            return project.new_output_dir(newOutputDir);
        }
        else if(cmd == "new-cont-dir")
        {
            //ensures correct number of parameters given
            if(noParams != 2)
                return parError(noParams, argv, "2");

            Directory newContDir = argv[2];

            if(newContDir != "" && newContDir[newContDir.size()-1] != '/' && newContDir[newContDir.size()-1] != '\\')
                newContDir += "/";

            return project.new_content_dir(newContDir);
        }
        else if(cmd == "new-cont-ext")
        {
            //ensures correct number of parameters given
            if(noParams != 2 && noParams != 3)
                return parError(noParams, argv, "2 or 3");

            if(noParams == 2)
                return project.new_content_ext(argv[2]);
            else
            {
                Name name = argv[2];

                int return_val = project.new_content_ext(name, argv[3]);

                if(!return_val) //informs user that content extension was successfully changed
                    std::cout << "successfully changed content extention for " << name << " to " << argv[3] << std::endl;

                return return_val;
            }
        }
        else if(cmd == "new-output-ext")
        {
            //ensures correct number of parameters given
            if(noParams != 2 && noParams != 3)
                return parError(noParams, argv, "2 or 3");

            if(noParams == 2)
                return project.new_output_ext(argv[2]);
            else
            {
                Name name = argv[2];

                int return_val = project.new_output_ext(name, argv[3]);

                if(!return_val) //informs user that output extension was successfully changed
                    std::cout << "successfully changed output extention for " << name << " to " << argv[3] << std::endl;

                return return_val;
            }
        }
        else if(cmd == "new-script-ext")
        {
            //ensures correct number of parameters given
            if(noParams != 2 && noParams != 3)
                return parError(noParams, argv, "2 or 3");

            if(noParams == 2)
                return project.new_script_ext(argv[2]);
            else
            {
                Name name = argv[2];

                int return_val = project.new_script_ext(name, argv[3]);

                if(!return_val) //informs user that script extension was successfully changed
                    std::cout << "successfully changed script extention for " << quote(name) << " to " << quote(argv[3]) << std::endl;

                return return_val;
            }
        }
        else if(cmd == "build-updated")
        {
            //ensures correct number of parameters given
            if(noParams > 1)
                return parError(noParams, argv, "1");

            int result = project.build_updated(std::cout);

            std::cout.precision(4);
            std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

            return result;
        }
        else if(cmd == "build-names")
        {
            //ensures correct number of parameters given
            if(noParams <= 1)
                return parError(noParams, argv, ">1");

            std::vector<Name> namesToBuild;
            for(int p=2; p<argc; p++)
                namesToBuild.push_back(argv[p]);

            int result = project.build_names(namesToBuild);

            std::cout.precision(4);
            std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

            return result;
        }
        else if(cmd == "build-all")
        {
            //ensures correct number of parameters given
            if(noParams != 1)
                return parError(noParams, argv, "1");

            int result = project.build_all();

            std::cout.precision(4);
            std::cout << "time taken: " << timer.getTime() << " seconds" << std::endl;

            return result;
        }
        else if(cmd == "serve")
        {
            //ensures correct number of parameters given
            if(noParams > 2)
                return parError(noParams, argv, "1 or 2");

            if(noParams == 2 && isNum(std::string(argv[2])))
            {
                sleepTime = std::stoi(std::string(argv[2]))*1000;
                noParams = 1;
                std::cout << "sleep time " << sleepTime << std::endl;
            }

            if(noParams == 2 && std::string(argv[2]) != "-s")
            {
                std::cout << "error: Nift does not recognise command serve " << argv[2] << std::endl;
                return 1;
            }

            //checks for pre-serve scripts
            if(run_script(std::cout, "pre-serve" + project.scriptExt, project.backupScripts, &os_mtx2))
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
            if(run_script(std::cout, "post-serve" + project.scriptExt, project.backupScripts, &os_mtx2))
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
