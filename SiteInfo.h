#ifndef SITE_INFO_H_
#define SITE_INFO_H_

#include <vector>

#include "PageBuilder.h"

struct SiteInfo
{
    std::set<PageInfo> pages;

    bool open();
    bool save();

    bool tracking(const PageInfo &page);
    bool tracking(const Path &pagePath);

    PageInfo get_tracked(const Path &pagePath);

    bool tracked();
    bool tracked_paths();

    bool track(const PageInfo &newPage);
    bool untrack(const Path &pagePathToUntrack);

    int build(std::vector<Path> pagePathsToBuild);
    int build_all();
    bool build_updated();

    bool status();
};

#endif //SITE_INFO_H_
