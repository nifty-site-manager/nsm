#ifndef QUOTED_H_
#define QUOTED_H_

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>

//reading a string which may be surrounded by quotes with spaces, and strips surrounding quotes
//note mutates white space into single spaces
bool read_quoted(std::istream &ifs, std::string &s);

//outputting a string with quotes surrounded if it contains space(s)
std::string quote(const std::string &unquoted);

//outputting a string with no quotes surrounded if it is quoted
std::string unquote(const std::string &quoted);

#endif //QUOTED_H_
