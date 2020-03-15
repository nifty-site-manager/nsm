#ifndef TITLE_H_
#define TITLE_H_

#include "ConsoleColor.h"
#include "Quoted.h"

struct Title
{
    std::string str;

	Title();
	Title(const std::string& Str);

    bool read_quoted_from(std::istream &is);

    Title& operator=(const std::string& Str);
};

//outputs title (quoted if it contains spaces)
std::ostream& operator<<(std::ostream &os, const Title &tit);
//reads a (possibly quoted) title from input stream
std::istream& operator>>(std::istream &is, Title &tit);

//equality relation
bool operator==(const Title &tit1, const Title &tit2);
//inequality relation
bool operator!=(const Title &tit1, const Title &tit2);

#endif //TITLE_H_
