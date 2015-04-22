#ifndef PAGE_INFO_H_
#define PAGE_INFO_H_

#include "Path.h"
#include "Title.h"

typedef std::string Name;

struct PageInfo
{
    Title pageTitle;
    Name pageName;
    Path pagePath,
        contentPath,
        templatePath;
};

//output fn
std::ostream& operator<<(std::ostream &os, const PageInfo &page);

//uses page path to order/compare PageInfo
bool operator<(const PageInfo &page1, const PageInfo &page2);

#endif //PAGE_INFO_H_
