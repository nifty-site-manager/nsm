#ifndef PAGE_BUILDER_H_
#define PAGE_BUILDER_H_

#include <sstream>
#include <set>

#include "PageInfo.h"
#include "DateTimeInfo.h"

struct PageBuilder
{
    std::set<PageInfo> pages;
    PageInfo pageToBuild;
    DateTimeInfo dateTimeInfo;
    int codeBlockDepth,
        htmlCommentDepth;
    std::string indentAmount;
    bool contentAdded;
    std::stringstream processedPage;
    std::set<Path> pageDeps;

    PageBuilder(const std::set<PageInfo> &Pages);

    int build(const PageInfo &pageInfo, std::ostream& os);
    int read_and_process(const Path &readPath, std::set<Path> antiDepsOfReadPath, std::ostream& os);
    int read_path(std::string &pathRead, size_t &linePos, const std::string &inLine, const Path &readPath, const int &lineNo, const std::string &callType, std::ostream& os);
	int read_sys_call(std::string &sys_call, size_t &linePos, const std::string &inLine, const Path &readPath, const int &lineNo, const std::string &callType, std::ostream& os);
};

#endif //PAGE_BUILDER_H_
