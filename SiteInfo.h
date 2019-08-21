#ifndef SITE_INFO_H_
#define SITE_INFO_H_

#include <mutex>
#include <thread>
#include <vector>

#include "PageBuilder.h"

struct SiteInfo
{
    Directory contentDir,
              siteDir;
    std::string contentExt,
                pageExt;
    Path defaultTemplate;
    std::set<PageInfo> pages;

    int open();
    int save();

    PageInfo make_info(const Name &pageName);
    PageInfo make_info(const Name &pageName, const Title &pageTitle);
    PageInfo make_info(const Name &pageName, const Title &pageTitle, const Path &templatePath);
    PageInfo get_info(const Name &pageName);
    int info(const std::vector<Name> &pageNames);
    int info_all();
    int info_names();

    bool tracking(const PageInfo &page);
    bool tracking(const Name &pageName);
    int track(const Name &name, const Title &title, const Path &templatePath);
    int untrack(const Name &pageNameToUntrack);
    int rm(const Name &pageNameToRemove);
    int mv(const Name &oldPageName, const Name &newPageName);
    int cp(const Name &trackedPageName, const Name &newPageName);

    int new_title(const Name &pageName, const Title &newTitle);
    int new_template(const Name &pageName, const Path &newTemplatePath);

    int build(const std::vector<Name>& pageNamesToBuild);
    int build_all();
    int build_updated();
    int build_updated_serve();

    int status();
};

#endif //SITE_INFO_H_
