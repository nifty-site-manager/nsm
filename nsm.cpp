#include "SiteInfo.h"
#include <unistd.h>

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

    if(cmd == "help")
    {
        std::cout << "-------------- available commands --------------" << std::endl;
        std::cout << "nsm init              | initialise managing a site" << std::endl;
        std::cout << "nsm info              | input: page-path-1 .. page-path-k" << std::endl;
        std::cout << "nsm info-all          | lists tracked pages" << std::endl;
        std::cout << "nsm info-paths        | lists tracked page paths" << std::endl;
        std::cout << "nsm track             | input: title page-path content-path template-path" << std::endl;
        std::cout << "nsm untrack           | input: page-path" << std::endl;
        std::cout << "nsm new-title         | input: page-path new-title" << std::endl;
        std::cout << "nsm new-page-path     | input: old-page-path new-page-path" << std::endl;
        std::cout << "nsm new-content-path  | input: page-path new-content-path" << std::endl;
        std::cout << "nsm new-template-path | input: page-path new-template-path" << std::endl;
        std::cout << "nsm build             | input: page-path-1 .. page-path-k" << std::endl;
        std::cout << "nsm build-updated     | builds updated pages" << std::endl;
        std::cout << "nsm build-all         | builds all tracked pages " << std::endl;
        std::cout << "nsm status            | lists updated and problem pages" << std::endl;
        std::cout << "nsm help              | lists all nsm commands" << std::endl;
        std::cout << "------------------------------------------------" << std::endl;

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
            std::cout << "error: nsm is already managing a site from this directory" << std::endl;
            return 1;
        }

        //sets up
        Path pagesListPath(".siteinfo/", "pages.list");
        //ensures pages list file exists
        pagesListPath.ensurePathExists();
        //adds read and write permissions to pages list file
        chmod(pagesListPath.str().c_str(), 0666);

        return 0;
    }

    //ensures nsm is managing a site from this directory or one of the parent directories
    std::string parentDir = "../",
        rootDir = "/",
        pwd = get_current_dir_name(),
        oPwd;

    while(!std::ifstream(".siteinfo/pages.list"))
    {
        //sets old pwd
        oPwd = pwd;

        //changes to parent directory
        chdir(parentDir.c_str());

        //gets new pwd
        pwd = get_current_dir_name();

        //checks we haven't made it to root directory or stuck at same dir
        if(pwd == rootDir || pwd == oPwd)
        {
            std::cout << "nsm is not managing a site from this directory (or any accessible parent directories)" << std::endl;
            return 1;
        }
    }

    //opens up site information
    SiteInfo site;
    if(site.open() > 0)
        return 1;

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

        std::vector<Path> pathsForInfo;
        Path infoPath;
        for(int p=2; p<argc; p++)
        {
            infoPath.set_file_path_from(argv[p]);
            pathsForInfo.push_back(infoPath);
        }
        return site.info(pathsForInfo);
    }
    else if(cmd == "info-all")
    {
        //ensures correct number of parameters given
        if(noParams > 1)
            return parError(noParams, argv, "1");

        return site.info_all();
    }
    else if(cmd == "info-paths")
    {
        //ensures correct number of parameters given
        if(noParams > 1)
            return parError(noParams, argv, "1");

        return site.info_paths();
    }
    else if(cmd == "track")
    {
        //ensures correct number of parameters given
        if(noParams != 5)
            return parError(noParams, argv, "5");

        return site.track(PageInfo(argv[2], argv[3], argv[4], argv[5]));
    }
    else if(cmd == "untrack")
    {
        //ensures correct number of parameters given
        if(noParams != 2)
            return parError(noParams, argv, "2");

        Path untrackPath;
        untrackPath.set_file_path_from(argv[2]);
        return site.untrack(untrackPath);
    }
    else if(cmd == "new-title")
    {
        //ensures correct number of parameters given
        if(noParams != 3)
            return parError(noParams, argv, "3");

        Path pagePath;
        pagePath.set_file_path_from(argv[2]);
        Title newTitle;
        newTitle.str = argv[3];

        return site.new_title(pagePath, newTitle);
    }
    else if(cmd == "new-page-path")
    {
        //ensures correct number of parameters given
        if(noParams != 3)
            return parError(noParams, argv, "3");

        Path oldPagePath;
        oldPagePath.set_file_path_from(argv[2]);
        Path newPagePath;
        newPagePath.set_file_path_from(argv[3]);

        return site.new_page_path(oldPagePath, newPagePath);
    }
    else if(cmd == "new-content-path")
    {
        //ensures correct number of parameters given
        if(noParams != 3)
            return parError(noParams, argv, "3");

        Path pagePath;
        pagePath.set_file_path_from(argv[2]);
        Path newContentPath;
        newContentPath.set_file_path_from(argv[3]);

        return site.new_content_path(pagePath, newContentPath);
    }
    else if(cmd == "new-template-path")
    {
        //ensures correct number of parameters given
        if(noParams != 3)
            return parError(noParams, argv, "3");

        Path pagePath;
        pagePath.set_file_path_from(argv[2]);
        Path newTemplatePath;
        newTemplatePath.set_file_path_from(argv[3]);

        return site.new_template_path(pagePath, newTemplatePath);
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

        std::vector<Path> pagePathsToBuild;
        Path buildPath;
        for(int p=2; p<argc; p++)
        {
            buildPath.set_file_path_from(argv[p]);
            pagePathsToBuild.push_back(buildPath);
        }
        return site.build(pagePathsToBuild);
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
