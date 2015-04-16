#include "PageInfo.h"

PageInfo::PageInfo()
{

};

PageInfo::PageInfo(const std::string PageTitle,
    const std::string PagePath,
    const std::string ContentPath,
    const std::string TemplatePath)
{
    pageTitle = PageTitle;
    pagePath = PagePath;
    contentPath = ContentPath;
    templatePath = TemplatePath;
};


/*bool PageInfo::build() const
{
    bool contentAdded = 0;

    std::cout << std::endl;

    //ensures content and template files exist
    if(!std::ifstream(contentPath.str))
    {
        std::cout << "error: cannot build " << pagePath << " as content file " << contentPath << " does not exist" << std::endl;
        return 1;
    }
    if(!std::ifstream(templatePath.str))
    {
        std::cout << "error: cannot build " << pagePath << " as template file " << templatePath << " does not exist." << std::endl;
        return 1;
    }

    std::cout << "building page " << pagePath << std::endl;

    //gets fixed current date, time and timezone
    DateTimeInfo dateTimeInfo;

    //stream for reading processed page into
    //note don't want to start writing to file in case of an error
    std::stringstream processedPage;

    //set for recording page dependencies
    std::set<Path> pageDeps;

    //adds content and template paths to dependencies
    pageDeps.insert(contentPath);
    pageDeps.insert(templatePath);

    //starts read_and_process from templatePath
    std::string indentAmount = "";
    std::set<Path> antiDepsOfReadPath;
    int codeBlockDepth = 0,
        htmlCommentDepth = 0;
    if(read_and_process(templatePath, antiDepsOfReadPath, contentPath, pageTitle, dateTimeInfo, indentAmount, codeBlockDepth, htmlCommentDepth, contentAdded, processedPage, pageDeps) > 0)
        return 1;

    //ensures @pagecontent was found inside template tree
    if(!contentAdded)
    {
        std::cout << "error: @insertcontent not found within template file " << templatePath << " or any of its dependencies, content from " << contentPath << " has not been inserted" << std::endl;
        return 1;
    }

    std::string systemCall;

    //makes sure we can write to page file
    systemCall = "if [ -f " + pagePath.str + " ]; then chmod +w " + pagePath.str + "; fi";
    system(systemCall.c_str());

    std::ofstream pageStream(pagePath.str);
    pageStream << processedPage.str();
    pageStream.close();

    //makes sure user can't edit page file
    systemCall = "if [ -f " + pagePath.str + " ]; then chmod -w " + pagePath.str + "; fi";
    system(systemCall.c_str());

    //gets path for storing page information
    Path pageInfoPath = pagePath.getInfoPath();

    //makes sure we can write to info file_
    systemCall = "if [ -f " + pageInfoPath.str + " ]; then chmod +w " + pageInfoPath.str + "; fi";
    system(systemCall.c_str());

    //writes dependencies to page info file
    std::ofstream infoStream(pageInfoPath.str);
    infoStream << *this << std::endl;
    for(auto pageDep=pageDeps.begin(); pageDep != pageDeps.end(); pageDep++)
        infoStream << *pageDep << std::endl;
    infoStream.close();

    //makes sure user can't edit info file
    systemCall = "if [ -f " + pageInfoPath.str + " ]; then chmod -w " + pageInfoPath.str + "; fi";
    system(systemCall.c_str());

    std::cout << "page build successful" << std::endl;

    return 0;
};*/

//uses page path to order/compare PageInfo
bool operator<(const PageInfo &page1, const PageInfo &page2)
{
    return (page1.pagePath < page2.pagePath);
};

std::istream& operator>>(std::istream &is, PageInfo &page)
{
    is >> page.pageTitle;
    is >> page.pagePath;
    is >> page.contentPath;
    is >> page.templatePath;

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
