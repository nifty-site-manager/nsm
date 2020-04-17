#ifndef PROJECT_INFO_H_
#define PROJECT_INFO_H_

#include <thread>

#include "GitInfo.h"
#include "Parser.h"
#include "Timer.h"
#include "WatchList.h"

struct InfoToTrack
{
    Name name;
    Title title;
    Path templatePath;
    std::string contExt, outputExt;

    InfoToTrack(const Name &inName,
	            const Title &inTitle,
	            const Path &inTemplatePath,
	            const std::string& cExt,
	            const std::string& oExt)
    {
        name = inName;
        title = inTitle;
        templatePath = inTemplatePath;
        contExt = cExt;
        outputExt = oExt;
    }
};

int create_default_html_template(const Path& templatePath);
int create_blank_template(const Path& templatePath);
int create_config_file(const Path& configPath, 
                       const std::string& outputExt, 
                       bool global);
bool upgradeProject();

struct ProjectInfo
{
    Directory contentDir,
              outputDir;
    bool backupScripts;
    int buildThreads;
    std::string contentExt,
                outputExt,
                scriptExt,
                unixTextEditor,
                winTextEditor,
                rootBranch,
                outputBranch;
    Path defaultTemplate;
    std::set<TrackedInfo> trackedAll;
    std::mutex os_mtx3; //should this be removed?

    int open();
    int open_config(const Path& configPath, bool global);
    int open_global_config();
    int open_local_config();
    int open_tracking();

    int save_config(const std::string& configPathStr, bool global);
    int save_global_config();
    int save_local_config();
    int save_tracking();

    TrackedInfo make_info(const Name& name);
    TrackedInfo make_info(const Name& name, const Title& title);
    TrackedInfo make_info(const Name& name, 
                          const Title& title, 
                          const Path& templatePath);
    TrackedInfo get_info(const Name& name);
    int info(const std::vector<Name>& names);
    int info_all();
    int info_watching();
    int info_tracking();
    int info_names();

    std::string get_ext(const TrackedInfo& trackedInfo, 
                        const std::string& extType);
    std::string get_cont_ext(const TrackedInfo& trackedInfo);
    std::string get_output_ext(const TrackedInfo& trackedInfo);
    std::string get_script_ext(const TrackedInfo& trackedInfo);

    bool tracking(const TrackedInfo& trackedInfo);
    bool tracking(const Name& name);
    int track(const Name& name, 
              const Title& title, 
              const Path& templatePath);
    int track_from_file(const std::string& filePath);
    int track_dir(const Path& dirPath, 
                  const std::string& cExt, 
                  const Path& templatePath, 
                  const std::string& oExt);
    int track(const std::vector<InfoToTrack>& toTrack);
    int track(const Name& name, 
              const Title& title, 
              const Path& templatePath, 
              const std::string& cExt, 
              const std::string& oExt);
    int untrack(const Name& nameToUntrack);
    int untrack(const std::vector<Name>& namesToUntrack);
    int untrack_from_file(const std::string& filePath);
    int untrack_dir(const Path& dirPath, const std::string& cExt);
    int rm_from_file(const std::string& filePath);
    int rm_dir(const Path& dirPath, const std::string& cExt);
    int rm(const Name& nameToRemove);
    int rm(const std::vector<Name>& namesToRemove);
    int mv(const Name& oldName, const Name& newName);
    int cp(const Name& trackedName, const Name& newName);

    int new_title(const Name& name, const Title& newTitle);
    int new_template(const Path& newTemplatePath);
    int new_template(const Name& name, const Path& newTemplatePath);
    int new_output_dir(const Directory& newOutputDir);
    int new_content_dir(const Directory& newContDir);
    int new_content_ext(const std::string& newExt);
    int new_content_ext(const Name& name, const std::string& newExt);
    int new_output_ext(const std::string& newExt);
    int new_output_ext(const Name& name, const std::string& newExt);
    int new_script_ext(const std::string& newExt);
    int new_script_ext(const Name& name, const std::string& newExt);

    int no_build_threads(int noThreads);

    int check_watch_dirs();


    int build_untracked(std::ostream& os, 
                        const int& addBuildStatus, 
                        const std::set<TrackedInfo> infoToBuild);
    int build_names(std::ostream& os, 
                    const int& addBuildStatus, 
                    const std::vector<Name>& namesToBuild);
    int build_all(std::ostream& os, const int& addBuildStatus);
    int build_updated(std::ostream& os, 
                      const int& addBuildStatus, 
                      const bool& addExpl, 
                      const bool& basicOpt);

    int status(std::ostream& os, 
               const int& addBuildStatus, 
               const bool& addExpl, 
               const bool& basicOpt);
};

#endif //PROJECT_INFO_H_
