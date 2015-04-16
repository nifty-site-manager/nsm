#ifndef PAGE_BUILDER_H_
#define PAGE_BUILDER_H_

#include <sstream>
#include <set>

#include "PageInfo.h"
#include "DateTimeInfo.h"

struct PageBuilder
{
    PageInfo pageToBuild;
    DateTimeInfo dateTimeInfo;
    int codeBlockDepth,
        htmlCommentDepth;
    std::string indentAmount;
    bool contentAdded;
    std::stringstream processedPage;
    std::set<Path> pageDeps;

    int build(const PageInfo &pageInfo);
    int read_and_process(const Path &readPath, std::set<Path> antiDepsOfReadPath);
};

#endif //PAGE_BUILDER_H_
