/*
    nsm (nifty site manager) is a cross-platform open source
    git-like and LaTeX-like site manager.

    site: https://nift.cc

    Copyright (c) 2015-present
    https://n-ham.com
*/

#include <unistd.h>

#include "SiteInfo.h"

std::string get_pwd()
{
    char * pwd_char = getcwd(NULL, 0);
    std::string pwd = pwd_char;
    free(pwd_char);
    return pwd;
}

//get present git branch
std::string get_pb()
{
    std::string branch = "";

    system("git status > .f242tgg43.txt");
    std::ifstream ifs(".f242tgg43.txt");

    while(ifs >> branch)
        if(branch == "branch")
            break;
    if(branch == "branch")
        ifs >> branch;

    ifs.close();

    system("rm -r .f242tgg43.txt");

    return branch;
}

//get set of git branches
std::set<std::string> get_git_branches()
{
    std::set<std::string> branches;
    std::string branch = "";

    system("git branch > .f242tgg43.txt");
    std::ifstream ifs(".f242tgg43.txt");

    while(ifs >> branch)
        if(branch != "*")
            branches.insert(branch);

    ifs.close();

    system("rm -r .f242tgg43.txt");

    return branches;
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

    if(cmd == "commands")
    {
        std::cout << "+---------- available commands ----------------------------------------+" << std::endl;
        std::cout << "| nsm commands       | lists all nsm commands                          |" << std::endl;
        std::cout << "| nsm config         | lists config settings                           |" << std::endl;
        std::cout << "| nsm clone          | input: clone-url                                |" << std::endl;
        std::cout << "| nsm init           | initialise managing a site - input: (site-name) |" << std::endl;
        std::cout << "| nsm status         | lists updated and problem pages                 |" << std::endl;
        std::cout << "| nsm info           | input: page-name-1 .. page-name-k               |" << std::endl;
        std::cout << "| nsm info-all       | lists tracked pages                             |" << std::endl;
        std::cout << "| nsm info-names     | lists tracked page names                        |" << std::endl;
        std::cout << "| nsm track          | input: page-name (page-title) (template-path)   |" << std::endl;
        std::cout << "| nsm untrack        | input: page-name                                |" << std::endl;
        std::cout << "| nsm rm             | input: page-name                                |" << std::endl;
        std::cout << "| nsm mv             | input: old-name new-name                        |" << std::endl;
        std::cout << "| nsm cp             | input: tracked-name new-name                    |" << std::endl;
        std::cout << "| nsm build          | input: page-name-1 .. page-name-k               |" << std::endl;
        std::cout << "| nsm build-updated  | builds updated pages                            |" << std::endl;
        std::cout << "| nsm build-all      | builds all tracked pages                        |" << std::endl;
        std::cout << "| nsm bcp            | input: commit-message                           |" << std::endl;
        std::cout << "| nsm new-title      | input: page-name new-title                      |" << std::endl;
        std::cout << "| nsm new-template   | input: page-name template-path                  |" << std::endl;
        std::cout << "+----------------------------------------------------------------------+" << std::endl;

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
            chdir(parentDir.c_str());

            //gets new pwd
            pwd = get_pwd();

            if(std::ifstream(".siteinfo/pages.list") || std::ifstream(".siteinfo/nsm.config"))
            {
                std::cout << "nsm is already managing a site in " << owd << " (or an accessible ancestor directory)" << std::endl;
                return 1;
            }
        }
        chdir(owd.c_str());

        //checks that directory is empty
        system("ls -a > .txt23235f2t.txt");
        std::ifstream ifs(".txt23235f2t.txt");
        std::string str = "", istr;
        while(getline(ifs, istr))
            str += istr + " ";
        ifs.close();
        system("rm -r .txt23235f2t.txt");
        if(str != ". .. .git .txt23235f2t.txt " && str != ". .. .txt23235f2t.txt ")
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
            system("attrib +h .siteinfo");
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
        site.build_updated();

        //system("nsm track index");
        //system("nsm build-updated");

        std::cout << "nsm: initialised empty site in " << get_pwd() << "/.siteinfo/" << std::endl;

        return 0;
    }
    else if(cmd == "clone")
    {
        //ensures correct number of parameters given
        if(noParams != 2)
            return parError(noParams, argv, "2");

        std::string cloneCmnd = "git clone " + std::string(argv[2]);
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

        system(cloneCmnd.c_str());

        for(auto i=cloneCmnd.find_last_of('/')+1; i < cloneCmnd.find_last_of('.'); ++i)
            dirName += cloneCmnd[i];

        if(cloneCmnd.find("gitlab") == std::string::npos)
        {
            chdir(dirName.c_str());

            system("git checkout stage > /dev/null 2>&1");

            std::set<std::string> branches = get_git_branches();

            if(branches.count("stage") && std::ifstream(".siteinfo/nsm.config"))
            {
                SiteInfo site;
                if(site.open() > 0)
                    return 1;

                chdir(parDir.c_str());

                std::string cpCmnd = "cp -r " + dirName + " .abcd143d";
                system(cpCmnd.c_str());

                chdir(dirName.c_str());

                std::string rmCmnd = "rm -r " + site.siteDir + " > /dev/null 2>&1";
                system(rmCmnd.c_str());

                chdir(parDir.c_str());
                std::string mvCmnd = "mv .abcd143d " + dirName + "/" + site.siteDir;
                system(mvCmnd.c_str());

                chdir((dirName + "/" + site.siteDir).c_str());

                system("git checkout master > /dev/null 2>&1");
            }
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
           cmd != "mv" &&
           cmd != "cp" &&
           cmd != "new-title" &&
           cmd != "new-template" &&
           cmd != "build-updated" &&
           cmd != "build" &&
           cmd != "build-all")
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
            chdir(parentDir.c_str());

            //gets new pwd
            pwd = get_pwd();

            //checks we haven't made it to root directory or stuck at same dir
            if(pwd == rootDir || pwd == prevPwd)
            {
                std::cout << "nsm is not managing a site from " << owd << " (or any accessible parent directories)" << std::endl;
                return 1;
            }
        }

        siteRootDir = get_pwd();

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

            system("git config --get remote.origin.url > .txt65232g42f.txt");
            std::ifstream ifs(".txt65232g42f.txt");
            std::string str="";
            ifs >> str;
            ifs.close();
            system("rm -r .txt65232g42f.txt");
            if(str == "")
            {
                std::cout << "error: no remote git url set" << std::endl;
                return 1;
            }

            if(site.build_updated())
                return 1;

            std::string commitCmnd = "git commit -m \"" + std::string(argv[2]) + "\"",
                        pushCmnd,
                        siteDirBranch,
                        siteRootDirBranch = get_pb();
            std::cout << commitCmnd << std::endl;

            chdir(site.siteDir.c_str());

            siteDirBranch = get_pb();

            if(siteDirBranch != siteRootDirBranch)
            {
                pushCmnd = "git push origin " + siteDirBranch;

                system("git add -A .");
                system(commitCmnd.c_str());
                system(pushCmnd.c_str());
            }

            chdir(siteRootDir.c_str());

            pushCmnd = "git push origin " + siteRootDirBranch;

            system("git add -A .");
            system(commitCmnd.c_str());
            system(pushCmnd.c_str());

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
        else if(cmd == "rm")
        {
            //ensures correct number of parameters given
            if(noParams != 2)
                return parError(noParams, argv, "2");

            Name pageNameToRemove = argv[2];

            return site.rm(pageNameToRemove);
        }
        else if(cmd == "mv")
        {
            //ensures correct number of parameters given
            if(noParams != 3)
                return parError(noParams, argv, "3");

            Name oldPageName = argv[2],
                 newPageName = argv[3];

            return site.mv(oldPageName, newPageName);
        }
        else if(cmd == "cp")
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

            return site.build_updated();
        }
        else if(cmd == "build")
        {
            //ensures correct number of parameters given
            if(noParams <= 1)
                return parError(noParams, argv, ">1");

            std::vector<Name> pageNamesToBuild;
            for(int p=2; p<argc; p++)
            {
                pageNamesToBuild.push_back(argv[p]);
            }
            return site.build(pageNamesToBuild);
        }
        else if(cmd == "build-all")
        {
            //ensures correct number of parameters given
            if(noParams != 1)
                return parError(noParams, argv, "1");

            return site.build_all();
        }
        else
        {
            unrecognisedCommand("nsm", cmd);
            return 1;
        }
    }

    return 0;
}
