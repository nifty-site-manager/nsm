#ifndef SITE_INFO_H_
#define SITE_INFO_H_

#include <cstdio>
#include <iostream>
#include <fstream>
#include <set>
#include <vector>

#include "Quoted.h"
#include "PageInfo.h"

struct SiteInfo
{
    std::set<PageInfo> pages;

    bool open()
    {
        pages.clear();

        if(!std::ifstream(".siteinfo/pages.list"))
        {
            //this should never happen!
            std::cout << "ERROR: SiteInfo.h: could not open site information as .siteinfo/pages.list does not exist" << std::endl;
            return 1;
        }

        std::ifstream ifs(".siteinfo/pages.list");
        PageInfo inPage;
        while(ifs >> inPage)
        {
            //checks that content and template files aren't the same
            if(inPage.contentLocation == inPage.templateLocation)
            {
                std::cout << "error: failed to open .siteinfo/pages.list" << std::endl;
                std::cout << "reason: page " << inPage.pageLocation << " has same content and template location " << inPage.templateLocation << std::endl;

                return 1;
            }

            //makes sure there's no duplicate entries in pages.list
            if(pages.count(inPage) > 0)
            {
                PageInfo cInfo = *(pages.find(inPage));

                std::cout << "error: failed to open .siteinfo/pages.list" << std::endl;
                std::cout << "reason: duplicate entry for " << inPage.pageLocation << std::endl;
                std::cout << std::endl;
                std::cout << "----- first entry -----" << std::endl;
                std::cout << "       page title: " << cInfo.pageTitle << std::endl;
                std::cout << "    page location: " << cInfo.pageLocation << std::endl;
                std::cout << " content location: " << cInfo.contentLocation << std::endl;
                std::cout << "template location: " << cInfo.templateLocation << std::endl;
                std::cout << "--------------------------------" << std::endl;
                std::cout << std::endl;
                std::cout << "----- second entry -----" << std::endl;
                std::cout << "       page title: " << inPage.pageTitle << std::endl;
                std::cout << "    page location: " << inPage.pageLocation << std::endl;
                std::cout << " content location: " << inPage.contentLocation << std::endl;
                std::cout << "template location: " << inPage.templateLocation << std::endl;
                std::cout << "--------------------------------" << std::endl;

                return 1;
            }

            pages.insert(inPage);
        }

        return 0;
    };

    bool save()
    {
        std::ofstream ofs(".siteinfo/pages.list");

        for(auto page=pages.begin(); page != pages.end(); page++)
        {
            ofs << page->pageTitle << std::endl;
            ofs << page->pageLocation << std::endl;
            ofs << page->contentLocation << std::endl;
            ofs << page->templateLocation << std::endl;
            ofs << std::endl;
        }

        return 0;
    };

    bool tracking(const PageInfo &page)
    {
        return pages.count(page);
    }

    bool tracking(const Location &pageLoc)
    {
        PageInfo page;
        page.pageLocation = pageLoc;
        return pages.count(page);
    }

    PageInfo get_tracked(const Location &pageLoc)
    {
        PageInfo page;
        page.pageLocation = pageLoc;
        return *pages.find(page);
    }

    bool tracked()
    {
        std::cout << std::endl;
        std::cout << "------ tracked pages ------" << std::endl;
        for(auto page=pages.begin(); page!=pages.end(); page++)
        {
            if(page != pages.begin())
                std::cout << std::endl;
            std::cout << "       page title: " << page->pageTitle << std::endl;
            std::cout << "    page location: " << page->pageLocation << std::endl;
            std::cout << " content location: " << page->contentLocation << std::endl;
            std::cout << "template location: " << page->templateLocation << std::endl;
        }
        std::cout << "-----------------------------" << std::endl;

        return 0;
    };

    bool tracked_locs()
    {
        std::cout << std::endl;
        std::cout << "--------- tracked page locations ---------" << std::endl;
        for(auto page=pages.begin(); page!=pages.end(); page++)
            std::cout << page->pageLocation << std::endl;
        std::cout << "------------------------------------------" << std::endl;

        return 0;
    };

    bool track(const PageInfo &newPage)
    {
        if(newPage.contentLocation == newPage.templateLocation)
        {
            std::cout << std::endl;
            std::cout << "error: content and template locations cannot be the same, page not tracked" << std::endl;
            return 1;
        }

        if(pages.count(newPage) > 0)
        {
            PageInfo cInfo = *(pages.find(newPage));

            std::cout << std::endl;
            std::cout << "error: nsm is already tracking " << newPage.pageLocation << std::endl;
            std::cout << "----- current page details -----" << std::endl;
            std::cout << "       page title: " << cInfo.pageTitle << std::endl;
            std::cout << "    page location: " << cInfo.pageLocation << std::endl;
            std::cout << " content location: " << cInfo.contentLocation << std::endl;
            std::cout << "template location: " << cInfo.templateLocation << std::endl;
            std::cout << "--------------------------------" << std::endl;

            return 1;
        }

        //warns user if content and/or template locs don't exist
        if(!std::ifstream(STR(newPage.contentLocation)))
        {
            std::cout << std::endl;
            std::cout << "warning: content loc " << newPage.contentLocation << " does not exist" << std::endl;
        }
        if(!std::ifstream(STR(newPage.templateLocation)))
        {
            std::cout << std::endl;
            std::cout << "warning: template loc " << newPage.templateLocation << " does not exist" << std::endl;
        }

        //inserts new page into set of pages
        pages.insert(newPage);

        //saves new set of pages to pages.list
        std::ofstream ofs(".siteinfo/pages.list");
        for(auto page=pages.begin(); page!=pages.end(); page++)
            ofs << *page << std::endl << std::endl;
        ofs.close();

        //informs user that page addition was successful
        std::cout << std::endl;
        std::cout << "successfully tracking " << newPage.pageLocation << std::endl;

        return 0;
    };

    bool untrack(const Location &pageLocToUntrack)
    {
        //checks that page is being tracked
        if(!tracking(pageLocToUntrack))
        {
            std::cout << std::endl;
            std::cout << "error: nsm is not tracking " << pageLocToUntrack << std::endl;
            return 1;
        }
        else
        {
            PageInfo pageToErase;
            pageToErase.pageLocation = pageLocToUntrack;

            //removes page info file
            std::string systemCall = "rm -f " + STR(getInfoLocation(pageLocToUntrack));
            system(systemCall.c_str());

            //removes page from pages set
            pages.erase(pageToErase);

            //saves new set of pages to pages.list
            std::ofstream ofs(".siteinfo/pages.list");
            for(auto page=pages.begin(); page!=pages.end(); page++)
                ofs << *page << std::endl << std::endl;
            ofs.close();

            //informs user that page was successfully untracked
            std::cout << std::endl;
            std::cout << "successfully untracked " << pageLocToUntrack << std::endl;
        }
    };

    int build_pages(std::vector<Location> pageLocsToBuild)
    {
        std::set<Location> untrackedPages, failedPages;

        for(auto pageLoc=pageLocsToBuild.begin(); pageLoc != pageLocsToBuild.end(); pageLoc++)
        {
            if(tracking(*pageLoc))
            {
                if(build_page(get_tracked(*pageLoc)) > 0)
                    failedPages.insert(*pageLoc);
            }
            else
                untrackedPages.insert(*pageLoc);
        }

        if(failedPages.size() > 0)
        {
            std::cout << std::endl;
            std::cout << "---- following pages failed to build ----" << std::endl;
            for(auto fLoc=failedPages.begin(); fLoc != failedPages.end(); fLoc++)
                std::cout << " " << *fLoc << std::endl;
            std::cout << "-----------------------------------------" << std::endl;
        }
        if(untrackedPages.size() > 0)
        {
            std::cout << std::endl;
            std::cout << "---- nsm not tracking following pages ----" << std::endl;
            for(auto uLoc=untrackedPages.begin(); uLoc != untrackedPages.end(); uLoc++)
                std::cout << *uLoc << std::endl;
            std::cout << "------------------------------------------" << std::endl;
        }
        if(failedPages.size() == 0 && untrackedPages.size() == 0)
        {
            std::cout << std::endl;
            std::cout << "all pages built successfully" << std::endl;
        }
    };

    int build_all()
    {
        if(pages.size() == 0)
        {
            std::cout << std::endl;
            std::cout << "nsm is not tracking any pages, nothing to build" << std::endl;
            return 0;
        }

        std::set<Location> failedPages;

        for(auto page=pages.begin(); page != pages.end(); page++)
            if(build_page(*page) > 0)
                failedPages.insert(page->pageLocation);

        if(failedPages.size() > 0)
        {
            std::cout << std::endl;
            std::cout << "---- following pages failed to build ----" << std::endl;
            for(auto fLoc=failedPages.begin(); fLoc != failedPages.end(); fLoc++)
                std::cout << " " << *fLoc;
            std::cout << std::endl;
            std::cout << "-----------------------------------------" << std::endl;
        }
        else
        {
            std::cout << std::endl;
            std::cout << "all pages built successfully" << std::endl;
        }

        return 0;
    };

    bool build_updated()
    {
        std::set<PageInfo> updatedPages;
        std::set<Location> builtPages,
            problemPages;

        std::cout << std::endl;
        for(auto page=pages.begin(); page != pages.end(); page++)
        {
            //checks whether content and template files exist
            if(!std::ifstream(STR(page->contentLocation)))
            {
                std::cout << page->pageLocation << ": content file " << page->contentLocation << " does not exist" << std::endl;
                problemPages.insert(page->pageLocation);
                continue;
            }
            if(!std::ifstream(STR(page->templateLocation)))
            {
                std::cout << page->pageLocation << ": template file " << page->templateLocation << " does not exist" << std::endl;
                problemPages.insert(page->pageLocation);
                continue;
            }

            //gets location of pages information from last time page was built
            Location pageInfoLoc = getInfoLocation(page->pageLocation);

            //checks whether info location exists
            if(!std::ifstream(STR(pageInfoLoc)))
            {
                std::cout << page->pageLocation << ": yet to be built" << std::endl;
                updatedPages.insert(*page);
                continue;
            }
            else
            {
                std::ifstream infoStream(STR(pageInfoLoc));
                PageInfo prevPageInfo;
                infoStream >> prevPageInfo;

                if(page->pageTitle != prevPageInfo.pageTitle)
                {
                    std::cout << page->pageLocation << ": title changed to " << page->pageTitle << " from " << prevPageInfo.pageTitle << std::endl;
                    updatedPages.insert(*page);
                    continue;
                }

                if(page->contentLocation != prevPageInfo.contentLocation)
                {
                    std::cout << page->pageLocation << ": content loc changed to " << page->contentLocation << " from " << prevPageInfo.contentLocation << std::endl;
                    updatedPages.insert(*page);
                    continue;
                }

                if(page->templateLocation != prevPageInfo.templateLocation)
                {
                    std::cout << page->pageLocation << ": template loc changed to " << page->templateLocation << " from " << prevPageInfo.templateLocation << std::endl;
                    updatedPages.insert(*page);
                    continue;
                }

                Location dep;
                while(infoStream >> dep)
                {
                    if(!std::ifstream(STR(dep)))
                    {
                        std::cout << page->pageLocation << ": dep loc " << dep << " removed since last build" << std::endl;
                        updatedPages.insert(*page);
                        break;
                    }
                    else if(modified_after(dep, pageInfoLoc))
                    {
                        std::cout << page->pageLocation << ": dep loc " << dep << " modified since last build" << std::endl;
                        updatedPages.insert(*page);
                        break;
                    }
                }
            }
        }

        for(auto page=updatedPages.begin(); page != updatedPages.end(); page++)
        {
            if(build_page(*page) > 0)
                problemPages.insert(page->pageLocation);
            else
                builtPages.insert(page->pageLocation);
        }

        if(builtPages.size() > 0)
        {
            std::cout << std::endl;
            std::cout << "----- pages successfully built -----" << std::endl;
            for(auto bPage=builtPages.begin(); bPage != builtPages.end(); bPage++)
                std::cout << *bPage << std::endl;
            std::cout << "------------------------------------" << std::endl;
        }

        if(problemPages.size() > 0)
        {
            std::cout << std::endl;
            std::cout << "----- updated pages which failed to build -----" << std::endl;
            for(auto uPage=problemPages.begin(); uPage != problemPages.end(); uPage++)
                std::cout << *uPage << std::endl;
            std::cout << "-----------------------------------------------" << std::endl;
        }

        if(builtPages.size() == 0 && problemPages.size() == 0)
        {
            //std::cout << std::endl;
            std::cout << "all pages are already up to date" << std::endl;
        }
    }

    bool status()
    {
        std::set<Location> updatedFiles, removedFiles;
        std::set<PageInfo> updatedPages, problemPages;

        for(auto page=pages.begin(); page != pages.end(); page++)
        {
            bool needsUpdating = 0;

            //checks whether content and template files exist
            if(!std::ifstream(STR(page->contentLocation)))
            {
                needsUpdating = 1; //shouldn't need this but doesn't cost much
                problemPages.insert(*page);
                //note user will be informed about this as a dep not existing
            }
            if(!std::ifstream(STR(page->templateLocation)))
            {
                needsUpdating = 1; //shouldn't need this but doesn't cost much
                problemPages.insert(*page);
                //note user will be informed about this as a dep not existing
            }

            //gets location of pages information from last time page was built
            Location pageInfoLoc = getInfoLocation(page->pageLocation);

            //checks whether info location exists
            if(!std::ifstream(STR(pageInfoLoc)))
            {
                std::cout << page->pageLocation << ": yet to be built" << std::endl;
                needsUpdating = 1;
            }
            else
            {
                std::ifstream infoStream(STR(pageInfoLoc));
                PageInfo prevPageInfo;
                infoStream >> prevPageInfo;

                if(page->pageTitle != prevPageInfo.pageTitle)
                {
                    std::cout << page->pageLocation << ": title changed to " << page->pageTitle << " from " << prevPageInfo.pageTitle << std::endl;
                    needsUpdating = 1;
                }

                if(page->contentLocation != prevPageInfo.contentLocation)
                {
                    std::cout << page->pageLocation << ": content loc changed to " << page->contentLocation << " from " << prevPageInfo.contentLocation << std::endl;
                    needsUpdating = 1;
                }

                if(page->templateLocation != prevPageInfo.templateLocation)
                {
                    std::cout << page->pageLocation << ": template loc changed to " << page->templateLocation << " from " << prevPageInfo.templateLocation << std::endl;
                    needsUpdating = 1;
                }

                Location dep;
                while(infoStream >> dep)
                {
                    if(!std::ifstream(STR(dep)))
                    {
                        removedFiles.insert(dep);
                        needsUpdating = 1;
                    }
                    else if(modified_after(dep, pageInfoLoc))
                    {
                        updatedFiles.insert(dep);
                        needsUpdating = 1;
                    }
                }
            }

            if(needsUpdating && problemPages.count(*page) == 0)
                updatedPages.insert(*page);
        }

        if(removedFiles.size() > 0)
        {
            std::cout << std::endl;
            std::cout << "---- removed content files ----" << std::endl;
            for(auto rFile=removedFiles.begin(); rFile != removedFiles.end(); rFile++)
                std::cout << *rFile << std::endl;
            std::cout << "-------------------------------" << std::endl;
        }

        if(updatedFiles.size() > 0)
        {
            std::cout << std::endl;
            std::cout << "------- updated content files ------" << std::endl;
            for(auto uFile=updatedFiles.begin(); uFile != updatedFiles.end(); uFile++)
                std::cout << *uFile << std::endl;
            std::cout << "------------------------------------" << std::endl;
        }

        if(updatedPages.size() > 0)
        {
            std::cout << std::endl;
            std::cout << "----- pages that need building -----" << std::endl;
            for(auto uPage=updatedPages.begin(); uPage != updatedPages.end(); uPage++)
                std::cout << uPage->pageLocation << std::endl;
            std::cout << "------------------------------------" << std::endl;
        }

        if(problemPages.size() > 0)
        {
            std::cout << std::endl;
            std::cout << "----- pages with missing content or template file -----" << std::endl;
            for(auto uPage=problemPages.begin(); uPage != problemPages.end(); uPage++)
                std::cout << uPage->pageLocation << std::endl;
            std::cout << "-------------------------------------------------------" << std::endl;
        }

        if(updatedFiles.size() == 0 && updatedPages.size() == 0 && problemPages.size() == 0)
        {
            std::cout << std::endl;
            std::cout << "all pages are already up to date" << std::endl;
        }
    };

};

#endif //SITE_INFO_H_
