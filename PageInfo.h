#ifndef PAGE_INFO_H_
#define PAGE_INFO_H_

#include "Location.h"
#include "Title.h"
#include "ReadAndProcess.h"


struct PageInfo
{
    Title pageTitle;
    Location pageLocation,
        contentLocation,
        templateLocation;

    PageInfo()
    {

    }

    PageInfo(const std::string PageTitle,
        const std::string PageLocation,
        const std::string ContentLocation,
        const std::string TemplateLocation)
    {
        pageTitle = PageTitle;
        pageLocation = PageLocation;
        contentLocation = ContentLocation;
        templateLocation = TemplateLocation;
    }

};

//uses page location to order/compare PageInfo
bool operator<(const PageInfo &page1, const PageInfo &page2)
{
    return (page1.pageLocation < page2.pageLocation);
}

std::istream& operator>>(std::istream &is, PageInfo &page)
{
    is >> page.pageTitle;
    is >> page.pageLocation;
    is >> page.contentLocation;
    is >> page.templateLocation;

    return is;
}

std::ostream& operator<<(std::ostream &os, const PageInfo &page)
{
    os << page.pageTitle << std::endl;
    os << page.pageLocation << std::endl;
    os << page.contentLocation << std::endl;
    os << page.templateLocation;

    return os;
};

bool build_page(const PageInfo &page)
{
    bool contentInserted = 0;

    std::cout << std::endl;

    //ensures content and template files exist
    if(!std::ifstream(STR(page.contentLocation)))
    {
        std::cout << "error: cannot build " << page.pageLocation << " as content file " << page.contentLocation << " does not exist" << std::endl;
        return 1;
    }
    if(!std::ifstream(STR(page.templateLocation)))
    {
        std::cout << "error: cannot build " << page.pageLocation << " as template file " << page.templateLocation << " does not exist." << std::endl;
        return 1;
    }

    std::cout << "building page " << page.pageLocation << std::endl;

    //gets fixed current date, time and timezone
    DateTimeInfo dateTimeInfo;

    //stream for reading processed page into
    //note don't want to start writing to file in case of an error
    std::stringstream processedPage;

    //set for recording page dependencies
    std::set<Location> pageDeps;

    //adds content and template location to dependencies
    pageDeps.insert(page.contentLocation);
    pageDeps.insert(page.templateLocation);

    //starts read_and_process from templateLocation
    std::string indentAmount = "";
    std::set<Location> antiDepsOfReadLocation;
    int codeBlockDepth = 0,
        htmlCommentDepth = 0;
    if(read_and_process(page.templateLocation, antiDepsOfReadLocation, page.contentLocation, page.pageTitle, dateTimeInfo, indentAmount, codeBlockDepth, htmlCommentDepth, contentInserted, processedPage, pageDeps) > 0)
        return 1;

    //ensures @pagecontent was found inside template tree
    if(!contentInserted)
    {
        std::cout << "error: @insertcontent not found within template file " << page.templateLocation << " or any of its dependencies, content from " << page.contentLocation << " has not been inserted" << std::endl;
        return 1;
    }

    std::string systemCall;

    //makes sure we can write to page file
    systemCall = "if [ -f " + STR(page.pageLocation) + " ]; then chmod +w " + STR(page.pageLocation) + "; fi";
    system(systemCall.c_str());

    std::ofstream pageStream(STR(page.pageLocation));
    pageStream << processedPage.str();
    pageStream.close();

    //makes sure user can't edit page file
    systemCall = "if [ -f " + STR(page.pageLocation) + " ]; then chmod -w " + STR(page.pageLocation) + "; fi";
    system(systemCall.c_str());

    //gets location for storing page information
    Location pageInfoLocation = getInfoLocation(page.pageLocation);

    //makes sure we can write to info file_
    systemCall = "if [ -f " + STR(pageInfoLocation) + " ]; then chmod +w " + STR(pageInfoLocation) + "; fi";
    system(systemCall.c_str());

    //writes dependencies to page info file
    std::ofstream infoStream(STR(pageInfoLocation));
    infoStream << page << std::endl;
    for(auto pageDep=pageDeps.begin(); pageDep != pageDeps.end(); pageDep++)
        infoStream << *pageDep << std::endl;
    infoStream.close();

    //makes sure user can't edit info file
    systemCall = "if [ -f " + STR(pageInfoLocation) + " ]; then chmod -w " + STR(pageInfoLocation) + "; fi";
    system(systemCall.c_str());

    std::cout << "page build successful" << std::endl;

    return 0;
};

#endif //PAGE_INFO_H_
