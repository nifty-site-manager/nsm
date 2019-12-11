#include "SiteInfo.h"

int SiteInfo::open()
{
    if(open_config())
        return 1;
    if(open_pages())
        return 1;

    return 0;
}

int SiteInfo::open_config()
{
    if(!std::ifstream(".siteinfo/nsm.config"))
    {
        //this should never happen!
        std::cout << "error: SiteInfo.cpp: open_config(): could not open Nift config file as '" << get_pwd() <<  "/.siteinfo/nsm.config' does not exist" << std::endl;
        return 1;
    }

    contentDir = siteDir = "";
    backupScripts = 1;
    buildThreads = 0;
    contentExt = pageExt = scriptExt = unixTextEditor = winTextEditor = rootBranch = siteBranch = "";
    defaultTemplate = Path("", "");

    //reads Nift config file
    std::ifstream ifs(".siteinfo/nsm.config");
    std::string inLine, inType, str = "";
    int lineNo = 0;
    while(getline(ifs, inLine))
    {
        lineNo++;

        if(!is_whitespace(inLine) && inLine.size() && inLine[0] != '#')
        {
            std::istringstream iss(inLine);

            iss >> inType;

            if(inType == "contentDir")
            {
                read_quoted(iss, contentDir);
                contentDir = comparable(contentDir);
            }
            else if(inType == "contentExt")
                read_quoted(iss, contentExt);
            else if(inType == "siteDir")
            {
                read_quoted(iss, siteDir);
                siteDir = comparable(siteDir);
            }
            else if(inType == "pageExt")
                read_quoted(iss, pageExt);
            else if(inType == "scriptExt")
                read_quoted(iss, scriptExt);
            else if(inType == "defaultTemplate")
                defaultTemplate.read_file_path_from(iss);
            else if(inType == "backupScripts")
                iss >> backupScripts;
            else if(inType == "buildThreads")
                iss >> buildThreads;
            else if(inType == "unixTextEditor")
                read_quoted(iss, unixTextEditor);
            else if(inType == "winTextEditor")
                read_quoted(iss, winTextEditor);
            else if(inType == "rootBranch")
                read_quoted(iss, rootBranch);
            else if(inType == "siteBranch")
                read_quoted(iss, siteBranch);
            else
            {
                continue;
                //if we throw error here can't compile sites for newer versions with older versions of Nift
                //std::cout << "error: .siteinfo/nsm.config: line " << lineNo << ": do not recognise confirguration parameter " << inType << std::endl;
                //return 1;
            }

            iss >> str;
            if(str != "" && str[0] != '#')
            {
                std::cout << "error: .siteinfo/nsm.config: line " << lineNo << ": was not expecting anything on this line from '" << unquote(str) << "' onwards" << std::endl;
                std::cout << "note: you can comment out the remainder of a line with #" << std::endl;
                return 1;
            }
        }
    }
    ifs.close();

    if(contentExt == "" || contentExt[0] != '.')
    {
        std::cout << "error: .siteinfo/nsm.config: content extension must start with a fullstop" << std::endl;
        return 1;
    }

    if(pageExt == "" || pageExt[0] != '.')
    {
        std::cout << "error: .siteinfo/nsm.config: page extension must start with a fullstop" << std::endl;
        return 1;
    }

    if(scriptExt != "" && scriptExt [0] != '.')
    {
        std::cout << "error: .siteinfo/nsm.config: script extension must start with a fullstop" << std::endl;
        return 1;
    }

    if(defaultTemplate == Path("", ""))
    {
        std::cout << "error: .siteinfo/nsm.config: default template is not specified" << std::endl;
        return 1;
    }

    if(contentDir == "")
    {
        std::cout << "error: .siteinfo/nsm.config: content directory not specified" << std::endl;
        return 1;
    }

    if(siteDir == "")
    {
        std::cout << "error: .siteinfo/nsm.config: site directory not specified" << std::endl;
        return 1;
    }

    bool configChanged = 0;

    if(scriptExt == "")
    {
        std::cout << "warning: .siteinfo/nsm.config: script extension not detected, set to default of '.py'" << std::endl;

        scriptExt = ".py";

        configChanged = 1;
    }

    if(buildThreads == 0)
    {
        std::cout << "warning: .siteinfo/nsm.config: number of build threads not detected or invalid, set to default of -1 (number of cores)" << std::endl;

        buildThreads = -1;

        configChanged = 1;
    }

    //code to set unixTextEditor if not present
    if(unixTextEditor == "")
    {
        std::cout << "warning: .siteinfo/nsm.config: unix text editor not detected, set to default of 'nano'" << std::endl;

        unixTextEditor = "nano";

        configChanged = 1;
    }

    //code to set winTextEditor if not present
    if(winTextEditor == "")
    {
        std::cout << "warning: .siteinfo/nsm.config: windows text editor not detected, set to default of 'notepad'" << std::endl;

        winTextEditor = "notepad";

        configChanged = 1;
    }

    //code to figure out rootBranch and siteBranch if not present
    if(rootBranch == "" || siteBranch == "")
    {
        std::cout << "warning: .siteinfo/nsm.config: root or site branch not detected, have attempted to determine them, ensure values in config file are correct" << std::endl;

        if(std::ifstream(".git"))
        {
            std::set<std::string> branches = get_git_branches();

            if(branches.count("stage"))
            {
                rootBranch = "stage";
                siteBranch = "master";
            }
            else
                rootBranch = siteBranch = "master";
        }
        else
            rootBranch = siteBranch = "##unset##";

        configChanged = 1;
    }

    if(configChanged)
        save_config();

    return 0;
}

int SiteInfo::open_pages()
{
    pages.clear();

    if(!std::ifstream(".siteinfo/pages.list"))
    {
        //this should never happen!
        std::cout << "error: SiteInfo.cpp: open(): could not open site information as '" << get_pwd() << "/.siteinfo/pages.list' does not exist" << std::endl;
        return 1;
    }

    //reads page list file
    std::ifstream ifs(".siteinfo/pages.list");
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

int SiteInfo::save_config()
{
    std::ofstream ofs(".siteinfo/nsm.config");
    ofs << "contentDir " << quote(contentDir) << "\n";
    ofs << "contentExt " << quote(contentExt) << "\n";
    ofs << "siteDir " << quote(siteDir) << "\n";
    ofs << "pageExt " << quote(pageExt) << "\n";
    ofs << "scriptExt " << quote(scriptExt) << "\n";
    ofs << "defaultTemplate " << defaultTemplate << "\n\n";
    ofs << "backupScripts " << backupScripts << "\n\n";
    ofs << "buildThreads " << buildThreads << "\n\n";
    ofs << "unixTextEditor " << quote(unixTextEditor) << "\n";
    ofs << "winTextEditor " << quote(winTextEditor) << "\n\n";
    ofs << "rootBranch " << quote(rootBranch) << "\n";
    ofs << "siteBranch " << quote(siteBranch) << "\n";
    ofs.close();

    return 0;
}

int SiteInfo::save_pages()
{
    std::ofstream ofs(".siteinfo/pages.list");

    for(auto page=pages.begin(); page!=pages.end(); page++)
        ofs << *page << "\n\n";

    ofs.close();

    return 0;
}

PageInfo SiteInfo::make_info(const Name& pageName)
{
    PageInfo pageInfo;

    pageInfo.pageName = pageName;

    Path pageNameAsPath;
    pageNameAsPath.set_file_path_from(unquote(pageName));

    pageInfo.contentPath = Path(contentDir + pageNameAsPath.dir, pageNameAsPath.file + contentExt);
    pageInfo.pagePath = Path(siteDir + pageNameAsPath.dir, pageNameAsPath.file + pageExt);

    pageInfo.pageTitle = pageName;
    pageInfo.templatePath = defaultTemplate;

    //checks for non-default extensions
    std::string inExt;
    Path extPath = pageInfo.pagePath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
    if(std::ifstream(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, inExt);
        ifs.close();

        pageInfo.contentPath.file = pageInfo.contentPath.file.substr(0, pageInfo.contentPath.file.find_first_of('.')) + inExt;
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".pageExt";
    if(std::ifstream(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, inExt);
        ifs.close();

        pageInfo.pagePath.file = pageInfo.pagePath.file.substr(0, pageInfo.pagePath.file.find_first_of('.')) + inExt;
    }

    return pageInfo;
}

PageInfo SiteInfo::make_info(const Name& pageName, const Title& pageTitle)
{
    PageInfo pageInfo;

    pageInfo.pageName = pageName;

    Path pageNameAsPath;
    pageNameAsPath.set_file_path_from(unquote(pageName));

    pageInfo.contentPath = Path(contentDir + pageNameAsPath.dir, pageNameAsPath.file + contentExt);
    pageInfo.pagePath = Path(siteDir + pageNameAsPath.dir, pageNameAsPath.file + pageExt);

    pageInfo.pageTitle = pageTitle;
    pageInfo.templatePath = defaultTemplate;

    //checks for non-default extensions
    std::string inExt;
    Path extPath = pageInfo.pagePath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
    if(std::ifstream(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, inExt);
        ifs.close();

        pageInfo.contentPath.file = pageInfo.contentPath.file.substr(0, pageInfo.contentPath.file.find_first_of('.')) + inExt;
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".pageExt";
    if(std::ifstream(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, inExt);
        ifs.close();

        pageInfo.pagePath.file = pageInfo.pagePath.file.substr(0, pageInfo.pagePath.file.find_first_of('.')) + inExt;
    }

    return pageInfo;
}

PageInfo SiteInfo::make_info(const Name& pageName, const Title& pageTitle, const Path& templatePath)
{
    PageInfo pageInfo;

    pageInfo.pageName = pageName;

    Path pageNameAsPath;
    pageNameAsPath.set_file_path_from(unquote(pageName));

    pageInfo.contentPath = Path(contentDir + pageNameAsPath.dir, pageNameAsPath.file + contentExt);
    pageInfo.pagePath = Path(siteDir + pageNameAsPath.dir, pageNameAsPath.file + pageExt);

    pageInfo.pageTitle = pageTitle;
    pageInfo.templatePath = templatePath;

    //checks for non-default extensions
    std::string inExt;
    Path extPath = pageInfo.pagePath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
    if(std::ifstream(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, inExt);
        ifs.close();

        pageInfo.contentPath.file = pageInfo.contentPath.file.substr(0, pageInfo.contentPath.file.find_first_of('.')) + inExt;
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".pageExt";
    if(std::ifstream(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, inExt);
        ifs.close();

        pageInfo.pagePath.file = pageInfo.pagePath.file.substr(0, pageInfo.pagePath.file.find_first_of('.')) + inExt;
    }

    return pageInfo;
}

PageInfo make_info(const Name& pageName, const Title& pageTitle, const Path& templatePath, const Directory& contentDir, const Directory& siteDir, const std::string& contentExt, const std::string& pageExt)
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

PageInfo SiteInfo::get_info(const Name& pageName)
{
    PageInfo page;
    page.pageName = pageName;
    auto result = pages.find(page);

    if(result != pages.end())
        return *result;

    page.pageName = "##not-found##";
    return page;
}

int SiteInfo::info(const std::vector<Name>& pageNames)
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
            std::cout << "    page name: " << quote(cPageInfo.pageName) << std::endl;
            std::cout << "   page title: " << cPageInfo.pageTitle << std::endl;
            std::cout << "    page path: " << cPageInfo.pagePath << std::endl;
            std::cout << "     page ext: " << quote(get_page_ext(cPageInfo)) << std::endl;
            std::cout << " content path: " << cPageInfo.contentPath << std::endl;
            std::cout << "  content ext: " << quote(get_cont_ext(cPageInfo)) << std::endl;
            std::cout << "   script ext: " << quote(get_script_ext(cPageInfo)) << std::endl;
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
    if(std::ifstream(".siteinfo/.watchinfo/watching.list"))
    {
        WatchList wl;
        if(wl.open())
        {
            std::cout << "error: info-all: failed to open watch list" << std::endl;
            return 1;
        }

        std::cout << std::endl;
        std::cout << "------ all watched directories ------" << std::endl;
        for(auto wd=wl.dirs.begin(); wd!=wl.dirs.end(); wd++)
        {
            if(wd != wl.dirs.begin())
                std::cout << std::endl;
            std::cout << "  watch directory: " << quote(wd->watchDir) << std::endl;
            for(auto ext=wd->contExts.begin(); ext!=wd->contExts.end(); ext++)
            {
                std::cout << "content extension: " << quote(*ext) << std::endl;
                std::cout << "    template path: " << wd->templatePaths.at(*ext) << std::endl;
                std::cout << "   page extension: " << wd->pageExts.at(*ext) << std::endl;
            }
        }
        std::cout << "-------------------------------------" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "--------- all tracked pages ---------" << std::endl;
    for(auto page=pages.begin(); page!=pages.end(); page++)
    {
        if(page != pages.begin())
            std::cout << std::endl;
        std::cout << "    page name: " << quote(page->pageName) << std::endl;
        std::cout << "   page title: " << page->pageTitle << std::endl;
        std::cout << "    page path: " << page->pagePath << std::endl;
        std::cout << "     page ext: " << quote(get_page_ext(*page)) << std::endl;
        std::cout << " content path: " << page->contentPath << std::endl;
        std::cout << "  content ext: " << quote(get_cont_ext(*page)) << std::endl;
        std::cout << "   script ext: " << quote(get_script_ext(*page)) << std::endl;
        std::cout << "template path: " << page->templatePath << std::endl;
    }
    std::cout << "------------------------------------" << std::endl;

    return 0;
}

int SiteInfo::info_watching()
{
    if(std::ifstream(".siteinfo/.watchinfo/watching.list"))
    {
        WatchList wl;
        if(wl.open())
        {
            std::cout << "error: info-watching: failed to open watch list" << std::endl;
            return 1;
        }

        std::cout << std::endl;
        std::cout << "------ all watched directories ------" << std::endl;
        for(auto wd=wl.dirs.begin(); wd!=wl.dirs.end(); wd++)
        {
            if(wd != wl.dirs.begin())
                std::cout << std::endl;
            std::cout << "  watch directory: " << quote(wd->watchDir) << std::endl;
            for(auto ext=wd->contExts.begin(); ext!=wd->contExts.end(); ext++)
            {
                std::cout << "content extension: " << quote(*ext) << std::endl;
                std::cout << "    template path: " << wd->templatePaths.at(*ext) << std::endl;
                std::cout << "   page extension: " << wd->pageExts.at(*ext) << std::endl;
            }
        }
        std::cout << "-------------------------------------" << std::endl;
    }
    else
    {
        std::cout << "not watching any directories" << std::endl;
    }

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

std::string SiteInfo::get_ext(const PageInfo& page, const std::string& extType)
{
    std::string ext;

    //checks for non-default page extension
    Path extPath = page.pagePath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + extType;

    if(std::ifstream(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, ext);
        ifs.close();
    }
    else if(extType == ".contExt")
        ext = contentExt;
    else if(extType == ".pageExt")
        ext = pageExt;
    else if(extType == ".scriptExt")
        ext = scriptExt;

    return ext;
}

std::string SiteInfo::get_cont_ext(const PageInfo& page)
{
    return get_ext(page, ".contExt");
}

std::string SiteInfo::get_page_ext(const PageInfo& page)
{
    return get_ext(page, ".pageExt");
}

std::string SiteInfo::get_script_ext(const PageInfo& page)
{
    return get_ext(page, ".scriptExt");
}

bool SiteInfo::tracking(const PageInfo& page)
{
    return pages.count(page);
}

bool SiteInfo::tracking(const Name& pageName)
{
    PageInfo page;
    page.pageName = pageName;
    return pages.count(page);
}

int SiteInfo::track(const Name& name, const Title& title, const Path& templatePath)
{
    if(name.find('.') != std::string::npos)
    {
        std::cout << "error: page names cannot contain '.'" << std::endl;
        std::cout << "note: you can add post-build scripts to move built pages/files to paths containing . other than for extensions if you want" << std::endl;
        return 1;
    }
    else if(name == "" || title.str == "" || templatePath == Path("", ""))
    {
        std::cout << "error: page name, title and template path must all be non-empty strings" << std::endl;
        return 1;
    }
    if(unquote(name)[unquote(name).size()-1] == '/' || unquote(name)[unquote(name).size()-1] == '\\')
    {
        std::cout << "error: page name " << quote(name) << " cannot end in '/' or '\\'" << std::endl;
        return 1;
    }
    else if(
                (unquote(name).find('"') != std::string::npos && unquote(name).find('\'') != std::string::npos) ||
                (unquote(title.str).find('"') != std::string::npos && unquote(title.str).find('\'') != std::string::npos) ||
                (unquote(templatePath.str()).find('"') != std::string::npos && unquote(templatePath.str()).find('\'') != std::string::npos)
            )
    {
        std::cout << "error: page name, title and template path cannot contain both single and double quotes" << std::endl;
        return 1;
    }

    PageInfo newPage = make_info(name, title, templatePath);

    if(newPage.contentPath == newPage.templatePath)
    {
        std::cout << "error: content and template paths cannot be the same, page not tracked" << std::endl;
        return 1;
    }

    if(pages.count(newPage) > 0)
    {
        PageInfo cInfo = *(pages.find(newPage));

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
        std::cout << "warning: content path " << newPage.contentPath << " did not exist" << std::endl;
        newPage.contentPath.ensureFileExists();
        //chmod(newPage.contentPath.str().c_str(), 0666);
    }
    if(!std::ifstream(newPage.templatePath.str()))
    {
        std::cout << "error: template path " << newPage.templatePath << " does not exist" << std::endl;
        return 1;
    }

    //ensures directories for page file and page info file exist
    newPage.pagePath.ensureDirExists();
    newPage.pagePath.getInfoPath().ensureDirExists();

    //inserts new page into set of pages
    pages.insert(newPage);

    //saves new set of pages to pages.list
    std::ofstream ofs(".siteinfo/pages.list");
    for(auto page=pages.begin(); page!=pages.end(); page++)
        ofs << *page << "\n\n";
    ofs.close();

    //informs user that page addition was successful
    std::cout << "successfully tracking " << newPage.pageName << std::endl;

    return 0;
}

int SiteInfo::track_from_file(const std::string& filePath)
{
    if(!std::ifstream(filePath))
    {
        std::cout << "error: track-from-file: track-file " << quote(filePath) << " does not exist" << std::endl;
        return 1;
    }

    Name name;
    Title title;
    Path templatePath;
    std::string cExt, pExt, str;
    int noPagesAdded = 0;

    std::ifstream ifs(filePath);
    while(getline(ifs, str))
    {
        std::istringstream iss(str);

        name = "";
        title.str = "";
        cExt = "";
        templatePath = Path("", "");
        pExt = "";

        if(read_quoted(iss, name))
            if(read_quoted(iss, title.str))
                if(read_quoted(iss, cExt))
                    if(templatePath.read_file_path_from(iss))
                        read_quoted(iss, pExt);

        if(name == "" || name[0] == '#')
            continue;

        if(title.str == "")
            title.str = name;

        if(cExt == "")
            cExt = contentExt;

        if(templatePath == Path("", ""))
            templatePath = defaultTemplate;

        if(pExt == "")
            pExt = pageExt;

        if(name.find('.') != std::string::npos)
        {
            std::cout << "error: page name " << quote(name) << " cannot contain '.'" << std::endl;
            std::cout << "note: you can add post-build scripts to move built pages/files to paths containing . other than for extensions if you want" << std::endl;
            return 1;
        }
        else if(name == "" || title.str == "" || templatePath == Path("", "") || cExt == "" || pExt == "")
        {
            std::cout << "error: page names, titles, template paths, content extensions and page extensions must all be non-empty strings" << std::endl;
            return 1;
        }
        if(unquote(name)[unquote(name).size()-1] == '/' || unquote(name)[unquote(name).size()-1] == '\\')
        {
            std::cout << "error: page name " << quote(name) << " cannot end in '/' or '\\'" << std::endl;
            return 1;
        }
        else if(
                    (unquote(name).find('"') != std::string::npos && unquote(name).find('\'') != std::string::npos) ||
                    (unquote(title.str).find('"') != std::string::npos && unquote(title.str).find('\'') != std::string::npos) ||
                    (unquote(templatePath.str()).find('"') != std::string::npos && unquote(templatePath.str()).find('\'') != std::string::npos)
                )
        {
            std::cout << "error: page names, titles and template paths cannot contain both single and double quotes" << std::endl;
            return 1;
        }

        PageInfo newPage = make_info(name, title, templatePath);

        if(cExt != contentExt)
        {
            newPage.contentPath.file = newPage.contentPath.file.substr(0, newPage.contentPath.file.find_first_of('.')) + cExt;

            Path extPath = newPage.pagePath.getInfoPath();
            extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
            std::ofstream ofs(extPath.str());
            ofs << cExt << std::endl;
            ofs.close();
        }

        if(pExt != pageExt)
        {
            newPage.pagePath.file = newPage.pagePath.file.substr(0, newPage.pagePath.file.find_first_of('.')) + pExt;

            Path extPath = newPage.pagePath.getInfoPath();
            extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".pageExt";
            std::ofstream ofs(extPath.str());
            ofs << pExt << std::endl;
            ofs.close();
        }

        if(newPage.contentPath == newPage.templatePath)
        {
            std::cout << "error: content and template paths cannot be the same, page not tracked" << std::endl;
            return 1;
        }

        if(pages.count(newPage) > 0)
        {
            PageInfo cInfo = *(pages.find(newPage));

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
            //std::cout << "warning: content path " << newPage.contentPath << " did not exist" << std::endl;
            newPage.contentPath.ensureFileExists();
        }
        if(!std::ifstream(newPage.templatePath.str()))
        {
            std::cout << "error: template path " << newPage.templatePath << " does not exist" << std::endl;
            return 1;
        }

        //ensures directories for page file and page info file exist
        newPage.pagePath.ensureDirExists();
        newPage.pagePath.getInfoPath().ensureDirExists();

        //inserts new page into set of pages
        pages.insert(newPage);
        noPagesAdded++;
    }
    ifs.close();

    //saves new set of pages to pages.list
    std::ofstream ofs(".siteinfo/pages.list");
    for(auto page=pages.begin(); page!=pages.end(); page++)
        ofs << *page << "\n\n";
    ofs.close();

    std::cout << "successfully tracked " << noPagesAdded << " pages" << std::endl;

    return 0;
}

int SiteInfo::track_dir(const Path& dirPath, const std::string& cExt, const Path& templatePath, const std::string& pExt)
{
    if(dirPath.file != "")
    {
        std::cout << "error: track-dir: directory path " << dirPath << " is to a file not a directory" << std::endl;
        return 1;
    }
    else if(dirPath.comparableStr().substr(0, contentDir.size()) != contentDir)
    {
        std::cout << "error: track-dir: cannot track files from directory " << dirPath << " as it is not a subdirectory of the site-wide content directory " << quote(contentDir) << std::endl;
        return 1;
    }
    else if(!std::ifstream(dirPath.str()))
    {
        std::cout << "error: track-dir: directory path " << dirPath << " does not exist" << std::endl;
        return 1;
    }

    if(cExt == "" || cExt[0] != '.')
    {
        std::cout << "error: track-dir: content extension " << quote(cExt) << " must start with a fullstop" << std::endl;
        return 1;
    }
    else if(pExt == "" || pExt[0] != '.')
    {
        std::cout << "error: track-dir: page extension " << quote(pExt) << " must start with a fullstop" << std::endl;
        return 1;
    }

    if(!std::ifstream(templatePath.str()))
    {
        std::cout << "error: track-dir: template path " << templatePath << " does not exist" << std::endl;
        return 1;
    }

    std::vector<TrackInfo> pages_to_track;
    std::string ext;
    std::string contDir2dirPath = "";
    if(comparable(dirPath.dir).size() > comparable(contentDir).size())
       contDir2dirPath = dirPath.dir.substr(contentDir.size(), dirPath.dir.size()-contentDir.size());
    Name pagename;
    size_t pos;
    std::vector<std::string> files = lsVec(dirPath.dir.c_str());
    for(size_t f=0; f<files.size(); f++)
    {
        pos = files[f].find_first_of('.');
        if(pos != std::string::npos)
        {
            ext = files[f].substr(pos, files[f].size()-pos);
            pagename = contDir2dirPath + files[f].substr(0, pos);

            if(cExt == ext)
                if(!tracking(pagename))
                    pages_to_track.push_back(TrackInfo(pagename, Title(pagename), templatePath, cExt, pExt));
        }
    }

    if(pages_to_track.size())
        track(pages_to_track);

    return 0;
}

int SiteInfo::track(const Name& name, const Title& title, const Path& templatePath, const std::string& cExt, const std::string& pExt)
{
    if(name.find('.') != std::string::npos)
    {
        std::cout << "error: page names cannot contain '.'" << std::endl;
        std::cout << "note: you can add post-build scripts to move built pages/files to paths containing . other than for extensions if you want" << std::endl;
        return 1;
    }
    else if(name == "" || title.str == "" || templatePath == Path("", "") || cExt == "" || pExt == "")
    {
        std::cout << "error: page name, title, template path, content extension and page extension must all be non-empty strings" << std::endl;
        return 1;
    }
    if(unquote(name)[unquote(name).size()-1] == '/' || unquote(name)[unquote(name).size()-1] == '\\')
    {
        std::cout << "error: page name " << quote(name) << " cannot end in '/' or '\\'" << std::endl;
        return 1;
    }
    else if(
                (unquote(name).find('"') != std::string::npos && unquote(name).find('\'') != std::string::npos) ||
                (unquote(title.str).find('"') != std::string::npos && unquote(title.str).find('\'') != std::string::npos) ||
                (unquote(templatePath.str()).find('"') != std::string::npos && unquote(templatePath.str()).find('\'') != std::string::npos)
            )
    {
        std::cout << "error: page name, title and template path cannot contain both single and double quotes" << std::endl;
        return 1;
    }

    PageInfo newPage = make_info(name, title, templatePath);

    if(cExt != contentExt)
    {
        newPage.contentPath.file = newPage.contentPath.file.substr(0, newPage.contentPath.file.find_first_of('.')) + cExt;

        Path extPath = newPage.pagePath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
        std::ofstream ofs(extPath.str());
        ofs << cExt << std::endl;
        ofs.close();
    }

    if(pExt != pageExt)
    {
        newPage.pagePath.file = newPage.pagePath.file.substr(0, newPage.pagePath.file.find_first_of('.')) + pExt;

        Path extPath = newPage.pagePath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".pageExt";
        std::ofstream ofs(extPath.str());
        ofs << pExt << std::endl;
        ofs.close();
    }

    if(newPage.contentPath == newPage.templatePath)
    {
        std::cout << "error: content and template paths cannot be the same, page not tracked" << std::endl;
        return 1;
    }

    if(pages.count(newPage) > 0)
    {
        PageInfo cInfo = *(pages.find(newPage));

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
        std::cout << "warning: content path " << newPage.contentPath << " did not exist" << std::endl;
        newPage.contentPath.ensureFileExists();
        //chmod(newPage.contentPath.str().c_str(), 0666);
    }
    if(!std::ifstream(newPage.templatePath.str()))
    {
        std::cout << "error: template path " << newPage.templatePath << " does not exist" << std::endl;
        return 1;
    }

    //ensures directories for page file and page info file exist
    newPage.pagePath.ensureDirExists();
    newPage.pagePath.getInfoPath().ensureDirExists();

    //inserts new page into set of pages
    pages.insert(newPage);

    //saves new set of pages to pages.list
    std::ofstream ofs(".siteinfo/pages.list");
    for(auto page=pages.begin(); page!=pages.end(); page++)
        ofs << *page << "\n\n";
    ofs.close();

    //informs user that page addition was successful
    std::cout << "successfully tracking " << newPage.pageName << std::endl;

    return 0;
}

int SiteInfo::track(const std::vector<TrackInfo>& pagesToTrack)
{
    Name name;
    Title title;
    Path templatePath;
    std::string cExt, pExt;

    for(size_t p=0; p<pagesToTrack.size(); p++)
    {
        name = pagesToTrack[p].pagename;
        title = pagesToTrack[p].title;
        templatePath = pagesToTrack[p].templatePath;
        cExt = pagesToTrack[p].contExt;
        pExt = pagesToTrack[p].pageExt;

        if(name.find('.') != std::string::npos)
        {
            std::cout << "error: page name " << quote(name) << " cannot contain '.'" << std::endl;
            std::cout << "note: you can add post-build scripts to move built pages/files to paths containing . other than for extensions if you want" << std::endl;
            return 1;
        }
        else if(name == "" || title.str == "" || templatePath == Path("", "") || cExt == "" || pExt == "")
        {
            std::cout << "error: page names, titles, template paths, content extensions and page extensions must all be non-empty strings" << std::endl;
            return 1;
        }
        if(unquote(name)[unquote(name).size()-1] == '/' || unquote(name)[unquote(name).size()-1] == '\\')
        {
            std::cout << "error: page name " << quote(name) << " cannot end in '/' or '\\'" << std::endl;
            return 1;
        }
        else if(
                    (unquote(name).find('"') != std::string::npos && unquote(name).find('\'') != std::string::npos) ||
                    (unquote(title.str).find('"') != std::string::npos && unquote(title.str).find('\'') != std::string::npos) ||
                    (unquote(templatePath.str()).find('"') != std::string::npos && unquote(templatePath.str()).find('\'') != std::string::npos)
                )
        {
            std::cout << "error: page names, titles and template paths cannot contain both single and double quotes" << std::endl;
            return 1;
        }

        PageInfo newPage = make_info(name, title, templatePath);

        if(cExt != contentExt)
        {
            newPage.contentPath.file = newPage.contentPath.file.substr(0, newPage.contentPath.file.find_first_of('.')) + cExt;

            Path extPath = newPage.pagePath.getInfoPath();
            extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
            std::ofstream ofs(extPath.str());
            ofs << cExt << std::endl;
            ofs.close();
        }

        if(pExt != pageExt)
        {
            newPage.pagePath.file = newPage.pagePath.file.substr(0, newPage.pagePath.file.find_first_of('.')) + pExt;

            Path extPath = newPage.pagePath.getInfoPath();
            extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".pageExt";
            std::ofstream ofs(extPath.str());
            ofs << pExt << std::endl;
            ofs.close();
        }

        if(newPage.contentPath == newPage.templatePath)
        {
            std::cout << "error: content and template paths cannot be the same, page not tracked" << std::endl;
            return 1;
        }

        if(pages.count(newPage) > 0)
        {
            PageInfo cInfo = *(pages.find(newPage));

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
            std::cout << "warning: content path " << newPage.contentPath << " did not exist" << std::endl;
            newPage.contentPath.ensureFileExists();
            //chmod(newPage.contentPath.str().c_str(), 0666);
        }
        if(!std::ifstream(newPage.templatePath.str()))
        {
            std::cout << "error: template path " << newPage.templatePath << " does not exist" << std::endl;
            return 1;
        }

        //ensures directories for page file and page info file exist
        newPage.pagePath.ensureDirExists();
        newPage.pagePath.getInfoPath().ensureDirExists();

        //inserts new page into set of pages
        pages.insert(newPage);

        //informs user that page addition was successful
        std::cout << "successfully tracking " << newPage.pageName << std::endl;
    }

    //saves new set of pages to pages.list
    std::ofstream ofs(".siteinfo/pages.list");
    for(auto page=pages.begin(); page!=pages.end(); page++)
        ofs << *page << "\n\n";
    ofs.close();

    return 0;
}

int SiteInfo::untrack(const Name& pageNameToUntrack)
{
    //checks that page is being tracked
    if(!tracking(pageNameToUntrack))
    {
        std::cout << "error: Nift is not tracking " << pageNameToUntrack << std::endl;
        return 1;
    }

    PageInfo pageToErase = get_info(pageNameToUntrack);

    //removes the extension files if they exist
    Path extPath = pageToErase.pagePath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
    if(std::ifstream(extPath.str()))
    {
        chmod(extPath.str().c_str(), 0666);
        remove_path(extPath);
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".pageExt";
    if(std::ifstream(extPath.str()))
    {
        chmod(extPath.str().c_str(), 0666);
        remove_path(extPath);
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";
    if(std::ifstream(extPath.str()))
    {
        chmod(extPath.str().c_str(), 0666);
        remove_path(extPath);
    }

    //removes page info file and containing dirs if now empty
    if(std::ifstream(pageToErase.pagePath.getInfoPath().dir))
    {
        chmod(pageToErase.pagePath.getInfoPath().str().c_str(), 0666);
        remove_path(pageToErase.pagePath.getInfoPath());
    }

    //removes page file and containing dirs if now empty
    if(std::ifstream(pageToErase.pagePath.dir))
    {
        chmod(pageToErase.pagePath.str().c_str(), 0666);
        remove_path(pageToErase.pagePath);
    }

    //removes page from pages set
    pages.erase(pageToErase);

    //saves new set of pages to .siteinfo/pages.list
    save_pages();

    //informs user that page was successfully untracked
    std::cout << "successfully untracked " << pageNameToUntrack << std::endl;

    return 0;
}

int SiteInfo::untrack(const std::vector<Name>& pageNamesToUntrack)
{
    PageInfo pageToErase;
    Path extPath;
    for(size_t p=0; p<pageNamesToUntrack.size(); p++)
    {
        //checks that page is being tracked
        if(!tracking(pageNamesToUntrack[p]))
        {
            std::cout << "error: Nift is not tracking " << pageNamesToUntrack[p] << std::endl;
            return 1;
        }

        pageToErase = get_info(pageNamesToUntrack[p]);

        //removes the extension files if they exist
        Path extPath = pageToErase.pagePath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
        if(std::ifstream(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".pageExt";
        if(std::ifstream(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";
        if(std::ifstream(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }

        //removes page info file and containing dirs if now empty
        if(std::ifstream(pageToErase.pagePath.getInfoPath().dir))
        {
            chmod(pageToErase.pagePath.getInfoPath().str().c_str(), 0666);
            remove_path(pageToErase.pagePath.getInfoPath());
        }

        //removes page file and containing dirs if now empty
        if(std::ifstream(pageToErase.pagePath.dir))
        {
            chmod(pageToErase.pagePath.str().c_str(), 0666);
            remove_path(pageToErase.pagePath);
        }

        //removes page from pages set
        pages.erase(pageToErase);

        //informs user that page was successfully untracked
        std::cout << "successfully untracked " << pageNamesToUntrack[p] << std::endl;
    }

    //saves new set of pages to .siteinfo/pages.list
    save_pages();

    return 0;
}

int SiteInfo::untrack_from_file(const std::string& filePath)
{
    if(!std::ifstream(filePath))
    {
        std::cout << "error: untrack-from-file: untrack-file " << quote(filePath) << " does not exist" << std::endl;
        return 1;
    }

    Name pageNameToUntrack;
    int noPagesUntracked = 0;
    std::string str;
    std::ifstream ifs(filePath);
    while(getline(ifs, str))
    {
        std::istringstream iss(str);

        read_quoted(iss, pageNameToUntrack);

        if(pageNameToUntrack == "" || pageNameToUntrack[0] == '#')
            continue;

        //checks that page is being tracked
        if(!tracking(pageNameToUntrack))
        {
            std::cout << "error: Nift is not tracking " << pageNameToUntrack << std::endl;
            return 1;
        }

        PageInfo pageToErase = get_info(pageNameToUntrack);

        //removes the extension files if they exist
        Path extPath = pageToErase.pagePath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
        if(std::ifstream(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".pageExt";
        if(std::ifstream(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";
        if(std::ifstream(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }

        //removes page info file and containing dirs if now empty
        if(std::ifstream(pageToErase.pagePath.getInfoPath().dir))
        {
            chmod(pageToErase.pagePath.getInfoPath().str().c_str(), 0666);
            remove_path(pageToErase.pagePath.getInfoPath());
        }

        //removes page file and containing dirs if now empty
        if(std::ifstream(pageToErase.pagePath.dir))
        {
            chmod(pageToErase.pagePath.str().c_str(), 0666);
            remove_path(pageToErase.pagePath);
        }

        //removes page from pages set
        pages.erase(pageToErase);
        noPagesUntracked++;
    }

    ifs.close();

    //saves new set of pages to .siteinfo/pages.list
    save_pages();

    std::cout << "successfully untracked " << noPagesUntracked << " pages" << std::endl;

    return 0;
}

int SiteInfo::untrack_dir(const Path& dirPath, const std::string& cExt)
{
    if(dirPath.file != "")
    {
        std::cout << "error: untrack-dir: directory path " << dirPath << " is to a file not a directory" << std::endl;
        return 1;
    }
    else if(dirPath.comparableStr().substr(0, contentDir.size()) != contentDir)
    {
        std::cout << "error: untrack-dir: cannot untrack pages from directory " << dirPath << " as it is not a subdirectory of the site-wide content directory " << quote(contentDir) << std::endl;
        return 1;
    }
    else if(!std::ifstream(dirPath.str()))
    {
        std::cout << "error: untrack-dir: directory path " << dirPath << " does not exist" << std::endl;
        return 1;
    }

    if(cExt == "" || cExt[0] != '.')
    {
        std::cout << "error: untrack-dir: content extension " << quote(cExt) << " must start with a fullstop" << std::endl;
        return 1;
    }

    std::vector<Name> pageNamesToUntrack;
    std::string ext;
    std::string contDir2dirPath = "";
    if(comparable(dirPath.dir).size() > comparable(contentDir).size())
       contDir2dirPath = dirPath.dir.substr(contentDir.size(), dirPath.dir.size()-contentDir.size());
    Name pagename;
    size_t pos;
    std::vector<std::string> files = lsVec(dirPath.dir.c_str());
    for(size_t f=0; f<files.size(); f++)
    {
        pos = files[f].find_first_of('.');
        if(pos != std::string::npos)
        {
            ext = files[f].substr(pos, files[f].size()-pos);
            pagename = contDir2dirPath + files[f].substr(0, pos);

            if(cExt == ext)
                if(tracking(pagename))
                    pageNamesToUntrack.push_back(pagename);
        }
    }

    if(pageNamesToUntrack.size())
        untrack(pageNamesToUntrack);

    return 0;
}

int SiteInfo::rm_from_file(const std::string& filePath)
{
    if(!std::ifstream(filePath))
    {
        std::cout << "error: rm-from-file: rm-file " << quote(filePath) << " does not exist" << std::endl;
        return 1;
    }

    Name pageNameToRemove;
    int noPagesRemoved = 0;
    std::string str;
    std::ifstream ifs(filePath);
    while(getline(ifs, str))
    {
        std::istringstream iss(str);

        read_quoted(iss, pageNameToRemove);

        if(pageNameToRemove == "" || pageNameToRemove[0] == '#')
            continue;

        //checks that page is being tracked
        if(!tracking(pageNameToRemove))
        {
            std::cout << "error: Nift is not tracking " << pageNameToRemove << std::endl;
            return 1;
        }

        PageInfo pageToErase = get_info(pageNameToRemove);

        //removes the extension files if they exist
        Path extPath = pageToErase.pagePath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
        if(std::ifstream(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".pageExt";
        if(std::ifstream(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";
        if(std::ifstream(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }

        //removes page info file and containing dirs if now empty
        if(std::ifstream(pageToErase.pagePath.getInfoPath().dir))
        {
            chmod(pageToErase.pagePath.getInfoPath().str().c_str(), 0666);
            remove_path(pageToErase.pagePath.getInfoPath());
        }

        //removes page file and containing dirs if now empty
        if(std::ifstream(pageToErase.pagePath.dir))
        {
            chmod(pageToErase.pagePath.str().c_str(), 0666);
            remove_path(pageToErase.pagePath);
        }

        //removes content file and containing dirs if now empty
        if(std::ifstream(pageToErase.contentPath.dir))
        {
            chmod(pageToErase.contentPath.str().c_str(), 0666);
            remove_path(pageToErase.contentPath);
        }

        //removes page from pages set
        pages.erase(pageToErase);
        noPagesRemoved++;
    }

    ifs.close();

    //saves new set of pages to pages.list
    save_pages();

    std::cout << "successfully removed " << noPagesRemoved << " pages" << std::endl;

    return 0;
}

int SiteInfo::rm_dir(const Path& dirPath, const std::string& cExt)
{
    if(dirPath.file != "")
    {
        std::cout << "error: rm-dir: directory path " << dirPath << " is to a file not a directory" << std::endl;
        return 1;
    }
    else if(dirPath.comparableStr().substr(0, contentDir.size()) != contentDir)
    {
        std::cout << "error: rm-dir: cannot remove pages from directory " << dirPath << " as it is not a subdirectory of the site-wide content directory " << quote(contentDir) << std::endl;
        return 1;
    }
    else if(!std::ifstream(dirPath.str()))
    {
        std::cout << "error: rm-dir: directory path " << dirPath << " does not exist" << std::endl;
        return 1;
    }

    if(cExt == "" || cExt[0] != '.')
    {
        std::cout << "error: rm-dir: content extension " << quote(cExt) << " must start with a fullstop" << std::endl;
        return 1;
    }

    std::vector<Name> pageNamesToRemove;
    std::string ext;
    std::string contDir2dirPath = "";
    if(comparable(dirPath.dir).size() > comparable(contentDir).size())
       contDir2dirPath = dirPath.dir.substr(contentDir.size(), dirPath.dir.size()-contentDir.size());
    Name pagename;
    size_t pos;
    std::vector<std::string> files = lsVec(dirPath.dir.c_str());
    for(size_t f=0; f<files.size(); f++)
    {
        pos = files[f].find_first_of('.');
        if(pos != std::string::npos)
        {
            ext = files[f].substr(pos, files[f].size()-pos);
            pagename = contDir2dirPath + files[f].substr(0, pos);

            if(cExt == ext)
                if(tracking(pagename))
                    pageNamesToRemove.push_back(pagename);
        }
    }

    if(pageNamesToRemove.size())
        rm(pageNamesToRemove);

    return 0;
}

int SiteInfo::rm(const Name& pageNameToRemove)
{
    //checks that page is being tracked
    if(!tracking(pageNameToRemove))
    {
        std::cout << "error: Nift is not tracking " << pageNameToRemove << std::endl;
        return 1;
    }

    PageInfo pageToErase = get_info(pageNameToRemove);

    //removes the extension files if they exist
    Path extPath = pageToErase.pagePath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
    if(std::ifstream(extPath.str()))
    {
        chmod(extPath.str().c_str(), 0666);
        remove_path(extPath);
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".pageExt";
    if(std::ifstream(extPath.str()))
    {
        chmod(extPath.str().c_str(), 0666);
        remove_path(extPath);
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";
    if(std::ifstream(extPath.str()))
    {
        chmod(extPath.str().c_str(), 0666);
        remove_path(extPath);
    }

    //removes page info file and containing dirs if now empty
    if(std::ifstream(pageToErase.pagePath.getInfoPath().dir))
    {
        chmod(pageToErase.pagePath.getInfoPath().str().c_str(), 0666);
        remove_path(pageToErase.pagePath.getInfoPath());
    }

    //removes page file and containing dirs if now empty
    if(std::ifstream(pageToErase.pagePath.dir))
    {
        chmod(pageToErase.pagePath.str().c_str(), 0666);
        remove_path(pageToErase.pagePath);
    }

    //removes content file and containing dirs if now empty
    if(std::ifstream(pageToErase.contentPath.dir))
    {
        chmod(pageToErase.contentPath.str().c_str(), 0666);
        remove_path(pageToErase.contentPath);
    }

    //removes page from pages set
    pages.erase(pageToErase);

    //saves new set of pages to pages.list
    save_pages();

    //informs user that page was successfully removed
    std::cout << "successfully removed " << pageNameToRemove << std::endl;

    return 0;
}

int SiteInfo::rm(const std::vector<Name>& pageNamesToRemove)
{
    PageInfo pageToErase;
    Path extPath;
    for(size_t p=0; p<pageNamesToRemove.size(); p++)
    {
        //checks that page is being tracked
        if(!tracking(pageNamesToRemove[p]))
        {
            std::cout << "error: Nift is not tracking " << pageNamesToRemove[p] << std::endl;
            return 1;
        }

        pageToErase = get_info(pageNamesToRemove[p]);

        //removes the extension files if they exist
        extPath = pageToErase.pagePath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
        if(std::ifstream(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".pageExt";
        if(std::ifstream(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";
        if(std::ifstream(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }

        //removes page info file and containing dirs if now empty
        if(std::ifstream(pageToErase.pagePath.getInfoPath().dir))
        {
            chmod(pageToErase.pagePath.getInfoPath().str().c_str(), 0666);
            remove_path(pageToErase.pagePath.getInfoPath());
        }

        //removes page file and containing dirs if now empty
        if(std::ifstream(pageToErase.pagePath.dir))
        {
            chmod(pageToErase.pagePath.str().c_str(), 0666);
            remove_path(pageToErase.pagePath);
        }

        //removes content file and containing dirs if now empty
        if(std::ifstream(pageToErase.contentPath.dir))
        {
            chmod(pageToErase.contentPath.str().c_str(), 0666);
            remove_path(pageToErase.contentPath);
        }

        //removes page from pages set
        pages.erase(pageToErase);

        //informs user that page was successfully removed
        std::cout << "successfully removed " << pageNamesToRemove[p] << std::endl;
    }

    //saves new set of pages to pages.list
    save_pages();

    return 0;
}

int SiteInfo::mv(const Name& oldPageName, const Name& newPageName)
{
    if(newPageName.find('.') != std::string::npos)
    {
        std::cout << "error: new page name cannot contain '.'" << std::endl;
        return 1;
    }
    else if(newPageName == "")
    {
        std::cout << "error: new page name must be a non-empty string" << std::endl;
        return 1;
    }
    else if(unquote(newPageName).find('"') != std::string::npos && unquote(newPageName).find('\'') != std::string::npos)
    {
        std::cout << "error: new page name cannot contain both single and double quotes" << std::endl;
        return 1;
    }
    else if(!tracking(oldPageName)) //checks old page is being tracked
    {
        std::cout << "error: Nift is not tracking " << oldPageName << std::endl;
        return 1;
    }
    else if(tracking(newPageName)) //checks new page isn't already tracked
    {
        std::cout << "error: Nift is already tracking " << newPageName << std::endl;
        return 1;
    }

    PageInfo oldPageInfo = get_info(oldPageName);

    Path oldExtPath = oldPageInfo.pagePath.getInfoPath();
    std::string pageContExt = contentExt,
                pagePageExt = pageExt;

    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".contExt";
    if(std::ifstream(oldExtPath.str()))
    {
        std::ifstream ifs(oldExtPath.str());
        getline(ifs, pageContExt);
        ifs.close();
    }
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".pageExt";
    if(std::ifstream(oldExtPath.str()))
    {
        std::ifstream ifs(oldExtPath.str());
        getline(ifs, pagePageExt);
        ifs.close();
    }

    PageInfo newPageInfo;
    newPageInfo.pageName = newPageName;
    newPageInfo.contentPath.set_file_path_from(contentDir + newPageName + pageContExt);
    newPageInfo.pagePath.set_file_path_from(siteDir + newPageName + pagePageExt);
    if(get_title(oldPageInfo.pageName) == oldPageInfo.pageTitle.str)
        newPageInfo.pageTitle = get_title(newPageName);
    else
        newPageInfo.pageTitle = oldPageInfo.pageTitle;
    newPageInfo.templatePath = oldPageInfo.templatePath;

    //moves content file
    newPageInfo.contentPath.ensureDirExists();
    if(rename(oldPageInfo.contentPath.str().c_str(), newPageInfo.contentPath.str().c_str()))
    {
        std::cout << "error: mv: failed to move " << oldPageInfo.contentPath << " to " << newPageInfo.contentPath << std::endl;
        return 1;
    }

    //moves extension files if they exist
    Path newExtPath = newPageInfo.pagePath.getInfoPath();
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".contExt";
    if(std::ifstream(oldExtPath.str()))
    {
        newExtPath.file = newExtPath.file.substr(0, newExtPath.file.find_first_of('.')) + ".contExt";
        newExtPath.ensureDirExists();
        if(rename(oldExtPath.str().c_str(), newExtPath.str().c_str()))
        {
            std::cout << "error: mv: failed to move " << oldExtPath << " to " << newExtPath << std::endl;
            return 1;
        }
    }
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".pageExt";
    if(std::ifstream(oldExtPath.str()))
    {
        newExtPath.file = newExtPath.file.substr(0, newExtPath.file.find_first_of('.')) + ".pageExt";
        newExtPath.ensureDirExists();
        if(rename(oldExtPath.str().c_str(), newExtPath.str().c_str()))
        {
            std::cout << "error: mv: failed to move " << oldExtPath << " to " << newExtPath << std::endl;
            return 1;
        }
    }
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".scriptExt";
    if(std::ifstream(oldExtPath.str()))
    {
        newExtPath.file = newExtPath.file.substr(0, newExtPath.file.find_first_of('.')) + ".scriptExt";
        newExtPath.ensureDirExists();
        if(rename(oldExtPath.str().c_str(), newExtPath.str().c_str()))
        {
            std::cout << "error: mv: failed to move " << oldExtPath << " to " << newExtPath << std::endl;
            return 1;
        }
    }

    //removes page info file and containing dirs if now empty
    if(std::ifstream(oldPageInfo.pagePath.getInfoPath().dir))
    {
        chmod(oldPageInfo.pagePath.getInfoPath().str().c_str(), 0666);
        remove_path(oldPageInfo.pagePath.getInfoPath());
    }

    //removes page file and containing dirs if now empty
    if(std::ifstream(oldPageInfo.pagePath.dir))
    {
        chmod(oldPageInfo.pagePath.str().c_str(), 0666);
        remove_path(oldPageInfo.pagePath);
    }

    //removes oldPageInfo from pages
    pages.erase(oldPageInfo);
    //adds newPageInfo to pages
    pages.insert(newPageInfo);

    //saves new set of pages to pages.list
    save_pages();

    //informs user that page was successfully moved
    std::cout << "successfully moved " << oldPageName << " to " << newPageName << std::endl;

    return 0;
}

int SiteInfo::cp(const Name& trackedPageName, const Name& newPageName)
{
    if(newPageName.find('.') != std::string::npos)
    {
        std::cout << "error: new page name cannot contain '.'" << std::endl;
        return 1;
    }
    else if(newPageName == "")
    {
        std::cout << "error: new page name must be a non-empty string" << std::endl;
        return 1;
    }
    else if(unquote(newPageName).find('"') != std::string::npos && unquote(newPageName).find('\'') != std::string::npos)
    {
        std::cout << "error: new page name cannot contain both single and double quotes" << std::endl;
        return 1;
    }
    else if(!tracking(trackedPageName)) //checks old page is being tracked
    {
        std::cout << "error: Nift is not tracking " << trackedPageName << std::endl;
        return 1;
    }
    else if(tracking(newPageName)) //checks new page isn't already tracked
    {
        std::cout << "error: Nift is already tracking " << newPageName << std::endl;
        return 1;
    }

    PageInfo trackedPageInfo = get_info(trackedPageName);

    Path oldExtPath = trackedPageInfo.pagePath.getInfoPath();
    std::string pageContExt = contentExt,
                pagePageExt = pageExt;

    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".contExt";
    if(std::ifstream(oldExtPath.str()))
    {
        std::ifstream ifs(oldExtPath.str());
        getline(ifs, pageContExt);
        ifs.close();
    }
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".pageExt";
    if(std::ifstream(oldExtPath.str()))
    {
        std::ifstream ifs(oldExtPath.str());
        getline(ifs, pagePageExt);
        ifs.close();
    }

    PageInfo newPageInfo;
    newPageInfo.pageName = newPageName;
    newPageInfo.contentPath.set_file_path_from(contentDir + newPageName + pageContExt);
    newPageInfo.pagePath.set_file_path_from(siteDir + newPageName + pagePageExt);
    if(get_title(trackedPageInfo.pageName) == trackedPageInfo.pageTitle.str)
        newPageInfo.pageTitle = get_title(newPageName);
    else
        newPageInfo.pageTitle = trackedPageInfo.pageTitle;
    newPageInfo.templatePath = trackedPageInfo.templatePath;

    //copies content file
    if(cpFile(trackedPageInfo.contentPath.str(), newPageInfo.contentPath.str()))
    {
        std::cout << "error: cp: failed to copy " << trackedPageInfo.contentPath << " to " << newPageInfo.contentPath << std::endl;
        return 1;
    }

    //copies extension files if they exist
    Path newExtPath = newPageInfo.pagePath.getInfoPath();
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".contExt";
    if(std::ifstream(oldExtPath.str()))
    {
        newExtPath.file = newExtPath.file.substr(0, newExtPath.file.find_first_of('.')) + ".contExt";
        newExtPath.ensureDirExists();
        if(cpFile(oldExtPath.str(), newExtPath.str()))
        {
            std::cout << "error: cp: failed to copy " << oldExtPath << " to " << newExtPath << std::endl;
            return 1;
        }
        chmod(newExtPath.str().c_str(), 0444);
    }
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".pageExt";
    if(std::ifstream(oldExtPath.str()))
    {
        newExtPath.file = newExtPath.file.substr(0, newExtPath.file.find_first_of('.')) + ".pageExt";
        newExtPath.ensureDirExists();
        if(cpFile(oldExtPath.str(), newExtPath.str()))
        {
            std::cout << "error: cp: failed to copy " << oldExtPath << " to " << newExtPath << std::endl;
            return 1;
        }
        chmod(newExtPath.str().c_str(), 0444);
    }
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".scriptExt";
    if(std::ifstream(oldExtPath.str()))
    {
        newExtPath.file = newExtPath.file.substr(0, newExtPath.file.find_first_of('.')) + ".scriptExt";
        newExtPath.ensureDirExists();
        if(cpFile(oldExtPath.str(), newExtPath.str()))
        {
            std::cout << "error: cp: failed to copy " << oldExtPath << " to " << newExtPath << std::endl;
            return 1;
        }
        chmod(newExtPath.str().c_str(), 0444);
    }

    //adds newPageInfo to pages
    pages.insert(newPageInfo);

    //saves new set of pages to pages.list
    save_pages();

    //informs user that page was successfully moved
    std::cout << "successfully copied " << trackedPageName << " to " << newPageName << std::endl;

    return 0;
}

int SiteInfo::new_title(const Name& pageName, const Title& newTitle)
{
    if(newTitle.str == "")
    {
        std::cout << "error: new title must be a non-empty string" << std::endl;
        return 1;
    }
    else if(unquote(newTitle.str).find('"') != std::string::npos && unquote(newTitle.str).find('\'') != std::string::npos)
    {
        std::cout << "error: new title cannot contain both single and double quotes" << std::endl;
        return 1;
    }

    PageInfo pageInfo;
    pageInfo.pageName = pageName;
    if(pages.count(pageInfo))
    {
        pageInfo = *(pages.find(pageInfo));
        pages.erase(pageInfo);
        pageInfo.pageTitle = newTitle;
        pages.insert(pageInfo);
        save_pages();

        //informs user that page title was successfully changed
        std::cout << "successfully changed page title to " << quote(newTitle.str) << std::endl;
    }
    else
    {
        std::cout << "error: Nift is not tracking " << quote(pageName) << std::endl;
        return 1;
    }

    return 0;
}

int SiteInfo::new_template(const Path& newTemplatePath)
{
    if(newTemplatePath == Path("", ""))
    {
        std::cout << "error: new template path must be a non-empty string" << std::endl;
        return 1;
    }
    else if(unquote(newTemplatePath.str()).find('"') != std::string::npos && unquote(newTemplatePath.str()).find('\'') != std::string::npos)
    {
        std::cout << "error: new template path cannot contain both single and double quotes" << std::endl;
        return 1;
    }

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
    remove_path(Path(".siteinfo/", "pages.list-old"));

    //sets new template
    defaultTemplate = newTemplatePath;

    //saves new default template to Nift config file
    save_config();

    //warns user if new template path doesn't exist
    if(!std::ifstream(newTemplatePath.str()))
        std::cout << "warning: new template path " << newTemplatePath << " does not exist" << std::endl;

    std::cout << "successfully changed default template to " << newTemplatePath << std::endl;

    return 0;
}

int SiteInfo::new_template(const Name& pageName, const Path& newTemplatePath)
{
    if(newTemplatePath == Path("", ""))
    {
        std::cout << "error: new template path must be a non-empty string" << std::endl;
        return 1;
    }
    else if(unquote(newTemplatePath.str()).find('"') != std::string::npos && unquote(newTemplatePath.str()).find('\'') != std::string::npos)
    {
        std::cout << "error: new template path cannot contain both single and double quotes" << std::endl;
        return 1;
    }

    PageInfo pageInfo;
    pageInfo.pageName = pageName;
    if(pages.count(pageInfo))
    {
        pageInfo = *(pages.find(pageInfo));
        pages.erase(pageInfo);
        pageInfo.templatePath = newTemplatePath;
        pages.insert(pageInfo);
        save_pages();

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

int SiteInfo::new_site_dir(const Directory& newSiteDir)
{
    if(newSiteDir == "")
    {
        std::cout << "error: new site directory cannot be the empty string" << std::endl;
        return 1;
    }
    else if(unquote(newSiteDir).find('"') != std::string::npos && unquote(newSiteDir).find('\'') != std::string::npos)
    {
        std::cout << "error: new site directory cannot contain both single and double quotes" << std::endl;
        return 1;
    }

    if(newSiteDir[newSiteDir.size()-1] != '/' && newSiteDir[newSiteDir.size()-1] != '\\')
    {
        std::cout << "error: new site directory should end in \\ or /" << std::endl;
        return 1;
    }

    if(newSiteDir == siteDir)
    {
        std::cout << "error: site directory is already " << quote(newSiteDir) << std::endl;
        return 1;
    }

    if(std::ifstream(newSiteDir) || std::ifstream(newSiteDir.substr(0, newSiteDir.size()-1)))
    {
        std::cout << "error: new site directory location " << quote(newSiteDir) << " already exists" << std::endl;
        return 1;
    }

    std::string newInfoDir = ".siteinfo/" + newSiteDir;

    if(std::ifstream(newInfoDir) || std::ifstream(newInfoDir.substr(0, newInfoDir.size()-1)))
    {
        std::cout << "error: new page info directory location " << quote(newInfoDir) << " already exists" << std::endl;
        return 1;
    }

    std::string parDir = "../",
                rootDir = "/",
                siteRootDir = get_pwd(),
                pwd = get_pwd();

    int ret_val = chdir(siteDir.c_str());
    if(ret_val)
    {
        std::cout << "error: SiteInfo.cpp: new_site_dir(" << quote(newSiteDir) << "): failed to change directory to " << quote(siteDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    ret_val = chdir(parDir.c_str());
    if(ret_val)
    {
        std::cout << "error: SiteInfo.cpp: new_site_dir(" << quote(newSiteDir) << "): failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    std::string delDir = get_pwd();
    ret_val = chdir(siteRootDir.c_str());
    if(ret_val)
    {
        std::cout << "error: SiteInfo.cpp: new_site_dir(" << quote(newSiteDir) << "): failed to change directory to " << quote(siteRootDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    //moves site directory to temp location
    rename(siteDir.c_str(), ".temp_site_dir");

    //ensures new site directory exists
    Path(newSiteDir, Filename("")).ensureDirExists();

    //moves site directory to final location
    rename(".temp_site_dir", newSiteDir.c_str());

    //deletes any remaining empty directories
    if(remove_path(Path(delDir, "")))
    {
        std::cout << "error: SiteInfo.cpp: new_site_dir(" << quote(newSiteDir) << "): failed to remove " << quote(delDir) << std::endl;
        return 1;
    }

    //changes back to site root directory
    ret_val = chdir(siteRootDir.c_str());
    if(ret_val)
    {
        std::cout << "error: SiteInfo.cpp: new_site_dir(" << quote(newSiteDir) << "): failed to change directory to " << quote(siteRootDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    ret_val = chdir((".siteinfo/" + siteDir).c_str());
    if(ret_val)
    {
        std::cout << "error: SiteInfo.cpp: new_site_dir(" << quote(newSiteDir) << "): failed to change directory to " << quote(".siteinfo/" + siteDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    ret_val = chdir(parDir.c_str());
    if(ret_val)
    {
        std::cout << "error: SiteInfo.cpp: new_site_dir(" << quote(newSiteDir) << "): failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    delDir = get_pwd();
    ret_val = chdir(siteRootDir.c_str());
    if(ret_val)
    {
        std::cout << "error: SiteInfo.cpp: new_site_dir(" << quote(newSiteDir) << "): failed to change directory to " << quote(siteRootDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    //moves old info directory to temp location
    rename((".siteinfo/" + siteDir).c_str(), ".temp_site_dir");

    //ensures new info directory exists
    Path(".siteinfo/" + newSiteDir, Filename("")).ensureDirExists();

    //moves old info directory to final location
    rename(".temp_site_dir", (".siteinfo/" + newSiteDir).c_str());

    //deletes any remaining empty directories
    if(remove_path(Path(delDir, "")))
    {
        std::cout << "error: SiteInfo.cpp: new_site_dir(" << quote(newSiteDir) << "): failed to remove " << quote(delDir) << std::endl;
        return 1;
    }

    //changes back to site root directory
    ret_val = chdir(siteRootDir.c_str());
    if(ret_val)
    {
        std::cout << "error: SiteInfo.cpp: new_site_dir(" << quote(newSiteDir) << "): failed to change directory to " << quote(siteRootDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    //sets new site directory
    siteDir = newSiteDir;

    //saves new site directory to Nift config file
    save_config();

    std::cout << "successfully changed site directory to " << quote(newSiteDir) << std::endl;

    return 0;
}

int SiteInfo::new_content_dir(const Directory& newContDir)
{
    if(newContDir == "")
    {
        std::cout << "error: new content directory cannot be the empty string" << std::endl;
        return 1;
    }
    else if(unquote(newContDir).find('"') != std::string::npos && unquote(newContDir).find('\'') != std::string::npos)
    {
        std::cout << "error: new content directory cannot contain both single and double quotes" << std::endl;
        return 1;
    }

    if(newContDir[newContDir.size()-1] != '/' && newContDir[newContDir.size()-1] != '\\')
    {
        std::cout << "error: new site directory should end in \\ or /" << std::endl;
        return 1;
    }

    if(newContDir == contentDir)
    {
        std::cout << "error: content directory is already " << quote(newContDir) << std::endl;
        return 1;
    }

    if(std::ifstream(newContDir) || std::ifstream(newContDir.substr(0, newContDir.size()-1)))
    {
        std::cout << "error: new content directory location " << quote(newContDir) << " already exists" << std::endl;
        return 1;
    }

    std::string parDir = "../",
                rootDir = "/",
                siteRootDir = get_pwd(),
                pwd = get_pwd();

    int ret_val = chdir(contentDir.c_str());
    if(ret_val)
    {
        std::cout << "error: SiteInfo.cpp: new_content_dir(" << quote(newContDir) << "): failed to change directory to " << quote(contentDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    ret_val = chdir(parDir.c_str());
    if(ret_val)
    {
        std::cout << "error: SiteInfo.cpp: new_content_dir(" << quote(newContDir) << "): failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    std::string delDir = get_pwd();
    ret_val = chdir(siteRootDir.c_str());
    if(ret_val)
    {
        std::cout << "error: SiteInfo.cpp: new_content_dir(" << quote(newContDir) << "): failed to change directory to " << quote(siteRootDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    //moves content directory to temp location
    rename(contentDir.c_str(), ".temp_cont_dir");

    //ensures new site directory exists
    Path(newContDir, Filename("")).ensureDirExists();

    //moves content directory to final location
    rename(".temp_cont_dir", newContDir.c_str());

    //deletes any remaining empty directories
    if(remove_path(Path(delDir, "")))
    {
        std::cout << "error: SiteInfo.cpp: new_content_dir(" << quote(newContDir) << "): failed to remove " << quote(delDir) << std::endl;
        return 1;
    }

    //changes back to site root directory
    ret_val = chdir(siteRootDir.c_str());
    if(ret_val)
    {
        std::cout << "error: SiteInfo.cpp: new_content_dir(" << quote(newContDir) << "): failed to change directory to " << quote(siteRootDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    //sets new site directory
    contentDir = newContDir;

    //saves new site directory to Nift config file
    save_config();

    std::cout << "successfully changed content directory to " << quote(newContDir) << std::endl;

    return 0;
}

int SiteInfo::new_content_ext(const std::string& newExt)
{
    if(newExt == "" || newExt[0] != '.')
    {
        std::cout << "error: page extension must start with a fullstop" << std::endl;
        return 1;
    }

    if(newExt == contentExt)
    {
        std::cout << "error: content extension is already " << quote(contentExt) << std::endl;
        return 1;
    }

    contentExt = newExt;

    save_config();

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

            if(oldExt == newExt)
            {
                chmod(extPath.str().c_str(), 0666);
                remove_path(extPath);
            }
        }
        else
        {
            //moves the content file
            if(std::ifstream(page->contentPath.str()))
            {
                Path newContPath = page->contentPath;
                newContPath.file = newContPath.file.substr(0, newContPath.file.find_first_of('.')) + newExt;
                if(newContPath.str() != page->contentPath.str())
                    rename(page->contentPath.str().c_str(), newContPath.str().c_str());
            }
        }
    }

    //informs user that page extension was successfully changed
    std::cout << "successfully changed content extension to " << quote(newExt) << std::endl;

    return 0;
}

int SiteInfo::new_content_ext(const Name& pageName, const std::string& newExt)
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

        std::string oldExt;
        if(std::ifstream(extPath.str()))
        {
            std::ifstream ifs(extPath.str());
            getline(ifs, oldExt);
            ifs.close();
        }
        else
            oldExt = contentExt;

        if(oldExt == newExt)
        {
            std::cout << "error: content extension for " << quote(pageName) << " is already " << quote(oldExt) << std::endl;
            return 1;
        }

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
            remove_path(extPath);

        //moves the content file
        if(std::ifstream(pageInfo.contentPath.str()))
        {
            Path newContPath = pageInfo.contentPath;
            newContPath.file = newContPath.file.substr(0, newContPath.file.find_first_of('.')) + newExt;
            if(newContPath.str() != pageInfo.contentPath.str())
                rename(pageInfo.contentPath.str().c_str(), newContPath.str().c_str());
        }
    }
    else
    {
        std::cout << "error: Nift is not tracking " << quote(pageName) << std::endl;
        return 1;
    }

    return 0;
}

int SiteInfo::new_page_ext(const std::string& newExt)
{
    if(newExt == "" || newExt[0] != '.')
    {
        std::cout << "error: page extension must start with a fullstop" << std::endl;
        return 1;
    }

    if(newExt == pageExt)
    {
        std::cout << "error: page extension is already " << quote(pageExt) << std::endl;
        return 1;
    }

    pageExt = newExt;

    save_config();

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

            if(oldExt == newExt)
            {
                chmod(extPath.str().c_str(), 0666);
                remove_path(extPath);
            }
        }
        else
        {
            //moves the built page
            if(std::ifstream(page->pagePath.str()))
            {
                Path newPagePath = page->pagePath;
                newPagePath.file = newPagePath.file.substr(0, newPagePath.file.find_first_of('.')) + newExt;
                if(newPagePath.str() != page->pagePath.str())
                    rename(page->pagePath.str().c_str(), newPagePath.str().c_str());
            }
        }
    }

    //informs user that page extension was successfully changed
    std::cout << "successfully changed page extension to " << quote(newExt) << std::endl;

    return 0;
}

int SiteInfo::new_page_ext(const Name& pageName, const std::string& newExt)
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

        std::string oldExt;
        if(std::ifstream(extPath.str()))
        {
            std::ifstream ifs(extPath.str());
            getline(ifs, oldExt);
            ifs.close();
        }
        else
            oldExt = pageExt;

        if(oldExt == newExt)
        {
            std::cout << "error: page extension for " << quote(pageName) << " is already " << quote(oldExt) << std::endl;
            return 1;
        }

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
            remove_path(extPath);

        //moves the built page
        if(std::ifstream(pageInfo.pagePath.str()))
        {
            Path newPagePath = pageInfo.pagePath;
            newPagePath.file = newPagePath.file.substr(0, newPagePath.file.find_first_of('.')) + newExt;
            if(newPagePath.str() != pageInfo.pagePath.str())
                rename(pageInfo.pagePath.str().c_str(), newPagePath.str().c_str());
        }
    }
    else
    {
        std::cout << "error: Nift is not tracking " << quote(pageName) << std::endl;
        return 1;
    }

    return 0;
}

int SiteInfo::new_script_ext(const std::string& newExt)
{
    if(newExt != "" && newExt[0] != '.')
    {
        std::cout << "error: non-empty script extension must start with a fullstop" << std::endl;
        return 1;
    }

    if(newExt == scriptExt)
    {
        std::cout << "error: script extension is already " << quote(scriptExt) << std::endl;
        return 1;
    }

    scriptExt = newExt;

    save_config();

    for(auto page=pages.begin(); page!=pages.end(); page++)
    {
        //checks for non-default page extension
        Path extPath = page->pagePath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";

        if(std::ifstream(extPath.str()))
        {
            std::ifstream ifs(extPath.str());
            std::string oldExt;

            read_quoted(ifs, oldExt);
            ifs.close();

            if(oldExt == newExt)
            {
                chmod(extPath.str().c_str(), 0666);
                remove_path(extPath);
            }
        }
    }

    //informs user that page extension was successfully changed
    std::cout << "successfully changed script extension to " << quote(newExt) << std::endl;

    return 0;
}

int SiteInfo::new_script_ext(const Name& pageName, const std::string& newExt)
{
    if(newExt != "" && newExt[0] != '.')
    {
        std::cout << "error: non-empty script extension must start with a fullstop" << std::endl;
        return 1;
    }

    PageInfo pageInfo = get_info(pageName);
    if(pages.count(pageInfo))
    {
        //checks for non-default script extension
        Path extPath = pageInfo.pagePath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";

        std::string oldExt;
        if(std::ifstream(extPath.str()))
        {
            std::ifstream ifs(extPath.str());
            getline(ifs, oldExt);
            ifs.close();
        }
        else
            oldExt = scriptExt;

        if(oldExt == newExt)
        {
            std::cout << "error: script extension for " << quote(pageName) << " is already " << quote(oldExt) << std::endl;
            return 1;
        }

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
            remove_path(extPath);
    }
    else
    {
        std::cout << "error: Nift is not tracking " << quote(pageName) << std::endl;
        return 1;
    }

    return 0;
}

int SiteInfo::no_build_threads(int no_threads)
{
    if(no_threads == 0)
    {
        std::cout << "error: number of build threads must be a non-zero integer (use negative numbers for a multiple of the number of cores on the machine)" << std::endl;
        return 1;
    }

    if(no_threads == buildThreads)
    {
        std::cout << "error: number of build threads is already " << buildThreads << std::endl;
        return 1;
    }

    buildThreads = no_threads;
    save_config();

    std::cout << "successfully changed number of build threads to " << no_threads << std::endl;

    return 0;
}

int SiteInfo::check_watch_dirs()
{
    if(std::ifstream(".siteinfo/.watchinfo/watching.list"))
    {
        WatchList wl;
        if(wl.open())
        {
            std::cout << "error: failed to open watch list '.siteinfo/.watchinfo/watching.list'" << std::endl;
            return 1;
        }

        for(auto wd=wl.dirs.begin(); wd!=wl.dirs.end(); wd++)
        {
            std::string file,
                        watchDirFilesStr = ".siteinfo/.watchinfo/" + replace_slashes(wd->watchDir) + ".tracked";
            PageInfo pageinfo;

            std::ifstream ifs(watchDirFilesStr);
            std::vector<Name> pageNamesToRemove;
            while(read_quoted(ifs, file))
            {
                pageinfo = get_info(file);
                if(pageinfo.pageName != "##not-found##")
                    if(!std::ifstream(pageinfo.contentPath.str()))
                        pageNamesToRemove.push_back(file);
            }
            ifs.close();

            if(pageNamesToRemove.size())
                rm(pageNamesToRemove);

            std::vector<TrackInfo> pages_to_track;
            std::vector<Name> pages_tracked;
            std::string ext;
            Name pagename;
            size_t pos;
            std::vector<std::string> files = lsVec(wd->watchDir.c_str());
            for(size_t f=0; f<files.size(); f++)
            {
                pos = files[f].find_first_of('.');
                if(pos != std::string::npos)
                {
                    ext = files[f].substr(pos, files[f].size()-pos);
                    if(wd->watchDir.size() == contentDir.size())
                        pagename = files[f].substr(0, pos);
                    else
                        pagename = wd->watchDir.substr(contentDir.size(), wd->watchDir.size()-contentDir.size()) + files[f].substr(0, pos);

                    if(wd->contExts.count(ext)) //we are watching the extension
                    {
                        if(!tracking(pagename))
                            pages_to_track.push_back(TrackInfo(pagename, Title(pagename), wd->templatePaths.at(ext), ext, wd->pageExts.at(ext)));

                        pages_tracked.push_back(pagename);
                    }
                }
            }

            if(pages_to_track.size())
                track(pages_to_track);

            //makes sure we can write to watchDir tracked file
            chmod(watchDirFilesStr.c_str(), 0644);

            std::ofstream ofs(watchDirFilesStr);

            for(size_t f=0; f<pages_tracked.size(); f++)
                ofs << quote(pages_tracked[f]) << std::endl;

            ofs.close();

            //makes sure user can't accidentally write to watchDir tracked file
            chmod(watchDirFilesStr.c_str(), 0444);
        }
    }

    return 0;
}

std::mutex os_mtx;

int SiteInfo::build(const std::vector<Name>& pageNamesToBuild)
{
    check_watch_dirs();

    PageBuilder pageBuilder(&pages, &os_mtx, contentDir, siteDir, contentExt, pageExt, scriptExt, defaultTemplate, backupScripts, unixTextEditor, winTextEditor);
    std::set<Name> untrackedPages, failedPages;

    //checks for pre-build scripts
    if(run_script(std::cout, "pre-build" + scriptExt, backupScripts, &os_mtx3))
        return 1;

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
    else if(untrackedPages.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "---- Nift not tracking following pages ----" << std::endl;
        for(auto uName=untrackedPages.begin(); uName != untrackedPages.end(); uName++)
            std::cout << " " << *uName << std::endl;
        std::cout << "-------------------------------------------" << std::endl;
    }
    else if(failedPages.size() == 0 && untrackedPages.size() == 0)
    {
        std::cout << "all " << pageNamesToBuild.size() << " pages built successfully" << std::endl;

        //checks for post-build scripts
        if(run_script(std::cout, "post-build" + scriptExt, backupScripts, &os_mtx3))
            return 1;
    }

    return 0;
}

std::mutex fail_mtx, built_mtx, set_mtx;
std::set<Name> failedPages, builtPages;
std::set<PageInfo>::iterator cPage;

std::atomic<int> counter;

void build_thread(std::ostream& os,
                  std::set<PageInfo>* pages,
                  const int& no_pages,
                  const Directory& ContentDir,
                  const Directory& SiteDir,
                  const std::string& ContentExt,
                  const std::string& PageExt,
                  const std::string& ScriptExt,
                  const Path& DefaultTemplate,
                  const bool& BackupScripts,
                  const std::string& UnixTextEditor,
                  const std::string& WinTextEditor)
{
    PageBuilder pageBuilder(pages, &os_mtx, ContentDir, SiteDir, ContentExt, PageExt, ScriptExt, DefaultTemplate, BackupScripts, UnixTextEditor, WinTextEditor);
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
    int no_threads;
    if(buildThreads < 0)
        no_threads = -buildThreads*std::thread::hardware_concurrency();
    else
        no_threads = buildThreads;

    std::set<Name> untrackedPages;

    check_watch_dirs();

    //checks for pre-build scripts
    if(run_script(std::cout, "pre-build" + scriptExt, backupScripts, &os_mtx3))
        return 1;

    cPage = pages.begin();
    counter = 0;

	std::vector<std::thread> threads;
	for(int i=0; i<no_threads; i++)
		threads.push_back(std::thread(build_thread, std::ref(std::cout), &pages, pages.size(), contentDir, siteDir, contentExt, pageExt, scriptExt, defaultTemplate, backupScripts, unixTextEditor, winTextEditor));

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
    else if(untrackedPages.size() > 0)
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
    else
    {
        std::cout << "all " << builtPages.size() << " pages built successfully" << std::endl;

        //checks for post-build scripts
        if(run_script(std::cout, "post-build" + scriptExt, backupScripts, &os_mtx3))
            return 1;
    }

    return 0;
}

std::mutex problem_mtx, updated_mtx, modified_mtx, removed_mtx;
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
    check_watch_dirs();

    builtPages.clear();
    failedPages.clear();
    problemPages.clear();
    updatedPages.clear();
    modifiedFiles.clear();
    removedFiles.clear();

    int no_threads;
    if(buildThreads < 0)
        no_threads = -buildThreads*std::thread::hardware_concurrency();
    else
        no_threads = buildThreads;

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

        //checks for pre-build scripts
        if(run_script(std::cout, "pre-build" + scriptExt, backupScripts, &os_mtx3))
            return 1;

        cPage = updatedPages.begin();
        counter = 0;

        threads.clear();
        for(int i=0; i<no_threads; i++)
            threads.push_back(std::thread(build_thread, std::ref(os), &pages, updatedPages.size(), contentDir, siteDir, contentExt, pageExt, scriptExt, defaultTemplate, backupScripts, unixTextEditor, winTextEditor));

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
        else
        {
            //checks for post-build scripts
            if(run_script(std::cout, "post-build" + scriptExt, backupScripts, &os_mtx3))
                return 1;
        }
    }

    if(updatedPages.size() == 0 && problemPages.size() == 0 && failedPages.size() == 0)
    {
        //os << std::endl;
        os << "all " << pages.size() << " pages are already up to date" << std::endl;
    }

	builtPages.clear();
	failedPages.clear();

    return 0;
}

int SiteInfo::status()
{
    if(pages.size() == 0)
    {
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
        std::cout << "all pages are already up to date" << std::endl;

    return 0;
}
