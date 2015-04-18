#include "PageInfo.h"

PageInfo::PageInfo()
{

};

PageInfo::PageInfo(const std::string PageTitleStr,
    const std::string PagePathStr,
    const std::string ContentPathStr,
    const std::string TemplatePathStr)
{
    pageTitle = PageTitleStr;
    pagePath.set_file_path_from(PagePathStr);
    contentPath.set_file_path_from(ContentPathStr);
    templatePath.set_file_path_from(TemplatePathStr);
};

//uses page path to order/compare PageInfo
bool operator<(const PageInfo &page1, const PageInfo &page2)
{
    return (page1.pagePath < page2.pagePath);
};

std::istream& operator>>(std::istream &is, PageInfo &page)
{
    is >> page.pageTitle;
    page.pagePath.read_file_path_from(is);
    page.contentPath.read_file_path_from(is);
    page.templatePath.read_file_path_from(is);

    return is;
};

std::ostream& operator<<(std::ostream &os, const PageInfo &page)
{
    os << page.pageTitle << std::endl;
    os << page.pagePath << std::endl;
    os << page.contentPath << std::endl;
    os << page.templatePath;

    return os;
};
