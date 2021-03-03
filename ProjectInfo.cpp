#include "ProjectInfo.h"

#if defined __APPLE__ || defined __linux__
    const std::string promptStr = "⥤ ";
#else
    const std::string promptStr = "=> ";
#endif

int create_default_html_template(const Path& templatePath)
{
    templatePath.ensureDirExists();

    std::ofstream ofs(templatePath.str());
    ofs << "<html>" << std::endl;
    ofs << "\t<head>" << std::endl;
    ofs << "\t\t@input(\"" <<  templatePath.dir << "head.content\")" << std::endl;
    ofs << "\t</head>" << std::endl << std::endl;
    ofs << "\t<body>" << std::endl;
    ofs << "\t\t@content" << std::endl;
    ofs << "\t</body>" << std::endl;
    ofs << "</html>" << std::endl << std::endl;
    ofs.close();

    ofs.open(Path(templatePath.dir, "head.content").str());
    ofs << "<title>empty site - $[title]</title>" << std::endl;
    ofs.close();

    return 0;
}

int create_blank_template(const Path& templatePath)
{
    templatePath.ensureDirExists();

    std::ofstream ofs(templatePath.str());
    ofs << "@content" << std::endl;
    ofs.close();

    return 0;
}

int create_config_file(const Path& configPath, const std::string& outputExt, bool global)
{
    configPath.ensureDirExists();

    ProjectInfo project;

    if(!global)
    {
        project.contentDir = "content/";
        project.contentExt = ".content";
        project.outputDir = "output/";
        project.outputExt = outputExt;
        project.scriptExt = ".f";
    }

    if(global)
    {
        project.defaultTemplate = Path(app_dir() + "/.nift/template/", "global.template");
        create_default_html_template(project.defaultTemplate);
    }
    else if(outputExt == ".html" || outputExt == ".php")
        project.defaultTemplate = Path("template/", "page.template");
    else
        project.defaultTemplate = Path("template/", "project.template");

    project.backupScripts = 1;

    project.buildThreads = -1;
    project.paginateThreads = -1;

    project.incrMode = INCR_MOD;

    project.terminal = "normal";

    project.unixTextEditor = "nano";
    project.winTextEditor = "notepad";

    if(!global)
    {
        if(dir_exists(".git/")) //should really be checking if outputDir exists and has .git directory
        {
            if(get_pb(project.outputBranch))
            {
                start_err(std::cout) << "failed to get present branch" << std::endl;
                return 1;
            }
            project.rootBranch = project.outputBranch;
        }
        else
            project.rootBranch = project.outputBranch = "##unset##";
    }

    project.save_config(configPath.str(), global);

    return 0;
}

bool upgradeProject()
{
    start_warn(std::cout) << "attempting to upgrade project for newer version of Nift" << std::endl;

    if(rename(".siteinfo/", ".nsm/"))
    {
        start_err(std::cout) << "upgrade: failed to move config directory '.siteinfo/' to '.nsm/'" << std::endl;
        return 1;
    }

    if(rename(".nsm/nsm.config", ".nsm/nift.config"))
    {
        start_err(std::cout) << "upgrade: failed to move config file '.nsm/nsm.config' to '.nsm/nift.config'" << std::endl;
        return 1;
    }

    if(rename(".nsm/pages.list", ".nsm/tracking.list"))
    {
        start_err(std::cout) << "upgrade: failed to move pages list file '.nsm/pages.list' to tracking list file '.nsm/tracking.list'" << std::endl;
        return 1;
    }

    ProjectInfo project;
    if(project.open_local_config(1))
    {
        start_err(std::cout) << "upgrade: failed, was unable to open configuration file" << std::endl;
        return 1;
    }
    else if(project.open_tracking(1)) //this needs to change
    {
        start_err(std::cout) << "upgrade: failed, was unable to open tracking list" << std::endl;
        return 1;
    }

    //need to update output extension files
    Path oldExtPath, newExtPath;
    for(auto tInfo=project.trackedAll.begin(); tInfo!=project.trackedAll.end(); tInfo++)
    {
        oldExtPath = newExtPath = tInfo->outputPath.getInfoPath();
        oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".pageExt";
        newExtPath.file = newExtPath.file.substr(0, newExtPath.file.find_first_of('.')) + ".outputExt";

        if(file_exists(oldExtPath.str()))
        {
            if(rename(oldExtPath.str().c_str(), newExtPath.str().c_str()))
            {
                start_err(std::cout) << "upgrade: failed to move output extension file " << quote(oldExtPath.str()) << " to new extension file " <<  quote(newExtPath.str()) << std::endl;
                return 1;
            }
        }
    }

    std::cout << "Successfully upgraded project configuration for newer version of Nift" << std::endl;
    std::cout << c_light_blue << "note" << c_white << ": the syntax to input page and project/site information has also changed, please check the content files page of the docs" << std::endl;

    return 0;
}

int ProjectInfo::open(const bool& addMsg)
{
    if(open_local_config(addMsg))
        return 1;
    if(open_tracking(addMsg))
        return 1;

    return 0;
}

int ProjectInfo::open_config(const Path& configPath, const bool& global, const bool& addMsg)
{
    if(!file_exists(configPath.str()))
    {
        if(global)
            create_config_file(configPath, ".html", 1);
        else
        {
            start_err(std::cout) << "open_config(): could not load Nift project config file as '" << get_pwd() <<  "/.nsm/nift.config' does not exist" << std::endl;

            return 1;
        }
    }

    if(addMsg)
    {
        std::cout << "loading ";
        if(global)
            std::cout << "global ";
        else
            std::cout << "project ";
        std::cout << "config file: " << configPath << std::endl;
        //std::cout << std::flush;
        std::fflush(stdout);
    }

    contentDir = outputDir = "";
    backupScripts = 1;
    buildThreads = paginateThreads = 0;
    incrMode = INCR_MOD;
    contentExt = outputExt = scriptExt = unixTextEditor = winTextEditor = rootBranch = outputBranch = "";
    defaultTemplate = Path("", "");

    //reads Nift config file
    std::ifstream ifs(configPath.str());
    bool configChanged = 0;
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
            else if(inType == "outputDir")
            {
                read_quoted(iss, outputDir);
                outputDir = comparable(outputDir);
            }
            else if(inType == "siteDir") //can delete this later
            {
                read_quoted(iss, outputDir);
                outputDir = comparable(outputDir);
                configChanged = 1;
                //start_warn(std::cout, Path(".nsm/", "nift.config"), lineNo) << "updated siteDir to outputDir" << std::endl;
            }
            else if(inType == "outputExt")
                read_quoted(iss, outputExt);
            else if(inType == "pageExt") //can delete this later
            {
                read_quoted(iss, outputExt);
                configChanged = 1;
                //start_warn(std::cout, Path(".nsm/", "nift.config"), lineNo) << "updated pageExt to outputExt" << std::endl;
            }
            else if(inType == "scriptExt")
                read_quoted(iss, scriptExt);
            else if(inType == "defaultTemplate")
                defaultTemplate.read_file_path_from(iss);
            else if(inType == "backupScripts")
                iss >> backupScripts;
            else if(inType == "buildThreads")
                iss >> buildThreads;
            else if(inType == "paginateThreads")
                iss >> paginateThreads;
            else if(inType == "incrementalMode")
                iss >> incrMode;
            else if(inType == "terminal")
            {
                read_quoted(iss, terminal);

                #if defined _WIN32 || defined _WIN64
                    if(terminal == "ps" || terminal == "powershell")
                        use_powershell_colours();
                #endif
            }
            else if(inType == "unixTextEditor")
                read_quoted(iss, unixTextEditor);
            else if(inType == "winTextEditor")
                read_quoted(iss, winTextEditor);
            else if(inType == "rootBranch")
                read_quoted(iss, rootBranch);
            else if(inType == "outputBranch")
                read_quoted(iss, outputBranch);
            else if(inType == "siteBranch") // can delete this later
            {
                read_quoted(iss, outputBranch);
                configChanged = 1;
                std::cout << std::endl;
                start_warn(std::cout, configPath, lineNo) << "updated siteBranch to outputBranch" << std::endl;
            }
            else
            {
                continue;
                //if we throw error here can't compile projects for newer versions with older versions of Nift
                //std::cout << std::endl;
                //start_err(std::cout, configPath, lineNo) << "do not recognise confirguration parameter " << inType << std::endl;
                //return 1;
            }

            iss >> str;
            if(str != "" && str[0] != '#')
            {
                start_err(std::cout, configPath, lineNo) << "was not expecting anything on this line from " << c_light_blue << quote(str) << c_white << " onwards" << std::endl;
                std::cout << c_aqua << "note: " << c_white << "you can comment out the remainder of a line with #" << std::endl;

                ifs.close();
                return 1;
            }
        }
    }
    ifs.close();

    add_colour();

    if(!global)
    {
        if(contentDir == "")
        {
            start_err(std::cout, configPath) << "content directory not specified" << std::endl;
            return 1;
        }
        if(contentExt == "" || contentExt[0] != '.')
        {
            start_err(std::cout, configPath) << "content extension must start with a fullstop" << std::endl;
            return 1;
        }

        if(outputDir == "")
        {
            start_err(std::cout, configPath) << "output directory not specified" << std::endl;
            return 1;
        }
        if(outputExt == "" || outputExt[0] != '.')
        {
            start_err(std::cout, configPath) << "output extension must start with a fullstop" << std::endl;
            return 1;
        }

        if(scriptExt != "" && scriptExt [0] != '.')
        {
            start_err(std::cout, configPath) << "script extension must start with a fullstop" << std::endl;
            return 1;
        }
        else if(scriptExt == "")
        {
            start_warn(std::cout, configPath) << "script extension not detected, set to default of '.f'" << std::endl;

            scriptExt = ".f";

            configChanged = 1;
        }
    }

    if(buildThreads == 0)
    {
        start_warn(std::cout, configPath) << "number of build threads not detected or invalid, set to default of -1 (number of cores)" << std::endl;

        buildThreads = -1;

        configChanged = 1;
    }

    if(paginateThreads == 0)
    {
        start_warn(std::cout, configPath) << "number of paginate threads not detected or invalid, set to default of -1 (number of cores)" << std::endl;

        paginateThreads = -1;

        configChanged = 1;
    }

    //code to set unixTextEditor if not present
    if(unixTextEditor == "")
    {
        start_warn(std::cout, configPath) << "unix text editor not detected, set to default of 'nano'" << std::endl;

        unixTextEditor = "nano";

        configChanged = 1;
    }

    //code to set winTextEditor if not present
    if(winTextEditor == "")
    {
        start_warn(std::cout, configPath) << "windows text editor not detected, set to default of 'notepad'" << std::endl;

        winTextEditor = "notepad";

        configChanged = 1;
    }

    if(!global)
    {
        //code to figure out rootBranch and outputBranch if not present
        if(rootBranch == "" || outputBranch == "")
        {
            start_warn(std::cout, configPath) << "root or output branch not detected, have attempted to determine them, ensure values in config file are correct" << std::endl;

            if(dir_exists(".git"))
            {
                std::set<std::string> branches;

                if(get_git_branches(branches))
                {
                    start_err(std::cout, configPath) << "open_config: failed to retrieve git branches" << std::endl;
                    return 1;
                }

                if(branches.count("stage"))
                {
                    rootBranch = "stage";
                    outputBranch = "master";
                }
                else
                    rootBranch = outputBranch = "master";
            }
            else
                rootBranch = outputBranch = "##unset##";

            configChanged = 1;
        }
    }

    //clear_console_line();

    if(configChanged)
        save_config(configPath.str(), global);

    return 0;
}

int ProjectInfo::open_global_config(const bool& addMsg)
{
    return open_config(Path(app_dir() + "/.nift/", "nift.config"), 1, addMsg);
}

int ProjectInfo::open_local_config(const bool& addMsg)
{
    return open_config(Path(".nsm/", "nift.config"), 0, addMsg);
}

int ProjectInfo::open_tracking(const bool& addMsg)
{
    if(addMsg)
        std::cout << "loading project tracking file: " << Path(".nsm/", "tracking.list") << std::endl;
    //std::cout << std::flush;
    std::fflush(stdout);

    trackedAll.clear();

    if(!file_exists(".nsm/tracking.list"))
    {
        start_err(std::cout) << "open_tracking(): could not load tracking information as '" << get_pwd() << "/.nsm/tracking.list' does not exist" << std::endl;
        return 1;
    }

    //reads tracking list file
    std::ifstream ifs(".nsm/tracking.list"), 
                  ifsx;
    Name inName;
    Title inTitle;
    Path inTemplatePath;
    std::string inExt;
    while(read_quoted(ifs, inName))
    {
        inTitle.read_quoted_from(ifs);
        inTemplatePath.read_file_path_from(ifs);

        TrackedInfo inInfo = make_info(inName, inTitle, inTemplatePath);

        //checks for non-default content extension
        Path extPath = inInfo.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";

        if(file_exists(extPath.str()))
        {
            ifsx.open(extPath.str());

            ifsx >> inExt;
            inInfo.contentPath.file = inInfo.contentPath.file.substr(0, inInfo.contentPath.file.find_first_of('.')) + inExt;

            ifsx.close();
        }

        //checks for non-default output extension
        extPath = inInfo.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";

        if(file_exists(extPath.str()))
        {
            ifsx.open(extPath.str());

            ifsx >> inExt;
            inInfo.outputPath.file = inInfo.outputPath.file.substr(0, inInfo.outputPath.file.find_first_of('.')) + inExt;

            ifsx.close();
        }

        //checks that content and template files aren't the same
        if(inInfo.contentPath == inInfo.templatePath)
        {
            start_err(std::cout) << "failed to open .nsm/tracking.list" << std::endl;
            std::cout << c_light_blue << "reason: " << c_white << quote(inInfo.name) << " has same content and template path " << inInfo.templatePath << std::endl;

            ifs.close();
            return 1;
        }

        //makes sure there's no duplicate entries in tracking.list
        if(trackedAll.count(inInfo) > 0)
        {
            TrackedInfo cInfo = *(trackedAll.find(inInfo));

            start_err(std::cout) << "failed to load " << Path(".nsm/", "tracking.list") << std::endl;
            std::cout << c_light_blue << "reason: " << c_white << "duplicate entry for " << inInfo.name << std::endl;
            std::cout << c_light_blue << promptStr << c_white << "first entry:" << std::endl;
            std::cout << "        title: " << cInfo.title << std::endl;
            std::cout << "  output path: " << cInfo.outputPath << std::endl;
            std::cout << " content path: " << cInfo.contentPath << std::endl;
            std::cout << "template path: " << cInfo.templatePath << std::endl;
            std::cout << c_light_blue << promptStr << c_white << "second entry:" << std::endl;
            std::cout << "        title: " << inInfo.title << std::endl;
            std::cout << "  output path: " << inInfo.outputPath << std::endl;
            std::cout << " content path: " << inInfo.contentPath << std::endl;
            std::cout << "template path: " << inInfo.templatePath << std::endl;

            ifs.close();
            return 1;
        }

        trackedAll.insert(inInfo);
    }

    ifs.close();

    //clear_console_line();

    return 0;
}

int ProjectInfo::save_config(const std::string& configPathStr, const bool& global)
{
    std::ofstream ofs(configPathStr);
    if(!global)
    {
        ofs << "contentDir " << quote(contentDir) << "\n";
        ofs << "contentExt " << quote(contentExt) << "\n";
        ofs << "outputDir " << quote(outputDir) << "\n";
        ofs << "outputExt " << quote(outputExt) << "\n";
        ofs << "scriptExt " << quote(scriptExt) << "\n";
    }
    ofs << "defaultTemplate " << defaultTemplate << "\n\n";
    ofs << "backupScripts " << backupScripts << "\n\n";
    ofs << "buildThreads " << buildThreads << "\n\n";
    ofs << "paginateThreads " << paginateThreads << "\n\n";
    ofs << "incrementalMode " << incrMode << "\n\n";
    ofs << "terminal " << quote(terminal) << "\n\n";
    ofs << "unixTextEditor " << quote(unixTextEditor) << "\n";
    ofs << "winTextEditor " << quote(winTextEditor) << "\n\n";
    if(!global)
    {
        ofs << "rootBranch " << quote(rootBranch) << "\n";
        ofs << "outputBranch " << quote(outputBranch) << "\n";
    }
    ofs.close();

    return 0;
}

int ProjectInfo::save_global_config()
{
    return save_config(app_dir() + "/.nift/nift.config", 1);
}

int ProjectInfo::save_local_config()
{
    return save_config(".nsm/nift.config", 0);
}

int ProjectInfo::save_tracking()
{
    std::ofstream ofs(".nsm/tracking.list");

    for(auto tInfo=trackedAll.begin(); tInfo!=trackedAll.end(); tInfo++)
        ofs << *tInfo << "\n\n";

    ofs.close();

    return 0;
}

TrackedInfo ProjectInfo::make_info(const Name& name)
{
    TrackedInfo trackedInfo;

    trackedInfo.name = name;

    Path nameAsPath;
    nameAsPath.set_file_path_from(unquote(name));

    trackedInfo.contentPath = Path(contentDir + nameAsPath.dir, nameAsPath.file + contentExt);
    trackedInfo.outputPath = Path(outputDir + nameAsPath.dir, nameAsPath.file + outputExt);

    trackedInfo.title = name;
    trackedInfo.templatePath = defaultTemplate;

    //checks for non-default extensions
    std::string inExt;
    Path extPath = trackedInfo.outputPath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
    if(file_exists(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, inExt);
        ifs.close();

        trackedInfo.contentPath.file = trackedInfo.contentPath.file.substr(0, trackedInfo.contentPath.file.find_first_of('.')) + inExt;
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
    if(file_exists(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, inExt);
        ifs.close();

        trackedInfo.outputPath.file = trackedInfo.outputPath.file.substr(0, trackedInfo.outputPath.file.find_first_of('.')) + inExt;
    }

    return trackedInfo;
}

TrackedInfo ProjectInfo::make_info(const Name& name, const Title& title)
{
    TrackedInfo trackedInfo;

    trackedInfo.name = name;

    Path nameAsPath;
    nameAsPath.set_file_path_from(unquote(name));

    trackedInfo.contentPath = Path(contentDir + nameAsPath.dir, nameAsPath.file + contentExt);
    trackedInfo.outputPath = Path(outputDir + nameAsPath.dir, nameAsPath.file + outputExt);

    trackedInfo.title = title;
    trackedInfo.templatePath = defaultTemplate;

    //checks for non-default extensions
    std::string inExt;
    Path extPath = trackedInfo.outputPath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
    if(file_exists(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, inExt);
        ifs.close();

        trackedInfo.contentPath.file = trackedInfo.contentPath.file.substr(0, trackedInfo.contentPath.file.find_first_of('.')) + inExt;
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
    if(file_exists(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, inExt);
        ifs.close();

        trackedInfo.outputPath.file = trackedInfo.outputPath.file.substr(0, trackedInfo.outputPath.file.find_first_of('.')) + inExt;
    }

    return trackedInfo;
}

TrackedInfo ProjectInfo::make_info(const Name& name, const Title& title, const Path& templatePath)
{
    TrackedInfo trackedInfo;

    trackedInfo.name = name;

    Path nameAsPath;
    nameAsPath.set_file_path_from(unquote(name));

    trackedInfo.contentPath = Path(contentDir + nameAsPath.dir, nameAsPath.file + contentExt);
    trackedInfo.outputPath = Path(outputDir + nameAsPath.dir, nameAsPath.file + outputExt);

    trackedInfo.title = title;
    trackedInfo.templatePath = templatePath;

    //checks for non-default extensions
    std::string inExt;
    Path extPath = trackedInfo.outputPath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
    if(file_exists(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, inExt);
        ifs.close();

        trackedInfo.contentPath.file = trackedInfo.contentPath.file.substr(0, trackedInfo.contentPath.file.find_first_of('.')) + inExt;
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
    if(file_exists(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, inExt);
        ifs.close();

        trackedInfo.outputPath.file = trackedInfo.outputPath.file.substr(0, trackedInfo.outputPath.file.find_first_of('.')) + inExt;
    }

    return trackedInfo;
}

TrackedInfo make_info(const Name& name, 
                      const Title& title, 
                      const Path& templatePath, 
                      const Directory& contentDir, 
                      const Directory& outputDir, 
                      const std::string& contentExt, 
                      const std::string& outputExt)
{
    TrackedInfo trackedInfo;

    trackedInfo.name = name;

    Path nameAsPath;
    nameAsPath.set_file_path_from(unquote(name));

    trackedInfo.contentPath = Path(contentDir + nameAsPath.dir, nameAsPath.file + contentExt);
    trackedInfo.outputPath = Path(outputDir + nameAsPath.dir, nameAsPath.file + outputExt);

    trackedInfo.title = title;
    trackedInfo.templatePath = templatePath;

    return trackedInfo;
}

TrackedInfo ProjectInfo::get_info(const Name& name)
{
    TrackedInfo trackedInfo;
    trackedInfo.name = name;
    auto result = trackedAll.find(trackedInfo);

    if(result != trackedAll.end())
        return *result;

    trackedInfo.name = "##not-found##";
    return trackedInfo;
}

int ProjectInfo::info(const std::vector<Name>& names)
{
    std::cout << c_light_blue << promptStr << c_white << "info on specified names:" << std::endl;
    for(auto name=names.begin(); name!=names.end(); name++)
    {
        if(name != names.begin())
            std::cout << std::endl;

        TrackedInfo cInfo;
        cInfo.name = *name;
        if(trackedAll.count(cInfo))
        {
            cInfo = *(trackedAll.find(cInfo));
            std::cout << "         name: " << c_green << quote(cInfo.name) << c_white << std::endl;
            std::cout << "        title: " << cInfo.title << std::endl;
            std::cout << "  output path: " << cInfo.outputPath << std::endl;
            std::cout << "   output ext: " << quote(get_output_ext(cInfo)) << std::endl;
            std::cout << " content path: " << cInfo.contentPath << std::endl;
            std::cout << "  content ext: " << quote(get_cont_ext(cInfo)) << std::endl;
            std::cout << "   script ext: " << quote(get_script_ext(cInfo)) << std::endl;
            std::cout << "template path: " << cInfo.templatePath << std::endl;
        }
        else
            std::cout << "Nift is not tracking " << *name << std::endl;
    }

    return 0;
}

int ProjectInfo::info_all()
{
    if(file_exists(".nsm/.watchinfo/watching.list"))
    {
        WatchList wl;
        if(wl.open())
        {
            start_err(std::cout) << "info-all: failed to open watch list" << std::endl;
            return 1;
        }

        std::cout << c_light_blue << promptStr << c_white << "all watched directories:" << std::endl;
        for(auto wd=wl.dirs.begin(); wd!=wl.dirs.end(); wd++)
        {
            if(wd != wl.dirs.begin())
                std::cout << std::endl;
            std::cout << "  watch directory: " << c_green << quote(wd->watchDir) << c_white << std::endl;
            for(auto ext=wd->contExts.begin(); ext!=wd->contExts.end(); ext++)
            {
                std::cout << "content extension: " << quote(*ext) << std::endl;
                std::cout << "    template path: " << wd->templatePaths.at(*ext) << std::endl;
                std::cout << " output extension: " << wd->outputExts.at(*ext) << std::endl;
            }
        }
    }

    std::cout << c_light_blue << promptStr << c_white << "all tracked names:" << std::endl;
    for(auto trackedInfo=trackedAll.begin(); trackedInfo!=trackedAll.end(); trackedInfo++)
    {
        if(trackedInfo != trackedAll.begin())
            std::cout << std::endl;
        std::cout << "         name: " << c_green << quote(trackedInfo->name) << c_white << std::endl;
        std::cout << "        title: " << trackedInfo->title << std::endl;
        std::cout << "  output path: " << trackedInfo->outputPath << std::endl;
        std::cout << "   output ext: " << quote(get_output_ext(*trackedInfo)) << std::endl;
        std::cout << " content path: " << trackedInfo->contentPath << std::endl;
        std::cout << "  content ext: " << quote(get_cont_ext(*trackedInfo)) << std::endl;
        std::cout << "   script ext: " << quote(get_script_ext(*trackedInfo)) << std::endl;
        std::cout << "template path: " << trackedInfo->templatePath << std::endl;
    }

    return 0;
}

int ProjectInfo::info_names()
{
    std::cout << c_light_blue << promptStr << c_white << "all tracked names:" << std::endl;
    for(auto trackedInfo=trackedAll.begin(); trackedInfo!=trackedAll.end(); trackedInfo++)
        std::cout << " " << c_green << trackedInfo->name << c_white << std::endl;

    return 0;
}

int ProjectInfo::info_tracking()
{
    std::cout << c_light_blue << promptStr << c_white << "all tracked info:" << std::endl;
    for(auto trackedInfo=trackedAll.begin(); trackedInfo!=trackedAll.end(); trackedInfo++)
    {
        if(trackedInfo != trackedAll.begin())
            std::cout << std::endl;
        std::cout << "         name: " << c_green << quote(trackedInfo->name) << c_white << std::endl;
        std::cout << "        title: " << trackedInfo->title << std::endl;
        std::cout << "  output path: " << trackedInfo->outputPath << std::endl;
        std::cout << "   output ext: " << quote(get_output_ext(*trackedInfo)) << std::endl;
        std::cout << " content path: " << trackedInfo->contentPath << std::endl;
        std::cout << "  content ext: " << quote(get_cont_ext(*trackedInfo)) << std::endl;
        std::cout << "   script ext: " << quote(get_script_ext(*trackedInfo)) << std::endl;
        std::cout << "template path: " << trackedInfo->templatePath << std::endl;
    }

    return 0;
}

int ProjectInfo::info_watching()
{
    if(file_exists(".nsm/.watchinfo/watching.list"))
    {
        WatchList wl;
        if(wl.open())
        {
            start_err(std::cout) << "info-watching: failed to open watch list" << std::endl;
            return 1;
        }

        std::cout << c_light_blue << promptStr << c_white << "all watched directories:" << std::endl;
        for(auto wd=wl.dirs.begin(); wd!=wl.dirs.end(); wd++)
        {
            if(wd != wl.dirs.begin())
                std::cout << std::endl;
            std::cout << "  watch directory: " << c_green << quote(wd->watchDir) << c_white << std::endl;
            for(auto ext=wd->contExts.begin(); ext!=wd->contExts.end(); ext++)
            {
                std::cout << "content extension: " << quote(*ext) << std::endl;
                std::cout << "    template path: " << wd->templatePaths.at(*ext) << std::endl;
                std::cout << " output extension: " << wd->outputExts.at(*ext) << std::endl;
            }
        }
    }
    else
    {
        std::cout << "not watching any directories" << std::endl;
    }

    return 0;
}

int ProjectInfo::set_incr_mode(const std::string& modeStr)
{
    if(modeStr == "mod")
    {
        if(incrMode == INCR_MOD)
        {
            start_err(std::cout) << "set_incr_mode: incremental mode is already " << quote(modeStr) << std::endl;
            return 1;
        }
        incrMode = INCR_MOD;
        int ret_val = remove_hash_files();
        if(ret_val)
            return ret_val;
    }
    else if(modeStr == "hash")
    {
        if(incrMode == INCR_HASH)
        {
            start_err(std::cout) << "set_incr_mode: incremental mode is already " << quote(modeStr) << std::endl;
            return 1;
        }
        incrMode = INCR_HASH;
    }
    else if(modeStr == "hybrid")
    {
        if(incrMode == INCR_HYB)
        {
            start_err(std::cout) << "set_incr_mode: incremental mode is already " << quote(modeStr) << std::endl;
            return 1;
        }
        incrMode = INCR_HYB;
    }
    else
    {
        start_err(std::cout) << "set_incr_mode: do not recognise incremental mode " << quote(modeStr) << std::endl;
        return 1;
    }

    save_local_config();

    std::cout << "successfully changed incremental mode to " << quote(modeStr) << std::endl;

    return 0;
}

int ProjectInfo::remove_hash_files()
{
    open_tracking(1);

    std::cout << "removing hashes.." << std::endl;

    Path dep, infoPath;
    std::string str;
    for(auto cInfo=trackedAll.begin(); cInfo!=trackedAll.end(); ++cInfo)
    {
        //gets path of info file from last time output file was built
        infoPath = cInfo->outputPath.getInfoPath();

        //checks whether info path exists
        if(file_exists(infoPath.str()))
        {
            std::ifstream infoStream(infoPath.str());
            std::string timeDateLine;
            Name prevName;
            Title prevTitle;
            Path prevTemplatePath;

            // reads date/time, name, title and template path
            getline(infoStream, str);
            getline(infoStream, str);
            getline(infoStream, str);
            getline(infoStream, str);

            while(dep.read_file_path_from(infoStream))
            {
                if(file_exists(dep.str()))
                {
                    int ret_val = remove_path(dep.getHashPath());
                    if(ret_val)
                        return ret_val;
                }
            }
        }
    }

    return 0;
}

std::string ProjectInfo::get_ext(const TrackedInfo& trackedInfo, const std::string& extType)
{
    std::string ext;

    //checks for non-default extension
    Path extPath = trackedInfo.outputPath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + extType;

    if(file_exists(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, ext);
        ifs.close();
    }
    else if(extType == ".contExt")
        ext = contentExt;
    else if(extType == ".outputExt")
        ext = outputExt;
    else if(extType == ".scriptExt")
        ext = scriptExt;

    return ext;
}

std::string ProjectInfo::get_cont_ext(const TrackedInfo& trackedInfo)
{
    return get_ext(trackedInfo, ".contExt");
}

std::string ProjectInfo::get_output_ext(const TrackedInfo& trackedInfo)
{
    return get_ext(trackedInfo, ".outputExt");
}

std::string ProjectInfo::get_script_ext(const TrackedInfo& trackedInfo)
{
    return get_ext(trackedInfo, ".scriptExt");
}

bool ProjectInfo::tracking(const TrackedInfo& trackedInfo)
{
    return trackedAll.count(trackedInfo);
}

bool ProjectInfo::tracking(const Name& name)
{
    TrackedInfo trackedInfo;
    trackedInfo.name = name;
    return trackedAll.count(trackedInfo);
}

int ProjectInfo::track(const Name& name, const Title& title, const Path& templatePath)
{
    if(name.find('.') != std::string::npos)
    {
        start_err(std::cout) << "names cannot contain '.'" << std::endl;
        std::cout << c_light_blue << "note: " << c_white << "you can add post-build scripts to move built/output files to paths containing . other than for extensions if you want" << std::endl;
        return 1;
    }
    else if(name == "" || title.str == "")
    {
        start_err(std::cout) << "name and title must all be non-empty strings" << std::endl;
        return 1;
    }
    if(unquote(name)[unquote(name).size()-1] == '/' || unquote(name)[unquote(name).size()-1] == '\\')
    {
        start_err(std::cout) << "name " << quote(name) << " cannot end in '/' or '\\'" << std::endl;
        return 1;
    }
    else if(
                (unquote(name).find('"') != std::string::npos && unquote(name).find('\'') != std::string::npos) ||
                (unquote(title.str).find('"') != std::string::npos && unquote(title.str).find('\'') != std::string::npos) ||
                (unquote(templatePath.str()).find('"') != std::string::npos && unquote(templatePath.str()).find('\'') != std::string::npos)
            )
    {
        start_err(std::cout) << "name, title and template path cannot contain both single and double quotes" << std::endl;
        return 1;
    }

    TrackedInfo newInfo = make_info(name, title, templatePath);

    if(newInfo.contentPath == newInfo.templatePath)
    {
        start_err(std::cout) << "content and template paths cannot be the same, not tracked" << std::endl;
        return 1;
    }

    if(trackedAll.count(newInfo) > 0)
    {
        TrackedInfo cInfo = *(trackedAll.find(newInfo));

        start_err(std::cout) << "Nift is already tracking " << newInfo.outputPath << std::endl;
        std::cout << c_light_blue << promptStr << c_white << "current tracked details:" << std::endl;
        std::cout << "        title: " << cInfo.title << std::endl;
        std::cout << "  output path: " << cInfo.outputPath << std::endl;
        std::cout << " content path: " << cInfo.contentPath << std::endl;
        std::cout << "template path: " << cInfo.templatePath << std::endl;

        return 1;
    }

    bool blankTemplate = 0;
    if(newInfo.templatePath.str() == "")
        blankTemplate = 1;

    //throws error if template path doesn't exist
    if(!blankTemplate && !file_exists(newInfo.templatePath.str()))
    {
        start_err(std::cout) << "template path " << newInfo.templatePath << " does not exist" << std::endl;
        return 1;
    }

    if(path_exists(newInfo.outputPath.str()))
    {
        start_err(std::cout) << "new output path " << newInfo.outputPath << " already exists" << std::endl;
        return 1;
    }

    if(!file_exists(newInfo.contentPath.str()))
    {
        newInfo.contentPath.ensureFileExists();
        //chmod(newInfo.contentPath.str().c_str(), 0666);
    }

    //ensures directories for output file and info file exist
    //newInfo.outputPath.ensureDirExists();
    //newInfo.outputPath.getInfoPath().ensureDirExists();

    //inserts new info into set of trackedAll
    trackedAll.insert(newInfo);

    //saves new tracking list to .nsm/tracking.list
    save_tracking();

    //informs user that addition was successful
    std::cout << "successfully tracking " << newInfo.name << std::endl;

    return 0;
}

int ProjectInfo::track_from_file(const std::string& filePath)
{
    if(!file_exists(filePath))
    {
        start_err(std::cout) << "track-from-file: track-file " << quote(filePath) << " does not exist" << std::endl;
        return 1;
    }

    Name name;
    Title title;
    Path templatePath;
    std::string cExt, oExt, str;
    int noAdded = 0;

    std::ifstream ifs(filePath);
    std::vector<TrackedInfo> toTrackVec;
    TrackedInfo newInfo;
    while(getline(ifs, str))
    {
        std::istringstream iss(str);

        name = "";
        title.str = "";
        cExt = "";
        templatePath = Path("", "");
        oExt = "";

        if(read_quoted(iss, name))
            if(read_quoted(iss, title.str))
                if(read_quoted(iss, cExt))
                    if(templatePath.read_file_path_from(iss))
                        read_quoted(iss, oExt);

        if(name == "" || name[0] == '#')
            continue;

        if(title.str == "")
            title.str = name;

        if(cExt == "")
            cExt = contentExt;

        if(templatePath == Path("", ""))
            templatePath = defaultTemplate;

        if(oExt == "")
            oExt = outputExt;

        if(name.find('.') != std::string::npos)
        {
            start_err(std::cout) << "name " << quote(name) << " cannot contain '.'" << std::endl;
            std::cout << c_light_blue << "note: " << c_white << "you can add post-build scripts to move built built/output files to paths containing . other than for extensions if you want" << std::endl;
            return 1;
        }
        else if(name == "" || title.str == "" || cExt == "" || oExt == "")
        {
            start_err(std::cout) << "names, titles, content extensions and output extensions must all be non-empty strings" << std::endl;
            return 1;
        }
        if(unquote(name)[unquote(name).size()-1] == '/' || unquote(name)[unquote(name).size()-1] == '\\')
        {
            start_err(std::cout) << "name " << quote(name) << " cannot end in '/' or '\\'" << std::endl;
            return 1;
        }
        else if(
                    (unquote(name).find('"') != std::string::npos && unquote(name).find('\'') != std::string::npos) ||
                    (unquote(title.str).find('"') != std::string::npos && unquote(title.str).find('\'') != std::string::npos) ||
                    (unquote(templatePath.str()).find('"') != std::string::npos && unquote(templatePath.str()).find('\'') != std::string::npos)
                )
        {
            start_err(std::cout) << "names, titles and template paths cannot contain both single and double quotes" << std::endl;
            return 1;
        }

        if(cExt == "" || cExt[0] != '.')
        {
            start_err(std::cout) << "track-from-file: content extension " << quote(cExt) << " must start with a fullstop" << std::endl;
            return 1;
        }
        else if(oExt == "" || oExt[0] != '.')
        {
            start_err(std::cout) << "track-from-file: output extension " << quote(oExt) << " must start with a fullstop" << std::endl;
            return 1;
        }

        newInfo = make_info(name, title, templatePath);

        if(cExt != contentExt)
            newInfo.contentPath.file = newInfo.contentPath.file.substr(0, newInfo.contentPath.file.find_first_of('.')) + cExt;

        if(oExt != outputExt)
            newInfo.outputPath.file = newInfo.outputPath.file.substr(0, newInfo.outputPath.file.find_first_of('.')) + oExt;

        if(newInfo.contentPath == newInfo.templatePath)
        {
            start_err(std::cout) << "content and template paths cannot be the same, " << newInfo.name << " not tracked" << std::endl;
            return 1;
        }

        if(trackedAll.count(newInfo) > 0)
        {
            TrackedInfo cInfo = *(trackedAll.find(newInfo));

            start_err(std::cout) << "Nift is already tracking " << newInfo.outputPath << std::endl;
            std::cout << c_light_blue << promptStr << c_white << "current tracked details:" << std::endl;
            std::cout << "        title: " << cInfo.title << std::endl;
            std::cout << "  output path: " << cInfo.outputPath << std::endl;
            std::cout << " content path: " << cInfo.contentPath << std::endl;
            std::cout << "template path: " << cInfo.templatePath << std::endl;

            return 1;
        }

        //throws error if template path doesn't exist
        bool blankTemplate = 0;
        if(newInfo.templatePath.str() == "")
            blankTemplate = 1;
        if(!blankTemplate && !file_exists(newInfo.templatePath.str()))
        {
            start_err(std::cout) << "template path " << newInfo.templatePath << " does not exist" << std::endl;
            return 1;
        }

        if(path_exists(newInfo.outputPath.str()))
        {
            start_err(std::cout) << "new output path " << newInfo.outputPath << " already exists" << std::endl;
            return 1;
        }

        toTrackVec.push_back(newInfo);
    }

    int pos;
    for(size_t i=0; i<toTrackVec.size(); i++)
    {
        newInfo = toTrackVec[i];

        pos = newInfo.contentPath.file.find_first_of('.');
        cExt = newInfo.contentPath.file.substr(pos, newInfo.contentPath.file.size() -1);
        pos = newInfo.outputPath.file.find_first_of('.');
        oExt = newInfo.outputPath.file.substr(pos, newInfo.outputPath.file.size() -1);

        if(cExt != contentExt)
        {
            Path extPath = newInfo.outputPath.getInfoPath();
            extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
            extPath.ensureFileExists();
            std::ofstream ofs(extPath.str());
            ofs << cExt << std::endl;
            ofs.close();
        }

        if(oExt != outputExt)
        {
            Path extPath = newInfo.outputPath.getInfoPath();
            extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
            extPath.ensureFileExists();
            std::ofstream ofs(extPath.str());
            ofs << oExt << std::endl;
            ofs.close();
        }

        if(!file_exists(newInfo.contentPath.str()))
            newInfo.contentPath.ensureFileExists();

        //ensures directories for output file and info file exist
        //newInfo.outputPath.ensureDirExists();
        //newInfo.outputPath.getInfoPath().ensureDirExists();

        //inserts new info into trackedAll set
        trackedAll.insert(newInfo);
        noAdded++;
    }
    ifs.close();

    //saves new tracking list to .nsm/tracking.list
    save_tracking();

    std::cout << "successfully tracked: " << noAdded << std::endl;

    return 0;
}

int ProjectInfo::track_dir(const Path& dirPath, 
                           const std::string& cExt, 
                           const Path& templatePath, 
                           const std::string& oExt)
{
    if(dirPath.file != "")
    {
        start_err(std::cout) << "track-dir: directory path " << dirPath << " is to a file not a directory" << std::endl;
        return 1;
    }
    else if(dirPath.comparableStr().substr(0, contentDir.size()) != contentDir)
    {
        start_err(std::cout) << "track-dir: cannot track files from directory " << dirPath << " as it is not a subdirectory of the project-wide content directory " << quote(contentDir) << std::endl;
        return 1;
    }
    else if(!dir_exists(dirPath.str()))
    {
        start_err(std::cout) << "track-dir: directory path " << dirPath << " does not exist" << std::endl;
        return 1;
    }

    if(cExt == "" || cExt[0] != '.')
    {
        start_err(std::cout) << "track-dir: content extension " << quote(cExt) << " must start with a fullstop" << std::endl;
        return 1;
    }
    else if(oExt == "" || oExt[0] != '.')
    {
        start_err(std::cout) << "track-dir: output extension " << quote(oExt) << " must start with a fullstop" << std::endl;
        return 1;
    }

    bool blankTemplate = 0;
    if(templatePath.str() == "")
        blankTemplate = 1;
    if(!blankTemplate && !file_exists(templatePath.str()))
    {
        start_err(std::cout) << "track-dir: template path " << templatePath << " does not exist" << std::endl;
        return 1;
    }

    std::vector<InfoToTrack> to_track;
    std::string ext;
    std::string contDir2dirPath = "";
    if(comparable(dirPath.dir).size() > comparable(contentDir).size())
       contDir2dirPath = dirPath.dir.substr(contentDir.size(), dirPath.dir.size()-contentDir.size());
    Name name;
    size_t pos;
    std::vector<std::string> files = lsVec(dirPath.dir.c_str());
    for(size_t f=0; f<files.size(); f++)
    {
        pos = files[f].find_first_of('.');
        if(pos != std::string::npos)
        {
            ext = files[f].substr(pos, files[f].size()-pos);
            name = contDir2dirPath + files[f].substr(0, pos);

            if(cExt == ext)
                if(!tracking(name))
                    to_track.push_back(InfoToTrack(name, Title(name), templatePath, cExt, oExt));
        }
    }

    if(to_track.size())
        track(to_track);

    return 0;
}

int ProjectInfo::track(const Name& name, 
                       const Title& title, 
                       const Path& templatePath, 
                       const std::string& cExt, 
                       const std::string& oExt)
{
    if(name.find('.') != std::string::npos)
    {
        start_err(std::cout) << "names cannot contain '.'" << std::endl;
        std::cout << c_light_blue << "note: " << c_white << "you can add post-build scripts to move built/output files to paths containing . other than for extensions if you want" << std::endl;
        return 1;
    }
    else if(name == "" || title.str == "" || cExt == "" || oExt == "")
    {
        start_err(std::cout) << "name, title, content extension and output extension must all be non-empty strings" << std::endl;
        return 1;
    }
    if(unquote(name)[unquote(name).size()-1] == '/' || unquote(name)[unquote(name).size()-1] == '\\')
    {
        start_err(std::cout) << "name " << quote(name) << " cannot end in '/' or '\\'" << std::endl;
        return 1;
    }
    else if(
                (unquote(name).find('"') != std::string::npos && unquote(name).find('\'') != std::string::npos) ||
                (unquote(title.str).find('"') != std::string::npos && unquote(title.str).find('\'') != std::string::npos) ||
                (unquote(templatePath.str()).find('"') != std::string::npos && unquote(templatePath.str()).find('\'') != std::string::npos)
            )
    {
        start_err(std::cout) << "name, title and template path cannot contain both single and double quotes" << std::endl;
        return 1;
    }

    if(cExt == "" || cExt[0] != '.')
    {
        start_err(std::cout) << "track: content extension " << quote(cExt) << " must start with a fullstop" << std::endl;
        return 1;
    }
    else if(oExt == "" || oExt[0] != '.')
    {
        start_err(std::cout) << "track: output extension " << quote(oExt) << " must start with a fullstop" << std::endl;
        return 1;
    }

    TrackedInfo newInfo = make_info(name, title, templatePath);

    if(cExt != contentExt)
        newInfo.contentPath.file = newInfo.contentPath.file.substr(0, newInfo.contentPath.file.find_first_of('.')) + cExt;

    if(oExt != outputExt)
        newInfo.outputPath.file = newInfo.outputPath.file.substr(0, newInfo.outputPath.file.find_first_of('.')) + oExt;

    if(newInfo.contentPath == newInfo.templatePath)
    {
        start_err(std::cout) << "content and template paths cannot be the same, " << newInfo.name << " not tracked" << std::endl;
        return 1;
    }

    if(trackedAll.count(newInfo) > 0)
    {
        TrackedInfo cInfo = *(trackedAll.find(newInfo));

        start_err(std::cout) << "Nift is already tracking " << newInfo.outputPath << std::endl;
        std::cout << c_light_blue << promptStr << c_white << "current tracked details:" << std::endl;
        std::cout << "        title: " << cInfo.title << std::endl;
        std::cout << "  output path: " << cInfo.outputPath << std::endl;
        std::cout << " content path: " << cInfo.contentPath << std::endl;
        std::cout << "template path: " << cInfo.templatePath << std::endl;

        return 1;
    }

    //throws error if template path doesn't exist
    bool blankTemplate = 0;
    if(newInfo.templatePath.str() == "")
        blankTemplate = 1;
    if(!blankTemplate && !file_exists(newInfo.templatePath.str()))
    {
        start_err(std::cout) << "template path " << newInfo.templatePath << " does not exist" << std::endl;
        return 1;
    }

    if(path_exists(newInfo.outputPath.str()))
    {
        start_err(std::cout) << "new output path " << newInfo.outputPath << " already exists" << std::endl;
        return 1;
    }

    if(cExt != contentExt)
    {
        Path extPath = newInfo.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
        extPath.ensureFileExists();
        std::ofstream ofs(extPath.str());
        ofs << cExt << std::endl;
        ofs.close();
    }

    if(oExt != outputExt)
    {
        Path extPath = newInfo.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
        extPath.ensureFileExists();
        std::ofstream ofs(extPath.str());
        ofs << oExt << std::endl;
        ofs.close();
    }

    if(!file_exists(newInfo.contentPath.str()))
    {
        newInfo.contentPath.ensureFileExists();
        //chmod(newInfo.contentPath.str().c_str(), 0666);
    }

    //ensures directories for output file and info file exist
    //newInfo.outputPath.ensureDirExists();
    //newInfo.outputPath.getInfoPath().ensureDirExists();

    //inserts new info into trackedAll set
    trackedAll.insert(newInfo);

    //saves new tracking list to .nsm/tracking.list
    save_tracking();

    //informs user that addition was successful
    std::cout << "successfully tracking " << newInfo.name << std::endl;

    return 0;
}

int ProjectInfo::track(const std::vector<InfoToTrack>& toTrack)
{
    Name name;
    Title title;
    Path templatePath;
    std::string cExt, oExt;

    for(size_t p=0; p<toTrack.size(); p++)
    {
        name = toTrack[p].name;
        title = toTrack[p].title;
        templatePath = toTrack[p].templatePath;
        cExt = toTrack[p].contExt;
        oExt = toTrack[p].outputExt;

        if(name.find('.') != std::string::npos)
        {
            start_err(std::cout) << "name " << quote(name) << " cannot contain '.'" << std::endl;
            std::cout << c_light_blue << "note: " << c_white << "you can add post-build scripts to move built/output files to paths containing . other than for extensions if you want" << std::endl;
            return 1;
        }
        else if(name == "" || title.str == "" || cExt == "" || oExt == "")
        {
            start_err(std::cout) << "names, titles, content extensions and output extensions must all be non-empty strings" << std::endl;
            return 1;
        }
        if(unquote(name)[unquote(name).size()-1] == '/' || unquote(name)[unquote(name).size()-1] == '\\')
        {
            start_err(std::cout) << "name " << quote(name) << " cannot end in '/' or '\\'" << std::endl;
            return 1;
        }
        else if(
                    (unquote(name).find('"') != std::string::npos && unquote(name).find('\'') != std::string::npos) ||
                    (unquote(title.str).find('"') != std::string::npos && unquote(title.str).find('\'') != std::string::npos) ||
                    (unquote(templatePath.str()).find('"') != std::string::npos && unquote(templatePath.str()).find('\'') != std::string::npos)
                )
        {
            start_err(std::cout) << "names, titles and template paths cannot contain both single and double quotes" << std::endl;
            return 1;
        }

        if(cExt == "" || cExt[0] != '.')
        {
            start_err(std::cout) << "track-dir: content extension " << quote(cExt) << " must start with a fullstop" << std::endl;
            return 1;
        }
        else if(oExt == "" || oExt[0] != '.')
        {
            start_err(std::cout) << "track-dir: output extension " << quote(oExt) << " must start with a fullstop" << std::endl;
            return 1;
        }

        TrackedInfo newInfo = make_info(name, title, templatePath);

        if(cExt != contentExt)
            newInfo.contentPath.file = newInfo.contentPath.file.substr(0, newInfo.contentPath.file.find_first_of('.')) + cExt;

        if(oExt != outputExt)
            newInfo.outputPath.file = newInfo.outputPath.file.substr(0, newInfo.outputPath.file.find_first_of('.')) + oExt;

        if(newInfo.contentPath == newInfo.templatePath)
        {
            start_err(std::cout) << "content and template paths cannot be the same, not tracked" << std::endl;
            return 1;
        }

        if(trackedAll.count(newInfo) > 0)
        {
            TrackedInfo cInfo = *(trackedAll.find(newInfo));

            start_err(std::cout) << "Nift is already tracking " << newInfo.outputPath << std::endl;
            std::cout << c_light_blue << promptStr << c_white << "current tracked details:" << std::endl;
            std::cout << "        title: " << cInfo.title << std::endl;
            std::cout << "  output path: " << cInfo.outputPath << std::endl;
            std::cout << " content path: " << cInfo.contentPath << std::endl;
            std::cout << "template path: " << cInfo.templatePath << std::endl;

            return 1;
        }

        //throws error if template path doesn't exist
        bool blankTemplate = 0;
        if(newInfo.templatePath.str() == "")
            blankTemplate = 1;
        if(!blankTemplate && !file_exists(newInfo.templatePath.str()))
        {
            start_err(std::cout) << "template path " << newInfo.templatePath << " does not exist" << std::endl;
            return 1;
        }

        if(path_exists(newInfo.outputPath.str()))
        {
            start_err(std::cout) << "new output path " << newInfo.outputPath << " already exists" << std::endl;
            return 1;
        }
    }

    for(size_t p=0; p<toTrack.size(); p++)
    {
        name = toTrack[p].name;
        title = toTrack[p].title;
        templatePath = toTrack[p].templatePath;
        cExt = toTrack[p].contExt;
        oExt = toTrack[p].outputExt;

        TrackedInfo newInfo = make_info(name, title, templatePath);

        if(cExt != contentExt)
        {
            newInfo.contentPath.file = newInfo.contentPath.file.substr(0, newInfo.contentPath.file.find_first_of('.')) + cExt;

            Path extPath = newInfo.outputPath.getInfoPath();
            extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
            extPath.ensureFileExists();
            std::ofstream ofs(extPath.str());
            ofs << cExt << std::endl;
            ofs.close();
        }

        if(oExt != outputExt)
        {
            newInfo.outputPath.file = newInfo.outputPath.file.substr(0, newInfo.outputPath.file.find_first_of('.')) + oExt;

            Path extPath = newInfo.outputPath.getInfoPath();
            extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
            extPath.ensureFileExists();
            std::ofstream ofs(extPath.str());
            ofs << oExt << std::endl;
            ofs.close();
        }

        if(!file_exists(newInfo.contentPath.str()))
        {
            newInfo.contentPath.ensureFileExists();
            //chmod(newInfo.contentPath.str().c_str(), 0666);
        }

        //ensures directories for output file and info file exist
        //newInfo.outputPath.ensureDirExists();
        //newInfo.outputPath.getInfoPath().ensureDirExists();

        //inserts new info into trackedAll set
        trackedAll.insert(newInfo);

        //informs user that addition was successful
        std::cout << "successfully tracking " << newInfo.name << std::endl;
    }

    //saves new tracking list to .nsm/tracking.list
    save_tracking();

    return 0;
}

int ProjectInfo::untrack(const Name& nameToUntrack)
{
    //checks that name is being tracked
    if(!tracking(nameToUntrack))
    {
        start_err(std::cout) << "Nift is not tracking " << nameToUntrack << std::endl;
        return 1;
    }

    TrackedInfo toErase = get_info(nameToUntrack);

    //removes the extension files if they exist
    Path extPath = toErase.outputPath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
    if(file_exists(extPath.str()))
    {
        chmod(extPath.str().c_str(), 0666);
        remove_path(extPath);
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
    if(file_exists(extPath.str()))
    {
        chmod(extPath.str().c_str(), 0666);
        remove_path(extPath);
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";
    if(file_exists(extPath.str()))
    {
        chmod(extPath.str().c_str(), 0666);
        remove_path(extPath);
    }

    //removes info file and containing dirs if now empty
    if(file_exists(toErase.outputPath.getInfoPath().str()))
    {
        chmod(toErase.outputPath.getInfoPath().str().c_str(), 0666);
        remove_path(toErase.outputPath.getInfoPath());
    }

    //removes output file and containing dirs if now empty
    if(file_exists(toErase.outputPath.str()))
    {
        chmod(toErase.outputPath.str().c_str(), 0666);
        remove_path(toErase.outputPath);
    }

    //removes info from trackedAll set
    trackedAll.erase(toErase);

    //saves new tracking list to .nsm/tracking.list
    save_tracking();

    //informs user that name was successfully untracked
    std::cout << "successfully untracked " << nameToUntrack << std::endl;

    return 0;
}

int ProjectInfo::untrack(const std::vector<Name>& namesToUntrack)
{
    TrackedInfo toErase;
    Path extPath;
    for(size_t p=0; p<namesToUntrack.size(); p++)
    {
        //checks that name is being tracked
        if(!tracking(namesToUntrack[p]))
        {
            start_err(std::cout) << "Nift is not tracking " << namesToUntrack[p] << std::endl;
            return 1;
        }
    }

    for(size_t p=0; p<namesToUntrack.size(); p++)
    {
        toErase = get_info(namesToUntrack[p]);

        //removes the extension files if they exist
        Path extPath = toErase.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
        if(file_exists(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
        if(file_exists(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";
        if(file_exists(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }

        //removes info file and containing dirs if now empty
        if(file_exists(toErase.outputPath.getInfoPath().str()))
        {
            chmod(toErase.outputPath.getInfoPath().str().c_str(), 0666);
            remove_path(toErase.outputPath.getInfoPath());
        }

        //removes output file and containing dirs if now empty
        if(file_exists(toErase.outputPath.str()))
        {
            chmod(toErase.outputPath.str().c_str(), 0666);
            remove_path(toErase.outputPath);
        }

        //removes info from trackedAll set
        trackedAll.erase(toErase);

        //informs user that name was successfully untracked
        std::cout << "successfully untracked " << namesToUntrack[p] << std::endl;
    }

    //saves new tracking list to .nsm/tracking.list
    save_tracking();

    return 0;
}

int ProjectInfo::untrack_from_file(const std::string& filePath)
{
    if(!file_exists(filePath))
    {
        start_err(std::cout) << "untrack-from-file: untrack-file " << quote(filePath) << " does not exist" << std::endl;
        return 1;
    }

    Name nameToUntrack;
    int noUntracked = 0;
    std::string str;
    std::ifstream ifs(filePath);
    std::vector<TrackedInfo> toEraseVec;
    while(getline(ifs, str))
    {
        std::istringstream iss(str);

        read_quoted(iss, nameToUntrack);

        if(nameToUntrack == "" || nameToUntrack[0] == '#')
            continue;

        //checks that name is being tracked
        if(!tracking(nameToUntrack))
        {
            start_err(std::cout) << "Nift is not tracking " << nameToUntrack << std::endl;
            return 1;
        }

        toEraseVec.push_back(get_info(nameToUntrack));
    }

    TrackedInfo toErase;
    for(size_t i=0; i<toEraseVec.size(); i++)
    {
        toErase = toEraseVec[i];

        //removes the extension files if they exist
        Path extPath = toErase.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
        if(file_exists(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
        if(file_exists(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";
        if(file_exists(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }

        //removes info file and containing dirs if now empty
        if(file_exists(toErase.outputPath.getInfoPath().str()))
        {
            chmod(toErase.outputPath.getInfoPath().str().c_str(), 0666);
            remove_path(toErase.outputPath.getInfoPath());
        }

        //removes output file and containing dirs if now empty
        if(file_exists(toErase.outputPath.str()))
        {
            chmod(toErase.outputPath.str().c_str(), 0666);
            remove_path(toErase.outputPath);
        }

        //removes info from trackedAll set
        trackedAll.erase(toErase);
        noUntracked++;
    }

    ifs.close();

    //saves new tracking list to .nsm/tracking.list
    save_tracking();

    std::cout << "successfully untracked count: " << noUntracked << std::endl;

    return 0;
}

int ProjectInfo::untrack_dir(const Path& dirPath, const std::string& cExt)
{
    if(dirPath.file != "")
    {
        start_err(std::cout) << "untrack-dir: directory path " << dirPath << " is to a file not a directory" << std::endl;
        return 1;
    }
    else if(dirPath.comparableStr().substr(0, contentDir.size()) != contentDir)
    {
        start_err(std::cout) << "untrack-dir: cannot untrack from directory " << dirPath << " as it is not a subdirectory of the project-wide content directory " << quote(contentDir) << std::endl;
        return 1;
    }
    else if(!dir_exists(dirPath.str()))
    {
        start_err(std::cout) << "untrack-dir: directory path " << dirPath << " does not exist" << std::endl;
        return 1;
    }

    if(cExt == "" || cExt[0] != '.')
    {
        start_err(std::cout) << "untrack-dir: content extension " << quote(cExt) << " must start with a fullstop" << std::endl;
        return 1;
    }

    std::vector<Name> namesToUntrack;
    std::string ext;
    std::string contDir2dirPath = "";
    if(comparable(dirPath.dir).size() > comparable(contentDir).size())
       contDir2dirPath = dirPath.dir.substr(contentDir.size(), dirPath.dir.size()-contentDir.size());
    Name name;
    size_t pos;
    std::vector<std::string> files = lsVec(dirPath.dir.c_str());
    for(size_t f=0; f<files.size(); f++)
    {
        pos = files[f].find_first_of('.');
        if(pos != std::string::npos)
        {
            ext = files[f].substr(pos, files[f].size()-pos);
            name = contDir2dirPath + files[f].substr(0, pos);

            if(cExt == ext)
                if(tracking(name))
                    namesToUntrack.push_back(name);
        }
    }

    if(namesToUntrack.size())
        untrack(namesToUntrack);

    return 0;
}

int ProjectInfo::rm_from_file(const std::string& filePath)
{
    if(!file_exists(filePath))
    {
        start_err(std::cout) << "rm-from-file: rm-file " << quote(filePath) << " does not exist" << std::endl;
        return 1;
    }

    Name nameToRemove;
    int noRemoved = 0;
    std::string str;
    std::ifstream ifs(filePath);
    std::vector<TrackedInfo> toEraseVec;
    while(getline(ifs, str))
    {
        std::istringstream iss(str);

        read_quoted(iss, nameToRemove);

        if(nameToRemove == "" || nameToRemove[0] == '#')
            continue;

        //checks that name is being tracked
        if(!tracking(nameToRemove))
        {
            start_err(std::cout) << "Nift is not tracking " << nameToRemove << std::endl;
            return 1;
        }

        toEraseVec.push_back(get_info(nameToRemove));
    }

    TrackedInfo toErase;
    for(size_t i=0; i<toEraseVec.size(); i++)
    {
        toErase = toEraseVec[i];

        //removes the extension files if they exist
        Path extPath = toErase.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
        if(file_exists(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
        if(file_exists(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";
        if(file_exists(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }

        //removes info file and containing dirs if now empty
        if(file_exists(toErase.outputPath.getInfoPath().str()))
        {
            chmod(toErase.outputPath.getInfoPath().str().c_str(), 0666);
            remove_path(toErase.outputPath.getInfoPath());
        }

        //removes output file and containing dirs if now empty
        if(file_exists(toErase.outputPath.str()))
        {
            chmod(toErase.outputPath.str().c_str(), 0666);
            remove_path(toErase.outputPath);
        }

        //removes content file and containing dirs if now empty
        if(file_exists(toErase.contentPath.str()))
        {
            chmod(toErase.contentPath.str().c_str(), 0666);
            remove_path(toErase.contentPath);
        }

        //removes toErase from trackedAll set
        trackedAll.erase(toErase);
        noRemoved++;
    }

    ifs.close();

    //saves new tracking list to .nsm/tracking.list
    save_tracking();

    std::cout << "successfully removed: " << noRemoved << std::endl;

    return 0;
}

int ProjectInfo::rm_dir(const Path& dirPath, const std::string& cExt)
{
    if(dirPath.file != "")
    {
        start_err(std::cout) << "rm-dir: directory path " << dirPath << " is to a file not a directory" << std::endl;
        return 1;
    }
    else if(dirPath.comparableStr().substr(0, contentDir.size()) != contentDir)
    {
        start_err(std::cout) << "rm-dir: cannot remove from directory " << dirPath << " as it is not a subdirectory of the project-wide content directory " << quote(contentDir) << std::endl;
        return 1;
    }
    else if(!dir_exists(dirPath.str()))
    {
        start_err(std::cout) << "rm-dir: directory path " << dirPath << " does not exist" << std::endl;
        return 1;
    }

    if(cExt == "" || cExt[0] != '.')
    {
        start_err(std::cout) << "rm-dir: content extension " << quote(cExt) << " must start with a fullstop" << std::endl;
        return 1;
    }

    std::vector<Name> namesToRemove;
    std::string ext;
    std::string contDir2dirPath = "";
    if(comparable(dirPath.dir).size() > comparable(contentDir).size())
       contDir2dirPath = dirPath.dir.substr(contentDir.size(), dirPath.dir.size()-contentDir.size());
    Name name;
    size_t pos;
    std::vector<std::string> files = lsVec(dirPath.dir.c_str());
    for(size_t f=0; f<files.size(); f++)
    {
        pos = files[f].find_first_of('.');
        if(pos != std::string::npos)
        {
            ext = files[f].substr(pos, files[f].size()-pos);
            name = contDir2dirPath + files[f].substr(0, pos);

            if(cExt == ext)
                if(tracking(name))
                    namesToRemove.push_back(name);
        }
    }

    if(namesToRemove.size())
        rm(namesToRemove);

    return 0;
}

int ProjectInfo::rm(const Name& nameToRemove)
{
    //checks that name is being tracked
    if(!tracking(nameToRemove))
    {
        start_err(std::cout) << "Nift is not tracking " << nameToRemove << std::endl;
        return 1;
    }

    TrackedInfo toErase = get_info(nameToRemove);

    //removes the extension files if they exist
    Path extPath = toErase.outputPath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
    if(file_exists(extPath.str()))
    {
        chmod(extPath.str().c_str(), 0666);
        remove_path(extPath);
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
    if(file_exists(extPath.str()))
    {
        chmod(extPath.str().c_str(), 0666);
        remove_path(extPath);
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";
    if(file_exists(extPath.str()))
    {
        chmod(extPath.str().c_str(), 0666);
        remove_path(extPath);
    }

    //removes info file and containing dirs if now empty
    if(file_exists(toErase.outputPath.getInfoPath().str()))
    {
        chmod(toErase.outputPath.getInfoPath().str().c_str(), 0666);
        remove_path(toErase.outputPath.getInfoPath());
    }

    //removes output file and containing dirs if now empty
    if(file_exists(toErase.outputPath.str()))
    {
        chmod(toErase.outputPath.str().c_str(), 0666);
        remove_path(toErase.outputPath);
    }

    //removes content file and containing dirs if now empty
    if(file_exists(toErase.contentPath.str()))
    {
        chmod(toErase.contentPath.str().c_str(), 0666);
        remove_path(toErase.contentPath);
    }

    //removes info from trackedAll set
    trackedAll.erase(toErase);

    //saves new tracking list to .nsm/tracking.list
    save_tracking();

    //informs user that removal was successful
    std::cout << "successfully removed " << nameToRemove << std::endl;

    return 0;
}

int ProjectInfo::rm(const std::vector<Name>& namesToRemove)
{
    TrackedInfo toErase;
    Path extPath;
    for(size_t p=0; p<namesToRemove.size(); p++)
    {
        //checks that name is being tracked
        if(!tracking(namesToRemove[p]))
        {
            start_err(std::cout) << "Nift is not tracking " << namesToRemove[p] << std::endl;
            return 1;
        }
    }

    for(size_t p=0; p<namesToRemove.size(); p++)
    {
        toErase = get_info(namesToRemove[p]);

        //removes the extension files if they exist
        extPath = toErase.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
        if(file_exists(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
        if(file_exists(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";
        if(file_exists(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }

        //removes info file and containing dirs if now empty
        if(file_exists(toErase.outputPath.getInfoPath().str()))
        {
            chmod(toErase.outputPath.getInfoPath().str().c_str(), 0666);
            remove_path(toErase.outputPath.getInfoPath());
        }

        //removes output file and containing dirs if now empty
        if(file_exists(toErase.outputPath.str()))
        {
            chmod(toErase.outputPath.str().c_str(), 0666);
            remove_path(toErase.outputPath);
        }

        //removes content file and containing dirs if now empty
        if(file_exists(toErase.contentPath.str()))
        {
            chmod(toErase.contentPath.str().c_str(), 0666);
            remove_path(toErase.contentPath);
        }

        //removes info from trackedAll set
        trackedAll.erase(toErase);

        //informs user that toErase was successfully removed
        std::cout << "successfully removed " << namesToRemove[p] << std::endl;
    }

    //saves new tracking list to .nsm/tracking.list
    save_tracking();

    return 0;
}

int ProjectInfo::mv(const Name& oldName, const Name& newName)
{
    if(newName.find('.') != std::string::npos)
    {
        start_err(std::cout) << "new name cannot contain '.'" << std::endl;
        return 1;
    }
    else if(newName == "")
    {
        start_err(std::cout) << "new name must be a non-empty string" << std::endl;
        return 1;
    }
    else if(unquote(newName).find('"') != std::string::npos && unquote(newName).find('\'') != std::string::npos)
    {
        start_err(std::cout) << "new name cannot contain both single and double quotes" << std::endl;
        return 1;
    }
    else if(!tracking(oldName)) //checks old name is being tracked
    {
        start_err(std::cout) << "Nift is not tracking " << oldName << std::endl;
        return 1;
    }
    else if(tracking(newName)) //checks new name isn't already tracked
    {
        start_err(std::cout) << "Nift is already tracking " << newName << std::endl;
        return 1;
    }

    TrackedInfo oldInfo = get_info(oldName);

    Path oldExtPath = oldInfo.outputPath.getInfoPath();
    std::string cContExt = contentExt,
                cOutputExt = outputExt;

    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".contExt";
    if(file_exists(oldExtPath.str()))
    {
        std::ifstream ifs(oldExtPath.str());
        getline(ifs, cContExt);
        ifs.close();
    }
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".outputExt";
    if(file_exists(oldExtPath.str()))
    {
        std::ifstream ifs(oldExtPath.str());
        getline(ifs, cOutputExt);
        ifs.close();
    }

    TrackedInfo newInfo;
    newInfo.name = newName;
    newInfo.contentPath.set_file_path_from(contentDir + newName + cContExt);
    newInfo.outputPath.set_file_path_from(outputDir + newName + cOutputExt);
    if(get_title(oldInfo.name) == oldInfo.title.str)
        newInfo.title = get_title(newName);
    else
        newInfo.title = oldInfo.title;
    newInfo.templatePath = oldInfo.templatePath;

    if(path_exists(newInfo.contentPath.str()))
    {
        start_err(std::cout) << "new content path " << newInfo.contentPath << " already exists" << std::endl;
        return 1;
    }
    else if(path_exists(newInfo.outputPath.str()))
    {
        start_err(std::cout) << "new output path " << newInfo.outputPath << " already exists" << std::endl;
        return 1;
    }

    //moves content file
    newInfo.contentPath.ensureDirExists();
    if(rename(oldInfo.contentPath.str().c_str(), newInfo.contentPath.str().c_str()))
    {
        start_err(std::cout) << "mv: failed to move " << oldInfo.contentPath << " to " << newInfo.contentPath << std::endl;
        return 1;
    }

    //moves extension files if they exist
    Path newExtPath = newInfo.outputPath.getInfoPath();
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".contExt";
    if(file_exists(oldExtPath.str()))
    {
        newExtPath.file = newExtPath.file.substr(0, newExtPath.file.find_first_of('.')) + ".contExt";
        newExtPath.ensureDirExists();
        chmod(oldExtPath.str().c_str(), 0666);
        if(rename(oldExtPath.str().c_str(), newExtPath.str().c_str()))
        {
            start_err(std::cout) << "mv: failed to move " << oldExtPath << " to " << newExtPath << std::endl;
            return 1;
        }
    }
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".outputExt";
    if(file_exists(oldExtPath.str()))
    {
        newExtPath.file = newExtPath.file.substr(0, newExtPath.file.find_first_of('.')) + ".outputExt";
        newExtPath.ensureDirExists();
        chmod(oldExtPath.str().c_str(), 0666);
        if(rename(oldExtPath.str().c_str(), newExtPath.str().c_str()))
        {
            start_err(std::cout) << "mv: failed to move " << oldExtPath << " to " << newExtPath << std::endl;
            return 1;
        }
    }
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".scriptExt";
    if(file_exists(oldExtPath.str()))
    {
        newExtPath.file = newExtPath.file.substr(0, newExtPath.file.find_first_of('.')) + ".scriptExt";
        newExtPath.ensureDirExists();
        chmod(oldExtPath.str().c_str(), 0666);
        if(rename(oldExtPath.str().c_str(), newExtPath.str().c_str()))
        {
            start_err(std::cout) << "mv: failed to move " << oldExtPath << " to " << newExtPath << std::endl;
            return 1;
        }
    }

    //removes info file and containing dirs if now empty
    if(file_exists(oldInfo.outputPath.getInfoPath().str()))
    {
        chmod(oldInfo.outputPath.getInfoPath().str().c_str(), 0666);
        remove_path(oldInfo.outputPath.getInfoPath());
    }

    //removes output file and containing dirs if now empty
    if(file_exists(oldInfo.outputPath.str()))
    {
        chmod(oldInfo.outputPath.str().c_str(), 0666);
        remove_path(oldInfo.outputPath);
    }

    //removes oldInfo from trackedAll
    trackedAll.erase(oldInfo);
    //adds newInfo to trackedAll
    trackedAll.insert(newInfo);

    //saves new tracking list to .nsm/tracking.list
    save_tracking();

    //informs user that name was successfully moved
    std::cout << "successfully moved " << oldName << " to " << newName << std::endl;

    return 0;
}

int ProjectInfo::cp(const Name& trackedName, const Name& newName)
{
    if(newName.find('.') != std::string::npos)
    {
        start_err(std::cout) << "new name cannot contain '.'" << std::endl;
        return 1;
    }
    else if(newName == "")
    {
        start_err(std::cout) << "new name must be a non-empty string" << std::endl;
        return 1;
    }
    else if(unquote(newName).find('"') != std::string::npos && unquote(newName).find('\'') != std::string::npos)
    {
        start_err(std::cout) << "new name cannot contain both single and double quotes" << std::endl;
        return 1;
    }
    else if(!tracking(trackedName)) //checks old name is being tracked
    {
        start_err(std::cout) << "Nift is not tracking " << trackedName << std::endl;
        return 1;
    }
    else if(tracking(newName)) //checks new name isn't already tracked
    {
        start_err(std::cout) << "Nift is already tracking " << newName << std::endl;
        return 1;
    }

    TrackedInfo trackedInfo = get_info(trackedName);

    Path oldExtPath = trackedInfo.outputPath.getInfoPath();
    std::string cContExt = contentExt,
                cOutputExt = outputExt;

    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".contExt";
    if(file_exists(oldExtPath.str()))
    {
        std::ifstream ifs(oldExtPath.str());
        getline(ifs, cContExt);
        ifs.close();
    }
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".outputExt";
    if(file_exists(oldExtPath.str()))
    {
        std::ifstream ifs(oldExtPath.str());
        getline(ifs, cOutputExt);
        ifs.close();
    }

    TrackedInfo newInfo;
    newInfo.name = newName;
    newInfo.contentPath.set_file_path_from(contentDir + newName + cContExt);
    newInfo.outputPath.set_file_path_from(outputDir + newName + cOutputExt);
    if(get_title(trackedInfo.name) == trackedInfo.title.str)
        newInfo.title = get_title(newName);
    else
        newInfo.title = trackedInfo.title;
    newInfo.templatePath = trackedInfo.templatePath;

    if(path_exists(newInfo.contentPath.str()))
    {
        start_err(std::cout) << "new content path " << newInfo.contentPath << " already exists" << std::endl;
        return 1;
    }
    else if(path_exists(newInfo.outputPath.str()))
    {
        start_err(std::cout) << "new output path " << newInfo.outputPath << " already exists" << std::endl;
        return 1;
    }

    //copies content file
    std::mutex os_mtx;
    Path emptyPath("", "");
    if(cpFile(trackedInfo.contentPath.str(), newInfo.contentPath.str(), -1, emptyPath, std::cout, 0, &os_mtx))
    {
        start_err(std::cout) << "cp: failed to copy " << trackedInfo.contentPath << " to " << newInfo.contentPath << std::endl;
        return 1;
    }

    //copies extension files if they exist
    Path newExtPath = newInfo.outputPath.getInfoPath();
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".contExt";
    if(file_exists(oldExtPath.str()))
    {
        newExtPath.file = newExtPath.file.substr(0, newExtPath.file.find_first_of('.')) + ".contExt";
        newExtPath.ensureDirExists();
        if(cpFile(oldExtPath.str(), newExtPath.str(), -1, emptyPath, std::cout, 0, &os_mtx))
        {
            start_err(std::cout) << "cp: failed to copy " << oldExtPath << " to " << newExtPath << std::endl;
            return 1;
        }
        chmod(newExtPath.str().c_str(), 0444);
    }
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".outputExt";
    if(file_exists(oldExtPath.str()))
    {
        newExtPath.file = newExtPath.file.substr(0, newExtPath.file.find_first_of('.')) + ".outputExt";
        newExtPath.ensureDirExists();
        if(cpFile(oldExtPath.str(), newExtPath.str(), -1, emptyPath, std::cout, 0, &os_mtx))
        {
            start_err(std::cout) << "cp: failed to copy " << oldExtPath << " to " << newExtPath << std::endl;
            return 1;
        }
        chmod(newExtPath.str().c_str(), 0444);
    }
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".scriptExt";
    if(file_exists(oldExtPath.str()))
    {
        newExtPath.file = newExtPath.file.substr(0, newExtPath.file.find_first_of('.')) + ".scriptExt";
        newExtPath.ensureDirExists();
        if(cpFile(oldExtPath.str(), newExtPath.str(), -1, emptyPath, std::cout, 0, &os_mtx))
        {
            start_err(std::cout) << "cp: failed to copy " << oldExtPath << " to " << newExtPath << std::endl;
            return 1;
        }
        chmod(newExtPath.str().c_str(), 0444);
    }

    //adds newInfo to trackedAll
    trackedAll.insert(newInfo);

    //saves new tracking list to .nsm/tracking.list
    save_tracking();

    //informs user that name was successfully moved
    std::cout << "successfully copied " << trackedName << " to " << newName << std::endl;

    return 0;
}

int ProjectInfo::new_title(const Name& name, const Title& newTitle)
{
    if(newTitle.str == "")
    {
        start_err(std::cout) << "new title must be a non-empty string" << std::endl;
        return 1;
    }
    else if(unquote(newTitle.str).find('"') != std::string::npos && unquote(newTitle.str).find('\'') != std::string::npos)
    {
        start_err(std::cout) << "new title cannot contain both single and double quotes" << std::endl;
        return 1;
    }

    TrackedInfo cInfo;
    cInfo.name = name;
    if(trackedAll.count(cInfo))
    {
        cInfo = *(trackedAll.find(cInfo));
        trackedAll.erase(cInfo);
        cInfo.title = newTitle;
        trackedAll.insert(cInfo);

        //saves new tracking list to .nsm/tracking.list
        save_tracking();

        //informs user that title was successfully changed
        std::cout << "successfully changed title to " << quote(newTitle.str) << std::endl;
    }
    else
    {
        start_err(std::cout) << "Nift is not tracking " << quote(name) << std::endl;
        return 1;
    }

    return 0;
}

int ProjectInfo::new_template(const Path& newTemplatePath)
{
    bool blankTemplate = 0;
    if(newTemplatePath == Path("", ""))
        blankTemplate = 1;
    else if(unquote(newTemplatePath.str()).find('"') != std::string::npos && unquote(newTemplatePath.str()).find('\'') != std::string::npos)
    {
        start_err(std::cout) << "new template path cannot contain both single and double quotes" << std::endl;
        return 1;
    }
    else if(defaultTemplate == newTemplatePath)
    {
        start_err(std::cout) << "default template path is already " << defaultTemplate << std::endl;
        return 1;
    }

    Name name;
    Title title;
    Path oldTemplatePath;
    rename(".nsm/tracking.list", ".nsm/tracking.list-old");
    std::ifstream ifs(".nsm/tracking.list-old");
    std::ofstream ofs(".nsm/tracking.list");
    while(read_quoted(ifs, name))
    {
        title.read_quoted_from(ifs);
        oldTemplatePath.read_file_path_from(ifs);

        ofs << quote(name) << std::endl;
        ofs << title << std::endl;
        if(oldTemplatePath == defaultTemplate)
            ofs << newTemplatePath << std::endl << std::endl;
        else
            ofs << oldTemplatePath << std::endl << std::endl;
    }
    ifs.close();
    ofs.close();
    remove_file(Path(".nsm/", "tracking.list-old"));

    //sets new template
    defaultTemplate = newTemplatePath;

    //saves new default template to Nift config file
    save_local_config();

    //warns user if new template path doesn't exist
    if(!blankTemplate && !file_exists(newTemplatePath.str()))
        std::cout << "warning: new template path " << newTemplatePath << " does not exist" << std::endl;

    std::cout << "successfully changed default template to " << newTemplatePath << std::endl;

    return 0;
}

int ProjectInfo::new_template(const Name& name, const Path& newTemplatePath)
{
    bool blankTemplate = 0;
    if(newTemplatePath == Path("", ""))
        blankTemplate = 1;
    else if(unquote(newTemplatePath.str()).find('"') != std::string::npos && unquote(newTemplatePath.str()).find('\'') != std::string::npos)
    {
        start_err(std::cout) << "new template path cannot contain both single and double quotes" << std::endl;
        return 1;
    }

    TrackedInfo cInfo;
    cInfo.name = name;
    if(trackedAll.count(cInfo))
    {
        cInfo = *(trackedAll.find(cInfo));

        if(cInfo.templatePath == newTemplatePath)
        {
            start_err(std::cout) << "template path for " << name << " is already " << newTemplatePath << std::endl;
            return 1;
        }

        trackedAll.erase(cInfo);
        cInfo.templatePath = newTemplatePath;
        trackedAll.insert(cInfo);
        //saves new tracking list to .nsm/tracking.list
        save_tracking();

        //warns user if new template path doesn't exist
        if(!blankTemplate && !file_exists(newTemplatePath.str()))
            std::cout << "warning: new template path " << newTemplatePath << " does not exist" << std::endl;
    }
    else
    {
        start_err(std::cout) << "Nift is not tracking " << name << std::endl;
        return 1;
    }

    return 0;
}

int ProjectInfo::new_output_dir(const Directory& newOutputDir)
{
    if(newOutputDir == "")
    {
        start_err(std::cout) << "new output directory cannot be the empty string" << std::endl;
        return 1;
    }
    else if(unquote(newOutputDir).find('"') != std::string::npos && unquote(newOutputDir).find('\'') != std::string::npos)
    {
        start_err(std::cout) << "new output directory cannot contain both single and double quotes" << std::endl;
        return 1;
    }

    if(newOutputDir[newOutputDir.size()-1] != '/' && newOutputDir[newOutputDir.size()-1] != '\\')
    {
        start_err(std::cout) << "new output directory should end in \\ or /" << std::endl;
        return 1;
    }

    if(newOutputDir == outputDir)
    {
        start_err(std::cout) << "output directory is already " << quote(newOutputDir) << std::endl;
        return 1;
    }

    if(path_exists(newOutputDir) || path_exists(newOutputDir.substr(0, newOutputDir.size()-1)))
    {
        start_err(std::cout) << "new output directory location " << quote(newOutputDir) << " already exists" << std::endl;
        return 1;
    }

    std::string newInfoDir = ".nsm/" + newOutputDir;

    if(path_exists(newInfoDir) || path_exists(newInfoDir.substr(0, newInfoDir.size()-1)))
    {
        start_err(std::cout) << "new info directory location " << quote(newInfoDir) << " already exists" << std::endl;
        return 1;
    }

    std::string parDir = "../",
                rootDir = "/",
                projectRootDir = get_pwd(),
                pwd = get_pwd();

    int ret_val = chdir(outputDir.c_str());
    if(ret_val)
    {
        start_err(std::cout) << "new_output_dir(" << quote(newOutputDir) << "): failed to change directory to " << quote(outputDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    ret_val = chdir(parDir.c_str());
    if(ret_val)
    {
        start_err(std::cout) << "new_output_dir(" << quote(newOutputDir) << "): failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    std::string delDir = get_pwd();
    ret_val = chdir(projectRootDir.c_str());
    if(ret_val)
    {
        start_err(std::cout) << "new_output_dir(" << quote(newOutputDir) << "): failed to change directory to " << quote(projectRootDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    //moves output directory to temp location
    rename(outputDir.c_str(), ".temp_output_dir");

    //ensures new output directory exists
    Path(newOutputDir, Filename("")).ensureDirExists();

    //moves output directory to final location
    rename(".temp_output_dir", newOutputDir.c_str());

    //deletes any remaining empty directories
    if(remove_path(Path(delDir, "")))
    {
        start_err(std::cout) << "new_output_dir(" << quote(newOutputDir) << "): failed to remove " << quote(delDir) << std::endl;
        return 1;
    }

    //changes back to project root directory
    ret_val = chdir(projectRootDir.c_str());
    if(ret_val)
    {
        start_err(std::cout) << "new_output_dir(" << quote(newOutputDir) << "): failed to change directory to " << quote(projectRootDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    ret_val = chdir((".nsm/" + outputDir).c_str());
    if(ret_val)
    {
        start_err(std::cout) << "new_output_dir(" << quote(newOutputDir) << "): failed to change directory to " << quote(".nsm/" + outputDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    ret_val = chdir(parDir.c_str());
    if(ret_val)
    {
        start_err(std::cout) << "new_output_dir(" << quote(newOutputDir) << "): failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    delDir = get_pwd();
    ret_val = chdir(projectRootDir.c_str());
    if(ret_val)
    {
        start_err(std::cout) << "new_output_dir(" << quote(newOutputDir) << "): failed to change directory to " << quote(projectRootDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    //moves old info directory to temp location
    rename((".nsm/" + outputDir).c_str(), ".temp_info_dir");

    //ensures new info directory exists
    Path(".nsm/" + newOutputDir, Filename("")).ensureDirExists();

    //moves old info directory to final location
    rename(".temp_info_dir", (".nsm/" + newOutputDir).c_str());

    //deletes any remaining empty directories
    if(remove_path(Path(delDir, "")))
    {
        start_err(std::cout) << "new_output_dir(" << quote(newOutputDir) << "): failed to remove " << quote(delDir) << std::endl;
        return 1;
    }

    //changes back to project root directory
    ret_val = chdir(projectRootDir.c_str());
    if(ret_val)
    {
        start_err(std::cout) << "new_output_dir(" << quote(newOutputDir) << "): failed to change directory to " << quote(projectRootDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    //sets new output directory
    outputDir = newOutputDir;

    //saves new output directory to Nift config file
    save_local_config();

    std::cout << "successfully changed output directory to " << quote(newOutputDir) << std::endl;

    return 0;
}

int ProjectInfo::new_content_dir(const Directory& newContDir)
{
    if(newContDir == "")
    {
        start_err(std::cout) << "new content directory cannot be the empty string" << std::endl;
        return 1;
    }
    else if(unquote(newContDir).find('"') != std::string::npos && unquote(newContDir).find('\'') != std::string::npos)
    {
        start_err(std::cout) << "new content directory cannot contain both single and double quotes" << std::endl;
        return 1;
    }

    if(newContDir[newContDir.size()-1] != '/' && newContDir[newContDir.size()-1] != '\\')
    {
        start_err(std::cout) << "new content directory should end in \\ or /" << std::endl;
        return 1;
    }

    if(newContDir == contentDir)
    {
        start_err(std::cout) << "content directory is already " << quote(newContDir) << std::endl;
        return 1;
    }

    if(path_exists(newContDir) || path_exists(newContDir.substr(0, newContDir.size()-1)))
    {
        start_err(std::cout) << "new content directory location " << quote(newContDir) << " already exists" << std::endl;
        return 1;
    }

    std::string parDir = "../",
                rootDir = "/",
                projectRootDir = get_pwd(),
                pwd = get_pwd();

    int ret_val = chdir(contentDir.c_str());
    if(ret_val)
    {
        start_err(std::cout) << "new_content_dir(" << quote(newContDir) << "): failed to change directory to " << quote(contentDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    ret_val = chdir(parDir.c_str());
    if(ret_val)
    {
        start_err(std::cout) << "new_content_dir(" << quote(newContDir) << "): failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    std::string delDir = get_pwd();
    ret_val = chdir(projectRootDir.c_str());
    if(ret_val)
    {
        start_err(std::cout) << "new_content_dir(" << quote(newContDir) << "): failed to change directory to " << quote(projectRootDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    //moves content directory to temp location
    rename(contentDir.c_str(), ".temp_content_dir");

    //ensures new content directory exists
    Path(newContDir, Filename("")).ensureDirExists();

    //moves content directory to final location
    rename(".temp_content_dir", newContDir.c_str());

    //deletes any remaining empty directories
    if(remove_path(Path(delDir, "")))
    {
        start_err(std::cout) << "new_content_dir(" << quote(newContDir) << "): failed to remove " << quote(delDir) << std::endl;
        return 1;
    }

    //changes back to project root directory
    ret_val = chdir(projectRootDir.c_str());
    if(ret_val)
    {
        start_err(std::cout) << "new_content_dir(" << quote(newContDir) << "): failed to change directory to " << quote(projectRootDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    //sets new content directory
    contentDir = newContDir;

    //saves new content directory to Nift config file
    save_local_config();

    std::cout << "successfully changed content directory to " << quote(newContDir) << std::endl;

    return 0;
}

int ProjectInfo::new_content_ext(const std::string& newExt)
{
    if(newExt == "" || newExt[0] != '.')
    {
        start_err(std::cout) << "content extension must start with a fullstop" << std::endl;
        return 1;
    }

    if(newExt == contentExt)
    {
        start_err(std::cout) << "content extension is already " << quote(contentExt) << std::endl;
        return 1;
    }

    Path newContPath, extPath;

    for(auto trackedInfo=trackedAll.begin(); trackedInfo!=trackedAll.end(); trackedInfo++)
    {
        newContPath = trackedInfo->contentPath;
        newContPath.file = newContPath.file.substr(0, newContPath.file.find_first_of('.')) + newExt;

        if(path_exists(newContPath.str()))
        {
            start_err(std::cout) << "new content path " << newContPath << " already exists" << std::endl;
            return 1;
        }
    }

    for(auto trackedInfo=trackedAll.begin(); trackedInfo!=trackedAll.end(); trackedInfo++)
    {
        //checks for non-default content extension
        extPath = trackedInfo->outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";

        if(file_exists(extPath.str()))
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
            if(file_exists(trackedInfo->contentPath.str())) //should we throw an error here if it doesn't exit?
            {
                newContPath = trackedInfo->contentPath;
                newContPath.file = newContPath.file.substr(0, newContPath.file.find_first_of('.')) + newExt;
                if(newContPath.str() != trackedInfo->contentPath.str())
                    rename(trackedInfo->contentPath.str().c_str(), newContPath.str().c_str());
            }
        }
    }

    contentExt = newExt;

    save_local_config();

    //informs user that content extension was successfully changed
    std::cout << "successfully changed content extension to " << quote(newExt) << std::endl;

    return 0;
}

int ProjectInfo::new_content_ext(const Name& name, const std::string& newExt)
{
    if(newExt == "" || newExt[0] != '.')
    {
        start_err(std::cout) << "content extension must start with a fullstop" << std::endl;
        return 1;
    }

    TrackedInfo cInfo = get_info(name);
    Path newContPath, extPath;
    std::string oldExt;
    int pos;
    if(trackedAll.count(cInfo))
    {
        pos = cInfo.contentPath.file.find_first_of('.');
        oldExt = cInfo.contentPath.file.substr(pos, cInfo.contentPath.file.size() - pos);

        if(oldExt == newExt)
        {
            start_err(std::cout) << "content extension for " << quote(name) << " is already " << quote(oldExt) << std::endl;
            return 1;
        }

        //throws error if new content file already exists
        newContPath = cInfo.contentPath;
        newContPath.file = newContPath.file.substr(0, newContPath.file.find_first_of('.')) + newExt;
        if(path_exists(newContPath.str()))
        {
            start_err(std::cout) << "new content path " << newContPath << " already exists" << std::endl;
            return 1;
        }

        //moves the content file
        if(file_exists(cInfo.contentPath.str()))
        {
            if(newContPath.str() != cInfo.contentPath.str())
                rename(cInfo.contentPath.str().c_str(), newContPath.str().c_str());
        }

        extPath = cInfo.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";

        if(newExt != contentExt)
        {
            //makes sure we can write to ext file
            chmod(extPath.str().c_str(), 0644);

            extPath.ensureFileExists();
            std::ofstream ofs(extPath.str());
            ofs << newExt << "\n";
            ofs.close();

            //makes sure user can't edit ext file
            chmod(extPath.str().c_str(), 0444);
        }
        else
        {
            chmod(extPath.str().c_str(), 0644);
            if(file_exists(extPath.str()) && remove_path(extPath))
            {
                start_err(std::cout) << "faield to remove content extension path " << extPath << std::endl;
                return 1;
            }
        }
    }
    else
    {
        start_err(std::cout) << "Nift is not tracking " << quote(name) << std::endl;
        return 1;
    }

    return 0;
}

int ProjectInfo::new_output_ext(const std::string& newExt)
{
    if(newExt == "" || newExt[0] != '.')
    {
        start_err(std::cout) << "output extension must start with a fullstop" << std::endl;
        return 1;
    }

    if(newExt == outputExt)
    {
        start_err(std::cout) << "output extension is already " << quote(outputExt) << std::endl;
        return 1;
    }

    Path newOutputPath, extPath;

    for(auto trackedInfo=trackedAll.begin(); trackedInfo!=trackedAll.end(); trackedInfo++)
    {
        newOutputPath = trackedInfo->outputPath;
        newOutputPath.file = newOutputPath.file.substr(0, newOutputPath.file.find_first_of('.')) + newExt;

        if(path_exists(newOutputPath.str()))
        {
            start_err(std::cout) << "new output path " << newOutputPath << " already exists" << std::endl;
            return 1;
        }
    }

    for(auto trackedInfo=trackedAll.begin(); trackedInfo!=trackedAll.end(); trackedInfo++)
    {
        //checks for non-default output extension
        extPath = trackedInfo->outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";

        if(file_exists(extPath.str()))
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
            //moves the output file
            if(file_exists(trackedInfo->outputPath.str()))
            {
                newOutputPath = trackedInfo->outputPath;
                newOutputPath.file = newOutputPath.file.substr(0, newOutputPath.file.find_first_of('.')) + newExt;
                if(newOutputPath.str() != trackedInfo->outputPath.str())
                    rename(trackedInfo->outputPath.str().c_str(), newOutputPath.str().c_str());
            }
        }
    }

    outputExt = newExt;

    save_local_config();

    //informs user that output extension was successfully changed
    std::cout << "successfully changed output extension to " << quote(newExt) << std::endl;

    return 0;
}

int ProjectInfo::new_output_ext(const Name& name, const std::string& newExt)
{
    if(newExt == "" || newExt[0] != '.')
    {
        start_err(std::cout) << "output extension must start with a fullstop" << std::endl;
        return 1;
    }

    TrackedInfo cInfo = get_info(name);
    Path newOutputPath, extPath;
    std::string oldExt;
    int pos;
    if(trackedAll.count(cInfo))
    {
        //checks for non-default output extension
        pos = cInfo.outputPath.file.find_first_of('.');
        oldExt = cInfo.outputPath.file.substr(pos, cInfo.outputPath.file.size() - pos);

        if(oldExt == newExt)
        {
            start_err(std::cout) << "output extension for " << quote(name) << " is already " << quote(oldExt) << std::endl;
            return 1;
        }

        //throws error if new output file already exists
        newOutputPath = cInfo.outputPath;
        newOutputPath.file = newOutputPath.file.substr(0, newOutputPath.file.find_first_of('.')) + newExt;
        if(path_exists(newOutputPath.str()))
        {
            start_err(std::cout) << "new output path " << newOutputPath << " already exists" << std::endl;
            return 1;
        }

        //moves the output file
        if(file_exists(cInfo.outputPath.str()))
        {
            if(newOutputPath.str() != cInfo.outputPath.str())
                rename(cInfo.outputPath.str().c_str(), newOutputPath.str().c_str());
        }

        extPath = cInfo.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";

        if(newExt != outputExt)
        {
            //makes sure we can write to ext file
            chmod(extPath.str().c_str(), 0644);

            extPath.ensureFileExists();
            std::ofstream ofs(extPath.str());
            ofs << newExt << "\n";
            ofs.close();

            //makes sure user can't edit ext file
            chmod(extPath.str().c_str(), 0444);
        }
        else
        {
            chmod(extPath.str().c_str(), 0644);
            if(file_exists(extPath.str()) && remove_path(extPath))
            {
                start_err(std::cout) << "faield to remove output extension path " << extPath << std::endl;
                return 1;
            }
        }
    }
    else
    {
        start_err(std::cout) << "Nift is not tracking " << quote(name) << std::endl;
        return 1;
    }

    return 0;
}

int ProjectInfo::new_script_ext(const std::string& newExt)
{
    if(newExt != "" && newExt[0] != '.')
    {
        start_err(std::cout) << "non-empty script extension must start with a fullstop" << std::endl;
        return 1;
    }

    if(newExt == scriptExt)
    {
        start_err(std::cout) << "script extension is already " << quote(scriptExt) << std::endl;
        return 1;
    }

    Path extPath;

    for(auto trackedInfo=trackedAll.begin(); trackedInfo!=trackedAll.end(); trackedInfo++)
    {
        //checks for non-default script extension
        extPath = trackedInfo->outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";

        if(file_exists(extPath.str()))
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

    scriptExt = newExt;

    save_local_config();

    //informs user that script extension was successfully changed
    std::cout << "successfully changed script extension to " << quote(newExt) << std::endl;

    return 0;
}

int ProjectInfo::new_script_ext(const Name& name, const std::string& newExt)
{
    if(newExt != "" && newExt[0] != '.')
    {
        start_err(std::cout) << "non-empty script extension must start with a fullstop" << std::endl;
        return 1;
    }

    TrackedInfo cInfo = get_info(name);
    Path extPath;
    std::string oldExt;
    if(trackedAll.count(cInfo))
    {
        extPath = cInfo.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";

        if(file_exists(extPath.str()))
        {
            std::ifstream ifs(extPath.str());
            getline(ifs, oldExt);
            ifs.close();
        }
        else
            oldExt = scriptExt;

        if(oldExt == newExt)
        {
            start_err(std::cout) << "script extension for " << quote(name) << " is already " << quote(oldExt) << std::endl;
            return 1;
        }

        if(newExt != scriptExt)
        {
            //makes sure we can write to ext file
            chmod(extPath.str().c_str(), 0644);

            extPath.ensureFileExists();
            std::ofstream ofs(extPath.str());
            ofs << newExt << "\n";
            ofs.close();

            //makes sure user can't edit ext file
            chmod(extPath.str().c_str(), 0444);
        }
        else
        {
            chmod(extPath.str().c_str(), 0644);
            if(file_exists(extPath.str()) && remove_path(extPath))
            {
                start_err(std::cout) << "faield to remove script extension path " << extPath << std::endl;
                return 1;
            }
        }
    }
    else
    {
        start_err(std::cout) << "Nift is not tracking " << quote(name) << std::endl;
        return 1;
    }

    return 0;
}

int ProjectInfo::no_build_threads(int no_threads)
{
    if(no_threads == 0)
    {
        start_err(std::cout) << "number of build threads must be a non-zero integer (use negative numbers for a multiple of the number of cores on the machine)" << std::endl;
        return 1;
    }

    if(no_threads == buildThreads)
    {
        start_err(std::cout) << "number of build threads is already " << buildThreads << std::endl;
        return 1;
    }

    buildThreads = no_threads;
    save_local_config();

    if(buildThreads < 0)
        std::cout << "successfully changed number of build threads to " << no_threads << " (" << -buildThreads*std::thread::hardware_concurrency() << " on this machine)" << std::endl;
    else
        std::cout << "successfully changed number of build threads to " << no_threads << std::endl;

    return 0;
}

int ProjectInfo::no_paginate_threads(int no_threads)
{
    if(no_threads == 0)
    {
        start_err(std::cout) << "number of paginate threads must be a non-zero integer (use negative numbers for a multiple of the number of cores on the machine)" << std::endl;
        return 1;
    }

    if(no_threads == paginateThreads)
    {
        start_err(std::cout) << "number of paginate threads is already " << paginateThreads << std::endl;
        return 1;
    }

    paginateThreads = no_threads;
    save_local_config();

    if(paginateThreads < 0)
        std::cout << "successfully changed number of paginate threads to " << no_threads << " (" << -paginateThreads*std::thread::hardware_concurrency() << " on this machine)" << std::endl;
    else
        std::cout << "successfully changed number of paginate threads to " << no_threads << std::endl;

    return 0;
}

int ProjectInfo::check_watch_dirs()
{
    if(file_exists(".nsm/.watchinfo/watching.list"))
    {
        WatchList wl;
        if(wl.open())
        {
            start_err(std::cout) << "failed to open watch list '.nsm/.watchinfo/watching.list'" << std::endl;
            return 1;
        }

        for(auto wd=wl.dirs.begin(); wd!=wl.dirs.end(); wd++)
        {
            std::string file,
                        watchDirFilesStr = ".nsm/.watchinfo/" + strip_trailing_slash(wd->watchDir) + ".tracked";
            TrackedInfo cInfo;

            //can delete this later
            std::string watchDirFilesStrOld = ".nsm/.watchinfo/" + replace_slashes(wd->watchDir) + ".tracked";
            if(file_exists(watchDirFilesStrOld))
            {
                Path(watchDirFilesStr).ensureDirExists();
                if(rename(watchDirFilesStrOld.c_str(), watchDirFilesStr.c_str()))
                {
                    start_err(std::cout) << "failed to rename " << quote(watchDirFilesStrOld) << " to " << quote(watchDirFilesStr) << std::endl;
                    return 1;
                }
            }

            std::ifstream ifs(watchDirFilesStr);
            std::vector<Name> namesToRemove;
            while(read_quoted(ifs, file))
            {
                cInfo = get_info(file);
                if(cInfo.name != "##not-found##")
                    if(!file_exists(cInfo.contentPath.str()))
                        namesToRemove.push_back(file);
            }
            ifs.close();

            if(namesToRemove.size())
                rm(namesToRemove);

            std::vector<InfoToTrack> to_track;
            std::set<Name> names_tracked;
            std::string ext;
            Name name;
            size_t pos;
            std::vector<std::string> files = lsVec(wd->watchDir.c_str());
            for(size_t f=0; f<files.size(); f++)
            {
                pos = files[f].find_first_of('.');
                if(pos != std::string::npos)
                {
                    ext = files[f].substr(pos, files[f].size()-pos);
                    if(wd->watchDir.size() == contentDir.size())
                        name = files[f].substr(0, pos);
                    else
                        name = wd->watchDir.substr(contentDir.size(), wd->watchDir.size()-contentDir.size()) + files[f].substr(0, pos);

                    if(wd->contExts.count(ext)) //we are watching the extension
                    {
                        if(names_tracked.count(name))
                        {
                            start_err(std::cout) << "watch: two content files with name '" << name << "', one with extension " << quote(ext) << std::endl;
                            return 1;
                        }

                        if(!tracking(name))
                            to_track.push_back(InfoToTrack(name, Title(name), wd->templatePaths.at(ext), ext, wd->outputExts.at(ext)));

                        names_tracked.insert(name);
                    }
                }
            }

            if(to_track.size())
                if(track(to_track))
                    return 1;

            //makes sure we can write to watchDir tracked file
            chmod(watchDirFilesStr.c_str(), 0644);

            std::ofstream ofs(watchDirFilesStr);

            for(auto tracked_name=names_tracked.begin(); tracked_name != names_tracked.end(); tracked_name++)
                ofs << quote(*tracked_name) << std::endl;

            ofs.close();

            //makes sure user can't accidentally write to watchDir tracked file
            chmod(watchDirFilesStr.c_str(), 0444);
        }
    }

    return 0;
}

std::mutex os_mtx;

std::mutex fail_mtx, built_mtx, set_mtx;
std::set<Name> failedNames, builtNames;
std::set<TrackedInfo>::iterator nextInfo;

Timer timer;
std::atomic<int> counter, noFinished, noPagesToBuild, noPagesFinished;
std::atomic<double> estNoPagesFinished;
std::atomic<int> cPhase;
const int DUMMY_PHASE = 1;
const int UPDATE_PHASE = 2;
const int BUILD_PHASE = 3;
const int END_PHASE = 4;

void build_progress(const int& no_to_build, const int& addBuildStatus)
{
    if(!addBuildStatus)
        return;

    #if defined __NO_PROGRESS__
        return;
    #endif

    int phaseToCheck = cPhase;

    int total_no_to_build, no_to_build_length, cFinished;

    while(1)
    {
        os_mtx.lock();

        total_no_to_build = no_to_build + noPagesToBuild;
        no_to_build_length = std::to_string(total_no_to_build).size();
        cFinished = noFinished + estNoPagesFinished;

        if(cPhase != phaseToCheck || cFinished >= total_no_to_build)
        {
            os_mtx.unlock();
            break;
        }

        clear_console_line();

        double cTime = timer.getTime();
        std::string remStr = int_to_timestr(cTime*(double)total_no_to_build/(double)(cFinished+1) - cTime);
        size_t cWidth = console_width();
        //std::cout << "wtf " << cFinished << " " << total_no_to_build << " " << cPhase << std::endl;
        if(19 + 2*no_to_build_length + remStr.size() <= cWidth)
        {
            if(cFinished < total_no_to_build)
                std::cout << c_light_blue << "progress: " << c_white << cFinished << "/" << total_no_to_build << " " << (100*cFinished)/total_no_to_build << "% (" << remStr << ")";
        }
        else if(17 + remStr.size() <= cWidth)
        {
            if(cFinished < total_no_to_build)
                std::cout << c_light_blue << "progress: " << c_white << (100*cFinished)/total_no_to_build << "% (" << remStr << ")";
        }
        else if(9 + remStr.size() <= cWidth)
        {
            if(cFinished < total_no_to_build)
                std::cout << c_light_blue << ": " << c_white << (100*cFinished)/total_no_to_build << "% (" << remStr << ")";
        }
        else if(4 <= cWidth)
        {
            if(cFinished < total_no_to_build)
                std::cout << c_light_blue << (100*cFinished)/total_no_to_build << "%" << c_white;
        }
        //std::cout << std::flush;
        if(addBuildStatus == 1)
        {
            std::fflush(stdout);
            usleep(200000);
        }
        else
        {
            std::cout << std::endl;
            usleep(990000);
        }
        //usleep(2000000);
        //usleep(200000);
        clear_console_line();
        os_mtx.unlock();

        //usleep(4000000);
        usleep(10000);
    }
}

std::atomic<size_t> paginationCounter;

void pagination_thread(const Pagination& pagesInfo,
                       const std::string& paginateName,
                       const std::string& outputExt,
                       const Path& mainOutputPath)
{
    std::string pageStr;
    size_t cPageNo;

    while(1)
    {
        cPageNo = paginationCounter++;
        if(cPageNo >= pagesInfo.noPages)
            break;

        std::istringstream iss(pagesInfo.pages[cPageNo]);
        std::string issLine;
        int fileLineNo = 0;

        pageStr = pagesInfo.splitFile.first;

        while(!iss.eof())
        {
            getline(iss, issLine);
            if(0 < fileLineNo++)
                pageStr += "\n" + pagesInfo.indentAmount;
            pageStr += issLine;
        }

        pageStr += pagesInfo.splitFile.second;

        Path outputPath = mainOutputPath;
        if(cPageNo)
            outputPath.file = paginateName + std::to_string(cPageNo+1) + outputExt;

        chmod(outputPath.str().c_str(), 0666);

        //removing file here gives significant speed improvements
        //same improvements NOT observed for output files or info files at scale
        //not sure why!
        remove_file(outputPath); 

        std::ofstream ofs(outputPath.str());
        ofs << pageStr << "\n";
        ofs.close();

        chmod(outputPath.str().c_str(), 0444);

        estNoPagesFinished = estNoPagesFinished + 0.55;
        noPagesFinished++;
    }

}

void build_thread(std::ostream& eos,
                  const int& no_paginate_threads,
                  std::set<TrackedInfo>* trackedAll,
                  const int& no_to_build,
                  const Directory& ContentDir,
                  const Directory& OutputDir,
                  const std::string& ContentExt,
                  const std::string& OutputExt,
                  const std::string& ScriptExt,
                  const Path& DefaultTemplate,
                  const bool& BackupScripts,
                  const std::string& UnixTextEditor,
                  const std::string& WinTextEditor)
{
    Parser parser(trackedAll, 
                  &os_mtx, 
                  ContentDir, 
                  OutputDir, 
                  ContentExt, 
                  OutputExt, 
                  ScriptExt, 
                  DefaultTemplate, 
                  BackupScripts, 
                  UnixTextEditor, 
                  WinTextEditor);
    std::set<TrackedInfo>::iterator cInfo;

    while(counter < no_to_build)
    {
        set_mtx.lock();
        if(counter >= no_to_build)
        {
            set_mtx.unlock();
            return;
        }
        counter++;
        cInfo = nextInfo++;
        set_mtx.unlock();

        int result = parser.build(*cInfo, estNoPagesFinished, noPagesToBuild, eos);

        //more pagination here
        parser.pagesInfo.noPages = parser.pagesInfo.pages.size();
        if(parser.pagesInfo.noPages)
        {
            Path outputPathBackup = parser.toBuild.outputPath;

            std::string innerPageStr;
            std::set<Path> antiDepsOfReadPath;
            for(size_t p=0; p<parser.pagesInfo.noPages; ++p)
            {
                
                if(p)
                    parser.toBuild.outputPath.file = parser.pagesInfo.paginateName + std::to_string(p+1) + parser.outputExt;
                antiDepsOfReadPath.clear();
                parser.pagesInfo.cPageNo = p+1;
                innerPageStr = parser.pagesInfo.templateStr;
                result = parser.parse_replace('n', 
                                       innerPageStr, 
                                       "pagination template string", 
                                       parser.pagesInfo.callPath, 
                                       antiDepsOfReadPath, 
                                       parser.pagesInfo.templateLineNo, 
                                       "paginate.template", 
                                       parser.pagesInfo.templateCallLineNo, 
                                       eos);
                if(result)
                {
                    if(!parser.consoleLocked)
                        parser.os_mtx->lock();
                    start_err(eos, parser.pagesInfo.callPath, parser.pagesInfo.callLineNo) << "paginate: failed here" << std::endl;
                    parser.os_mtx->unlock();
                    break;
                }

                parser.pagesInfo.pages[p] = innerPageStr;
                estNoPagesFinished = estNoPagesFinished + 0.4;
            }

            //pagination threads
            std::vector<std::thread> threads;
            for(int i=0; i<no_paginate_threads; i++)
                threads.push_back(std::thread(pagination_thread, 
                                  parser.pagesInfo,
                                  parser.pagesInfo.paginateName,
                                  parser.outputExt,
                                  outputPathBackup));

            for(int i=0; i<no_paginate_threads; i++)
                threads[i].join();
        }

        if(result)
        {
            fail_mtx.lock();
            failedNames.insert(cInfo->name);
            fail_mtx.unlock();
        }
        else
        {
			built_mtx.lock();
			builtNames.insert(cInfo->name);
			built_mtx.unlock();
        }

		noFinished++;
    }
}

/*int ProjectInfo::build_untracked(std::ostream& os, const int& addBuildStatus, const std::set<TrackedInfo> infoToBuild)
{

}*/

int ProjectInfo::build_names(std::ostream& os, const int& addBuildStatus, const std::vector<Name>& namesToBuild)
{
    if(check_watch_dirs())
        return 1;

    std::set<Name> untrackedNames, failedNames;

    std::set<TrackedInfo> trackedInfoToBuild;
    for(auto name=namesToBuild.begin(); name != namesToBuild.end(); ++name)
    {
        if(tracking(*name))
            trackedInfoToBuild.insert(get_info(*name));
        else
            untrackedNames.insert(*name);
    }

    Parser parser(&trackedAll, 
                  &os_mtx, 
                  contentDir, 
                  outputDir, 
                  contentExt, 
                  outputExt, 
                  scriptExt, 
                  defaultTemplate, 
                  backupScripts, 
                  unixTextEditor, 
                  winTextEditor);

    //checks for pre-build scripts
    if(parser.run_script(os, Path("", "pre-build" + scriptExt), backupScripts, 1))
        return 1;

    nextInfo = trackedInfoToBuild.begin();
    counter = noFinished = estNoPagesFinished = noPagesFinished = 0;

    os_mtx.lock();
    if(addBuildStatus) // should we check if os is std::cout?
        clear_console_line();
    os << "building files.." << std::endl;
    os_mtx.unlock();

    if(addBuildStatus)
        timer.start();

    int no_threads;
    if(buildThreads < 0)
        no_threads = -buildThreads*std::thread::hardware_concurrency();
    else
        no_threads = buildThreads;

    int no_paginate_threads;
    if(paginateThreads < 0)
        no_paginate_threads = -paginateThreads*std::thread::hardware_concurrency();
    else
        no_paginate_threads = paginateThreads;

    setIncrMode(incrMode);

    std::vector<std::thread> threads;
    for(int i=0; i<no_threads; i++)
        threads.push_back(std::thread(build_thread,  
                          std::ref(std::cout),
                          no_paginate_threads, 
                          &trackedAll, 
                          trackedInfoToBuild.size(), 
                          contentDir, 
                          outputDir, 
                          contentExt, 
                          outputExt, 
                          scriptExt, 
                          defaultTemplate, 
                          backupScripts, 
                          unixTextEditor, 
                          winTextEditor));

    cPhase = BUILD_PHASE;
    std::thread thrd(build_progress, trackedInfoToBuild.size(), addBuildStatus);

	for(int i=0; i<no_threads; i++)
		threads[i].join();
    cPhase = END_PHASE;

    thrd.detach();
    //can't lock console here because it gets stalled from build_progress
    if(addBuildStatus)
        clear_console_line();

    if(failedNames.size() || untrackedNames.size())
    {
        if(noPagesFinished)
            os << "paginated files: " << noPagesFinished << std::endl;

        if(builtNames.size() == 1)
            os << builtNames.size() << " specified file built successfully" << std::endl;
        else
            os << builtNames.size() << " specified files built successfully" << std::endl;
    }

    if(failedNames.size())
    {
        size_t noToDisplay = std::max(5, -5 + (int)console_height());
        os << c_light_blue << promptStr << c_white << "failed to build:" << std::endl;
        if(failedNames.size() < noToDisplay)
        {
            for(auto fName=failedNames.begin(); fName != failedNames.end(); fName++)
                os << " " << c_red << *fName << c_white << std::endl;
        }
        else
        {
            size_t x=0;
            for(auto fName=failedNames.begin(); x < noToDisplay; fName++, x++)
                os << " " << c_red << *fName << c_white << std::endl;
            os << " and " << failedNames.size() - noToDisplay << " more" << std::endl;
        }
    }
    if(untrackedNames.size())
    {
        size_t noToDisplay = std::max(5, -5 + (int)console_height());
        os << c_light_blue << promptStr << c_white << "not tracking:" << std::endl;
        if(untrackedNames.size() < noToDisplay)
        {
            for(auto uName=untrackedNames.begin(); uName != untrackedNames.end(); uName++)
                os << " " << c_red << *uName << c_white << std::endl;
        }
        else
        {
            size_t x=0;
            for(auto uName=untrackedNames.begin(); x < noToDisplay; uName++, x++)
                os << " " << c_red << *uName << c_white << std::endl;
            os << " and " << untrackedNames.size() - noToDisplay << " more" << std::endl;
        }
    }

    if(failedNames.size() == 0 && untrackedNames.size() == 0)
    {
        #if defined __APPLE__ || defined __linux__
            const std::string pkgStr = "📦 ";
        #else  //*nix
            const std::string pkgStr = ":: ";
        #endif

        if(noPagesFinished)
            os << "paginated files: " << noPagesFinished << std::endl;

        std::cout << c_light_blue << pkgStr << c_white << "all " << namesToBuild.size() << " specified files built successfully" << std::endl;

        //checks for post-build scripts
        if(parser.run_script(os, Path("", "post-build" + scriptExt), backupScripts, 1))
            return 1;
    }

    return 0;
}

int ProjectInfo::build_all(std::ostream& os, const int& addBuildStatus)
{
    if(check_watch_dirs())
        return 1;

    if(trackedAll.size() == 0)
    {
        std::cout << "Nift does not have anything tracked" << std::endl;
        return 0;
    }

    Parser parser(&trackedAll, 
                  &os_mtx, 
                  contentDir, 
                  outputDir, 
                  contentExt, 
                  outputExt, 
                  scriptExt, 
                  defaultTemplate, 
                  backupScripts, 
                  unixTextEditor, 
                  winTextEditor);

    //checks for pre-build scripts
    if(parser.run_script(os, Path("", "pre-build" + scriptExt), backupScripts, 1))
        return 1;

    nextInfo = trackedAll.begin();
    counter = noFinished = estNoPagesFinished = noPagesFinished = 0;

    os_mtx.lock();
    if(addBuildStatus)
        clear_console_line();
    std::cout << "building project.." << std::endl;
    os_mtx.unlock();

    if(addBuildStatus)
        timer.start();

    int no_threads;
    if(buildThreads < 0)
        no_threads = -buildThreads*std::thread::hardware_concurrency();
    else
        no_threads = buildThreads;

    int no_paginate_threads;
    if(paginateThreads < 0)
        no_paginate_threads = -paginateThreads*std::thread::hardware_concurrency();
    else
        no_paginate_threads = paginateThreads;

    setIncrMode(incrMode);

    std::vector<std::thread> threads;
    for(int i=0; i<no_threads; i++)
        threads.push_back(std::thread(build_thread, 
                                      std::ref(std::cout), 
                                      no_paginate_threads,
                                      &trackedAll, 
                                      trackedAll.size(), 
                                      contentDir, 
                                      outputDir, 
                                      contentExt, 
                                      outputExt, 
                                      scriptExt, 
                                      defaultTemplate, 
                                      backupScripts, 
                                      unixTextEditor, 
                                      winTextEditor));

    cPhase = BUILD_PHASE;
    std::thread thrd(build_progress, trackedAll.size(), addBuildStatus);

	for(int i=0; i<no_threads; i++)
		threads[i].join();
    cPhase = END_PHASE;

    thrd.detach();
    //can't lock console here because it gets stalled from build_progress
    if(addBuildStatus)
        clear_console_line();

    if(failedNames.size() > 0)
    {
        if(noPagesFinished)
            os << "paginated files: " << noPagesFinished << std::endl;

        if(builtNames.size() == 1)
            os << builtNames.size() << " file built successfully" << std::endl;
        else
            os << builtNames.size() << " files built successfully" << std::endl;

        size_t noToDisplay = std::max(5, -5 + (int)console_height());
        os << c_light_blue << promptStr << c_white << "failed to build:" << std::endl;
        if(failedNames.size() < noToDisplay)
        {
            for(auto fName=failedNames.begin(); fName != failedNames.end(); fName++)
                os << " " << c_red << *fName << c_white << std::endl;
        }
        else
        {
            size_t x=0;
            for(auto fName=failedNames.begin(); x < noToDisplay; fName++, x++)
                os << " " << c_red << *fName << c_white << std::endl;
            os << " and " << failedNames.size() - noToDisplay << " more" << std::endl;
        }
    }
    else
    {
        #if defined __APPLE__ || defined __linux__
            const std::string pkgStr = "📦 ";
        #else  //*nix
            const std::string pkgStr = ":: ";
        #endif

        if(noPagesFinished)
            os << "paginated files: " << noPagesFinished << std::endl;
        os << c_light_blue << pkgStr << c_white << "all " << noFinished << " tracked files built successfully" << std::endl;

        //checks for post-build scripts
        if(parser.run_script(os, Path("", "post-build" + scriptExt), backupScripts, 1))
            return 1;
    }

    return 0;
}

std::mutex problem_mtx, 
           updated_mtx, 
           mod_hash_mtx, 
           modified_mtx, 
           removed_mtx;
std::set<Name> problemNames;
std::set<TrackedInfo> updatedInfo;
std::set<Path> modifiedFiles,
               removedFiles;

void dep_thread(std::ostream& os,
                const bool& addExpl,
                const int& incrMode,
                const int& no_to_check,
                const Directory& contentDir,
                const Directory& outputDir,
                const std::string& contentExt,
                const std::string& outputExt)
{
    std::set<TrackedInfo>::iterator cInfo;

    while(counter < no_to_check)
    {
        set_mtx.lock();
        if(counter >= no_to_check)
        {
            set_mtx.unlock();
            return;
        }
        counter++;
        cInfo = nextInfo++;
        set_mtx.unlock();

        //checks whether content and template files exist
        if(!file_exists(cInfo->contentPath.str()))
        {
            if(addExpl)
            {
                os_mtx.lock();
                os << cInfo->name << ": content file " << cInfo->contentPath << " does not exist" << std::endl;
                os_mtx.unlock();
            }
            problem_mtx.lock();
            problemNames.insert(cInfo->name);
            problem_mtx.unlock();
            continue;
        }
        if(!file_exists(cInfo->templatePath.str()))
        {
            if(addExpl)
            {
                os_mtx.lock();
                os << cInfo->name << ": template file " << cInfo->templatePath << " does not exist" << std::endl;
                os_mtx.unlock();
            }
            problem_mtx.lock();
            problemNames.insert(cInfo->name);
            problem_mtx.unlock();
            continue;
        }

        //gets path of info file from last time output file was built
        Path infoPath = cInfo->outputPath.getInfoPath();

        //checks whether info path exists
        if(!file_exists(infoPath.str()))
        {
            if(addExpl)
            {
                os_mtx.lock();
                os << cInfo->outputPath << ": yet to be built" << std::endl;
                os_mtx.unlock();
            }
            updated_mtx.lock();
            updatedInfo.insert(*cInfo);
            updated_mtx.unlock();
            continue;
        }
        else
        {
            std::ifstream infoStream(infoPath.str());
            std::string timeDateLine;
            Name prevName;
            Title prevTitle;
            Path prevTemplatePath;

            getline(infoStream, timeDateLine);
            read_quoted(infoStream, prevName);
            prevTitle.read_quoted_from(infoStream);
            prevTemplatePath.read_file_path_from(infoStream);

            TrackedInfo prevInfo = make_info(prevName, prevTitle, prevTemplatePath, contentDir, outputDir, contentExt, outputExt);

            //note we haven't checked for non-default content/output extension, pretty sure we don't need to here

            if(cInfo->name != prevInfo.name)
            {
                if(addExpl)
                {
                    os_mtx.lock();
                    os << cInfo->outputPath << ": name changed to " << cInfo->name << " from " << prevInfo.name << std::endl;
                    os_mtx.unlock();
                }
                updated_mtx.lock();
                updatedInfo.insert(*cInfo);
                updated_mtx.unlock();
                continue;
            }

            if(cInfo->title != prevInfo.title)
            {
                if(addExpl)
                {
                    os_mtx.lock();
                    os << cInfo->outputPath << ": title changed to " << cInfo->title << " from " << prevInfo.title << std::endl;
                    os_mtx.unlock();
                }
                updated_mtx.lock();
                updatedInfo.insert(*cInfo);
                updated_mtx.unlock();
                continue;
            }

            if(cInfo->templatePath != prevInfo.templatePath)
            {
                if(addExpl)
                {
                    os_mtx.lock();
                    os << cInfo->outputPath << ": template path changed to " << cInfo->templatePath << " from " << prevInfo.templatePath << std::endl;
                    os_mtx.unlock();
                }
                updated_mtx.lock();
                updatedInfo.insert(*cInfo);
                updated_mtx.unlock();
                continue;
            }

            Path dep;
            while(dep.read_file_path_from(infoStream))
            {
                if(!file_exists(dep.str()))
                {
                    if(addExpl)
                    {
                        os_mtx.lock();
                        os << cInfo->outputPath << ": dep path " << dep << " removed since last build" << std::endl;
                        os_mtx.unlock();
                    }
                    removed_mtx.lock();
                    removedFiles.insert(dep);
                    removed_mtx.unlock();
                    updated_mtx.lock();
                    updatedInfo.insert(*cInfo);
                    updated_mtx.unlock();
                    break;
                }
                else if(incrMode != INCR_HASH && dep.modified_after(infoPath))
                {
                    if(addExpl)
                    {
                        os_mtx.lock();
                        os << cInfo->outputPath << ": dep path " << dep << " modified since last build" << std::endl;
                        os_mtx.unlock();
                    }
                    modified_mtx.lock();
                    modifiedFiles.insert(dep);
                    modified_mtx.unlock();
                    updated_mtx.lock();
                    updatedInfo.insert(*cInfo);
                    updated_mtx.unlock();
                    break;
                }
                else if(incrMode != INCR_MOD)
                {
                    //gets path of info file from last time output file was built
                    Path hashPath = dep.getHashPath();
                    std::string hashPathStr = hashPath.str();

                    if(!file_exists(hashPathStr))
                    {
                        if(addExpl)
                        {
                            os_mtx.lock();
                            os << cInfo->outputPath << ": " << "hash file " << hashPath << " does not exist" << std::endl;
                            os_mtx.unlock();
                        }
                        updated_mtx.lock();
                        updatedInfo.insert(*cInfo);
                        updated_mtx.unlock();
                        break;
                    }
                    else if((unsigned) std::atoi(string_from_file(hashPathStr).c_str()) != FNVHash(string_from_file(dep.str())))
                    {
                        if(addExpl)
                        {
                            os_mtx.lock();
                            os << cInfo->outputPath << ": dep path " << dep << " modified since last build" << std::endl;
                            os_mtx.unlock();
                        }
                        modified_mtx.lock();
                        modifiedFiles.insert(dep);
                        modified_mtx.unlock();
                        updated_mtx.lock();
                        updatedInfo.insert(*cInfo);
                        updated_mtx.unlock();
                        break;
                    }
                }
            }

			infoStream.close();

            //checks for user-defined dependencies
            Path depsPath = cInfo->contentPath;
            depsPath.file = depsPath.file.substr(0, depsPath.file.find_first_of('.')) + ".deps";

            if(file_exists(depsPath.str()))
            {
                std::ifstream depsFile(depsPath.str());
                while(dep.read_file_path_from(depsFile))
                {
                    if(!path_exists(dep.str()))
                    {
                        if(addExpl)
                        {
                            os_mtx.lock();
                            os << cInfo->outputPath << ": user defined dep path " << dep << " does not exist" << std::endl;
                            os_mtx.unlock();
                        }
                        removed_mtx.lock();
                        removedFiles.insert(dep);
                        removed_mtx.unlock();
                        updated_mtx.lock();
                        updatedInfo.insert(*cInfo);
                        updated_mtx.unlock();
                        break;
                    }
                    else if(dep.modified_after(infoPath))
                    {
                        if(addExpl)
                        {
                            os_mtx.lock();
                            os << cInfo->outputPath << ": user defined dep path " << dep << " modified since last build" << std::endl;
                            os_mtx.unlock();
                        }
                        modified_mtx.lock();
                        modifiedFiles.insert(dep);
                        modified_mtx.unlock();
                        updated_mtx.lock();
                        updatedInfo.insert(*cInfo);
                        updated_mtx.unlock();
                        break;
                    }
                }
            }
        }

        noFinished++;
    }
}

int ProjectInfo::build_updated(std::ostream& os, const int& addBuildStatus, const bool& addExpl, const bool& basicOpt)
{
    if(check_watch_dirs())
        return 1;

    if(trackedAll.size() == 0)
    {
        os_mtx.lock();
        os << "Nift does not have anything tracked" << std::endl;
        os_mtx.unlock();
        return 0;
    }

    builtNames.clear();
    failedNames.clear();
    problemNames.clear();
    updatedInfo.clear();
    modifiedFiles.clear();
    removedFiles.clear();

    int no_threads;
    if(buildThreads < 0)
        no_threads = -buildThreads*std::thread::hardware_concurrency();
    else
        no_threads = buildThreads;

    int no_paginate_threads;
    if(paginateThreads < 0)
        no_paginate_threads = -paginateThreads*std::thread::hardware_concurrency();
    else
        no_paginate_threads = paginateThreads;

    nextInfo = trackedAll.begin();
    counter = noFinished = estNoPagesFinished = noPagesFinished = 0;

    if(addBuildStatus)
    {
        os_mtx.lock();
        os << "checking for updates.." << std::endl;
        os_mtx.unlock();

        timer.start();
    }

    std::vector<std::thread> threads;
	for(int i=0; i<no_threads; i++)
		threads.push_back(std::thread(dep_thread, 
                                      std::ref(os), 
                                      addExpl, 
                                      incrMode,
                                      trackedAll.size(), 
                                      contentDir, 
                                      outputDir, 
                                      contentExt, 
                                      outputExt));

    cPhase = UPDATE_PHASE;
    std::thread thrd(build_progress, trackedAll.size(), addBuildStatus);

	for(int i=0; i<no_threads; i++)
		threads[i].join();
    cPhase = DUMMY_PHASE;

    thrd.detach();
    if(addBuildStatus)
        clear_console_line();

    size_t noToDisplay = std::max(5, -5 + (int)console_height());

    if(!basicOpt)
    {
        if(removedFiles.size() > 0)
        {
            os << c_light_blue << promptStr << c_white << "removed dependency files:" << std::endl;
            if(removedFiles.size() < noToDisplay)
            {
                for(auto rFile=removedFiles.begin(); rFile != removedFiles.end(); rFile++)
                    os << " " << *rFile << std::endl;
            }
            else
            {
                size_t x=0;
                for(auto rFile=removedFiles.begin(); x < noToDisplay; rFile++, x++)
                    os << " " << *rFile << std::endl;
                os << " and " << removedFiles.size() - noToDisplay << " more" << std::endl;
            }
        }

        if(modifiedFiles.size() > 0)
        {
            os << c_light_blue << promptStr << c_white << "modified dependency files:" << std::endl;
            if(modifiedFiles.size() < noToDisplay)
            {
                for(auto uFile=modifiedFiles.begin(); uFile != modifiedFiles.end(); uFile++)
                    os << " " << *uFile << std::endl;
            }
            else
            {
                size_t x=0;
                for(auto uFile=modifiedFiles.begin(); x < noToDisplay; uFile++, x++)
                    os << " " << *uFile << std::endl;
                os << " and " << modifiedFiles.size() - noToDisplay << " more" << std::endl;
            }
        }

        if(problemNames.size() > 0)
        {
            os << c_light_blue << promptStr << c_white << "missing content/template file:" << std::endl;
            if(problemNames.size() < noToDisplay)
            {
                for(auto pName=problemNames.begin(); pName != problemNames.end(); pName++)
                    os << " " << c_red << *pName << c_white << std::endl;
            }
            else
            {
                size_t x=0;
                for(auto pName=problemNames.begin(); x < noToDisplay; pName++, x++)
                    os << " " << c_red << *pName << c_white << std::endl;
                os << " and " << problemNames.size() - noToDisplay << " more" << std::endl;
            }
        }
    }

    if(updatedInfo.size() > 0)
    {
        if(!basicOpt)
        {
            os << c_light_blue << promptStr << c_white << "need building:" << std::endl;
            if(updatedInfo.size() < noToDisplay)
            {
                for(auto uInfo=updatedInfo.begin(); uInfo != updatedInfo.end(); uInfo++)
                    os << " " << uInfo->outputPath << std::endl;
            }
            else
            {
                size_t x=0;
                for(auto uInfo=updatedInfo.begin(); x < noToDisplay; uInfo++, x++)
                    os << " " << uInfo->outputPath << std::endl;
                os << " and " << updatedInfo.size() - noToDisplay << " more" << std::endl;
            }
        }

        Parser parser(&trackedAll, 
                  &os_mtx, 
                  contentDir, 
                  outputDir, 
                  contentExt, 
                  outputExt, 
                  scriptExt, 
                  defaultTemplate, 
                  backupScripts, 
                  unixTextEditor, 
                  winTextEditor);

        //checks for pre-build scripts
        if(parser.run_script(os, Path("", "pre-build" + scriptExt), backupScripts, 1))
            return 1;

        setIncrMode(incrMode);

        nextInfo = updatedInfo.begin();
        counter = noFinished = 0;

        //os_mtx.lock();
        if(addBuildStatus)
            clear_console_line();
        os << "building updated files.." << std::endl;
        //os_mtx.unlock();

        if(addBuildStatus)
            timer.start();

        threads.clear();
        for(int i=0; i<no_threads; i++)
            threads.push_back(std::thread(build_thread, 
                                          std::ref(os), 
                                          no_paginate_threads, 
                                          &trackedAll, 
                                          updatedInfo.size(), 
                                          contentDir, 
                                          outputDir, 
                                          contentExt, 
                                          outputExt, 
                                          scriptExt, 
                                          defaultTemplate, 
                                          backupScripts, 
                                          unixTextEditor, 
                                          winTextEditor));

        cPhase = BUILD_PHASE;
        std::thread thrd(build_progress, updatedInfo.size(), addBuildStatus);

        for(int i=0; i<no_threads; i++)
            threads[i].join();
        cPhase = END_PHASE;

        thrd.detach();
        if(addBuildStatus)
            clear_console_line();

        if(failedNames.size() > 0)
        {
            if(noPagesFinished)
                os << "paginated files: " << noPagesFinished << std::endl;

            if(builtNames.size() == 1)
                os << builtNames.size() << " file built successfully" << std::endl;
            else
                os << builtNames.size() << " files built successfully" << std::endl;

            os << c_light_blue << promptStr << c_white << "failed to build:" << std::endl;
            if(failedNames.size() < noToDisplay)
            {
                for(auto fName=failedNames.begin(); fName != failedNames.end(); fName++)
                    os << " " << c_red << *fName << c_white << std::endl;
            }
            else
            {
                size_t x=0;
                for(auto fName=failedNames.begin(); x < noToDisplay; fName++, x++)
                    os << " " << c_red << *fName << c_white << std::endl;
                os << " and " << failedNames.size() - noToDisplay << " more" << std::endl;
            }
        }
        else
        {
            if(basicOpt)
            {
                #if defined __APPLE__ || defined __linux__
                    const std::string pkgStr = "📦 ";
                #else  //*nix
                    const std::string pkgStr = ":: ";
                #endif

                if(noPagesFinished)
                    os << "paginated files: " << noPagesFinished << std::endl;

                if(builtNames.size() == 1)
                    std::cout << c_light_blue << pkgStr << c_white << builtNames.size() << " updated file built successfully" << std::endl;
                else
                    std::cout << c_light_blue << pkgStr << c_white << builtNames.size() << " updated files built successfully" << std::endl;
            }
            else if(builtNames.size() > 0)
            {
                #if defined __APPLE__ || defined __linux__
                    const std::string pkgStr = "📦 ";
                #else  // FreeBSD/Windows
                    const std::string pkgStr = ":: ";
                #endif

                if(noPagesFinished)
                    os << "paginated files: " << noPagesFinished << std::endl;

                os << c_light_blue << pkgStr << c_white << "successfully built:" << std::endl;
                if(builtNames.size() < noToDisplay)
                {
                    for(auto bName=builtNames.begin(); bName != builtNames.end(); bName++)
                        os << " " << c_green << *bName << c_white << std::endl;
                }
                else
                {
                    size_t x=0;
                    for(auto bName=builtNames.begin(); x < noToDisplay; bName++, x++)
                        os << " " << c_green << *bName << c_white << std::endl;
                    os << " and " << builtNames.size() - noToDisplay << " more" << std::endl;
                }
            }

            //checks for post-build scripts
            if(parser.run_script(os, Path("", "post-build" + scriptExt), backupScripts, 1))
                return 1;
        }
    }

    if(updatedInfo.size() == 0 && problemNames.size() == 0 && failedNames.size() == 0)
        os << "all " << trackedAll.size() << " tracked files are already up to date" << std::endl;

    builtNames.clear();
	failedNames.clear();

    return 0;
}

int ProjectInfo::status(std::ostream& os, const int& addBuildStatus, const bool& addExpl, const bool& basicOpt)
{
    if(check_watch_dirs())
        return 1;

    if(trackedAll.size() == 0)
    {
        std::cout << "Nift does not have anything tracked" << std::endl;
        return 0;
    }

    builtNames.clear();
    failedNames.clear();
    problemNames.clear();
    updatedInfo.clear();
    modifiedFiles.clear();
    removedFiles.clear();

    int no_threads;
    if(buildThreads < 0)
        no_threads = -buildThreads*std::thread::hardware_concurrency();
    else
        no_threads = buildThreads;

    nextInfo = trackedAll.begin();
    counter = noFinished = 0;

    os_mtx.lock();
    std::cout << "checking for updates.." << std::endl;
    os_mtx.unlock();

    if(addBuildStatus)
        timer.start();

    std::vector<std::thread> threads;
	for(int i=0; i<no_threads; i++)
		threads.push_back(std::thread(dep_thread, 
                                      std::ref(os), 
                                      addExpl, 
                                      incrMode,
                                      trackedAll.size(), 
                                      contentDir, 
                                      outputDir, 
                                      contentExt, 
                                      outputExt));

    cPhase = UPDATE_PHASE;
    std::thread thrd(build_progress, trackedAll.size(), addBuildStatus);

	for(int i=0; i<no_threads; i++)
		threads[i].join();
    cPhase = END_PHASE;

    thrd.detach();
    if(addBuildStatus)
        clear_console_line();

    size_t noToDisplay = std::max(5, -5 + (int)console_height());

    if(!basicOpt)
    {
        if(removedFiles.size() > 0)
        {
            os << c_light_blue << promptStr << c_white << "removed dependency files:" << std::endl;
            if(removedFiles.size() < noToDisplay)
            {
                for(auto rFile=removedFiles.begin(); rFile != removedFiles.end(); rFile++)
                    os << " " << *rFile << std::endl;
            }
            else
            {
                size_t x=0;
                for(auto rFile=removedFiles.begin(); x < noToDisplay; rFile++, x++)
                    os << " " << *rFile << std::endl;
                os << " and " << removedFiles.size() - noToDisplay << " more" << std::endl;
            }
        }

        if(modifiedFiles.size() > 0)
        {
            os << c_light_blue << promptStr << c_white << "modified dependency files:" << std::endl;
            if(modifiedFiles.size() < noToDisplay)
            {
                for(auto uFile=modifiedFiles.begin(); uFile != modifiedFiles.end(); uFile++)
                    os << " " << *uFile << std::endl;
            }
            else
            {
                size_t x=0;
                for(auto uFile=modifiedFiles.begin(); x < noToDisplay; uFile++, x++)
                    os << " " << *uFile << std::endl;
                os << " and " << modifiedFiles.size() - noToDisplay << " more" << std::endl;
            }
        }

        if(problemNames.size() > 0)
        {
            os << c_light_blue << promptStr << c_white << "missing content/template file:" << std::endl;
            if(problemNames.size() < noToDisplay)
            {
                for(auto pName=problemNames.begin(); pName != problemNames.end(); pName++)
                    os << " " << c_red << *pName << c_white << std::endl;
            }
            else
            {
                size_t x=0;
                for(auto pName=problemNames.begin(); x < noToDisplay; pName++, x++)
                    os << " " << c_red << *pName << c_white << std::endl;
                os << " and " << problemNames.size() - noToDisplay << " more" << std::endl;
            }
        }
    }

    if(updatedInfo.size() > 0)
    {
        if(!basicOpt)
        {
            os << c_light_blue << promptStr << c_white << "need building:" << std::endl;
            if(updatedInfo.size() < noToDisplay)
            {
                for(auto uInfo=updatedInfo.begin(); uInfo != updatedInfo.end(); uInfo++)
                    os << " " << uInfo->outputPath << std::endl;
            }
            else
            {
                size_t x=0;
                for(auto uInfo=updatedInfo.begin(); x < noToDisplay; uInfo++, x++)
                    os << " " << uInfo->outputPath << std::endl;
                os << " and " << updatedInfo.size() - noToDisplay << " more" << std::endl;
            }
        }
    }

    if(updatedInfo.size() == 0 && problemNames.size() == 0 && failedNames.size() == 0)
        os << "all " << trackedAll.size() << " tracked files are already up to date" << std::endl;

    failedNames.clear();

    return 0;
}

