#include "SiteInfo.h"

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
    int trash = chdir(dir.c_str());
    trash = trash + 1; //gets rid of stupid warning

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

int SiteInfo::open()
{
    pages.clear();

    if(!std::ifstream(".siteinfo/nsm.config"))
    {
        //this should never happen!
        std::cout << "ERROR: SiteInfo.h: could not open Nift config file as .siteinfo/nsm.config does not exist" << std::endl;
        return 1;
    }

    if(!std::ifstream(".siteinfo/pages.list"))
    {
        //this should never happen!
        std::cout << "ERROR: SiteInfo.h: could not open site information as .siteinfo/pages.list does not exist" << std::endl;
        return 1;
    }

    //reads Nift config file
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

    if(contentExt == "" || contentExt[0] != '.')
    {
        std::cout << "error: content extension must start with a fullstop" << std::endl;
        return 1;
    }

    if(pageExt == "" || pageExt[0] != '.')
    {
        std::cout << "error: page extension must start with a fullstop" << std::endl;
        return 1;
    }

    //reads page list file
    ifs.open(".siteinfo/pages.list");
    Name inName;
    Title inTitle;
    Path inTemplatePath;
    std::ifstream ifsx;
    std::string inExt;
    while(read_quoted(ifs, inName))
    {
        inTitle.read_quoted_from(ifs);
        inTemplatePath.read_file_path_from(ifs);

        PageInfo inPage = make_info(inName, inTitle, inTemplatePath);

        //checks for non-default content extension
        Path extPath = inPage.pagePath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";

        if(std::ifstream(extPath.str()))
        {
            ifsx.open(extPath.str());

            ifsx >> inExt;
            inPage.contentPath.file = inPage.contentPath.file.substr(0, inPage.contentPath.file.find_first_of('.')) + inExt;

            ifsx.close();
        }

        //checks for non-default page extension
        extPath = inPage.pagePath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".pageExt";

        if(std::ifstream(extPath.str()))
        {
            ifsx.open(extPath.str());

            ifsx >> inExt;
            inPage.pagePath.file = inPage.pagePath.file.substr(0, inPage.pagePath.file.find_first_of('.')) + inExt;

            ifsx.close();
        }

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

    ifs.close();

    return 0;
}

int SiteInfo::save()
{
    std::ofstream ofs(".siteinfo/pages.list");

    for(auto page=pages.begin(); page!=pages.end(); page++)
        ofs << *page << "\n\n";

    ofs.close();

    return 0;
}

PageInfo SiteInfo::make_info(const Name &pageName)
{
    PageInfo pageInfo;

    pageInfo.pageName = pageName;

    Path pageNameAsPath;
    pageNameAsPath.set_file_path_from(unquote(pageName));

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
    pageNameAsPath.set_file_path_from(unquote(pageName));

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
    pageNameAsPath.set_file_path_from(unquote(pageName));

    pageInfo.contentPath = Path(contentDir + pageNameAsPath.dir, pageNameAsPath.file + contentExt);
    pageInfo.pagePath = Path(siteDir + pageNameAsPath.dir, pageNameAsPath.file + pageExt);

    pageInfo.pageTitle = pageTitle;
    pageInfo.templatePath = templatePath;

    return pageInfo;
}

PageInfo make_info(const Name &pageName, const Title &pageTitle, const Path &templatePath, const Directory& contentDir, const Directory& siteDir, const std::string& contentExt, const std::string& pageExt)
{
    PageInfo pageInfo;

    pageInfo.pageName = pageName;

    Path pageNameAsPath;
    pageNameAsPath.set_file_path_from(unquote(pageName));

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
            std::cout << "Nift is not tracking " << *pageName << std::endl;
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
        std::cout << "    page name: " << page->pageName << std::endl;
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
        std::cout << "error: Nift is already tracking " << newPage.pagePath << std::endl;
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
        ofs << *page << "\n\n";
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
        std::cout << "error: Nift is not tracking " << pageNameToUntrack << std::endl;
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
        std::cout << "error: Nift is not tracking " << pageNameToRemove << std::endl;
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
        std::cout << "error: Nift is not tracking " << oldPageName << std::endl;
        return 1;
    }
    else if(tracking(newPageName)) //checks new page isn't already tracked
    {
        std::cout << std::endl;
        std::cout << "error: Nift is already tracking " << newPageName << std::endl;
        return 1;
    }

    PageInfo oldPageInfo = get_info(oldPageName);

    PageInfo newPageInfo;
    newPageInfo.pageName = newPageName;
    newPageInfo.contentPath.set_file_path_from(contentDir + newPageName + contentExt);
    newPageInfo.pagePath.set_file_path_from(siteDir + newPageName + pageExt);
    if(get_title(oldPageInfo.pageName) == oldPageInfo.pageTitle.str)
        newPageInfo.pageTitle = get_title(newPageName);
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
        ofs << inLine << "\n";
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
        std::cout << "error: Nift is not tracking " << trackedPageName << std::endl;
        return 1;
    }
    else if(tracking(newPageName)) //checks new page isn't already tracked
    {
        std::cout << std::endl;
        std::cout << "error: Nift is already tracking " << newPageName << std::endl;
        return 1;
    }

    PageInfo trackedPageInfo = get_info(trackedPageName);

    PageInfo newPageInfo;
    newPageInfo.pageName = newPageName;
    newPageInfo.contentPath.set_file_path_from(contentDir + newPageName + contentExt);
    newPageInfo.pagePath.set_file_path_from(siteDir + newPageName + pageExt);
    if(get_title(trackedPageInfo.pageName) == trackedPageInfo.pageTitle.str)
        newPageInfo.pageTitle = get_title(newPageName);
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
        ofs << inLine << "\n";
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
        std::cout << "successfully changed page title to " << newTitle << std::endl;
    }
    else
    {
        std::cout << "error: Nift is not tracking " << pageName << std::endl;
        return 1;
    }

    return 0;
}

int SiteInfo::new_template(const Path &newTemplatePath)
{
    Name pageName;
    Title pageTitle;
    Path pageTemplatePath;
    rename(".siteinfo/pages.list", ".siteinfo/pages.list-old");
    std::ifstream ifs(".siteinfo/pages.list-old");
    std::ofstream ofs(".siteinfo/pages.list");
    while(read_quoted(ifs, pageName))
    {
        pageTitle.read_quoted_from(ifs);
        pageTemplatePath.read_file_path_from(ifs);

        ofs << quote(pageName) << std::endl;
        ofs << pageTitle << std::endl;
        if(pageTemplatePath == defaultTemplate)
            ofs << newTemplatePath << std::endl << std::endl;
        else
            ofs << pageTemplatePath << std::endl << std::endl;
    }
    ifs.close();
    ofs.close();
    Path(".siteinfo/", "pages.list-old").removePath();

    //sets new template
    defaultTemplate = newTemplatePath;

    //saves new default template to Nift config file
    ofs.open(".siteinfo/nsm.config");
    ofs << "contentDir " << quote(contentDir) << "\n";
    ofs << "contentExt " << quote(contentExt) << "\n";
    ofs << "siteDir " << quote(siteDir) << "\n";
    ofs << "pageExt " << quote(pageExt) << "\n";
    ofs << "defaultTemplate " << defaultTemplate << "\n";
    ofs.close();

    //warns user if new template path doesn't exist
    if(!std::ifstream(newTemplatePath.str()))
        std::cout << "warning: new template path " << newTemplatePath << " does not exist" << std::endl;

    std::cout << "successfully changed default template to " << newTemplatePath << std::endl;

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
            std::cout << "warning: new template path " << newTemplatePath << " does not exist" << std::endl;
    }
    else
    {
        std::cout << "error: Nift is not tracking " << pageName << std::endl;
        return 1;
    }

    return 0;
}

int SiteInfo::new_site_dir(const Directory &newSiteDir)
{
    if(newSiteDir == "")
    {
        std::cout << "error: new site directory cannot be the empty string" << std::endl;
        return 1;
    }

    if(newSiteDir == siteDir)
    {
        std::cout << "error: site directory is already " << quote(newSiteDir) << std::endl;
        return 1;
    }

    if(std::ifstream(newSiteDir))
    {
        std::cout << "error: new site directory " << quote(newSiteDir) << " already exists" << std::endl;
        return 1;
    }

    std::string parDir = "../",
                rootDir = "/",
                siteRootDir = get_pwd(),
                pwd = get_pwd();

    int trash = chdir(siteDir.c_str());
    trash = trash + 1; //gets rid of stupid warning
    trash = chdir(parDir.c_str());
    std::string delDir = get_pwd();
    trash = chdir(siteRootDir.c_str());

    //ensures new site directory exists
    Path(newSiteDir, Filename("")).ensurePathExists();

    //moves site directory
    rename(siteDir.c_str(), newSiteDir.c_str());

    //deletes any remaining empty directories
    trash = chdir(delDir.c_str());
    size_t pos;
    while(ls("./") == ". .. ")
    {
        pwd = get_pwd();
        trash = chdir(parDir.c_str());
        pos = pwd.find_last_of('/');
        trash = rmdir(pwd.substr(pos+1, pwd.size()-(pos+1)).c_str());
    }

    //changes back to site root directory
    trash = chdir(siteRootDir.c_str());

    //ensures new info directory exists
    Path(".siteinfo/" + newSiteDir, Filename("")).ensurePathExists();

    //moves old info directory
    rename((".siteinfo/" + siteDir).c_str(), (".siteinfo/" + newSiteDir).c_str());

    //deletes any remaining empty directories
    trash = chdir((".siteinfo/" + delDir).c_str());
    while(ls("./") == ". .. ")
    {
        pwd = get_pwd();
        trash = chdir(parDir.c_str());
        pos = pwd.find_last_of('/');
        trash = rmdir(pwd.substr(pos+1, pwd.size()-(pos+1)).c_str());
    }

    //changes back to site root directory
    trash = chdir(siteRootDir.c_str());

    //sets new site directory
    siteDir = newSiteDir;

    //saves new site directory to Nift config file
    std::ofstream ofs(".siteinfo/nsm.config");
    ofs << "contentDir " << quote(contentDir) << "\n";
    ofs << "contentExt " << quote(contentExt) << "\n";
    ofs << "siteDir " << quote(siteDir) << "\n";
    ofs << "pageExt " << quote(pageExt) << "\n";
    ofs << "defaultTemplate " << defaultTemplate << "\n";
    ofs.close();

    std::cout << "successfully changed site directory to " << quote(newSiteDir) << std::endl;

    return 0;
}

int SiteInfo::new_content_dir(const Directory &newContDir)
{
    if(newContDir == "")
    {
        std::cout << "error: new content directory cannot be the empty string" << std::endl;
        return 1;
    }

    if(newContDir == contentDir)
    {
        std::cout << "error: content directory is already " << quote(newContDir) << std::endl;
        return 1;
    }

    if(std::ifstream(newContDir))
    {
        std::cout << "error: new content directory " << quote(newContDir) << " already exists" << std::endl;
        return 1;
    }

    std::string parDir = "../",
                rootDir = "/",
                siteRootDir = get_pwd(),
                pwd = get_pwd();

    int trash = chdir(contentDir.c_str());
    trash = trash + 1; //gets rid of stupid warning
    trash = chdir(parDir.c_str());
    std::string delDir = get_pwd();
    trash = chdir(siteRootDir.c_str());

    //ensures new site directory exists
    Path(newContDir, Filename("")).ensurePathExists();

    //moves content directory
    rename(contentDir.c_str(), newContDir.c_str());

    //deletes any remaining empty directories
    trash = chdir(delDir.c_str());
    size_t pos;
    while(ls("./") == ". .. ")
    {
        pwd = get_pwd();
        trash = chdir(parDir.c_str());
        pos = pwd.find_last_of('/');
        trash = rmdir(pwd.substr(pos+1, pwd.size()-(pos+1)).c_str());
    }

    //changes back to site root directory
    trash = chdir(siteRootDir.c_str());

    //sets new site directory
    contentDir = newContDir;

    //saves new site directory to Nift config file
    std::ofstream ofs(".siteinfo/nsm.config");
    ofs << "contentDir " << quote(contentDir) << "\n";
    ofs << "contentExt " << quote(contentExt) << "\n";
    ofs << "siteDir " << quote(siteDir) << "\n";
    ofs << "pageExt " << quote(pageExt) << "\n";
    ofs << "defaultTemplate " << defaultTemplate << "\n";
    ofs.close();

    std::cout << "successfully changed content directory to " << quote(newContDir) << std::endl;

    return 0;
}

int SiteInfo::new_content_ext(const std::string &newExt)
{
    if(newExt == "" || newExt[0] != '.')
    {
        std::cout << "error: page extension must start with a fullstop" << std::endl;
        return 1;
    }

    contentExt = newExt;

    std::ofstream ofs(".siteinfo/nsm.config");
    ofs << "contentDir " << quote(contentDir) << "\n";
    ofs << "contentExt " << quote(contentExt) << "\n";
    ofs << "siteDir " << quote(siteDir) << "\n";
    ofs << "pageExt " << quote(pageExt) << "\n";
    ofs << "defaultTemplate " << defaultTemplate << "\n";
    ofs.close();

    for(auto page=pages.begin(); page!=pages.end(); page++)
    {
        //checks for non-default page extension
        Path extPath = page->pagePath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";

        if(std::ifstream(extPath.str()))
        {
            std::ifstream ifs(extPath.str());
            std::string oldExt;

            read_quoted(ifs, oldExt);
            ifs.close();

            if(oldExt != newExt)
                continue;
        }

        new_content_ext(page->pageName, newExt);
    }

    //informs user that page extension was successfully changed
    std::cout << "successfully changed page extention to " << newExt << std::endl;

    return 0;
}

int SiteInfo::new_content_ext(const Name &pageName, const std::string &newExt)
{
    if(newExt == "" || newExt[0] != '.')
    {
        std::cout << "error: content extension must start with a fullstop" << std::endl;
        return 1;
    }

    PageInfo pageInfo = get_info(pageName);
    if(pages.count(pageInfo))
    {
        //checks for non-default page extension
        Path extPath = pageInfo.pagePath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";

        //makes sure we can write to ext file
        chmod(extPath.str().c_str(), 0644);

        if(newExt != contentExt)
        {
            std::ofstream ofs(extPath.str());
            ofs << newExt << "\n";
            ofs.close();

            //makes sure user can't edit ext file
            chmod(extPath.str().c_str(), 0444);
        }
        else
            extPath.removePath();

        //moves the content file
        if(std::ifstream(pageInfo.pagePath.str()))
        {
            Path newContPath = pageInfo.contentPath;
            newContPath.file = newContPath.file.substr(0, newContPath.file.find_first_of('.')) + newExt;
            if(newContPath.str() != pageInfo.contentPath.str())
            {
                std::ifstream ifs(pageInfo.contentPath.str());
                std::ofstream ofs(newContPath.str());
                std::string str;
                while(getline(ifs, str))
                    ofs << str << "\n";
                ofs.close();
                ifs.close();
                pageInfo.contentPath.removePath();
            }
        }
    }
    else
    {
        std::cout << "error: Nift is not tracking " << pageName << std::endl;
        return 1;
    }

    return 0;
}

int SiteInfo::new_page_ext(const std::string &newExt)
{
    if(newExt == "" || newExt[0] != '.')
    {
        std::cout << "error: page extension must start with a fullstop" << std::endl;
        return 1;
    }

    pageExt = newExt;

    std::ofstream ofs(".siteinfo/nsm.config");
    ofs << "contentDir " << quote(contentDir) << "\n";
    ofs << "contentExt " << quote(contentExt) << "\n";
    ofs << "siteDir " << quote(siteDir) << "\n";
    ofs << "pageExt " << quote(pageExt) << "\n";
    ofs << "defaultTemplate " << defaultTemplate << "\n";
    ofs.close();

    for(auto page=pages.begin(); page!=pages.end(); page++)
    {
        //checks for non-default page extension
        Path extPath = page->pagePath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".pageExt";

        if(std::ifstream(extPath.str()))
        {
            std::ifstream ifs(extPath.str());
            std::string oldExt;

            read_quoted(ifs, oldExt);
            ifs.close();

            if(oldExt != newExt)
                continue;
        }

        new_page_ext(page->pageName, newExt);
    }

    //informs user that page extension was successfully changed
    std::cout << "successfully changed page extention to " << newExt << std::endl;

    return 0;
}

int SiteInfo::new_page_ext(const Name &pageName, const std::string &newExt)
{
    if(newExt == "" || newExt[0] != '.')
    {
        std::cout << "error: page extension must start with a fullstop" << std::endl;
        return 1;
    }

    PageInfo pageInfo = get_info(pageName);
    if(pages.count(pageInfo))
    {
        //checks for non-default page extension
        Path extPath = pageInfo.pagePath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".pageExt";

        //makes sure we can write to ext file
        chmod(extPath.str().c_str(), 0644);

        if(newExt != pageExt)
        {
            std::ofstream ofs(extPath.str());
            ofs << newExt << "\n";
            ofs.close();

            //makes sure user can't edit ext file
            chmod(extPath.str().c_str(), 0444);
        }
        else
            extPath.removePath();

        //moves the built page
        if(std::ifstream(pageInfo.pagePath.str()))
        {
            Path newPagePath = pageInfo.pagePath;
            newPagePath.file = newPagePath.file.substr(0, newPagePath.file.find_first_of('.')) + newExt;
            if(newPagePath.str() != pageInfo.pagePath.str())
            {
                std::ifstream ifs(pageInfo.pagePath.str());
                std::ofstream ofs(newPagePath.str());
                std::string str;
                while(getline(ifs, str))
                    ofs << str << "\n";
                ofs.close();
                ifs.close();
                pageInfo.pagePath.removePath();
            }
        }
    }
    else
    {
        std::cout << "error: Nift is not tracking " << pageName << std::endl;
        return 1;
    }

    return 0;
}

int SiteInfo::build(const std::vector<Name>& pageNamesToBuild)
{
    PageBuilder pageBuilder(&pages);
    std::set<Name> untrackedPages, failedPages;

    for(auto pageName=pageNamesToBuild.begin(); pageName != pageNamesToBuild.end(); pageName++)
    {
        if(tracking(*pageName))
        {
            if(pageBuilder.build(get_info(*pageName), std::cout) > 0)
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
        std::cout << "---- Nift not tracking following pages ----" << std::endl;
        for(auto uName=untrackedPages.begin(); uName != untrackedPages.end(); uName++)
            std::cout << " " << *uName << std::endl;
        std::cout << "-------------------------------------------" << std::endl;
    }
    if(failedPages.size() == 0 && untrackedPages.size() == 0)
    {
        std::cout << std::endl;
        std::cout << "all pages built successfully" << std::endl;
    }

    return 0;
}

std::mutex fail_mtx, built_mtx, set_mtx;
std::set<Name> failedPages, builtPages;
std::set<PageInfo>::iterator cPage;

std::atomic<int> counter;

void build_thread(std::ostream& os, std::set<PageInfo>* pages, const int& no_pages)
{
    PageBuilder pageBuilder(pages);
    std::set<PageInfo>::iterator pageInfo;

    while(counter < no_pages)
    {
        set_mtx.lock();
        if(counter >= no_pages)
        {
            set_mtx.unlock();
            return;
        }
        counter++;
        pageInfo = cPage++;
        set_mtx.unlock();

        if(pageBuilder.build(*pageInfo, os) > 0)
        {
            fail_mtx.lock();
            failedPages.insert(pageInfo->pageName);
            fail_mtx.unlock();
        }
		else
		{
			built_mtx.lock();
			builtPages.insert(pageInfo->pageName);
			built_mtx.unlock();
		}
    }
}

int SiteInfo::build_all()
{
    int no_threads = std::thread::hardware_concurrency();

    std::set<Name> untrackedPages;

    cPage = pages.begin();
    counter = 0;

	std::vector<std::thread> threads;
	for(int i=0; i<no_threads; i++)
		threads.push_back(std::thread(build_thread, std::ref(std::cout), &pages, pages.size()));

	for(int i=0; i<no_threads; i++)
		threads[i].join();

    if(failedPages.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "---- following pages failed to build ----" << std::endl;
        if(failedPages.size() < 20)
            for(auto fName=failedPages.begin(); fName != failedPages.end(); fName++)
                std::cout << " " << *fName << std::endl;
        else
        {
            int x=0;
            for(auto fName=failedPages.begin(); x < 20; fName++, x++)
                std::cout << " " << *fName << std::endl;
            std::cout << " along with " << failedPages.size() - 20 << " other pages" << std::endl;
        }
        std::cout << "-----------------------------------------" << std::endl;
    }
    if(untrackedPages.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "---- Nift not tracking following pages ----" << std::endl;
        if(untrackedPages.size() < 20)
            for(auto uName=untrackedPages.begin(); uName != untrackedPages.end(); uName++)
                std::cout << " " << *uName << std::endl;
        else
        {
            int x=0;
            for(auto uName=untrackedPages.begin(); x < 20; uName++, x++)
                std::cout << " " << *uName << std::endl;
            std::cout << " along with " << untrackedPages.size() - 20 << " other pages" << std::endl;
        }
        std::cout << "-------------------------------------------" << std::endl;
    }
    if(failedPages.size() == 0 && untrackedPages.size() == 0)
        std::cout << "all " << builtPages.size() << " pages built successfully" << std::endl;

    return 0;
}

/*int SiteInfo::build_all_old()
{
    std::set<Name> untrackedPages;
    std::set<PageInfo> pages1, pages2, pages3, pages4;

    int s = pages.size(),
        n1 = pages.size()/4,
        n2 = 2*n1,
        n3 = 3*n1;

    int i = 0;
    for(auto page=pages.begin(); i < s; ++page, ++i)
    {
        if(!tracking(page->pageName))
            untrackedPages.insert(page->pageName);
        else if(i < n1)
            pages1.insert(*page);
        else if(i < n2)
            pages2.insert(*page);
        else if(i < n3)
            pages3.insert(*page);
        else
            pages4.insert(*page);
    }

    std::thread thread1(build_thread, pages, pages1),
                thread2(build_thread, pages, pages2),
                thread3(build_thread, pages, pages3),
                thread4(build_thread, pages, pages4);

    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();

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
        std::cout << "---- Nift not tracking following pages ----" << std::endl;
        for(auto uName=untrackedPages.begin(); uName != untrackedPages.end(); uName++)
            std::cout << " " << *uName << std::endl;
        std::cout << "-------------------------------------------" << std::endl;
    }
    if(failedPages.size() == 0 && untrackedPages.size() == 0)
    {
        std::cout << std::endl;
        std::cout << "all " << pages.size() << " pages built successfully" << std::endl;
    }

    return 0;
}*/

/*int SiteInfo::build_all_oldest()
{
    PageBuilder pageBuilder(pages);

    if(pages.size() == 0)
    {
        std::cout << std::endl;
        std::cout << "Nift is not tracking any pages, nothing to build" << std::endl;
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
            std::cout << " " << *fPath << std::endl;
        std::cout << "-----------------------------------------" << std::endl;
    }
    else
    {
        std::cout << std::endl;
        std::cout << "all pages built successfully" << std::endl;
    }

    return 0;
}*/

std::mutex problem_mtx, updated_mtx, modified_mtx, removed_mtx, os_mtx;
std::set<PageInfo> updatedPages;
std::set<Path> modifiedFiles,
    removedFiles,
    problemPages;

void dep_thread(std::ostream& os, const int& no_pages, const Directory& contentDir, const Directory& siteDir, const std::string& contentExt, const std::string& pageExt)
{
    std::set<PageInfo>::iterator page;

    while(counter < no_pages)
    {
        set_mtx.lock();
        if(counter >= no_pages)
        {
            set_mtx.unlock();
            return;
        }
        counter++;
        page = cPage++;
        set_mtx.unlock();

        //checks whether content and template files exist
        if(!std::ifstream(page->contentPath.str()))
        {
            os_mtx.lock();
            os << page->pagePath << ": content file " << page->contentPath << " does not exist" << std::endl;
            os_mtx.unlock();
            problem_mtx.lock();
            problemPages.insert(page->pagePath);
            problem_mtx.unlock();
            continue;
        }
        if(!std::ifstream(page->templatePath.str()))
        {
            os_mtx.lock();
            os << page->pagePath << ": template file " << page->templatePath << " does not exist" << std::endl;
            os_mtx.unlock();
            problem_mtx.lock();
            problemPages.insert(page->pagePath);
            problem_mtx.unlock();
            continue;
        }

        //gets path of pages information from last time page was built
        Path pageInfoPath = page->pagePath.getInfoPath();

        //checks whether info path exists
        if(!std::ifstream(pageInfoPath.str()))
        {
            os_mtx.lock();
            os << page->pagePath << ": yet to be built" << std::endl;
            os_mtx.unlock();
            updated_mtx.lock();
            updatedPages.insert(*page);
            updated_mtx.unlock();
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

            PageInfo prevPageInfo = make_info(prevName, prevTitle, prevTemplatePath, contentDir, siteDir, contentExt, pageExt);

            //note we haven't checked for non-default content/page extension, pretty sure we don't need to here

            if(page->pageName != prevPageInfo.pageName)
            {
                os_mtx.lock();
                os << page->pagePath << ": page name changed to " << page->pageName << " from " << prevPageInfo.pageName << std::endl;
                os_mtx.unlock();
                updated_mtx.lock();
                updatedPages.insert(*page);
                updated_mtx.unlock();
                continue;
            }

            if(page->pageTitle != prevPageInfo.pageTitle)
            {
                os_mtx.lock();
                os << page->pagePath << ": title changed to " << page->pageTitle << " from " << prevPageInfo.pageTitle << std::endl;
                os_mtx.unlock();
                updated_mtx.lock();
                updatedPages.insert(*page);
                updated_mtx.unlock();
                continue;
            }

            if(page->templatePath != prevPageInfo.templatePath)
            {
                os_mtx.lock();
                os << page->pagePath << ": template path changed to " << page->templatePath << " from " << prevPageInfo.templatePath << std::endl;
                os_mtx.unlock();
                updated_mtx.lock();
                updatedPages.insert(*page);
                updated_mtx.unlock();
                continue;
            }

            Path dep;
            while(dep.read_file_path_from(infoStream))
            {
                if(!std::ifstream(dep.str()))
                {
                    os_mtx.lock();
                    os << page->pagePath << ": dep path " << dep << " removed since last build" << std::endl;
                    os_mtx.unlock();
                    removed_mtx.lock();
                    removedFiles.insert(dep);
                    removed_mtx.unlock();
                    updated_mtx.lock();
                    updatedPages.insert(*page);
                    updated_mtx.unlock();
                    break;
                }
                else if(dep.modified_after(pageInfoPath))
                {
                    os_mtx.lock();
                    os << page->pagePath << ": dep path " << dep << " modified since last build" << std::endl;
                    os_mtx.unlock();
                    modified_mtx.lock();
                    modifiedFiles.insert(dep);
                    modified_mtx.unlock();
                    updated_mtx.lock();
                    updatedPages.insert(*page);
                    updated_mtx.unlock();
                    break;
                }
            }

			infoStream.close();

            //checks for user-defined dependencies
            Path depsPath = page->contentPath;
            depsPath.file = depsPath.file.substr(0, depsPath.file.find_first_of('.')) + ".deps";

            if(std::ifstream(depsPath.str()))
            {
                std::ifstream depsFile(depsPath.str());
                while(dep.read_file_path_from(depsFile))
                {
                    if(!std::ifstream(dep.str()))
                    {
                        os_mtx.lock();
                        os << page->pagePath << ": user defined dep path " << dep << " does not exist" << std::endl;
                        os_mtx.unlock();
                        removed_mtx.lock();
                        removedFiles.insert(dep);
                        removed_mtx.unlock();
                        updated_mtx.lock();
                        updatedPages.insert(*page);
                        updated_mtx.unlock();
                        break;
                    }
                    else if(dep.modified_after(pageInfoPath))
                    {
                        os_mtx.lock();
                        os << page->pagePath << ": user defined dep path " << dep << " modified since last build" << std::endl;
                        os_mtx.unlock();
                        modified_mtx.lock();
                        modifiedFiles.insert(dep);
                        modified_mtx.unlock();
                        updated_mtx.lock();
                        updatedPages.insert(*page);
                        updated_mtx.unlock();
                        break;
                    }
                }
            }
        }
    }
}

int SiteInfo::build_updated(std::ostream& os)
{
    builtPages.clear();
    failedPages.clear();
    problemPages.clear();
    updatedPages.clear();
    modifiedFiles.clear();
    removedFiles.clear();

    int no_threads = std::thread::hardware_concurrency();

    cPage = pages.begin();
    counter = 0;

	std::vector<std::thread> threads;
	for(int i=0; i<no_threads; i++)
		threads.push_back(std::thread(dep_thread, std::ref(os), pages.size(), contentDir, siteDir, contentExt, pageExt));

	for(int i=0; i<no_threads; i++)
		threads[i].join();

    if(removedFiles.size() > 0)
    {
        os << std::endl;
        os << "---- removed dependency files ----" << std::endl;
        if(removedFiles.size() < 20)
            for(auto rFile=removedFiles.begin(); rFile != removedFiles.end(); rFile++)
                os << " " << *rFile << std::endl;
        else
        {
            int x=0;
            for(auto rFile=removedFiles.begin(); x < 20; rFile++, x++)
                os << " " << *rFile << std::endl;
            os << " along with " << removedFiles.size() - 20 << " other dependency files" << std::endl;
        }
        os << "----------------------------------" << std::endl;
    }

    if(modifiedFiles.size() > 0)
    {
        os << std::endl;
        os << "------- updated dependency files ------" << std::endl;
        if(modifiedFiles.size() < 20)
            for(auto uFile=modifiedFiles.begin(); uFile != modifiedFiles.end(); uFile++)
                os << " " << *uFile << std::endl;
        else
        {
            int x=0;
            for(auto uFile=modifiedFiles.begin(); x < 20; uFile++, x++)
                os << " " << *uFile << std::endl;
            os << " along with " << modifiedFiles.size() - 20 << " other dependency files" << std::endl;
        }
        os << "---------------------------------------" << std::endl;
    }

    if(updatedPages.size() > 0)
    {
        os << std::endl;
        os << "----- pages that need building -----" << std::endl;
        if(updatedPages.size() < 20)
            for(auto uPage=updatedPages.begin(); uPage != updatedPages.end(); uPage++)
                os << " " << uPage->pagePath << std::endl;
        else
        {
            int x=0;
            for(auto uPage=updatedPages.begin(); x < 20; uPage++, x++)
                os << " " << uPage->pagePath << std::endl;
            os << " along with " << updatedPages.size() - 20 << " other pages" << std::endl;
        }
        os << "------------------------------------" << std::endl;
    }

    if(problemPages.size() > 0)
    {
        os << std::endl;
        os << "----- pages with missing content or template file -----" << std::endl;
        if(problemPages.size() < 20)
            for(auto pPage=problemPages.begin(); pPage != problemPages.end(); pPage++)
                os << " " << *pPage << std::endl;
        else
        {
            int x=0;
            for(auto pPage=problemPages.begin(); x < 20; pPage++, x++)
                os << " " << *pPage << std::endl;
            os << " along with " << problemPages.size() - 20 << " other pages" << std::endl;
        }
        os << "-------------------------------------------------------" << std::endl;
    }

    cPage = updatedPages.begin();
    counter = 0;

	threads.clear();
	for(int i=0; i<no_threads; i++)
		threads.push_back(std::thread(build_thread, std::ref(os), &pages, updatedPages.size()));

	for(int i=0; i<no_threads; i++)
		threads[i].join();

    if(builtPages.size() > 0)
    {
        os << std::endl;
        os << "------- pages successfully built -------" << std::endl;
        if(builtPages.size() < 20)
            for(auto bName=builtPages.begin(); bName != builtPages.end(); bName++)
                os << " " << *bName << std::endl;
        else
        {
            int x=0;
            for(auto bName=builtPages.begin(); x < 20; bName++, x++)
                os << " " << *bName << std::endl;
            os << " along with " << builtPages.size() - 20 << " other pages" << std::endl;
        }
        os << "----------------------------------------" << std::endl;
    }

    if(failedPages.size() > 0)
    {
        os << std::endl;
        os << "---- following pages failed to build ----" << std::endl;
        if(failedPages.size() < 20)
            for(auto fName=failedPages.begin(); fName != failedPages.end(); fName++)
                os << " " << *fName << std::endl;
        else
        {
            int x=0;
            for(auto fName=failedPages.begin(); x < 20; fName++, x++)
                os << " " << *fName << std::endl;
            os << " along with " << failedPages.size() - 20 << " other pages" << std::endl;
        }
        os << "-----------------------------------------" << std::endl;
    }

    if(updatedPages.size() == 0 && problemPages.size() == 0 && failedPages.size() == 0)
    {
        //os << std::endl;
        os << "all pages are already up to date" << std::endl;
    }

	builtPages.clear();
	failedPages.clear();

    return 0;
}

/*int SiteInfo::build_updated_old()
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

			infoStream.close();
        }
    }

    if(removedFiles.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "---- removed content files ----" << std::endl;
        for(auto rFile=removedFiles.begin(); rFile != removedFiles.end(); rFile++)
            std::cout << " " << *rFile << std::endl;
        std::cout << "-------------------------------" << std::endl;
    }

    if(modifiedFiles.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "------- updated content files ------" << std::endl;
        for(auto uFile=modifiedFiles.begin(); uFile != modifiedFiles.end(); uFile++)
            std::cout << " " << *uFile << std::endl;
        std::cout << "------------------------------------" << std::endl;
    }

    if(updatedPages.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "----- pages that need building -----" << std::endl;
        for(auto uPage=updatedPages.begin(); uPage != updatedPages.end(); uPage++)
            std::cout << " " << uPage->pagePath << std::endl;
        std::cout << "------------------------------------" << std::endl;
    }

    if(problemPages.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "----- pages with missing content or template file -----" << std::endl;
        for(auto pPage=problemPages.begin(); pPage != problemPages.end(); pPage++)
            std::cout << " " << *pPage << std::endl;
        std::cout << "-------------------------------------------------------" << std::endl;
    }

    for(auto uPage=updatedPages.begin(); uPage != updatedPages.end(); uPage++)
    {
        if(pageBuilder.build(*uPage, std::cout) > 0)
            failedPages.insert(uPage->pagePath);
        else
            builtPages.insert(uPage->pagePath);
    }

    if(builtPages.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "----- pages successfully built -----" << std::endl;
        for(auto bPage=builtPages.begin(); bPage != builtPages.end(); bPage++)
            std::cout << " " << *bPage << std::endl;
        std::cout << "------------------------------------" << std::endl;
    }

    if(failedPages.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "----- pages that failed to build -----" << std::endl;
        for(auto pPage=failedPages.begin(); pPage != failedPages.end(); pPage++)
            std::cout << " " << *pPage << std::endl;
        std::cout << "--------------------------------------" << std::endl;
    }

    if(updatedPages.size() == 0 && problemPages.size() == 0 && failedPages.size() == 0)
    {
        //std::cout << std::endl;
        std::cout << "all pages are already up to date" << std::endl;
    }

    return 0;
}*/

int SiteInfo::status()
{
    if(pages.size() == 0)
    {
        std::cout << std::endl;
        std::cout << "Nift does not have any pages tracked" << std::endl;
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

			infoStream.close();
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
