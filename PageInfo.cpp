#include "PageInfo.h"

//uses page name to order/compare PageInfo
bool operator<(const PageInfo &page1, const PageInfo &page2)
{
    return (page1.pageName < page2.pageName);
}

std::ostream& operator<<(std::ostream &os, const PageInfo &page)
{
    os << page.pageName << std::endl;
    os << page.pageTitle << std::endl;
    os << page.templatePath;

    return os;
}
