#include "Title.h"

Title& Title::operator=(const std::string Str)
{
    str = Str;
    return *this;
};

//outputs title (quoted if it contains spaces)
std::ostream& operator<<(std::ostream &os, const Title &tit)
{
    os << quote(tit.str);

    return os;
};

//reads a (possibly quoted) title from input stream
std::istream& operator>>(std::istream &is, Title &tit)
{
    read_quoted(is, tit.str);

    return is;
};

//equality relation
bool operator==(const Title &tit1, const Title &tit2)
{
    return (tit1.str == tit2.str);
};

//inequality relation
bool operator!=(const Title &tit1, const Title &tit2)
{
    return (tit1.str != tit2.str);
};

