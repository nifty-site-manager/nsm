#include "Pagination.h"

Pagination::Pagination()
{
    reset();
}

void Pagination::reset()
{
    cPageNo = noPages = 0;
    noItemsPerPage = 25;
    pagesDir = "";
    callLineNo = separatorLineNo = templateCallLineNo = templateLineNo = -1;
    indentAmount = "";
    separator = "\n\n";
    templateStr = "$[paginate.page]";
    items.clear();
    pages.clear();
    itemLineNos.clear();
}
