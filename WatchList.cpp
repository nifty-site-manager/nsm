#include "WatchList.h"

std::string replace_slashes(const std::string& source)
{
    std::string result = "";

    for(size_t i=0; i<source.size(); i++)
    {
        if(source[i] == '/' || source[i] == '\\')
            result += '|';
        else
            result += source[i];
    }

    return result;
}

WatchDir::WatchDir()
{
}

bool operator<(const WatchDir& wd1, const WatchDir& wd2)
{
    return (wd1.watchDir < wd2.watchDir);
}

WatchList::WatchList()
{
}

int WatchList::open()
{
    if(std::ifstream(".nsm/nift.config"))
    {
        if(std::ifstream(".nsm/.watchinfo/watching.list"))
        {
            WatchDir wd;
            std::string contExt;
            Path templatePath;

            std::ifstream ifs(".nsm/.watchinfo/watching.list");

            while(read_quoted(ifs, wd.watchDir))
            {
                wd.contExts.clear();
                wd.templatePaths.clear();
                wd.outputExts.clear();
                if(wd.watchDir != "" && wd.watchDir[0] != '#')
                {
                    std::string watchDirExtsFileStr = ".nsm/.watchinfo/" + replace_slashes(wd.watchDir) + ".exts";

                    if(std::ifstream(watchDirExtsFileStr))
                    {
                        std::ifstream ifxs(watchDirExtsFileStr);
                        while(read_quoted(ifxs, contExt))
                        {
                            if(contExt != "" && contExt[0] != '#')
                            {
                                if(wd.contExts.count(contExt))
                                {
                                    std::cout << "error: watched content extensions file " << quote(watchDirExtsFileStr) << " contains duplicate copies of content extension " << quote(contExt) << std::endl;
                                    return 1;
                                }
                                else
                                {
                                    wd.contExts.insert(contExt);
                                    wd.templatePaths[contExt].read_file_path_from(ifxs);
                                    read_quoted(ifxs, wd.outputExts[contExt]);
                                }
                            }
                        }
                        ifxs.close();
                    }
                    else
                    {
                        std::cout << "error: watched content extensions file " << quote(watchDirExtsFileStr) << " does not exist for watched directory" << std::endl;
                        return 1;
                    }

                    dirs.insert(wd);
                }
            }

            ifs.close();
        }
        else
        {
            //watch list isn't present, so technically already open
            /*std::cout << "error: cannot open watch list as '.nsm/watching.list' file does not exist" << std::endl;
            return 1;*/
        }
    }
    else
    {
        //no Nift configuration file found
        std::cout << "error: cannot open watch list as Nift configuration file '.nsm/nift.config' does not exist" << std::endl;
        return 1;
    }

    return 0;
}

int WatchList::save()
{
    if(std::ifstream(".nsm/nift.config"))
    {
        if(dirs.size())
        {
            Path(".nsm/.watchinfo/", "").ensureDirExists();
            std::ofstream ofs(".nsm/.watchinfo/watching.list");

            for(auto wd=dirs.begin(); wd!=dirs.end(); wd++)
            {
                ofs << quote(wd->watchDir) << "\n";

                std::string watchDirExtsFileStr = ".nsm/.watchinfo/" + replace_slashes(wd->watchDir) + ".exts";
                std::ofstream ofxs(watchDirExtsFileStr);
                for(auto ext=wd->contExts.begin(); ext!=wd->contExts.end(); ext++)
                    ofxs << *ext << " " << wd->templatePaths.at(*ext) << " " << wd->outputExts.at(*ext) << "\n";
                ofxs.close();
            }

            ofs.close();
        }
        else
        {
            remove_path(Path(".nsm/.watchinfo/", "watching.list"));
            //Path(".nsm/.watchinfo/", "").removePath();
        }
    }
    else
    {
        std::cout << "error: cannot save watch list to '.nsm/watching.list' as cannot find Nift configuration file '.nsm/nift.config'" << std::endl;
        return 1;
    }

    return 0;
}
