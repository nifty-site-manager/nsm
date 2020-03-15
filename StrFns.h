#ifndef STRFNS_H_
#define STRFNS_H_

#include <cstdio>
#include <iostream>
#include <vector>

bool is_whitespace(const std::string& str);
std::string into_whitespace(const std::string& str);
void strip_leading_line(std::string& str);
void strip_trailing_line(std::string& str);
void strip_leading_whitespace(std::string& str);
void strip_leading_whitespace_multiline(std::string& str);
void strip_trailing_whitespace(std::string& str);
void strip_trailing_whitespace_multiline(std::string& str);
void strip_surrounding_whitespace(std::string& str);
void strip_surrounding_whitespace_multiline(std::string& str);

std::string join(const std::vector<std::string>& vec, const std::string& str);

std::string findAndReplaceAll(const std::string& orig, const std::string& toSearch, const std::string& replaceStr);

#endif //STRFNS_H_
