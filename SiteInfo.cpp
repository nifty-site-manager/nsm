#include "SiteInfo.h"


int SiteInfo::open()
{
    pages.clear();

    if(!std::ifstream(".siteinfo/nsm.config"))
    {
        //this should never happen!
        std::cout << "ERROR: SiteInfo.h: could not open nsm config file as .siteinfo/nsm.config does not exist" << std::endl;
        return 1;
    }

    if(!std::ifstream(".siteinfo/pages.list"))
    {
        //this should never happen!
        std::cout << "ERROR: SiteInfo.h: could not open site information as .siteinfo/pages.list does not exist" << std::endl;
        return 1;
    }

    //reads nsm config file
    std::ifstream ifs(".siteinfo/nsm.config");
    std::string inType;
    while(ifs >> inType)
    {
        if(inType == "contentDir")
            read_quoted(ifs, contentDir);
        else if(inType == "contentExt")
            read_quoted(ifs, contentExt);
        else if(inType == "siteDir")
            read_quoted(ifs, siteDir);
        else if(inType == "pageExt")
            read_quoted(ifs, pageExt);
        else if(inType == "defaultTemplate")
            defaultTemplate.read_file_path_from(ifs);
    }
    ifs.close();

    //reads page list file
    ifs.open(".siteinfo/pages.list");
    Name inName;
    Title inTitle;
    Path inTemplatePath;
    while(read_quoted(ifs, inName))
    {
        inTitle.read_quoted_from(ifs);
        inTemplatePath.read_file_path_from(ifs);

        PageInfo inPage = make_info(inName, inTitle, inTemplatePath);

        //checks that content and template files aren't the same
        if(inPage.contentPath == inPage.templatePath)
        {
            std::cout << "error: failed to open .siteinfo/pages.list" << std::endl;
            std::cout << "reason: page " << inPage.pagePath << " has same content and template path" << inPage.templatePath << std::endl;

            return 1;
        }

        //makes sure there's no duplicate entries in pages.list
        if(pages.count(inPage) > 0)
        {
            PageInfo cInfo = *(pages.find(inPage));

            std::cout << "error: failed to open .siteinfo/pages.list" << std::endl;
            std::cout << "reason: duplicate entry for " << inPage.pagePath << std::endl;
            std::cout << std::endl;
            std::cout << "----- first entry -----" << std::endl;
            std::cout << "   page title: " << cInfo.pageTitle << std::endl;
            std::cout << "    page path: " << cInfo.pagePath << std::endl;
            std::cout << " content path: " << cInfo.contentPath << std::endl;
            std::cout << "template path: " << cInfo.templatePath << std::endl;
            std::cout << "--------------------------------" << std::endl;
            std::cout << std::endl;
            std::cout << "----- second entry -----" << std::endl;
            std::cout << "   page title: " << inPage.pageTitle << std::endl;
            std::cout << "    page path: " << inPage.pagePath << std::endl;
            std::cout << " content path: " << inPage.contentPath << std::endl;
            std::cout << "template path: " << inPage.templatePath << std::endl;
            std::cout << "--------------------------------" << std::endl;

            return 1;
        }

        pages.insert(inPage);
    }

    return 0;
}

int SiteInfo::save()
{
    std::ofstream ofs(".siteinfo/pages.list");

    for(auto page=pages.begin(); page != pages.end(); page++)
    {
        ofs << page->pageName << std::endl;
        ofs << page->pageTitle << std::endl;
        ofs << page->templatePath << std::endl;
        ofs << std::endl;
    }

    return 0;
}

PageInfo SiteInfo::make_info(const Name &pageName)
{
    PageInfo pageInfo;

    pageInfo.pageName = pageName;

    Path pageNameAsPath;
    pageNameAsPath.set_file_path_from(pageName);

    pageInfo.contentPath = Path(contentDir + pageNameAsPath.dir, pageNameAsPath.file + contentExt);
    pageInfo.pagePath = Path(siteDir + pageNameAsPath.dir, pageNameAsPath.file + pageExt);

    pageInfo.pageTitle = pageName;
    pageInfo.templatePath = defaultTemplate;

    return pageInfo;
}

PageInfo SiteInfo::make_info(const Name &pageName, const Title &pageTitle)
{
    PageInfo pageInfo;

    pageInfo.pageName = pageName;

    Path pageNameAsPath;
    pageNameAsPath.set_file_path_from(pageName);

    pageInfo.contentPath = Path(contentDir + pageNameAsPath.dir, pageNameAsPath.file + contentExt);
    pageInfo.pagePath = Path(siteDir + pageNameAsPath.dir, pageNameAsPath.file + pageExt);

    pageInfo.pageTitle = pageTitle;
    pageInfo.templatePath = defaultTemplate;

    return pageInfo;
}

PageInfo SiteInfo::make_info(const Name &pageName, const Title &pageTitle, const Path &templatePath)
{
    PageInfo pageInfo;

    pageInfo.pageName = pageName;

    Path pageNameAsPath;
    pageNameAsPath.set_file_path_from(pageName);

    pageInfo.contentPath = Path(contentDir + pageNameAsPath.dir, pageNameAsPath.file + contentExt);
    pageInfo.pagePath = Path(siteDir + pageNameAsPath.dir, pageNameAsPath.file + pageExt);

    pageInfo.pageTitle = pageTitle;
    pageInfo.templatePath = templatePath;

    return pageInfo;
}

PageInfo SiteInfo::get_info(const Name &pageName)
{
    PageInfo page;
    page.pageName = pageName;
    return *pages.find(page);
}

int SiteInfo::info(const std::vector<Name> &pageNames)
{
    std::cout << std::endl;
    std::cout << "------ information on specified pages ------" << std::endl;
    for(auto pageName=pageNames.begin(); pageName!=pageNames.end(); pageName++)
    {
        if(pageName != pageNames.begin())
            std::cout << std::endl;

        PageInfo cPageInfo;
        cPageInfo.pageName = *pageName;
        if(pages.count(cPageInfo))
        {
            cPageInfo = *(pages.find(cPageInfo));
            std::cout << "   page title: " << cPageInfo.pageTitle << std::endl;
            std::cout << "    page path: " << cPageInfo.pagePath << std::endl;
            std::cout << " content path: " << cPageInfo.contentPath << std::endl;
            std::cout << "template path: " << cPageInfo.templatePath << std::endl;
        }
        else
            std::cout << "nsm is not tracking " << *pageName << std::endl;
    }
    std::cout << "--------------------------------------------" << std::endl;

    return 0;
}

int SiteInfo::info_all()
{
    std::cout << std::endl;
    std::cout << "--------- all tracked pages ---------" << std::endl;
    for(auto page=pages.begin(); page!=pages.end(); page++)
    {
        if(page != pages.begin())
            std::cout << std::endl;
        std::cout << "   page title: " << page->pageTitle << std::endl;
        std::cout << "    page path: " << page->pagePath << std::endl;
        std::cout << " content path: " << page->contentPath << std::endl;
        std::cout << "template path: " << page->templatePath << std::endl;
    }
    std::cout << "------------------------------------" << std::endl;

    return 0;
}

int SiteInfo::info_names()
{
    std::cout << std::endl;
    std::cout << "--------- all tracked page names ---------" << std::endl;
    for(auto page=pages.begin(); page!=pages.end(); page++)
        std::cout << page->pageName << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    return 0;
}

bool SiteInfo::tracking(const PageInfo &page)
{
    return pages.count(page);
}

bool SiteInfo::tracking(const Name &pageName)
{
    PageInfo page;
    page.pageName = pageName;
    return pages.count(page);
}

int SiteInfo::track(const Name &name, const Title &title, const Path &templatePath)
{
    PageInfo newPage = make_info(name, title, templatePath);

    if(newPage.contentPath == newPage.templatePath)
    {
        std::cout << std::endl;
        std::cout << "error: content and template paths cannot be the same, page not tracked" << std::endl;
        return 1;
    }

    if(pages.count(newPage) > 0)
    {
        PageInfo cInfo = *(pages.find(newPage));

        std::cout << std::endl;
        std::cout << "error: nsm is already tracking " << newPage.pagePath << std::endl;
        std::cout << "----- current page details -----" << std::endl;
        std::cout << "   page title: " << cInfo.pageTitle << std::endl;
        std::cout << "    page path: " << cInfo.pagePath << std::endl;
        std::cout << " content path: " << cInfo.contentPath << std::endl;
        std::cout << "template path: " << cInfo.templatePath << std::endl;
        std::cout << "--------------------------------" << std::endl;

        return 1;
    }

    //warns user if content and/or template paths don't exist
    if(!std::ifstream(newPage.contentPath.str()))
    {
        std::cout << std::endl;
        std::cout << "warning: content path " << newPage.contentPath << " did not exist" << std::endl;
        newPage.contentPath.ensurePathExists();
        chmod(newPage.contentPath.str().c_str(), 0666);
    }
    if(!std::ifstream(newPage.templatePath.str()))
    {
        std::cout << std::endl;
        std::cout << "warning: template path " << newPage.templatePath << " does not exist" << std::endl;
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
    std::cout << "successfully tracking " << newPage.pageName << std::endl;

    return 0;
}

int SiteInfo::untrack(const Name &pageNameToUntrack)
{
    //checks that page is being tracked
    if(!tracking(pageNameToUntrack))
    {
        std::cout << std::endl;
        std::cout << "error: nsm is not tracking " << pageNameToUntrack << std::endl;
        return 1;
    }

    PageInfo pageToErase = get_info(pageNameToUntrack);

    //removes page info file and containing dir if now empty
    chmod(pageToErase.pagePath.getInfoPath().str().c_str(), 0666);
    pageToErase.pagePath.getInfoPath().removePath();
    std::cout << "removed " << pageToErase.pagePath.getInfoPath().str() << std::endl;
    rmdir(pageToErase.pagePath.getInfoPath().dir.c_str());

    //removes page file and containing dir if now empty
    chmod(pageToErase.pagePath.str().c_str(), 0666);
    pageToErase.pagePath.removePath();
    std::cout << "removed " << pageToErase.pagePath.str() << std::endl;
    rmdir(pageToErase.pagePath.dir.c_str());

    //removes page from pages set
    pages.erase(pageToErase);

    //saves new set of pages to pages.list
    save();

    //informs user that page was successfully untracked
    std::cout << std::endl;
    std::cout << "successfully untracked " << pageNameToUntrack << std::endl;

    return 0;
}

int SiteInfo::rm(const Name &pageNameToRemove)
{
    //checks that page is being tracked
    if(!tracking(pageNameToRemove))
    {
        std::cout << std::endl;
        std::cout << "error: nsm is not tracking " << pageNameToRemove << std::endl;
        return 1;
    }

    PageInfo pageToErase = get_info(pageNameToRemove);

    //removes page info file and containing dir if now empty
    chmod(pageToErase.pagePath.getInfoPath().str().c_str(), 0666);
    pageToErase.pagePath.getInfoPath().removePath();
    std::cout << "removed " << pageToErase.pagePath.getInfoPath().str() << std::endl;
    rmdir(pageToErase.pagePath.getInfoPath().dir.c_str());

    //removes page file and containing dir if now empty
    chmod(pageToErase.pagePath.str().c_str(), 0666);
    pageToErase.pagePath.removePath();
    std::cout << "removed " << pageToErase.pagePath.str() << std::endl;
    rmdir(pageToErase.pagePath.dir.c_str());

    //removes content file and containing dir if now empty
    chmod(pageToErase.contentPath.str().c_str(), 0666);
    pageToErase.contentPath.removePath();
    std::cout << "removed " << pageToErase.contentPath.str() << std::endl;
    rmdir(pageToErase.contentPath.dir.c_str());

    //removes page from pages set
    pages.erase(pageToErase);

    //saves new set of pages to pages.list
    save();

    //informs user that page was successfully removed
    std::cout << std::endl;
    std::cout << "successfully removed " << pageNameToRemove << std::endl;

    return 0;
}

int SiteInfo::mv(const Name &oldPageName, const Name &newPageName)
{
    if(!tracking(oldPageName)) //checks old page is being tracked
    {
        std::cout << std::endl;
        std::cout << "error: nsm is not tracking " << oldPageName << std::endl;
        return 1;
    }
    else if(tracking(newPageName)) //checks new page isn't already tracked
    {
        std::cout << std::endl;
        std::cout << "error: nsm is already tracking " << newPageName << std::endl;
        return 1;
    }

    PageInfo oldPageInfo = get_info(oldPageName);

    PageInfo newPageInfo;
    newPageInfo.pageName = newPageName;
    newPageInfo.contentPath = Path(contentDir, newPageName + contentExt);
    newPageInfo.pagePath = Path(siteDir, newPageName + pageExt);
    if(oldPageInfo.pageName == oldPageInfo.pageTitle.str)
        newPageInfo.pageTitle = newPageName;
    else
        newPageInfo.pageTitle = oldPageInfo.pageTitle;
    newPageInfo.templatePath = oldPageInfo.templatePath;

    //moves content file
    std::ifstream ifs(oldPageInfo.contentPath.str());
    newPageInfo.contentPath.ensurePathExists();
    chmod(newPageInfo.contentPath.str().c_str(), 0666);
    std::ofstream ofs(newPageInfo.contentPath.str());
    std::string inLine;
    while(getline(ifs, inLine))
        ofs << inLine << std::endl;
    ofs.close();
    ifs.close();

    //removes old page info file and containing dir if now empty
    chmod(oldPageInfo.pagePath.getInfoPath().str().c_str(), 0666);
    oldPageInfo.pagePath.getInfoPath().removePath();
    std::cout << "removed " << oldPageInfo.pagePath.getInfoPath().str() << std::endl;
    rmdir(oldPageInfo.pagePath.getInfoPath().dir.c_str());

    //removes old page file and containing dir if now empty
    chmod(oldPageInfo.pagePath.str().c_str(), 0666);
    oldPageInfo.pagePath.removePath();
    std::cout << "removed " << oldPageInfo.pagePath.str() << std::endl;
    rmdir(oldPageInfo.pagePath.dir.c_str());

    //removes old content file and containing dir if now empty
    chmod(oldPageInfo.contentPath.str().c_str(), 0666);
    oldPageInfo.contentPath.removePath();
    std::cout << "removed " << oldPageInfo.contentPath.str() << std::endl;
    rmdir(oldPageInfo.contentPath.dir.c_str());

    //removes oldPageInfo from pages
    pages.erase(oldPageInfo);
    //adds newPageInfo to pages
    pages.insert(newPageInfo);

    //saves new set of pages to pages.list
    save();

    //informs user that page was successfully moved
    std::cout << std::endl;
    std::cout << "successfully moved " << oldPageName << " to " << newPageName << std::endl;

    return 0;
}

int SiteInfo::cp(const Name &trackedPageName, const Name &newPageName)
{
    if(!tracking(trackedPageName)) //checks old page is being tracked
    {
        std::cout << std::endl;
        std::cout << "error: nsm is not tracking " << trackedPageName << std::endl;
        return 1;
    }
    else if(tracking(newPageName)) //checks new page isn't already tracked
    {
        std::cout << std::endl;
        std::cout << "error: nsm is already tracking " << newPageName << std::endl;
        return 1;
    }

    PageInfo trackedPageInfo = get_info(trackedPageName);

    PageInfo newPageInfo;
    newPageInfo.pageName = newPageName;
    newPageInfo.contentPath = Path(contentDir, newPageName + contentExt);
    newPageInfo.pagePath = Path(siteDir, newPageName + pageExt);
    if(trackedPageInfo.pageName == trackedPageInfo.pageTitle.str)
        newPageInfo.pageTitle = newPageName;
    else
        newPageInfo.pageTitle = trackedPageInfo.pageTitle;
    newPageInfo.templatePath = trackedPageInfo.templatePath;

    //copies content file
    std::ifstream ifs(trackedPageInfo.contentPath.str());
    newPageInfo.contentPath.ensurePathExists();
    chmod(newPageInfo.contentPath.str().c_str(), 0666);
    std::ofstream ofs(newPageInfo.contentPath.str());
    std::string inLine;
    while(getline(ifs, inLine))
        ofs << inLine << std::endl;
    ofs.close();
    ifs.close();

    //adds newPageInfo to pages
    pages.insert(newPageInfo);

    //saves new set of pages to pages.list
    save();

    //informs user that page was successfully moved
    std::cout << std::endl;
    std::cout << "successfully copied " << trackedPageName << " to " << newPageName << std::endl;

    return 0;
}

int SiteInfo::new_title(const Name &pageName, const Title &newTitle)
{
    PageInfo pageInfo;
    pageInfo.pageName = pageName;
    if(pages.count(pageInfo))
    {
        pageInfo = *(pages.find(pageInfo));
        pages.erase(pageInfo);
        pageInfo.pageTitle = newTitle;
        pages.insert(pageInfo);
        save();

        //informs user that page title was successfully changed
        std::cout << std::endl;
        std::cout << "successfully changed page title to " << newTitle << std::endl;
    }
    else
    {
        std::cout << "error: nsm is not tracking " << pageName << std::endl;
        return 1;
    }

    return 0;
}

int SiteInfo::new_template(const Name &pageName, const Path &newTemplatePath)
{
    PageInfo pageInfo;
    pageInfo.pageName = pageName;
    if(pages.count(pageInfo))
    {
        pageInfo = *(pages.find(pageInfo));
        pages.erase(pageInfo);
        pageInfo.templatePath = newTemplatePath;
        pages.insert(pageInfo);
        save();

        //warns user if new template path doesn't exist
        if(!std::ifstream(newTemplatePath.str()))
        {
            std::cout << std::endl;
            std::cout << "warning: new template path " << newTemplatePath.str() << " does not exist" << std::endl;
        }

        //informs user that page title was successfully changed
        std::cout << std::endl;
        std::cout << "successfully changed template path to " << newTemplatePath.str() << std::endl;
    }
    else
    {
        std::cout << "error: nsm is not tracking " << pageName << std::endl;
        return 1;
    }

    return 0;
}


int SiteInfo::build(std::vector<Name> pageNamesToBuild)
{
    PageBuilder pageBuilder(pages);
    std::set<Name> untrackedPages, failedPages;

    for(auto pageName=pageNamesToBuild.begin(); pageName != pageNamesToBuild.end(); pageName++)
    {
        if(tracking(*pageName))
        {
            if(pageBuilder.build(get_info(*pageName)) > 0)
                failedPages.insert(*pageName);
        }
        else
            untrackedPages.insert(*pageName);
    }

    if(failedPages.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "---- following pages failed to build ----" << std::endl;
        for(auto fName=failedPages.begin(); fName != failedPages.end(); fName++)
            std::cout << " " << *fName << std::endl;
        std::cout << "-----------------------------------------" << std::endl;
    }
    if(untrackedPages.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "---- nsm not tracking following pages ----" << std::endl;
        for(auto uName=untrackedPages.begin(); uName != untrackedPages.end(); uName++)
            std::cout << *uName << std::endl;
        std::cout << "------------------------------------------" << std::endl;
    }
    if(failedPages.size() == 0 && untrackedPages.size() == 0)
    {
        std::cout << std::endl;
        std::cout << "all pages built successfully" << std::endl;
    }

    return 0;
}

int SiteInfo::build_all()
{
    PageBuilder pageBuilder(pages);

    if(pages.size() == 0)
    {
        std::cout << std::endl;
        std::cout << "nsm is not tracking any pages, nothing to build" << std::endl;
        return 0;
    }

    std::set<Path> failedPages;

    for(auto page=pages.begin(); page != pages.end(); page++)
        if(pageBuilder.build(*page) > 0)
            failedPages.insert(page->pagePath);

    if(failedPages.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "---- following pages failed to build ----" << std::endl;
        for(auto fPath=failedPages.begin(); fPath != failedPages.end(); fPath++)
            std::cout << " " << *fPath;
        std::cout << std::endl;
        std::cout << "-----------------------------------------" << std::endl;
    }
    else
    {
        std::cout << std::endl;
        std::cout << "all pages built successfully" << std::endl;
    }

    return 0;
}

int SiteInfo::build_updated()
{
    PageBuilder pageBuilder(pages);
    std::set<PageInfo> updatedPages;
    std::set<Path> modifiedFiles,
        removedFiles,
        problemPages,
        builtPages,
        failedPages;

    std::cout << std::endl;
    for(auto page=pages.begin(); page != pages.end(); page++)
    {
        //checks whether content and template files exist
        if(!std::ifstream(page->contentPath.str()))
        {
            std::cout << page->pagePath << ": content file " << page->contentPath << " does not exist" << std::endl;
            problemPages.insert(page->pagePath);
            continue;
        }
        if(!std::ifstream(page->templatePath.str()))
        {
            std::cout << page->pagePath << ": template file " << page->templatePath << " does not exist" << std::endl;
            problemPages.insert(page->pagePath);
            continue;
        }

        //gets path of pages information from last time page was built
        Path pageInfoPath = page->pagePath.getInfoPath();

        //checks whether info path exists
        if(!std::ifstream(pageInfoPath.str()))
        {
            std::cout << page->pagePath << ": yet to be built" << std::endl;
            updatedPages.insert(*page);
            continue;
        }
        else
        {
            std::ifstream infoStream(pageInfoPath.str());
            std::string timeDateLine;
            Name prevName;
            Title prevTitle;
            Path prevTemplatePath;

            getline(infoStream, timeDateLine);
            read_quoted(infoStream, prevName);
            prevTitle.read_quoted_from(infoStream);
            prevTemplatePath.read_file_path_from(infoStream);

            //probably don't even need this
            PageInfo prevPageInfo = make_info(prevName, prevTitle, prevTemplatePath);

            if(page->pageName != prevPageInfo.pageName)
            {
                std::cout << page->pagePath << ": page name changed to " << page->pageName << " from " << prevPageInfo.pageName << std::endl;
                updatedPages.insert(*page);
                continue;
            }

            if(page->pageTitle != prevPageInfo.pageTitle)
            {
                std::cout << page->pagePath << ": title changed to " << page->pageTitle << " from " << prevPageInfo.pageTitle << std::endl;
                updatedPages.insert(*page);
                continue;
            }

            if(page->templatePath != prevPageInfo.templatePath)
            {
                std::cout << page->pagePath << ": template path changed to " << page->templatePath << " from " << prevPageInfo.templatePath << std::endl;
                updatedPages.insert(*page);
                continue;
            }

            Path dep;
            while(dep.read_file_path_from(infoStream))
            {
                if(!std::ifstream(dep.str()))
                {
                    std::cout << page->pagePath << ": dep path " << dep << " removed since last build" << std::endl;
                    removedFiles.insert(dep);
                    updatedPages.insert(*page);
                    break;
                }
                else if(dep.modified_after(pageInfoPath))
                {
                    std::cout << page->pagePath << ": dep path " << dep << " modified since last build" << std::endl;
                    modifiedFiles.insert(dep);
                    updatedPages.insert(*page);
                    break;
                }
            }
        }
    }

    if(removedFiles.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "---- removed content files ----" << std::endl;
        for(auto rFile=removedFiles.begin(); rFile != removedFiles.end(); rFile++)
            std::cout << *rFile << std::endl;
        std::cout << "-------------------------------" << std::endl;
    }

    if(modifiedFiles.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "------- updated content files ------" << std::endl;
        for(auto uFile=modifiedFiles.begin(); uFile != modifiedFiles.end(); uFile++)
            std::cout << *uFile << std::endl;
        std::cout << "------------------------------------" << std::endl;
    }

    if(updatedPages.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "----- pages that need building -----" << std::endl;
        for(auto uPage=updatedPages.begin(); uPage != updatedPages.end(); uPage++)
            std::cout << uPage->pagePath << std::endl;
        std::cout << "------------------------------------" << std::endl;
    }

    if(problemPages.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "----- pages with missing content or template file -----" << std::endl;
        for(auto pPage=problemPages.begin(); pPage != problemPages.end(); pPage++)
            std::cout << *pPage << std::endl;
        std::cout << "-------------------------------------------------------" << std::endl;
    }

    for(auto uPage=updatedPages.begin(); uPage != updatedPages.end(); uPage++)
    {
        if(pageBuilder.build(*uPage) > 0)
            failedPages.insert(uPage->pagePath);
        else
            builtPages.insert(uPage->pagePath);
    }

    if(builtPages.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "----- pages successfully built -----" << std::endl;
        for(auto bPage=builtPages.begin(); bPage != builtPages.end(); bPage++)
            std::cout << *bPage << std::endl;
        std::cout << "------------------------------------" << std::endl;
    }

    if(failedPages.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "----- pages that failed to build -----" << std::endl;
        for(auto pPage=failedPages.begin(); pPage != failedPages.end(); pPage++)
            std::cout << *pPage << std::endl;
        std::cout << "--------------------------------------" << std::endl;
    }

    if(updatedPages.size() == 0 && problemPages.size() == 0 && failedPages.size() == 0)
    {
        //std::cout << std::endl;
        std::cout << "all pages are already up to date" << std::endl;
    }

    return 0;
}


int SiteInfo::status()
{
    if(pages.size() == 0)
    {
        std::cout << std::endl;
        std::cout << "nsm does not have any pages tracked" << std::endl;
        return 0;
    }

    std::set<Path> updatedFiles, removedFiles;
    std::set<PageInfo> updatedPages, problemPages;

    for(auto page=pages.begin(); page != pages.end(); page++)
    {
        bool needsUpdating = 0;

        //checks whether content and template files exist
        if(!std::ifstream(page->contentPath.str()))
        {
            needsUpdating = 1; //shouldn't need this but doesn't cost much
            problemPages.insert(*page);
            //note user will be informed about this as a dep not existing
        }
        if(!std::ifstream(page->templatePath.str()))
        {
            needsUpdating = 1; //shouldn't need this but doesn't cost much
            problemPages.insert(*page);
            //note user will be informed about this as a dep not existing
        }

        //gets path of pages information from last time page was built
        Path pageInfoPath = page->pagePath.getInfoPath();

        //checks whether info path exists
        if(!std::ifstream(pageInfoPath.str()))
        {
            std::cout << page->pagePath << ": yet to be built" << std::endl;
            needsUpdating = 1;
        }
        else
        {
            std::ifstream infoStream(pageInfoPath.str());
            std::string timeDateLine;
            Name prevName;
            Title prevTitle;
            Path prevTemplatePath;

            getline(infoStream, timeDateLine);
            read_quoted(infoStream, prevName);
            prevTitle.read_quoted_from(infoStream);
            prevTemplatePath.read_file_path_from(infoStream);

            //probably don't even need this
            PageInfo prevPageInfo = make_info(prevName, prevTitle, prevTemplatePath);

            if(page->pageName != prevPageInfo.pageName)
            {
                std::cout << page->pagePath << ": page name changed to " << page->pageName << " from " << prevPageInfo.pageName << std::endl;
                needsUpdating = 1;
            }

            if(page->pageTitle != prevPageInfo.pageTitle)
            {
                std::cout << page->pagePath << ": title changed to " << page->pageTitle << " from " << prevPageInfo.pageTitle << std::endl;
                needsUpdating = 1;
            }

            if(page->templatePath != prevPageInfo.templatePath)
            {
                std::cout << page->pagePath << ": template path changed to " << page->templatePath << " from " << prevPageInfo.templatePath << std::endl;
                needsUpdating = 1;
            }

            Path dep;
            while(dep.read_file_path_from(infoStream))
            {
                if(!std::ifstream(dep.str()))
                {
                    removedFiles.insert(dep);
                    needsUpdating = 1;
                }
                else if(dep.modified_after(pageInfoPath))
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
            std::cout << uPage->pagePath << std::endl;
        std::cout << "------------------------------------" << std::endl;
    }

    if(problemPages.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "----- pages with missing content or template file -----" << std::endl;
        for(auto uPage=problemPages.begin(); uPage != problemPages.end(); uPage++)
            std::cout << uPage->pagePath << std::endl;
        std::cout << "-------------------------------------------------------" << std::endl;
    }

    if(updatedFiles.size() == 0 && updatedPages.size() == 0 && problemPages.size() == 0)
    {
        std::cout << std::endl;
        std::cout << "all pages are already up to date" << std::endl;
    }

    return 0;
}
