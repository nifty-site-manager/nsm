#ifndef SITE_INFO_H_
#define SITE_INFO_H_

#include <vector>

#include "PageBuilder.h"

struct SiteInfo
{
    std::set<PageInfo> pages;

    int open();
    int save();

    PageInfo get_info(const Path &pagePath);
    int info(const std::vector<Path> &pathsForInfo);
    int info_all();
    int info_paths();

    bool tracking(const PageInfo &page);
    bool tracking(const Path &pagePath);
    int track(const PageInfo &newPage);
    int untrack(const Path &pagePathToUntrack);

    int new_title(const Path &pagePath, const Title &newTitle);
    int new_page_path(const Path &oldpagePath, const Path &newPagePath);
    int new_content_path(const Path &pagePath, const Path &newContentPath);
    int new_template_path(const Path &pagePath, const Path &newTemplatePath);

    int build(std::vector<Path> pagePathsToBuild);
    int build_all();
    int build_updated();

    int status();
};

#endif //SITE_INFO_H_
