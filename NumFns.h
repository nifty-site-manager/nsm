#ifndef NUMFNS_H_
#define NUMFNS_H_

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>

bool isInt(const std::string& str);
bool isPosInt(const std::string& str);
bool isNonNegInt(const std::string& str);
bool isDouble(const std::string& str);
int getTypeInt(const std::string& str);
std::string int_to_timestr(int s);
std::string double_to_string_prec(const double& d, const int& prec);

#endif //NUMFNS_H_
