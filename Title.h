#ifndef TITLE_H_
#define TITLE_H_

#include <cstdio>
#include <iostream>
#include <fstream>
#include "Quoted.h"

class Title
{
    std::string str;

    public:
    Title& operator=(const std::string Str)
    {
        str = Str;
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& os, const Title& tit);
    friend std::istream& operator>>(std::istream &is, Title &tit);
    friend std::string getComparable(const Title &tit);
    //friend bool operator==(const Title &tit1, const Title &tit2);
    friend bool operator!=(const Title &tit1, const Title &tit2);
    //friend bool operator<(const Title &tit1, const Title &tit2);
    friend std::string STR(const Title &tit);
};

//outputs title (quoted if it contains spaces).
std::ostream& operator<<(std::ostream &os, const Title &tit)
{
    os << quote(tit.str);

    return os;
}

//reads a (possibly quoted) title from input stream
std::istream& operator>>(std::istream &is, Title &tit)
{
    read_quoted(is, tit.str);

    return is;
}

/*bool operator==(const Title &tit1, const Title &tit2)
{
    return (tit1.str == tit2.str);
}*/

bool operator!=(const Title &tit1, const Title &tit2)
{
    return (tit1.str != tit2.str);
}

/*bool operator<(const Title &tit1, const Title &tit2)
{
    return (tit1.str < tit2.str);
}*/

std::string STR(const Title &tit)
{
    return tit.str;
};

#endif //TITLE_H_
