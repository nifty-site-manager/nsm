#ifndef PAGE_INFO_H_
#define PAGE_INFO_H_

#include "Path.h"
#include "Title.h"

struct PageInfo
{
    Title pageTitle;
    Path pagePath,
        contentPath,
        templatePath;

    PageInfo();

    PageInfo(const std::string PageTitle,
        const std::string PagePath,
        const std::string ContentPath,
        const std::string TemplatePath);
};

//input fn
std::istream& operator>>(std::istream &is, PageInfo &page);
//output fn
std::ostream& operator<<(std::ostream &os, const PageInfo &page);

//uses page path to order/compare PageInfo
bool operator<(const PageInfo &page1, const PageInfo &page2);

#endif //PAGE_INFO_H_
