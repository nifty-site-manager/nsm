#ifndef PAGINATION_H_
#define PAGINATION_H_

#include "Path.h"

struct Pagination
{
    bool ftl; //first-to-last
    size_t noItemsPerPage,
           noPages;
    int callLineNo,
        separatorLineNo,
        templateCallLineNo,
        templateLineNo;
    size_t cPageNo;
    std::string pagesDir;
    std::string indentAmount,
                separator,
                templateStr;
    Path callPath;
    std::pair<std::string, std::string> splitFile;
    std::vector<std::string> items, pages;
    std::vector<int> itemLineNos, itemCallLineNos;
    std::vector<Path> itemCallPaths;

    Pagination();

    void reset();
};


#endif //PAGINATION_H_
