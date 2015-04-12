#include <cstdio>
#include <iostream>

#include "SiteInfo.h"

void unrecognisedCommand(const std::string from, const std::string cmd)
{
    std::cout << "error: " << from << " does not recognise the command '" << cmd << "'" << std::endl;
}

bool parError(int noParams, char* argv[], int expectedNo)
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
        std::cout << "nsm build-updated  | builds updated pages" << std::endl;
        std::cout << "nsm build-all      | builds all tracked pages " << std::endl;
        std::cout << "nsm build-pages    | input: page-loc-1 .. page-loc-k" << std::endl;
        std::cout << "nsm init           | initialise managing a site" << std::endl;
        std::cout << "nsm track          | input: title page-loc content-loc template-loc" << std::endl;
        std::cout << "nsm tracked        | lists tracked pages" << std::endl;
        std::cout << "nsm tracked-locs   | lists tracked page locations" << std::endl;
        std::cout << "nsm untrack        | input: page-loc" << std::endl;
        std::cout << "nsm status         | lists updated and problem pages" << std::endl;
        std::cout << "------------------------------------------------" << std::endl;

        return 0;
    }

    if(cmd == "init")
    {
        //ensures correct number of parameters given
        if(noParams > 1)
            return parError(noParams, argv, 1);

        //ensures nsm isn't already managing a site from directory
        if(std::ifstream(".siteinfo/pages.list"))
        {
            std::cout << "error: nsm is already managing a site from this directory" << std::endl;
            return 1;
        }

        //sets up
        system("if [ ! -d .siteinfo ]; then mkdir .siteinfo; fi");
        system("if [ ! -f .siteinfo/pages.list ]; then touch .siteinfo/pages.list; fi");

        return 0;
    }
    //ensures nsm is managing a site from this directory
    else if(!std::ifstream(".siteinfo/pages.list"))
    {
        std::cout << "nsm is not managing a site from this directory" << std::endl;
        return 1;
    }

    //opens up site information
    SiteInfo site;
    if(site.open() > 0)
        return 1;

    if(cmd == "status")
    {
        //ensures correct number of parameters given
        if(noParams != 1)
            return parError(noParams, argv, 1);

        return site.status();
    }
    else if(cmd == "tracked")
    {
        //ensures correct number of parameters given
        if(noParams > 1)
            return parError(noParams, argv, 1);

        return site.tracked();
    }
    else if(cmd == "tracked-locs")
    {
        //ensures correct number of parameters given
        if(noParams > 1)
            return parError(noParams, argv, 1);

        return site.tracked_locs();
    }
    else if(cmd == "track")
    {
        //ensures correct number of parameters given
        if(noParams != 5)
            return parError(noParams, argv, 5);

        return site.track(PageInfo(argv[2], argv[3], argv[4], argv[5]));
    }
    else if(cmd == "untrack")
    {
        //ensures correct number of parameters given
        if(noParams != 2)
            return parError(noParams, argv, 1);

        return site.untrack(Location(argv[2]));
    }
    else if(cmd == "build-updated")
    {
        //ensures correct number of parameters given
        if(noParams > 1)
            return parError(noParams, argv, 1);

        return site.build_updated();
    }
    else if(cmd == "build-pages")
    {
        //ensures correct number of parameters given
        if(noParams <= 1)
            return parError(noParams, argv, -1);

        std::vector<Location> pageLocsToBuild;
        for(int p=2; p<argc; p++)
            pageLocsToBuild.push_back(Location(argv[p]));
        return site.build_pages(pageLocsToBuild);
    }
    else if(cmd == "build-all")
    {
        //ensures correct number of parameters given
        if(noParams != 1)
            return parError(noParams, argv, 1);

        return site.build_all();
    }
    else
    {
        unrecognisedCommand("nsm", cmd);
    }

    return 0;
}
