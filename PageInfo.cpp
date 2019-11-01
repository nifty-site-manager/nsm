#include "PageInfo.h"

std::string get_title(const Name &name)
{
    if(name.find_last_of('/') == std::string::npos)
        return name;

    return name.substr(name.find_last_of('/') + 1, name.length() - name.find_last_of('/') - 1);
}

//uses page name to order/compare PageInfo
bool operator<(const PageInfo &page1, const PageInfo &page2)
{
    return (quote(page1.pageName) < quote(page2.pageName));
}

std::ostream& operator<<(std::ostream &os, const PageInfo &page)
{
    os << quote(page.pageName) << "\n";
    os << page.pageTitle << "\n";
    os << page.templatePath;

    return os;
}
