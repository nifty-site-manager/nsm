#ifndef SITE_INFO_H_
#define SITE_INFO_H_

#include <cmath>
#include <math.h>
#include <mutex>
#include <thread>

#include <atomic>

#include "GitInfo.h"
#include "PageBuilder.h"

struct SiteInfo
{
    Directory contentDir,
              siteDir;
    int buildThreads;
    std::string contentExt,
                pageExt,
                scriptExt,
                unixTextEditor,
                winTextEditor,
                rootBranch,
                siteBranch;
    Path defaultTemplate;
    std::set<PageInfo> pages;

    int open();
    int open_config();
    int open_pages();
    int save_pages();
    int save_config();

    PageInfo make_info(const Name &pageName);
    PageInfo make_info(const Name &pageName, const Title &pageTitle);
    PageInfo make_info(const Name &pageName, const Title &pageTitle, const Path &templatePath);
    PageInfo get_info(const Name &pageName);
    int info(const std::vector<Name> &pageNames);
    int info_all();
    int info_names();

    std::string get_ext(const PageInfo& page, const std::string& extType);
    std::string get_cont_ext(const PageInfo& page);
    std::string get_page_ext(const PageInfo& page);
    std::string get_script_ext(const PageInfo& page);

    bool tracking(const PageInfo &page);
    bool tracking(const Name &pageName);
    int track(const Name &name, const Title &title, const Path &templatePath);
    int untrack(const Name &pageNameToUntrack);
    int rm(const Name &pageNameToRemove);
    int mv(const Name &oldPageName, const Name &newPageName);
    int cp(const Name &trackedPageName, const Name &newPageName);

    int new_title(const Name &pageName, const Title &newTitle);
    int new_template(const Path &newTemplatePath);
    int new_template(const Name &pageName, const Path &newTemplatePath);
    int new_site_dir(const Directory &newSiteDir);
    int new_content_dir(const Directory &newContDir);
    int new_content_ext(const std::string &newExt);
    int new_content_ext(const Name &pageName, const std::string &newExt);
    int new_page_ext(const std::string &newExt);
    int new_page_ext(const Name &pageName, const std::string &newExt);
    int new_script_ext(const std::string &newExt);
    int new_script_ext(const Name &pageName, const std::string &newExt);

    int no_build_threads(int noThreads);

    int build(const std::vector<Name>& pageNamesToBuild);
    int build_all();
    int build_updated(std::ostream& os);

    int status();
};

#endif //SITE_INFO_H_
