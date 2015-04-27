#include "SiteInfo.h"
#include <unistd.h>

std::string get_pwd()
{
    char * pwd_char = getcwd(NULL, 0);
    std::string pwd = pwd_char;
    free(pwd_char);
    return pwd;
}

void unrecognisedCommand(const std::string from, const std::string cmd)
{
    std::cout << "error: " << from << " does not recognise the command '" << cmd << "'" << std::endl;
}

bool parError(int noParams, char* argv[], const std::string &expectedNo)
{
        std::cout << "error: " << noParams << " is more than the " << expectedNo << " parameters expected" << std::endl;
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

    if(cmd == "commands")
    {
        std::cout << "------------------- available commands -------------------" << std::endl;
        std::cout << "nsm commands      | lists all nsm commands" << std::endl;
        std::cout << "nsm config        | lists config settings" << std::endl;
        std::cout << "nsm init          | initialise managing a site" << std::endl;
        std::cout << "nsm status        | lists updated and problem pages" << std::endl;
        std::cout << "nsm info          | input: page-name-1 .. page-name-k" << std::endl;
        std::cout << "nsm info-all      | lists tracked pages" << std::endl;
        std::cout << "nsm info-names    | lists tracked page names" << std::endl;
        std::cout << "nsm track         | input: page-name (page-title) (template-path)" << std::endl;
        std::cout << "nsm untrack       | input: page-name" << std::endl;
        std::cout << "nsm rm            | input: page-name" << std::endl;
        std::cout << "nsm mv            | input: old-name new-name" << std::endl;
        std::cout << "nsm cp            | input: tracked-name new-name" << std::endl;
        std::cout << "nsm build         | input: page-name-1 .. page-name-k" << std::endl;
        std::cout << "nsm build-updated | builds updated pages" << std::endl;
        std::cout << "nsm build-all     | builds all tracked pages " << std::endl;
        std::cout << "nsm new-title     | input: page-name new-title" << std::endl;
        std::cout << "nsm new-template  | input: page-name template-path" << std::endl;
        std::cout << "----------------------------------------------------------" << std::endl;

        return 0;
    }

    if(cmd == "init")
    {
        //ensures correct number of parameters given
        if(noParams > 1)
            return parError(noParams, argv, "1");

        //ensures nsm isn't already managing a site from directory
        if(std::ifstream(".siteinfo/pages.list"))
        {
            std::cout << "error: nsm is already managing a site in " << get_pwd() << "/" << std::endl;
            return 1;
        }

        //sets up
        Path pagesListPath(".siteinfo/", "pages.list");
        //ensures pages list file exists
        pagesListPath.ensurePathExists();
        //adds read and write permissions to pages list file
        chmod(pagesListPath.str().c_str(), 0666);

        std::ofstream ofs(".siteinfo/nsm.config");
        ofs << "contentDir content/" << std::endl;
        ofs << "contentExt .content" << std::endl;
        ofs << "siteDir site/" << std::endl;
        ofs << "pageExt .html" << std::endl;
        ofs << "defaultTemplate template/page.template" << std::endl;
        ofs.close();

        std::cout << "nsm: initialised empty site in " << get_pwd() << "/.siteinfo/" << std::endl;

        return 0;
    }

    //ensures nsm is managing a site from this directory or one of the parent directories
    std::string parentDir = "../",
        rootDir = "/",
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
    if(cmd == "status")
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

        Name newPageName = argv[2];
        Title newPageTitle;
        if(noParams >= 3)
            newPageTitle = argv[3];
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
    }

    return 0;
}
