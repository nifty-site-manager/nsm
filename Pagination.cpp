#include "Pagination.h"

Pagination::Pagination()
{
    reset();
}

void Pagination::reset()
{
    cPageNo = noPages = 0;
    noItemsPerPage = 25;
    callLineNo = separatorLineNo = templateCallLineNo = templateLineNo = -1;
    indentAmount = paginateName = "";
    separator = "\n\n";
    templateStr = "$[paginate.page]";
    items.clear();
    pages.clear();
    itemLineNos.clear();
}
