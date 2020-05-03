#include "Title.h"

Title::Title()
{
	str = "";
}

Title::Title(const std::string& Str)
{
	str = Str;
}

Title& Title::operator=(const std::string& Str)
{
    str = Str;
    return *this;
}

bool Title::read_quoted_from(std::istream &is)
{
    return read_quoted(is, str);
}

//outputs title (quoted if it contains spaces)
std::ostream& operator<<(std::ostream &os, const Title &tit)
{
	os << c_light_blue << quote(tit.str) << c_white;

    return os;
}

//reads a (possibly quoted) title from input stream
std::istream& operator>>(std::istream &is, Title &tit)
{
    read_quoted(is, tit.str);

    return is;
}

//equality relation
bool operator==(const Title &tit1, const Title &tit2)
{
    return (tit1.str == tit2.str);
}

//inequality relation
bool operator!=(const Title &tit1, const Title &tit2)
{
    return (tit1.str != tit2.str);
}
