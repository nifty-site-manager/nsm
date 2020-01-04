#include "ProjectInfo.h"

int ProjectInfo::open()
{
    if(open_config())
        return 1;
    if(open_tracking())
        return 1;

    return 0;
}

int ProjectInfo::open_config()
{
    if(!std::ifstream(".nsm/nift.config"))
    {
        //this should never happen!
        std::cout << "error: ProjectInfo.cpp: open_config(): could not open Nift config file as '" << get_pwd() <<  "/.nsm/nift.config' does not exist" << std::endl;
        return 1;
    }

    contentDir = outputDir = "";
    backupScripts = 1;
    buildThreads = 0;
    contentExt = outputExt = scriptExt = unixTextEditor = winTextEditor = rootBranch = outputBranch = "";
    defaultTemplate = Path("", "");

    //reads Nift config file
    std::ifstream ifs(".nsm/nift.config");
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
                //std::cout << "warning: .nsm/nift.config: updated siteDir to outputDir" << std::endl;
            }
            else if(inType == "outputExt")
                read_quoted(iss, outputExt);
            else if(inType == "pageExt") //can delete this later
            {
                read_quoted(iss, outputExt);
                configChanged = 1;
                //std::cout << "warning: .nsm/nift.config: updated pageExt to outputExt" << std::endl;
            }
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
            else if(inType == "outputBranch")
                read_quoted(iss, outputBranch);
            else if(inType == "siteBranch") // can delete this later
            {
                read_quoted(iss, outputBranch);
                configChanged = 1;
                //std::cout << "warning: .nsm/nift.config: updated siteBranch to outputBranch" << std::endl;
            }
            else
            {
                continue;
                //if we throw error here can't compile projects for newer versions with older versions of Nift
                //std::cout << "error: .nsm/nift.config: line " << lineNo << ": do not recognise confirguration parameter " << inType << std::endl;
                //return 1;
            }

            iss >> str;
            if(str != "" && str[0] != '#')
            {
                std::cout << "error: .nsm/nift.config: line " << lineNo << ": was not expecting anything on this line from '" << unquote(str) << "' onwards" << std::endl;
                std::cout << "note: you can comment out the remainder of a line with #" << std::endl;
                return 1;
            }
        }
    }
    ifs.close();

    if(contentExt == "" || contentExt[0] != '.')
    {
        std::cout << "error: .nsm/nift.config: content extension must start with a fullstop" << std::endl;
        return 1;
    }

    if(outputExt == "" || outputExt[0] != '.')
    {
        std::cout << "error: .nsm/nift.config: output extension must start with a fullstop" << std::endl;
        return 1;
    }

    if(scriptExt != "" && scriptExt [0] != '.')
    {
        std::cout << "error: .nsm/nift.config: script extension must start with a fullstop" << std::endl;
        return 1;
    }

    if(defaultTemplate == Path("", ""))
    {
        std::cout << "error: .nsm/nift.config: default template is not specified" << std::endl;
        return 1;
    }

    if(contentDir == "")
    {
        std::cout << "error: .nsm/nift.config: content directory not specified" << std::endl;
        return 1;
    }

    if(outputDir == "")
    {
        std::cout << "error: .nsm/nift.config: output directory not specified" << std::endl;
        return 1;
    }

    if(scriptExt == "")
    {
        std::cout << "warning: .nsm/nift.config: script extension not detected, set to default of '.py'" << std::endl;

        scriptExt = ".py";

        configChanged = 1;
    }

    if(buildThreads == 0)
    {
        std::cout << "warning: .nsm/nift.config: number of build threads not detected or invalid, set to default of -1 (number of cores)" << std::endl;

        buildThreads = -1;

        configChanged = 1;
    }

    //code to set unixTextEditor if not present
    if(unixTextEditor == "")
    {
        std::cout << "warning: .nsm/nift.config: unix text editor not detected, set to default of 'nano'" << std::endl;

        unixTextEditor = "nano";

        configChanged = 1;
    }

    //code to set winTextEditor if not present
    if(winTextEditor == "")
    {
        std::cout << "warning: .nsm/nift.config: windows text editor not detected, set to default of 'notepad'" << std::endl;

        winTextEditor = "notepad";

        configChanged = 1;
    }

    //code to figure out rootBranch and outputBranch if not present
    if(rootBranch == "" || outputBranch == "")
    {
        std::cout << "warning: .nsm/nift.config: root or output branch not detected, have attempted to determine them, ensure values in config file are correct" << std::endl;

        if(std::ifstream(".git"))
        {
            std::set<std::string> branches = get_git_branches();

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

    if(configChanged)
        save_config();

    return 0;
}

int ProjectInfo::open_tracking()
{
    trackedAll.clear();

    if(!std::ifstream(".nsm/tracking.list"))
    {
        //this should never happen!
        std::cout << "error: ProjectInfo.cpp: open(): could not open tracking information as '" << get_pwd() << "/.nsm/tracking.list' does not exist" << std::endl;
        return 1;
    }

    //reads tracking list file
    std::ifstream ifs(".nsm/tracking.list");
    Name inName;
    Title inTitle;
    Path inTemplatePath;
    std::ifstream ifsx;
    std::string inExt;
    while(read_quoted(ifs, inName))
    {
        inTitle.read_quoted_from(ifs);
        inTemplatePath.read_file_path_from(ifs);

        TrackedInfo inInfo = make_info(inName, inTitle, inTemplatePath);

        //checks for non-default content extension
        Path extPath = inInfo.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";

        if(std::ifstream(extPath.str()))
        {
            ifsx.open(extPath.str());

            ifsx >> inExt;
            inInfo.contentPath.file = inInfo.contentPath.file.substr(0, inInfo.contentPath.file.find_first_of('.')) + inExt;

            ifsx.close();
        }

        //checks for non-default output extension
        extPath = inInfo.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";

        if(std::ifstream(extPath.str()))
        {
            ifsx.open(extPath.str());

            ifsx >> inExt;
            inInfo.outputPath.file = inInfo.outputPath.file.substr(0, inInfo.outputPath.file.find_first_of('.')) + inExt;

            ifsx.close();
        }

        //checks that content and template files aren't the same
        if(inInfo.contentPath == inInfo.templatePath)
        {
            std::cout << "error: failed to open .nsm/tracking.list" << std::endl;
            std::cout << "reason: " << inInfo.name << " has same content and template path" << inInfo.templatePath << std::endl;

            return 1;
        }

        //makes sure there's no duplicate entries in tracking.list
        if(trackedAll.count(inInfo) > 0)
        {
            TrackedInfo cInfo = *(trackedAll.find(inInfo));

            std::cout << "error: failed to open .nsm/tracking.list" << std::endl;
            std::cout << "reason: duplicate entry for " << inInfo.name << std::endl;
            std::cout << std::endl;
            std::cout << "----- first entry -----" << std::endl;
            std::cout << "        title: " << cInfo.title << std::endl;
            std::cout << "  output path: " << cInfo.outputPath << std::endl;
            std::cout << " content path: " << cInfo.contentPath << std::endl;
            std::cout << "template path: " << cInfo.templatePath << std::endl;
            std::cout << "--------------------------------" << std::endl;
            std::cout << std::endl;
            std::cout << "----- second entry -----" << std::endl;
            std::cout << "        title: " << inInfo.title << std::endl;
            std::cout << "  output path: " << inInfo.outputPath << std::endl;
            std::cout << " content path: " << inInfo.contentPath << std::endl;
            std::cout << "template path: " << inInfo.templatePath << std::endl;
            std::cout << "--------------------------------" << std::endl;

            return 1;
        }

        trackedAll.insert(inInfo);
    }

    ifs.close();

    return 0;
}

int ProjectInfo::save_config()
{
    std::ofstream ofs(".nsm/nift.config");
    ofs << "contentDir " << quote(contentDir) << "\n";
    ofs << "contentExt " << quote(contentExt) << "\n";
    ofs << "outputDir " << quote(outputDir) << "\n";
    ofs << "outputExt " << quote(outputExt) << "\n";
    ofs << "scriptExt " << quote(scriptExt) << "\n";
    ofs << "defaultTemplate " << defaultTemplate << "\n\n";
    ofs << "backupScripts " << backupScripts << "\n\n";
    ofs << "buildThreads " << buildThreads << "\n\n";
    ofs << "unixTextEditor " << quote(unixTextEditor) << "\n";
    ofs << "winTextEditor " << quote(winTextEditor) << "\n\n";
    ofs << "rootBranch " << quote(rootBranch) << "\n";
    ofs << "outputBranch " << quote(outputBranch) << "\n";
    ofs.close();

    return 0;
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
    if(std::ifstream(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, inExt);
        ifs.close();

        trackedInfo.contentPath.file = trackedInfo.contentPath.file.substr(0, trackedInfo.contentPath.file.find_first_of('.')) + inExt;
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
    if(std::ifstream(extPath.str()))
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
    if(std::ifstream(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, inExt);
        ifs.close();

        trackedInfo.contentPath.file = trackedInfo.contentPath.file.substr(0, trackedInfo.contentPath.file.find_first_of('.')) + inExt;
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
    if(std::ifstream(extPath.str()))
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
    if(std::ifstream(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, inExt);
        ifs.close();

        trackedInfo.contentPath.file = trackedInfo.contentPath.file.substr(0, trackedInfo.contentPath.file.find_first_of('.')) + inExt;
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
    if(std::ifstream(extPath.str()))
    {
        std::ifstream ifs(extPath.str());
        getline(ifs, inExt);
        ifs.close();

        trackedInfo.outputPath.file = trackedInfo.outputPath.file.substr(0, trackedInfo.outputPath.file.find_first_of('.')) + inExt;
    }

    return trackedInfo;
}

TrackedInfo make_info(const Name& name, const Title& title, const Path& templatePath, const Directory& contentDir, const Directory& outputDir, const std::string& contentExt, const std::string& outputExt)
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
    std::cout << std::endl;
    std::cout << "------ information on specified names ------" << std::endl;
    for(auto name=names.begin(); name!=names.end(); name++)
    {
        if(name != names.begin())
            std::cout << std::endl;

        TrackedInfo cInfo;
        cInfo.name = *name;
        if(trackedAll.count(cInfo))
        {
            cInfo = *(trackedAll.find(cInfo));
            std::cout << "         name: " << quote(cInfo.name) << std::endl;
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
    std::cout << "--------------------------------------------" << std::endl;

    return 0;
}

int ProjectInfo::info_all()
{
    if(std::ifstream(".nsm/.watchinfo/watching.list"))
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
                std::cout << " output extension: " << wd->outputExts.at(*ext) << std::endl;
            }
        }
        std::cout << "-------------------------------------" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "--------- all tracked names ---------" << std::endl;
    for(auto trackedInfo=trackedAll.begin(); trackedInfo!=trackedAll.end(); trackedInfo++)
    {
        if(trackedInfo != trackedAll.begin())
            std::cout << std::endl;
        std::cout << "         name: " << quote(trackedInfo->name) << std::endl;
        std::cout << "        title: " << trackedInfo->title << std::endl;
        std::cout << "  output path: " << trackedInfo->outputPath << std::endl;
        std::cout << "   output ext: " << quote(get_output_ext(*trackedInfo)) << std::endl;
        std::cout << " content path: " << trackedInfo->contentPath << std::endl;
        std::cout << "  content ext: " << quote(get_cont_ext(*trackedInfo)) << std::endl;
        std::cout << "   script ext: " << quote(get_script_ext(*trackedInfo)) << std::endl;
        std::cout << "template path: " << trackedInfo->templatePath << std::endl;
    }
    std::cout << "------------------------------------" << std::endl;

    return 0;
}

int ProjectInfo::info_names()
{
    std::cout << std::endl;
    std::cout << "--------- all tracked names ---------" << std::endl;
    for(auto trackedInfo=trackedAll.begin(); trackedInfo!=trackedAll.end(); trackedInfo++)
        std::cout << trackedInfo->name << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    return 0;
}

int ProjectInfo::info_tracking()
{
    std::cout << "--------- all tracked names ---------" << std::endl;
    for(auto trackedInfo=trackedAll.begin(); trackedInfo!=trackedAll.end(); trackedInfo++)
    {
        if(trackedInfo != trackedAll.begin())
            std::cout << std::endl;
        std::cout << "         name: " << quote(trackedInfo->name) << std::endl;
        std::cout << "        title: " << trackedInfo->title << std::endl;
        std::cout << "  output path: " << trackedInfo->outputPath << std::endl;
        std::cout << "   output ext: " << quote(get_output_ext(*trackedInfo)) << std::endl;
        std::cout << " content path: " << trackedInfo->contentPath << std::endl;
        std::cout << "  content ext: " << quote(get_cont_ext(*trackedInfo)) << std::endl;
        std::cout << "   script ext: " << quote(get_script_ext(*trackedInfo)) << std::endl;
        std::cout << "template path: " << trackedInfo->templatePath << std::endl;
    }
    std::cout << "------------------------------------" << std::endl;

    return 0;
}

int ProjectInfo::info_watching()
{
    if(std::ifstream(".nsm/.watchinfo/watching.list"))
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
                std::cout << " output extension: " << wd->outputExts.at(*ext) << std::endl;
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

std::string ProjectInfo::get_ext(const TrackedInfo& trackedInfo, const std::string& extType)
{
    std::string ext;

    //checks for non-default extension
    Path extPath = trackedInfo.outputPath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + extType;

    if(std::ifstream(extPath.str()))
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
        std::cout << "error: names cannot contain '.'" << std::endl;
        std::cout << "note: you can add post-build scripts to move built/output files to paths containing . other than for extensions if you want" << std::endl;
        return 1;
    }
    else if(name == "" || title.str == "" || templatePath == Path("", ""))
    {
        std::cout << "error: name, title and template path must all be non-empty strings" << std::endl;
        return 1;
    }
    if(unquote(name)[unquote(name).size()-1] == '/' || unquote(name)[unquote(name).size()-1] == '\\')
    {
        std::cout << "error: name " << quote(name) << " cannot end in '/' or '\\'" << std::endl;
        return 1;
    }
    else if(
                (unquote(name).find('"') != std::string::npos && unquote(name).find('\'') != std::string::npos) ||
                (unquote(title.str).find('"') != std::string::npos && unquote(title.str).find('\'') != std::string::npos) ||
                (unquote(templatePath.str()).find('"') != std::string::npos && unquote(templatePath.str()).find('\'') != std::string::npos)
            )
    {
        std::cout << "error: name, title and template path cannot contain both single and double quotes" << std::endl;
        return 1;
    }

    TrackedInfo newInfo = make_info(name, title, templatePath);

    if(newInfo.contentPath == newInfo.templatePath)
    {
        std::cout << "error: content and template paths cannot be the same, not tracked" << std::endl;
        return 1;
    }

    if(trackedAll.count(newInfo) > 0)
    {
        TrackedInfo cInfo = *(trackedAll.find(newInfo));

        std::cout << "error: Nift is already tracking " << newInfo.outputPath << std::endl;
        std::cout << "----- current tracked details -----" << std::endl;
        std::cout << "        title: " << cInfo.title << std::endl;
        std::cout << "  output path: " << cInfo.outputPath << std::endl;
        std::cout << " content path: " << cInfo.contentPath << std::endl;
        std::cout << "template path: " << cInfo.templatePath << std::endl;
        std::cout << "--------------------------------" << std::endl;

        return 1;
    }

    //throws error if template path doesn't exist
    if(!std::ifstream(newInfo.templatePath.str()))
    {
        std::cout << "error: template path " << newInfo.templatePath << " does not exist" << std::endl;
        return 1;
    }

    if(std::ifstream(newInfo.outputPath.str()))
    {
        std::cout << "error: new output path " << newInfo.outputPath << " already exists" << std::endl;
        return 1;
    }

    if(!std::ifstream(newInfo.contentPath.str()))
    {
        newInfo.contentPath.ensureFileExists();
        //chmod(newInfo.contentPath.str().c_str(), 0666);
    }

    //ensures directories for output file and info file exist
    newInfo.outputPath.ensureDirExists();
    newInfo.outputPath.getInfoPath().ensureDirExists();

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
    if(!std::ifstream(filePath))
    {
        std::cout << "error: track-from-file: track-file " << quote(filePath) << " does not exist" << std::endl;
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
            std::cout << "error: name " << quote(name) << " cannot contain '.'" << std::endl;
            std::cout << "note: you can add post-build scripts to move built built/output files to paths containing . other than for extensions if you want" << std::endl;
            return 1;
        }
        else if(name == "" || title.str == "" || templatePath == Path("", "") || cExt == "" || oExt == "")
        {
            std::cout << "error: names, titles, template paths, content extensions and output extensions must all be non-empty strings" << std::endl;
            return 1;
        }
        if(unquote(name)[unquote(name).size()-1] == '/' || unquote(name)[unquote(name).size()-1] == '\\')
        {
            std::cout << "error: name " << quote(name) << " cannot end in '/' or '\\'" << std::endl;
            return 1;
        }
        else if(
                    (unquote(name).find('"') != std::string::npos && unquote(name).find('\'') != std::string::npos) ||
                    (unquote(title.str).find('"') != std::string::npos && unquote(title.str).find('\'') != std::string::npos) ||
                    (unquote(templatePath.str()).find('"') != std::string::npos && unquote(templatePath.str()).find('\'') != std::string::npos)
                )
        {
            std::cout << "error: names, titles and template paths cannot contain both single and double quotes" << std::endl;
            return 1;
        }

        if(cExt == "" || cExt[0] != '.')
        {
            std::cout << "error: track-from-file: content extension " << quote(cExt) << " must start with a fullstop" << std::endl;
            return 1;
        }
        else if(oExt == "" || oExt[0] != '.')
        {
            std::cout << "error: track-from-file: output extension " << quote(oExt) << " must start with a fullstop" << std::endl;
            return 1;
        }

        newInfo = make_info(name, title, templatePath);

        if(cExt != contentExt)
            newInfo.contentPath.file = newInfo.contentPath.file.substr(0, newInfo.contentPath.file.find_first_of('.')) + cExt;

        if(oExt != outputExt)
            newInfo.outputPath.file = newInfo.outputPath.file.substr(0, newInfo.outputPath.file.find_first_of('.')) + oExt;

        if(newInfo.contentPath == newInfo.templatePath)
        {
            std::cout << "error: content and template paths cannot be the same, " << newInfo.name << " not tracked" << std::endl;
            return 1;
        }

        if(trackedAll.count(newInfo) > 0)
        {
            TrackedInfo cInfo = *(trackedAll.find(newInfo));

            std::cout << "error: Nift is already tracking " << newInfo.outputPath << std::endl;
            std::cout << "----- current tracked details -----" << std::endl;
            std::cout << "        title: " << cInfo.title << std::endl;
            std::cout << "  output path: " << cInfo.outputPath << std::endl;
            std::cout << " content path: " << cInfo.contentPath << std::endl;
            std::cout << "template path: " << cInfo.templatePath << std::endl;
            std::cout << "--------------------------------" << std::endl;

            return 1;
        }

        //throws error if template path doesn't exist
        if(!std::ifstream(newInfo.templatePath.str()))
        {
            std::cout << "error: template path " << newInfo.templatePath << " does not exist" << std::endl;
            return 1;
        }

        if(std::ifstream(newInfo.outputPath.str()))
        {
            std::cout << "error: new output path " << newInfo.outputPath << " already exists" << std::endl;
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
            std::ofstream ofs(extPath.str());
            ofs << cExt << std::endl;
            ofs.close();
        }

        if(oExt != outputExt)
        {
            Path extPath = newInfo.outputPath.getInfoPath();
            extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
            std::ofstream ofs(extPath.str());
            ofs << oExt << std::endl;
            ofs.close();
        }

        if(!std::ifstream(newInfo.contentPath.str()))
            newInfo.contentPath.ensureFileExists();

        //ensures directories for output file and info file exist
        newInfo.outputPath.ensureDirExists();
        newInfo.outputPath.getInfoPath().ensureDirExists();

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

int ProjectInfo::track_dir(const Path& dirPath, const std::string& cExt, const Path& templatePath, const std::string& oExt)
{
    if(dirPath.file != "")
    {
        std::cout << "error: track-dir: directory path " << dirPath << " is to a file not a directory" << std::endl;
        return 1;
    }
    else if(dirPath.comparableStr().substr(0, contentDir.size()) != contentDir)
    {
        std::cout << "error: track-dir: cannot track files from directory " << dirPath << " as it is not a subdirectory of the project-wide content directory " << quote(contentDir) << std::endl;
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
    else if(oExt == "" || oExt[0] != '.')
    {
        std::cout << "error: track-dir: output extension " << quote(oExt) << " must start with a fullstop" << std::endl;
        return 1;
    }

    if(!std::ifstream(templatePath.str()))
    {
        std::cout << "error: track-dir: template path " << templatePath << " does not exist" << std::endl;
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

int ProjectInfo::track(const Name& name, const Title& title, const Path& templatePath, const std::string& cExt, const std::string& oExt)
{
    if(name.find('.') != std::string::npos)
    {
        std::cout << "error: names cannot contain '.'" << std::endl;
        std::cout << "note: you can add post-build scripts to move built/output files to paths containing . other than for extensions if you want" << std::endl;
        return 1;
    }
    else if(name == "" || title.str == "" || templatePath == Path("", "") || cExt == "" || oExt == "")
    {
        std::cout << "error: name, title, template path, content extension and output extension must all be non-empty strings" << std::endl;
        return 1;
    }
    if(unquote(name)[unquote(name).size()-1] == '/' || unquote(name)[unquote(name).size()-1] == '\\')
    {
        std::cout << "error: name " << quote(name) << " cannot end in '/' or '\\'" << std::endl;
        return 1;
    }
    else if(
                (unquote(name).find('"') != std::string::npos && unquote(name).find('\'') != std::string::npos) ||
                (unquote(title.str).find('"') != std::string::npos && unquote(title.str).find('\'') != std::string::npos) ||
                (unquote(templatePath.str()).find('"') != std::string::npos && unquote(templatePath.str()).find('\'') != std::string::npos)
            )
    {
        std::cout << "error: name, title and template path cannot contain both single and double quotes" << std::endl;
        return 1;
    }

    if(cExt == "" || cExt[0] != '.')
    {
        std::cout << "error: track: content extension " << quote(cExt) << " must start with a fullstop" << std::endl;
        return 1;
    }
    else if(oExt == "" || oExt[0] != '.')
    {
        std::cout << "error: track: output extension " << quote(oExt) << " must start with a fullstop" << std::endl;
        return 1;
    }

    TrackedInfo newInfo = make_info(name, title, templatePath);

    if(cExt != contentExt)
        newInfo.contentPath.file = newInfo.contentPath.file.substr(0, newInfo.contentPath.file.find_first_of('.')) + cExt;

    if(oExt != outputExt)
        newInfo.outputPath.file = newInfo.outputPath.file.substr(0, newInfo.outputPath.file.find_first_of('.')) + oExt;

    if(newInfo.contentPath == newInfo.templatePath)
    {
        std::cout << "error: content and template paths cannot be the same, " << newInfo.name << " not tracked" << std::endl;
        return 1;
    }

    if(trackedAll.count(newInfo) > 0)
    {
        TrackedInfo cInfo = *(trackedAll.find(newInfo));

        std::cout << "error: Nift is already tracking " << newInfo.outputPath << std::endl;
        std::cout << "----- current tracked details -----" << std::endl;
        std::cout << "        title: " << cInfo.title << std::endl;
        std::cout << "  output path: " << cInfo.outputPath << std::endl;
        std::cout << " content path: " << cInfo.contentPath << std::endl;
        std::cout << "template path: " << cInfo.templatePath << std::endl;
        std::cout << "--------------------------------" << std::endl;

        return 1;
    }

    //throws error if template path doesn't exist
    if(!std::ifstream(newInfo.templatePath.str()))
    {
        std::cout << "error: template path " << newInfo.templatePath << " does not exist" << std::endl;
        return 1;
    }

    if(std::ifstream(newInfo.outputPath.str()))
    {
        std::cout << "error: new output path " << newInfo.outputPath << " already exists" << std::endl;
        return 1;
    }

    if(cExt != contentExt)
    {
        Path extPath = newInfo.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
        std::ofstream ofs(extPath.str());
        ofs << cExt << std::endl;
        ofs.close();
    }

    if(oExt != outputExt)
    {
        Path extPath = newInfo.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
        std::ofstream ofs(extPath.str());
        ofs << oExt << std::endl;
        ofs.close();
    }

    if(!std::ifstream(newInfo.contentPath.str()))
    {
        newInfo.contentPath.ensureFileExists();
        //chmod(newInfo.contentPath.str().c_str(), 0666);
    }

    //ensures directories for output file and info file exist
    newInfo.outputPath.ensureDirExists();
    newInfo.outputPath.getInfoPath().ensureDirExists();

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
            std::cout << "error: name " << quote(name) << " cannot contain '.'" << std::endl;
            std::cout << "note: you can add post-build scripts to move built/output files to paths containing . other than for extensions if you want" << std::endl;
            return 1;
        }
        else if(name == "" || title.str == "" || templatePath == Path("", "") || cExt == "" || oExt == "")
        {
            std::cout << "error: names, titles, template paths, content extensions and output extensions must all be non-empty strings" << std::endl;
            return 1;
        }
        if(unquote(name)[unquote(name).size()-1] == '/' || unquote(name)[unquote(name).size()-1] == '\\')
        {
            std::cout << "error: name " << quote(name) << " cannot end in '/' or '\\'" << std::endl;
            return 1;
        }
        else if(
                    (unquote(name).find('"') != std::string::npos && unquote(name).find('\'') != std::string::npos) ||
                    (unquote(title.str).find('"') != std::string::npos && unquote(title.str).find('\'') != std::string::npos) ||
                    (unquote(templatePath.str()).find('"') != std::string::npos && unquote(templatePath.str()).find('\'') != std::string::npos)
                )
        {
            std::cout << "error: names, titles and template paths cannot contain both single and double quotes" << std::endl;
            return 1;
        }

        if(cExt == "" || cExt[0] != '.')
        {
            std::cout << "error: track-dir: content extension " << quote(cExt) << " must start with a fullstop" << std::endl;
            return 1;
        }
        else if(oExt == "" || oExt[0] != '.')
        {
            std::cout << "error: track-dir: output extension " << quote(oExt) << " must start with a fullstop" << std::endl;
            return 1;
        }

        TrackedInfo newInfo = make_info(name, title, templatePath);

        if(cExt != contentExt)
            newInfo.contentPath.file = newInfo.contentPath.file.substr(0, newInfo.contentPath.file.find_first_of('.')) + cExt;

        if(oExt != outputExt)
            newInfo.outputPath.file = newInfo.outputPath.file.substr(0, newInfo.outputPath.file.find_first_of('.')) + oExt;

        if(newInfo.contentPath == newInfo.templatePath)
        {
            std::cout << "error: content and template paths cannot be the same, not tracked" << std::endl;
            return 1;
        }

        if(trackedAll.count(newInfo) > 0)
        {
            TrackedInfo cInfo = *(trackedAll.find(newInfo));

            std::cout << "error: Nift is already tracking " << newInfo.outputPath << std::endl;
            std::cout << "----- current tracked details -----" << std::endl;
            std::cout << "        title: " << cInfo.title << std::endl;
            std::cout << "  output path: " << cInfo.outputPath << std::endl;
            std::cout << " content path: " << cInfo.contentPath << std::endl;
            std::cout << "template path: " << cInfo.templatePath << std::endl;
            std::cout << "--------------------------------" << std::endl;

            return 1;
        }

        //throws error if template path doesn't exist
        if(!std::ifstream(newInfo.templatePath.str()))
        {
            std::cout << "error: template path " << newInfo.templatePath << " does not exist" << std::endl;
            return 1;
        }

        if(std::ifstream(newInfo.outputPath.str()))
        {
            std::cout << "error: new output path " << newInfo.outputPath << " already exists" << std::endl;
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
            std::ofstream ofs(extPath.str());
            ofs << cExt << std::endl;
            ofs.close();
        }

        if(oExt != outputExt)
        {
            newInfo.outputPath.file = newInfo.outputPath.file.substr(0, newInfo.outputPath.file.find_first_of('.')) + oExt;

            Path extPath = newInfo.outputPath.getInfoPath();
            extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
            std::ofstream ofs(extPath.str());
            ofs << oExt << std::endl;
            ofs.close();
        }

        if(!std::ifstream(newInfo.contentPath.str()))
        {
            newInfo.contentPath.ensureFileExists();
            //chmod(newInfo.contentPath.str().c_str(), 0666);
        }

        //ensures directories for output file and info file exist
        newInfo.outputPath.ensureDirExists();
        newInfo.outputPath.getInfoPath().ensureDirExists();

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
        std::cout << "error: Nift is not tracking " << nameToUntrack << std::endl;
        return 1;
    }

    TrackedInfo toErase = get_info(nameToUntrack);

    //removes the extension files if they exist
    Path extPath = toErase.outputPath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
    if(std::ifstream(extPath.str()))
    {
        chmod(extPath.str().c_str(), 0666);
        remove_path(extPath);
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
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

    //removes info file and containing dirs if now empty
    if(std::ifstream(toErase.outputPath.getInfoPath().dir))
    {
        chmod(toErase.outputPath.getInfoPath().str().c_str(), 0666);
        remove_path(toErase.outputPath.getInfoPath());
    }

    //removes output file and containing dirs if now empty
    if(std::ifstream(toErase.outputPath.dir))
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
            std::cout << "error: Nift is not tracking " << namesToUntrack[p] << std::endl;
            return 1;
        }
    }

    for(size_t p=0; p<namesToUntrack.size(); p++)
    {
        toErase = get_info(namesToUntrack[p]);

        //removes the extension files if they exist
        Path extPath = toErase.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
        if(std::ifstream(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
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

        //removes info file and containing dirs if now empty
        if(std::ifstream(toErase.outputPath.getInfoPath().dir))
        {
            chmod(toErase.outputPath.getInfoPath().str().c_str(), 0666);
            remove_path(toErase.outputPath.getInfoPath());
        }

        //removes output file and containing dirs if now empty
        if(std::ifstream(toErase.outputPath.dir))
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
    if(!std::ifstream(filePath))
    {
        std::cout << "error: untrack-from-file: untrack-file " << quote(filePath) << " does not exist" << std::endl;
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
            std::cout << "error: Nift is not tracking " << nameToUntrack << std::endl;
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
        if(std::ifstream(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
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

        //removes info file and containing dirs if now empty
        if(std::ifstream(toErase.outputPath.getInfoPath().dir))
        {
            chmod(toErase.outputPath.getInfoPath().str().c_str(), 0666);
            remove_path(toErase.outputPath.getInfoPath());
        }

        //removes output file and containing dirs if now empty
        if(std::ifstream(toErase.outputPath.dir))
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
        std::cout << "error: untrack-dir: directory path " << dirPath << " is to a file not a directory" << std::endl;
        return 1;
    }
    else if(dirPath.comparableStr().substr(0, contentDir.size()) != contentDir)
    {
        std::cout << "error: untrack-dir: cannot untrack from directory " << dirPath << " as it is not a subdirectory of the project-wide content directory " << quote(contentDir) << std::endl;
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
    if(!std::ifstream(filePath))
    {
        std::cout << "error: rm-from-file: rm-file " << quote(filePath) << " does not exist" << std::endl;
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
            std::cout << "error: Nift is not tracking " << nameToRemove << std::endl;
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
        if(std::ifstream(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
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

        //removes info file and containing dirs if now empty
        if(std::ifstream(toErase.outputPath.getInfoPath().dir))
        {
            chmod(toErase.outputPath.getInfoPath().str().c_str(), 0666);
            remove_path(toErase.outputPath.getInfoPath());
        }

        //removes output file and containing dirs if now empty
        if(std::ifstream(toErase.outputPath.dir))
        {
            chmod(toErase.outputPath.str().c_str(), 0666);
            remove_path(toErase.outputPath);
        }

        //removes content file and containing dirs if now empty
        if(std::ifstream(toErase.contentPath.dir))
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
        std::cout << "error: rm-dir: directory path " << dirPath << " is to a file not a directory" << std::endl;
        return 1;
    }
    else if(dirPath.comparableStr().substr(0, contentDir.size()) != contentDir)
    {
        std::cout << "error: rm-dir: cannot remove from directory " << dirPath << " as it is not a subdirectory of the project-wide content directory " << quote(contentDir) << std::endl;
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
        std::cout << "error: Nift is not tracking " << nameToRemove << std::endl;
        return 1;
    }

    TrackedInfo toErase = get_info(nameToRemove);

    //removes the extension files if they exist
    Path extPath = toErase.outputPath.getInfoPath();
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
    if(std::ifstream(extPath.str()))
    {
        chmod(extPath.str().c_str(), 0666);
        remove_path(extPath);
    }
    extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
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

    //removes info file and containing dirs if now empty
    if(std::ifstream(toErase.outputPath.getInfoPath().dir))
    {
        chmod(toErase.outputPath.getInfoPath().str().c_str(), 0666);
        remove_path(toErase.outputPath.getInfoPath());
    }

    //removes output file and containing dirs if now empty
    if(std::ifstream(toErase.outputPath.dir))
    {
        chmod(toErase.outputPath.str().c_str(), 0666);
        remove_path(toErase.outputPath);
    }

    //removes content file and containing dirs if now empty
    if(std::ifstream(toErase.contentPath.dir))
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
            std::cout << "error: Nift is not tracking " << namesToRemove[p] << std::endl;
            return 1;
        }
    }

    for(size_t p=0; p<namesToRemove.size(); p++)
    {
        toErase = get_info(namesToRemove[p]);

        //removes the extension files if they exist
        extPath = toErase.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".contExt";
        if(std::ifstream(extPath.str()))
        {
            chmod(extPath.str().c_str(), 0666);
            remove_path(extPath);
        }
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";
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

        //removes info file and containing dirs if now empty
        if(std::ifstream(toErase.outputPath.getInfoPath().dir))
        {
            chmod(toErase.outputPath.getInfoPath().str().c_str(), 0666);
            remove_path(toErase.outputPath.getInfoPath());
        }

        //removes output file and containing dirs if now empty
        if(std::ifstream(toErase.outputPath.dir))
        {
            chmod(toErase.outputPath.str().c_str(), 0666);
            remove_path(toErase.outputPath);
        }

        //removes content file and containing dirs if now empty
        if(std::ifstream(toErase.contentPath.dir))
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
        std::cout << "error: new name cannot contain '.'" << std::endl;
        return 1;
    }
    else if(newName == "")
    {
        std::cout << "error: new name must be a non-empty string" << std::endl;
        return 1;
    }
    else if(unquote(newName).find('"') != std::string::npos && unquote(newName).find('\'') != std::string::npos)
    {
        std::cout << "error: new name cannot contain both single and double quotes" << std::endl;
        return 1;
    }
    else if(!tracking(oldName)) //checks old name is being tracked
    {
        std::cout << "error: Nift is not tracking " << oldName << std::endl;
        return 1;
    }
    else if(tracking(newName)) //checks new name isn't already tracked
    {
        std::cout << "error: Nift is already tracking " << newName << std::endl;
        return 1;
    }

    TrackedInfo oldInfo = get_info(oldName);

    Path oldExtPath = oldInfo.outputPath.getInfoPath();
    std::string cContExt = contentExt,
                cOutputExt = outputExt;

    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".contExt";
    if(std::ifstream(oldExtPath.str()))
    {
        std::ifstream ifs(oldExtPath.str());
        getline(ifs, cContExt);
        ifs.close();
    }
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".outputExt";
    if(std::ifstream(oldExtPath.str()))
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

    if(std::ifstream(newInfo.contentPath.str()))
    {
        std::cout << "error: new content path " << newInfo.contentPath << " already exists" << std::endl;
        return 1;
    }
    else if(std::ifstream(newInfo.outputPath.str()))
    {
        std::cout << "error: new output path " << newInfo.outputPath << " already exists" << std::endl;
        return 1;
    }

    //moves content file
    newInfo.contentPath.ensureDirExists();
    if(rename(oldInfo.contentPath.str().c_str(), newInfo.contentPath.str().c_str()))
    {
        std::cout << "error: mv: failed to move " << oldInfo.contentPath << " to " << newInfo.contentPath << std::endl;
        return 1;
    }

    //moves extension files if they exist
    Path newExtPath = newInfo.outputPath.getInfoPath();
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
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".outputExt";
    if(std::ifstream(oldExtPath.str()))
    {
        newExtPath.file = newExtPath.file.substr(0, newExtPath.file.find_first_of('.')) + ".outputExt";
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

    //removes info file and containing dirs if now empty
    if(std::ifstream(oldInfo.outputPath.getInfoPath().dir))
    {
        chmod(oldInfo.outputPath.getInfoPath().str().c_str(), 0666);
        remove_path(oldInfo.outputPath.getInfoPath());
    }

    //removes output file and containing dirs if now empty
    if(std::ifstream(oldInfo.outputPath.dir))
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
        std::cout << "error: new name cannot contain '.'" << std::endl;
        return 1;
    }
    else if(newName == "")
    {
        std::cout << "error: new name must be a non-empty string" << std::endl;
        return 1;
    }
    else if(unquote(newName).find('"') != std::string::npos && unquote(newName).find('\'') != std::string::npos)
    {
        std::cout << "error: new name cannot contain both single and double quotes" << std::endl;
        return 1;
    }
    else if(!tracking(trackedName)) //checks old name is being tracked
    {
        std::cout << "error: Nift is not tracking " << trackedName << std::endl;
        return 1;
    }
    else if(tracking(newName)) //checks new name isn't already tracked
    {
        std::cout << "error: Nift is already tracking " << newName << std::endl;
        return 1;
    }

    TrackedInfo trackedInfo = get_info(trackedName);

    Path oldExtPath = trackedInfo.outputPath.getInfoPath();
    std::string cContExt = contentExt,
                cOutputExt = outputExt;

    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".contExt";
    if(std::ifstream(oldExtPath.str()))
    {
        std::ifstream ifs(oldExtPath.str());
        getline(ifs, cContExt);
        ifs.close();
    }
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".outputExt";
    if(std::ifstream(oldExtPath.str()))
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

    if(std::ifstream(newInfo.contentPath.str()))
    {
        std::cout << "error: new content path " << newInfo.contentPath << " already exists" << std::endl;
        return 1;
    }
    else if(std::ifstream(newInfo.outputPath.str()))
    {
        std::cout << "error: new output path " << newInfo.outputPath << " already exists" << std::endl;
        return 1;
    }

    //copies content file
    if(cpFile(trackedInfo.contentPath.str(), newInfo.contentPath.str()))
    {
        std::cout << "error: cp: failed to copy " << trackedInfo.contentPath << " to " << newInfo.contentPath << std::endl;
        return 1;
    }

    //copies extension files if they exist
    Path newExtPath = newInfo.outputPath.getInfoPath();
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
    oldExtPath.file = oldExtPath.file.substr(0, oldExtPath.file.find_first_of('.')) + ".outputExt";
    if(std::ifstream(oldExtPath.str()))
    {
        newExtPath.file = newExtPath.file.substr(0, newExtPath.file.find_first_of('.')) + ".outputExt";
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
        std::cout << "error: new title must be a non-empty string" << std::endl;
        return 1;
    }
    else if(unquote(newTitle.str).find('"') != std::string::npos && unquote(newTitle.str).find('\'') != std::string::npos)
    {
        std::cout << "error: new title cannot contain both single and double quotes" << std::endl;
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
        std::cout << "error: Nift is not tracking " << quote(name) << std::endl;
        return 1;
    }

    return 0;
}

int ProjectInfo::new_template(const Path& newTemplatePath)
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
    save_config();

    //warns user if new template path doesn't exist
    if(!std::ifstream(newTemplatePath.str()))
        std::cout << "warning: new template path " << newTemplatePath << " does not exist" << std::endl;

    std::cout << "successfully changed default template to " << newTemplatePath << std::endl;

    return 0;
}

int ProjectInfo::new_template(const Name& name, const Path& newTemplatePath)
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

    TrackedInfo cInfo;
    cInfo.name = name;
    if(trackedAll.count(cInfo))
    {
        cInfo = *(trackedAll.find(cInfo));
        trackedAll.erase(cInfo);
        cInfo.templatePath = newTemplatePath;
        trackedAll.insert(cInfo);
        //saves new tracking list to .nsm/tracking.list
        save_tracking();

        //warns user if new template path doesn't exist
        if(!std::ifstream(newTemplatePath.str()))
            std::cout << "warning: new template path " << newTemplatePath << " does not exist" << std::endl;
    }
    else
    {
        std::cout << "error: Nift is not tracking " << name << std::endl;
        return 1;
    }

    return 0;
}

int ProjectInfo::new_output_dir(const Directory& newOutputDir)
{
    if(newOutputDir == "")
    {
        std::cout << "error: new output directory cannot be the empty string" << std::endl;
        return 1;
    }
    else if(unquote(newOutputDir).find('"') != std::string::npos && unquote(newOutputDir).find('\'') != std::string::npos)
    {
        std::cout << "error: new output directory cannot contain both single and double quotes" << std::endl;
        return 1;
    }

    if(newOutputDir[newOutputDir.size()-1] != '/' && newOutputDir[newOutputDir.size()-1] != '\\')
    {
        std::cout << "error: new output directory should end in \\ or /" << std::endl;
        return 1;
    }

    if(newOutputDir == outputDir)
    {
        std::cout << "error: output directory is already " << quote(newOutputDir) << std::endl;
        return 1;
    }

    if(std::ifstream(newOutputDir) || std::ifstream(newOutputDir.substr(0, newOutputDir.size()-1)))
    {
        std::cout << "error: new output directory location " << quote(newOutputDir) << " already exists" << std::endl;
        return 1;
    }

    std::string newInfoDir = ".nsm/" + newOutputDir;

    if(std::ifstream(newInfoDir) || std::ifstream(newInfoDir.substr(0, newInfoDir.size()-1)))
    {
        std::cout << "error: new info directory location " << quote(newInfoDir) << " already exists" << std::endl;
        return 1;
    }

    std::string parDir = "../",
                rootDir = "/",
                projectRootDir = get_pwd(),
                pwd = get_pwd();

    int ret_val = chdir(outputDir.c_str());
    if(ret_val)
    {
        std::cout << "error: ProjectInfo.cpp: new_output_dir(" << quote(newOutputDir) << "): failed to change directory to " << quote(outputDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    ret_val = chdir(parDir.c_str());
    if(ret_val)
    {
        std::cout << "error: ProjectInfo.cpp: new_output_dir(" << quote(newOutputDir) << "): failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    std::string delDir = get_pwd();
    ret_val = chdir(projectRootDir.c_str());
    if(ret_val)
    {
        std::cout << "error: ProjectInfo.cpp: new_output_dir(" << quote(newOutputDir) << "): failed to change directory to " << quote(projectRootDir) << " from " << quote(get_pwd()) << std::endl;
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
        std::cout << "error: ProjectInfo.cpp: new_output_dir(" << quote(newOutputDir) << "): failed to remove " << quote(delDir) << std::endl;
        return 1;
    }

    //changes back to project root directory
    ret_val = chdir(projectRootDir.c_str());
    if(ret_val)
    {
        std::cout << "error: ProjectInfo.cpp: new_output_dir(" << quote(newOutputDir) << "): failed to change directory to " << quote(projectRootDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    ret_val = chdir((".nsm/" + outputDir).c_str());
    if(ret_val)
    {
        std::cout << "error: ProjectInfo.cpp: new_output_dir(" << quote(newOutputDir) << "): failed to change directory to " << quote(".nsm/" + outputDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    ret_val = chdir(parDir.c_str());
    if(ret_val)
    {
        std::cout << "error: ProjectInfo.cpp: new_output_dir(" << quote(newOutputDir) << "): failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    delDir = get_pwd();
    ret_val = chdir(projectRootDir.c_str());
    if(ret_val)
    {
        std::cout << "error: ProjectInfo.cpp: new_output_dir(" << quote(newOutputDir) << "): failed to change directory to " << quote(projectRootDir) << " from " << quote(get_pwd()) << std::endl;
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
        std::cout << "error: ProjectInfo.cpp: new_output_dir(" << quote(newOutputDir) << "): failed to remove " << quote(delDir) << std::endl;
        return 1;
    }

    //changes back to project root directory
    ret_val = chdir(projectRootDir.c_str());
    if(ret_val)
    {
        std::cout << "error: ProjectInfo.cpp: new_output_dir(" << quote(newOutputDir) << "): failed to change directory to " << quote(projectRootDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    //sets new output directory
    outputDir = newOutputDir;

    //saves new output directory to Nift config file
    save_config();

    std::cout << "successfully changed output directory to " << quote(newOutputDir) << std::endl;

    return 0;
}

int ProjectInfo::new_content_dir(const Directory& newContDir)
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
        std::cout << "error: new content directory should end in \\ or /" << std::endl;
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
                projectRootDir = get_pwd(),
                pwd = get_pwd();

    int ret_val = chdir(contentDir.c_str());
    if(ret_val)
    {
        std::cout << "error: ProjectInfo.cpp: new_content_dir(" << quote(newContDir) << "): failed to change directory to " << quote(contentDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    ret_val = chdir(parDir.c_str());
    if(ret_val)
    {
        std::cout << "error: ProjectInfo.cpp: new_content_dir(" << quote(newContDir) << "): failed to change directory to " << quote(parDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    std::string delDir = get_pwd();
    ret_val = chdir(projectRootDir.c_str());
    if(ret_val)
    {
        std::cout << "error: ProjectInfo.cpp: new_content_dir(" << quote(newContDir) << "): failed to change directory to " << quote(projectRootDir) << " from " << quote(get_pwd()) << std::endl;
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
        std::cout << "error: ProjectInfo.cpp: new_content_dir(" << quote(newContDir) << "): failed to remove " << quote(delDir) << std::endl;
        return 1;
    }

    //changes back to project root directory
    ret_val = chdir(projectRootDir.c_str());
    if(ret_val)
    {
        std::cout << "error: ProjectInfo.cpp: new_content_dir(" << quote(newContDir) << "): failed to change directory to " << quote(projectRootDir) << " from " << quote(get_pwd()) << std::endl;
        return ret_val;
    }

    //sets new content directory
    contentDir = newContDir;

    //saves new content directory to Nift config file
    save_config();

    std::cout << "successfully changed content directory to " << quote(newContDir) << std::endl;

    return 0;
}

int ProjectInfo::new_content_ext(const std::string& newExt)
{
    if(newExt == "" || newExt[0] != '.')
    {
        std::cout << "error: content extension must start with a fullstop" << std::endl;
        return 1;
    }

    if(newExt == contentExt)
    {
        std::cout << "error: content extension is already " << quote(contentExt) << std::endl;
        return 1;
    }

    Path newContPath, extPath;

    for(auto trackedInfo=trackedAll.begin(); trackedInfo!=trackedAll.end(); trackedInfo++)
    {
        newContPath = trackedInfo->contentPath;
        newContPath.file = newContPath.file.substr(0, newContPath.file.find_first_of('.')) + newExt;

        if(std::ifstream(newContPath.str()))
        {
            std::cout << "error: new content path " << newContPath << " already exists" << std::endl;
            return 1;
        }
    }

    for(auto trackedInfo=trackedAll.begin(); trackedInfo!=trackedAll.end(); trackedInfo++)
    {
        //checks for non-default content extension
        extPath = trackedInfo->outputPath.getInfoPath();
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
            if(std::ifstream(trackedInfo->contentPath.str()))
            {
                newContPath = trackedInfo->contentPath;
                newContPath.file = newContPath.file.substr(0, newContPath.file.find_first_of('.')) + newExt;
                if(newContPath.str() != trackedInfo->contentPath.str())
                    rename(trackedInfo->contentPath.str().c_str(), newContPath.str().c_str());
            }
        }
    }

    contentExt = newExt;

    save_config();

    //informs user that content extension was successfully changed
    std::cout << "successfully changed content extension to " << quote(newExt) << std::endl;

    return 0;
}

int ProjectInfo::new_content_ext(const Name& name, const std::string& newExt)
{
    if(newExt == "" || newExt[0] != '.')
    {
        std::cout << "error: content extension must start with a fullstop" << std::endl;
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
            std::cout << "error: content extension for " << quote(name) << " is already " << quote(oldExt) << std::endl;
            return 1;
        }

        //throws error if new content file already exists
        newContPath = cInfo.contentPath;
        newContPath.file = newContPath.file.substr(0, newContPath.file.find_first_of('.')) + newExt;
        if(std::ifstream(newContPath.str()))
        {
            std::cout << "error: new content path " << newContPath << " already exists" << std::endl;
            return 1;
        }

        //moves the content file
        if(std::ifstream(cInfo.contentPath.str()))
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

            std::ofstream ofs(extPath.str());
            ofs << newExt << "\n";
            ofs.close();

            //makes sure user can't edit ext file
            chmod(extPath.str().c_str(), 0444);
        }
        else
        {
            if(remove_path(extPath))
            {
                std::cout << "error: faield to remove content extension path " << extPath << std::endl;
                return 1;
            }
        }
    }
    else
    {
        std::cout << "error: Nift is not tracking " << quote(name) << std::endl;
        return 1;
    }

    return 0;
}

int ProjectInfo::new_output_ext(const std::string& newExt)
{
    if(newExt == "" || newExt[0] != '.')
    {
        std::cout << "error: output extension must start with a fullstop" << std::endl;
        return 1;
    }

    if(newExt == outputExt)
    {
        std::cout << "error: output extension is already " << quote(outputExt) << std::endl;
        return 1;
    }

    Path newOutputPath, extPath;

    for(auto trackedInfo=trackedAll.begin(); trackedInfo!=trackedAll.end(); trackedInfo++)
    {
        newOutputPath = trackedInfo->outputPath;
        newOutputPath.file = newOutputPath.file.substr(0, newOutputPath.file.find_first_of('.')) + newExt;

        if(std::ifstream(newOutputPath.str()))
        {
            std::cout << "error: new output path " << newOutputPath << " already exists" << std::endl;
            return 1;
        }
    }

    for(auto trackedInfo=trackedAll.begin(); trackedInfo!=trackedAll.end(); trackedInfo++)
    {
        //checks for non-default output extension
        extPath = trackedInfo->outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".outputExt";

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
            //moves the output file
            if(std::ifstream(trackedInfo->outputPath.str()))
            {
                newOutputPath = trackedInfo->outputPath;
                newOutputPath.file = newOutputPath.file.substr(0, newOutputPath.file.find_first_of('.')) + newExt;
                if(newOutputPath.str() != trackedInfo->outputPath.str())
                    rename(trackedInfo->outputPath.str().c_str(), newOutputPath.str().c_str());
            }
        }
    }

    outputExt = newExt;

    save_config();

    //informs user that output extension was successfully changed
    std::cout << "successfully changed output extension to " << quote(newExt) << std::endl;

    return 0;
}

int ProjectInfo::new_output_ext(const Name& name, const std::string& newExt)
{
    if(newExt == "" || newExt[0] != '.')
    {
        std::cout << "error: output extension must start with a fullstop" << std::endl;
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
            std::cout << "error: output extension for " << quote(name) << " is already " << quote(oldExt) << std::endl;
            return 1;
        }

        //throws error if new output file already exists
        newOutputPath = cInfo.outputPath;
        newOutputPath.file = newOutputPath.file.substr(0, newOutputPath.file.find_first_of('.')) + newExt;
        if(std::ifstream(newOutputPath.str()))
        {
            std::cout << "error: new output path " << newOutputPath << " already exists" << std::endl;
            return 1;
        }

        //moves the output file
        if(std::ifstream(cInfo.outputPath.str()))
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

            std::ofstream ofs(extPath.str());
            ofs << newExt << "\n";
            ofs.close();

            //makes sure user can't edit ext file
            chmod(extPath.str().c_str(), 0444);
        }
        else
        {
            if(remove_path(extPath))
            {
                std::cout << "error: faield to remove output extension path " << extPath << std::endl;
                return 1;
            }
        }
    }
    else
    {
        std::cout << "error: Nift is not tracking " << quote(name) << std::endl;
        return 1;
    }

    return 0;
}

int ProjectInfo::new_script_ext(const std::string& newExt)
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

    Path extPath;

    for(auto trackedInfo=trackedAll.begin(); trackedInfo!=trackedAll.end(); trackedInfo++)
    {
        //checks for non-default script extension
        extPath = trackedInfo->outputPath.getInfoPath();
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

    scriptExt = newExt;

    save_config();

    //informs user that script extension was successfully changed
    std::cout << "successfully changed script extension to " << quote(newExt) << std::endl;

    return 0;
}

int ProjectInfo::new_script_ext(const Name& name, const std::string& newExt)
{
    if(newExt != "" && newExt[0] != '.')
    {
        std::cout << "error: non-empty script extension must start with a fullstop" << std::endl;
        return 1;
    }

    TrackedInfo cInfo = get_info(name);
    Path extPath;
    std::string oldExt;
    if(trackedAll.count(cInfo))
    {
        extPath = cInfo.outputPath.getInfoPath();
        extPath.file = extPath.file.substr(0, extPath.file.find_first_of('.')) + ".scriptExt";

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
            std::cout << "error: script extension for " << quote(name) << " is already " << quote(oldExt) << std::endl;
            return 1;
        }

        if(newExt != scriptExt)
        {
            //makes sure we can write to ext file
            chmod(extPath.str().c_str(), 0644);

            std::ofstream ofs(extPath.str());
            ofs << newExt << "\n";
            ofs.close();

            //makes sure user can't edit ext file
            chmod(extPath.str().c_str(), 0444);
        }
        else
        {
            if(remove_path(extPath))
            {
                std::cout << "error: faield to remove script extension path " << extPath << std::endl;
                return 1;
            }
        }
    }
    else
    {
        std::cout << "error: Nift is not tracking " << quote(name) << std::endl;
        return 1;
    }

    return 0;
}

int ProjectInfo::no_build_threads(int no_threads)
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

int ProjectInfo::check_watch_dirs()
{
    if(std::ifstream(".nsm/.watchinfo/watching.list"))
    {
        WatchList wl;
        if(wl.open())
        {
            std::cout << "error: failed to open watch list '.nsm/.watchinfo/watching.list'" << std::endl;
            return 1;
        }

        for(auto wd=wl.dirs.begin(); wd!=wl.dirs.end(); wd++)
        {
            std::string file,
                        watchDirFilesStr = ".nsm/.watchinfo/" + replace_slashes(wd->watchDir) + ".tracked";
            TrackedInfo cInfo;

            std::ifstream ifs(watchDirFilesStr);
            std::vector<Name> namesToRemove;
            while(read_quoted(ifs, file))
            {
                cInfo = get_info(file);
                if(cInfo.name != "##not-found##")
                    if(!std::ifstream(cInfo.contentPath.str()))
                        namesToRemove.push_back(file);
            }
            ifs.close();

            if(namesToRemove.size())
                rm(namesToRemove);

            std::vector<InfoToTrack> to_track;
            std::vector<Name> names_tracked;
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
                        if(!tracking(name))
                            to_track.push_back(InfoToTrack(name, Title(name), wd->templatePaths.at(ext), ext, wd->outputExts.at(ext)));

                        names_tracked.push_back(name);
                    }
                }
            }

            if(to_track.size())
                track(to_track);

            //makes sure we can write to watchDir tracked file
            chmod(watchDirFilesStr.c_str(), 0644);

            std::ofstream ofs(watchDirFilesStr);

            for(size_t f=0; f<names_tracked.size(); f++)
                ofs << quote(names_tracked[f]) << std::endl;

            ofs.close();

            //makes sure user can't accidentally write to watchDir tracked file
            chmod(watchDirFilesStr.c_str(), 0444);
        }
    }

    return 0;
}

std::mutex os_mtx;

int ProjectInfo::build_names(const std::vector<Name>& namesToBuild)
{
    check_watch_dirs();

    Parser parser(&trackedAll, &os_mtx, contentDir, outputDir, contentExt, outputExt, scriptExt, defaultTemplate, backupScripts, unixTextEditor, winTextEditor);
    std::set<Name> untrackedNames, failedNames;

    //checks for pre-build scripts
    if(run_script(std::cout, "pre-build" + scriptExt, backupScripts, &os_mtx3))
        return 1;

    for(auto name=namesToBuild.begin(); name != namesToBuild.end(); name++)
    {
        if(tracking(*name))
        {
            if(parser.build(get_info(*name), std::cout) > 0)
                failedNames.insert(*name);
        }
        else
            untrackedNames.insert(*name);
    }

    if(failedNames.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "---- following failed to build ----" << std::endl;
        for(auto fName=failedNames.begin(); fName != failedNames.end(); fName++)
            std::cout << " " << *fName << std::endl;
        std::cout << "-----------------------------------" << std::endl;
    }
    else if(untrackedNames.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "---- Nift not tracking the following ----" << std::endl;
        for(auto uName=untrackedNames.begin(); uName != untrackedNames.end(); uName++)
            std::cout << " " << *uName << std::endl;
        std::cout << "-----------------------------------------" << std::endl;
    }
    else if(failedNames.size() == 0 && untrackedNames.size() == 0)
    {
        std::cout << "all " << namesToBuild.size() << " files built successfully" << std::endl;

        //checks for post-build scripts
        if(run_script(std::cout, "post-build" + scriptExt, backupScripts, &os_mtx3))
            return 1;
    }

    return 0;
}

std::mutex fail_mtx, built_mtx, set_mtx;
std::set<Name> failedNames, builtNames;
std::set<TrackedInfo>::iterator nextInfo;

std::atomic<int> counter;

void build_thread(std::ostream& os,
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
    Parser parser(trackedAll, &os_mtx, ContentDir, OutputDir, ContentExt, OutputExt, ScriptExt, DefaultTemplate, BackupScripts, UnixTextEditor, WinTextEditor);
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

        if(parser.build(*cInfo, os) > 0)
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
    }
}

int ProjectInfo::build_all()
{
    int no_threads;
    if(buildThreads < 0)
        no_threads = -buildThreads*std::thread::hardware_concurrency();
    else
        no_threads = buildThreads;

    std::set<Name> untrackedNames;

    check_watch_dirs();

    //checks for pre-build scripts
    if(run_script(std::cout, "pre-build" + scriptExt, backupScripts, &os_mtx3))
        return 1;

    nextInfo = trackedAll.begin();
    counter = 0;

	std::vector<std::thread> threads;
	for(int i=0; i<no_threads; i++)
		threads.push_back(std::thread(build_thread, std::ref(std::cout), &trackedAll, trackedAll.size(), contentDir, outputDir, contentExt, outputExt, scriptExt, defaultTemplate, backupScripts, unixTextEditor, winTextEditor));

	for(int i=0; i<no_threads; i++)
		threads[i].join();

    if(failedNames.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "---- following failed to build ----" << std::endl;
        if(failedNames.size() < 20)
            for(auto fName=failedNames.begin(); fName != failedNames.end(); fName++)
                std::cout << " " << *fName << std::endl;
        else
        {
            int x=0;
            for(auto fName=failedNames.begin(); x < 20; fName++, x++)
                std::cout << " " << *fName << std::endl;
            std::cout << " along with " << failedNames.size() - 20 << " other names" << std::endl;
        }
        std::cout << "-----------------------------------" << std::endl;
    }
    else if(untrackedNames.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "---- Nift not tracking following names ----" << std::endl;
        if(untrackedNames.size() < 20)
            for(auto uName=untrackedNames.begin(); uName != untrackedNames.end(); uName++)
                std::cout << " " << *uName << std::endl;
        else
        {
            int x=0;
            for(auto uName=untrackedNames.begin(); x < 20; uName++, x++)
                std::cout << " " << *uName << std::endl;
            std::cout << " along with " << untrackedNames.size() - 20 << " other names" << std::endl;
        }
        std::cout << "-------------------------------------------" << std::endl;
    }
    else
    {
        std::cout << "all " << builtNames.size() << " output files built successfully" << std::endl;

        //checks for post-build scripts
        if(run_script(std::cout, "post-build" + scriptExt, backupScripts, &os_mtx3))
            return 1;
    }

    return 0;
}

std::mutex problem_mtx, updated_mtx, modified_mtx, removed_mtx;
std::set<Name> problemNames;
std::set<TrackedInfo> updatedInfo;
std::set<Path> modifiedFiles,
    removedFiles;

void dep_thread(std::ostream& os, const int& no_to_check, const Directory& contentDir, const Directory& outputDir, const std::string& contentExt, const std::string& outputExt)
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
        if(!std::ifstream(cInfo->contentPath.str()))
        {
            os_mtx.lock();
            os << cInfo->name << ": content file " << cInfo->contentPath << " does not exist" << std::endl;
            os_mtx.unlock();
            problem_mtx.lock();
            problemNames.insert(cInfo->name);
            problem_mtx.unlock();
            continue;
        }
        if(!std::ifstream(cInfo->templatePath.str()))
        {
            os_mtx.lock();
            os << cInfo->name << ": template file " << cInfo->templatePath << " does not exist" << std::endl;
            os_mtx.unlock();
            problem_mtx.lock();
            problemNames.insert(cInfo->name);
            problem_mtx.unlock();
            continue;
        }

        //gets path of info file from last time output file was built
        Path infoPath = cInfo->outputPath.getInfoPath();

        //checks whether info path exists
        if(!std::ifstream(infoPath.str()))
        {
            os_mtx.lock();
            os << cInfo->outputPath << ": yet to be built" << std::endl;
            os_mtx.unlock();
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
                os_mtx.lock();
                os << cInfo->outputPath << ": name changed to " << cInfo->name << " from " << prevInfo.name << std::endl;
                os_mtx.unlock();
                updated_mtx.lock();
                updatedInfo.insert(*cInfo);
                updated_mtx.unlock();
                continue;
            }

            if(cInfo->title != prevInfo.title)
            {
                os_mtx.lock();
                os << cInfo->outputPath << ": title changed to " << cInfo->title << " from " << prevInfo.title << std::endl;
                os_mtx.unlock();
                updated_mtx.lock();
                updatedInfo.insert(*cInfo);
                updated_mtx.unlock();
                continue;
            }

            if(cInfo->templatePath != prevInfo.templatePath)
            {
                os_mtx.lock();
                os << cInfo->outputPath << ": template path changed to " << cInfo->templatePath << " from " << prevInfo.templatePath << std::endl;
                os_mtx.unlock();
                updated_mtx.lock();
                updatedInfo.insert(*cInfo);
                updated_mtx.unlock();
                continue;
            }

            Path dep;
            while(dep.read_file_path_from(infoStream))
            {
                if(!std::ifstream(dep.str()))
                {
                    os_mtx.lock();
                    os << cInfo->outputPath << ": dep path " << dep << " removed since last build" << std::endl;
                    os_mtx.unlock();
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
                    os_mtx.lock();
                    os << cInfo->outputPath << ": dep path " << dep << " modified since last build" << std::endl;
                    os_mtx.unlock();
                    modified_mtx.lock();
                    modifiedFiles.insert(dep);
                    modified_mtx.unlock();
                    updated_mtx.lock();
                    updatedInfo.insert(*cInfo);
                    updated_mtx.unlock();
                    break;
                }
            }

			infoStream.close();

            //checks for user-defined dependencies
            Path depsPath = cInfo->contentPath;
            depsPath.file = depsPath.file.substr(0, depsPath.file.find_first_of('.')) + ".deps";

            if(std::ifstream(depsPath.str()))
            {
                std::ifstream depsFile(depsPath.str());
                while(dep.read_file_path_from(depsFile))
                {
                    if(!std::ifstream(dep.str()))
                    {
                        os_mtx.lock();
                        os << cInfo->outputPath << ": user defined dep path " << dep << " does not exist" << std::endl;
                        os_mtx.unlock();
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
                        os_mtx.lock();
                        os << cInfo->outputPath << ": user defined dep path " << dep << " modified since last build" << std::endl;
                        os_mtx.unlock();
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
    }
}

int ProjectInfo::build_updated(std::ostream& os)
{
    check_watch_dirs();

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
    counter = 0;

	std::vector<std::thread> threads;
	for(int i=0; i<no_threads; i++)
		threads.push_back(std::thread(dep_thread, std::ref(os), trackedAll.size(), contentDir, outputDir, contentExt, outputExt));

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
        os << "------- modified dependency files ------" << std::endl;
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
        os << "----------------------------------------" << std::endl;
    }

    if(problemNames.size() > 0)
    {
        os << std::endl;
        os << "----- missing content and/or template file -----" << std::endl;
        if(problemNames.size() < 20)
            for(auto pName=problemNames.begin(); pName != problemNames.end(); pName++)
                os << " " << *pName << std::endl;
        else
        {
            int x=0;
            for(auto pName=problemNames.begin(); x < 20; pName++, x++)
                os << " " << *pName << std::endl;
            os << " along with " << problemNames.size() - 20 << " other names" << std::endl;
        }
        os << "------------------------------------------------" << std::endl;
    }

    if(updatedInfo.size() > 0)
    {
        os << std::endl;
        os << "----- following need building -----" << std::endl;
        if(updatedInfo.size() < 20)
            for(auto uInfo=updatedInfo.begin(); uInfo != updatedInfo.end(); uInfo++)
                os << " " << uInfo->outputPath << std::endl;
        else
        {
            int x=0;
            for(auto uInfo=updatedInfo.begin(); x < 20; uInfo++, x++)
                os << " " << uInfo->outputPath << std::endl;
            os << " along with " << updatedInfo.size() - 20 << " more" << std::endl;
        }
        os << "-----------------------------------" << std::endl;

        //checks for pre-build scripts
        if(run_script(std::cout, "pre-build" + scriptExt, backupScripts, &os_mtx3))
            return 1;

        nextInfo = updatedInfo.begin();
        counter = 0;

        threads.clear();
        for(int i=0; i<no_threads; i++)
            threads.push_back(std::thread(build_thread, std::ref(os), &trackedAll, updatedInfo.size(), contentDir, outputDir, contentExt, outputExt, scriptExt, defaultTemplate, backupScripts, unixTextEditor, winTextEditor));

        for(int i=0; i<no_threads; i++)
            threads[i].join();

        if(builtNames.size() > 0)
        {
            os << std::endl;
            os << "------- output files successfully built -------" << std::endl;
            if(builtNames.size() < 20)
                for(auto bName=builtNames.begin(); bName != builtNames.end(); bName++)
                    os << " " << *bName << std::endl;
            else
            {
                int x=0;
                for(auto bName=builtNames.begin(); x < 20; bName++, x++)
                    os << " " << *bName << std::endl;
                os << " along with " << builtNames.size() - 20 << " other output files" << std::endl;
            }
            os << "-----------------------------------------------" << std::endl;
        }

        if(failedNames.size() > 0)
        {
            os << std::endl;
            os << "---- following failed to build ----" << std::endl;
            if(failedNames.size() < 20)
                for(auto fName=failedNames.begin(); fName != failedNames.end(); fName++)
                    os << " " << *fName << std::endl;
            else
            {
                int x=0;
                for(auto fName=failedNames.begin(); x < 20; fName++, x++)
                    os << " " << *fName << std::endl;
                os << " along with " << failedNames.size() - 20 << " other output files" << std::endl;
            }
            os << "-----------------------------------" << std::endl;
        }
        else
        {
            //checks for post-build scripts
            if(run_script(std::cout, "post-build" + scriptExt, backupScripts, &os_mtx3))
                return 1;
        }
    }

    if(updatedInfo.size() == 0 && problemNames.size() == 0 && failedNames.size() == 0)
    {
        //os << std::endl;
        os << "all " << trackedAll.size() << " output files are already up to date" << std::endl;
    }

	builtNames.clear();
	failedNames.clear();

    return 0;
}

int ProjectInfo::status()
{
    if(trackedAll.size() == 0)
    {
        std::cout << "Nift does not have anything tracked" << std::endl;
        return 0;
    }

    problemNames.clear();
    std::set<Path> updatedFiles, removedFiles;
    std::set<TrackedInfo> updatedInfo;

    for(auto trackedInfo=trackedAll.begin(); trackedInfo != trackedAll.end(); trackedInfo++)
    {
        bool needsUpdating = 0;

        //checks whether content and template files exist
        if(!std::ifstream(trackedInfo->contentPath.str()))
        {
            needsUpdating = 1; //shouldn't need this but doesn't cost much
            problemNames.insert(trackedInfo->name);
            //note user will be informed about this as a dep not existing
        }
        if(!std::ifstream(trackedInfo->templatePath.str()))
        {
            needsUpdating = 1; //shouldn't need this but doesn't cost much
            problemNames.insert(trackedInfo->name);
            //note user will be informed about this as a dep not existing
        }

        //gets path of info file from last time output file was built
        Path InfoPath = trackedInfo->outputPath.getInfoPath();

        //checks whether info path exists
        if(!std::ifstream(InfoPath.str()))
        {
            std::cout << trackedInfo->outputPath << ": yet to be built" << std::endl;
            needsUpdating = 1;
        }
        else
        {
            std::ifstream infoStream(InfoPath.str());
            std::string timeDateLine;
            Name prevName;
            Title prevTitle;
            Path prevTemplatePath;

            getline(infoStream, timeDateLine);
            read_quoted(infoStream, prevName);
            prevTitle.read_quoted_from(infoStream);
            prevTemplatePath.read_file_path_from(infoStream);

            //probably don't even need this
            TrackedInfo prevInfo = make_info(prevName, prevTitle, prevTemplatePath);

            if(trackedInfo->name != prevInfo.name)
            {
                std::cout << trackedInfo->outputPath << ": name changed to " << trackedInfo->name << " from " << prevInfo.name << std::endl;
                needsUpdating = 1;
            }

            if(trackedInfo->title != prevInfo.title)
            {
                std::cout << trackedInfo->outputPath << ": title changed to " << trackedInfo->title << " from " << prevInfo.title << std::endl;
                needsUpdating = 1;
            }

            if(trackedInfo->templatePath != prevInfo.templatePath)
            {
                std::cout << trackedInfo->outputPath << ": template path changed to " << trackedInfo->templatePath << " from " << prevInfo.templatePath << std::endl;
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
                else if(dep.modified_after(InfoPath))
                {
                    modifiedFiles.insert(dep);
                    needsUpdating = 1;
                }
            }

			infoStream.close();
        }

        if(needsUpdating && problemNames.count(trackedInfo->name) == 0)
            updatedInfo.insert(*trackedInfo);
    }

    if(removedFiles.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "---- removed dependency files ----" << std::endl;
        if(removedFiles.size() < 20)
            for(auto rFile=removedFiles.begin(); rFile != removedFiles.end(); rFile++)
                std::cout << " " << *rFile << std::endl;
        else
        {
            int x=0;
            for(auto rFile=removedFiles.begin(); x < 20; rFile++, x++)
                std::cout << " " << *rFile << std::endl;
            std::cout << " along with " << removedFiles.size() - 20 << " other dependency files" << std::endl;
        }
        std::cout << "----------------------------------" << std::endl;
    }

    if(modifiedFiles.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "------- modified dependency files ------" << std::endl;
        if(modifiedFiles.size() < 20)
            for(auto uFile=modifiedFiles.begin(); uFile != modifiedFiles.end(); uFile++)
                std::cout << " " << *uFile << std::endl;
        else
        {
            int x=0;
            for(auto uFile=modifiedFiles.begin(); x < 20; uFile++, x++)
                std::cout << " " << *uFile << std::endl;
            std::cout << " along with " << modifiedFiles.size() - 20 << " other dependency files" << std::endl;
        }
        std::cout << "----------------------------------------" << std::endl;
    }

    if(updatedInfo.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "----- following need building -----" << std::endl;
        if(updatedInfo.size() < 20)
            for(auto uInfo=updatedInfo.begin(); uInfo != updatedInfo.end(); uInfo++)
                std::cout << " " << uInfo->outputPath << std::endl;
        else
        {
            int x=0;
            for(auto uInfo=updatedInfo.begin(); x < 20; uInfo++, x++)
                std::cout << " " << uInfo->outputPath << std::endl;
            std::cout << " along with " << updatedInfo.size() - 20 << " more" << std::endl;
        }
        std::cout << "-----------------------------------" << std::endl;
    }

    if(problemNames.size() > 0)
    {
        std::cout << std::endl;
        std::cout << "----- missing content and/or template file -----" << std::endl;
        if(problemNames.size() < 20)
            for(auto pName=problemNames.begin(); pName != problemNames.end(); pName++)
                std::cout << " " << *pName << std::endl;
        else
        {
            int x=0;
            for(auto pName=problemNames.begin(); x < 20; pName++, x++)
                std::cout << " " << *pName << std::endl;
            std::cout << " along with " << problemNames.size() - 20 << " other names" << std::endl;
        }
        std::cout << "------------------------------------------------" << std::endl;
    }

    if(updatedFiles.size() == 0 && updatedInfo.size() == 0 && problemNames.size() == 0)
        std::cout << "all output files are already up to date" << std::endl;

    return 0;
}
